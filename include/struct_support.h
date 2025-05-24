#ifndef STRUCT_SUPPORT_H
#define STRUCT_SUPPORT_H

#include "ast.h"

// Maximum number of struct definitions in a program
#define MAX_STRUCT_DEFS 100

// Global table of struct definitions
extern StructInfo* g_struct_table[MAX_STRUCT_DEFS];
extern int g_struct_count;

// Function to add a struct definition to the global table
void addStructDefinition(StructInfo* structInfo);

// Function to find a struct definition by name
StructInfo* findStructDefinition(const char* name);

// Function to create a struct member
StructMember* createStructMember(const char* name, TypeInfo typeInfo, int offset);

// Function to compute the size and member offsets of a struct
void layoutStruct(StructInfo* structInfo);

// Function to get member offset within a struct
int getMemberOffset(StructInfo* structInfo, const char* memberName);

// Function to get member type within a struct
TypeInfo* getMemberType(StructInfo* structInfo, const char* memberName);

#endif // STRUCT_SUPPORT_H
