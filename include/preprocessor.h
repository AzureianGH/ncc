#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <stdio.h>

// Preprocessor directive types
typedef enum {
    PP_INCLUDE,
    PP_DEFINE,
    PP_UNDEF,
    PP_IF,
    PP_IFDEF,
    PP_IFNDEF,
    PP_ELIF,
    PP_ELSE,
    PP_ENDIF,
    PP_LINE,
    PP_ERROR,
    PP_WARNING,     // GCC extension
    PP_PRAGMA,
    PP_UNKNOWN
} PreprocessorDirective;

// Macro types
typedef enum {
    MACRO_OBJECT,       // #define NAME value
    MACRO_FUNCTION,     // #define NAME(args) value
    MACRO_PREDEFINED    // __FILE__, __LINE__, etc.
} MacroType;

// Macro structure
typedef struct Macro {
    char* name;
    MacroType type;
    char* replacement;
    char** parameters;
    int paramCount;
    int isVariadic;         // C99 variadic macros
    int line;
    char* filename;
    struct Macro* next;
} Macro;

// Include path structure
typedef struct IncludePath {
    char* path;
    int isSystem;
    struct IncludePath* next;
} IncludePath;

// Conditional compilation stack
typedef struct CondStack {
    int condition;
    int wasTrue;
    int line;
    struct CondStack* next;
} CondStack;

// Source location tracking
typedef struct SourceLocation {
    char* filename;
    int line;
    int originalLine;
    char* originalFile;
} SourceLocation;

// Preprocessor state
typedef struct Preprocessor {
    // Macros
    Macro* macros;
    int macroCount;
    
    // Include paths
    IncludePath* includePaths;
    IncludePath* systemPaths;
    
    // Conditional compilation
    CondStack* condStack;
    int skipLevel;
    
    // Source tracking
    SourceLocation* locations;
    int locationCount;
    int locationCapacity;
    
    // Files
    FILE** fileStack;
    char** filenameStack;
    int* lineStack;
    int fileStackDepth;
    int maxIncludeDepth;
    
    // Options
    struct {
        unsigned int enableTrigraphs : 1;
        unsigned int enableC99 : 1;
        unsigned int enableGNU : 1;
        unsigned int keepComments : 1;
        unsigned int enableWarnings : 1;
        unsigned int expandMacrosInStrings : 1;
    } options;
    
    // Current state
    char* currentFile;
    int currentLine;
    char* buffer;
    int bufferSize;
    int bufferCapacity;
    
    // Error handling
    int errorCount;
    int warningCount;
} Preprocessor;

// Function declarations

// Initialization and cleanup
void initPreprocessor(void);
void cleanupPreprocessor(void);

// Main preprocessing functions
char* preprocessFile(const char* filename);
char* preprocessSource(const char* source);
char* preprocessString(const char* input, const char* filename, int line);

// Include path management
void addIncludePath(const char* path);
void addSystemIncludePath(const char* path);
void clearIncludePaths(void);
char* findIncludeFile(const char* filename, int isSystem);

// Macro management
void defineMacro(const char* name, const char* replacement);
void defineFunctionMacro(const char* name, char** params, int paramCount, 
                        const char* replacement, int isVariadic);
void undefineMacro(const char* name);
Macro* findMacro(const char* name);
int isMacroDefined(const char* name);

// Predefined macros
void definePredefinedMacros(void);
void updateLineNumber(int line);
void updateFilename(const char* filename);

// C99 specific macros
void defineC99Macros(void);
void defineGNUMacros(void);

// Macro expansion
char* expandMacro(Macro* macro, char** args, int argCount);
char* expandMacros(const char* input);
int needsExpansion(const char* input);

// Conditional compilation
int evaluatePreprocessorExpression(const char* expr);
void pushCondition(int condition);
void popCondition(void);
int shouldSkip(void);

// Directive processing
PreprocessorDirective parseDirective(const char* line);
void processInclude(const char* args);
void processDefine(const char* args);
void processUndef(const char* args);
void processIf(const char* args);
void processIfdef(const char* args);
void processIfndef(const char* args);
void processElif(const char* args);
void processElse(void);
void processEndif(void);
void processLine(const char* args);
void processError(const char* args);
void processWarning(const char* args);
void processPragma(const char* args);

// String processing
char* processTrigraphs(const char* input);
char* removeComments(const char* input);
char* joinLines(const char* input);
char* normalizeWhitespace(const char* input);

// Token pasting and stringification
char* pasteTokens(const char* left, const char* right);
char* stringifyArgument(const char* arg);

// Error handling
void preprocessorError(const char* format, ...);
void preprocessorWarning(const char* format, ...);
int hasPreprocessorErrors(void);

// Line mapping for debugging
void recordSourceLocation(int line, const char* filename, int originalLine, const char* originalFile);
SourceLocation* getSourceLocation(int line);
void printLineMappings(void);

// Utility functions
char* readFile(const char* filename);
void writeFile(const char* filename, const char* content);
char* duplicateString(const char* str);
char** splitArguments(const char* args, int* count);
void freeArguments(char** args, int count);

// Expression evaluation for #if
long long evaluateConstantExpression(const char* expr);
int isValidIdentifier(const char* name);
char* skipWhitespace(const char* str);
char* skipToEndOfLine(const char* str);

// C99 specific features
void processVariadicMacro(const char* name, char** params, int paramCount, const char* replacement);
char* expandVariadicArgs(const char* replacement, char** args, int argCount, int variadicStart);

// Pragma handling
void processPragmaOnce(void);
void processPragmaPack(const char* args);
void processPragmaComment(const char* args);
void processGCCPragma(const char* args);

// Advanced features
void enableTrigraphs(int enable);
void enableC99Extensions(int enable);
void enableGNUExtensions(int enable);
void setKeepComments(int keep);
void setMaxIncludeDepth(int depth);

#endif // PREPROCESSOR_H
