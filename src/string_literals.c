#include "codegen.h"
#include "ast.h"
#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Globals from codegen.c
extern FILE* asmFile;
extern int stringLiteralCount;
extern char** stringLiterals;

// New globals for array handling
extern int arrayCount;
extern char** arrayNames;
extern int* arraySizes;
extern DataType* arrayTypes;

// Has the string/array marker been found?
extern int stringMarkerFound;
extern int arrayMarkerFound;

// Function to sanitize filename for use as identifier prefix
char* getSanitizedFilenamePrefix() {
    const char* filename = getCurrentSourceFilename();
    char* prefix = (char*)malloc(strlen(filename) + 1);
    if (!prefix) return NULL;
    
    strcpy(prefix, filename);
    
    // Remove extension
    char* dot = strrchr(prefix, '.');
    if (dot) *dot = '\0';
    
    // Replace invalid characters with underscore
    for (char* c = prefix; *c; c++) {
        if (!isalnum(*c) && *c != '_') {
            *c = '_';
        }
    }
    
    return prefix;
}

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
    if (!str) {
        fprintf(stderr, "Debug: Null string passed to addStringLiteral\n");
        return -1;
    }
    
    // Make a copy of the string to work with
    char* workStr = strdup(str);
    if (!workStr) {
        fprintf(stderr, "Debug: Failed to allocate memory for string copy\n");
        return -1;
    }
    
    // Strip quotes if present
    size_t len = strlen(workStr);
    char* unquoted = NULL;
    
    // Check if the string has surrounding quotes
    if (len >= 2 && workStr[0] == '\"' && workStr[len-1] == '\"') {
        // Remove the surrounding quotes
        unquoted = (char*)malloc(len - 1); // -2 for quotes, +1 for null
        if (!unquoted) {
            fprintf(stderr, "Debug: Failed to allocate memory for unquoted string\n");
            free(workStr);
            return -1;
        }
        
        strncpy(unquoted, workStr + 1, len - 2);
        unquoted[len - 2] = '\0';
        free(workStr);
    } else {
        // No quotes, use as is
        unquoted = workStr;
    }
    
    // Process escape sequences
    char* escaped = escapeStringForAsm(unquoted);
    free(unquoted);
    
    if (!escaped) {
        fprintf(stderr, "Debug: Failed to process escape sequences\n");
        return -1;
    }
    
    // Add to string table
    if (stringLiterals == NULL) {
        stringLiterals = (char**)malloc(sizeof(char*));
        if (!stringLiterals) {
            fprintf(stderr, "Debug: Failed to allocate memory for string table\n");
            free(escaped);
            return -1;
        }
    } else {
        char** newTable = (char**)realloc(stringLiterals, (stringLiteralCount + 1) * sizeof(char*));
        if (!newTable) {
            fprintf(stderr, "Debug: Failed to reallocate memory for string table\n");
            free(escaped);
            return -1;
        }
        stringLiterals = newTable;
    }
    
    stringLiterals[stringLiteralCount] = escaped;
    int index = stringLiteralCount++;
    
    return index;
}

// Add an array declaration to track it for generation later
int addArrayDeclaration(const char* name, int size, DataType type) {
    // Initialize arrays if first time
    if (arrayNames == NULL) {
        arrayNames = (char**)malloc(sizeof(char*));
        arraySizes = (int*)malloc(sizeof(int));
        arrayTypes = (DataType*)malloc(sizeof(DataType));
        if (!arrayNames || !arraySizes || !arrayTypes) {
            fprintf(stderr, "Debug: Failed to allocate memory for array tracking\n");
            return -1;
        }
    } else {
        // Expand arrays
        char** newNames = (char**)realloc(arrayNames, (arrayCount + 1) * sizeof(char*));
        int* newSizes = (int*)realloc(arraySizes, (arrayCount + 1) * sizeof(int));
        DataType* newTypes = (DataType*)realloc(arrayTypes, (arrayCount + 1) * sizeof(DataType));
        
        if (!newNames || !newSizes || !newTypes) {
            fprintf(stderr, "Debug: Failed to reallocate memory for array tracking\n");
            return -1;
        }
        
        arrayNames = newNames;
        arraySizes = newSizes;
        arrayTypes = newTypes;
    }
    
    // Store array info
    arrayNames[arrayCount] = strdup(name);
    if (!arrayNames[arrayCount]) {
        fprintf(stderr, "Debug: Failed to allocate memory for array name\n");
        return -1;
    }
    
    arraySizes[arrayCount] = size;
    arrayTypes[arrayCount] = type;
    
    return arrayCount++;
}

char** existingStrings = NULL;

// Generate string literals at specific location (after marker function)
void generateStringsAtMarker() {
    // Get access to the redefine flag from codegen.c
    extern int redefineLocalsFound;
    extern int redefineStringStartIndex;
    
    // Count how many strings already exist
    int existingCount = 0;
    if (existingStrings) {
        while (existingStrings[existingCount] != NULL) {
            existingCount++;
        }
    }

    // Skip if strings were already generated and we're not redefining locations
    if ((stringMarkerFound && !redefineLocalsFound) || stringLiteralCount == 0) return;
    
    // Mark that strings have been generated
    stringMarkerFound = 1;
    
    fprintf(asmFile, "; String literals placed at _NCC_STRING_LOC%s\n", 
            redefineLocalsFound ? " (redefined)" : "");
    
    // Get sanitized filename prefix
    char* prefix = getSanitizedFilenamePrefix();
    if (!prefix) prefix = strdup("unknown");
    
    // If redefining, only generate strings added after the redefine marker
    int startIdx = redefineLocalsFound ? redefineStringStartIndex : 0;
    
    for (int i = startIdx; i < stringLiteralCount; i++) {
        // Offset the index by the number of already existing strings
        fprintf(asmFile, "%s_string_%d: db ", prefix, existingCount + (i - startIdx));
        
        // Output each byte of the string as a decimal value
        size_t len = strlen(stringLiterals[i]);
        for (size_t j = 0; j < len; j++) {
            fprintf(asmFile, "%d", (unsigned char)stringLiterals[i][j]);
            if (j < len - 1) {
                fprintf(asmFile, ", ");
            }
        }
        fprintf(asmFile, ", 0  ; null terminator\n");
    }
    
    free(prefix);
    fprintf(asmFile, "; String literal location marker%s\n", redefineLocalsFound ? " (redefined)" : "");
}

// Forward declaration
extern void generateArrayWithInitializers(ASTNode* node);

// Generate array declarations at specific location (after marker function)
void generateArraysAtMarker() {
    // Get access to the redefine flag from codegen.c
    extern int redefineLocalsFound;
    extern int redefineArrayStartIndex;
    
    // Skip if arrays were already generated and we're not redefining locations
    if ((arrayMarkerFound && !redefineLocalsFound) || arrayCount == 0) return;
      
    // Mark that arrays have been generated
    arrayMarkerFound = 1;
      
    fprintf(asmFile, "; Array declarations placed at _NCC_ARRAY_LOC%s\n",
            redefineLocalsFound ? " (redefined)" : "");
    
    // Get sanitized filename prefix
    char* prefix = getSanitizedFilenamePrefix();
    if (!prefix) prefix = strdup("unknown");
    
    // Determine starting index based on whether we're redefining
    int startIdx = redefineLocalsFound ? redefineArrayStartIndex : 0;
    
    for (int i = startIdx; i < arrayCount; i++) {
        fprintf(asmFile, "_%s_%s: ", prefix, arrayNames[i]);
        
        // Determine element size and directive
        const char* directive;
        int elementSize;
        
        switch (arrayTypes[i]) {
            case TYPE_CHAR:
            case TYPE_UNSIGNED_CHAR:
            case TYPE_BOOL:
                directive = "db";
                elementSize = 1;
                break;
            default:
                directive = "dw";
                elementSize = 2;
                break;
        }
        
        // Output array declaration with zeros
        fprintf(asmFile, "times %d %s 0 ; Array of %d bytes\n", 
                arraySizes[i], directive, arraySizes[i] * elementSize);
    }
}

// Generate data section with string literals and arrays
void generateStringLiteralsSection() {
    // Skip if nothing to generate or everything is already generated
    if ((stringLiteralCount == 0 || stringMarkerFound) && 
        (arrayCount == 0 || arrayMarkerFound)) return;
    
    fprintf(asmFile, "\n; Data section for strings and arrays\n");
    
    // Generate string literals if not already done
    if (!stringMarkerFound && stringLiteralCount > 0) {        fprintf(asmFile, "; String literals section\n");
        
        // Get sanitized filename prefix
        char* prefix = getSanitizedFilenamePrefix();
        if (!prefix) prefix = strdup("unknown");
        
        for (int i = 0; i < stringLiteralCount; i++) {
            fprintf(asmFile, "%s_string_%d: db ", prefix, i);
            
            // Output each byte of the string as a decimal value
            size_t len = strlen(stringLiterals[i]);
            for (size_t j = 0; j < len; j++) {
                fprintf(asmFile, "%d", (unsigned char)stringLiterals[i][j]);
                if (j < len - 1) {
                    fprintf(asmFile, ", ");
                }            }
            fprintf(asmFile, ", 0  ; null terminator\n");
        }
        
        // Free the prefix
        free(prefix);
    }
    
    // Generate array declarations if not already done
    if (!arrayMarkerFound && arrayCount > 0) {        fprintf(asmFile, "\n; Array declarations section\n");
        
        // Get sanitized filename prefix
        char* prefix = getSanitizedFilenamePrefix();
        if (!prefix) prefix = strdup("unknown");
        
        for (int i = 0; i < arrayCount; i++) {
            fprintf(asmFile, "_%s_%s: ", prefix, arrayNames[i]);
              // Determine element size and directive
            const char* directive;
            int elementSize;
            
            switch (arrayTypes[i]) {
                case TYPE_CHAR:
                case TYPE_UNSIGNED_CHAR:
                case TYPE_BOOL:
                    directive = "db";
                    elementSize = 1;
                    break;
                default:
                    directive = "dw";
                    elementSize = 2;
                    break;
            }
            
            // Output array declaration with zeros
            fprintf(asmFile, "times %d %s 0 ; Array of %d bytes\n",                arraySizes[i], directive, arraySizes[i] * elementSize);
        }
        
        // Free the prefix
        free(prefix);
    }
}
