#include "array_ops.h"
#include "codegen.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration of getTypeInfo
TypeInfo* getTypeInfo(const char* name);

// Forward declarations from codegen.c
extern FILE* asmFile;
extern int getVariableOffset(const char* name);
extern int isParameter(const char* name);
extern void generateExpression(ASTNode* node);

// Check if we're accessing a string literal or array
int isStringLiteralAccess(ASTNode* array, ASTNode* index) {
    // Check if the array is a variable that points to a string literal
    if (array->type == NODE_IDENTIFIER) {
        // We'd need symbol table lookup to know if this is a string literal
        // For now, return false - in a more complete implementation we could
        // check the symbol table
        return 0;
    }
    
    // Simple case: literal integer index
    if (index->type == NODE_LITERAL && 
        (index->literal.data_type == TYPE_INT || index->literal.data_type == TYPE_CHAR)) {
        return 1;
    }
    
    return 0;
}

// Generate optimized code for array indexing with literal index
void generateOptimizedArrayAccess(ASTNode* array, ASTNode* index) {
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
      // Determine array element type
    TypeInfo* arrayTypeInfo = NULL;
    int elementSize = 1; // Default to byte size (char)
    
    if (array->type == NODE_IDENTIFIER) {
        arrayTypeInfo = getTypeInfo(array->identifier);
        if (arrayTypeInfo) {
            // Check element type (for array of pointers, etc.)
            if (arrayTypeInfo->type == TYPE_INT || 
                arrayTypeInfo->type == TYPE_SHORT || 
                arrayTypeInfo->type == TYPE_UNSIGNED_INT || 
                arrayTypeInfo->type == TYPE_UNSIGNED_SHORT) {
                elementSize = 2; // Word size for int/short
            }
        }
    }
    
    // If the index is a literal, we can directly compute the offset
    if (index->type == NODE_LITERAL) {
        int indexValue = index->literal.int_value;
        int offset = indexValue * elementSize; // Scale by element size
        
        if (indexValue == 0) {
            // Direct access to first element
            fprintf(asmFile, "    ; Direct access to array element 0\n");
            if (elementSize == 1) {
                fprintf(asmFile, "    mov al, [bx] ; Access byte array[0]\n");
                fprintf(asmFile, "    xor ah, ah ; Clear high byte\n");
            } else {
                fprintf(asmFile, "    mov ax, [bx] ; Access word array[0]\n");
            }
        } else {
            // Direct access with fixed offset
            fprintf(asmFile, "    ; Direct access to array element %d (offset %d)\n", indexValue, offset);
            if (elementSize == 1) {
                fprintf(asmFile, "    mov al, [bx+%d] ; Access byte array[%d]\n", offset, indexValue);
                fprintf(asmFile, "    xor ah, ah ; Clear high byte\n");
            } else {
                fprintf(asmFile, "    mov ax, [bx+%d] ; Access word array[%d]\n", offset, indexValue);
            }
        }
    } else {
        // Variable index needs more complex code
        generateExpression(index);
        
        // Scale index by element size if needed
        if (elementSize > 1) {
            fprintf(asmFile, "    ; Scale index by element size (%d)\n", elementSize);
            if (elementSize == 2) {
                fprintf(asmFile, "    shl ax, 1 ; Multiply index by 2 for word elements\n");
            } else if (elementSize == 4) {
                fprintf(asmFile, "    shl ax, 2 ; Multiply index by 4 for dword elements\n");
            }
        }
        
        fprintf(asmFile, "    ; Computing array access\n");
        fprintf(asmFile, "    add bx, ax ; Add scaled index to base address\n");
        
        if (elementSize == 1) {
            fprintf(asmFile, "    mov al, [bx] ; Load byte array element\n");
            fprintf(asmFile, "    xor ah, ah ; Clear high byte\n");
        } else {
            fprintf(asmFile, "    mov ax, [bx] ; Load word array element\n");
        }
    }
}
