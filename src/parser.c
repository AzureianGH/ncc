#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include "error_manager.h"
#include "token_debug.h"
#include "attributes.h"
#include "type_checker.h"
#include "struct_support.h"
#include "struct_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global program root for checking deprecated functions 
ASTNode* g_program_root = NULL;

// Forward declarations
ASTNode* parseLogicalAndExpression();
ASTNode* parseLogicalOrExpression();

// Initialize parser
void initParser() {
    // Nothing to initialize, just start parsing from the first token
}

// Helper function to expect and consume a token
void expect(TokenType type) {
    if (!consume(type)) {
        Token token = getCurrentToken();
        reportError(token.pos, "Expected %s but got %s", 
                  getTokenName(type), getTokenName(token.type));
        exit(1);
    }
}

// Check if a token represents a type name
int isTypeName(Token token) {
    return token.type == TOKEN_INT || 
           token.type == TOKEN_SHORT || 
           token.type == TOKEN_LONG || 
           token.type == TOKEN_CHAR || 
           token.type == TOKEN_VOID || 
           token.type == TOKEN_UNSIGNED ||
           token.type == TOKEN_FAR ||
           token.type == TOKEN_BOOL ||
           token.type == TOKEN_STRUCT;
}

// Forward declaration for struct type parsing
TypeInfo parseStructType();

// Parse a type (int, short, char, etc.)
TypeInfo parseType() {
    TypeInfo typeInfo;
    memset(&typeInfo, 0, sizeof(TypeInfo));
    
    // Check if it's a struct type
    if (tokenIs(TOKEN_STRUCT)) {
        return parseStructType();
    }
    
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
        }    }
      // Parse base type
    if (tokenIs(TOKEN_INT)) {
        consume(TOKEN_INT);
        typeInfo.type = isUnsigned ? TYPE_UNSIGNED_INT : TYPE_INT;
    } else if (tokenIs(TOKEN_SHORT)) {
        consume(TOKEN_SHORT);
        typeInfo.type = isUnsigned ? TYPE_UNSIGNED_SHORT : TYPE_SHORT;
    } else if (tokenIs(TOKEN_LONG)) {
        consume(TOKEN_LONG);
        typeInfo.type = isUnsigned ? TYPE_UNSIGNED_LONG : TYPE_LONG;
    } else if (tokenIs(TOKEN_CHAR)) {
        consume(TOKEN_CHAR);
        typeInfo.type = isUnsigned ? TYPE_UNSIGNED_CHAR : TYPE_CHAR;
    } else if (tokenIs(TOKEN_BOOL)) {
        consume(TOKEN_BOOL);
        typeInfo.type = TYPE_BOOL;
    } else if (tokenIs(TOKEN_VOID)) {
        consume(TOKEN_VOID);
        typeInfo.type = TYPE_VOID;
    } else if (isUnsigned) {
        // If only 'unsigned' is specified without another type, default to unsigned int
        typeInfo.type = TYPE_UNSIGNED_INT;
    } else {
        Token token = getCurrentToken();
        reportError(token.pos, "Expected type specifier");
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
    
    // Store the program root for later usage (deprecation checking)
    g_program_root = root;
    
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

// Forward declaration
ASTNode* parseStructDefinition();

// Parse a declaration (variable or function)
ASTNode* parseDeclaration() {
    // Check for function attributes before the type
    // This is used for cases like __attribute__((naked)) void func()
    FunctionInfo tempFuncInfo = {0};
    int hasAttributes = 0;
    int isStatic = 0;
    
    // Check for static keyword
    if (tokenIs(TOKEN_STATIC)) {
        consume(TOKEN_STATIC);
        isStatic = 1;
    }
    
    if (tokenIs(TOKEN_ATTRIBUTE) || tokenIs(TOKEN_ATTR_OPEN)) {
        parseFunctionAttributes(&tempFuncInfo);
        hasAttributes = 1;
    }
    
    // Check for struct definition
    if (tokenIs(TOKEN_STRUCT)) {
        return parseStructDefinition();
    }
    
    // Parse the type
    TypeInfo typeInfo = parseType();
    
    // Apply storage class if static
    if (isStatic) {
        typeInfo.is_static = 1;
    }
      // Get the identifier name
    if (!tokenIs(TOKEN_IDENTIFIER)) {
        Token token = getCurrentToken();
        reportError(token.pos, "Expected identifier after type specifier");
        exit(1);
    }
    
    char* name = strdupc(getCurrentToken().value);
    consume(TOKEN_IDENTIFIER);
    
    // Check if this is a function definition
    if (tokenIs(TOKEN_LPAREN)) {
        // This is a function
        ASTNode* fnNode = parseFunctionDefinition(name, typeInfo);
        if (hasAttributes) {
            fnNode->function.info.is_naked = tempFuncInfo.is_naked;
            fnNode->function.info.is_stackframe = tempFuncInfo.is_stackframe;
            fnNode->function.info.is_far = tempFuncInfo.is_far;
            fnNode->function.info.is_deprecated = tempFuncInfo.is_deprecated;
            fnNode->function.info.deprecation_msg = tempFuncInfo.deprecation_msg;
        }
        return fnNode;
    } else {
        // This is a variable declaration
        return parseVariableDeclaration(name, typeInfo);
    }
}

// Parse a function parameter
ASTNode* parseParameter() {
    TypeInfo typeInfo = parseType();
      if (!tokenIs(TOKEN_IDENTIFIER)) {
        Token token = getCurrentToken();
        reportError(token.pos, "Expected parameter name");
        exit(1);
    }
    
    char* name = strdupc(getCurrentToken().value);
    consume(TOKEN_IDENTIFIER);
    
    // Check if trying to declare a parameter of type void (which is invalid)
    // void pointers are allowed, but plain void parameters are not
    if (typeInfo.type == TYPE_VOID && !typeInfo.is_pointer) {
        Token token = getCurrentToken();
        reportError(token.pos, "Parameter '%s' has incomplete type 'void'", name);
        exit(1);
    }
      ASTNode* param = createNode(NODE_DECLARATION);
    param->declaration.var_name = name;
    param->declaration.type_info = typeInfo;
    
    // Register the parameter and its type in the symbol table for type checking
    addTypeSymbol(name, typeInfo);
    
    return param;
}

// Parse a function definition
ASTNode* parseFunctionDefinition(char* name, TypeInfo returnType) {
    ASTNode* node = createNode(NODE_FUNCTION);
    node->function.func_name = name;
    node->function.info.return_type = returnType;
    
    // Transfer function attributes from the return type
    node->function.info.is_stackframe = returnType.is_stackframe;    node->function.info.is_far = returnType.is_far;
    node->function.info.is_static = returnType.is_static; // Transfer static attribute
    node->function.info.is_naked = 0; // Initialize naked flag
    node->function.info.is_deprecated = 0; // Initialize deprecated flag
    node->function.info.deprecation_msg = NULL; // Initialize deprecation message
    node->function.info.is_variadic = 0; // Initialize variadic flag
    
    // Allow __attribute__ between return type and parameter list
    if (tokenIs(TOKEN_ATTRIBUTE) || tokenIs(TOKEN_ATTR_OPEN)) {
        parseFunctionAttributes(&node->function.info);
    }
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
            
            // Check for variadic arguments (...) 
            if (tokenIs(TOKEN_ELLIPSIS)) {
                consume(TOKEN_ELLIPSIS);
                node->function.info.is_variadic = 1;
                break; // Ellipsis must be the last parameter
            }
            
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
    
    // Parse function attributes if present
    parseFunctionAttributes(&node->function.info);
    
    // Parse function body
    node->function.body = parseBlock();
    
    return node;
}

// Parse a variable declaration
ASTNode* parseVariableDeclaration(char* name, TypeInfo typeInfo) {
    // Check if trying to declare a variable of type void (which is invalid)
    // void pointers are allowed, but plain void variables are not
    if (typeInfo.type == TYPE_VOID && !typeInfo.is_pointer) {
        Token token = getCurrentToken();
        reportError(token.pos, "Variable '%s' has incomplete type 'void'", name);
        exit(1);
    }
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
        
        // Check for struct initializer: struct Point p = {10, 20};
        if (node->declaration.type_info.type == TYPE_STRUCT && tokenIs(TOKEN_LBRACE)) {
            consume(TOKEN_LBRACE);
            
            // Create a linked list of initializer expressions for struct members
            ASTNode* initializers = NULL;
            ASTNode* lastInitializer = NULL;
            int initCount = 0;
            
            // Parse comma-separated expressions until closing brace
            if (!tokenIs(TOKEN_RBRACE)) {
                // First initializer
                initializers = parseExpression();
                lastInitializer = initializers;
                initCount = 1;
                
                // Additional initializers
                while (tokenIs(TOKEN_COMMA)) {
                    consume(TOKEN_COMMA);
                    if (tokenIs(TOKEN_RBRACE)) {
                        break; // Allow trailing comma
                    }
                    ASTNode* expr = parseExpression();
                    lastInitializer->next = expr;
                    lastInitializer = expr;
                    initCount++;
                }
            }
            
            expect(TOKEN_RBRACE);
            
            // Store the initializers and count
            node->declaration.initializer = initializers;
        }
        // Check for array initializer with braces: int arr[5] = {1, 2, 3, 4, 5};
        else if (node->declaration.type_info.is_array && tokenIs(TOKEN_LBRACE)) {
            consume(TOKEN_LBRACE);
            
            // Create a linked list of initializer expressions
            ASTNode* initializers = NULL;
            ASTNode* lastInitializer = NULL;
            int initCount = 0;
            
            // Parse comma-separated expressions until closing brace
            if (!tokenIs(TOKEN_RBRACE)) {
                // First initializer
                initializers = parseExpression();
                lastInitializer = initializers;
                initCount = 1;
                
                // Parse additional initializers
                while (tokenIs(TOKEN_COMMA)) {
                    consume(TOKEN_COMMA);
                    ASTNode* expr = parseExpression();
                    lastInitializer->next = expr;
                    lastInitializer = expr;
                    initCount++;
                }
            }
            
            expect(TOKEN_RBRACE);
            
            // Store the list of initializers
            node->declaration.initializer = initializers;
            
            // If array size wasn't specified, use the number of initializers
            if (node->declaration.type_info.array_size == 0) {
                node->declaration.type_info.array_size = initCount;
            }
        } else {
            // Regular single-value initializer
            node->declaration.initializer = parseExpression();
        }
    }
    
    // Register the variable and its type in the symbol table for type checking (after array/initializer processed)
    addTypeSymbol(node->declaration.var_name, node->declaration.type_info);

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
    expect(TOKEN_ASM); // Consume the __asm token

    // Check for block-style syntax: __asm { ... }
    if (tokenIs(TOKEN_LBRACE)) {
        return parseAsmBlock();
    }

    // Traditional syntax: __asm("instruction")
    ASTNode* node = createNode(NODE_ASM);
    
    // Initialize operand-related fields
    node->asm_stmt.operands = NULL;
    node->asm_stmt.constraints = NULL;
    node->asm_stmt.operand_count = 0;

    // Expect an opening parenthesis for inline assembly syntax: __asm("...")
    expect(TOKEN_LPAREN);
    
    if (!tokenIs(TOKEN_STRING)) {
        Token token = getCurrentToken();
        reportError(token.pos, "Expected string literal in __asm statement");
        exit(1);
    }
    
    node->asm_stmt.code = strdupc(getCurrentToken().value);
    consume(TOKEN_STRING);
    
    // Check for extended syntax with colons for operands: __asm("instr %0" : : "r"(var))
    if (tokenIs(TOKEN_COLON)) {
        consume(TOKEN_COLON); // Consume first colon (output operands, not supported yet)
        
        // Check for second colon (input operands)
        if (tokenIs(TOKEN_COLON)) {
            consume(TOKEN_COLON);
            
            // Allocate initial space for operands and constraints
            node->asm_stmt.operand_count = 0;
            node->asm_stmt.operands = (ASTNode**)malloc(sizeof(ASTNode*) * 8); // Start with space for 8 operands
            node->asm_stmt.constraints = (char**)malloc(sizeof(char*) * 8);
            
            if (!node->asm_stmt.operands || !node->asm_stmt.constraints) {
                reportError(getCurrentToken().pos, "Memory allocation failed for assembly operands");
                exit(1);
            }
            
            // Parse operands until the closing parenthesis
            while (!tokenIs(TOKEN_RPAREN)) {
                // Skip optional commas between operands
                if (tokenIs(TOKEN_COMMA)) {
                    consume(TOKEN_COMMA);
                }
                
                // Check if we've reached the end of operands
                if (tokenIs(TOKEN_RPAREN)) {
                    break;
                }
                
                // Parse constraint string: "r", "m", etc.
                if (!tokenIs(TOKEN_STRING)) {
                    Token token = getCurrentToken();
                    reportError(token.pos, "Expected constraint string for assembly operand");
                    exit(1);
                }
                
                // Store the constraint
                node->asm_stmt.constraints[node->asm_stmt.operand_count] = strdupc(getCurrentToken().value);
                consume(TOKEN_STRING);
                
                // Parse operand expression: (variable)
                expect(TOKEN_LPAREN);
                node->asm_stmt.operands[node->asm_stmt.operand_count] = parseExpression();
                expect(TOKEN_RPAREN);
                
                // Increment operand count
                node->asm_stmt.operand_count++;
                
                // Resize arrays if needed
                if (node->asm_stmt.operand_count % 8 == 0) {
                    int new_size = node->asm_stmt.operand_count + 8;
                    node->asm_stmt.operands = (ASTNode**)realloc(node->asm_stmt.operands, sizeof(ASTNode*) * new_size);
                    node->asm_stmt.constraints = (char**)realloc(node->asm_stmt.constraints, sizeof(char*) * new_size);
                    
                    if (!node->asm_stmt.operands || !node->asm_stmt.constraints) {
                        reportError(getCurrentToken().pos, "Memory allocation failed for assembly operands");
                        exit(1);
                    }
                }
            }
        }
    }

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
        return parseWhileStatement();
    } else if (tokenIs(TOKEN_DO)) {
        // Do-while statement
        return parseDoWhileStatement();
    }else if (tokenIs(TOKEN_FOR)) {
        // For statement
        return parseForStatement();
    } else if (tokenIs(TOKEN_RETURN)) {
        // Return statement
        return parseReturnStatement();    } else if (tokenIs(TOKEN_ASM)) {
        // Inline assembly
        return parseInlineAssembly();    } else if (tokenIs(TOKEN_STATIC) || tokenIs(TOKEN_INT) || tokenIs(TOKEN_SHORT) || 
               tokenIs(TOKEN_CHAR) || tokenIs(TOKEN_VOID) || tokenIs(TOKEN_UNSIGNED)) {
        // Check for static keyword in local variable declarations
        if (tokenIs(TOKEN_STATIC)) {
            reportWarning(peekNextToken().pos, "Static local variables are not supported - 'static' ignored in local context");
            consume(TOKEN_STATIC);
        }
        
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



// Parse a comma expression (expr, expr, expr)
ASTNode* parseCommaExpression() {
    ASTNode* left = parseAssignmentExpression();
    
    while (tokenIs(TOKEN_COMMA)) {
        consume(TOKEN_COMMA);
        
        // For comma operator, create a binary op node
        ASTNode* right = parseAssignmentExpression();
        
        // Create a special binary operation for comma operator
        // The result is the value of the right operand
        ASTNode* node = createNode(NODE_BINARY_OP);
        node->operation.op = OP_COMMA;  // We'll add this to the enum
        node->left = left;   // Left operand is evaluated first
        node->right = right; // Right operand's value becomes the result
        
        left = node;
    }
    
    return left;
}

// Parse an expression
ASTNode* parseExpression() {
    return parseCommaExpression();
}

// Parse a relational expression
ASTNode* parseRelationalExpression() {
    ASTNode* left = parseBitwiseExpression();
    
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

// Parse a logical AND expression
ASTNode* parseLogicalAndExpression() {
    ASTNode* left = parseRelationalExpression();
    while (tokenIs(TOKEN_AND)) {
        consume(TOKEN_AND);
        ASTNode* right = parseRelationalExpression();
        ASTNode* node = createNode(NODE_BINARY_OP);
        node->operation.op = OP_LAND;
        node->left = left;
        node->right = right;
        left = node;
    }
    return left;
}

// Parse a logical OR expression
ASTNode* parseLogicalOrExpression() {
    ASTNode* left = parseLogicalAndExpression();
    while (tokenIs(TOKEN_OR)) {
        consume(TOKEN_OR);
        ASTNode* right = parseLogicalAndExpression();
        ASTNode* node = createNode(NODE_BINARY_OP);
        node->operation.op = OP_LOR;
        node->left = left;
        node->right = right;
        left = node;
    }
    return left;
}

// Parse a ternary conditional expression (condition ? true_expr : false_expr)
ASTNode* parseTernaryExpression() {
    ASTNode* condition = parseLogicalOrExpression();
    
    if (tokenIs(TOKEN_QUESTION)) {
        consume(TOKEN_QUESTION);
        
        // Create a ternary operation node
        ASTNode* node = createNode(NODE_TERNARY);
        node->ternary.condition = condition;
        
        // Parse the expression for the true case, can include comma expressions
        node->ternary.true_expr = parseCommaExpression();
        
        // Expect a colon separating the true and false expressions
        expect(TOKEN_COLON);
        
        // Parse the expression for the false case, can include comma expressions
        node->ternary.false_expr = parseCommaExpression();
        
        return node;
    }
    
    return condition;
}

// Parse an assignment expression
ASTNode* parseAssignmentExpression() {
    ASTNode* left = parseTernaryExpression();

    if (tokenIs(TOKEN_ASSIGN) || tokenIs(TOKEN_PLUS_ASSIGN) || tokenIs(TOKEN_MINUS_ASSIGN) ||
        tokenIs(TOKEN_MUL_ASSIGN) || tokenIs(TOKEN_DIV_ASSIGN) || tokenIs(TOKEN_MOD_ASSIGN) ||
        tokenIs(TOKEN_LEFT_SHIFT_ASSIGN) || tokenIs(TOKEN_RIGHT_SHIFT_ASSIGN)) {
        
        ASTNode* node = createNode(NODE_ASSIGNMENT);
        // Determine assignment operator
        if (tokenIs(TOKEN_PLUS_ASSIGN)) {
            node->assignment.op = OP_PLUS_ASSIGN;
            consume(TOKEN_PLUS_ASSIGN);
        } else if (tokenIs(TOKEN_MINUS_ASSIGN)) {
            node->assignment.op = OP_MINUS_ASSIGN;
            consume(TOKEN_MINUS_ASSIGN);
        } else if (tokenIs(TOKEN_MUL_ASSIGN)) {
            node->assignment.op = OP_MUL_ASSIGN;
            consume(TOKEN_MUL_ASSIGN);
        } else if (tokenIs(TOKEN_DIV_ASSIGN)) {
            node->assignment.op = OP_DIV_ASSIGN;
            consume(TOKEN_DIV_ASSIGN);
        } else if (tokenIs(TOKEN_MOD_ASSIGN)) {
            node->assignment.op = OP_MOD_ASSIGN;
            consume(TOKEN_MOD_ASSIGN);
        } else if (tokenIs(TOKEN_LEFT_SHIFT_ASSIGN)) {
            node->assignment.op = OP_LEFT_SHIFT_ASSIGN;
            consume(TOKEN_LEFT_SHIFT_ASSIGN);
        } else if (tokenIs(TOKEN_RIGHT_SHIFT_ASSIGN)) {
            node->assignment.op = OP_RIGHT_SHIFT_ASSIGN;
            consume(TOKEN_RIGHT_SHIFT_ASSIGN);
        } else {
            node->assignment.op = 0; // simple assignment
            consume(TOKEN_ASSIGN);
        }// Check if left side is a void pointer dereference (i.e., *void_ptr = value)
        if (left->type == NODE_UNARY_OP && left->unary_op.op == UNARY_DEREFERENCE) {
            if (isVoidPointer(left->right)) {
                Token token = getCurrentToken();
                reportError(token.pos, "Cannot assign to a dereferenced void pointer - it has no defined size");
                exit(1);
            }
        }
        
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
            op = OP_DIV;        } else {
            consume(TOKEN_PERCENT);
            op = OP_MOD;
        }
        
        ASTNode* right = parseUnaryExpression();
        
        ASTNode* node = createNode(NODE_BINARY_OP);
        node->operation.op = op;
        node->left = left;
        node->right = right;
        
        left = node;
    }
    
    return left;
}

// Parse expressions with bitwise operators
ASTNode* parseBitwiseExpression() {
    ASTNode* left = parseShiftExpression();
    
    // Handle bitwise operators: &, |, ^
    while (tokenIs(TOKEN_AMPERSAND) || tokenIs(TOKEN_PIPE) || tokenIs(TOKEN_XOR)) {
        OperatorType op;
        
        if (tokenIs(TOKEN_AMPERSAND)) {
            consume(TOKEN_AMPERSAND);
            op = OP_BITWISE_AND;
        } else if (tokenIs(TOKEN_PIPE)) {
            consume(TOKEN_PIPE);
            op = OP_BITWISE_OR;
        } else {
            consume(TOKEN_XOR);
            op = OP_BITWISE_XOR;
        }
        
        ASTNode* right = parseShiftExpression();
        
        ASTNode* node = createNode(NODE_BINARY_OP);
        node->operation.op = op;
        node->left = left;
        node->right = right;
        
        left = node;
    }
    
    return left;
}

// Parse expressions with shift operators
ASTNode* parseShiftExpression() {
    ASTNode* left = parseAdditiveExpression();
    
    // Handle shift operators: <<, >>
    while (tokenIs(TOKEN_LEFT_SHIFT) || tokenIs(TOKEN_RIGHT_SHIFT)) {
        OperatorType op;
        
        if (tokenIs(TOKEN_LEFT_SHIFT)) {
            consume(TOKEN_LEFT_SHIFT);
            op = OP_LEFT_SHIFT;
        } else {
            consume(TOKEN_RIGHT_SHIFT);
            op = OP_RIGHT_SHIFT;
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

// Parse a primary expression
ASTNode* parsePrimaryExpression() {
    if (tokenIs(TOKEN_IDENTIFIER)) {
        // Check if this is a function call
        char* name = strdupc(getCurrentToken().value);
        consume(TOKEN_IDENTIFIER);
        
        if (tokenIs(TOKEN_LPAREN)) {
            // Function call
            ASTNode* node = createNode(NODE_CALL);
            node->call.func_name = name;
            
            // Check if the function is deprecated
            ASTNode* current = g_program_root ? g_program_root->left : NULL;
            while (current) {
                if (current->type == NODE_FUNCTION && 
                    strcmp(current->function.func_name, name) == 0) {
                    if (current->function.info.is_deprecated) {
                        if (current->function.info.deprecation_msg) {
                            reportWarning(getCurrentToken().pos, 
                                "Call to deprecated function '%s': %s", 
                                name, current->function.info.deprecation_msg);
                        } else {
                            reportWarning(getCurrentToken().pos, 
                                "Call to deprecated function '%s'", name);
                        }
                        break;
                    }
                }
                current = current->next;
            }
              consume(TOKEN_LPAREN);
            
            // Parse arguments
            ASTNode* argList = NULL;
            ASTNode* lastArg = NULL;
            int argCount = 0;
            
            if (!tokenIs(TOKEN_RPAREN)) {
                // First argument
                ASTNode* arg = parseAssignmentExpression();  // Use assignment expression for arguments
                argList = arg;
                lastArg = arg;
                argCount++;
                
                // Additional arguments
                while (tokenIs(TOKEN_COMMA)) {
                    consume(TOKEN_COMMA);
                    arg = parseAssignmentExpression();  // Use assignment expression for arguments
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
            
            if (!tokenIs(TOKEN_NUMBER)) {                Token token = getCurrentToken();
                reportError(token.pos, "Expected offset value after segment in far pointer");
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
    } else if (tokenIs(TOKEN_CHAR_LITERAL)) {
        ASTNode* node = createNode(NODE_LITERAL);
        node->literal.data_type = TYPE_INT; // Treat chars as integers
        
        // Get the character value and convert to its ASCII/hex value
        char ch = getCurrentToken().value[0];
        node->literal.int_value = (unsigned char)ch; // ASCII value of the character
        
        consume(TOKEN_CHAR_LITERAL);
        return node;    } else if (tokenIs(TOKEN_STRING)) {
        ASTNode* node = createNode(NODE_LITERAL);
        node->literal.data_type = TYPE_CHAR;
        
        // Store the string value including quotes
        const char* tokenValue = getCurrentToken().value;
        if (tokenValue) {
            node->literal.string_value = strdupc(tokenValue);
        } else {
            node->literal.string_value = strdupc(""); // Empty string as fallback
        }
        
        consume(TOKEN_STRING);
        return node;
    } else if (tokenIs(TOKEN_TRUE) || tokenIs(TOKEN_FALSE)) {
        ASTNode* node = createNode(NODE_LITERAL);
        node->literal.data_type = TYPE_BOOL;
        node->literal.int_value = tokenIs(TOKEN_TRUE) ? 1 : 0;
        
        consume(tokenIs(TOKEN_TRUE) ? TOKEN_TRUE : TOKEN_FALSE);
        return node;
    } else if (tokenIs(TOKEN_LPAREN)) {
        consume(TOKEN_LPAREN);
        ASTNode* expr = parseExpression();
        expect(TOKEN_RPAREN);
        return expr;
    } else {        Token token = getCurrentToken();
        reportError(token.pos, "Expected expression");
        exit(1);
        return NULL;  // To avoid compiler warning
    }
}

// Save and restore parser position (for lookahead)
int getCurrentPosition() {
    return getTokenPosition();
}

void setPosition(int pos) {
    setTokenPosition(pos);
}