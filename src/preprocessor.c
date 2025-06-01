#include "preprocessor.h"
#include "error_manager.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Global array of macros
static Macro macros[MAX_MACROS];
static int numMacros = 0;

// Include paths
static char* includePaths[MAX_INCLUDE_PATHS];
static int numIncludePaths = 0;

// Array to track files already included with #pragma once
static char includedFiles[MAX_INCLUDED_FILES][MAX_FILENAME_LEN];
static int numIncludedFiles = 0;

// Initialize the preprocessor
void initPreprocessor() {
    // Reset macro table
    memset(macros, 0, sizeof(macros));
    numMacros = 0;
    
    // Reset include paths
    for (int i = 0; i < numIncludePaths; i++) {
        free(includePaths[i]);
    }
    memset(includePaths, 0, sizeof(includePaths));
    numIncludePaths = 0;
    
    // Reset pragma once tracking
    memset(includedFiles, 0, sizeof(includedFiles));
    numIncludedFiles = 0;
    
    // Define some built-in macros
    defineMacro("__NCC__", "65536");      // Compiler ID
    defineMacro("__NCC_MAJOR__", "1");    // Major version (1 == First Release)
    defineMacro("__NCC_MINOR__", "0");   // Minor version
    // 0.45v
    defineMacro("__x86_16__", "1");

}

// Add an include path
void addIncludePath(const char* path) {
    if (numIncludePaths >= MAX_INCLUDE_PATHS) {
        fprintf(stderr, "Error: Too many include paths, limit is %d\n", MAX_INCLUDE_PATHS);
        return;
    }
    
    includePaths[numIncludePaths++] = strdupc(path);
}

// Check if a file has been included with #pragma once
static int isFileAlreadyIncluded(const char* filename) {
    char normalizedPath[MAX_FILENAME_LEN];
    
    // Simple normalization - converting to lowercase for case-insensitive comparison
    // In a real implementation, you'd want to use absolute paths
    strncpy(normalizedPath, filename, MAX_FILENAME_LEN - 1);
    normalizedPath[MAX_FILENAME_LEN - 1] = '\0';
    
    for (int i = 0; normalizedPath[i]; i++) {
        normalizedPath[i] = tolower(normalizedPath[i]);
    }
    
    // Check if this file has already been included
    for (int i = 0; i < numIncludedFiles; i++) {
        if (strcmp(normalizedPath, includedFiles[i]) == 0) {
            return 1;
        }
    }
    
    // If not, add it to our list
    if (numIncludedFiles < MAX_INCLUDED_FILES) {
        strncpy(includedFiles[numIncludedFiles], normalizedPath, MAX_FILENAME_LEN - 1);
        includedFiles[numIncludedFiles][MAX_FILENAME_LEN - 1] = '\0';
        numIncludedFiles++;
    }
    
    return 0;
}

// Define a macro
void defineMacro(const char* name, const char* value) {
    // Check if we've reached the maximum number of macros
    if (numMacros >= MAX_MACROS) {
        fprintf(stderr, "Error: Too many macro definitions, limit is %d\n", MAX_MACROS);
        return;
    }
    
    // Check if macro already exists, update its value if it does
    for (int i = 0; i < numMacros; i++) {
        if (strcmp(macros[i].name, name) == 0) {
            strncpy(macros[i].value, value, MAX_MACRO_VALUE_LEN - 1);
            macros[i].value[MAX_MACRO_VALUE_LEN - 1] = '\0';
            macros[i].defined = 1;
            return;
        }
    }
    
    // Add new macro
    strncpy(macros[numMacros].name, name, MAX_MACRO_NAME_LEN - 1);
    macros[numMacros].name[MAX_MACRO_NAME_LEN - 1] = '\0';
    strncpy(macros[numMacros].value, value, MAX_MACRO_VALUE_LEN - 1);
    macros[numMacros].value[MAX_MACRO_VALUE_LEN - 1] = '\0';
    macros[numMacros].defined = 1;
    numMacros++;
}

// Check if a macro is defined
int isMacroDefined(const char* name) {
    for (int i = 0; i < numMacros; i++) {
        if (strcmp(macros[i].name, name) == 0) {
            return macros[i].defined;
        }
    }
    return 0;  // Not found
}

// Get the value of a defined macro
const char* getMacroValue(const char* name) {
    for (int i = 0; i < numMacros; i++) {
        if (strcmp(macros[i].name, name) == 0 && macros[i].defined) {
            return macros[i].value;
        }
    }
    return NULL;  // Not found or undefined
}

// Extract a macro name from the directive line
static char* extractMacroName(const char* line, int* pos) {
    static char buffer[MAX_MACRO_NAME_LEN];
    int len = 0;
    
    // Skip leading whitespace
    while (isspace(line[*pos])) (*pos)++;
    
    // Extract identifier (macro name)
    while (isalnum(line[*pos]) || line[*pos] == '_') {
        if (len < MAX_MACRO_NAME_LEN - 1) {
            buffer[len++] = line[*pos];
        }
        (*pos)++;
    }
    
    buffer[len] = '\0';
    return buffer;
}

// Extract a macro value from the directive line
static char* extractMacroValue(const char* line, int* pos) {
    static char buffer[MAX_MACRO_VALUE_LEN];
    int len = 0;
    
    // Skip leading whitespace
    while (isspace(line[*pos])) (*pos)++;
    
    // Extract the rest of the line as the value
    while (line[*pos] && line[*pos] != '\n' && line[*pos] != '\r') {
        if (len < MAX_MACRO_VALUE_LEN - 1) {
            buffer[len++] = line[*pos];
        }
        (*pos)++;
    }
    
    // Remove trailing whitespace
    while (len > 0 && isspace(buffer[len-1])) {
        len--;
    }
    
    buffer[len] = '\0';
    return buffer;
}

// Find the include file in include paths
static char* findIncludeFile(const char* filename, int isSystemHeader) {
    FILE* file = NULL;
    char fullPath[MAX_FILENAME_LEN];
    
    // If the filename is an absolute path or a path relative to current directory
    if (!isSystemHeader) {
        file = fopen(filename, "r");
        if (file) {
            fclose(file);
            return strdupc(filename);
        }
    }
    
    // Try each include path
    for (int i = 0; i < numIncludePaths; i++) {
        snprintf(fullPath, MAX_FILENAME_LEN, "%s/%s", includePaths[i], filename);
        file = fopen(fullPath, "r");
        if (file) {
            fclose(file);
            return strdupc(fullPath);
        }
    }
    
    return NULL;
}

// Read a file into memory
static char* readFileToString(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer for file content plus null terminator
    char* buffer = (char*)malloc(fileSize + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    // Read file content
    size_t bytesRead = fread(buffer, 1, fileSize, file);
    buffer[bytesRead] = '\0';
    
    fclose(file);
    return buffer;
}

// Process a file and return its content after preprocessing
char* preprocessFile(const char* filename) {
    // Check if the file was already processed with #pragma once
    if (isFileAlreadyIncluded(filename)) {
        return strdupc(""); // Return empty string for already included files
    }
    
    // Read file content
    char* fileContent = readFileToString(filename);
    if (!fileContent) {
        fprintf(stderr, "Error: Cannot read file '%s'\n", filename);
        return NULL;
    }
    
    // Update __FILE__ macro with current filename
    char filenameMacro[MAX_FILENAME_LEN + 3]; // +3 for quotes and null terminator
    snprintf(filenameMacro, sizeof(filenameMacro), "\"%s\"", filename);
    defineMacro("__FILE__", filenameMacro);
    
    // Process file content
    char* processedContent = preprocessSource(fileContent);
    free(fileContent);
    
    return processedContent;
}

// Process a single preprocessor directive line
static void processDirective(const char* line, int* ifLevel, int* skipLevel, const char* currentFilename) {
    int pos = 0;
    
    // Skip the '#' character
    pos++;
    
    // Skip any whitespace after the '#'
    while (isspace(line[pos])) pos++;
    
    // Debug output for directive processing
    #ifdef DEBUG_PREPROCESSOR
    fprintf(stderr, "Processing directive: %s (ifLevel=%d, skipLevel=%d)\n", line, *ifLevel, *skipLevel);
    #endif
    
    // Determine directive type
    if (strncmp(line + pos, "define", 6) == 0 && isspace(line[pos+6])) {
        // #define directive
        pos += 6;  // Skip "define"
        
        // Don't process directives in skipped blocks
        if (*skipLevel > 0) {
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  Skipping #define in false condition block\n");
            #endif
            return;
        }
        
        char* macroName = extractMacroName(line, &pos);
        char* macroValue = extractMacroValue(line, &pos);
        
        if (macroName[0] != '\0') {
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  Defining macro %s = '%s'\n", macroName, macroValue);
            #endif
            defineMacro(macroName, macroValue);
        }
    } 
    else if (strncmp(line + pos, "undef", 5) == 0 && isspace(line[pos+5])) {
        // #undef directive
        pos += 5;  // Skip "undef"
        
        // Don't process directives in skipped blocks
        if (*skipLevel > 0) {
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  Skipping #undef in false condition block\n");
            #endif
            return;
        }
        
        char* macroName = extractMacroName(line, &pos);
        
        #ifdef DEBUG_PREPROCESSOR
        fprintf(stderr, "  Undefining macro %s\n", macroName);
        #endif
        
        // Mark the macro as undefined if it exists
        for (int i = 0; i < numMacros; i++) {
            if (strcmp(macros[i].name, macroName) == 0) {
                macros[i].defined = 0;
                break;
            }
        }
    } 
    else if (strncmp(line + pos, "ifdef", 5) == 0 && isspace(line[pos+5])) {
        // #ifdef directive
        pos += 5;  // Skip "ifdef"
        (*ifLevel)++;
        
        // Only evaluate condition if not already skipping
        if (*skipLevel > 0) {
            (*skipLevel)++;
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  Already in false block, increasing skipLevel to %d\n", *skipLevel);
            #endif
            return;
        }
        
        char* macroName = extractMacroName(line, &pos);
        int defined = isMacroDefined(macroName);
        
        #ifdef DEBUG_PREPROCESSOR
        fprintf(stderr, "  #ifdef %s: macro is %s\n", macroName, defined ? "defined" : "not defined");
        #endif
        
        if (!defined) {
            *skipLevel = *ifLevel;  // Skip until matching endif or else
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  Condition false, setting skipLevel to %d\n", *skipLevel);
            #endif
        }
    } 
    else if (strncmp(line + pos, "ifndef", 6) == 0 && isspace(line[pos+6])) {
        // #ifndef directive
        pos += 6;  // Skip "ifndef"
        (*ifLevel)++;
        
        // Only evaluate condition if not already skipping
        if (*skipLevel > 0) {
            (*skipLevel)++;
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  Already in false block, increasing skipLevel to %d\n", *skipLevel);
            #endif
            return;
        }
        
        char* macroName = extractMacroName(line, &pos);
        int defined = isMacroDefined(macroName);
        
        #ifdef DEBUG_PREPROCESSOR
        fprintf(stderr, "  #ifndef %s: macro is %s\n", macroName, defined ? "defined" : "not defined");
        #endif
        
        if (defined) {
            *skipLevel = *ifLevel;  // Skip until matching endif or else
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  Condition false, setting skipLevel to %d\n", *skipLevel);
            #endif
        }
    } 
    else if (strncmp(line + pos, "if", 2) == 0 && isspace(line[pos+2])) {
        // #if directive
        pos += 2;  // Skip "if"
        (*ifLevel)++;
        
        // Only evaluate condition if not already skipping
        if (*skipLevel > 0) {
            (*skipLevel)++;
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  Already in false block, increasing skipLevel to %d\n", *skipLevel);
            #endif
            return;
        }
        
        // Skip whitespace
        while (isspace(line[pos])) pos++;
        
        // Extract and evaluate the condition
        const char* expr = line + pos;
        int result = evaluatePreprocessorExpression(expr);
        
        #ifdef DEBUG_PREPROCESSOR
        fprintf(stderr, "  #if expression evaluated to: %d\n", result);
        #endif
        
        if (!result) {
            *skipLevel = *ifLevel;  // Skip until matching endif or else
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  Condition false, setting skipLevel to %d\n", *skipLevel);
            #endif
        }
    } 
    else if (strncmp(line + pos, "else", 4) == 0 && 
             (isspace(line[pos+4]) || line[pos+4] == '\0' || line[pos+4] == '\n' || line[pos+4] == '\r')) {
        // #else directive
        #ifdef DEBUG_PREPROCESSOR
        fprintf(stderr, "  Processing #else directive\n");
        #endif
        
        // If we're at the matching if level of what we're skipping, toggle skip status
        if (*skipLevel == *ifLevel) {
            *skipLevel = 0;  // Stop skipping
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  Found matching else, stopping skipping\n");
            #endif
        } 
        // If we weren't skipping at this level, start skipping
        else if (*skipLevel == 0 && *ifLevel > 0) {
            *skipLevel = *ifLevel;
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  In taken branch, now skipping the else part (skipLevel=%d)\n", *skipLevel);
            #endif
        }
    } 
    else if (strncmp(line + pos, "endif", 5) == 0 && 
             (isspace(line[pos+5]) || line[pos+5] == '\0' || line[pos+5] == '\n' || line[pos+5] == '\r')) {
        // #endif directive
        #ifdef DEBUG_PREPROCESSOR
        fprintf(stderr, "  Processing #endif (ifLevel=%d, skipLevel=%d)\n", *ifLevel, *skipLevel);
        #endif
        
        // If this endif matches our skip level, stop skipping
        if (*skipLevel == *ifLevel) {
            *skipLevel = 0;
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  Found matching endif, stopping skipping\n");
            #endif
        } else if (*skipLevel > *ifLevel) {
            // Adjust skipLevel if we're in a nested if
            (*skipLevel)--;
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  Decreasing skipLevel to %d\n", *skipLevel);
            #endif
        }
        
        // Decrease nesting level
        if (*ifLevel > 0) (*ifLevel)--;
        
        #ifdef DEBUG_PREPROCESSOR
        fprintf(stderr, "  New ifLevel=%d, skipLevel=%d\n", *ifLevel, *skipLevel);
        #endif
    }
    else if (strncmp(line + pos, "org", 3) == 0 && isspace(line[pos+3])) {
        // #org directive (custom extension to set origin address)
        pos += 3;  // Skip "org"
        
        // Don't process directives in skipped blocks
        if (*skipLevel > 0) {
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  Skipping #org in false condition block\n");
            #endif
            return;
        }
        
        // Skip whitespace
        while (isspace(line[pos])) pos++;
        
        // Extract value (hexadecimal or decimal)
        char* orgValue = extractMacroValue(line, &pos);
        
        #ifdef DEBUG_PREPROCESSOR
        fprintf(stderr, "  Setting origin address to: %s\n", orgValue);
        #endif
        
        // Define a macro that can be used by code generator
        defineMacro("__ORG_ADDRESS__", orgValue);
    }
    else if (strncmp(line + pos, "include", 7) == 0 && isspace(line[pos+7])) {
        // #include directive
        pos += 7;  // Skip "include"
        
        // Don't process if skipping code in a false condition block
        if (*skipLevel > 0) {
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  Skipping #include in false condition block\n");
            #endif
            return;
        }
        
        // Skip whitespace
        while (isspace(line[pos])) pos++;
        
        // Determine include style (< > for system, " " for local)
        int isSystemHeader = 0;
        char includePath[MAX_FILENAME_LEN] = {0};
        int pathLen = 0;
        
        if (line[pos] == '<') {
            isSystemHeader = 1;
            pos++; // Skip <
            
            // Extract path until >
            while (line[pos] && line[pos] != '>' && pathLen < MAX_FILENAME_LEN - 1) {
                includePath[pathLen++] = line[pos++];
            }
            includePath[pathLen] = '\0';
            
            if (line[pos] != '>') {
                fprintf(stderr, "Error: Malformed #include directive, missing closing >\n");
                return;
            }
        }
        else if (line[pos] == '"') {
            pos++; // Skip "
            
            // Extract path until closing "
            while (line[pos] && line[pos] != '"' && pathLen < MAX_FILENAME_LEN - 1) {
                includePath[pathLen++] = line[pos++];
            }
            includePath[pathLen] = '\0';
            
            if (line[pos] != '"') {
                fprintf(stderr, "Error: Malformed #include directive, missing closing \"\n");
                return;
            }
        }
        else {
            fprintf(stderr, "Error: Malformed #include directive, expected < or \"\n");
            return;
        }
        
        #ifdef DEBUG_PREPROCESSOR
        fprintf(stderr, "  Including file: %s (system: %s)\n", includePath, isSystemHeader ? "yes" : "no");
        #endif
        
        // Find and process the include file
        char* resolvedPath = findIncludeFile(includePath, isSystemHeader);
        if (!resolvedPath) {
            reportError(-1, "Cannot find include file '%s'", includePath);
            return;
        }
        
        // Process the include file recursively
        char* includedContent = preprocessFile(resolvedPath);
        free(resolvedPath);
        
        if (!includedContent) {
            fprintf(stderr, "Error: Failed to preprocess include file '%s'\n", includePath);
            return;
        }
        
        // The included content will be inserted in place of the #include directive
        // by the calling function
        free(includedContent);
    }
    else if (strncmp(line + pos, "pragma", 6) == 0 && isspace(line[pos+6])) {
        pos += 6;  // Skip "pragma"
        
        // Don't process if skipping code in a false condition block
        if (*skipLevel > 0) {
            #ifdef DEBUG_PREPROCESSOR
            fprintf(stderr, "  Skipping #pragma in false condition block\n");
            #endif
            return;
        }
        
        // Skip whitespace
        while (isspace(line[pos])) pos++;
        
        // Check for "once" directive
        if (strncmp(line + pos, "once", 4) == 0) {
            // Mark the current file as included with #pragma once
            if (currentFilename) {
                isFileAlreadyIncluded(currentFilename);
                #ifdef DEBUG_PREPROCESSOR
                fprintf(stderr, "  Marked file as #pragma once: %s\n", currentFilename);
                #endif
            }
        }
        // Other pragma directives can be added here
    }
}

// Helper function to check if a character can be part of an identifier
static int isIdentifierChar(char c) {
    return isalnum(c) || c == '_';
}

// Check if this is a valid identifier boundary (start or end)
static int isIdentifierBoundary(char c) {
    return !isIdentifierChar(c);
}

// Process source code with preprocessor directives
char* preprocessSource(const char* source) {
    if (!source) return NULL;
    
    // Output buffer that will grow as needed
    size_t outCapacity = strlen(source) * 2;  // Initial capacity (2x source length)
    char* output = (char*)malloc(outCapacity);
    if (!output) return NULL;
    
    size_t outLen = 0;  // Current length of output
    int lineStart = 1;  // Flag to indicate start of a line
    int skipLevel = 0;  // Current level of code skipping (for #ifdef/#ifndef)
    int ifLevel = 0;    // Current nesting level of #if directives
    
    // Buffer for a line with a directive
    char directiveBuffer[4096];
    size_t directiveLen = 0;

    // Buffer for identifier comparison
    char identBuffer[MAX_MACRO_NAME_LEN];
    size_t identLen = 0;
    
    // Process source character by character
    for (size_t i = 0; source[i]; i++) {
        char c = source[i];
        
        // Check for directive at the start of a line
        if (lineStart && c == '#') {
            // Reset directive buffer
            directiveLen = 0;
            directiveBuffer[directiveLen++] = c;
            
            // Collect the entire directive line
            while (source[i+1] && source[i+1] != '\n') {
                i++;
                if (directiveLen < sizeof(directiveBuffer) - 1) {
                    directiveBuffer[directiveLen++] = source[i];
                }
            }
            directiveBuffer[directiveLen] = '\0';
            
            // Process the directive
            processDirective(directiveBuffer, &ifLevel, &skipLevel, NULL);
            
            // Move to the next line
            lineStart = 1;
            continue;
        }
        
        // Check if this is the start of a new line
        if (c == '\n' || c == '\r') {
            lineStart = 1;
            
            // Don't add to output if skipping code
            if (skipLevel > 0) continue;
            
            // Add newline to output
            if (outLen + 1 >= outCapacity) {
                outCapacity *= 2;
                output = (char*)realloc(output, outCapacity);
                if (!output) return NULL;
            }
            output[outLen++] = c;
            continue;
        }
        
        // After first non-whitespace character, we're no longer at line start
        if (!isspace(c)) {
            lineStart = 0;
        }
        
        // Skip output if we're in a false #if block
        if (skipLevel > 0) continue;

        // Look for identifiers to replace with macro values
        if (isalpha(c) || c == '_') {
            // Possible identifier start - check if it's a boundary
            int prevCharPos = i > 0 ? i-1 : -1u;
            if (prevCharPos < 0 || isIdentifierBoundary(source[prevCharPos])) {
                // Start of an identifier - collect it
                identLen = 0;
                identBuffer[identLen++] = c;
                
                // Gather the entire identifier
                size_t j = i + 1;
                while (source[j] && (isalnum(source[j]) || source[j] == '_')) {
                    if (identLen < MAX_MACRO_NAME_LEN - 1) {
                        identBuffer[identLen++] = source[j];
                    }
                    j++;
                }
                identBuffer[identLen] = '\0';
                
                // Check if it's a defined macro and should be replaced
                const char* value = getMacroValue(identBuffer);
                if (value != NULL && source[j] && isIdentifierBoundary(source[j])) {
                    // It's a macro and next char is valid boundary - replace it
                    i = j - 1; // Skip the identifier, will increment in the loop
                    
                    // Add the replacement value to output
                    for (size_t k = 0; value[k]; k++) {
                        if (outLen + 1 >= outCapacity) {
                            outCapacity *= 2;
                            output = (char*)realloc(output, outCapacity);
                            if (!output) return NULL;
                        }
                        output[outLen++] = value[k];
                    }
                    continue;
                }
            }
        }
        
        // Add the character to output
        if (outLen + 1 >= outCapacity) {
            outCapacity *= 2;
            output = (char*)realloc(output, outCapacity);
            if (!output) return NULL;
        }
        output[outLen++] = c;
    }
    
    // Add null terminator
    if (outLen + 1 >= outCapacity) {
        outCapacity++;
        output = (char*)realloc(output, outCapacity);
        if (!output) return NULL;
    }
    output[outLen] = '\0';
    
    return output;
}

// Free preprocessor resources
void cleanupPreprocessor() {
    // Reset the macro table
    memset(macros, 0, sizeof(macros));
    numMacros = 0;
    
    // Free include paths
    for (int i = 0; i < numIncludePaths; i++) {
        free(includePaths[i]);
    }
    numIncludePaths = 0;
    
    // Reset pragma once tracking
    memset(includedFiles, 0, sizeof(includedFiles));
    numIncludedFiles = 0;
}