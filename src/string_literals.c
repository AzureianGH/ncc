#include "codegen.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Globals from codegen.c
extern FILE* asmFile;
extern int stringLiteralCount;
extern char** stringLiterals;

// Function to escape string for assembly
char* escapeStringForAsm(const char* input) {
    if (!input) return NULL;
    
    size_t len = strlen(input);
    // Allocate memory for worst case (every char needs escaping)
    char* output = (char*)malloc(2 * len + 1);
    if (!output) return NULL;
    
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        switch (input[i]) {
            case '\\':
                // Handle escape sequences
                i++;
                if (i < len) {
                    switch (input[i]) {
                        case 'n': output[j++] = 10; break;  // Newline
                        case 'r': output[j++] = 13; break;  // Carriage return
                        case 't': output[j++] = 9; break;   // Tab
                        case '0': output[j++] = 0; break;   // Null
                        case '\\': output[j++] = '\\'; break; // Backslash
                        case '\"': output[j++] = '\"'; break; // Quote
                        default: output[j++] = input[i]; break;
                    }
                }
                break;
            default:
                output[j++] = input[i];
                break;
        }
    }
    output[j] = '\0';
    return output;
}

// Add a string literal to the string table and return its index
int addStringLiteral(const char* str) {
    // Strip quotes from the string
    size_t len = strlen(str);
    if (len >= 2 && str[0] == '\"' && str[len-1] == '\"') {
        // Remove the surrounding quotes and handle escape sequences
        char* unquoted = (char*)malloc(len - 1); // -2 for quotes, +1 for null
        if (!unquoted) return -1;
        
        strncpy(unquoted, str + 1, len - 2);
        unquoted[len - 2] = '\0';
        
        // Process escape sequences
        char* escaped = escapeStringForAsm(unquoted);
        free(unquoted);
        
        // Add to string table
        stringLiterals = (char**)realloc(stringLiterals, (stringLiteralCount + 1) * sizeof(char*));
        stringLiterals[stringLiteralCount] = escaped;
        return stringLiteralCount++;
    }
    
    return -1;
}

// Generate data section with string literals
void generateStringLiteralsSection() {
    if (stringLiteralCount == 0) return;
    
    fprintf(asmFile, "\n; String literals section\nsection .data\n");
    
    for (int i = 0; i < stringLiteralCount; i++) {
        fprintf(asmFile, "string_%d: db ", i);
        
        // Output each byte of the string as a decimal value
        for (size_t j = 0; j < strlen(stringLiterals[i]); j++) {
            fprintf(asmFile, "%d", (unsigned char)stringLiterals[i][j]);
            if (j < strlen(stringLiterals[i]) - 1) {
                fprintf(asmFile, ", ");
            }
        }
        fprintf(asmFile, ", 0  ; null terminator\n");
    }
    
    fprintf(asmFile, "\nsection .text\n");
}
