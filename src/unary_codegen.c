#include "codegen.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration from codegen.c
extern FILE* asmFile;
extern int getVariableOffset(const char* name);
extern int isParameter(const char* name);

// Generate code for unary operations
void generateUnaryOp(ASTNode* node) {
    if (!node || node->type != NODE_UNARY_OP) return;
    
    switch (node->unary_op.op) {
        case UNARY_DEREFERENCE:
            // Generate code to evaluate the pointer expression
            generateExpression(node->right);
            
            // AX now contains the address, dereference by loading from [AX]
            fprintf(asmFile, "    ; Dereferencing pointer\n");
            fprintf(asmFile, "    mov bx, ax ; Move address to BX\n");
            fprintf(asmFile, "    mov ax, [bx] ; Dereference pointer\n");
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
            
        default:
            fprintf(stderr, "Warning: Unsupported unary operator: %d\n", node->unary_op.op);
            break;
    }
}
