#ifndef ERROR_MANAGER_H
#define ERROR_MANAGER_H

// Initialize the error manager
void initErrorManager(const char* filename, char* source, int quiet);

// Get the current source filename (without path)
const char* getCurrentSourceFilename();

// Report an error with position information
void reportError(int position, const char* format, ...);

// Report a warning with position information
void reportWarning(int position, const char* format, ...);

// Report a note (additional information)
void reportNote(int position, const char* format, ...);

// Get the number of errors
int getErrorCount();

// Get the number of warnings
int getWarningCount();

// Set the maximum number of errors before giving up
void setMaxErrors(int max);

#endif // ERROR_MANAGER_H
