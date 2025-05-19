#include "codegen.h"
#include "ast.h"
#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Global variable tracking
static int globalCount = 0;
static ASTNode** globalDeclarations = NULL;
extern int globalMarkerFound; // Made non-static for access from codegen.c
static int redefineGlobalStartIndex = 0; // Track where redefined globals start

// Helper function to get sanitized filename prefix
static char* getSanitizedFilenamePrefix() {
    const char* filename = getCurrentSourceFilename();
    char* prefix = (char*)malloc(strlen(filename) + 1);
    if (!prefix) return NULL;
    
    strcpy(prefix, filename);
    
    // Remove extension
    char* dot = strrchr(prefix, '.');
    if (dot) *dot = '\0';
    
    // Replace invalid characters with underscore
    for (char* c = prefix; *c; c++) {
        if (!isalnum(*c) && *c != '_') {
            *c = '_';
        }
    }
    
    return prefix;
}

// Add a global declaration to be generated later
void addGlobalDeclaration(ASTNode* node) {
    if (!node || node->type != NODE_DECLARATION) return;
    
    // Resize the array if needed
    if (globalCount == 0) {
        globalDeclarations = malloc(sizeof(ASTNode*));
    } else {
        globalDeclarations = realloc(globalDeclarations, (globalCount + 1) * sizeof(ASTNode*));
    }
    
    if (!globalDeclarations) {
        fprintf(stderr, "Error: Memory allocation failed for global declarations\n");
        exit(1);
    }
    
    // Add to the globals list
    globalDeclarations[globalCount++] = node;
}

// Check if the globals marker was found
int isGlobalMarkerFound() {
    return globalMarkerFound;
}

// Mark the current index as the starting point for redefined globals
void markRedefineGlobalsStart() {
    redefineGlobalStartIndex = globalCount;
}

// Mark that the global variables were generated
void setGlobalMarkerFound(int found) {
    globalMarkerFound = found;
}

// Generate all collected global variables
void generateGlobalsAtMarker(FILE* asmFile) {
    // Get access to the redefine flag from codegen.c
    extern int redefineLocalsFound;
    
    // Skip if globals were already generated and we're not redefining locations
    if ((globalMarkerFound && !redefineLocalsFound) || globalCount == 0) return;
    
    // Mark that globals have been generated
    globalMarkerFound = 1;
      
    fprintf(asmFile, "; Global variables placed at _NCC_GLOBAL_LOC%s\n", 
            redefineLocalsFound ? " (redefined)" : "");
    
    // Get sanitized filename prefix
    char* prefix = getSanitizedFilenamePrefix();
    if (!prefix) prefix = strdup("unknown");
    
    // Determine starting index based on whether we're redefining
    int startIdx = redefineLocalsFound ? redefineGlobalStartIndex : 0;
    
    for (int i = startIdx; i < globalCount; i++) {
        ASTNode* node = globalDeclarations[i];
        
        if (node && node->type == NODE_DECLARATION) {
            // Skip arrays - they have their own dedicated section
            if (node->declaration.type_info.is_array) continue;
            
            // Check if this is a static global variable
            if (node->declaration.type_info.is_static) {
                fprintf(asmFile, "; Static global variable (file scope): %s\n", node->declaration.var_name);
            } else {
                fprintf(asmFile, "; Global variable (program scope): %s\n", node->declaration.var_name);
            }
            
            // All globals already use filename prefix for uniqueness
            // For static globals, this is required for file-local linkage
            // For non-static globals, this helps avoid name collisions
            fprintf(asmFile, "_%s_%s:\n", prefix, node->declaration.var_name);
            
            // Initialize global variables
            if (node->declaration.initializer && node->declaration.initializer->type == NODE_LITERAL) {
                // Literal initializer
                switch (node->declaration.initializer->literal.data_type) {
                    case TYPE_INT:
                        fprintf(asmFile, "    dw %d ; Integer value\n\n", 
                            node->declaration.initializer->literal.int_value);
                        break;
                    case TYPE_CHAR:
                        fprintf(asmFile, "    db '%c' ; Character value\n\n", 
                            node->declaration.initializer->literal.char_value);
                        break;
                    case TYPE_BOOL:
                        fprintf(asmFile, "    db %d ; Boolean value (%s)\n\n", 
                            node->declaration.initializer->literal.int_value, 
                            node->declaration.initializer->literal.int_value ? "true" : "false");
                        break;
                    case TYPE_FAR_POINTER:
                        // Far pointer is stored as offset (low word) followed by segment (high word)
                        fprintf(asmFile, "    dw %d ; Offset\n", 
                            node->declaration.initializer->literal.offset);
                        fprintf(asmFile, "    dw %d ; Segment\n\n", 
                            node->declaration.initializer->literal.segment);
                        break;
                    default:
                        fprintf(asmFile, "    dw 0 ; Default zero initialization\n\n");
                }
            } else {                // No initializer - use zero
                // Determine size based on type
                if (node->declaration.type_info.type == TYPE_CHAR || 
                    node->declaration.type_info.type == TYPE_UNSIGNED_CHAR ||
                    node->declaration.type_info.type == TYPE_BOOL) {
                    // Use byte (1 byte) for char types
                    fprintf(asmFile, "    db 0 ; Zero initialization\n\n");
                } else if (node->declaration.type_info.is_far_pointer) {
                    // Far pointer is 4 bytes (2 for offset, 2 for segment)
                    fprintf(asmFile, "    dw 0 ; Offset (zero initialization)\n");
                    fprintf(asmFile, "    dw 0 ; Segment (zero initialization)\n\n");
                } else {
                    fprintf(asmFile, "    dw 0 ; Zero initialization\n\n");
                }            }
        }
    }
    
    // Free the prefix
    free(prefix);
}

// Generate any remaining globals that weren't emitted at a marker
void generateRemainingGlobals(FILE* asmFile) {
    // Skip if globals were already generated at a marker
    if (globalMarkerFound || globalCount == 0) return;
    
    fprintf(asmFile, "; Global variables (no _NCC_GLOBAL_LOC marker found)\n");
    generateGlobalsAtMarker(asmFile);
}

// Free allocated memory
void cleanupGlobals() {
    if (globalDeclarations) {
        free(globalDeclarations);
        globalDeclarations = NULL;
    }
    globalCount = 0;
    globalMarkerFound = 0;
}
