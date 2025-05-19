#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

// Initialize parser
void initParser();

// Parse a program
ASTNode* parseProgram();

// Parse a declaration (variable or function)
ASTNode* parseDeclaration();

// Parse a function definition
ASTNode* parseFunctionDefinition(char* name, TypeInfo returnType);

// Parse a parameter
ASTNode* parseParameter();

// Parse a variable declaration
ASTNode* parseVariableDeclaration(char* name, TypeInfo typeInfo);

// Parse a block of statements
ASTNode* parseBlock();

// Parse a statement
ASTNode* parseStatement();

// Parse an expression statement
ASTNode* parseExpressionStatement();

// Parse a return statement
ASTNode* parseReturnStatement();

// Parse a for statement
ASTNode* parseForStatement();

// Parse a while statement
ASTNode* parseWhileStatement();

// Parse a do-while statement
ASTNode* parseDoWhileStatement();

// Parse an if statement
ASTNode* parseIfStatement();

// Parse an inline assembly block
ASTNode* parseAsmBlock();

// Parse an expression
ASTNode* parseExpression();

// Parse an assignment expression
ASTNode* parseAssignmentExpression();

// Parse a logical OR expression
ASTNode* parseLogicalOrExpression();

// Parse a logical AND expression
ASTNode* parseLogicalAndExpression();

// Parse a equality expression
ASTNode* parseEqualityExpression();

// Parse a relational expression
ASTNode* parseRelationalExpression();

// Parse an additive expression
ASTNode* parseAdditiveExpression();

// Parse a multiplicative expression
ASTNode* parseMultiplicativeExpression();

// Parse a unary expression
ASTNode* parseUnaryExpression();

// Parse a postfix expression
ASTNode* parsePostfixExpression();

// Parse a primary expression
ASTNode* parsePrimaryExpression();

// Parse a type
TypeInfo parseType();

// Expect a specific token type
void expect(TokenType type);

// Parse expressions with bitwise operators
ASTNode* parseBitwiseExpression();

// Parse expressions with shift operators
ASTNode* parseShiftExpression();

// Is a type name
int isTypeName(Token token);

#endif // PARSER_H