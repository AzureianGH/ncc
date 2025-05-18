#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "ast.h"

// Add a symbol to the table for type checking
void addTypeSymbol(const char* name, TypeInfo type);

// Find a symbol in the table
TypeInfo* findTypeSymbol(const char* name);

// Function to check if a node's type is a void pointer
int isVoidPointer(ASTNode* node);

// Function to check if a node represents a dereferenced void pointer
int isVoidPointerDereference(ASTNode* node);

#endif // TYPE_CHECKER_H
