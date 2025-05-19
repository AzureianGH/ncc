#include "codegen.h"
#include "ast.h"
#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// External functions
extern const char* getCurrentSourceFilename();

// Reference to the output file defined in codegen.c
extern FILE* asmFile;

// Generate code for an array with initializers
void generateArrayWithInitializers(ASTNode* node) {
    if (!node || node->type != NODE_DECLARATION || 
        !node->declaration.type_info.is_array || 
        !node->declaration.initializer) {
        fprintf(asmFile, "; DEBUG: Invalid node for array initializer\n");
        return;
    }
      // Get sanitized filename for static array prefix
    const char* filename = getCurrentSourceFilename();
    char* prefix = strdup(filename);
    
    // Remove extension and sanitize for label use
    char* dot = strrchr(prefix, '.');
    if (dot) *dot = '\0';
    
    // Replace invalid characters with underscore
    for (char* c = prefix; *c; c++) {
        if (!isalnum(*c) && *c != '_') {
            *c = '_';
        }
    }

    // Output array label
    if (node->declaration.type_info.is_static) {
        fprintf(asmFile, "; Static array with initializers: %s[%d]\n", 
                node->declaration.var_name, 
                node->declaration.type_info.array_size);
        fprintf(asmFile, "_%s_%s:\n", prefix, node->declaration.var_name);
    } else {
        fprintf(asmFile, "; Array with initializers: %s[%d]\n", 
                node->declaration.var_name, 
                node->declaration.type_info.array_size);
        fprintf(asmFile, "_%s_%s:\n", prefix, node->declaration.var_name);
    }
    
    // Free the prefix
    free(prefix);
    
    // Determine directive based on element type
    const char* directive;
    if (node->declaration.type_info.type == TYPE_CHAR || 
        node->declaration.type_info.type == TYPE_UNSIGNED_CHAR) {
        directive = "db";
    } else {
        directive = "dw";
    }
    
    // Process initializers
    ASTNode* initializer = node->declaration.initializer;
    int count = 0;
    
    // If there are multiple initializers in a list
    if (initializer->next) {
        fprintf(asmFile, "    %s ", directive);
        
        // Output each initializer
        while (initializer) {
            if (initializer->type == NODE_LITERAL) {
                if (initializer->literal.data_type == TYPE_CHAR) {
                    // Character literal
                    fprintf(asmFile, "'%c'", initializer->literal.char_value);
                } else {
                    // Integer literal
                    fprintf(asmFile, "%d", initializer->literal.int_value);
                }
            } else {
                // Non-literal initializer not fully supported
                fprintf(asmFile, "0 ; Non-literal initializer not fully supported");
            }
            
            count++;
            initializer = initializer->next;
            
            if (initializer) {
                fprintf(asmFile, ", ");
            }
        }
        
        fprintf(asmFile, "\n");
    } else {
        // Single initializer that might be a literal
        if (initializer->type == NODE_LITERAL) {
            if (initializer->literal.data_type == TYPE_CHAR) {
                // Character literal
                fprintf(asmFile, "    %s '%c'\n", directive, initializer->literal.char_value);
            } else {
                // Integer literal
                fprintf(asmFile, "    %s %d\n", directive, initializer->literal.int_value);
            }
            count = 1;
        }
    }
      // Fill remaining array elements with zeros if needed
    if (count < node->declaration.type_info.array_size) {
        fprintf(asmFile, "    times %d %s 0\n", 
                node->declaration.type_info.array_size - count, directive);
    }
    
    fprintf(asmFile, "\n");
}
