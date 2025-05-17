#ifndef COMPOUND_EXPRESSIONS_H
#define COMPOUND_EXPRESSIONS_H

#include "ast.h"

// Generate code for compound dereference and postfix increment (*ptr++)
void generateDereferencePostfixIncrement(ASTNode* node);

// Function to check if a node represents a *ptr++ pattern
int isDereferencePostfixIncrement(ASTNode* node);

// Generate code for other compound expressions like (*ptr)++, etc.
void generateCompoundExpression(ASTNode* node);

#endif // COMPOUND_EXPRESSIONS_H
