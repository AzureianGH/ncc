#ifndef ERROR_MANAGER_H
#define ERROR_MANAGER_H

#include <stdarg.h>

// Error severity levels
typedef enum {
    ERROR_NOTE,
    ERROR_WARNING,
    ERROR_ERROR,
    ERROR_FATAL
} ErrorSeverity;

// Error categories
typedef enum {
    ERROR_LEXER,
    ERROR_PARSER,
    ERROR_SEMANTIC,
    ERROR_CODEGEN,
    ERROR_LINKER,
    ERROR_INTERNAL
} ErrorCategory;

// Error structure
typedef struct Error {
    ErrorSeverity severity;
    ErrorCategory category;
    int line;
    int column;
    char* filename;
    char* message;
    char* source_line;
    struct Error* next;
} Error;

// Error manager state
typedef struct ErrorManager {
    Error* errors;
    Error* lastError;
    int errorCount;
    int warningCount;
    int maxErrors;
    int stopOnFirstError;
    int suppressWarnings;
    int warningsAsErrors;
    int quietMode;
    const char* sourceCode;
    const char* currentFile;
} ErrorManager;

// Function declarations
void initErrorManager(const char* filename, const char* sourceCode, int quietMode);
void cleanupErrorManager(void);

// Error reporting
void reportError(ErrorSeverity severity, ErrorCategory category, 
                int line, int column, const char* format, ...);
void reportErrorV(ErrorSeverity severity, ErrorCategory category,
                 int line, int column, const char* format, va_list args);

// Convenience functions
void lexerError(int line, int column, const char* format, ...);
void parserError(int line, int column, const char* format, ...);
void semanticError(int line, int column, const char* format, ...);
void codegenError(int line, int column, const char* format, ...);

void lexerWarning(int line, int column, const char* format, ...);
void parserWarning(int line, int column, const char* format, ...);
void semanticWarning(int line, int column, const char* format, ...);
void codegenWarning(int line, int column, const char* format, ...);

void internalError(const char* format, ...);
void fatalError(const char* format, ...);

// Error query functions
int hasErrors(void);
int hasWarnings(void);
int getErrorCount(void);
int getWarningCount(void);
Error* getErrors(void);

// Error display
void printError(Error* error);
void printAllErrors(void);
void printErrorsSummary(void);

// Error settings
void setMaxErrors(int maxErrors);
void setStopOnFirstError(int enable);
void setSuppressWarnings(int suppress);
void setWarningsAsErrors(int enable);
void setQuietMode(int quiet);

// Source context
void setCurrentFile(const char* filename);
const char* getCurrentFile(void);
char* getSourceLine(int lineNumber);
void showSourceContext(int line, int column);

// Error utilities
const char* severityToString(ErrorSeverity severity);
const char* categoryToString(ErrorCategory category);
void freeError(Error* error);

#endif // ERROR_MANAGER_H
