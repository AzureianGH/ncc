#include "codegen.h"
#include <stdio.h>

// Set the optimization level based on the provided level flag
void setOptimizationLevel(int level, int debugMode) {
    optimizationState.level = level;
    
    // Configure specific optimizations based on level
    switch (level) {
        case OPT_LEVEL_NONE:
            // No optimizations
            optimizationState.mergeStrings = 0;
            break;
            
        case OPT_LEVEL_BASIC:
            // Basic optimizations
            optimizationState.mergeStrings = 1;
            break;
            
        default:
            // Unknown level, use no optimizations
            optimizationState.level = OPT_LEVEL_NONE;
            optimizationState.mergeStrings = 0;
            break;
    }
    
    // Print info about optimization if not in quiet mode
    #ifndef QUIET_MODE
    if (debugMode) printf("Compiler optimization level: O%d\n", optimizationState.level);
    if (optimizationState.mergeStrings) {
        printf("  - String merging: enabled\n");
    }
    #endif
}
