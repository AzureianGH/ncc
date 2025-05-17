#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

// Global variables for error management
static const char* sourceFilename = NULL;
static char* sourceBuffer = NULL;
static int errorCount = 0;
static int warningCount = 0;
static int maxErrors = 20;
static int quietMode = 0;

// ANSI color codes for terminal output
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_RESET   "\033[0m"

// Initialize the error manager
void initErrorManager(const char* filename, char* source, int quiet) {
    sourceFilename = filename;
    sourceBuffer = source;
    errorCount = 0;
    warningCount = 0;
    quietMode = quiet;
}

// Find the line start position given a position in the buffer
static const char* findLineStart(const char* buffer, int position) {
    const char* lineStart = buffer;
    
    // Find the start of the line
    for (int i = 0; i < position; i++) {
        if (buffer[i] == '\n') {
            lineStart = &buffer[i + 1];
        }
    }
    
    return lineStart;
}

// Find the line end position given a position in the buffer
static const char* findLineEnd(const char* buffer, int position) {
    const char* lineEnd = strchr(&buffer[position], '\n');
    if (!lineEnd) {
        lineEnd = buffer + strlen(buffer);
    }
    return lineEnd;
}

// Count lines up to a position
static int countLines(const char* buffer, int position) {
    int lines = 1;
    for (int i = 0; i < position; i++) {
        if (buffer[i] == '\n') {
            lines++;
        }
    }
    return lines;
}

// Get column position in the current line
static int getColumn(const char* buffer, int position) {
    const char* lineStart = findLineStart(buffer, position);
    return (int)(&buffer[position] - lineStart) + 1;
}

// Print a snippet of code with an error indicator
static void printCodeSnippet(const char* buffer, int position) {
    if (!buffer || !sourceBuffer) return;
    
    const char* lineStart = findLineStart(buffer, position);
    const char* lineEnd = findLineEnd(buffer, position);
    int lineLength = (int)(lineEnd - lineStart);
    int column = (int)(&buffer[position] - lineStart);
    
    // Print line number and code snippet
    int lineNum = countLines(sourceBuffer, position);
    fprintf(stderr, " %4d | ", lineNum);
    
    // Print the line content
    fwrite(lineStart, 1, lineLength, stderr);
    fprintf(stderr, "\n");
    
    // Print the error indicator
    fprintf(stderr, "      | ");
    for (int i = 0; i < column; i++) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "^~~~\n");
}

// Report an error
void reportError(int position, const char* format, ...) {
    if (errorCount >= maxErrors) {
        return;
    }

    errorCount++;
    
    fprintf(stderr, "%serror:%s ", COLOR_RED, COLOR_RESET);
    
    if (sourceFilename) {
        int line = countLines(sourceBuffer, position);
        int col = getColumn(sourceBuffer, position);
        fprintf(stderr, "%s:%d:%d: ", sourceFilename, line, col);
    }
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    
    // Print code snippet if source is available
    if (sourceBuffer) {
        printCodeSnippet(sourceBuffer, position);
    }
    
    if (errorCount >= maxErrors) {
        fprintf(stderr, "Too many errors, stopping compilation.\n");
        exit(1);
    }
}

// Report a warning
void reportWarning(int position, const char* format, ...) {
    if (quietMode) return;
    
    warningCount++;
    
    fprintf(stderr, "%swarning:%s ", COLOR_YELLOW, COLOR_RESET);
    
    if (sourceFilename) {
        int line = countLines(sourceBuffer, position);
        int col = getColumn(sourceBuffer, position);
        fprintf(stderr, "%s:%d:%d: ", sourceFilename, line, col);
    }
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    
    // Print code snippet if source is available
    if (sourceBuffer) {
        printCodeSnippet(sourceBuffer, position);
    }
}

// Report a note (additional information)
void reportNote(int position, const char* format, ...) {
    if (quietMode) return;
    
    fprintf(stderr, "%snote:%s ", COLOR_BLUE, COLOR_RESET);
    
    if (sourceFilename && position >= 0) {
        int line = countLines(sourceBuffer, position);
        int col = getColumn(sourceBuffer, position);
        fprintf(stderr, "%s:%d:%d: ", sourceFilename, line, col);
    }
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    
    // Print code snippet if source is available and position is valid
    if (sourceBuffer && position >= 0) {
        printCodeSnippet(sourceBuffer, position);
    }
}

// Get the number of errors
int getErrorCount() {
    return errorCount;
}

// Get the number of warnings
int getWarningCount() {
    return warningCount;
}

// Set the maximum number of errors before giving up
void setMaxErrors(int max) {
    maxErrors = max;
}
