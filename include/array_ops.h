#ifndef ARRAY_OPS_H
#define ARRAY_OPS_H

#include "ast.h"

// Generate optimized code for array indexing
void generateOptimizedArrayAccess(ASTNode* array, ASTNode* index);

// Check if a node represents an array access
int isStringLiteralAccess(ASTNode* array, ASTNode* index);

#endif // ARRAY_OPS_H
