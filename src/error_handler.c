#include "error_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global counters for errors and warnings
int error_count = 0;
int warning_count = 0;

// Get the error type as a string
static const char* get_error_type_str(ErrorType type) {
    switch (type) {
        case ERROR_SYNTAX:
            return "syntax error";
        case ERROR_SEMANTIC:
            return "semantic error";
        case ERROR_CODEGEN:
            return "code generation error";
        case ERROR_INTERNAL:
            return "internal error";
        case WARNING:
            return "warning";
        default:
            return "error";
    }
}

// Print an error in GCC style
void print_error(ErrorType type, const char* filename, int line, int column, 
                 const char* format, ...) {
    va_list args;
    
    // Increment appropriate counter
    if (type == WARNING) {
        warning_count++;
    } else {
        error_count++;
    }
      // Print filename, line and column
    fprintf(stderr, "%s:%d:%d: ", filename, line, column);
    
    // Use colors depending on error type (GCC style)
    if (type == WARNING) {
        fprintf(stderr, "\033[1;35mwarning:\033[0m "); // Magenta for warnings
    } else {
        fprintf(stderr, "\033[1;31merror:\033[0m ");   // Red for errors
    }
    
    // Print the error message
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

// Helper function to get a specific line from source
static char* get_source_line(const char* source, int line_number) {
    char* line = malloc(1024);
    if (!line) return NULL;
    
    const char* ptr = source;
    int current_line = 1;
    
    // Find the start of the requested line
    while (*ptr && current_line < line_number) {
        if (*ptr == '\n') {
            current_line++;
        }
        ptr++;
    }
    
    // Copy the line
    int i = 0;
    while (*ptr && *ptr != '\n' && i < 1023) {
        line[i++] = *ptr++;
    }
    line[i] = '\0';
    
    return line;
}

// Show the source line with error marker
void show_error_location(const char* source, int line, int column, int length) {
    char* source_line = get_source_line(source, line);
    if (!source_line) return;
    
    // Print the source line
    fprintf(stderr, "        %s\n", source_line);
    
    // Print the error indicator
    fprintf(stderr, "        ");
    for (int i = 1; i < column; i++) {
        fprintf(stderr, " ");
    }
    
    // Print the carets (^) for the error location
    for (int i = 0; i < (length > 0 ? length : 1); i++) {
        fprintf(stderr, "^");
    }
    fprintf(stderr, "\n");
    
    free(source_line);
}
