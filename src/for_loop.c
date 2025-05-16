#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations to resolve circular dependencies
extern ASTNode* parseStatement();
extern ASTNode* parseExpression();
extern ASTNode* parseExpressionStatement();
extern ASTNode* parseDeclaration();

// Parse a for loop statement: for (init; condition; update) body
ASTNode* parseForStatement() {
    ASTNode* node = createNode(NODE_FOR);
    
    // Consume the 'for' keyword
    consume(TOKEN_FOR);
    
    // Expect opening parenthesis
    expect(TOKEN_LPAREN);
    
    // Parse initialization part: can be either a declaration or an expression
    if (tokenIs(TOKEN_INT) || tokenIs(TOKEN_SHORT) || tokenIs(TOKEN_CHAR) || 
        tokenIs(TOKEN_VOID) || tokenIs(TOKEN_UNSIGNED)) {
        // This is a declaration
        node->for_loop.init = parseDeclaration();
    } else if (!tokenIs(TOKEN_SEMICOLON)) {
        // This is an expression
        ASTNode* expr_node = createNode(NODE_EXPRESSION);
        expr_node->left = parseExpression();
        node->for_loop.init = expr_node;
        
        // We need to consume the semicolon manually
        expect(TOKEN_SEMICOLON);
    } else {
        // Empty initialization
        consume(TOKEN_SEMICOLON);
        node->for_loop.init = NULL;
    }
    
    // Parse condition part
    if (!tokenIs(TOKEN_SEMICOLON)) {
        node->for_loop.condition = parseExpression();
    } else {
        // No condition means "always true"
        node->for_loop.condition = NULL;
    }
    
    // Consume semicolon after condition
    expect(TOKEN_SEMICOLON);
    
    // Parse update part
    if (!tokenIs(TOKEN_RPAREN)) {
        ASTNode* update_expr = parseExpression();
        // Wrap in an expression statement node
        ASTNode* update_stmt = createNode(NODE_EXPRESSION);
        update_stmt->left = update_expr;
        node->for_loop.update = update_stmt;
    } else {
        // No update expression
        node->for_loop.update = NULL;
    }
    
    // Expect closing parenthesis
    expect(TOKEN_RPAREN);
    
    // Parse the loop body
    node->for_loop.body = parseStatement();
    
    return node;
}
