// Attribute parsing functions for NCC compiler
#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include "ast.h"

// Parse function attributes like __attribute__((naked))
void parseFunctionAttributes(FunctionInfo* funcInfo);

#endif // ATTRIBUTES_H
