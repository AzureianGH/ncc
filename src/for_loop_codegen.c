#include "codegen.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations from codegen.c
extern FILE* asmFile;
extern char* currentFunction;
extern char* generateLabel(const char* prefix);
extern void generateStatement(ASTNode* node);
extern void generateExpression(ASTNode* node);
extern void generateBlock(ASTNode* node);  // Add forward declaration for blocks

// Generate code for a for loop
void generateForLoop(ASTNode* node) {
    if (!node || node->type != NODE_FOR) return;
    
    fprintf(asmFile, "    ; For loop\n");
    
    // Generate labels for the start, condition, update, and end of the loop
    char* startLabel = generateLabel("for_start");
    char* condLabel = generateLabel("for_cond");
    char* updateLabel = generateLabel("for_update");
    char* endLabel = generateLabel("for_end");
    
    // Generate initialization code
    if (node->for_loop.init) {
        fprintf(asmFile, "    ; For loop initialization\n");
        generateStatement(node->for_loop.init);
    }
    
    // Jump to condition check
    fprintf(asmFile, "    jmp %s\n", condLabel);
      // Start of the loop body
    fprintf(asmFile, "%s:\n", startLabel);
    
    // Generate code for the loop body
    if (node->for_loop.body) {
        fprintf(asmFile, "    ; For loop body\n");
        if (node->for_loop.body->type == NODE_BLOCK) {
            generateBlock(node->for_loop.body);
        } else {
            generateStatement(node->for_loop.body);
        }
    }
    
    // Generate update code
    fprintf(asmFile, "%s:\n", updateLabel);
    if (node->for_loop.update) {
        fprintf(asmFile, "    ; For loop update\n");
        generateStatement(node->for_loop.update);
    }
    
    // Condition check
    fprintf(asmFile, "%s:\n", condLabel);
    if (node->for_loop.condition) {
        // Generate condition code
        fprintf(asmFile, "    ; For loop condition\n");
        generateExpression(node->for_loop.condition);
        
        // Test the result and jump back to the start if true (non-zero)
        fprintf(asmFile, "    test ax, ax\n");
        fprintf(asmFile, "    jnz %s\n", startLabel);
    } else {
        // No condition means always loop
        fprintf(asmFile, "    jmp %s ; Unconditional loop\n", startLabel);
    }
    
    // End of the loop
    fprintf(asmFile, "%s:\n", endLabel);
    
    // Free the labels
    free(startLabel);
    free(condLabel);
    free(updateLabel);
    free(endLabel);
}
