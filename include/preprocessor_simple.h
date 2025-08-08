#ifndef PREPROCESSOR_SIMPLE_H
#define PREPROCESSOR_SIMPLE_H

#include <stdio.h>

// Forward declarations
typedef struct MacroData MacroData;

typedef struct {
    char* name;
    char* replacement;
    int isFunction;
    MacroData* data;
} Macro;

typedef struct {
    Macro* macros;
    int macroCount;
    int maxMacros;
    char** includePaths;
    int fileStackDepth;
    char* currentFile;
} Preprocessor;

extern Preprocessor preprocessor;

// Core functions
void initPreprocessor(void);
void cleanupPreprocessor(void);
void preprocessorError(const char* format, ...);
char* preprocessSource(const char* source);
char* preprocessFile(const char* filename);
char* preprocessSource(const char* source); // Add missing function

// Macro functions
Macro* createMacro(const char* name, const char* replacement, int isFunction);
void addMacro(Macro* macro);
void removeMacro(const char* name);

// Include functions
void addIncludePath(const char* path);
void clearIncludePaths(void);

#endif // PREPROCESSOR_SIMPLE_H
