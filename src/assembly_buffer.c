#include "assembly_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// The assembly buffer
static char* assemblyBuffer = NULL;
static size_t bufferSize = 0;
static size_t bufferCapacity = 0;

#define INITIAL_BUFFER_SIZE 4096
#define BUFFER_GROWTH_FACTOR 2

// Initialize the assembly buffer
void initAssemblyBuffer() {
    bufferSize = 0;
    bufferCapacity = INITIAL_BUFFER_SIZE;
    assemblyBuffer = (char*)malloc(bufferCapacity);
    if (!assemblyBuffer) {
        fprintf(stderr, "Error: Failed to allocate assembly buffer\n");
        exit(1);
    }
    assemblyBuffer[0] = '\0'; // Empty string
}

// Ensure the buffer has enough capacity for additional content
static void ensureBufferCapacity(size_t additionalSize) {
    if (bufferSize + additionalSize + 1 > bufferCapacity) {
        // Grow the buffer
        size_t newCapacity = bufferCapacity * BUFFER_GROWTH_FACTOR;
        while (newCapacity < bufferSize + additionalSize + 1) {
            newCapacity *= BUFFER_GROWTH_FACTOR;
        }
        
        char* newBuffer = (char*)realloc(assemblyBuffer, newCapacity);
        if (!newBuffer) {
            fprintf(stderr, "Error: Failed to resize assembly buffer\n");
            exit(1);
        }
        
        assemblyBuffer = newBuffer;
        bufferCapacity = newCapacity;
    }
}

// Write to the assembly buffer
void writeToBuffer(const char* format, ...) {
    if (!assemblyBuffer) {
        initAssemblyBuffer();
    }
    
    va_list args;
    va_start(args, format);
    
    // Calculate the required size for this write
    va_list args_copy;
    va_copy(args_copy, args);
    int requiredSize = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    
    if (requiredSize < 0) {
        fprintf(stderr, "Error: vsnprintf encoding error\n");
        va_end(args);
        return;
    }
    
    // Ensure we have enough space
    ensureBufferCapacity(requiredSize);
    
    // Write to the buffer
    int written = vsnprintf(assemblyBuffer + bufferSize, bufferCapacity - bufferSize, format, args);
    va_end(args);
    
    if (written < 0) {
        fprintf(stderr, "Error: Failed to write to assembly buffer\n");
        return;
    }
    
    bufferSize += written;
}

// Get the current buffer contents as a string (caller must not free)
char* getBufferContents() {
    return assemblyBuffer ? assemblyBuffer : "";
}

// Free the assembly buffer
void freeAssemblyBuffer() {
    if (assemblyBuffer) {
        free(assemblyBuffer);
        assemblyBuffer = NULL;
        bufferSize = 0;
        bufferCapacity = 0;
    }
}
