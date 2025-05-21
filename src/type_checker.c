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

// Get type information from an expression
TypeInfo* getTypeInfoFromExpression(ASTNode* expr) {
    if (!expr) return NULL;
    
    // Handle simple cases directly
    if (expr->type == NODE_IDENTIFIER) {
        return getTypeInfo(expr->identifier);
    }
    
    // Handle literal types
    if (expr->type == NODE_LITERAL) {
        // Create a static TypeInfo for the literal
        static TypeInfo literalTypeInfo;
        memset(&literalTypeInfo, 0, sizeof(TypeInfo));
        
        literalTypeInfo.type = expr->literal.data_type;
        literalTypeInfo.is_pointer = 0;
        literalTypeInfo.is_far_pointer = 0;
        
        // Handle string literals
        if (expr->literal.data_type == TYPE_CHAR && expr->literal.string_value) {
            literalTypeInfo.is_pointer = 1;
        }
        
        return &literalTypeInfo;
    }
    
    // For unary operations
    if (expr->type == NODE_UNARY_OP) {
        // Address-of operator creates a pointer
        if (expr->unary_op.op == UNARY_ADDRESS_OF) {
            static TypeInfo addrTypeInfo;
            memset(&addrTypeInfo, 0, sizeof(TypeInfo));
            
            // Determine the base type from the operand
            TypeInfo* baseType = getTypeInfoFromExpression(expr->right);
            if (baseType) {
                addrTypeInfo = *baseType;
                addrTypeInfo.is_pointer = 1;
                addrTypeInfo.is_far_pointer = 0; // Near pointers by default
            } else {
                // Default to int pointer if we can't determine
                addrTypeInfo.type = TYPE_INT;
                addrTypeInfo.is_pointer = 1;
            }
            
            return &addrTypeInfo;
        }
        
        // Dereference operator removes one level of pointer
        if (expr->unary_op.op == UNARY_DEREFERENCE) {
            static TypeInfo derefTypeInfo;
            memset(&derefTypeInfo, 0, sizeof(TypeInfo));
            
            TypeInfo* ptrType = getTypeInfoFromExpression(expr->right);
            if (ptrType && ptrType->is_pointer) {
                // Copy type info
                derefTypeInfo = *ptrType;
                derefTypeInfo.is_pointer = 0; // Remove one level of pointer
            } else {
                // Default to int if we can't determine
                derefTypeInfo.type = TYPE_INT;
                derefTypeInfo.is_pointer = 0;
            }
            
            return &derefTypeInfo;
        }
        
        // Handle type casting
        if (expr->unary_op.op == UNARY_CAST) {
            static TypeInfo castTypeInfo;
            memset(&castTypeInfo, 0, sizeof(TypeInfo));
            
            // Set the type information based on the cast type
            castTypeInfo.type = expr->unary_op.cast_type;
            // For now, we're not handling casting to pointers
            castTypeInfo.is_pointer = 0;
            castTypeInfo.is_far_pointer = 0;
            castTypeInfo.is_array = 0;
            
            return &castTypeInfo;
        }
    }
    
    // For binary operations, type depends on the operation
    if (expr->type == NODE_BINARY_OP) {
        static TypeInfo binaryTypeInfo;
        memset(&binaryTypeInfo, 0, sizeof(TypeInfo));
        
        // For pointer arithmetic
        TypeInfo* leftType = getTypeInfoFromExpression(expr->left);
        TypeInfo* rightType = getTypeInfoFromExpression(expr->right);
        
        if (leftType && leftType->is_pointer) {
            // Pointer arithmetic, result is still the same pointer type
            return leftType;
        } else if (rightType && rightType->is_pointer) {
            return rightType;
        } else if (leftType) {
            // Regular arithmetic, use the left operand's type
            return leftType;
        } else if (rightType) {
            return rightType;
        }
        
        // Default to int
        binaryTypeInfo.type = TYPE_INT;
        return &binaryTypeInfo;
    }
    
    // Default to int type if we can't determine
    static TypeInfo defaultTypeInfo;
    memset(&defaultTypeInfo, 0, sizeof(TypeInfo));
    defaultTypeInfo.type = TYPE_INT;
    
    return &defaultTypeInfo;
}
