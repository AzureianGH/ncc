#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Parse a unary expression
ASTNode* parseUnaryExpression() {
    if (tokenIs(TOKEN_AMPERSAND)) {
        // Address-of operator (&)
        consume(TOKEN_AMPERSAND);
        ASTNode* operand = parseUnaryExpression();
        
        ASTNode* node = createNode(NODE_UNARY_OP);
        node->operation.op = UNARY_ADDRESS_OF;
        node->right = operand;
        return node;
    }
    else if (tokenIs(TOKEN_STAR)) {
        // Dereference operator (*)
        consume(TOKEN_STAR);
        ASTNode* operand = parseUnaryExpression();
        
        ASTNode* node = createNode(NODE_UNARY_OP);
        node->operation.op = UNARY_DEREFERENCE;
        node->right = operand;
        return node;
    }
    else if (tokenIs(TOKEN_MINUS)) {
        // Unary minus (-)
        consume(TOKEN_MINUS);
        ASTNode* operand = parseUnaryExpression();
        
        ASTNode* node = createNode(NODE_UNARY_OP);
        node->operation.op = UNARY_NEGATE;
        node->right = operand;
        return node;
    }
    else if (tokenIs(TOKEN_NOT)) {
        // Logical not (!)
        consume(TOKEN_NOT);
        ASTNode* operand = parseUnaryExpression();
        
        ASTNode* node = createNode(NODE_UNARY_OP);
        node->operation.op = UNARY_NOT;
        node->right = operand;
        return node;
    }
    
    // If no unary operator is found, parse as a postfix expression
    return parsePostfixExpression();
}

// Parse a postfix expression
ASTNode* parsePostfixExpression() {
    ASTNode* left = parsePrimaryExpression();
    
    // Handle array subscript [index]
    while (tokenIs(TOKEN_LBRACKET)) {
        consume(TOKEN_LBRACKET);
        ASTNode* index = parseExpression();
        expect(TOKEN_RBRACKET);
        
        // Create a binary operation node for array indexing
        // This is equivalent to *(left + index)
        ASTNode* addNode = createNode(NODE_BINARY_OP);
        addNode->operation.op = OP_ADD;
        addNode->left = left;
        addNode->right = index;
        
        // Create a unary dereference node
        ASTNode* derefNode = createNode(NODE_UNARY_OP);
        derefNode->operation.op = UNARY_DEREFERENCE;
        derefNode->right = addNode;
        
        left = derefNode;
    }
    
    return left;
}
