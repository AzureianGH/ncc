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

// Generate code for a while loop
void generateWhileLoop(ASTNode* node) {
    if (!node || node->type != NODE_WHILE) return;
    
    // Generate labels for loop condition and end
    char* condLabel = generateLabel("while_cond");
    char* endLabel = generateLabel("while_end");
    
    // Start with condition check
    fprintf(asmFile, "    ; While loop\n");
    fprintf(asmFile, "%s:\n", condLabel);
    
    // Generate condition evaluation
    if (node->while_loop.condition) {
        generateExpression(node->while_loop.condition);
        
        // Test condition result and skip body if false
        fprintf(asmFile, "    test ax, ax\n");
        fprintf(asmFile, "    jz %s\n", endLabel);
    }
    
    // Generate loop body
    if (node->while_loop.body) {
        generateStatement(node->while_loop.body);
    }
    
    // Jump back to condition
    fprintf(asmFile, "    jmp %s\n", condLabel);
    
    // End of loop
    fprintf(asmFile, "%s:\n", endLabel);
}
