#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include "error_manager.h"
#include "type_checker.h"
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
    }    else if (tokenIs(TOKEN_STAR)) {
        // Dereference operator (*)
        consume(TOKEN_STAR);
        ASTNode* operand = parseUnaryExpression();

        // Check if we're dereferencing a void pointer, which is not allowed
        if (isVoidPointer(operand)) {
            Token token = getCurrentToken();
            reportError(token.pos, "Cannot dereference a void pointer - it has no defined size");
            exit(1);
        }
        
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
    }    else if (tokenIs(TOKEN_NOT)) {
        // Logical not (!)
        consume(TOKEN_NOT);
        ASTNode* operand = parseUnaryExpression();
        
        ASTNode* node = createNode(NODE_UNARY_OP);
        node->operation.op = UNARY_NOT;
        node->right = operand;
        return node;
    }
    else if (tokenIs(TOKEN_BITWISE_NOT)) {
        // Bitwise not (~)
        consume(TOKEN_BITWISE_NOT);
        ASTNode* operand = parseUnaryExpression();
        
        ASTNode* node = createNode(NODE_UNARY_OP);
        node->operation.op = UNARY_BITWISE_NOT;
        node->right = operand;
        return node;
    }    else if (tokenIs(TOKEN_SIZEOF)) {
        // sizeof operator
        consume(TOKEN_SIZEOF);
        
        // sizeof expects a parenthesized expression or type name
        expect(TOKEN_LPAREN);
        
        // First check if it's a type name like int, char, etc.
        Token token = getCurrentToken();
        
        if (token.type == TOKEN_INT || token.type == TOKEN_SHORT || token.type == TOKEN_CHAR ||
            token.type == TOKEN_VOID || token.type == TOKEN_BOOL || token.type == TOKEN_UNSIGNED) {
            // This is a type name, create a special identifier node for it
            ASTNode* operand = createNode(NODE_IDENTIFIER);
            
            if (token.type == TOKEN_UNSIGNED) {
                consume(token.type); // consume 'unsigned'
                token = getCurrentToken();
                
                if (token.type == TOKEN_INT) {
                    operand->identifier = strdup("unsigned int");
                    consume(token.type);
                } else if (token.type == TOKEN_CHAR) {
                    operand->identifier = strdup("unsigned char");
                    consume(token.type);
                } else if (token.type == TOKEN_SHORT) {
                    operand->identifier = strdup("unsigned short");
                    consume(token.type);
                } else {
                    operand->identifier = strdup("unsigned");
                    // No need to consume anything else
                }
            } else {
                // Regular type
                if (token.type == TOKEN_INT) operand->identifier = strdup("int");
                else if (token.type == TOKEN_CHAR) operand->identifier = strdup("char");
                else if (token.type == TOKEN_SHORT) operand->identifier = strdup("short");
                else if (token.type == TOKEN_VOID) operand->identifier = strdup("void");
                else if (token.type == TOKEN_BOOL) operand->identifier = strdup("bool");
                
                consume(token.type);
            }
            
            // Handle pointer types (e.g., int*, char*)
            while (tokenIs(TOKEN_STAR)) {
                char* oldType = operand->identifier;
                char* newType = malloc(strlen(oldType) + 2);  // +2 for '*' and '\0'
                sprintf(newType, "%s*", oldType);
                free(oldType);
                operand->identifier = newType;
                consume(TOKEN_STAR);
            }
            
            expect(TOKEN_RPAREN);
            
            ASTNode* node = createNode(NODE_UNARY_OP);
            node->operation.op = UNARY_SIZEOF;
            node->right = operand;
            return node;
        } else {
            // Not a type name, just a regular expression
            ASTNode* operand = parseExpression();
            
            expect(TOKEN_RPAREN);
            
            ASTNode* node = createNode(NODE_UNARY_OP);
            node->operation.op = UNARY_SIZEOF;
            node->right = operand;
            return node;
        }
    }
    else if (tokenIs(TOKEN_INCREMENT)) {
        // Prefix increment (++x)
        consume(TOKEN_INCREMENT);
        ASTNode* operand = parseUnaryExpression();
        
        ASTNode* node = createNode(NODE_UNARY_OP);
        node->operation.op = PREFIX_INCREMENT;
        node->right = operand;
        return node;
    }
    else if (tokenIs(TOKEN_DECREMENT)) {
        // Prefix decrement (--x)
        consume(TOKEN_DECREMENT);
        ASTNode* operand = parseUnaryExpression();
        
        ASTNode* node = createNode(NODE_UNARY_OP);
        node->operation.op = PREFIX_DECREMENT;
        node->right = operand;
        return node;
    }
    else if (tokenIs(TOKEN_LPAREN)) {
        // This could be a cast operator like (unsigned int)x
        // Save current position in case it's not a cast
        int start_pos = getCurrentPosition();
        
        // Try to parse a cast operation
        consume(TOKEN_LPAREN);
        Token token = getCurrentToken();
        
        // Check if it's a type name
        if (token.type == TOKEN_INT || token.type == TOKEN_SHORT || token.type == TOKEN_CHAR ||
            token.type == TOKEN_VOID || token.type == TOKEN_BOOL || token.type == TOKEN_UNSIGNED) {
            
            // Determine the cast target type
            DataType castType = TYPE_INT; // Default
            int isUnsigned = 0;
            
            if (token.type == TOKEN_UNSIGNED) {
                isUnsigned = 1;
                consume(TOKEN_UNSIGNED);
                token = getCurrentToken();
                
                if (token.type == TOKEN_INT) {
                    castType = TYPE_UNSIGNED_INT;
                    consume(TOKEN_INT);
                } else if (token.type == TOKEN_CHAR) {
                    castType = TYPE_UNSIGNED_CHAR;
                    consume(TOKEN_CHAR);
                } else if (token.type == TOKEN_SHORT) {
                    castType = TYPE_UNSIGNED_SHORT;
                    consume(TOKEN_SHORT);
                } else {
                    // If just 'unsigned' without a specific type, default to unsigned int
                    castType = TYPE_UNSIGNED_INT;
                }
            } else {
                // Handle regular types
                if (token.type == TOKEN_INT) {
                    castType = TYPE_INT;
                    consume(TOKEN_INT);
                } else if (token.type == TOKEN_CHAR) {
                    castType = TYPE_CHAR;
                    consume(TOKEN_CHAR);
                } else if (token.type == TOKEN_SHORT) {
                    castType = TYPE_SHORT;
                    consume(TOKEN_SHORT);
                } else if (token.type == TOKEN_VOID) {
                    castType = TYPE_VOID;
                    consume(TOKEN_VOID);
                } else if (token.type == TOKEN_BOOL) {
                    castType = TYPE_BOOL;
                    consume(TOKEN_BOOL);
                }
            }
            
            // Check for pointers (not implementing pointer casts yet)
            int isPointer = 0;
            while (tokenIs(TOKEN_STAR)) {
                consume(TOKEN_STAR);
                isPointer = 1;
            }
              // Complete the cast syntax
            if (tokenIs(TOKEN_RPAREN)) {
                consume(TOKEN_RPAREN);
                
                // Now we've confirmed this is a cast, parse the expression to cast
                // Parse unary expression to properly handle cases like (unsigned short)(-temp)
                ASTNode* operand = parseUnaryExpression();
                
                // Create the cast node
                ASTNode* node = createNode(NODE_UNARY_OP);
                node->unary_op.op = UNARY_CAST;
                node->unary_op.cast_type = castType;
                node->right = operand;
                return node;
            }
        }
        
        // If we get here, it wasn't a valid cast, so rewind and treat as a grouped expression
        setPosition(start_pos);
        
        // Handle it as a parenthesized expression
        consume(TOKEN_LPAREN);
        ASTNode* expr = parseExpression();
        expect(TOKEN_RPAREN);
        return expr;
    }
    
    // If no unary operator is found, parse as a postfix expression
    return parsePostfixExpression();
}

// Parse a postfix expression
ASTNode* parsePostfixExpression() {
    ASTNode* left = parsePrimaryExpression();
    
    // Continue processing postfix operators
    for (;;) {
        // Handle array subscript [index]
        if (tokenIs(TOKEN_LBRACKET)) {
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
        // Handle postfix increment (x++)
        else if (tokenIs(TOKEN_INCREMENT)) {
            consume(TOKEN_INCREMENT);
            
            ASTNode* node = createNode(NODE_UNARY_OP);
            node->operation.op = POSTFIX_INCREMENT;
            node->right = left;
            left = node;
        }
        // Handle postfix decrement (x--)
        else if (tokenIs(TOKEN_DECREMENT)) {
            consume(TOKEN_DECREMENT);
            
            ASTNode* node = createNode(NODE_UNARY_OP);
            node->operation.op = POSTFIX_DECREMENT;
            node->right = left;
            left = node;
        }
        else {
            // No more postfix operators
            break;
        }
    }
    
    return left;
}
