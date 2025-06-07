#include "codegen.h"
#include "ast.h"
#include "error_manager.h"
#include "string_literals.h"  // Added for addArrayDeclaration
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// External functions
extern const char* getCurrentSourceFilename();

// Reference to the output file defined in codegen.c
extern FILE* asmFile;

// Forward declaration for the initialization list function
static void generateInitializerList(FILE* file, ASTNode* initializer, const char* directive);

// Add array with initializers to be generated later with the right data
void generateArrayWithInitializers(ASTNode* node) {
    if (!node || node->type != NODE_DECLARATION || 
        !node->declaration.type_info.is_array || 
        !node->declaration.initializer) {
        return;
    }
    
    // Register the array to be generated later at the proper location
    addArrayDeclarationWithInitializers(
        node->declaration.var_name,
        node->declaration.type_info.array_size,
        node->declaration.type_info.type,
        getCurrentFunctionName(),
        node->declaration.initializer,
        node->declaration.type_info.is_static
    );
}

// Called by string_literals.c during array generation
void writeArrayWithInitializers(FILE* outFile, const char* arrayName, int arraySize,
                               DataType arrayType, ASTNode* initializer) {
    // Determine directive based on element type
    const char* directive;
    if (arrayType == TYPE_CHAR || arrayType == TYPE_UNSIGNED_CHAR || arrayType == TYPE_BOOL) {
        directive = "db";
    } else {
        directive = "dw";
    }
    
    // Process initializers
    int count = 0;
    
    // If there are multiple initializers in a list
    if (initializer->next) {
        fprintf(outFile, "    %s ", directive);
        generateInitializerList(outFile, initializer, directive);
        
        // Count the initializers
        ASTNode* current = initializer;
        while (current) {
            count++;
            current = current->next;
        }
    } else {
        // Single initializer that might be a literal
        if (initializer->type == NODE_LITERAL) {
            if (initializer->literal.data_type == TYPE_CHAR && !initializer->literal.string_value) {
                // Character literal
                fprintf(outFile, "    %s '%c'", directive, initializer->literal.char_value);
            } else if (initializer->literal.data_type == TYPE_CHAR && initializer->literal.string_value) {
                // String literal - handle char by char
                fprintf(outFile, "    %s ", directive);
                const char* str = initializer->literal.string_value;
                // Remove surrounding quotes if present
                if (str[0] == '"' && str[strlen(str)-1] == '"') {
                    str++;
                    for (size_t i = 0; i < strlen(str) - 1; i++) {
                        fprintf(outFile, "%d", (unsigned char)str[i]);
                        if (i < strlen(str) - 2) {
                            fprintf(outFile, ", ");
                        }
                    }
                    fprintf(outFile, ", 0");  // Null terminator
                } else {
                    // Just emit the raw string
                    for (size_t i = 0; i < strlen(str); i++) {
                        fprintf(outFile, "%d", (unsigned char)str[i]);
                        if (i < strlen(str) - 1) {
                            fprintf(outFile, ", ");
                        }
                    }
                    fprintf(outFile, ", 0");  // Null terminator
                }
                count = strlen(str) + 1;  // Include null terminator
            } else {
                // Integer literal
                fprintf(outFile, "    %s %d", directive, initializer->literal.int_value);
                count = 1;
            }
            fprintf(outFile, "\n");
        }
    }
    
    // Fill remaining array elements with zeros if needed
    if (count < arraySize) {
        fprintf(outFile, "    #times %d %s 0\n", arraySize - count, directive);
    }
}

// Helper function to generate initializer list
static void generateInitializerList(FILE* file, ASTNode* initializer, const char* directive) {
    while (initializer) {
        if (initializer->type == NODE_LITERAL) {
            if (initializer->literal.data_type == TYPE_CHAR && !initializer->literal.string_value) {
                // Character literal
                fprintf(file, "'%c'", initializer->literal.char_value);
            } else if (initializer->literal.data_type == TYPE_CHAR && initializer->literal.string_value) {
                // String literal in an initializer list - not valid C, but handle anyway
                reportWarning(-1, "String literal in array initializer list is not valid C");
                fprintf(file, "0 ; Invalid string literal in initializer list");
            } else {
                // Integer literal
                fprintf(file, "%d", initializer->literal.int_value);
            }
        } else {
            // Non-literal initializer not fully supported
            fprintf(file, "0 ; Non-literal initializer not fully supported");
        }
        
        initializer = initializer->next;
        if (initializer) {
            fprintf(file, ", ");
        }
    }
    
    fprintf(file, "\n");
}
