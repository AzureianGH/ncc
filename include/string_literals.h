#ifndef STRING_LITERALS_H
#define STRING_LITERALS_H

#include "ast.h"  // For DataType enum

// Add a string literal to the string table and return its index
int addStringLiteral(const char* str);

// Add an array declaration to track it for generation later
int addArrayDeclaration(const char* name, int size, DataType type);

// Generate data section with string literals
void generateStringLiteralsSection();

// Generate string literals at specific location (after marker function)
void generateStringsAtMarker();

// Generate array declarations at specific location (after marker function)
void generateArraysAtMarker();

#endif // STRING_LITERALS_H
