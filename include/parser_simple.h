#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

// Parser options
typedef struct {
    int enableC99;
    int enableGNU;
    int enableInlineAsm;
    int strictMode;
} ParserOptions;

// Parser state
typedef struct {
    Token currentToken;
    Token lookaheadToken;
    ParserOptions options;
    int errorCount;
    int warningCount;
    char* lastError;
    int panicMode;
    int synchronizing;
} Parser;

// Precedence levels for expression parsing
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_BITWISE_OR,
    PREC_BITWISE_XOR,
    PREC_BITWISE_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_SHIFT,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY
} Precedence;

// Parser initialization and cleanup
void initParser(void);
void cleanupParser(void);

// Main parsing function
ASTNode* parseProgram(void);

// Error handling (note: these are implemented in error_manager.c)
void parserErrorAt(Token token, const char* message);
void synchronize(void);
int hasParserErrors(void);
const char* getLastParserError(void);

// Utility functions
int isTypeToken(TokenType type);
int isStorageClassToken(TokenType type);
Precedence getPrecedence(TokenType type);
BinaryOperator getTokenBinaryOperator(TokenType type);
TypeInfo* parseTypeSpecifier(void);

#endif // PARSER_H
