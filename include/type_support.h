#ifndef TYPE_SUPPORT_H
#define TYPE_SUPPORT_H

#include "ast.h"

// Extended version of getTypeSize (uses the one from ast.c internally)
int getTypeSizeEx(DataType type);

// Generate code for type conversion
void generateTypeConversion(DataType fromType, DataType toType);

// Check if a type is an unsigned type
int isUnsignedType(DataType type);

// Generate appropriate division instruction based on type
void generateDivision(ASTNode* left, ASTNode* right, int isMod);

#endif // TYPE_SUPPORT_H
