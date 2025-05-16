#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations
extern ASTNode* parseStatement();
extern ASTNode* parseExpression();

// Parse an if statement: if (condition) statement [else statement]
ASTNode* parseIfStatement() {
    ASTNode* node = createNode(NODE_IF);
    
    // Consume the 'if' keyword
    consume(TOKEN_IF);
    
    // Expect opening parenthesis
    expect(TOKEN_LPAREN);
    
    // Parse condition
    node->if_stmt.condition = parseExpression();
    
    // Expect closing parenthesis
    expect(TOKEN_RPAREN);
    
    // Parse if body
    node->if_stmt.if_body = parseStatement();
    
    // Check for else
    if (tokenIs(TOKEN_ELSE)) {
        consume(TOKEN_ELSE);
        node->if_stmt.else_body = parseStatement();
    } else {
        node->if_stmt.else_body = NULL;
    }
    
    return node;
}
