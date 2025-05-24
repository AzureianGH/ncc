#ifndef STRUCT_PARSER_H
#define STRUCT_PARSER_H

#include "ast.h"
#include "struct_support.h"

// Parse a struct type (used in variable declarations)
TypeInfo parseStructType();

// Parse a struct definition (struct name { ... };)
ASTNode* parseStructDefinition();

#endif // STRUCT_PARSER_H
