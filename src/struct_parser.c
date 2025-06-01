#include "struct_parser.h"
#include "parser.h"
#include "lexer.h"
#include "struct_support.h"
#include "error_manager.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External functions
extern void expect(TokenType type);
extern int tokenIs(TokenType type);
extern Token getCurrentToken();
extern int consume(TokenType type);

// Parse a struct type (used in variable declarations)
TypeInfo parseStructType() {
    TypeInfo typeInfo;
    memset(&typeInfo, 0, sizeof(TypeInfo));
    typeInfo.type = TYPE_STRUCT;
    
    consume(TOKEN_STRUCT);
    
    // A struct type must have a name 
    if (!tokenIs(TOKEN_IDENTIFIER)) {
        Token token = getCurrentToken();
        reportError(token.pos, "Expected struct name after 'struct' keyword");
        exit(1);
    }
    
    char* structName = strdupc(getCurrentToken().value);
    consume(TOKEN_IDENTIFIER);
    
    // Find the struct definition by name
    StructInfo* structInfo = findStructDefinition(structName);
    if (!structInfo) {
        reportError(-1, "Unknown struct type '%s'", structName);
        exit(1);
    }
    
    // Store pointer to the struct definition
    typeInfo.struct_info = structInfo;
    
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

// Parse a struct definition (struct name { ... };)
ASTNode* parseStructDefinition() {
    ASTNode* node = createNode(NODE_STRUCT_DEF);
    
    consume(TOKEN_STRUCT);
    
    // Struct name is required
    if (!tokenIs(TOKEN_IDENTIFIER)) {
        Token token = getCurrentToken();
        reportError(token.pos, "Expected struct name after 'struct' keyword");
        exit(1);
    }
    
    node->struct_def.struct_name = strdupc(getCurrentToken().value);
    consume(TOKEN_IDENTIFIER);
    
    // Check for duplicate struct definition
    if (findStructDefinition(node->struct_def.struct_name)) {
        reportError(-1, "Duplicate definition of struct '%s'", node->struct_def.struct_name);
        exit(1);
    }
    
    // Create struct info
    StructInfo* structInfo = (StructInfo*)malloc(sizeof(StructInfo));
    if (!structInfo) {
        reportError(-1, "Memory allocation failed for struct info");
        exit(1);
    }
    structInfo->name = strdupc(node->struct_def.struct_name);
    structInfo->members = NULL;
    structInfo->size = 0;
    
    node->struct_def.info = structInfo;
    
    // Add to global struct table immediately so we can handle recursive definitions
    addStructDefinition(structInfo);
    
    // Parse struct body
    expect(TOKEN_LBRACE);
    
    // Parse member declarations until closing brace
    StructMember* lastMember = NULL;
    while (!tokenIs(TOKEN_RBRACE) && !tokenIs(TOKEN_EOF)) {
        // Parse member type
        TypeInfo memberTypeInfo = parseType();
        
        // Member name is required
        if (!tokenIs(TOKEN_IDENTIFIER)) {
            Token token = getCurrentToken();
            reportError(token.pos, "Expected member name in struct definition");
            exit(1);
        }
        
        char* memberName = strdupc(getCurrentToken().value);
        consume(TOKEN_IDENTIFIER);
        
        // Check for array declaration
        if (tokenIs(TOKEN_LBRACKET)) {
            consume(TOKEN_LBRACKET);
            
            // Parse array size if provided
            if (tokenIs(TOKEN_NUMBER)) {
                memberTypeInfo.is_array = 1;
                memberTypeInfo.array_size = atoi(getCurrentToken().value);
                consume(TOKEN_NUMBER);
            } else {
                reportError(-1, "Array member '%s' must have a size", memberName);
                exit(1);
            }
            
            expect(TOKEN_RBRACKET);
        }
        
        // Create member node
        StructMember* member = createStructMember(memberName, memberTypeInfo, 0); // offset will be computed later
        
        // Add to member list
        if (!structInfo->members) {
            structInfo->members = member;
        } else if (lastMember) {
            lastMember->next = member;
        }
        lastMember = member;
        
        // Create AST node for member declaration
        ASTNode* memberNode = createNode(NODE_DECLARATION);
        memberNode->declaration.var_name = strdupc(memberName);
        memberNode->declaration.type_info = memberTypeInfo;
        
        // Add to member list in AST
        if (!node->struct_def.members) {
            node->struct_def.members = memberNode;
        } else {
            ASTNode* lastNodeMember = node->struct_def.members;
            while (lastNodeMember->next) {
                lastNodeMember = lastNodeMember->next;
            }
            lastNodeMember->next = memberNode;
        }
        
        expect(TOKEN_SEMICOLON);
    }
    
    expect(TOKEN_RBRACE);
    expect(TOKEN_SEMICOLON);
    
    // Compute member offsets and struct size
    layoutStruct(structInfo);
    
    return node;
}
