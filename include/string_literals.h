#ifndef STRING_LITERALS_H
#define STRING_LITERALS_H

#include "ast.h"  // For DataType enum
#include <stdio.h>

// Add a string literal to the string literals table and return its index
int addStringLiteral(const char* str);

// Add an array declaration to track for generation later
int addArrayDeclaration(const char* name, int size, DataType type, const char* funcName);

// Add an array declaration with initializers
int addArrayDeclarationWithInitializers(const char* name, int size, DataType type, 
                                      const char* funcName, ASTNode* initializer, 
                                      int is_static);

// Generate string literals at the marker position
void generateStringsAtMarker();

// Generate array declarations at the marker position
void generateArraysAtMarker();

// Generate string literals and arrays at end of file if not already done
void generateStringLiteralsSection();

// Get sanitized filename prefix (for use in labels)
char* getSanitizedFilenamePrefix();

// Clean up allocated memory
void cleanupStringAndArrayTables();

#endif // STRING_LITERALS_H
