#include "ast.h"
#include "codegen.h"
#include "struct_support.h"
#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External declarations
extern TypeInfo* getTypeInfoFromExpression(ASTNode* expr);
extern TypeInfo* getTypeInfo(const char* name);
extern const char* getCurrentFunctionName();
extern int localVarCount;
extern FILE* asmFile;
extern int getLocalVarOffset(const char* name);

// Generate code to load the address of an expression into AX
// This is useful for struct member access and address-of operations
void generateAddressOf(ASTNode* expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case NODE_IDENTIFIER:
            {
                // Check if it's a local variable or global
                int offset = getLocalVarOffset(expr->identifier);
                if (offset != 0) {
                    // Local variable - address is BP + offset
                    fprintf(asmFile, "    lea ax, [bp-%d]  ; Address of local var %s\n", 
                            offset, expr->identifier);
                } else {
                    // Global variable - use its symbol
                    fprintf(asmFile, "    mov ax, offset _%s  ; Address of global var %s\n", 
                            expr->identifier, expr->identifier);
                }
            }
            break;
        
        case NODE_UNARY_OP:
            if (expr->unary_op.op == UNARY_DEREFERENCE) {
                // For a dereference (*ptr), the address is the value of ptr
                generateExpression(expr->right);
                // Value is now in AX, which is the address we want
            } else {
                reportError(-1, "Cannot take address of this expression");
            }
            break;
        
        case NODE_MEMBER_ACCESS:
            if (expr->member_access.op == OP_DOT) {
                // For s.m, get address of s, then add member offset
                generateAddressOf(expr->left);
                
                // Get the struct type
                TypeInfo* baseType = getTypeInfoFromExpression(expr->left);
                if (baseType && baseType->type == TYPE_STRUCT) {
                    int offset = getMemberOffset(baseType->struct_info, expr->member_access.member_name);
                    if (offset > 0) {
                        fprintf(asmFile, "    add ax, %d  ; Add member offset to struct address\n", offset);
                    }
                }
            } else if (expr->member_access.op == OP_ARROW) {
                // For p->m, the address is p + member offset
                generateExpression(expr->left);
                
                // Get the struct pointer type
                TypeInfo* baseType = getTypeInfoFromExpression(expr->left);
                if (baseType && baseType->type == TYPE_STRUCT && baseType->is_pointer) {
                    int offset = getMemberOffset(baseType->struct_info, expr->member_access.member_name);
                    if (offset > 0) {
                        fprintf(asmFile, "    add ax, %d  ; Add member offset to struct pointer\n", offset);
                    }
                }
            }
            break;
        
        case NODE_BINARY_OP:
            if (expr->operation.op == OP_ADD || expr->operation.op == OP_SUB) {
                // Handle pointer arithmetic for array indexing
                TypeInfo* leftType = getTypeInfoFromExpression(expr->left);
                if (leftType && leftType->is_pointer) {
                    // Generate the base pointer
                    generateExpression(expr->left);
                    fprintf(asmFile, "    push ax  ; Save base address\n");
                    
                    // Generate the index
                    generateExpression(expr->right);
                    
                    // Calculate element size
                    int elementSize = 1;
                    if (leftType->type == TYPE_INT || leftType->type == TYPE_SHORT ||
                        leftType->type == TYPE_UNSIGNED_INT || leftType->type == TYPE_UNSIGNED_SHORT) {
                        elementSize = 2;
                    } else if (leftType->type == TYPE_LONG || leftType->type == TYPE_UNSIGNED_LONG) {
                        elementSize = 4;
                    } else if (leftType->type == TYPE_STRUCT && leftType->struct_info) {
                        elementSize = leftType->struct_info->size;
                    }
                    
                    // Multiply index by element size if needed
                    if (elementSize > 1) {
                        if (elementSize == 2) {
                            fprintf(asmFile, "    shl ax, 1  ; Multiply index by 2\n");
                        } else if (elementSize == 4) {
                            fprintf(asmFile, "    shl ax, 2  ; Multiply index by 4\n");
                        } else {
                            fprintf(asmFile, "    mov cx, %d  ; Element size\n", elementSize);
                            fprintf(asmFile, "    mul cx      ; Multiply index by element size\n");
                        }
                    }
                    
                    // Add or subtract the offset
                    fprintf(asmFile, "    pop bx   ; Restore base address\n");
                    if (expr->operation.op == OP_ADD) {
                        fprintf(asmFile, "    add ax, bx  ; Add offset to base\n");
                    } else {
                        fprintf(asmFile, "    sub bx, ax  ; Subtract offset from base\n");
                        fprintf(asmFile, "    mov ax, bx  ; Result to AX\n");
                    }
                } else {
                    reportError(-1, "Cannot take address of this arithmetic expression");
                }
            } else {
                reportError(-1, "Cannot take address of this binary expression");
            }
            break;
        
        default:
            reportError(-1, "Cannot take address of this expression type");
            break;
    }
}

// Generate code to get the size of a struct
void generateStructSizeOf(StructInfo* structInfo) {
    if (!structInfo) {
        reportError(-1, "Cannot get size of unknown struct");
        return;
    }
    
    // Simply load the struct size into AX
    fprintf(asmFile, "    mov ax, %d  ; Size of struct %s\n", structInfo->size, structInfo->name);
}
