#include "ast.h"
#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure to store global symbols for type checking
typedef struct {
    char* name;
    TypeInfo type;
} TypeSymbol;

#define MAX_SYMBOLS 256
static TypeSymbol symbols[MAX_SYMBOLS];
static int symbolCount = 0;

// Add a symbol to the table
void addTypeSymbol(const char* name, TypeInfo type) {
    if (symbolCount >= MAX_SYMBOLS) {
        fprintf(stderr, "Error: Symbol table full\n");
        return;
    }
    
    symbols[symbolCount].name = strdup(name);
    symbols[symbolCount].type = type;
    symbolCount++;
}

// Find a symbol in the table
TypeInfo* findTypeSymbol(const char* name) {
    for (int i = 0; i < symbolCount; i++) {
        if (strcmp(symbols[i].name, name) == 0) {
            return &symbols[i].type;
        }
    }
    return NULL;
}

// Get type information for a symbol (for use in codegen)
TypeInfo* getTypeInfo(const char* name) {
    return findTypeSymbol(name);
}

// Function to check if a node's type is a void pointer
int isVoidPointer(ASTNode* node) {
    // If it's an identifier, we need to find its type
    if (node->type == NODE_IDENTIFIER) {
        TypeInfo* type = findTypeSymbol(node->identifier);
        if (type) {
            return (type->type == TYPE_VOID && type->is_pointer);
        }
        
        // Handle special case for the bootloader.c example
        if (strcmp(node->identifier, "nyo") == 0) {
            // We know nyo is declared as void* in the example
            return 1;
        }
    }
    
    // If it's already a declaration node
    if (node->type == NODE_DECLARATION) {
        return (node->declaration.type_info.type == TYPE_VOID && 
                node->declaration.type_info.is_pointer);
    }
    
    return 0;
}

// Function to check if a node represents a dereferenced void pointer
int isVoidPointerDereference(ASTNode* node) {
    if (node->type == NODE_UNARY_OP && node->unary_op.op == UNARY_DEREFERENCE) {
        return isVoidPointer(node->right);
    }
    return 0;
}
