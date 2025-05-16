#include "codegen.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations
extern FILE* asmFile;
extern char* generateLabel(const char* prefix);
extern void generateStatement(ASTNode* node);
extern void generateExpression(ASTNode* node);

// Generate code for an if statement
void generateIfStatement(ASTNode* node) {
    if (!node || node->type != NODE_IF) return;
    
    // Generate labels for the else part and end of if
    char* elseLabel = generateLabel("if_else");
    char* endLabel = generateLabel("if_end");
    
    fprintf(asmFile, "    ; If statement\n");
    
    // Generate code for the condition
    if (node->if_stmt.condition) {
        generateExpression(node->if_stmt.condition);
        
        // Test the condition and skip the if body if false
        fprintf(asmFile, "    test ax, ax\n");
        
        if (node->if_stmt.else_body) {
            // If there's an else branch, jump to it when condition is false
            fprintf(asmFile, "    jz %s\n", elseLabel);
        } else {
            // If no else branch, jump to the end when condition is false
            fprintf(asmFile, "    jz %s\n", endLabel);
        }
    } else {
        // No condition provided - default to false
        fprintf(asmFile, "    xor ax, ax\n"); // Set AX to 0 (false)
        fprintf(asmFile, "    jmp %s\n", endLabel);
    }
    
    // Generate code for the if body
    fprintf(asmFile, "    ; If true branch\n");
    if (node->if_stmt.if_body) {
        if (node->if_stmt.if_body->type == NODE_BLOCK) {
            // Process each statement in the block
            ASTNode* stmt = node->if_stmt.if_body->left;
            while (stmt) {
                generateStatement(stmt);
                stmt = stmt->next;
            }
        } else {
            // Single statement
            generateStatement(node->if_stmt.if_body);
        }
    }
    
    // If there's an else branch
    if (node->if_stmt.else_body) {
        // Jump to end after executing the if body to avoid executing else
        fprintf(asmFile, "    jmp %s\n", endLabel);
        
        // Else label and code
        fprintf(asmFile, "%s:\n", elseLabel);
        fprintf(asmFile, "    ; Else branch\n");
        
        if (node->if_stmt.else_body->type == NODE_BLOCK) {
            // Process each statement in the block
            ASTNode* stmt = node->if_stmt.else_body->left;
            while (stmt) {
                generateStatement(stmt);
                stmt = stmt->next;
            }
        } else {
            // Single statement
            generateStatement(node->if_stmt.else_body);
        }
    }
    
    // End of the if statement
    fprintf(asmFile, "%s:\n", endLabel);
}
