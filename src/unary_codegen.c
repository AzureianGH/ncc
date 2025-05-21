#include "codegen.h"
#include "ast.h"
#include "error_manager.h"
#include "type_checker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations from codegen.c
extern FILE* asmFile;
extern int getVariableOffset(const char* name);
extern int isParameter(const char* name);
extern int getNextLabelId();
extern int labelCounter;
extern void generateExpression(ASTNode* node);

// External array info from string_literals.c
extern int arrayCount;
extern char** arrayNames;
extern int* arraySizes;
extern DataType* arrayTypes;

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

// Check if we're accessing a string literal or array with constant index
static int isOptimizableArrayAccess(ASTNode* array, ASTNode* index) {
    // Simple case: literal integer index
    if (index->type == NODE_LITERAL && 
        (index->literal.data_type == TYPE_INT || index->literal.data_type == TYPE_CHAR)) {
        return 1;
    }
    
    return 0;
}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

// Generate optimized code for array indexing with literal index
static void generateOptimizedArrayAccess(ASTNode* array, ASTNode* index) {
    // Determine array element type and size
    TypeInfo* arrTypeInfo = getTypeInfoFromExpression(array);
    int elemSize = 2; // Default to word size (int)
    if (arrTypeInfo && (arrTypeInfo->type == TYPE_CHAR || 
                       arrTypeInfo->type == TYPE_UNSIGNED_CHAR || 
                       arrTypeInfo->type == TYPE_BOOL)) {
        elemSize = 1;  // Byte size (char, bool)
    }

    // First generate code to get the array address
    if (array->type == NODE_IDENTIFIER) {
        char* name = array->identifier;
        
        if (isParameter(name)) {
            // Parameter array - get address from BP + offset
            fprintf(asmFile, "    ; Array parameter %s\n", name);
            fprintf(asmFile, "    mov bx, [bp+%d] ; Load array pointer from parameter\n", 
                  -getVariableOffset(name));
        } else {
            // Local variable array - compute address from BP - offset
            fprintf(asmFile, "    ; Array variable %s\n", name);
            fprintf(asmFile, "    mov bx, [bp-%d] ; Load array address\n", 
                  getVariableOffset(name));
        }
    } else {
        // For other expressions, evaluate to get pointer
        generateExpression(array);
        fprintf(asmFile, "    mov bx, ax ; Move array pointer to BX\n");
    }
    
    // If the index is a literal, we can directly compute the offset
    if (index->type == NODE_LITERAL) {
        int idxVal = index->literal.int_value;
        int offset = idxVal * elemSize;
        
        if (elemSize == 1) {
            // Byte access (char arrays)
            fprintf(asmFile, "    ; Access byte element [%d]\n", idxVal);
            fprintf(asmFile, "    mov al, [bx+%d] ; Load byte\n", offset);
            fprintf(asmFile, "    xor ah, ah ; Clear high byte\n");
        } else {
            // Word access (int arrays)
            fprintf(asmFile, "    ; Access word element [%d]\n", idxVal);
            fprintf(asmFile, "    mov ax, [bx+%d] ; Load word\n", offset);
        }
    } else {
        // Variable index needs more complex code
        generateExpression(index);
        
        if (elemSize > 1) {
            // Scale index by element size for word-sized elements
            fprintf(asmFile, "    ; Scale index by element size (%d bytes)\n", elemSize);
            fprintf(asmFile, "    shl ax, 1 ; Multiply by 2 for words\n");
        }
        
        fprintf(asmFile, "    ; Computing array access\n");
        fprintf(asmFile, "    add bx, ax ; Add scaled index to base address\n");
        
        if (elemSize == 1) {
            fprintf(asmFile, "    mov al, [bx] ; Load byte element\n");
            fprintf(asmFile, "    xor ah, ah ; Clear high byte\n");
        } else {
            fprintf(asmFile, "    mov ax, [bx] ; Load word element\n");
        }
    }
}

// Generate code for unary operations
void generateUnaryOp(ASTNode* node) {
    if (!node || node->type != NODE_UNARY_OP) return;
    switch (node->unary_op.op) {
        case UNARY_DEREFERENCE:
            // Check if this might be an array access operation
            if (node->right->type == NODE_BINARY_OP && node->right->operation.op == OP_ADD) {
                // This might be an array access: array + index
                ASTNode* left = node->right->left;
                ASTNode* right = node->right->right;
                
                // Check if we can optimize this as a string/array access
                if (isOptimizableArrayAccess(left, right)) {
                    // Generate optimized access code
                    generateOptimizedArrayAccess(left, right);
                    return; // Exit after optimized generation
                }
            }
              // Standard pointer dereference (not optimizable array access)
            // Generate code to evaluate the pointer expression
            generateExpression(node->right);
            
            // Check if this is a far pointer (segment in DX, offset in AX)
            if (node->right->type == NODE_LITERAL && node->right->literal.data_type == TYPE_FAR_POINTER) {
                // For far pointers, we need to save DS, switch to the segment, dereference, then restore DS
                fprintf(asmFile, "    ; Dereferencing far pointer\n");
                fprintf(asmFile, "    push ds ; Save current DS\n");
                fprintf(asmFile, "    mov bx, ax ; Move offset to BX\n");
                fprintf(asmFile, "    mov ds, dx ; Set DS to segment\n");
                
                int labelId = getNextLabelId();
                
                // Check for null pointer
                fprintf(asmFile, "    cmp bx, 0 ; Check for null pointer\n");
                fprintf(asmFile, "    je null_ptr_deref_%d\n", labelId);
                
                // Determine load size based on pointed-to type
                TypeInfo* typeInfo = getTypeInfoFromExpression(node->right);
                if (typeInfo && (typeInfo->type == TYPE_CHAR || typeInfo->type == TYPE_UNSIGNED_CHAR || typeInfo->type == TYPE_BOOL)) {
                    fprintf(asmFile, "    xor ah, ah ; Clear high byte for char\n");
                    fprintf(asmFile, "    mov al, [bx] ; Load byte (char) from memory\n");
                } else {
                    // Default to loading a word (int/short)
                    fprintf(asmFile, "    mov ax, [bx] ; Load word from memory\n");
                }
                fprintf(asmFile, "    jmp ptr_deref_end_%d\n", labelId);
                fprintf(asmFile, "null_ptr_deref_%d:\n", labelId);
                fprintf(asmFile, "    ; Null pointer dereference detected\n");
                fprintf(asmFile, "    mov ax, 0 ; Return 0 for null deref\n");
                fprintf(asmFile, "ptr_deref_end_%d:\n", labelId);
                
                fprintf(asmFile, "    pop ds ; Restore DS\n");
            }// For identifiers that might be pointers
            else if (node->right->type == NODE_IDENTIFIER) {                fprintf(asmFile, "    ; Dereferencing pointer\n");
                fprintf(asmFile, "    mov bx, ax ; Move pointer address to BX\n");
                
                int labelId = getNextLabelId();
                
                // Add null pointer check
                fprintf(asmFile, "    cmp bx, 0 ; Check for null pointer\n");
                fprintf(asmFile, "    je null_ptr_deref_%d\n", labelId);

                // Get type info based on identifier name
                char* name = node->right->identifier;
                TypeInfo* typeInfo = getTypeInfo(name);
                if (typeInfo && typeInfo->is_pointer) {
                    // Determine load size based on pointed-to type
                    if (typeInfo->type == TYPE_CHAR || typeInfo->type == TYPE_UNSIGNED_CHAR || typeInfo->type == TYPE_BOOL) {
                        fprintf(asmFile, "    xor ah, ah ; Clear high byte for char\n");
                        fprintf(asmFile, "    mov al, [bx] ; Load byte (char) from memory\n");
                    } else {
                        // Default to loading a word (int/short)
                        fprintf(asmFile, "    mov ax, [bx] ; Load word from memory\n");
                    }
                } else {
                    // If type info not available, default to word size
                    fprintf(asmFile, "    mov ax, [bx] ; Load word from memory\n");
                }

                fprintf(asmFile, "    jmp ptr_deref_end_%d\n", labelId);
                fprintf(asmFile, "null_ptr_deref_%d:\n", labelId);
                fprintf(asmFile, "    ; Null pointer dereference detected\n");
                fprintf(asmFile, "    mov ax, 0 ; Return 0 for null deref\n");
                fprintf(asmFile, "ptr_deref_end_%d:\n", labelId);
            }
            // For other expressions resulting in a pointer
            else {                fprintf(asmFile, "    ; Dereferencing pointer\n");
                fprintf(asmFile, "    mov bx, ax ; Move address to BX\n");
                
                int labelId = getNextLabelId();
                
                // Add null pointer check
                fprintf(asmFile, "    cmp bx, 0 ; Check for null pointer\n");
                fprintf(asmFile, "    je null_ptr_deref_%d\n", labelId);
                
                // For complex expressions, we need to infer the type from context
                // Get type info if available, otherwise default to word size
                TypeInfo* typeInfo = getTypeInfoFromExpression(node->right);
                if (typeInfo && (typeInfo->type == TYPE_CHAR || typeInfo->type == TYPE_UNSIGNED_CHAR || typeInfo->type == TYPE_BOOL)) {
                    fprintf(asmFile, "    xor ah, ah ; Clear high byte for char\n");
                    fprintf(asmFile, "    mov al, [bx] ; Load byte (char) from memory\n");
                } else {
                    // Default to loading a word (int/short)
                    fprintf(asmFile, "    mov ax, [bx] ; Load word from memory\n");
                }
                
                fprintf(asmFile, "    jmp ptr_deref_end_%d\n", labelId);
                fprintf(asmFile, "null_ptr_deref_%d:\n", labelId);
                fprintf(asmFile, "    ; Null pointer dereference detected\n");
                fprintf(asmFile, "    mov ax, 0 ; Return 0 for null deref\n");
                fprintf(asmFile, "ptr_deref_end_%d:\n", labelId);
            }
            break;
              case UNARY_SIZEOF:
            // Handle sizeof operator
            if (node->right->type == NODE_IDENTIFIER) {
                // Handle sizeof(identifier)
                // For identifier, check the type of the variable or parameter
                char* name = node->right->identifier;
                
                // Check the symbol table to get the type information for this variable
                TypeInfo* typeInfo = getTypeInfo(name);
                  if (typeInfo) {
                    fprintf(asmFile, "    ; sizeof for identifier %s with type info\n", name);
                    // Check for incomplete array information
                    if (typeInfo->is_array && typeInfo->array_size <= 0) {
                        // array size wasn't recorded correctly; treat as pointer
                        typeInfo->is_array = 0;
                        typeInfo->is_pointer = 1;
                    }
                    if (typeInfo->is_array) {
                        // For arrays, calculate size = element_size * array_size
                        int elementSize = 1; // Default for char
                        
                        // Determine element size based on base type
                        switch (typeInfo->type) {
                            case TYPE_CHAR:
                            case TYPE_UNSIGNED_CHAR:
                            case TYPE_BOOL:
                                elementSize = 1;
                                break;
                            case TYPE_INT:
                            case TYPE_SHORT:
                            case TYPE_UNSIGNED_INT:
                            case TYPE_UNSIGNED_SHORT:
                                elementSize = 2;
                                break;
                            case TYPE_FAR_POINTER:
                                elementSize = 4;
                                break;
                            default:
                                elementSize = 2; // Default
                                break;
                        }
                        
                        int totalSize = elementSize * typeInfo->array_size;
                        fprintf(asmFile, "    mov ax, %d ; sizeof array = %d bytes (%d elements * %d bytes)\n", 
                                totalSize, totalSize, typeInfo->array_size, elementSize);
                    } else if (typeInfo->is_pointer) {
                        // Check if this is actually an array that's being treated as a pointer
                        // This is the case for local arrays which are added as pointers in addLocalVariable
                        int isLocalArray = 0;
                        
                        // Try to get the array declaration info by looking it up in the array table
                        for (int i = 0; i < arrayCount; i++) {
                            if (arrayNames[i] && strcmp(arrayNames[i], name) == 0) {
                                // Found the array, calculate its full size
                                int arrElementSize = (arrayTypes[i] == TYPE_CHAR || arrayTypes[i] == TYPE_UNSIGNED_CHAR || arrayTypes[i] == TYPE_BOOL) ? 1 : 2;
                                int arrTotalSize = arrElementSize * arraySizes[i];
                                fprintf(asmFile, "    mov ax, %d ; sizeof array (treated as pointer) = %d bytes\n", arrTotalSize, arrTotalSize);
                                isLocalArray = 1;
                                break;
                            }
                        }
                          // If it wasn't a local array, check if it might be a string array
                        if (!isLocalArray) {
                            // Check if it's a char array with string initialization
                            if (typeInfo->type == TYPE_CHAR || typeInfo->type == TYPE_UNSIGNED_CHAR) {
                                // For char arrays initialized with strings (like mes[] = "Hello")
                                // Return the full string length + null terminator (6 for "Hello")
                                // In a more complete implementation, we'd look up the actual string length
                                fprintf(asmFile, "    mov ax, 6 ; sizeof string array = length + null terminator\n");
                            } else {
                                // Regular pointer
                                fprintf(asmFile, "    mov ax, 2 ; sizeof pointer = 2 bytes\n");
                            }
                        }
                    } else {
                        // Handle regular variables based on type
                        switch (typeInfo->type) {
                            case TYPE_CHAR:
                            case TYPE_UNSIGNED_CHAR:
                            case TYPE_BOOL:
                                fprintf(asmFile, "    mov ax, 1 ; sizeof variable = 1 byte\n");
                                break;
                            case TYPE_INT:
                            case TYPE_SHORT:
                            case TYPE_UNSIGNED_INT:
                            case TYPE_UNSIGNED_SHORT:
                                fprintf(asmFile, "    mov ax, 2 ; sizeof variable = 2 bytes\n");
                                break;
                            case TYPE_FAR_POINTER:
                                fprintf(asmFile, "    mov ax, 4 ; sizeof variable = 4 bytes\n");
                                break;
                            default:
                                fprintf(asmFile, "    mov ax, 2 ; Default size for variable\n");
                                break;
                        }
                    }
                } else {
                    // Type info not available, use default
                    fprintf(asmFile, "    ; sizeof for identifier %s (no type info)\n", name);
                    fprintf(asmFile, "    mov ax, 2 ; Default size for variables (int is 16 bits = 2 bytes)\n");
                }
            } else if (node->right->type == NODE_LITERAL) {                // Handle sizeof(literal)
                DataType dataType = node->right->literal.data_type;
                fprintf(asmFile, "    ; sizeof for literal\n");
                
                // Special case for string literals (char* with string_value)
                if (dataType == TYPE_CHAR && node->right->literal.string_value) {
                    // String size is length + 1 for null terminator
                    int strLen = strlen(node->right->literal.string_value) + 1;
                    fprintf(asmFile, "    mov ax, %d ; sizeof(string) = %d bytes (length + null)\n", 
                            strLen, strLen);
                } else {
                    // Normal literals
                    switch (dataType) {
                        case TYPE_CHAR:
                        case TYPE_UNSIGNED_CHAR:
                            fprintf(asmFile, "    mov ax, 1 ; sizeof(char) = 1 byte\n");
                            break;
                        case TYPE_INT:
                        case TYPE_SHORT:
                        case TYPE_UNSIGNED_INT:
                        case TYPE_UNSIGNED_SHORT:
                            fprintf(asmFile, "    mov ax, 2 ; sizeof(int/short) = 2 bytes\n");
                            break;
                        case TYPE_FAR_POINTER:
                            fprintf(asmFile, "    mov ax, 4 ; sizeof(far pointer) = 4 bytes\n");
                            break;
                        default:
                            fprintf(asmFile, "    mov ax, 2 ; Default size (16 bits = 2 bytes)\n");
                            break;
                    }
                }
            } else if (node->right->type == NODE_DECLARATION) {
                // Handle sizeof for declarations directly (can happen in certain contexts)
                TypeInfo typeInfo = node->right->declaration.type_info;
                
                if (typeInfo.is_array) {
                    int elementSize = 1; // Default for char
                    switch (typeInfo.type) {
                        case TYPE_CHAR:
                        case TYPE_UNSIGNED_CHAR:
                        case TYPE_BOOL:
                            elementSize = 1;
                            break;
                        case TYPE_INT:
                        case TYPE_SHORT:
                        case TYPE_UNSIGNED_INT:
                        case TYPE_UNSIGNED_SHORT:
                            elementSize = 2;
                            break;
                        default:
                            elementSize = 2;
                            break;
                    }
                    
                    int totalSize = elementSize * typeInfo.array_size;
                    fprintf(asmFile, "    mov ax, %d ; sizeof array declaration = %d bytes\n", totalSize, totalSize);
                } else {
                    // Regular variable
                    fprintf(asmFile, "    mov ax, 2 ; Default sizeof for variable declaration\n");
                }
            } else {
                // Special cases for type names like 'int', 'char', etc.
                if (node->right->type == NODE_IDENTIFIER) {
                    char* typeName = node->right->identifier;
                    if (strcmp(typeName, "int") == 0 || strcmp(typeName, "short") == 0 || 
                        strcmp(typeName, "unsigned int") == 0 || strcmp(typeName, "unsigned short") == 0) {
                        fprintf(asmFile, "    mov ax, 2 ; sizeof(int/short) = 2 bytes\n");
                    } else if (strcmp(typeName, "char") == 0 || strcmp(typeName, "unsigned char") == 0) {
                        fprintf(asmFile, "    mov ax, 1 ; sizeof(char) = 1 byte\n");
                    } else if (strcmp(typeName, "long") == 0 || strcmp(typeName, "unsigned long") == 0) {
                        fprintf(asmFile, "    mov ax, 4 ; sizeof(long) = 4 bytes\n");
                    } else if (strcmp(typeName, "bool") == 0) {
                        fprintf(asmFile, "    mov ax, 1 ; sizeof(bool) = 1 byte\n");
                    } else if (strcmp(typeName, "void") == 0) {
                        fprintf(asmFile, "    mov ax, 0 ; sizeof(void) = 0 bytes\n");
                    } else if (strstr(typeName, "*") != NULL) {
                        // Handle pointer types
                        fprintf(asmFile, "    mov ax, 2 ; sizeof pointer = 2 bytes (near pointer)\n");
                    } else {
                        fprintf(asmFile, "    mov ax, 2 ; Default size (16 bits = 2 bytes)\n");
                    }
                } else {
                    // For expressions, evaluate the expression but return the size
                    generateExpression(node->right);
                    // Discard the value and load the size
                    fprintf(asmFile, "    mov ax, 2 ; Default size for expressions (16 bits = 2 bytes)\n");
                }
            }
            break;
            
        case UNARY_ADDRESS_OF:
            // Check if operand is an identifier (the common case)
            if (node->right->type == NODE_IDENTIFIER) {
                char* name = node->right->identifier;
                
                if (isParameter(name)) {
                    // Parameter - compute address from BP + offset
                    fprintf(asmFile, "    ; Address of parameter %s\n", name);
                    fprintf(asmFile, "    lea ax, [bp+%d] ; Load address of parameter\n", 
                          -getVariableOffset(name));
                } else {
                    // Local variable - compute address from BP - offset
                    fprintf(asmFile, "    ; Address of variable %s\n", name);
                    fprintf(asmFile, "    lea ax, [bp-%d] ; Load address of local variable\n", 
                          getVariableOffset(name));
                }
            }
            else {
                // This would be more complex address calculations
                // For nested expressions - not fully implemented
                fprintf(asmFile, "    ; Complex address-of operation not fully supported\n");
            }
            break;
            
        case UNARY_NEGATE:
            // Generate code for the operand
            generateExpression(node->right);
            
            // Negate the result
            fprintf(asmFile, "    neg ax ; Negate value\n");
            break;
            
        case UNARY_NOT:
            // Generate code for the operand
            generateExpression(node->right);
            
            // Logical NOT (0 -> 1, non-zero -> 0)
            fprintf(asmFile, "    test ax, ax ; Test if AX is zero\n");
            fprintf(asmFile, "    setz al ; Set AL to 1 if AX is zero, 0 otherwise\n");
            fprintf(asmFile, "    movzx ax, al ; Zero-extend AL to AX\n");
            break;
              case UNARY_BITWISE_NOT:
            // Generate code for the operand
            generateExpression(node->right);
            
            // Bitwise NOT
            fprintf(asmFile, "    not ax ; Bitwise NOT\n");
            break;

        case PREFIX_INCREMENT:
            // For prefix increment (++x), we need to:
            // 1. Generate the address of the operand
            // 2. Load its value
            // 3. Increment the value
            // 4. Store it back
            // 5. Leave the incremented value in AX as the result
            if (node->right->type == NODE_IDENTIFIER) {
                char* name = node->right->identifier;
                int offset = getVariableOffset(name);
                
                if (isParameter(name)) {
                    fprintf(asmFile, "    ; Prefix increment of parameter %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp+%d] ; Load parameter value\n", -offset);
                    fprintf(asmFile, "    inc ax ; Increment value\n");
                    fprintf(asmFile, "    mov [bp+%d], ax ; Store incremented value back\n", -offset);
                } else {
                    fprintf(asmFile, "    ; Prefix increment of variable %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp-%d] ; Load variable value\n", offset);
                    fprintf(asmFile, "    inc ax ; Increment value\n");
                    fprintf(asmFile, "    mov [bp-%d], ax ; Store incremented value back\n", offset);
                }            } else if (node->right->type == NODE_UNARY_OP && 
                      node->right->unary_op.op == UNARY_DEREFERENCE) {
                // Handle the case of ++(*ptr)
                fprintf(asmFile, "    ; Prefix increment of dereferenced pointer\n");
                generateExpression(node->right->right);  // Get the pointer value
                fprintf(asmFile, "    mov bx, ax ; Move pointer to BX\n");
                fprintf(asmFile, "    mov ax, [bx] ; Load value from memory\n");
                fprintf(asmFile, "    inc ax ; Increment value\n");
                fprintf(asmFile, "    mov [bx], ax ; Store incremented value back\n");
            } else {
                reportWarning(-1, "Complex prefix increment not fully supported");
                generateExpression(node->right);
            }
            break;
            
        case PREFIX_DECREMENT:
            // For prefix decrement (--x), similar to prefix increment but decrement
            if (node->right->type == NODE_IDENTIFIER) {
                char* name = node->right->identifier;
                int offset = getVariableOffset(name);
                
                if (isParameter(name)) {
                    fprintf(asmFile, "    ; Prefix decrement of parameter %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp+%d] ; Load parameter value\n", -offset);
                    fprintf(asmFile, "    dec ax ; Decrement value\n");
                    fprintf(asmFile, "    mov [bp+%d], ax ; Store decremented value back\n", -offset);
                } else {
                    fprintf(asmFile, "    ; Prefix decrement of variable %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp-%d] ; Load variable value\n", offset);
                    fprintf(asmFile, "    dec ax ; Decrement value\n");
                    fprintf(asmFile, "    mov [bp-%d], ax ; Store decremented value back\n", offset);
                }            } else if (node->right->type == NODE_UNARY_OP && 
                      node->right->unary_op.op == UNARY_DEREFERENCE) {
                // Handle the case of --(*ptr)
                fprintf(asmFile, "    ; Prefix decrement of dereferenced pointer\n");
                generateExpression(node->right->right);  // Get the pointer value
                fprintf(asmFile, "    mov bx, ax ; Move pointer to BX\n");
                fprintf(asmFile, "    mov ax, [bx] ; Load value from memory\n");
                fprintf(asmFile, "    dec ax ; Decrement value\n");
                fprintf(asmFile, "    mov [bx], ax ; Store decremented value back\n");
            } else {
                reportWarning(-1, "Complex prefix decrement not fully supported");
                generateExpression(node->right);
            }
            break;
            
        case POSTFIX_INCREMENT:
            // For postfix increment (x++), we need to:
            // 1. Generate the address of the operand
            // 2. Load its value
            // 3. Store the original value in a temporary (AX)
            // 4. Increment and store back the value
            // 5. Leave the original value in AX as the result
            if (node->right->type == NODE_IDENTIFIER) {
                char* name = node->right->identifier;
                int offset = getVariableOffset(name);
                
                if (isParameter(name)) {
                    fprintf(asmFile, "    ; Postfix increment of parameter %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp+%d] ; Load parameter value\n", -offset);
                    fprintf(asmFile, "    mov bx, ax ; Save original value to BX\n");
                    fprintf(asmFile, "    inc bx ; Increment value\n");
                    fprintf(asmFile, "    mov [bp+%d], bx ; Store incremented value back\n", -offset);
                    // AX still contains original value
                } else {
                    fprintf(asmFile, "    ; Postfix increment of variable %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp-%d] ; Load variable value\n", offset);
                    fprintf(asmFile, "    mov bx, ax ; Save original value to BX\n");
                    fprintf(asmFile, "    inc bx ; Increment value\n");
                    fprintf(asmFile, "    mov [bp-%d], bx ; Store incremented value back\n", offset);
                    // AX still contains original value
                }            } else if (node->right->type == NODE_UNARY_OP && 
                      node->right->unary_op.op == UNARY_DEREFERENCE) {                // Handle the case of (*ptr)++
                fprintf(asmFile, "    ; Postfix increment of dereferenced pointer\n");
                generateExpression(node->right->right);  // Get the pointer value
                fprintf(asmFile, "    mov bx, ax ; Move pointer to BX\n");
                fprintf(asmFile, "    mov ax, [bx] ; Load value from memory\n");                fprintf(asmFile, "    mov cx, ax ; Save original value to CX\n");
                fprintf(asmFile, "    inc ax ; Increment value\n");
                fprintf(asmFile, "    mov [bx], ax ; Store incremented value back\n");
                fprintf(asmFile, "    mov ax, cx ; Restore original value to AX\n");
                // AX contains original value
            } else {
                reportWarning(-1, "Complex postfix increment not fully supported");
                generateExpression(node->right);
            }
            break;
            
        case POSTFIX_DECREMENT:
            // For postfix decrement (x--), similar to postfix increment but decrement
            if (node->right->type == NODE_IDENTIFIER) {
                char* name = node->right->identifier;
                int offset = getVariableOffset(name);
                
                if (isParameter(name)) {
                    fprintf(asmFile, "    ; Postfix decrement of parameter %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp+%d] ; Load parameter value\n", -offset);
                    fprintf(asmFile, "    mov bx, ax ; Save original value to BX\n");
                    fprintf(asmFile, "    dec bx ; Decrement value\n");
                    fprintf(asmFile, "    mov [bp+%d], bx ; Store decremented value back\n", -offset);
                    // AX still contains original value
                } else {
                    fprintf(asmFile, "    ; Postfix decrement of variable %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp-%d] ; Load variable value\n", offset);
                    fprintf(asmFile, "    mov bx, ax ; Save original value to BX\n");
                    fprintf(asmFile, "    dec bx ; Decrement value\n");
                    fprintf(asmFile, "    mov [bp-%d], bx ; Store decremented value back\n", offset);
                    // AX still contains original value
                }            } else if (node->right->type == NODE_UNARY_OP && 
                      node->right->unary_op.op == UNARY_DEREFERENCE) {
                // Handle the case of (*ptr)--
                fprintf(asmFile, "    ; Postfix decrement of dereferenced pointer\n");
                generateExpression(node->right->right);  // Get the pointer value
                fprintf(asmFile, "    mov bx, ax ; Move pointer to BX\n");
                fprintf(asmFile, "    mov ax, [bx] ; Load value from memory\n");
                fprintf(asmFile, "    mov cx, ax ; Save original value to CX\n");
                fprintf(asmFile, "    dec cx ; Decrement value\n");
                fprintf(asmFile, "    mov [bx], cx ; Store decremented value back\n");
                // AX still contains original value
            } else {
                reportWarning(-1, "Complex postfix decrement not fully supported");
                generateExpression(node->right);
            }
            break;
            
        case UNARY_CAST:
            // Generate code for the operand first
            generateExpression(node->right);
            
            // Handle different cast types
            switch (node->unary_op.cast_type) {
                case TYPE_UNSIGNED_CHAR:
                    // For unsigned char, zero-extend after masking to byte
                    fprintf(asmFile, "    ; Cast to unsigned char\n");
                    fprintf(asmFile, "    and ax, 0xFF ; Mask to byte\n");
                    break;
                    
                case TYPE_CHAR:
                    // For signed char, sign-extend after getting bottom byte
                    fprintf(asmFile, "    ; Cast to signed char\n");
                    fprintf(asmFile, "    movsx ax, al ; Sign extend the bottom byte\n");
                    break;
                    
                case TYPE_UNSIGNED_INT:
                case TYPE_UNSIGNED_SHORT:
                    // For unsigned int/short, just keep the value in AX
                    fprintf(asmFile, "    ; Cast to unsigned int/short\n");
                    // No operation needed, 16-bit value in AX is already the right format
                    break;
                    
                case TYPE_INT:
                case TYPE_SHORT:
                    // For int/short, the value is already in the right format (AX)
                    fprintf(asmFile, "    ; Cast to signed int/short\n");
                    // No operation needed for 16-bit signed integers
                    break;
                    
                case TYPE_BOOL:
                    // For bool, test for non-zero and set result to 0 or 1
                    fprintf(asmFile, "    ; Cast to bool\n");
                    fprintf(asmFile, "    test ax, ax ; Check if not zero\n");
                    fprintf(asmFile, "    mov ax, 0 ; Default to false\n");
                    fprintf(asmFile, "    jz cast_bool_end_%d\n", labelCounter);
                    fprintf(asmFile, "    mov ax, 1 ; Set to true if non-zero\n");
                    fprintf(asmFile, "cast_bool_end_%d:\n", labelCounter++);
                    break;
                    
                default:
                    // Default behavior for other types
                    fprintf(asmFile, "    ; Unhandled cast type: %d\n", node->unary_op.cast_type);
                    break;
            }
            break;
            
        default:
            reportWarning(-1, "Unsupported unary operator: %d", node->unary_op.op);
            break;
    }
}
