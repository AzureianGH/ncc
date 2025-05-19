#include "codegen.h"
#include "ast.h"
#include "string_literals.h"
#include "error_manager.h"
#include "global_variables.h"
#include "type_checker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External declarations from type_checker.c
extern TypeInfo* getTypeInfo(const char* name);
extern TypeInfo* getTypeInfoFromExpression(ASTNode* expr);

// Output file for assembly code
FILE* asmFile = NULL;

// String literals table
int stringLiteralCount = 0;
char** stringLiterals = NULL;

// Array declarations table
int arrayCount = 0;
char** arrayNames = NULL;
int* arraySizes = NULL;
DataType* arrayTypes = NULL;

// Flags to track if marker functions were found
int stringMarkerFound = 0;
int arrayMarkerFound = 0;

// Label counter for unique labels
int labelCounter = 0;

// Current function being generated
static char* currentFunction = NULL;
static int currentFunctionIsNaked = 0;

// Variable tracking for stack offsets
#define MAX_LOCALS 64
typedef struct {
    char* name;
    int offset;
} LocalVariable;

static LocalVariable localVars[MAX_LOCALS];
static int localVarCount = 0;
static int stackSize = 0;

// Origin address for ORG directive
static unsigned int originAddress = 0;

// Clear local variables when entering a new function
void clearLocalVars() {
    for (int i = 0; i < localVarCount; i++) {
        if (localVars[i].name) {
            free(localVars[i].name);
        }
    }
    localVarCount = 0;
    stackSize = 0;
}

// gcc disable unused variable warnings
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif


// Add a local variable and return its stack offset
int addLocalVariable(const char* name, int size) {
    if (localVarCount >= MAX_LOCALS) {
        fprintf(stderr, "Error: Too many local variables\n");
        exit(1);
    }
      
    // Always use word-sized (2-byte) allocations on the stack for x86
    // This ensures proper alignment and simplifies address calculations
    int allocationSize = 2; // Always allocate at least 2 bytes (word-sized)
    
    stackSize += allocationSize;
    localVars[localVarCount].name = strdup(name);
    localVars[localVarCount].offset = stackSize;
    localVarCount++;
    
    return stackSize;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

// Get the stack offset for a variable
int getVariableOffset(const char* name) {
    for (int i = 0; i < localVarCount; i++) {
        if (strcmp(localVars[i].name, name) == 0) {
            return localVars[i].offset;
        }
    }
      // Variable not found - might be a global
    return 0;
}

// Check if a variable is a parameter (parameters have negative offsets)
int isParameter(const char* name) {
    for (int i = 0; i < localVarCount; i++) {
        if (strcmp(localVars[i].name, name) == 0) {
            return localVars[i].offset < 0;
        }
    }
    return 0;
}

// Initialize code generator
void initCodeGen(const char* outputFilename, unsigned int orgAddr) {
    asmFile = fopen(outputFilename, "w");
    if (!asmFile) {
        fprintf(stderr, "Error: Could not open output file %s\n", outputFilename);
        exit(1);
    }
    labelCounter = 0;
    
    // Initialize string tracking
    stringLiteralCount = 0;
    stringLiterals = NULL;
    stringMarkerFound = 0;
      // Initialize array tracking
    arrayCount = 0;
    arrayNames = NULL;
    arraySizes = NULL;
    arrayTypes = NULL;
    arrayMarkerFound = 0;
    
    originAddress = orgAddr;
}

// Close code generator
void finalizeCodeGen() {
    if (asmFile) {
        // Generate any global variables that weren't emitted at a marker
        generateRemainingGlobals(asmFile);
        
        // Generate string literals section before closing
        generateStringLiteralsSection();
        
        // Free string literals
        for (int i = 0; i < stringLiteralCount; i++) {
            if (stringLiterals[i]) {
                free(stringLiterals[i]);
            }
        }
        free(stringLiterals);
        
        // Free array tracking
        for (int i = 0; i < arrayCount; i++) {
            if (arrayNames[i]) {
                free(arrayNames[i]);
            }
        }
        free(arrayNames);
        free(arraySizes);
        free(arrayTypes);
        
        // Clean up global variables
        cleanupGlobals();
        
        fclose(asmFile);
        asmFile = NULL;
    }
}

// Check if a node represents a pointer type
int isPointerType(ASTNode* node) {
    if (!node) return 0;
    
    // Direct check for literals - string literals are pointers
    if (node->type == NODE_LITERAL) {
        // String literals are pointers
        if (node->literal.data_type == TYPE_CHAR && node->literal.string_value) {
            return 1;
        }
        // Far pointers
        if (node->literal.data_type == TYPE_FAR_POINTER) {
            return 1;
        }
    }
    
    // For identifiers, check the symbol table
    if (node->type == NODE_IDENTIFIER) {
        TypeInfo* typeInfo = getTypeInfo(node->identifier);
        return typeInfo && typeInfo->is_pointer;
    }
    
    // For expressions, use the type inferred from the expression
    TypeInfo* typeInfo = getTypeInfoFromExpression(node);
    return typeInfo && typeInfo->is_pointer;
}

// Generate a unique label ID
int getNextLabelId() {
    return labelCounter++;
}

// Generate a label with prefix
char* generateLabel(const char* prefix) {
    char* label = malloc(strlen(prefix) + 10);
    sprintf(label, "%s%d", prefix, getNextLabelId());
    return label;
}

void generateGlobalDeclaration(ASTNode* node);
void generateFunction(ASTNode* node);
void generateBlock(ASTNode* node);
void generateStatement(ASTNode* node);
void generateVariableDeclaration(ASTNode* node);
void generateExpression(ASTNode* node);
void generateBinaryOp(ASTNode* node);
void generateReturnStatement(ASTNode* node);
void generateFunctionCall(ASTNode* node);
void generateAsmBlock(ASTNode* node);
void generateAsmStmt(ASTNode* node);
void generateForLoop(ASTNode* node);

// Generate the header for the program
void generateProgramHeader() {
    fprintf(asmFile, "; 8086 Assembly generated by NCC Compiler\n");
    fprintf(asmFile, "bits 16\n");
    // ORG directive for load address
    fprintf(asmFile, "org 0x%X\n\n", originAddress);
     // No program entry point boilerplate for flat binary
}

// Main code generation function
void generateCode(ASTNode* root) {
    if (!root) {
        fprintf(stderr, "Error: Empty AST\n");
        return;
    }
      // Debug message removed to reduce output verbosity
    
    // Start with program header
    generateProgramHeader();
    
    // Process the AST - starting from the program node
    if (root->type == NODE_PROGRAM) {
        // Debug message removed to reduce output verbosity
          // Process all top-level declarations and functions
        ASTNode* current = root->left;
          if (!current) {
            fprintf(stderr, "Warning: Program node has no children (empty program)\n");
        }
        
        int nodeCount = 0;
        while (current) {
            nodeCount++;
              switch (current->type) {
                case NODE_FUNCTION:
                    // Debug message removed to reduce output verbosity
                    generateFunction(current);
                    break;
                case NODE_DECLARATION:
                    // Debug message removed to reduce output verbosity 
                    generateGlobalDeclaration(current);
                    break;
                default:
                    fprintf(stderr, "Warning: Unsupported top-level node type: %d\n", current->type);
            }
            current = current->next;
        }
    }
}

// Generate code for a function
void generateFunction(ASTNode* node) {
    if (!node || node->type != NODE_FUNCTION) return;
    
    char* funcName = node->function.func_name;    // Check if this is a special marker function
    if (strcmp(funcName, "_NCC_STRING_LOC") == 0) {
        // This is where string literals should go
        generateStringsAtMarker();
        
        // Output the naked function label but no real code
        fprintf(asmFile, "; String literal location marker\n");
        fprintf(asmFile, "_%s:\n", funcName);
        return;
    }
    else if (strcmp(funcName, "_NCC_ARRAY_LOC") == 0) {
        // This is where arrays should go
        generateArraysAtMarker();
        
        // Output the naked function label but no real code
        fprintf(asmFile, "; Array location marker\n");
        fprintf(asmFile, "_%s:\n", funcName);
        return;
    }
    else if (strcmp(funcName, "_NCC_GLOBAL_LOC") == 0) {
        // This is where global variables should go
        generateGlobalsAtMarker(asmFile);
        
        // Output the naked function label but no real code
        fprintf(asmFile, "; Global variable location marker\n");
        fprintf(asmFile, "_%s:\n", funcName);
        return;
    }
    
    // Regular function processing
    // Clear any local variables from previous functions
    clearLocalVars();
    funcName = node->function.func_name;
    currentFunction = funcName;
    currentFunctionIsNaked = node->function.info.is_naked;
    
    fprintf(asmFile, "; Function: %s\n", funcName);
    fprintf(asmFile, "_%s:\n", funcName);  // Prepend underscore to function names
      // Check if this is a naked function (no prologue/epilogue)
    if (currentFunctionIsNaked) {
        fprintf(asmFile, "    ; Naked function - no prologue generated\n");
    }
    // Check if this function uses stackframe
    else if (node->function.info.is_stackframe) {
        fprintf(asmFile, "    ; Setup stackframe with register preservation\n");
        fprintf(asmFile, "    push bp\n");
        fprintf(asmFile, "    mov bp, sp\n");
        fprintf(asmFile, "    push bx\n");
        fprintf(asmFile, "    push cx\n");
        fprintf(asmFile, "    push dx\n");
        fprintf(asmFile, "    push si\n");
        fprintf(asmFile, "    push di\n");
        
        // We'll calculate the actual space needed after processing all declarations
        fprintf(asmFile, "    ; Space for local variables will be allocated later\n\n");
    } else {
        // Standard function prologue
        fprintf(asmFile, "    push bp\n");
        fprintf(asmFile, "    mov bp, sp\n\n");
    }
    
    // Add function parameters to local variable table
    // Parameters start at bp+4 (return address is at bp+2)
    int paramOffset = 4;
    ASTNode* param = node->function.params;
    while (param) {
        // Parameters are accessed via positive offsets from bp
        if (param->type == NODE_DECLARATION) {
            // Store parameters in reverse order for easier access
            localVars[localVarCount].name = strdup(param->declaration.var_name);
            localVars[localVarCount].offset = -paramOffset; // Negative offset means it's a parameter
            localVarCount++;
            
            // Parameters always take 2 bytes on the stack in 16-bit mode
            paramOffset += 2;
        }
        param = param->next;
    }
    
    // Generate code for function body
    if (node->function.body) {
        generateBlock(node->function.body);
    }
    
    // Generate function exit label
    fprintf(asmFile, "\n_%s_exit:\n", funcName);
      // Function epilogue
    if (node->function.info.is_naked) {
        fprintf(asmFile, "    ; Naked function - no epilogue generated\n");
        // No ret instruction for naked functions - user must provide it
    }
    else if (node->function.info.is_stackframe) {
        fprintf(asmFile, "    ; Restore stackframe with registers\n");
        
        // If we allocated space for locals, deallocate it here
        if (stackSize > 0) {
            fprintf(asmFile, "    add sp, %d ; Remove space for local variables\n", stackSize);
        }
        
        fprintf(asmFile, "    mov sp, bp\n");
        fprintf(asmFile, "    pop di\n");
        fprintf(asmFile, "    pop si\n");
        fprintf(asmFile, "    pop dx\n");
        fprintf(asmFile, "    pop cx\n");
        fprintf(asmFile, "    pop bx\n");
        fprintf(asmFile, "    pop bp\n");
        fprintf(asmFile, "    ret\n");
    } else {
        fprintf(asmFile, "    ; Standard function epilogue\n");
        fprintf(asmFile, "    mov sp, bp\n");
        fprintf(asmFile, "    pop bp\n");
        fprintf(asmFile, "    ret\n");
    }
      fprintf(asmFile, "\n");
    
    currentFunction = NULL;
    currentFunctionIsNaked = 0;
}

// Generate code for a block of statements
void generateBlock(ASTNode* node) {
    if (!node || node->type != NODE_BLOCK) return;
    
    ASTNode* statement = node->left;
    
    while (statement) {
        generateStatement(statement);
        statement = statement->next;
    }
}

// Generate code for a statement
void generateStatement(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_DECLARATION:
            generateVariableDeclaration(node);
            break;
              case NODE_ASSIGNMENT:
            fprintf(asmFile, "    ; Assignment statement\n");
            // Generate code for right-hand side
            if (node->assignment.op != 0 && node->left->type == NODE_IDENTIFIER) {
                // Compound assignment: compute old value and RHS
                // Load current LHS value
                if (isParameter(node->left->identifier)) {
                    fprintf(asmFile, "    mov ax, [bp+%d] ; Load parameter %s for compound assignment\n", 
                            -getVariableOffset(node->left->identifier), node->left->identifier);
                } else {
                    fprintf(asmFile, "    mov ax, [bp-%d] ; Load local variable %s for compound assignment\n", 
                            getVariableOffset(node->left->identifier), node->left->identifier);
                }
                fprintf(asmFile, "    push ax ; Save old value\n");
                // Evaluate RHS
                generateExpression(node->right);
                fprintf(asmFile, "    push ax ; Save RHS value\n");
                // Pop into registers: BX=rhs, AX=old
                fprintf(asmFile, "    pop bx ; RHS value\n");
                fprintf(asmFile, "    pop ax ; Old LHS value\n");
                // Apply operation
                switch (node->assignment.op) {
                    case OP_PLUS_ASSIGN:
                        fprintf(asmFile, "    add ax, bx ; +=\n");
                        break;
                    case OP_MINUS_ASSIGN:
                        fprintf(asmFile, "    sub ax, bx ; -=\n");
                        break;
                    case OP_MUL_ASSIGN:
                        fprintf(asmFile, "    imul bx ; *=\n");
                        break;
                    case OP_DIV_ASSIGN:
                        fprintf(asmFile, "    cwd ; Sign extend AX into DX:AX for division\n");
                        fprintf(asmFile, "    idiv bx ; /=\n");
                        break;
                    case OP_MOD_ASSIGN:
                        fprintf(asmFile, "    cwd ; Sign extend AX into DX:AX for mod\n");
                        fprintf(asmFile, "    idiv bx ;\n");
                        fprintf(asmFile, "    mov ax, dx ; remainder in DX\n");
                        break;
                    default:
                        break;
                }
            } else if (node->assignment.op == 0) {
                // Simple assignment: evaluate RHS
                generateExpression(node->right);
            } else {
                // Other targets or ops: fallback to simple RHS
                generateExpression(node->right);
            }

            // Store result in AX to the left-hand side
            if (node->left->type == NODE_IDENTIFIER) {
                // Check if this is a parameter or local variable
                if (isParameter(node->left->identifier)) {
                    // Parameters have positive offsets from bp
                    fprintf(asmFile, "    mov [bp+%d], ax ; Store in parameter %s\n", 
                            -getVariableOffset(node->left->identifier), node->left->identifier);
                } else {
                    // Local variables have negative offsets from bp
                    fprintf(asmFile, "    mov [bp-%d], ax ; Store in local variable %s\n", 
                            getVariableOffset(node->left->identifier), node->left->identifier);
                }
            }            else if (node->left->type == NODE_UNARY_OP && node->left->unary_op.op == UNARY_DEREFERENCE) {
                // Pointer assignment (e.g., *ptr = value)
                // Save the right-hand side result temporarily
                fprintf(asmFile, "    push ax ; Save right-hand side value\n");
                
                // Generate code to evaluate the pointer expression
                generateExpression(node->left->right);
                
                // Check if this is a far pointer (segment in DX, offset in AX)
                if (node->left->right->type == NODE_LITERAL && node->left->right->literal.data_type == TYPE_FAR_POINTER) {
                    fprintf(asmFile, "    ; Far pointer assignment\n");
                    fprintf(asmFile, "    push ds ; Save current DS\n");
                    fprintf(asmFile, "    mov bx, ax ; Move offset to BX\n");
                    fprintf(asmFile, "    mov ds, dx ; Set DS to segment\n");
                    fprintf(asmFile, "    pop ax ; Restore right-hand side value\n");
                    
                    // Get type info to determine proper store size
                    TypeInfo* typeInfo = getTypeInfoFromExpression(node->left->right);
                    if (typeInfo && (typeInfo->type == TYPE_CHAR || 
                                    typeInfo->type == TYPE_UNSIGNED_CHAR || 
                                    typeInfo->type == TYPE_BOOL)) {
                        fprintf(asmFile, "    mov [bx], al ; Store byte value through far pointer\n");
                    } else {
                        fprintf(asmFile, "    mov [bx], ax ; Store word value through far pointer\n");
                    }
                    
                    fprintf(asmFile, "    pop ds ; Restore DS\n");
                } else {
                    // Regular near pointer
                    fprintf(asmFile, "    mov bx, ax ; Move pointer address to BX\n");
                    
                    // Restore the value and store through the pointer
                    fprintf(asmFile, "    pop ax ; Restore right-hand side value\n");
                    
                    // Get type info to determine proper store size
                    TypeInfo* typeInfo = getTypeInfoFromExpression(node->left->right);
                    if (typeInfo && (typeInfo->type == TYPE_CHAR || 
                                    typeInfo->type == TYPE_UNSIGNED_CHAR || 
                                    typeInfo->type == TYPE_BOOL)) {
                        fprintf(asmFile, "    mov [bx], al ; Store byte value through pointer\n");
                    } else {
                        fprintf(asmFile, "    mov [bx], ax ; Store word value through pointer\n");
                    }
                }
            }
            else {
                reportWarning(-1, "Unsupported assignment target");
            }
            break;
            
        case NODE_RETURN:
            generateReturnStatement(node);
            break;
            
        case NODE_EXPRESSION:
            // Generate code for the expression, but we don't need the result
            generateExpression(node->left);
            break;
              case NODE_ASM_BLOCK:
            generateAsmBlock(node);
            break;
            
        case NODE_ASM:
            generateAsmStmt(node);
            break;
            
        case NODE_FOR:
            generateForLoop(node);
            break;
            
        case NODE_WHILE:
            generateWhileLoop(node);
            break;
            
        case NODE_IF:
            generateIfStatement(node);
            break;
              default:
            reportWarning(-1, "Unsupported statement type: %d", node->type);
            break;
    }
}

// Generate code for variable declaration
void generateVariableDeclaration(ASTNode* node) {
    if (!node || node->type != NODE_DECLARATION) return;
    
    // Check if this is an array with a fixed size
    if (node->declaration.type_info.is_array && node->declaration.type_info.array_size > 0) {
        // Check if this array has initializers
        if (node->declaration.initializer) {
            fprintf(asmFile, "    ; Array with initializers\n");
            // Generate global array with initializers at data section
            generateArrayWithInitializers(node);
        } else {
            // Register the array to be generated later with zeros
            addArrayDeclaration(node->declaration.var_name,
                               node->declaration.type_info.array_size,
                               node->declaration.type_info.type);
        }
        
        // Set up a pointer to the array
        fprintf(asmFile, "    ; Array variable declaration: %s[%d]\n", 
                node->declaration.var_name, 
                node->declaration.type_info.array_size);
        fprintf(asmFile, "    mov ax, _%s ; Address of array\n", node->declaration.var_name);
        fprintf(asmFile, "    push ax ; Store pointer to array\n");
        
        // Add to local variable table
        addLocalVariable(node->declaration.var_name, 2);  // Pointer size is 2 bytes
        
        return;
    }
    
    // For non-array local variables, proceed as before
    fprintf(asmFile, "    ; Local variable declaration: %s\n", node->declaration.var_name);
    
    // Determine variable size based on type
    int varSize = 2; // Default for int, short
    if (node->declaration.type_info.type == TYPE_CHAR || 
        node->declaration.type_info.type == TYPE_UNSIGNED_CHAR) {
        varSize = 1;
    }    // If there's an initializer, generate code for the assignment
    if (node->declaration.initializer) {
        // Generate the value
        generateExpression(node->declaration.initializer);
        
        // For regular values, just push AX (char_test.c now uses regular pointers)
        fprintf(asmFile, "    push ax ; Initialize local variable\n");
    } else {
        // Just reserve space by pushing a zero
        fprintf(asmFile, "    push 0 ; Uninitialized local variable\n");
    }
    
    // Add to local variable table
    addLocalVariable(node->declaration.var_name, varSize);
}

// Generate code for global variable declaration
void generateGlobalDeclaration(ASTNode* node) {
    if (!node || node->type != NODE_DECLARATION) return;
    
    // Check if this is an array with a fixed size
    if (node->declaration.type_info.is_array && node->declaration.type_info.array_size > 0) {
        // Check if array has initializers
        if (node->declaration.initializer) {
            // Output the array with initializer values directly
            generateArrayWithInitializers(node);
        } else {
            // Register the array to be generated later at the proper location with zeros
            addArrayDeclaration(node->declaration.var_name,
                               node->declaration.type_info.array_size,
                               node->declaration.type_info.type);
        }
        
        // No code is emitted here - arrays will be generated at the marker or at the end
        return;
    }
    
    // Add global declaration to be generated at the _NCC_GLOBAL_LOC marker if present
    addGlobalDeclaration(node);
}



// Generate all collected global variables
// Generate code for an expression
void generateExpression(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_LITERAL:
            // Handle different literal types
            if (node->literal.data_type == TYPE_FAR_POINTER) {
                // Load segment into dx, offset into ax for far pointers
                fprintf(asmFile, "    mov dx, 0x%04X ; Segment\n", node->literal.segment);
                fprintf(asmFile, "    mov ax, 0x%04X ; Offset\n", node->literal.offset);
            } else if (node->literal.data_type == TYPE_CHAR && node->literal.string_value) {
                // Add string to the string literals table and get its index
                int strIndex = addStringLiteral(node->literal.string_value);
                
                if (strIndex >= 0) {
                    // Load the address of the string into AX
                    fprintf(asmFile, "    ; String literal: %s\n", node->literal.string_value);
                    fprintf(asmFile, "    mov ax, string_%d ; Address of string\n", strIndex);
                } else {
                    fprintf(asmFile, "    ; Error processing string literal: %s\n", 
                            node->literal.string_value ? node->literal.string_value : "(null)");
                    fprintf(asmFile, "    mov ax, 0 ; Using null pointer as fallback\n");
                }            } else if (node->literal.data_type == TYPE_CHAR && !node->literal.string_value) {
                // For character literals, load the ASCII value into al (8-bit)
                // Then zero-extend to ax for consistent value handling
                fprintf(asmFile, "    mov al, %d ; Load character value (ASCII: '%c')\n", 
                       (unsigned char)node->literal.char_value, node->literal.char_value);
                fprintf(asmFile, "    mov ah, 0 ; Zero-extend to 16-bit\n");
            } else if (node->literal.data_type == TYPE_BOOL) {
                // For boolean literals, load 0 or 1 into ax
                fprintf(asmFile, "    mov ax, %d ; Load boolean value (%s)\n", 
                       node->literal.int_value, node->literal.int_value ? "true" : "false");
            } else {
                // For numbers, load the value into ax
                fprintf(asmFile, "    mov ax, %d ; Load literal\n", node->literal.int_value);
            }
            break;
        
        case NODE_IDENTIFIER:
            // Check if this is a parameter or local variable
            if (isParameter(node->identifier)) {
                // Parameters have positive offsets from bp
                fprintf(asmFile, "    mov ax, [bp+%d] ; Load parameter %s\n", 
                        -getVariableOffset(node->identifier), node->identifier);
            } else {
            // Local variables have negative offsets from bp
                int varOffset = getVariableOffset(node->identifier);
                
                if (varOffset == 0) {
                    // Could be a global variable
                    fprintf(asmFile, "    ; Loading potentially global variable %s\n", node->identifier);
                    fprintf(asmFile, "    mov ax, [_%s] ; Load global variable\n", node->identifier);
                } else {
                    // Ensure we use word-aligned offsets for local variables
                    fprintf(asmFile, "    mov ax, [bp-%d] ; Load local variable %s\n", 
                            varOffset, node->identifier);
                }
            }
            break;
              case NODE_BINARY_OP:
            // Generate code for binary operation
            generateBinaryOp(node);
            break;
            
        case NODE_UNARY_OP:
            // Generate code for unary operation
            generateUnaryOp(node);
            break;
            
        case NODE_CALL:
            // Generate function call
            generateFunctionCall(node);
            break;
        case NODE_ASSIGNMENT:
            // Assignment used as expression: generate code and leave assigned value in AX
            generateStatement(node);
            break;        default:
            reportWarning(-1, "Unsupported expression type: %d", node->type);
            break;
    }
}

// Generate code for binary operations
void generateBinaryOp(ASTNode* node) {
    // Short-circuit logical operators
    if (node->operation.op == OP_LAND) {
        char* falseLabel = generateLabel("land_false");
        char* endLabel = generateLabel("land_end");
        generateExpression(node->left);
        fprintf(asmFile, "    test ax, ax ; logical AND left test\n");
        fprintf(asmFile, "    jz %s ; left false, skip right\n", falseLabel);
        generateExpression(node->right);
        fprintf(asmFile, "    test ax, ax ; logical AND right test\n");
        fprintf(asmFile, "    jz %s ; right false, result false\n", falseLabel);
        fprintf(asmFile, "    mov ax, 1 ; both true -> true\n");
        fprintf(asmFile, "    jmp %s\n", endLabel);
        fprintf(asmFile, "%s:\n", falseLabel);
        fprintf(asmFile, "    mov ax, 0 ; false\n");
        fprintf(asmFile, "%s:\n", endLabel);
        free(falseLabel);
        free(endLabel);
        return;
    }
    if (node->operation.op == OP_LOR) {
        char* trueLabel = generateLabel("lor_true");
        char* endLabel = generateLabel("lor_end");
        generateExpression(node->left);
        fprintf(asmFile, "    test ax, ax ; logical OR left test\n");
        fprintf(asmFile, "    jnz %s ; left true, result true\n", trueLabel);
        generateExpression(node->right);
        fprintf(asmFile, "    test ax, ax ; logical OR right test\n");
        fprintf(asmFile, "    jnz %s ; right true -> true\n", trueLabel);
        fprintf(asmFile, "    mov ax, 0 ; both false -> false\n");
        fprintf(asmFile, "    jmp %s\n", endLabel);
        fprintf(asmFile, "%s:\n", trueLabel);
        fprintf(asmFile, "    mov ax, 1 ; true\n");
        fprintf(asmFile, "%s:\n", endLabel);
        free(trueLabel);
        free(endLabel);
        return;
    }
    // Generate left operand and save
    generateExpression(node->left);
    fprintf(asmFile, "    push ax ; Save left operand\n");
    
    // Generate right operand, result in ax
    generateExpression(node->right);
    
    // Move right operand to bx and restore left operand to ax
    fprintf(asmFile, "    mov bx, ax ; Right operand to bx\n");
    fprintf(asmFile, "    pop ax ; Restore left operand\n");
      // Perform operation based on operator type
    switch (node->operation.op) {
        case OP_ADD:
            // Check for pointer arithmetic (pointer + integer)
            if (isPointerType(node->left)) {
                // Get element size for scaling
                TypeInfo* typeInfo = getTypeInfoFromExpression(node->left);
                if (typeInfo) {
                    int elemSize = (typeInfo->type == TYPE_CHAR || 
                                   typeInfo->type == TYPE_UNSIGNED_CHAR || 
                                   typeInfo->type == TYPE_BOOL) ? 1 : 2;
                    
                    if (elemSize > 1) {
                        // Scale the offset by element size
                        fprintf(asmFile, "    ; Pointer arithmetic: scale by element size %d\n", elemSize);
                        fprintf(asmFile, "    shl bx, 1 ; Scale index by 2 for word elements\n");
                    }
                }
            } else if (isPointerType(node->right)) {
                // Case of integer + pointer (need to scale integer)
                TypeInfo* typeInfo = getTypeInfoFromExpression(node->right);
                if (typeInfo) {
                    int elemSize = (typeInfo->type == TYPE_CHAR || 
                                   typeInfo->type == TYPE_UNSIGNED_CHAR || 
                                   typeInfo->type == TYPE_BOOL) ? 1 : 2;
                    
                    if (elemSize > 1) {
                        // Scale the offset by element size
                        fprintf(asmFile, "    ; Pointer arithmetic: scale by element size %d\n", elemSize);
                        fprintf(asmFile, "    shl ax, 1 ; Scale index by 2 for word elements\n");
                    }
                    
                    // Swap operands to ensure pointer is in AX
                    fprintf(asmFile, "    xchg ax, bx ; Swap to put pointer in AX\n");
                }
            }
            fprintf(asmFile, "    add ax, bx ; Addition\n");
            break;
            
        case OP_SUB:
            // Check for pointer arithmetic (pointer - integer or pointer - pointer)
            if (isPointerType(node->left)) {
                if (isPointerType(node->right)) {
                    // Pointer - Pointer: yields a count of elements between pointers
                    fprintf(asmFile, "    ; Pointer difference\n");
                    fprintf(asmFile, "    sub ax, bx ; Calculate raw byte difference\n");
                    
                    // Divide by element size to get element count
                    TypeInfo* typeInfo = getTypeInfoFromExpression(node->left);
                    if (typeInfo && (typeInfo->type != TYPE_CHAR && 
                                    typeInfo->type != TYPE_UNSIGNED_CHAR && 
                                    typeInfo->type != TYPE_BOOL)) {
                        fprintf(asmFile, "    sar ax, 1 ; Divide by 2 for word elements\n");
                    }
                } else {
                    // Pointer - Integer: scale integer by element size
                    TypeInfo* typeInfo = getTypeInfoFromExpression(node->left);
                    if (typeInfo && (typeInfo->type != TYPE_CHAR && 
                                    typeInfo->type != TYPE_UNSIGNED_CHAR && 
                                    typeInfo->type != TYPE_BOOL)) {
                        fprintf(asmFile, "    ; Pointer arithmetic: scale by element size\n");
                        fprintf(asmFile, "    shl bx, 1 ; Scale index by 2 for word elements\n");
                    }
                    fprintf(asmFile, "    sub ax, bx ; Subtraction\n");
                }
            } else {
                fprintf(asmFile, "    sub ax, bx ; Subtraction\n");
            }
            break;
            
        case OP_MUL:
            fprintf(asmFile, "    imul bx ; Multiplication (signed)\n");
            break;
        case OP_DIV:
            fprintf(asmFile, "    cwd ; Sign extend AX into DX:AX\n");
            fprintf(asmFile, "    idiv bx ; Division (signed)\n");
            break;
        case OP_MOD:
            fprintf(asmFile, "    cwd ; Sign extend AX into DX:AX\n");
            fprintf(asmFile, "    idiv bx ; Division (signed)\n");
            fprintf(asmFile, "    mov ax, dx ; Remainder is in DX\n");
            break;
              // Comparison operators for 8086 (without setcc instructions)
        case OP_EQ:
            fprintf(asmFile, "    cmp ax, bx ; Equal comparison\n");
            fprintf(asmFile, "    mov ax, 0  ; Assume false\n");
            fprintf(asmFile, "    je eq_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp eq_end_%d\n", labelCounter);
            fprintf(asmFile, "eq_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1  ; Set true\n");
            fprintf(asmFile, "eq_end_%d:\n", labelCounter++);
            break;
        case OP_NEQ:
            fprintf(asmFile, "    cmp ax, bx ; Not equal comparison\n");
            fprintf(asmFile, "    mov ax, 0  ; Assume false\n");
            fprintf(asmFile, "    jne neq_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp neq_end_%d\n", labelCounter);
            fprintf(asmFile, "neq_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1  ; Set true\n");
            fprintf(asmFile, "neq_end_%d:\n", labelCounter++);
            break;
        case OP_LT:
            fprintf(asmFile, "    cmp ax, bx ; Less than comparison\n");
            fprintf(asmFile, "    mov ax, 0  ; Assume false\n");
            fprintf(asmFile, "    jl lt_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp lt_end_%d\n", labelCounter);
            fprintf(asmFile, "lt_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1  ; Set true\n");
            fprintf(asmFile, "lt_end_%d:\n", labelCounter++);
            break;
        case OP_LTE:
            fprintf(asmFile, "    cmp ax, bx ; Less than or equal comparison\n");
            fprintf(asmFile, "    mov ax, 0  ; Assume false\n");
            fprintf(asmFile, "    jle lte_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp lte_end_%d\n", labelCounter);
            fprintf(asmFile, "lte_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1  ; Set true\n");
            fprintf(asmFile, "lte_end_%d:\n", labelCounter++);
            break;
        case OP_GT:
            fprintf(asmFile, "    cmp ax, bx ; Greater than comparison\n");
            fprintf(asmFile, "    mov ax, 0  ; Assume false\n");
            fprintf(asmFile, "    jg gt_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp gt_end_%d\n", labelCounter);
            fprintf(asmFile, "gt_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1  ; Set true\n");
            fprintf(asmFile, "gt_end_%d:\n", labelCounter++);
            break;
        case OP_GTE:
            fprintf(asmFile, "    cmp ax, bx ; Greater than or equal comparison\n");
            fprintf(asmFile, "    mov ax, 0  ; Assume false\n");
            fprintf(asmFile, "    jge gte_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp gte_end_%d\n", labelCounter);
            fprintf(asmFile, "gte_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1  ; Set true\n");
            fprintf(asmFile, "gte_end_%d:\n", labelCounter++);
            break;
              case OP_BITWISE_AND:
            fprintf(asmFile, "    and ax, bx ; Bitwise AND\n");
            break;
            
        case OP_BITWISE_OR:
            fprintf(asmFile, "    or ax, bx ; Bitwise OR\n");
            break;
            
        case OP_BITWISE_XOR:
            fprintf(asmFile, "    xor ax, bx ; Bitwise XOR\n");
            break;
            
        case OP_LEFT_SHIFT:
            fprintf(asmFile, "    mov cx, bx ; Set shift count in CX\n");
            fprintf(asmFile, "    shl ax, cl ; Shift left\n");
            break;
            
        case OP_RIGHT_SHIFT:
            fprintf(asmFile, "    mov cx, bx ; Set shift count in CX\n");
            fprintf(asmFile, "    sar ax, cl ; Shift right (arithmetic, preserves sign)\n");
            break;
              default:
            reportWarning(-1, "Unsupported binary operator: %d", node->operation.op);
            break;
    }
}

// Generate code for a function call
void generateFunctionCall(ASTNode* node) {
    if (!node || node->type != NODE_CALL) return;
    
    fprintf(asmFile, "    ; Function call to %s\n", node->call.func_name);
    
    // Push arguments in reverse order
    int argCount = 0;
    ASTNode* arg = node->call.args;
    ASTNode* args[32]; // Maximum 32 arguments
    
    // Collect arguments in an array first
    while (arg) {
        args[argCount++] = arg;
        arg = arg->next;
    }
    
    // Push arguments in reverse order
    for (int i = argCount - 1; i >= 0; i--) {
        generateExpression(args[i]);
        fprintf(asmFile, "    push ax ; Argument %d\n", i + 1);
    }
    
    // Check if it's a far call
    // TODO: Add support for far calls with segment:offset addressing
    
    // Call the function
    fprintf(asmFile, "    call _%s\n", node->call.func_name);
    
    // Clean up stack (caller-cleanup convention)
    if (argCount > 0) {
        fprintf(asmFile, "    add sp, %d ; Remove arguments\n", argCount * 2);
    }
    
    // Result is already in ax
}

// Generate code for a return statement
void generateReturnStatement(ASTNode* node) {
    if (!node || node->type != NODE_RETURN) return;
    
    fprintf(asmFile, "    ; Return statement\n");
      // Generate code for return value if present
    if (node->return_stmt.expr) {
        generateExpression(node->return_stmt.expr);
        // Return value is now in AX
    }
    
    // For naked functions, don't generate automatic control flow
    if (!currentFunctionIsNaked) {
        // Jump to function epilogue
        fprintf(asmFile, "    jmp _%s_exit\n", currentFunction);
    } else {
        fprintf(asmFile, "    ; Naked function - no automatic jump to epilogue generated\n");
    }
}

// Generate code for an inline assembly block
void generateAsmBlock(ASTNode* node) {
    if (!node || node->type != NODE_ASM_BLOCK || !node->asm_block.code) return;
    
    fprintf(asmFile, "    ; Inline assembly block\n");
    fprintf(asmFile, "%s\n", node->asm_block.code);
}

// Generate code for an inline assembly statement
void generateAsmStmt(ASTNode* node) {
    if (!node || node->type != NODE_ASM || !node->asm_stmt.code) return;
    
    fprintf(asmFile, "    ; Inline assembly statement\n");
    fprintf(asmFile, "    %s\n", node->asm_stmt.code);
}