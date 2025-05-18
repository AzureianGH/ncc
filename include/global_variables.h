#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

#include "ast.h"
#include <stdio.h>

// Add a global declaration to be generated later
void addGlobalDeclaration(ASTNode* node);

// Generate all collected global variables at the marker
void generateGlobalsAtMarker(FILE* asmFile);

// Generate any remaining globals that weren't emitted at a marker
void generateRemainingGlobals(FILE* asmFile);

// Check if the globals marker was found
int isGlobalMarkerFound();

// Mark that the global variables were generated
void setGlobalMarkerFound(int found);

// Free allocated memory
void cleanupGlobals();

#endif // GLOBAL_VARIABLES_H
