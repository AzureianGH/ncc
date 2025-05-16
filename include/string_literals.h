#ifndef STRING_LITERALS_H
#define STRING_LITERALS_H

// Add a string literal to the string table and return its index
int addStringLiteral(const char* str);

// Generate data section with string literals
void generateStringLiteralsSection();

#endif // STRING_LITERALS_H
