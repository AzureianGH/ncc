#ifndef UNARY_OPS_H
#define UNARY_OPS_H

#include "ast.h"

// Parse a unary expression
ASTNode* parseUnaryExpression();

// Parse a postfix expression
ASTNode* parsePostfixExpression();

// Generate code for unary operations
void generateUnaryOp(ASTNode* node);

#endif // UNARY_OPS_H
