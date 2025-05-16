#include "pointer_ops.h"
#include "codegen.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations from codegen.c
extern FILE* asmFile;
extern void generateExpression(ASTNode* node);
extern int labelCounter;

// Generate code for pointer arithmetic
void generatePointerArithmetic(ASTNode* left, ASTNode* right, OperatorType op) {
    // First generate the left operand (the pointer)
    generateExpression(left);
    fprintf(asmFile, "    push ax ; Save pointer address\n");
    
    // Generate the right operand (the offset or index)
    generateExpression(right);
    
    // For pointer arithmetic, we need to multiply the offset by the size of the target type
    // For now, simplify by assuming all pointers point to 2-byte values (e.g., int)
    fprintf(asmFile, "    ; Pointer arithmetic - scale by target size\n");
    fprintf(asmFile, "    shl ax, 1 ; Multiply offset by 2 (size of int)\n");
    
    // Pop pointer address back to BX
    fprintf(asmFile, "    pop bx ; Restore pointer address\n");
    
    // Perform the operation
    if (op == OP_ADD) {
        fprintf(asmFile, "    add ax, bx ; Pointer addition\n");
    } else if (op == OP_SUB) {
        fprintf(asmFile, "    sub bx, ax ; Pointer subtraction\n");
        fprintf(asmFile, "    mov ax, bx ; Move result to AX\n");
    }
}

// Generate code for pointer comparison
void generatePointerComparison(ASTNode* left, ASTNode* right, OperatorType op) {
    // Generate the pointer addresses
    generateExpression(left);
    fprintf(asmFile, "    push ax ; Save first pointer address\n");
    generateExpression(right);
    fprintf(asmFile, "    pop bx ; Restore first pointer address\n");
    
    // Now AX has second pointer, BX has first pointer
    // Perform the comparison
    switch (op) {
        case OP_EQ:
            fprintf(asmFile, "    cmp bx, ax ; Pointer equality comparison\n");
            fprintf(asmFile, "    mov ax, 0 ; Assume false\n");
            fprintf(asmFile, "    je ptr_eq_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp ptr_eq_end_%d\n", labelCounter);
            fprintf(asmFile, "ptr_eq_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1 ; Set true\n");
            fprintf(asmFile, "ptr_eq_end_%d:\n", labelCounter++);
            break;
            
        case OP_NEQ:
            fprintf(asmFile, "    cmp bx, ax ; Pointer inequality comparison\n");
            fprintf(asmFile, "    mov ax, 0 ; Assume false\n");
            fprintf(asmFile, "    jne ptr_neq_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp ptr_neq_end_%d\n", labelCounter);
            fprintf(asmFile, "ptr_neq_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1 ; Set true\n");
            fprintf(asmFile, "ptr_neq_end_%d:\n", labelCounter++);
            break;
            
        case OP_LT:
            fprintf(asmFile, "    cmp bx, ax ; Pointer less than comparison\n");
            fprintf(asmFile, "    mov ax, 0 ; Assume false\n");
            fprintf(asmFile, "    jl ptr_lt_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp ptr_lt_end_%d\n", labelCounter);
            fprintf(asmFile, "ptr_lt_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1 ; Set true\n");
            fprintf(asmFile, "ptr_lt_end_%d:\n", labelCounter++);
            break;
            
        case OP_LTE:
            fprintf(asmFile, "    cmp bx, ax ; Pointer less than or equal comparison\n");
            fprintf(asmFile, "    mov ax, 0 ; Assume false\n");
            fprintf(asmFile, "    jle ptr_lte_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp ptr_lte_end_%d\n", labelCounter);
            fprintf(asmFile, "ptr_lte_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1 ; Set true\n");
            fprintf(asmFile, "ptr_lte_end_%d:\n", labelCounter++);
            break;
            
        case OP_GT:
            fprintf(asmFile, "    cmp bx, ax ; Pointer greater than comparison\n");
            fprintf(asmFile, "    mov ax, 0 ; Assume false\n");
            fprintf(asmFile, "    jg ptr_gt_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp ptr_gt_end_%d\n", labelCounter);
            fprintf(asmFile, "ptr_gt_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1 ; Set true\n");
            fprintf(asmFile, "ptr_gt_end_%d:\n", labelCounter++);
            break;
            
        case OP_GTE:
            fprintf(asmFile, "    cmp bx, ax ; Pointer greater than or equal comparison\n");
            fprintf(asmFile, "    mov ax, 0 ; Assume false\n");
            fprintf(asmFile, "    jge ptr_gte_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp ptr_gte_end_%d\n", labelCounter);
            fprintf(asmFile, "ptr_gte_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1 ; Set true\n");
            fprintf(asmFile, "ptr_gte_end_%d:\n", labelCounter++);
            break;
            
        default:
            fprintf(stderr, "Warning: Unsupported pointer comparison operator: %d\n", op);
            break;
    }
}

// Generate code for pointer assignment through indirection
void generatePointerAssignment(ASTNode* ptr, ASTNode* value) {
    // Generate the value to assign
    generateExpression(value);
    fprintf(asmFile, "    push ax ; Save value to assign\n");
    
    // Generate pointer address
    generateExpression(ptr);
    fprintf(asmFile, "    mov bx, ax ; Move pointer address to BX\n");
    
    // Restore value and store through the pointer
    fprintf(asmFile, "    pop ax ; Restore value\n");
    fprintf(asmFile, "    mov [bx], ax ; Store value through pointer\n");
}

// Get the size of the type that a pointer points to
int getPointerTargetSize(ASTNode* ptr) {
    // For simplicity, assume all pointers point to 2-byte values (e.g. int)
    // In a full implementation, this would analyze the declaration to determine
    // the exact type the pointer points to
    return 2;
}

// Check if a node is likely a pointer type
int isPointerType(ASTNode* node) {
    // This is a simplified check - a more robust compiler would have full type information
    if (node->type == NODE_IDENTIFIER) {
        // For now we can't easily check if an identifier is pointer without symbol table
        // This would need type tracking in the symbol table
        return 0; // Default conservative assumption
    }
    else if (node->type == NODE_UNARY_OP && node->unary_op.op == UNARY_ADDRESS_OF) {
        // Address-of operation always produces a pointer
        return 1;
    }
    
    return 0; // Default assumption
}
