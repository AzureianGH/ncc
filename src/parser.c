#include "parser_simple.h"
#include "lexer.h"
#include "ast.h"
#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Global parser state
static Parser parser;
// Track the last consumed token for correct `previous()` semantics
static Token previousToken;

// Forward declarations
static ASTNode* parseExpression(void);
static ASTNode* parseStatement(void);
static ASTNode* parseDeclaration(void);
static ASTNode* parseLocalDeclaration(void);
static ASTNode* parseVariableDeclaration(void);
static ASTNode* parsePostfix(void);

// Parser initialization
void initParser(void) {
    memset(&parser, 0, sizeof(Parser));
    
    // Set default options
    parser.options.enableC99 = 1;
    parser.options.enableGNU = 1;
    parser.options.enableInlineAsm = 1;
    parser.options.strictMode = 0;
    
    parser.errorCount = 0;
    parser.warningCount = 0;
    parser.lastError = NULL;
    parser.panicMode = 0;
    
    // Initialize token stream
    parser.currentToken = nextToken();
    parser.lookaheadToken = nextToken();
    previousToken = parser.currentToken;
    parser.synchronizing = 0;
}

void cleanupParser(void) {
    if (parser.lastError) {
        free(parser.lastError);
    }
    memset(&parser, 0, sizeof(Parser));
}

// Token management
static Token advance(void) {
    // Don't advance if we're already at EOF
    if (parser.currentToken.type == TOKEN_EOF) {
        static int eofAdvanceCount = 0;
        eofAdvanceCount++;
        if (eofAdvanceCount > 10) {
            // Set a flag to stop parsing gracefully
            parser.panicMode = true;
        }
        return parser.currentToken;
    }
    
    Token prev = parser.currentToken;
    previousToken = prev;
    parser.currentToken = parser.lookaheadToken;
    parser.lookaheadToken = nextToken();
    return prev;
}

static int check(TokenType type) {
    return parser.currentToken.type == type;
}

static int match(TokenType type) {
    if (check(type)) {
        advance();
        return 1;
    }
    return 0;
}

static Token peek(void) {
    return parser.lookaheadToken;
}

static Token previous(void) {
    return previousToken;
}

static int isAtEnd(void) {
    return parser.currentToken.type == TOKEN_EOF;
}

static bool consume(TokenType type, const char* message) {
    if (parser.currentToken.type == type) {
        advance();
        return true;
    }
    
    parserErrorAt(parser.currentToken, message);
    return false;
}

// Error handling
static void parserErrorSimple(const char* message) {
    parserErrorAt(parser.currentToken, message);
}

void parserErrorAt(Token token, const char* message) {
    if (parser.panicMode) return;
    
    parser.panicMode = 1;
    parser.errorCount++;
    
    // Call the error manager function directly
    parserError(token.line, token.column, "%s", message);
    
    if (parser.lastError) {
        free(parser.lastError);
    }
    parser.lastError = strdup(message);
}

static void parserWarningSimple(const char* message) {
    parser.warningCount++;
    parserWarning(parser.currentToken.line, parser.currentToken.column, "%s", message);
}

void synchronize(void) {
    parser.panicMode = 0;
    
    while (parser.currentToken.type != TOKEN_EOF) {
        if (previous().type == TOKEN_SEMICOLON) return;
        
        switch (parser.currentToken.type) {
            case TOKEN_STRUCT:
            case TOKEN_UNION:
            case TOKEN_ENUM:
            case TOKEN_IF:
            case TOKEN_FOR:
            case TOKEN_WHILE:
            case TOKEN_RETURN:
            case TOKEN_BREAK:
            case TOKEN_CONTINUE:
                return;
            default:
                break;
        }
        
        advance();
    }
}

int hasParserErrors(void) {
    return parser.errorCount > 0;
}

const char* getLastParserError(void) {
    return parser.lastError;
}

// Expression parsing (simplified Pratt parser)
static ASTNode* parsePrimary(void) {
    // Check for EOF first
    if (isAtEnd()) {
        return NULL;
    }
    // Gracefully signal "no expression" for common non-starters so callers can decide
    switch (parser.currentToken.type) {
        case TOKEN_RIGHT_BRACE:
        case TOKEN_RIGHT_PAREN:
        case TOKEN_SEMICOLON:
            return NULL;
        default:
            break;
    }
    
    if (match(TOKEN_TRUE)) {
        ASTNode* node = createASTNode(AST_BOOL_LITERAL);
        node->data.literal.intValue = 1;
        return node;
    }
    
    if (match(TOKEN_FALSE)) {
        ASTNode* node = createASTNode(AST_BOOL_LITERAL);
        node->data.literal.intValue = 0;
        return node;
    }
    
    if (match(TOKEN_INTEGER_LITERAL)) {
        ASTNode* node = createASTNode(AST_INTEGER_LITERAL);
        Token prev = previous();
        node->data.literal.intValue = prev.numericValue.intValue;
        node->line = prev.line;
        node->column = prev.column;
        return node;
    }
    
    if (match(TOKEN_DOUBLE_LITERAL)) {
        ASTNode* node = createASTNode(AST_DOUBLE_LITERAL);
        Token prev = previous();
        node->data.literal.floatValue = prev.numericValue.floatValue;
        node->line = prev.line;
        node->column = prev.column;
        return node;
    }
    
    if (match(TOKEN_CHAR_LITERAL)) {
        ASTNode* node = createASTNode(AST_CHAR_LITERAL);
        Token prev = previous();
        node->data.literal.intValue = prev.numericValue.charValue;
        node->line = prev.line;
        node->column = prev.column;
        return node;
    }
    
    if (match(TOKEN_STRING_LITERAL)) {
        ASTNode* node = createASTNode(AST_STRING_LITERAL);
        Token prev = previous();
        node->data.literal.stringValue = strdup(prev.value);
        node->data.literal.stringLength = prev.length;
        node->line = prev.line;
        node->column = prev.column;
        return node;
    }
    
    if (match(TOKEN_IDENTIFIER)) {
        ASTNode* node = createASTNode(AST_IDENTIFIER);
        Token prev = previous();
        node->data.identifier.name = strdup(prev.value);
        node->line = prev.line;
        node->column = prev.column;
        return node;
    }
    
    if (match(TOKEN_LEFT_PAREN)) {
        ASTNode* expr = parseExpression();
        consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }
    
    if (match(TOKEN_SIZEOF)) {
        ASTNode* node = createASTNode(AST_SIZEOF);
        
        if (match(TOKEN_LEFT_PAREN)) {
            // Could be sizeof(type) or sizeof(expression)
            // For now, assume expression
            node->data.unary.operand = parseExpression();
            consume(TOKEN_RIGHT_PAREN, "Expected ')' after sizeof");
        } else {
            node->data.unary.operand = parseExpression();
        }
        
        return node;
    }
    
    parserErrorSimple("Expected expression");
    return NULL;
}

static ASTNode* parseUnary(void) {
    if (match(TOKEN_LOGICAL_NOT) || match(TOKEN_BITWISE_NOT) || 
        match(TOKEN_MINUS) || match(TOKEN_PLUS)) {
        Token operator = previous();
        ASTNode* node = createASTNode(AST_UNARY_OP);
        
        switch (operator.type) {
            case TOKEN_LOGICAL_NOT:
                node->data.unary.op = UNARY_NOT;
                break;
            case TOKEN_BITWISE_NOT:
                node->data.unary.op = UNARY_BITWISE_NOT;
                break;
            case TOKEN_MINUS:
                node->data.unary.op = UNARY_MINUS;
                break;
            case TOKEN_PLUS:
                node->data.unary.op = UNARY_PLUS;
                break;
            default:
                break;
        }
        
        node->data.unary.operand = parseUnary();
        node->line = operator.line;
        node->column = operator.column;
        return node;
    }
    
    if (match(TOKEN_INCREMENT) || match(TOKEN_DECREMENT)) {
        Token operator = previous();
        ASTNode* node = createASTNode(AST_UNARY_OP);
        
        node->data.unary.op = (operator.type == TOKEN_INCREMENT) ? 
                              UNARY_PRE_INCREMENT : UNARY_PRE_DECREMENT;
        node->data.unary.operand = parseUnary();
        node->line = operator.line;
        node->column = operator.column;
        return node;
    }
    
    if (match(TOKEN_BITWISE_AND)) {
        Token operator = previous();
        ASTNode* node = createASTNode(AST_UNARY_OP);
        node->data.unary.op = UNARY_ADDRESS_OF;
        node->data.unary.operand = parseUnary();
        node->line = operator.line;
        node->column = operator.column;
        return node;
    }
    
    if (match(TOKEN_MULTIPLY)) {
        Token operator = previous();
        ASTNode* node = createASTNode(AST_UNARY_OP);
        node->data.unary.op = UNARY_DEREFERENCE;
        node->data.unary.operand = parseUnary();
        node->line = operator.line;
        node->column = operator.column;
        return node;
    }
    
    return parsePostfix();
}

static ASTNode* parsePostfix(void) {
    ASTNode* expr = parsePrimary();
    
    // If parsePrimary returned NULL (likely EOF), return NULL
    if (!expr) {
        return NULL;
    }
    
    while (1) {
        if (match(TOKEN_LEFT_BRACKET)) {
            ASTNode* index = parseExpression();
            consume(TOKEN_RIGHT_BRACKET, "Expected ']' after array index");
            
            ASTNode* node = createASTNode(AST_ARRAY_ACCESS);
            node->data.arrayAccess.array = expr;
            node->data.arrayAccess.index = index;
            expr = node;
        } else if (match(TOKEN_LEFT_PAREN)) {
            ASTNode* node = createASTNode(AST_FUNCTION_CALL);
            node->data.call.function = expr;
            node->data.call.arguments = NULL;
            node->data.call.argCount = 0;
            
            if (!check(TOKEN_RIGHT_PAREN)) {
                do {
                    node->data.call.arguments = realloc(node->data.call.arguments,
                                                       sizeof(ASTNode*) * (node->data.call.argCount + 1));
                    node->data.call.arguments[node->data.call.argCount] = parseExpression();
                    node->data.call.argCount++;
                } while (match(TOKEN_COMMA));
            }
            
            consume(TOKEN_RIGHT_PAREN, "Expected ')' after arguments");
            expr = node;
        } else if (match(TOKEN_DOT)) {
            consume(TOKEN_IDENTIFIER, "Expected member name after '.'");
            Token member = previous();
            
            ASTNode* node = createASTNode(AST_MEMBER_ACCESS);
            node->data.memberAccess.object = expr;
            node->data.memberAccess.memberName = strdup(member.value);
            node->data.memberAccess.isPointer = 0;
            expr = node;
        } else if (match(TOKEN_ARROW)) {
            consume(TOKEN_IDENTIFIER, "Expected member name after '->'");
            Token member = previous();
            
            ASTNode* node = createASTNode(AST_POINTER_ACCESS);
            node->data.memberAccess.object = expr;
            node->data.memberAccess.memberName = strdup(member.value);
            node->data.memberAccess.isPointer = 1;
            expr = node;
        } else if (match(TOKEN_INCREMENT) || match(TOKEN_DECREMENT)) {
            Token operator = previous();
            ASTNode* node = createASTNode(AST_UNARY_OP);
            node->data.unary.op = (operator.type == TOKEN_INCREMENT) ?
                                  UNARY_POST_INCREMENT : UNARY_POST_DECREMENT;
            node->data.unary.operand = expr;
            expr = node;
        } else {
            break;
        }
    }
    
    return expr;
}

static ASTNode* parseBinary(ASTNode* left, int minPrec) {
    while (1) {
        TokenType op = parser.currentToken.type;
        int prec = getPrecedence(op);
        // Stop if current token is not a binary operator or precedence is too low
        if (op == TOKEN_QUESTION) {
            // Ternary operator has lower precedence than assignment; treat specially
            advance(); // consume '?'
            ASTNode* trueExpr = parseExpression();
            consume(TOKEN_COLON, "Expected ':' in ternary expression");
            ASTNode* falseExpr = parseExpression();
            ASTNode* node = createASTNode(AST_TERNARY_OP);
            node->data.ternary.condition = left;
            node->data.ternary.trueExpr = trueExpr;
            node->data.ternary.falseExpr = falseExpr;
            left = node;
            continue;
        }
        if (prec == PREC_NONE || prec < minPrec) break;
        
        Token opTok = advance();
        ASTNode* right = parseUnary();
        if (!right) {
            // Propagate error/null
            return left;
        }
        
        TokenType nextOp = parser.currentToken.type;
        int nextPrec = getPrecedence(nextOp);
        
        if (prec < nextPrec) {
            right = parseBinary(right, prec + 1);
        }
        
        ASTNode* node = createASTNode(AST_BINARY_OP);
        node->data.binary.op = getTokenBinaryOperator(opTok.type);
        node->data.binary.left = left;
        node->data.binary.right = right;
        left = node;
    }
    
    return left;
}

static ASTNode* parseExpression(void) {
    ASTNode* expr = parseUnary();
    if (!expr) return NULL;
    return parseBinary(expr, 0);
}

// Statement parsing
static ASTNode* parseExpressionStatement(void) {
    // Don't try to parse an expression if the block is ending
    if (check(TOKEN_RIGHT_BRACE) || isAtEnd()) {
        return NULL;
    }
    // Allow empty statements ';' without creating an AST node
    if (match(TOKEN_SEMICOLON)) {
        return createASTNode(AST_EXPRESSION_STMT); // empty statement
    }
    ASTNode* expr = parseExpression();
    consume(TOKEN_SEMICOLON, "Expected ';' after expression");
    
    ASTNode* node = createASTNode(AST_EXPRESSION_STMT);
    node->data.returnStmt.expression = expr;
    return node;
}

static ASTNode* parseIfStatement(void) {
    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'if'");
    ASTNode* condition = parseExpression();
    if (!match(TOKEN_RIGHT_PAREN)) {
        const char* got = parser.currentToken.value ? parser.currentToken.value : "(token)";
        parserError(parser.currentToken.line, parser.currentToken.column,
                    "Expected ')' after if condition, got '%s'", got);
        return NULL;
    }
    
    ASTNode* thenBranch = parseStatement();
    ASTNode* elseBranch = NULL;
    
    if (match(TOKEN_ELSE)) {
        elseBranch = parseStatement();
    }
    
    ASTNode* node = createASTNode(AST_IF_STMT);
    node->data.ifStmt.condition = condition;
    node->data.ifStmt.thenStmt = thenBranch;
    node->data.ifStmt.elseStmt = elseBranch;
    
    return node;
}

static ASTNode* parseWhileStatement(void) {
    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'while'");
    ASTNode* condition = parseExpression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after while condition");
    
    ASTNode* body = parseStatement();
    
    ASTNode* node = createASTNode(AST_WHILE_STMT);
    node->data.whileStmt.condition = condition;
    node->data.whileStmt.body = body;
    
    return node;
}

static ASTNode* parseForStatement(void) {
    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'for'");
    
    ASTNode* init = NULL;
    if (!check(TOKEN_SEMICOLON)) {
        if (isTypeToken(parser.currentToken.type) || 
            check(TOKEN_STATIC) || check(TOKEN_EXTERN) || check(TOKEN_AUTO) || check(TOKEN_REGISTER)) {
            // Local declaration in for-init; this will consume its own semicolon
            init = parseLocalDeclaration();
        } else {
            init = parseExpression();
            consume(TOKEN_SEMICOLON, "Expected ';' after for loop initializer");
        }
    } else {
        advance(); // consume semicolon
    }
    
    ASTNode* condition = NULL;
    if (!check(TOKEN_SEMICOLON)) {
        condition = parseExpression();
    }
    consume(TOKEN_SEMICOLON, "Expected ';' after for loop condition");
    
    ASTNode* update = NULL;
    if (!check(TOKEN_RIGHT_PAREN)) {
        update = parseExpression();
    }
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after for clauses");
    
    ASTNode* body = parseStatement();
    
    ASTNode* node = createASTNode(AST_FOR_STMT);
    node->data.forStmt.init = init;
    node->data.forStmt.condition = condition;
    node->data.forStmt.update = update;
    node->data.forStmt.body = body;
    
    return node;
}

static ASTNode* parseReturnStatement(void) {
    ASTNode* expr = NULL;
    if (!check(TOKEN_SEMICOLON)) {
        expr = parseExpression();
    }
    consume(TOKEN_SEMICOLON, "Expected ';' after return value");
    
    ASTNode* node = createASTNode(AST_RETURN_STMT);
    node->data.returnStmt.expression = expr;
    
    return node;
}

static ASTNode* parseCompoundStatement(void) {
    ASTNode* node = createASTNode(AST_COMPOUND_STMT);
    node->data.compound.statements = NULL;
    node->data.compound.count = 0;
    
    while (!check(TOKEN_RIGHT_BRACE) && !isAtEnd()) {
        ASTNode* stmt = parseStatement();
        if (stmt) {
            node->data.compound.statements = realloc(node->data.compound.statements,
                                                   sizeof(ASTNode*) * (node->data.compound.count + 1));
            node->data.compound.statements[node->data.compound.count] = stmt;
            node->data.compound.count++;
        } else {
            // If parseStatement returns NULL, it means we've hit the end of statements
            // This can happen when we encounter '}' or EOF
            break;
        }
    }
    
    consume(TOKEN_RIGHT_BRACE, "Expected '}' after block");
    return node;
}

// Inline assembly parsing
static ASTNode* parseInlineAssembly(void) {
    ASTNode* node = createASTNode(AST_INLINE_ASM);
    
    int isVolatile = 0;
    if (match(TOKEN_VOLATILE)) {
        isVolatile = 1;
    }
    
    consume(TOKEN_LEFT_PAREN, "Expected '(' after asm");
    consume(TOKEN_STRING_LITERAL, "Expected assembly string");
    
    Token asmStr = previous();
    node->data.inlineAsm.assembly = strdup(asmStr.value);
    
    // Now we need to remove the quotes at the start and end
    char* assembly = node->data.inlineAsm.assembly;
    size_t len = strlen(assembly);
    if (len >= 2 && assembly[0] == '"' && assembly[len - 1] == '"') {
        memmove(assembly, assembly + 1, len - 2);
        assembly[len - 2] = '\0';
    }

    node->data.inlineAsm.isVolatile = isVolatile;
    
    // Initialize extended-asm fields
    node->data.inlineAsm.outputCount = 0;
    node->data.inlineAsm.inputCount = 0;
    node->data.inlineAsm.clobberCount = 0;
    node->data.inlineAsm.outputConstraints = NULL;
    node->data.inlineAsm.inputConstraints = NULL;
    node->data.inlineAsm.clobbers = NULL;
    node->data.inlineAsm.outputOperands = NULL;
    node->data.inlineAsm.inputOperands = NULL;
    
    // Try to parse minimal extended asm: 
    //   : "=r" ( identifier )
    // inputs/clobbers are skipped. This is forgiving and best-effort.
    if (match(TOKEN_COLON)) {
        // outputs section starts
        // Accept a single constraint string and an identifier in parentheses
        if (check(TOKEN_STRING_LITERAL)) {
            advance();
            Token cstr = previous();
            // Optional whitespace/commas then ( identifier )
            if (match(TOKEN_LEFT_PAREN)) {
                if (check(TOKEN_IDENTIFIER)) {
                    advance();
                    Token id = previous();
                    consume(TOKEN_RIGHT_PAREN, "Expected ')' after asm output operand");
                    node->data.inlineAsm.outputCount = 1;
                    node->data.inlineAsm.outputConstraints = malloc(sizeof(char*));
                    node->data.inlineAsm.outputOperands = malloc(sizeof(char*));
                    node->data.inlineAsm.outputConstraints[0] = strdup(cstr.value);
                    node->data.inlineAsm.outputOperands[0] = strdup(id.value);
                } else {
                    // No identifier; still consume until ')'
                    while (!check(TOKEN_RIGHT_PAREN) && !isAtEnd()) advance();
                    if (check(TOKEN_RIGHT_PAREN)) advance();
                }
            }
        }

        // Optional inputs section — if a single operand exists and starts with "=", treat as output (compat shim)
        if (match(TOKEN_COLON)) {
            if (check(TOKEN_STRING_LITERAL)) {
                advance();
                Token icstr = previous();
                if (match(TOKEN_LEFT_PAREN)) {
                    if (check(TOKEN_IDENTIFIER)) {
                        advance();
                        Token iid = previous();
                        consume(TOKEN_RIGHT_PAREN, "Expected ')' after asm input operand");
                        const char* s = icstr.value;
                        if (s && s[0] == '"' && s[1] == '=' && node->data.inlineAsm.outputCount == 0) {
                            node->data.inlineAsm.outputCount = 1;
                            node->data.inlineAsm.outputConstraints = malloc(sizeof(char*));
                            node->data.inlineAsm.outputOperands = malloc(sizeof(char*));
                            node->data.inlineAsm.outputConstraints[0] = strdup(icstr.value);
                            node->data.inlineAsm.outputOperands[0] = strdup(iid.value);
                        } else {
                            // Treat as one input operand
                            node->data.inlineAsm.inputCount = 1;
                            node->data.inlineAsm.inputConstraints = malloc(sizeof(char*));
                            node->data.inlineAsm.inputOperands = malloc(sizeof(char*));
                            node->data.inlineAsm.inputConstraints[0] = strdup(icstr.value);
                            node->data.inlineAsm.inputOperands[0] = strdup(iid.value);
                        }
                    }
                }
            }
            // Skip rest of inputs until ':' or ')' to be forgiving
            while (!check(TOKEN_RIGHT_PAREN) && !check(TOKEN_COLON) && !isAtEnd()) advance();
        }
        // Optional clobbers section — skip
        if (match(TOKEN_COLON)) {
            while (!check(TOKEN_RIGHT_PAREN) && !isAtEnd()) advance();
        }
    }
    // Balanced skip to the matching ')' of asm( ... )
    int depth = 1;
    while (depth > 0 && !isAtEnd()) {
        if (match(TOKEN_LEFT_PAREN)) depth++;
        else if (match(TOKEN_RIGHT_PAREN)) depth--;
        else advance();
    }

    if (check(TOKEN_SEMICOLON)) {
        advance();
    }
    
    return node;
}

static ASTNode* parseStatement(void) {
    // If we hit a closing brace, we're done with statements in this block
    if (check(TOKEN_RIGHT_BRACE)) {
        return NULL;
    }
    
    if (isAtEnd()) {
        return NULL;
    }
    
    if (match(TOKEN_IF)) {
        return parseIfStatement();
    }
    
    if (match(TOKEN_WHILE)) {
        return parseWhileStatement();
    }
    
    if (match(TOKEN_FOR)) {
        return parseForStatement();
    }
    
    if (match(TOKEN_RETURN)) {
        return parseReturnStatement();
    }
    
    if (match(TOKEN_LEFT_BRACE)) {
        return parseCompoundStatement();
    }
    
    if (match(TOKEN_ASM)) {
        return parseInlineAssembly();
    }
    
    if (match(TOKEN_BREAK)) {
        consume(TOKEN_SEMICOLON, "Expected ';' after 'break'");
        return createASTNode(AST_BREAK_STMT);
    }
    
    if (match(TOKEN_CONTINUE)) {
        consume(TOKEN_SEMICOLON, "Expected ';' after 'continue'");
        return createASTNode(AST_CONTINUE_STMT);
    }
    
    // Check for declarations
    if (isTypeToken(parser.currentToken.type) || 
        check(TOKEN_STATIC) || check(TOKEN_EXTERN) || check(TOKEN_AUTO) || check(TOKEN_REGISTER)) {
        return parseLocalDeclaration();
    }

    // Only parse an expression statement if the token can start an expression
    switch (parser.currentToken.type) {
        case TOKEN_IDENTIFIER:
        case TOKEN_INTEGER_LITERAL:
        case TOKEN_FLOAT_LITERAL:
        case TOKEN_DOUBLE_LITERAL:
        case TOKEN_LONG_DOUBLE_LITERAL:
        case TOKEN_CHAR_LITERAL:
        case TOKEN_STRING_LITERAL:
        case TOKEN_TRUE:
        case TOKEN_FALSE:
        case TOKEN_LEFT_PAREN:
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_MULTIPLY:      // unary *
        case TOKEN_BITWISE_AND:   // unary &
        case TOKEN_LOGICAL_NOT:
        case TOKEN_BITWISE_NOT:
        case TOKEN_INCREMENT:
        case TOKEN_DECREMENT:
        case TOKEN_SIZEOF:
            return parseExpressionStatement();
        default:
            // Not a valid statement start; let caller handle (e.g., '}' ends block)
            return NULL;
    }
}

// Local declaration parsing (for inside function bodies)
static ASTNode* parseLocalDeclaration(void) {
    // Local declarations are always variable declarations
    // Functions cannot be declared inside other functions in C
    return parseVariableDeclaration();
}

// Declaration parsing (simplified)
static ASTNode* parseVariableDeclaration(void) {
    // Check for EOF first
    if (isAtEnd()) {
        return NULL;
    }
    // Optional storage class specifiers (ignored for now in type info)
    while (check(TOKEN_STATIC) || check(TOKEN_EXTERN) || check(TOKEN_AUTO) || check(TOKEN_REGISTER)) {
        advance();
    }

    // Parse type specifiers
    TypeInfo* type = parseTypeSpecifier();
    if (!type) {
        return NULL;
    }
    // Support pointer declarators like: char *name;
    int ptrDepth = 0;
    while (match(TOKEN_MULTIPLY)) {
        ptrDepth++;
    }
    if (!type) {
        return NULL; // EOF or error in parseTypeSpecifier
    }
    
    if (!consume(TOKEN_IDENTIFIER, "Expected variable name")) {
        // Error recovery: advance past current token to avoid infinite loop
        if (!isAtEnd()) advance();
        return NULL;
    }
    Token name = previous();

    // Parse array declarators after the identifier: name[10][20]...
    int dims[16];
    int dimCount = 0;
    while (match(TOKEN_LEFT_BRACKET)) {
        // Optional constant size (require integer literal for now)
        int size = -1;
        if (check(TOKEN_INTEGER_LITERAL)) {
            Token sz = advance();
            size = (int)sz.numericValue.intValue;
        } else if (!check(TOKEN_RIGHT_BRACKET)) {
            parserError(parser.currentToken.line, parser.currentToken.column, "Expected constant array size inside []");
            return NULL;
        }
        if (!consume(TOKEN_RIGHT_BRACKET, "Expected ']' after array size")) {
            return NULL;
        }
        if (dimCount < (int)(sizeof(dims)/sizeof(dims[0]))) {
            dims[dimCount++] = size;
        } else {
            parserError(parser.currentToken.line, parser.currentToken.column, "Too many array dimensions");
            return NULL;
        }
    }

    // If this is actually a function declarator (e.g., "int foo(...)") handle it here
    if (check(TOKEN_LEFT_PAREN)) {
        // Build the function return type by applying pointer wrapping
        for (int i = 0; i < ptrDepth; i++) {
            TypeInfo* p = createTypeInfo(TYPE_POINTER);
            p->pointsTo = type;
            type = p;
        }

        // Consume '('
        advance();

        // Parse parameters
        Parameter** parameters = NULL;
        int paramCount = 0;
        if (!check(TOKEN_RIGHT_PAREN)) {
            if (check(TOKEN_VOID) && parser.lookaheadToken.type == TOKEN_RIGHT_PAREN) {
                advance(); // void
            } else {
                do {
                    TypeInfo* ptype = parseTypeSpecifier();
                    if (!ptype) return NULL;
                    int pPtrDepth = 0;
                    while (match(TOKEN_MULTIPLY)) {
                        pPtrDepth++;
                    }
                    if (!check(TOKEN_IDENTIFIER)) {
                        parserError(parser.currentToken.line, parser.currentToken.column, "Expected parameter name");
                        return NULL;
                    }
                    Token pname = parser.currentToken;
                    advance();
                    Parameter* p = malloc(sizeof(Parameter));
                    p->name = strdup(pname.value);
                    for (int k = 0; k < pPtrDepth; k++) {
                        TypeInfo* wrap = createTypeInfo(TYPE_POINTER);
                        wrap->pointsTo = ptype;
                        ptype = wrap;
                    }
                    p->type = ptype;
                    parameters = realloc(parameters, sizeof(Parameter*) * (paramCount + 1));
                    parameters[paramCount++] = p;
                } while (match(TOKEN_COMMA));
            }
        }
        if (!consume(TOKEN_RIGHT_PAREN, "Expected ')' after parameters")) {
            return NULL;
        }

        ASTNode* body = NULL;
        if (check(TOKEN_LEFT_BRACE)) {
            advance();
            body = parseCompoundStatement();
        } else {
            if (!consume(TOKEN_SEMICOLON, "Expected ';' after function declaration")) {
                return NULL;
            }
        }

        ASTNode* fnode = createASTNode(AST_FUNCTION_DECL);
        fnode->data.funcDecl.name = strdup(name.value);
        fnode->data.funcDecl.returnType = type;
        fnode->data.funcDecl.parameters = parameters;
        fnode->data.funcDecl.paramCount = paramCount;
        fnode->data.funcDecl.body = body;
        return fnode;
    }

    // Apply pointer wrapping first, then arrays bind to the identifier
    for (int i = 0; i < ptrDepth; i++) {
        TypeInfo* p = createTypeInfo(TYPE_POINTER);
        p->pointsTo = type;
        type = p;
    }
    // Wrap arrays from rightmost to leftmost
    for (int i = dimCount - 1; i >= 0; --i) {
        TypeInfo* arr = createTypeInfo(TYPE_ARRAY);
        arr->elementType = type;
        arr->arraySize = dims[i];
        type = arr;
    }
    
    ASTNode* initializer = NULL;
    if (match(TOKEN_ASSIGN)) {
        initializer = parseExpression();
    }
    
    if (!match(TOKEN_SEMICOLON)) {
        // Provide more context on what we actually saw
        const char* got = parser.currentToken.value ? parser.currentToken.value : "(token)";
        parserError(parser.currentToken.line, parser.currentToken.column,
                    "Expected ';' after variable declaration, got '%s'", got);
        // Error recovery: advance past current token to avoid infinite loop
        if (!isAtEnd()) advance();
        return NULL;
    }
    
    ASTNode* node = createASTNode(AST_VARIABLE_DECL);
    node->data.varDecl.name = strdup(name.value);
    node->data.varDecl.type = type;
    node->data.varDecl.initializer = initializer;
    
    return node;
}

static ASTNode* parseFunctionDeclaration(void) {
    // Parse return type
    TypeInfo* returnType = parseTypeSpecifier();
    if (!returnType) return NULL;
    // Allow pointer return types written like: char *func(...)
    int retPtrDepth = 0;
    while (match(TOKEN_MULTIPLY)) {
        retPtrDepth++;
    }
    
    // Capture the function name token before consuming it
    Token name = parser.currentToken;
    if (!consume(TOKEN_IDENTIFIER, "Expected function name")) {
        return NULL;
    }
    
    if (!consume(TOKEN_LEFT_PAREN, "Expected '(' after function name")) {
        return NULL;
    }

    // Apply pointer wrapping on return type
    for (int i = 0; i < retPtrDepth; i++) {
        TypeInfo* p = createTypeInfo(TYPE_POINTER);
        p->pointsTo = returnType;
        returnType = p;
    }
    
    // Parse parameters: ( [type ident] {, type ident} ) | (void)
    Parameter** parameters = NULL;
    int paramCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        // Handle single 'void' as empty parameter list
        if (check(TOKEN_VOID) && parser.lookaheadToken.type == TOKEN_RIGHT_PAREN) {
            advance(); // void
        } else {
            // Parse first parameter
            do {
                TypeInfo* ptype = parseTypeSpecifier();
                if (!ptype) return NULL;
                int pPtrDepth = 0;
                while (match(TOKEN_MULTIPLY)) {
                    pPtrDepth++;
                }
                if (!check(TOKEN_IDENTIFIER)) {
                    parserError(parser.currentToken.line, parser.currentToken.column, "Expected parameter name");
                    return NULL;
                }
                Token pname = parser.currentToken;
                advance();
                Parameter* p = malloc(sizeof(Parameter));
                p->name = strdup(pname.value);
                for (int k = 0; k < pPtrDepth; k++) {
                    TypeInfo* wrap = createTypeInfo(TYPE_POINTER);
                    wrap->pointsTo = ptype;
                    ptype = wrap;
                }
                p->type = ptype;
                parameters = realloc(parameters, sizeof(Parameter*) * (paramCount + 1));
                parameters[paramCount++] = p;
            } while (match(TOKEN_COMMA));
        }
    }
    if (!consume(TOKEN_RIGHT_PAREN, "Expected ')' after parameters")) {
        return NULL;
    }
    
    ASTNode* body = NULL;
    if (check(TOKEN_LEFT_BRACE)) {
        advance();
        body = parseCompoundStatement();
        
        // After parsing function body, if we're at EOF, ensure proper token state
        if (isAtEnd()) {
            // Function parsing complete, no more tokens expected
        }
    } else {
        if (!consume(TOKEN_SEMICOLON, "Expected ';' after function declaration")) {
            return NULL;
        }
    }
    
    ASTNode* node = createASTNode(AST_FUNCTION_DECL);
    node->data.funcDecl.name = strdup(name.value);
    node->data.funcDecl.returnType = returnType;
    node->data.funcDecl.parameters = parameters;
    node->data.funcDecl.paramCount = paramCount;
    node->data.funcDecl.body = body;
    
    return node;
}

static ASTNode* parseDeclaration(void) {
    // If we're at EOF, don't try to parse anything
    if (isAtEnd()) {
        return NULL;
    }
    
    // Handle C++-style attributes [[...]] preceding a declaration
    int pendingIsNaked = 0;
    int pendingIsDeprecated = 0;
    char* pendingDeprecatedMsg = NULL;
    while (check(TOKEN_LEFT_BRACKET) && parser.lookaheadToken.type == TOKEN_LEFT_BRACKET) {
        // Consume [[
        advance(); advance();
        // Parse a comma-separated list inside [[ ... ]]
        while (!check(TOKEN_RIGHT_BRACKET) && !isAtEnd()) {
            // Attribute identifier
            if (!check(TOKEN_IDENTIFIER)) {
                parserErrorAt(parser.currentToken, "Expected attribute identifier inside [[ ]] ");
                break;
            }
            Token attr = advance();
            int isDeprecated = (strcmp(attr.value, "deprecated") == 0);
            int isNaked = (strcmp(attr.value, "naked") == 0);
            if (isDeprecated) {
                pendingIsDeprecated = 1;
                if (match(TOKEN_LEFT_PAREN)) {
                    if (check(TOKEN_STRING_LITERAL)) {
                        Token msgTok = advance();
                        if (pendingDeprecatedMsg) free(pendingDeprecatedMsg);
                        pendingDeprecatedMsg = strdup(msgTok.value);
                    }
                    consume(TOKEN_RIGHT_PAREN, "Expected ')' after deprecated message");
                }
            } else if (isNaked) {
                pendingIsNaked = 1;
            } else {
                // Unknown attribute: parse optional (...) then ignore
                if (match(TOKEN_LEFT_PAREN)) {
                    // Skip until matching ')'
                    int depth = 1;
                    while (!isAtEnd() && depth > 0) {
                        if (match(TOKEN_LEFT_PAREN)) depth++;
                        else if (match(TOKEN_RIGHT_PAREN)) depth--;
                        else advance();
                    }
                }
            }
            // Comma-separated list
            if (!match(TOKEN_COMMA)) break;
        }
        // Expect closing ]]
        consume(TOKEN_RIGHT_BRACKET, "Expected ']' to close attribute");
        consume(TOKEN_RIGHT_BRACKET, "Expected ']]' to close attribute");
    }

    // At top level, try to parse as function declaration if we see type + identifier
    // Most top-level declarations are functions in simple C programs
    if (isTypeToken(parser.currentToken.type) && 
        parser.lookaheadToken.type == TOKEN_IDENTIFIER) {
        ASTNode* f = parseFunctionDeclaration();
        if (f && f->type == AST_FUNCTION_DECL) {
            f->data.funcDecl.isNaked = pendingIsNaked;
            f->data.funcDecl.isDeprecated = pendingIsDeprecated;
            f->data.funcDecl.deprecatedMessage = pendingDeprecatedMsg ? strdup(pendingDeprecatedMsg) : NULL;
        }
        if (pendingDeprecatedMsg) free(pendingDeprecatedMsg);
        return f;
    }
    
    return parseVariableDeclaration();
}

// Type parsing (simplified)
TypeInfo* parseTypeSpecifier(void) {
    // Accept combinations like: unsigned int, long long, long double, short, etc.
    if (isAtEnd()) return NULL;

    int sawUnsigned = 0;
    int sawSigned = 0;
    int longCount = 0;   // 0: none, 1: long, 2: long long
    int sawShort = 0;
    DataType explicitBase = TYPE_UNKNOWN; // if a concrete base like int/char/double is seen

    // Consume a sequence of type specifiers/modifiers in any reasonable order
    int progressed = 1;
    while (!isAtEnd() && progressed) {
        progressed = 0;
        if (match(TOKEN_UNSIGNED)) { sawUnsigned = 1; progressed = 1; continue; }
        if (match(TOKEN_SIGNED))   { sawSigned = 1;   progressed = 1; continue; }
        if (match(TOKEN_SHORT))    { sawShort = 1;    progressed = 1; continue; }
        if (match(TOKEN_LONG)) {
            longCount++;
            if (longCount > 2) longCount = 2; // cap at 'long long'
            progressed = 1;
            continue;
        }
        if (match(TOKEN_VOID))    { explicitBase = TYPE_VOID;   progressed = 1; break; }
        if (match(TOKEN_CHAR))    { explicitBase = TYPE_CHAR;   progressed = 1; break; }
        if (match(TOKEN_INT))     { explicitBase = TYPE_INT;    progressed = 1; break; }
        if (match(TOKEN_FLOAT))   { explicitBase = TYPE_FLOAT;  progressed = 1; break; }
        if (peek().type == TOKEN_DOUBLE) {
            // Handle 'long double' specially; don't consume here to allow preceding 'long'
            match(TOKEN_DOUBLE);
            explicitBase = TYPE_DOUBLE;
            progressed = 1;
            break;
        }
        if (match(TOKEN_BOOL))    { explicitBase = TYPE_BOOL;   progressed = 1; break; }
    }

    DataType baseType = TYPE_INT; // default if only modifiers present

    if (explicitBase == TYPE_VOID) {
        baseType = TYPE_VOID;
    } else if (explicitBase == TYPE_CHAR) {
        baseType = TYPE_CHAR; // signed/unsigned ignored for now
    } else if (explicitBase == TYPE_INT) {
        // Apply length modifiers to int
        if (sawShort) baseType = TYPE_SHORT;
        else if (longCount == 2) baseType = TYPE_LONG_LONG;
        else if (longCount == 1) baseType = TYPE_LONG;
        else baseType = TYPE_INT;
    } else if (explicitBase == TYPE_FLOAT) {
        // 'long float' isn't standard; treat as float
        baseType = TYPE_FLOAT;
    } else if (explicitBase == TYPE_DOUBLE) {
        // 'long double'
        if (longCount >= 1) baseType = TYPE_LONG_DOUBLE;
        else baseType = TYPE_DOUBLE;
    } else if (explicitBase == TYPE_BOOL) {
        baseType = TYPE_BOOL;
    } else {
        // No explicit base, only modifiers like 'short' or 'long long'
        if (sawShort) baseType = TYPE_SHORT;
        else if (longCount == 2) baseType = TYPE_LONG_LONG;
        else if (longCount == 1) baseType = TYPE_LONG;
        else baseType = TYPE_INT; // plain 'signed'/'unsigned' => int (signedness ignored)
    }

    return createTypeInfo(baseType);
}

// Utility functions
int isTypeToken(TokenType type) {
    return type == TOKEN_VOID || type == TOKEN_CHAR || type == TOKEN_SHORT ||
           type == TOKEN_INT || type == TOKEN_LONG || type == TOKEN_FLOAT ||
           type == TOKEN_DOUBLE || type == TOKEN_SIGNED || type == TOKEN_UNSIGNED ||
           type == TOKEN_BOOL || type == TOKEN_STRUCT || type == TOKEN_UNION ||
           type == TOKEN_ENUM;
}

int isStorageClassToken(TokenType type) {
    return type == TOKEN_STATIC || type == TOKEN_EXTERN || type == TOKEN_AUTO ||
           type == TOKEN_REGISTER || type == TOKEN_TYPEDEF;
}

Precedence getPrecedence(TokenType type) {
    switch (type) {
        case TOKEN_ASSIGN:
        case TOKEN_PLUS_ASSIGN:
        case TOKEN_MINUS_ASSIGN:
        case TOKEN_MULTIPLY_ASSIGN:
        case TOKEN_DIVIDE_ASSIGN:
        case TOKEN_MODULO_ASSIGN:
            return PREC_ASSIGNMENT;
        case TOKEN_LOGICAL_OR:
            return PREC_OR;
        case TOKEN_LOGICAL_AND:
            return PREC_AND;
        case TOKEN_BITWISE_OR:
            return PREC_BITWISE_OR;
        case TOKEN_BITWISE_XOR:
            return PREC_BITWISE_XOR;
        case TOKEN_BITWISE_AND:
            return PREC_BITWISE_AND;
        case TOKEN_EQUAL:
        case TOKEN_NOT_EQUAL:
            return PREC_EQUALITY;
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL:
        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL:
            return PREC_COMPARISON;
        case TOKEN_LEFT_SHIFT:
        case TOKEN_RIGHT_SHIFT:
            return PREC_SHIFT;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return PREC_TERM;
        case TOKEN_MULTIPLY:
        case TOKEN_DIVIDE:
        case TOKEN_MODULO:
            return PREC_FACTOR;
        default:
            return PREC_NONE;
    }
}

BinaryOperator getTokenBinaryOperator(TokenType type) {
    switch (type) {
        case TOKEN_PLUS: return BIN_ADD;
        case TOKEN_MINUS: return BIN_SUB;
        case TOKEN_MULTIPLY: return BIN_MUL;
        case TOKEN_DIVIDE: return BIN_DIV;
        case TOKEN_MODULO: return BIN_MOD;
    case TOKEN_PLUS_ASSIGN: return BIN_ADD_ASSIGN;
    case TOKEN_MINUS_ASSIGN: return BIN_SUB_ASSIGN;
    case TOKEN_MULTIPLY_ASSIGN: return BIN_MUL_ASSIGN;
    case TOKEN_DIVIDE_ASSIGN: return BIN_DIV_ASSIGN;
    case TOKEN_MODULO_ASSIGN: return BIN_MOD_ASSIGN;
    case TOKEN_AND_ASSIGN: return BIN_AND_ASSIGN;
    case TOKEN_OR_ASSIGN: return BIN_OR_ASSIGN;
    case TOKEN_XOR_ASSIGN: return BIN_XOR_ASSIGN;
    case TOKEN_LEFT_SHIFT_ASSIGN: return BIN_LEFT_SHIFT_ASSIGN;
    case TOKEN_RIGHT_SHIFT_ASSIGN: return BIN_RIGHT_SHIFT_ASSIGN;
        case TOKEN_EQUAL: return BIN_EQ;
        case TOKEN_NOT_EQUAL: return BIN_NE;
        case TOKEN_LESS: return BIN_LT;
        case TOKEN_LESS_EQUAL: return BIN_LE;
        case TOKEN_GREATER: return BIN_GT;
        case TOKEN_GREATER_EQUAL: return BIN_GE;
        case TOKEN_LOGICAL_AND: return BIN_LOGICAL_AND;
        case TOKEN_LOGICAL_OR: return BIN_LOGICAL_OR;
        case TOKEN_BITWISE_AND: return BIN_BITWISE_AND;
        case TOKEN_BITWISE_OR: return BIN_BITWISE_OR;
        case TOKEN_BITWISE_XOR: return BIN_BITWISE_XOR;
        case TOKEN_LEFT_SHIFT: return BIN_LEFT_SHIFT;
        case TOKEN_RIGHT_SHIFT: return BIN_RIGHT_SHIFT;
        case TOKEN_ASSIGN: return BIN_ASSIGN;
        default: return BIN_ADD; // fallback
    }
}

// Main parsing function
ASTNode* parseProgram(void) {
    ASTNode* program = createASTNode(AST_PROGRAM);
    program->data.compound.statements = NULL;
    program->data.compound.count = 0;
    
    while (!isAtEnd() && !parser.panicMode) {
        Token prevToken = parser.currentToken; // Save token to detect infinite loops
        
        ASTNode* decl = parseDeclaration();
        if (decl) {
            program->data.compound.statements = realloc(program->data.compound.statements,
                                                      sizeof(ASTNode*) * (program->data.compound.count + 1));
            program->data.compound.statements[program->data.compound.count] = decl;
            program->data.compound.count++;
            
            // After successfully parsing a declaration, check if we should continue
            if (isAtEnd()) {
                break;
            }
        } else {
            // If parseDeclaration returns NULL and we haven't advanced, we're likely at EOF
            if (parser.currentToken.type == prevToken.type && 
                parser.currentToken.line == prevToken.line &&
                parser.currentToken.column == prevToken.column) {
                
                // If we're stuck on EOF, break out of the loop
                if (parser.currentToken.type == TOKEN_EOF) {
                    break;
                }
                
                parserErrorAt(parser.currentToken, "Unexpected token, skipping");
                advance(); // Force advancement to prevent infinite loop
                
                // Additional safety: if we've advanced but still have the same EOF token, break
                if (parser.currentToken.type == TOKEN_EOF) {
                    break;
                }
            }
        }
        
        if (parser.panicMode) {
            synchronize();
        }
    }
    
    return program;
}
