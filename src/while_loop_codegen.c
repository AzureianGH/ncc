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
extern void pushLoopContext(const char* continueLabel, const char* breakLabel);
extern void popLoopContext();

// Generate code for a while loop
void generateWhileLoop(ASTNode* node) {
    if (!node || node->type != NODE_WHILE) return;
    
    // Generate labels for loop condition, loop body and end
    char* condLabel = generateLabel("while_cond");
    char* bodyLabel = generateLabel("while_body");
    char* endLabel = generateLabel("while_end");
      // Start with condition check
    fprintf(asmFile, "    ; While loop\n");
    fprintf(asmFile, "%s:\n", condLabel);
    
    // Push loop context for break/continue statements
    pushLoopContext(condLabel, endLabel);
      
    // Generate condition evaluation
    if (node->while_loop.condition) {
        generateExpression(node->while_loop.condition);
        
        // Test condition result and skip body if false
        fprintf(asmFile, "    test ax, ax\n");
        fprintf(asmFile, "    jz %s\n", endLabel);
    }
    
    // Body start label
    fprintf(asmFile, "%s:\n", bodyLabel);

    // Generate loop body
    if (node->while_loop.body) {
        fprintf(asmFile, "    ; Loop body\n");
        if (node->while_loop.body->type == NODE_BLOCK) {
            generateBlock(node->while_loop.body);
        } else {
            generateStatement(node->while_loop.body);
        }
    } else {
        fprintf(asmFile, "    ; Warning: Empty loop body\n");
    }
      // Jump back to condition after loop body execution
    fprintf(asmFile, "    jmp %s\n", condLabel);
    
    // End of loop
    fprintf(asmFile, "%s:\n", endLabel);
    
    // Pop loop context
    popLoopContext();
    
    // Free the allocated labels
    free(condLabel);
    free(bodyLabel);
    free(endLabel);
}
