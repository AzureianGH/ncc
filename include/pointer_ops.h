#ifndef POINTER_OPS_H
#define POINTER_OPS_H

#include "ast.h"

// Generate code for pointer arithmetic
void generatePointerArithmetic(ASTNode* left, ASTNode* right, OperatorType op);

// Generate code for pointer comparison
void generatePointerComparison(ASTNode* left, ASTNode* right, OperatorType op);

// Generate code for pointer assignment through indirection
void generatePointerAssignment(ASTNode* ptr, ASTNode* value);

// Get the type size for pointer arithmetic
int getPointerTargetSize(ASTNode* ptr);

// Check if a node is likely a pointer type
int isPointerType(ASTNode* node);

#endif // POINTER_OPS_H
