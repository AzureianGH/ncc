#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include "error_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize parser
void initParser() {
    // Nothing to initialize, just start parsing from the first token
}

#include "token_debug.h"

// Helper function to expect and consume a token
void expect(TokenType type) {
    if (!consume(type)) {
        Token token = getCurrentToken();
        fprintf(stderr, "Syntax error: Expected %s but got %s\n", 
                getTokenName(type), getTokenName(token.type));
        exit(1);
    }
}

// Parse a type (int, short, char, etc.)
TypeInfo parseType() {
    TypeInfo typeInfo;
    memset(&typeInfo, 0, sizeof(TypeInfo));
    
    // Handle unsigned if present
    int isUnsigned = 0;
    if (tokenIs(TOKEN_UNSIGNED)) {
        consume(TOKEN_UNSIGNED);
        isUnsigned = 1;
    }
      // Handle function attributes
    // Check for multiple possible attributes in any order
    while (tokenIs(TOKEN_STACKFRAME) || tokenIs(TOKEN_FAR) || tokenIs(TOKEN_FARCALLED)) {
        if (tokenIs(TOKEN_STACKFRAME)) {
            consume(TOKEN_STACKFRAME);
            typeInfo.is_stackframe = 1;
        } else if (tokenIs(TOKEN_FAR) || tokenIs(TOKEN_FARCALLED)) {
            if (tokenIs(TOKEN_FAR)) {
                consume(TOKEN_FAR);
            } else {
                consume(TOKEN_FARCALLED);
            }
            typeInfo.is_far = 1;
        }
    }
      // Parse base type
    if (tokenIs(TOKEN_INT)) {
        consume(TOKEN_INT);
        typeInfo.type = isUnsigned ? TYPE_UNSIGNED_INT : TYPE_INT;
    } else if (tokenIs(TOKEN_SHORT)) {
        consume(TOKEN_SHORT);
        typeInfo.type = isUnsigned ? TYPE_UNSIGNED_SHORT : TYPE_SHORT;
    } else if (tokenIs(TOKEN_CHAR)) {
        consume(TOKEN_CHAR);
        typeInfo.type = isUnsigned ? TYPE_UNSIGNED_CHAR : TYPE_CHAR;
    } else if (tokenIs(TOKEN_VOID)) {
        consume(TOKEN_VOID);
        typeInfo.type = TYPE_VOID;
    } else {
        fprintf(stderr, "Syntax error: Expected type specifier\n");
        exit(1);
    }
    
    // Copy any __stackframe or __far attributes from typeInfo to the return_type in a function
    
    // Handle pointers
    while (tokenIs(TOKEN_STAR)) {
        consume(TOKEN_STAR);
        
        // Check for FAR keyword for pointers
        if (tokenIs(TOKEN_FAR)) {
            consume(TOKEN_FAR);
            typeInfo.is_far_pointer = 1;
        }
        
        typeInfo.is_pointer++;
    }
    
    return typeInfo;
}

// Parse the entire program
ASTNode* parseProgram() {
    ASTNode* root = createNode(NODE_PROGRAM);
    ASTNode* last = NULL;
    
    // Parse declarations and functions until end of file
    while (!tokenIs(TOKEN_EOF)) {
        ASTNode* decl = parseDeclaration();
        
        if (!root->left) {
            root->left = decl;
        } else if (last) {
            last->next = decl;
        }
        
        last = decl;
    }
    
    return root;
}

// Parse a declaration (variable or function)
ASTNode* parseDeclaration() {
    // Parse the type
    TypeInfo typeInfo = parseType();
    
    // Get the identifier name
    if (!tokenIs(TOKEN_IDENTIFIER)) {
        fprintf(stderr, "Syntax error: Expected identifier\n");
        exit(1);
    }
    
    char* name = strdup(getCurrentToken().value);
    consume(TOKEN_IDENTIFIER);
    
    // Check if this is a function definition
    if (tokenIs(TOKEN_LPAREN)) {
        // This is a function
        return parseFunctionDefinition(name, typeInfo);
    } else {
        // This is a variable declaration
        return parseVariableDeclaration(name, typeInfo);
    }
}

// Parse a function parameter
ASTNode* parseParameter() {
    TypeInfo typeInfo = parseType();
    
    if (!tokenIs(TOKEN_IDENTIFIER)) {
        fprintf(stderr, "Syntax error: Expected parameter name\n");
        exit(1);
    }
    
    char* name = strdup(getCurrentToken().value);
    consume(TOKEN_IDENTIFIER);
    
    ASTNode* param = createNode(NODE_DECLARATION);
    param->declaration.var_name = name;
    param->declaration.type_info = typeInfo;
    
    return param;
}

// Parse a function definition
ASTNode* parseFunctionDefinition(char* name, TypeInfo returnType) {
    ASTNode* node = createNode(NODE_FUNCTION);
    node->function.func_name = name;
    node->function.info.return_type = returnType;
    
    // Transfer function attributes from the return type
    node->function.info.is_stackframe = returnType.is_stackframe;
    node->function.info.is_far = returnType.is_far;
    
    // Parse parameters
    expect(TOKEN_LPAREN);
    
    // Parse parameter list if not empty
    if (!tokenIs(TOKEN_RPAREN)) {
        ASTNode* firstParam = parseParameter();
        node->function.params = firstParam;
        node->function.info.param_count = 1;
        
        // Parse additional parameters
        ASTNode* lastParam = firstParam;
        
        while (tokenIs(TOKEN_COMMA)) {
            consume(TOKEN_COMMA);
            ASTNode* param = parseParameter();
            lastParam->next = param;
            lastParam = param;
            node->function.info.param_count++;
        }
    } else {
        node->function.params = NULL;
        node->function.info.param_count = 0;
    }
    
    expect(TOKEN_RPAREN);
    
    // Parse function body
    node->function.body = parseBlock();
    
    return node;
}

// Parse a variable declaration
ASTNode* parseVariableDeclaration(char* name, TypeInfo typeInfo) {
    ASTNode* node = createNode(NODE_DECLARATION);
    node->declaration.var_name = name;
    node->declaration.type_info = typeInfo;
    
    // Check for array declaration
    if (tokenIs(TOKEN_LBRACKET)) {
        consume(TOKEN_LBRACKET);
        
        // Parse array size if provided
        if (tokenIs(TOKEN_NUMBER)) {
            node->declaration.type_info.is_array = 1;
            node->declaration.type_info.array_size = atoi(getCurrentToken().value);
            consume(TOKEN_NUMBER);
        } else {
            node->declaration.type_info.is_array = 1;
            node->declaration.type_info.array_size = 0;  // Unknown size
        }
        
        expect(TOKEN_RBRACKET);
    }
    
    // Check for initializer
    if (tokenIs(TOKEN_ASSIGN)) {
        consume(TOKEN_ASSIGN);
        node->declaration.initializer = parseExpression();
    }
    
    expect(TOKEN_SEMICOLON);
    return node;
}

// Parse a block of statements
ASTNode* parseBlock() {
    ASTNode* node = createNode(NODE_BLOCK);
    ASTNode* lastStatement = NULL;
    
    expect(TOKEN_LBRACE);
    
    // Parse statements until closing brace
    while (!tokenIs(TOKEN_RBRACE) && !tokenIs(TOKEN_EOF)) {
        ASTNode* statement = parseStatement();
        
        if (!node->left) {
            // First statement
            node->left = statement;
        } else if (lastStatement) {
            // Add to linked list of statements
            lastStatement->next = statement;
        }
        
        lastStatement = statement;
    }
    
    expect(TOKEN_RBRACE);
    return node;
}
ASTNode* parseInlineAssembly() {
    expect(TOKEN_ASM); // Assuming TOKEN_ASM is the token for __asm

    ASTNode* node = createNode(NODE_ASM);

    // Expect an opening parenthesis for inline assembly syntax: __asm("...")
    expect(TOKEN_LPAREN);

    if (!tokenIs(TOKEN_STRING)) {
        fprintf(stderr, "Syntax error: Expected string literal in __asm\n");
        exit(1);
    }    node->asm_stmt.code = strdup(getCurrentToken().value);
    consume(TOKEN_STRING);

    expect(TOKEN_RPAREN);
    expect(TOKEN_SEMICOLON); // __asm("...");

    return node;
}

// Parse a statement
ASTNode* parseStatement() {
    // Debug message and token variable removed for quiet mode
    
    if (tokenIs(TOKEN_LBRACE)) {
        // Block statement
        return parseBlock();    } else if (tokenIs(TOKEN_IF)) {
        // If statement
        return parseIfStatement();
    } else if (tokenIs(TOKEN_WHILE)) {
        // While statement
        return parseWhileStatement();}else if (tokenIs(TOKEN_FOR)) {
        // For statement
        return parseForStatement();
    } else if (tokenIs(TOKEN_RETURN)) {
        // Return statement
        return parseReturnStatement();    } else if (tokenIs(TOKEN_ASM)) {
        // Inline assembly
        return parseInlineAssembly();
    } else if (tokenIs(TOKEN_INT) || tokenIs(TOKEN_SHORT) || tokenIs(TOKEN_CHAR) || 
               tokenIs(TOKEN_VOID) || tokenIs(TOKEN_UNSIGNED)) {
        // Declaration
        return parseDeclaration();
    } else {
        // Expression statement
        return parseExpressionStatement();
    }
}

// Parse a return statement
ASTNode* parseReturnStatement() {
    ASTNode* node = createNode(NODE_RETURN);
    
    consume(TOKEN_RETURN);
    
    // Return value is optional
    if (!tokenIs(TOKEN_SEMICOLON)) {
        node->return_stmt.expr = parseExpression();
    }
    
    expect(TOKEN_SEMICOLON);
    return node;
}

// Parse an expression statement
ASTNode* parseExpressionStatement() {
    ASTNode* node = createNode(NODE_EXPRESSION);
    
    node->left = parseExpression();
    expect(TOKEN_SEMICOLON);
    
    return node;
}



// Parse an inline assembly block
ASTNode* parseAsmBlock() {
    ASTNode* node = createNode(NODE_ASM_BLOCK);
    expect(TOKEN_ASM);
    expect(TOKEN_LBRACE);

    char* asmCode = malloc(1);
    asmCode[0] = '\0';

    int braceCount = 1;
    while (braceCount > 0 && !tokenIs(TOKEN_EOF)) {
        Token token = getCurrentToken();
        const char* text = token.value ? token.value : "";

        // Track braces to handle nested { ... } inside inline asm
        if (tokenIs(TOKEN_LBRACE)) {
            braceCount++;
        } else if (tokenIs(TOKEN_RBRACE)) {
            braceCount--;
        }

        // Append token text
        size_t oldLen = strlen(asmCode);
        size_t tokenLen = strlen(text);
        asmCode = realloc(asmCode, oldLen + tokenLen + 2);
        strcat(asmCode, text);
        strcat(asmCode, " ");

        getNextToken();
    }

    node->asm_block.code = asmCode;

    // We already consumed the final '}' above
    expect(TOKEN_SEMICOLON);
    return node;
}



// Parse an expression
ASTNode* parseExpression() {
    return parseAssignmentExpression();
}

// Parse a relational expression
ASTNode* parseRelationalExpression() {
    ASTNode* left = parseAdditiveExpression();
    
    // Handle relational operators: <, >, <=, >=, ==, !=
    while (tokenIs(TOKEN_LT) || tokenIs(TOKEN_GT) || 
           tokenIs(TOKEN_LTE) || tokenIs(TOKEN_GTE) ||
           tokenIs(TOKEN_EQ) || tokenIs(TOKEN_NEQ)) {
        
        OperatorType op;
        if (tokenIs(TOKEN_LT)) {
            consume(TOKEN_LT);
            op = OP_LT;
        } else if (tokenIs(TOKEN_GT)) {
            consume(TOKEN_GT);
            op = OP_GT;
        } else if (tokenIs(TOKEN_LTE)) {
            consume(TOKEN_LTE);
            op = OP_LTE;
        } else if (tokenIs(TOKEN_GTE)) {
            consume(TOKEN_GTE);
            op = OP_GTE;
        } else if (tokenIs(TOKEN_EQ)) {
            consume(TOKEN_EQ);
            op = OP_EQ;
        } else {
            consume(TOKEN_NEQ);
            op = OP_NEQ;
        }
        
        ASTNode* right = parseAdditiveExpression();
        
        ASTNode* node = createNode(NODE_BINARY_OP);
        node->operation.op = op;
        node->left = left;
        node->right = right;
        
        left = node;
    }
    
    return left;
}

// Parse an assignment expression
ASTNode* parseAssignmentExpression() {
    ASTNode* left = parseRelationalExpression();
    
    if (tokenIs(TOKEN_ASSIGN)) {
        consume(TOKEN_ASSIGN);
        
        ASTNode* node = createNode(NODE_ASSIGNMENT);
        node->left = left;
        node->right = parseAssignmentExpression();
        
        return node;
    }
    
    return left;
}

// Parse an additive expression
ASTNode* parseAdditiveExpression() {
    ASTNode* left = parseMultiplicativeExpression();
    
    while (tokenIs(TOKEN_PLUS) || tokenIs(TOKEN_MINUS)) {
        OperatorType op = tokenIs(TOKEN_PLUS) ? OP_ADD : OP_SUB;
        
        if (tokenIs(TOKEN_PLUS))
            consume(TOKEN_PLUS);
        else
            consume(TOKEN_MINUS);
        
        ASTNode* right = parseMultiplicativeExpression();
        
        ASTNode* node = createNode(NODE_BINARY_OP);
        node->operation.op = op;
        node->left = left;
        node->right = right;
        
        left = node;
    }
    
    return left;
}

// Parse a multiplicative expression
ASTNode* parseMultiplicativeExpression() {
    ASTNode* left = parseUnaryExpression();
    
    while (tokenIs(TOKEN_STAR) || tokenIs(TOKEN_SLASH) || tokenIs(TOKEN_PERCENT)) {
        OperatorType op;
        
        if (tokenIs(TOKEN_STAR)) {
            consume(TOKEN_STAR);
            op = OP_MUL;
        } else if (tokenIs(TOKEN_SLASH)) {
            consume(TOKEN_SLASH);
            op = OP_DIV;
        } else {
            consume(TOKEN_PERCENT);
            op = OP_MOD;
        }
        
        ASTNode* right = parsePrimaryExpression();
        
        ASTNode* node = createNode(NODE_BINARY_OP);
        node->operation.op = op;
        node->left = left;
        node->right = right;
        
        left = node;
    }
    
    return left;
}

// Parse a primary expression
ASTNode* parsePrimaryExpression() {
    if (tokenIs(TOKEN_IDENTIFIER)) {
        // Check if this is a function call
        char* name = strdup(getCurrentToken().value);
        consume(TOKEN_IDENTIFIER);
        
        if (tokenIs(TOKEN_LPAREN)) {
            // Function call
            ASTNode* node = createNode(NODE_CALL);
            node->call.func_name = name;
            
            consume(TOKEN_LPAREN);
            
            // Parse arguments
            ASTNode* argList = NULL;
            ASTNode* lastArg = NULL;
            int argCount = 0;
            
            if (!tokenIs(TOKEN_RPAREN)) {
                // First argument
                ASTNode* arg = parseExpression();
                argList = arg;
                lastArg = arg;
                argCount++;
                
                // Additional arguments
                while (tokenIs(TOKEN_COMMA)) {
                    consume(TOKEN_COMMA);
                    arg = parseExpression();
                    lastArg->next = arg;
                    lastArg = arg;
                    argCount++;
                }
            }
            
            node->call.args = argList;
            node->call.arg_count = argCount;
            
            expect(TOKEN_RPAREN);
            return node;
        } else {
            // Identifier
            ASTNode* node = createNode(NODE_IDENTIFIER);
            node->identifier = name;
            return node;
        }
    } else if (tokenIs(TOKEN_NUMBER)) {
        ASTNode* node = createNode(NODE_LITERAL);
        node->literal.data_type = TYPE_INT;
        
        char* numStr = getCurrentToken().value;
        // Handle hex numbers (starting with 0x)
        if (strlen(numStr) > 2 && numStr[0] == '0' && (numStr[1] == 'x' || numStr[1] == 'X')) {
            node->literal.int_value = (int)strtol(numStr, NULL, 16);
        } else {
            node->literal.int_value = atoi(numStr);
        }
        
        consume(TOKEN_NUMBER);
        
        // Check for segment:offset syntax for far pointers
        if (tokenIs(TOKEN_COLON)) {
            consume(TOKEN_COLON);
            
            if (!tokenIs(TOKEN_NUMBER)) {
                fprintf(stderr, "Syntax error: Expected offset after segment in far pointer\n");
                exit(1);
            }
            
            int segment = node->literal.int_value;
            numStr = getCurrentToken().value;
            
            // Parse offset value
            int offset;
            if (strlen(numStr) > 2 && numStr[0] == '0' && (numStr[1] == 'x' || numStr[1] == 'X')) {
                offset = (int)strtol(numStr, NULL, 16);
            } else {
                offset = atoi(numStr);
            }
            
            consume(TOKEN_NUMBER);
            
            // Create a far pointer node
            node->literal.data_type = TYPE_FAR_POINTER;
            node->literal.segment = segment;
            node->literal.offset = offset;
        }
        
        return node;
    } else if (tokenIs(TOKEN_STRING)) {
        ASTNode* node = createNode(NODE_LITERAL);
        node->literal.data_type = TYPE_CHAR;
        node->literal.string_value = strdup(getCurrentToken().value);
        consume(TOKEN_STRING);
        return node;
    } else if (tokenIs(TOKEN_LPAREN)) {
        consume(TOKEN_LPAREN);
        ASTNode* expr = parseExpression();
        expect(TOKEN_RPAREN);
        return expr;
    } else {
        fprintf(stderr, "Syntax error: Expected expression\n");
        exit(1);
        return NULL;  // To avoid compiler warning
    }
}