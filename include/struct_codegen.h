#ifndef STRUCT_CODEGEN_H
#define STRUCT_CODEGEN_H

#include "ast.h"
#include "struct_support.h"

// Generate code to load the address of an expression into AX
void generateAddressOf(ASTNode* expr);

// Generate code to get the size of a struct
void generateStructSizeOf(StructInfo* structInfo);

#endif // STRUCT_CODEGEN_H
