#ifndef ASSEMBLY_BUFFER_H
#define ASSEMBLY_BUFFER_H

// Initialize the assembly buffer
void initAssemblyBuffer();

// Write to the assembly buffer
void writeToBuffer(const char* format, ...);

// Get the current buffer contents as a string
char* getBufferContents();

// Free the assembly buffer
void freeAssemblyBuffer();

#endif // ASSEMBLY_BUFFER_H
