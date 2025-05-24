#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Create a new AST node
ASTNode* createNode(NodeType type) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: Failed to allocate memory for AST node\n");
        exit(1);
    }
    
    // Initialize the node
    memset(node, 0, sizeof(ASTNode));
    node->type = type;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    
    return node;
}

// Get the name of a node type for debugging
const char* getNodeTypeName(NodeType type) {
    switch (type) {
        case NODE_PROGRAM: return "PROGRAM";
        case NODE_FUNCTION: return "FUNCTION";
        case NODE_BLOCK: return "BLOCK";
        case NODE_DECLARATION: return "DECLARATION";
        case NODE_ASSIGNMENT: return "ASSIGNMENT";
        case NODE_BINARY_OP: return "BINARY_OP";
        case NODE_UNARY_OP: return "UNARY_OP";
        case NODE_IDENTIFIER: return "IDENTIFIER";
        case NODE_LITERAL: return "LITERAL";
        case NODE_RETURN: return "RETURN";
        case NODE_IF: return "IF";
        case NODE_WHILE: return "WHILE";
        case NODE_FOR: return "FOR";
        case NODE_CALL: return "CALL";
        case NODE_ASM_BLOCK: return "ASM_BLOCK";
        case NODE_ASM: return "ASM";
        case NODE_EXPRESSION: return "EXPRESSION";
        case NODE_TERNARY: return "TERNARY";
        default: return "UNKNOWN";
    }
}

// Get the name of a data type for debugging
const char* getDataTypeName(DataType type) {
    switch (type) {
        case TYPE_INT: return "int";
        case TYPE_SHORT: return "short";
        case TYPE_LONG: return "long";
        case TYPE_UNSIGNED_INT: return "unsigned int";
        case TYPE_UNSIGNED_SHORT: return "unsigned short";
        case TYPE_UNSIGNED_LONG: return "unsigned long";
        case TYPE_CHAR: return "char";
        case TYPE_UNSIGNED_CHAR: return "unsigned char";
        case TYPE_VOID: return "void";
        case TYPE_FAR_POINTER: return "far pointer";
        case TYPE_BOOL: return "bool";
        default: return "unknown";
    }
}

// Get the size of a data type in bytes
int getTypeSize(DataType type) {
    switch (type) {
        case TYPE_INT:
        case TYPE_SHORT:
        case TYPE_UNSIGNED_INT:
        case TYPE_UNSIGNED_SHORT:
            return 2;  // 16-bit
        case TYPE_LONG:
        case TYPE_UNSIGNED_LONG:
            return 4;  // 32-bit
        case TYPE_CHAR:
        case TYPE_UNSIGNED_CHAR:
        case TYPE_BOOL:
            return 1;  // 8-bit
        case TYPE_VOID:
            return 0;
        case TYPE_FAR_POINTER:
            return 4;  // 32-bit (segment:offset)
        default:
            return 0;
    }
}

// Print the AST for debugging
void printAST(ASTNode* node, int indent) {
    if (!node) return;
    
    // Print indentation
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    
    // Print node type
    printf("%s", getNodeTypeName(node->type));
    
    // Print node-specific information
    switch (node->type) {
        case NODE_FUNCTION:
            printf(" (name: %s, stackframe: %s)", 
                node->function.func_name,
                node->function.info.is_stackframe ? "yes" : "no");
            break;
        case NODE_DECLARATION:
            printf(" (name: %s, type: %s)", 
                node->declaration.var_name,
                getDataTypeName(node->declaration.type_info.type));
            break;
        case NODE_IDENTIFIER:
            printf(" (name: %s)", node->identifier);
            break;        case NODE_LITERAL:
            if (node->literal.data_type == TYPE_INT) {
                printf(" (value: %d)", node->literal.int_value);
            } else if (node->literal.data_type == TYPE_FAR_POINTER) {
                printf(" (far ptr: %04X:%04X)", node->literal.segment, node->literal.offset);
            } else if (node->literal.data_type == TYPE_BOOL) {
                printf(" (value: %s)", node->literal.int_value ? "true" : "false");
            }
            break;
        case NODE_CALL:
            printf(" (function: %s)", node->call.func_name);
            break;
        case NODE_ASM_BLOCK:
            printf(" (asm block)");
            break;
        case NODE_BINARY_OP:
            printf(" (op: %d)", node->operation.op);
            break;
        default:
            break;
    }
    
    printf("\n");
    
    // Print children
    if (node->left) {
        printAST(node->left, indent + 1);
    }
    if (node->right) {
        printAST(node->right, indent + 1);
    }
    
    // Print linked list nodes
    ASTNode* next = node->next;
    while (next) {
        printAST(next, indent);
        next = next->next;
    }
}