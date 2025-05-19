#include "ast_cleanup.h"
#include <stdlib.h>

// Free memory allocated for an AST node
void freeASTNode(ASTNode* node) {
    if (!node) return;
    
    // Free children first
    freeASTNode(node->left);
    freeASTNode(node->right);
    freeASTNode(node->next);
    
    // Free specific resources based on node type
    switch (node->type) {
        case NODE_IDENTIFIER:
            free(node->identifier);
            break;
            
        case NODE_LITERAL:
            if (node->literal.data_type == TYPE_CHAR && node->literal.string_value) {
                free(node->literal.string_value);
            }
            break;
            
        case NODE_DECLARATION:
            free(node->declaration.var_name);
            break;
            
        case NODE_FUNCTION:
            free(node->function.func_name);
            if (node->function.info.deprecation_msg) {
                free(node->function.info.deprecation_msg);
            }
            break;
            
        case NODE_ASM_BLOCK:
            free(node->asm_block.code);
            break;
            
        case NODE_ASM:
            free(node->asm_stmt.code);
            // Free operands and constraints if present
            if (node->asm_stmt.operands) {
                for (int i = 0; i < node->asm_stmt.operand_count; i++) {
                    // Note: operands are ASTNode* that are already freed by the recursive call
                    if (node->asm_stmt.constraints && node->asm_stmt.constraints[i]) {
                        free(node->asm_stmt.constraints[i]);
                    }
                }
                free(node->asm_stmt.operands);
                if (node->asm_stmt.constraints) {
                    free(node->asm_stmt.constraints);
                }
            }
            break;
            
        case NODE_CALL:
            free(node->call.func_name);
            break;
            
        default:
            break;
    }
    
    // Finally free the node itself
    free(node);
}
