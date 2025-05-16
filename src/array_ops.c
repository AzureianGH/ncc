#include "array_ops.h"
#include "codegen.h"
#include "ast.h"
#include "type_support.h"  // For getTypeSize
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations from codegen.c
extern FILE* asmFile;
extern int getVariableOffset(const char* name);
extern int isParameter(const char* name);
extern void generateExpression(ASTNode* node);

// Check if a node represents an array access (a[i])
int isArrayAccess(ASTNode* node) {
    // Array access is represented as a dereference of a binary add operation
    if (node->type == NODE_UNARY_OP && node->unary_op.op == UNARY_DEREFERENCE) {
        ASTNode* addNode = node->right;
        if (addNode && addNode->type == NODE_BINARY_OP && addNode->operation.op == OP_ADD) {
            return 1;
        }
    }
    return 0;
}

// Get array base address in BX and element size in CX
void setupArrayAccess(ASTNode* arrayNode) {
    if (arrayNode->type == NODE_IDENTIFIER) {
        char* name = arrayNode->identifier;
        
        if (isParameter(name)) {
            // Parameter array - get address from BP + offset
            fprintf(asmFile, "    ; Array parameter %s\n", name);
            fprintf(asmFile, "    mov bx, [bp+%d] ; Load array pointer from parameter\n", 
                  -getVariableOffset(name));
        } else {
            // Local variable array - compute address from BP - offset
            fprintf(asmFile, "    ; Array variable %s\n", name);
            fprintf(asmFile, "    lea bx, [bp-%d] ; Load array address\n", 
                  getVariableOffset(name));
        }
    } else {
        // For other expressions, evaluate to get pointer
        generateExpression(arrayNode);
        fprintf(asmFile, "    mov bx, ax ; Move array pointer to BX\n");
    }
}

// Generate code for array indexing
void generateArrayAccess(ASTNode* array, ASTNode* index) {
    // Setup array base address in BX
    setupArrayAccess(array);
    
    // Generate code for index expression (result in AX)
    generateExpression(index);
    
    // Compute element size based on array type (default to 2 bytes for int)
    fprintf(asmFile, "    ; Computing array index\n");
    fprintf(asmFile, "    shl ax, 1 ; Multiply index by 2 (element size for int)\n");
    fprintf(asmFile, "    add bx, ax ; Add offset to array base\n");
    fprintf(asmFile, "    mov ax, [bx] ; Load array element into AX\n");
}

// Get the size of array element based on declaration
int getArrayElementSize(ASTNode* arrayDecl) {
    if (arrayDecl && arrayDecl->type == NODE_DECLARATION) {
        DataType type = arrayDecl->declaration.type_info.type;
        return getTypeSize(type);
    }
    // Default to int (2 bytes)
    return 2;
}

// Generate code for array assignment (arr[i] = value)
void generateArrayAssignment(ASTNode* arrayAccess, ASTNode* value) {
    // First extract the array and index expressions
    ASTNode* derefNode = arrayAccess;
    ASTNode* addNode = derefNode->right;
    ASTNode* array = addNode->left;
    ASTNode* index = addNode->right;
    
    // Evaluate the right-hand side and save it
    generateExpression(value);
    fprintf(asmFile, "    push ax ; Save right-hand side value\n");
    
    // Setup array base address in BX
    setupArrayAccess(array);
    
    // Generate code for index expression (result in AX)
    generateExpression(index);
    
    // Compute element offset
    fprintf(asmFile, "    ; Computing array index for assignment\n");
    fprintf(asmFile, "    shl ax, 1 ; Multiply index by 2 (element size for int)\n");
    fprintf(asmFile, "    add bx, ax ; Add offset to array base\n");
    
    // Store the value in the array element
    fprintf(asmFile, "    pop ax ; Restore right-hand side value\n");
    fprintf(asmFile, "    mov [bx], ax ; Store value in array element\n");
}
