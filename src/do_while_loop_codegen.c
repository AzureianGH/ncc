#include "codegen.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations from codegen.c
extern FILE* asmFile;
extern char* generateLabel(const char* prefix);
extern void generateStatement(ASTNode* node);
extern void generateExpression(ASTNode* node);
extern void generateBlock(ASTNode* node);

// Generate code for a do-while loop
void generateDoWhileLoop(ASTNode* node) {
    if (!node || node->type != NODE_DO_WHILE) return;
    
    // Generate labels for loop body start, condition check and end
    char* bodyLabel = generateLabel("do_body");
    char* condLabel = generateLabel("do_cond");
    char* endLabel = generateLabel("do_end");
    
    // Start with loop body
    fprintf(asmFile, "    ; Do-while loop\n");
    fprintf(asmFile, "%s:\n", bodyLabel);
    
    // Generate loop body
    if (node->do_while_loop.body) {
        fprintf(asmFile, "    ; Loop body\n");
        if (node->do_while_loop.body->type == NODE_BLOCK) {
            generateBlock(node->do_while_loop.body);
        } else {
            generateStatement(node->do_while_loop.body);
        }
    } else {
        fprintf(asmFile, "    ; Warning: Empty loop body\n");
    }
    
    // After body, check condition
    fprintf(asmFile, "%s:\n", condLabel);
    
    // Generate condition evaluation
    if (node->do_while_loop.condition) {
        generateExpression(node->do_while_loop.condition);
        
        // Test condition result and loop back if true
        fprintf(asmFile, "    test ax, ax\n");
        fprintf(asmFile, "    jnz %s\n", bodyLabel);
    }
    
    // End of loop
    fprintf(asmFile, "%s:\n", endLabel);
}
