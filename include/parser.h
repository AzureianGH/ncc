#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

// Parser state
typedef struct Parser {
    Token currentToken;
    Token lookaheadToken;
    
    // Parser options
    struct {
        unsigned int enableC99 : 1;
        unsigned int enableGNU : 1;
        unsigned int enableInlineAsm : 1;
        unsigned int strictMode : 1;
    } options;
    
    // Error handling
    int errorCount;
    int warningCount;
    char* lastError;
    
    // Parse state
    int panicMode;
    int synchronizing;
} Parser;

// Operator precedence levels
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,    // = += -= *= /=
    PREC_TERNARY,       // ?:
    PREC_OR,            // ||
    PREC_AND,           // &&
    PREC_BITWISE_OR,    // |
    PREC_BITWISE_XOR,   // ^
    PREC_BITWISE_AND,   // &
    PREC_EQUALITY,      // == !=
    PREC_COMPARISON,    // < > <= >=
    PREC_SHIFT,         // << >>
    PREC_TERM,          // + -
    PREC_FACTOR,        // * / %
    PREC_UNARY,         // ! ~ + - ++ -- & *
    PREC_POSTFIX,       // [] () . ->
    PREC_PRIMARY
} Precedence;

// Parse rule structure for Pratt parsing
typedef struct ParseRule {
    ASTNode* (*prefix)(void);
    ASTNode* (*infix)(ASTNode* left);
    Precedence precedence;
} ParseRule;

// Function declarations

// Parser initialization and cleanup
void initParser(void);
void cleanupParser(void);
ASTNode* parseProgram(void);

// Top-level parsing
ASTNode* parseTranslationUnit(void);
ASTNode* parseExternalDeclaration(void);

// Declarations
ASTNode* parseDeclaration(void);
ASTNode* parseFunctionDeclaration(void);
ASTNode* parseVariableDeclaration(void);
ASTNode* parseStructDeclaration(void);
ASTNode* parseUnionDeclaration(void);
ASTNode* parseEnumDeclaration(void);
ASTNode* parseTypedefDeclaration(void);

// C99 specific declarations
ASTNode* parseInlineDeclaration(void);
ASTNode* parseStaticAssert(void);

// Type parsing
TypeInfo* parseTypeSpecifier(void);
TypeInfo* parseDeclarationSpecifiers(void);
TypeInfo* parseStorageClassSpecifier(void);
TypeInfo* parseTypeQualifier(void);
TypeInfo* parseFunctionSpecifier(void);
TypeInfo* parsePointer(void);
TypeInfo* parseDirectDeclarator(TypeInfo* baseType);
TypeInfo* parseAbstractDeclarator(void);

// C99 type parsing
TypeInfo* parseComplexType(void);
TypeInfo* parseRestrictQualifier(void);
TypeInfo* parseInlineSpecifier(void);

// Statements
ASTNode* parseStatement(void);
ASTNode* parseCompoundStatement(void);
ASTNode* parseExpressionStatement(void);
ASTNode* parseIfStatement(void);
ASTNode* parseWhileStatement(void);
ASTNode* parseDoWhileStatement(void);
ASTNode* parseForStatement(void);
ASTNode* parseSwitchStatement(void);
ASTNode* parseCaseStatement(void);
ASTNode* parseDefaultStatement(void);
ASTNode* parseBreakStatement(void);
ASTNode* parseContinueStatement(void);
ASTNode* parseReturnStatement(void);
ASTNode* parseGotoStatement(void);
ASTNode* parseLabelStatement(void);

// Expressions - Pratt parser
ASTNode* parseExpression(void);
ASTNode* parseExpressionWithPrecedence(Precedence precedence);
ASTNode* parseAssignmentExpression(void);
ASTNode* parseConditionalExpression(void);
ASTNode* parseLogicalOrExpression(void);
ASTNode* parseLogicalAndExpression(void);
ASTNode* parseBitwiseOrExpression(void);
ASTNode* parseBitwiseXorExpression(void);
ASTNode* parseBitwiseAndExpression(void);
ASTNode* parseEqualityExpression(void);
ASTNode* parseRelationalExpression(void);
ASTNode* parseShiftExpression(void);
ASTNode* parseAdditiveExpression(void);
ASTNode* parseMultiplicativeExpression(void);
ASTNode* parseCastExpression(void);
ASTNode* parseUnaryExpression(void);
ASTNode* parsePostfixExpression(void);
ASTNode* parsePrimaryExpression(void);

// C99 specific expressions
ASTNode* parseCompoundLiteral(void);
ASTNode* parseDesignatedInitializer(void);
ASTNode* parseGenericSelection(void);
ASTNode* parseAlignofExpression(void);

// Initializers
ASTNode* parseInitializer(void);
ASTNode* parseInitializerList(void);
ASTNode* parseDesignation(void);
ASTNode* parseDesignator(void);

// Inline assembly parsing
ASTNode* parseInlineAssembly(void);
ASTNode* parseSimpleInlineAsm(void);
ASTNode* parseExtendedInlineAsm(void);
ASTNode* parseAsmConstraints(void);
ASTNode* parseAsmClobbers(void);

// Attributes (GNU extensions)
ASTNode* parseAttributes(void);
ASTNode* parseAttribute(void);
ASTNode* parseAttributeList(void);

// Utility functions
int match(TokenType type);
int check(TokenType type);
Token advance(void);
int isAtEnd(void);
Token peek(void);
Token previous(void);
void consume(TokenType type, const char* message);

// Error handling
void parserError(int line, int column, const char* format, ...);
void parserErrorAt(Token token, const char* message);
void parserWarning(int line, int column, const char* format, ...);
void synchronize(void);
int hasParserErrors(void);
const char* getLastParserError(void);

// Parse rule table
ParseRule* getParseRule(TokenType type);
void initParseRules(void);

// Precedence utilities
Precedence getPrecedence(TokenType type);
BinaryOperator getTokenBinaryOperator(TokenType type);
UnaryOperator getTokenUnaryOperator(TokenType type);

// Type utilities
int isTypeToken(TokenType type);
int isStorageClassToken(TokenType type);
int isTypeQualifierToken(TokenType type);
int isFunctionSpecifierToken(TokenType type);

// Declarator parsing helpers
char* parseDeclaratorName(TypeInfo** type);
ASTNode* parseParameterList(void);
ASTNode* parseParameter(void);

// Expression utilities
ASTNode* parseConstantExpression(void);
long long evaluateConstantExpression(ASTNode* expr);
int isConstantExpression(ASTNode* expr);

// C99 variadic function support
int parseEllipsis(void);
ASTNode* parseVaArg(void);

// Symbol table integration
void enterScope(void);
void exitScope(void);
void declareVariable(const char* name, TypeInfo* type);
void declareFunction(const char* name, TypeInfo* type);

// Parser state management
void saveParserState(void);
void restoreParserState(void);

// Debugging
void printParserState(void);
void dumpParseStack(void);

#endif // PARSER_H
