#include "array_ops.h"
#include "codegen.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    
    // If the index is a literal, we can directly compute the offset
    if (index->type == NODE_LITERAL) {
        int indexValue = index->literal.int_value;
        if (indexValue == 0) {
            // Direct access to first element
            fprintf(asmFile, "    ; Direct access to array element 0\n");
            fprintf(asmFile, "    mov al, [bx] ; Access array[0]\n");
            fprintf(asmFile, "    xor ah, ah ; Clear high byte\n");
        } else {
            // Direct access with fixed offset
            fprintf(asmFile, "    ; Direct access to array element %d\n", indexValue);
            fprintf(asmFile, "    mov al, [bx+%d] ; Access array[%d]\n", indexValue, indexValue);
            fprintf(asmFile, "    xor ah, ah ; Clear high byte\n");
        }
    } else {
        // Variable index needs more complex code
        generateExpression(index);
        fprintf(asmFile, "    ; Computing array access\n");
        fprintf(asmFile, "    add bx, ax ; Add index to base address\n");
        fprintf(asmFile, "    mov al, [bx] ; Load array element\n");
        fprintf(asmFile, "    xor ah, ah ; Clear high byte\n");
    }
}
