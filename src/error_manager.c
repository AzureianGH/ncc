#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global error manager state
static ErrorManager errorManager;

void initErrorManager(const char* filename, const char* sourceCode, int quietMode) {
    memset(&errorManager, 0, sizeof(ErrorManager));
    
    errorManager.currentFile = filename ? strdup(filename) : NULL;
    errorManager.sourceCode = sourceCode;
    errorManager.quietMode = quietMode;
    errorManager.maxErrors = 100; // Default max errors
    errorManager.stopOnFirstError = 1; // Default: stop after first error
    errorManager.suppressWarnings = 0;
    errorManager.warningsAsErrors = 0;
    errorManager.errors = NULL;
    errorManager.lastError = NULL;
    errorManager.errorCount = 0;
    errorManager.warningCount = 0;
}

void cleanupErrorManager(void) {
    Error* current = errorManager.errors;
    while (current) {
        Error* next = current->next;
        freeError(current);
        current = next;
    }
    
    if (errorManager.currentFile) {
        free((char*)errorManager.currentFile);
    }
    
    memset(&errorManager, 0, sizeof(ErrorManager));
}

void reportError(ErrorSeverity severity, ErrorCategory category, 
                int line, int column, const char* format, ...) {
    va_list args;
    va_start(args, format);
    reportErrorV(severity, category, line, column, format, args);
    va_end(args);
}

void reportErrorV(ErrorSeverity severity, ErrorCategory category,
                 int line, int column, const char* format, va_list args) {
    // Check if we should suppress warnings
    if (severity == ERROR_WARNING && errorManager.suppressWarnings) {
        return;
    }
    
    // Convert warnings to errors if requested
    if (severity == ERROR_WARNING && errorManager.warningsAsErrors) {
        severity = ERROR_ERROR;
    }
    
    // Check error limit
    if (severity >= ERROR_ERROR && errorManager.errorCount >= errorManager.maxErrors) {
        return;
    }
    
    // Create error structure
    Error* error = malloc(sizeof(Error));
    if (!error) {
        fprintf(stderr, "Fatal: Out of memory reporting error\n");
        exit(1);
    }
    
    error->severity = severity;
    error->category = category;
    error->line = line;
    error->column = column;
    error->filename = errorManager.currentFile ? strdup(errorManager.currentFile) : NULL;
    error->next = NULL;
    
    // Format message
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    error->message = strdup(buffer);
    
    // Get source line if available
    error->source_line = getSourceLine(line);
    
    // Add to error list
    if (!errorManager.errors) {
        errorManager.errors = error;
        errorManager.lastError = error;
    } else {
        errorManager.lastError->next = error;
        errorManager.lastError = error;
    }
    
    // Update counts
    if (severity >= ERROR_ERROR) {
        errorManager.errorCount++;
    } else if (severity == ERROR_WARNING) {
        errorManager.warningCount++;
    }
    
    // Print error immediately if not in quiet mode
    if (!errorManager.quietMode) {
        printError(error);
    }
    
    // Exit on fatal errors or stop-on-first-error
    if (severity == ERROR_FATAL) {
        fprintf(stderr, "Fatal error encountered, exiting.\n");
        exit(1);
    }
    if (severity >= ERROR_ERROR && errorManager.stopOnFirstError) {
        // Print a short summary and exit immediately
        fprintf(stderr, "Compilation terminated after first error.\n");
        exit(1);
    }
}

// Convenience functions
void parserError(int line, int column, const char* format, ...) {
    va_list args;
    va_start(args, format);
    reportErrorV(ERROR_ERROR, ERROR_PARSER, line, column, format, args);
    va_end(args);
}

void semanticError(int line, int column, const char* format, ...) {
    va_list args;
    va_start(args, format);
    reportErrorV(ERROR_ERROR, ERROR_SEMANTIC, line, column, format, args);
    va_end(args);
}

void codegenError(int line, int column, const char* format, ...) {
    va_list args;
    va_start(args, format);
    reportErrorV(ERROR_ERROR, ERROR_CODEGEN, line, column, format, args);
    va_end(args);
}

void parserWarning(int line, int column, const char* format, ...) {
    va_list args;
    va_start(args, format);
    reportErrorV(ERROR_WARNING, ERROR_PARSER, line, column, format, args);
    va_end(args);
}

void semanticWarning(int line, int column, const char* format, ...) {
    va_list args;
    va_start(args, format);
    reportErrorV(ERROR_WARNING, ERROR_SEMANTIC, line, column, format, args);
    va_end(args);
}

void codegenWarning(int line, int column, const char* format, ...) {
    va_list args;
    va_start(args, format);
    reportErrorV(ERROR_WARNING, ERROR_CODEGEN, line, column, format, args);
    va_end(args);
}

void internalError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    reportErrorV(ERROR_FATAL, ERROR_INTERNAL, 0, 0, format, args);
    va_end(args);
}

void fatalError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    reportErrorV(ERROR_FATAL, ERROR_INTERNAL, 0, 0, format, args);
    va_end(args);
}

// Error query functions
int hasErrors(void) {
    return errorManager.errorCount > 0;
}

int hasWarnings(void) {
    return errorManager.warningCount > 0;
}

int getErrorCount(void) {
    return errorManager.errorCount;
}

int getWarningCount(void) {
    return errorManager.warningCount;
}

Error* getErrors(void) {
    return errorManager.errors;
}

// Error display
void printError(Error* error) {
    if (!error) return;
    
    // Print error location
    if (error->filename) {
        fprintf(stderr, "%s:", error->filename);
    }
    if (error->line > 0) {
        fprintf(stderr, "%d:", error->line);
    }
    if (error->column > 0) {
        fprintf(stderr, "%d:", error->column);
    }
    
    // Print severity and category
    fprintf(stderr, " %s: ", severityToString(error->severity));
    if (error->category != ERROR_INTERNAL) {
        fprintf(stderr, "[%s] ", categoryToString(error->category));
    }
    
    // Print message
    fprintf(stderr, "%s\n", error->message);
    
    // Print source context if available
    if (error->source_line && error->line > 0 && error->column > 0) {
        fprintf(stderr, "  %s\n", error->source_line);
        
        // Print caret pointing to error location
        fprintf(stderr, "  ");
        for (int i = 1; i < error->column; i++) {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "^\n");
    }
}

void printAllErrors(void) {
    Error* current = errorManager.errors;
    while (current) {
        printError(current);
        current = current->next;
    }
}

void printErrorsSummary(void) {
    if (errorManager.errorCount > 0) {
        fprintf(stderr, "%d error(s) generated.\n", errorManager.errorCount);
    }
    if (errorManager.warningCount > 0) {
        fprintf(stderr, "%d warning(s) generated.\n", errorManager.warningCount);
    }
}

// Error settings
void setMaxErrors(int maxErrors) {
    errorManager.maxErrors = maxErrors;
}

void setStopOnFirstError(int enable) {
    errorManager.stopOnFirstError = enable ? 1 : 0;
}

void setSuppressWarnings(int suppress) {
    errorManager.suppressWarnings = suppress;
}

void setWarningsAsErrors(int enable) {
    errorManager.warningsAsErrors = enable;
}

void setQuietMode(int quiet) {
    errorManager.quietMode = quiet;
}

// Source context
void setCurrentFile(const char* filename) {
    if (errorManager.currentFile) {
        free((char*)errorManager.currentFile);
    }
    errorManager.currentFile = filename ? strdup(filename) : NULL;
}

const char* getCurrentFile(void) {
    return errorManager.currentFile;
}

char* getSourceLine(int lineNumber) {
    if (!errorManager.sourceCode || lineNumber <= 0) {
        return NULL;
    }
    
    const char* source = errorManager.sourceCode;
    int currentLine = 1;
    const char* lineStart = source;
    
    // Find the start of the requested line
    while (*source && currentLine < lineNumber) {
        if (*source == '\n') {
            currentLine++;
            lineStart = source + 1;
        }
        source++;
    }
    
    if (currentLine != lineNumber) {
        return NULL; // Line not found
    }
    
    // Find the end of the line
    const char* lineEnd = lineStart;
    while (*lineEnd && *lineEnd != '\n') {
        lineEnd++;
    }
    
    // Copy the line
    int length = (int)(lineEnd - lineStart);
    char* line = malloc(length + 1);
    if (line) {
        memcpy(line, lineStart, length);
        line[length] = '\0';
    }
    
    return line;
}

void showSourceContext(int line, int column) {
    char* sourceLine = getSourceLine(line);
    if (sourceLine) {
        fprintf(stderr, "  %s\n", sourceLine);
        
        if (column > 0) {
            fprintf(stderr, "  ");
            for (int i = 1; i < column; i++) {
                fprintf(stderr, " ");
            }
            fprintf(stderr, "^\n");
        }
        
        free(sourceLine);
    }
}

// Error utilities
const char* severityToString(ErrorSeverity severity) {
    switch (severity) {
        case ERROR_NOTE: return "note";
        case ERROR_WARNING: return "warning";
        case ERROR_ERROR: return "error";
        case ERROR_FATAL: return "fatal error";
        default: return "unknown";
    }
}

const char* categoryToString(ErrorCategory category) {
    switch (category) {
        case ERROR_LEXER: return "lexer";
        case ERROR_PARSER: return "parser";
        case ERROR_SEMANTIC: return "semantic";
        case ERROR_CODEGEN: return "codegen";
        case ERROR_LINKER: return "linker";
        case ERROR_INTERNAL: return "internal";
        default: return "unknown";
    }
}

void freeError(Error* error) {
    if (!error) return;
    
    if (error->filename) {
        free(error->filename);
    }
    if (error->message) {
        free(error->message);
    }
    if (error->source_line) {
        free(error->source_line);
    }
    
    free(error);
}
