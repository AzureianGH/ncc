#ifndef ARRAY_OPS_H
#define ARRAY_OPS_H

#include "ast.h"

// Generate code for array indexing
void generateArrayAccess(ASTNode* array, ASTNode* index);

// Check if a node represents an array access
int isArrayAccess(ASTNode* node);

// Get the type size of array element based on declaration
int getArrayElementSize(ASTNode* arrayDecl);

// Generate code for array assignment (arr[i] = value)
void generateArrayAssignment(ASTNode* arrayAccess, ASTNode* value);

#endif // ARRAY_OPS_H
