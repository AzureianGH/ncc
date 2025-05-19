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

// Parse a do-while loop statement: do statement while (condition);
ASTNode* parseDoWhileStatement() {
    ASTNode* node = createNode(NODE_DO_WHILE);
    
    // Consume the 'do' keyword
    consume(TOKEN_DO);
    
    // Parse the loop body
    node->do_while_loop.body = parseStatement();
    
    // Expect the 'while' keyword
    expect(TOKEN_WHILE);
    
    // Expect an opening parenthesis
    expect(TOKEN_LPAREN);
    
    // Parse the condition
    node->do_while_loop.condition = parseExpression();
    
    // Expect a closing parenthesis
    expect(TOKEN_RPAREN);
    
    // Expect a semicolon
    expect(TOKEN_SEMICOLON);
    
    return node;
}
