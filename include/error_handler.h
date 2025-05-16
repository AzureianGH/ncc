#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <stdarg.h>

// Error types
typedef enum {
    ERROR_SYNTAX,       // Syntax errors
    ERROR_SEMANTIC,     // Semantic errors
    ERROR_CODEGEN,      // Code generation errors
    ERROR_INTERNAL,     // Internal compiler errors
    WARNING             // Warnings
} ErrorType;

// Print an error in GCC style
void print_error(ErrorType type, const char* filename, int line, int column, 
                 const char* format, ...);

// Show the source line with error marker
void show_error_location(const char* source, int line, int column, int length);

// Global settings for error reporting
extern int error_count;
extern int warning_count;

#endif // ERROR_HANDLER_H
