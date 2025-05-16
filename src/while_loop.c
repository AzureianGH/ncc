#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include "token_debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations to resolve circular dependencies
extern ASTNode* parseStatement();
extern ASTNode* parseExpression();

// Parse a while loop statement: while (condition) body
ASTNode* parseWhileStatement() {
    ASTNode* node = createNode(NODE_WHILE);
    
    // Consume the 'while' keyword
    consume(TOKEN_WHILE);
    
    // Expect an opening parenthesis
    expect(TOKEN_LPAREN);
    
    // Parse the condition
    node->while_loop.condition = parseExpression();
    
    // Expect a closing parenthesis
    expect(TOKEN_RPAREN);
    
    // Parse the loop body
    node->while_loop.body = parseStatement();
    
    return node;
}
