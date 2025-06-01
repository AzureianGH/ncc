#include "struct_support.h"
#include "error_manager.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global table of struct definitions
StructInfo* g_struct_table[MAX_STRUCT_DEFS];
int g_struct_count = 0;

// Function to add a struct definition to the global table
void addStructDefinition(StructInfo* structInfo) {
    if (g_struct_count >= MAX_STRUCT_DEFS) {
        reportError(-1, "Maximum number of struct definitions (%d) exceeded", MAX_STRUCT_DEFS);
        exit(1);
    }
    
    // Check for duplicate struct name
    for (int i = 0; i < g_struct_count; i++) {
        if (strcmp(g_struct_table[i]->name, structInfo->name) == 0) {
            reportError(-1, "Duplicate struct definition for '%s'", structInfo->name);
            exit(1);
        }
    }
    
    g_struct_table[g_struct_count++] = structInfo;
}

// Function to find a struct definition by name
StructInfo* findStructDefinition(const char* name) {
    for (int i = 0; i < g_struct_count; i++) {
        if (strcmp(g_struct_table[i]->name, name) == 0) {
            return g_struct_table[i];
        }
    }
    return NULL;
}

// Function to create a struct member
StructMember* createStructMember(const char* name, TypeInfo typeInfo, int offset) {
    StructMember* member = (StructMember*)malloc(sizeof(StructMember));
    if (!member) {
        reportError(-1, "Memory allocation failed for struct member");
        exit(1);
    }
    
    member->name = strdupc(name);
    member->type_info = typeInfo;
    member->offset = offset;
    member->next = NULL;
    
    return member;
}

// Function to compute the size and member offsets of a struct
void layoutStruct(StructInfo* structInfo) {
    int currentOffset = 0;
    StructMember* current = structInfo->members;
    
    while (current) {
        // Set the member's offset
        current->offset = currentOffset;
        
        // Compute size of this member
        int memberSize;
        if (current->type_info.type == TYPE_STRUCT && current->type_info.struct_info) {
            // For nested structs, use the struct's size
            memberSize = current->type_info.struct_info->size;
        } else if (current->type_info.is_array) {
            // For arrays, multiply element size by array size
            int elementSize;
            switch (current->type_info.type) {
                case TYPE_CHAR:
                case TYPE_UNSIGNED_CHAR:
                case TYPE_BOOL:
                    elementSize = 1;
                    break;
                case TYPE_LONG:
                case TYPE_UNSIGNED_LONG:
                    elementSize = 4;
                    break;
                default:
                    elementSize = 2; // Default for most types (int, short, pointer)
                    break;
            }
            memberSize = elementSize * current->type_info.array_size;
        } else {
            // For regular types
            switch (current->type_info.type) {
                case TYPE_CHAR:
                case TYPE_UNSIGNED_CHAR:
                case TYPE_BOOL:
                    memberSize = 1;
                    break;
                case TYPE_INT:
                case TYPE_SHORT:
                case TYPE_UNSIGNED_INT:
                case TYPE_UNSIGNED_SHORT:
                    memberSize = 2;
                    break;
                case TYPE_LONG:
                case TYPE_UNSIGNED_LONG:
                    memberSize = 4;
                    break;
                default:
                    // For pointers
                    if (current->type_info.is_pointer) {
                        memberSize = current->type_info.is_far_pointer ? 4 : 2;
                    } else {
                        memberSize = 2; // Default for unknown types
                    }
                    break;
            }
        }
        
        // Move to the next offset
        currentOffset += memberSize;
        current = current->next;
    }
    
    // Store the total size of the struct
    structInfo->size = currentOffset;
}

// Function to get member offset within a struct
int getMemberOffset(StructInfo* structInfo, const char* memberName) {
    if (!structInfo) return -1;
    
    StructMember* current = structInfo->members;
    while (current) {
        if (strcmp(current->name, memberName) == 0) {
            return current->offset;
        }
        current = current->next;
    }
    
    return -1; // Member not found
}

// Function to get member type within a struct
TypeInfo* getMemberType(StructInfo* structInfo, const char* memberName) {
    if (!structInfo) return NULL;
    
    StructMember* current = structInfo->members;
    while (current) {
        if (strcmp(current->name, memberName) == 0) {
            return &current->type_info;
        }
        current = current->next;
    }
    
    return NULL; // Member not found
}
