#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum number of macros that can be defined
#define MAX_MACROS 1024

// Maximum length of macro name and definition
#define MAX_MACRO_NAME_LEN 64
#define MAX_MACRO_VALUE_LEN 1024

// Maximum number of include paths
#define MAX_INCLUDE_PATHS 64

// Maximum number of included files to track for #pragma once
#define MAX_INCLUDED_FILES 256

// Maximum length of a filename
#define MAX_FILENAME_LEN 256

// Macro definition structure
typedef struct {
    char name[MAX_MACRO_NAME_LEN];
    char value[MAX_MACRO_VALUE_LEN];
    int defined;  // 1 if defined, 0 if not (used for #undef)
} Macro;

// Initialize the preprocessor
void initPreprocessor();

// Add include path for header file resolution
void addIncludePath(const char* path);

// Process source code with preprocessor directives
// Returns a newly allocated string with processed source code
char* preprocessSource(const char* source);

// Process a file and return its content after preprocessing
char* preprocessFile(const char* filename);

// Define a macro programmatically (used for built-in macros)
void defineMacro(const char* name, const char* value);

// Check if a macro is defined
int isMacroDefined(const char* name);

// Get the value of a defined macro (returns NULL if undefined)
const char* getMacroValue(const char* name);

// Evaluate a preprocessing expression (for #if directive)
// Returns 1 if the expression evaluates to non-zero, 0 otherwise
int evaluatePreprocessorExpression(const char* expr);

#endif // PREPROCESSOR_H