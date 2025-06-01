#include "codegen.h"
#include "ast.h"
#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Globals from codegen.c
extern FILE* asmFile;
extern int stringLiteralCount;
extern char** stringLiterals;

// New globals for array handling
extern int arrayCount;
extern char** arrayNames;
extern int* arraySizes;
extern DataType* arrayTypes;
extern char** arrayFunctions;

// Has the string/array marker been found?
extern int stringMarkerFound;
extern int arrayMarkerFound;

// Function to sanitize filename for use as identifier prefix
char* getSanitizedFilenamePrefix() {
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

// Function to escape string for assembly
char* escapeStringForAsm(const char* input) {
    if (!input) return NULL;
    
    size_t len = strlen(input);
    // Allocate memory for worst case (every char needs escaping)
    char* output = (char*)malloc(2 * len + 1);
    if (!output) return NULL;
    
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        switch (input[i]) {
            case '\\':
                // Handle escape sequences
                i++;
                if (i < len) {
                    switch (input[i]) {
                        case 'n': output[j++] = 10; break;  // Newline
                        case 'r': output[j++] = 13; break;  // Carriage return
                        case 't': output[j++] = 9; break;   // Tab
                        case '0': output[j++] = 0; break;   // Null
                        case '\\': output[j++] = '\\'; break; // Backslash
                        case '\"': output[j++] = '\"'; break; // Quote
                        default: output[j++] = input[i]; break;
                    }
                }
                break;
            default:
                output[j++] = input[i];
                break;
        }
    }
    output[j] = '\0';
    return output;
}

// Add a string literal to the string table and return its index
int addStringLiteral(const char* str) {
    if (!str) {
        fprintf(stderr, "Debug: Null string passed to addStringLiteral\n");
        return -1;
    }
    
    // Make a copy of the string to work with
    char* workStr = strdupc(str);
    if (!workStr) {
        fprintf(stderr, "Debug: Failed to allocate memory for string copy\n");
        return -1;
    }
    
    // Strip quotes if present
    size_t len = strlen(workStr);
    char* unquoted = NULL;
    
    // Check if the string has surrounding quotes
    if (len >= 2 && workStr[0] == '\"' && workStr[len-1] == '\"') {
        // Remove the surrounding quotes
        unquoted = (char*)malloc(len - 1); // -2 for quotes, +1 for null
        if (!unquoted) {
            fprintf(stderr, "Debug: Failed to allocate memory for unquoted string\n");
            free(workStr);
            return -1;
        }
        
        strncpy(unquoted, workStr + 1, len - 2);
        unquoted[len - 2] = '\0';
        free(workStr);
    } else {
        // No quotes, use as is
        unquoted = workStr;
    }
    
    // Process escape sequences
    char* escaped = escapeStringForAsm(unquoted);
    free(unquoted);
    
    if (!escaped) {
        fprintf(stderr, "Debug: Failed to process escape sequences\n");
        return -1;
    }
    
    // Check if string merging is enabled and look for identical strings
    if (optimizationState.mergeStrings) {
        for (int i = 0; i < stringLiteralCount; i++) {
            if (strcmp(stringLiterals[i], escaped) == 0) {
                // Found an identical string, reuse it
                free(escaped);
                return i;
            }
        }
    }
    
    // Add to string table
    if (stringLiterals == NULL) {
        stringLiterals = (char**)malloc(sizeof(char*));
        if (!stringLiterals) {
            fprintf(stderr, "Debug: Failed to allocate memory for string table\n");
            free(escaped);
            return -1;
        }
    } else {
        char** newTable = (char**)realloc(stringLiterals, (stringLiteralCount + 1) * sizeof(char*));
        if (!newTable) {
            fprintf(stderr, "Debug: Failed to reallocate memory for string table\n");
            free(escaped);
            return -1;
        }
        stringLiterals = newTable;
    }
    
    stringLiterals[stringLiteralCount] = escaped;
    int index = stringLiteralCount++;
    
    return index;
}

// Add an array declaration to track it for generation later, including function context
int addArrayDeclaration(const char* name, int size, DataType type, const char* funcName) {
    // Initialize arrays if first time
    if (arrayNames == NULL) {
        arrayNames = (char**)malloc(sizeof(char*));
        arraySizes = (int*)malloc(sizeof(int));
        arrayTypes = (DataType*)malloc(sizeof(DataType));
        arrayFunctions = (char**)malloc(sizeof(char*));
        if (!arrayNames || !arraySizes || !arrayTypes || !arrayFunctions) {
            fprintf(stderr, "Debug: Failed to allocate memory for array tracking\n");
            return -1;
        }
    } else {
        // Expand arrays
        char** newNames = (char**)realloc(arrayNames, (arrayCount + 1) * sizeof(char*));
        int* newSizes = (int*)realloc(arraySizes, (arrayCount + 1) * sizeof(int));
        DataType* newTypes = (DataType*)realloc(arrayTypes, (arrayCount + 1) * sizeof(DataType));
        char** newFuncs = (char**)realloc(arrayFunctions, (arrayCount + 1) * sizeof(char*));
        if (!newNames || !newSizes || !newTypes || !newFuncs) {
            fprintf(stderr, "Debug: Failed to reallocate memory for array tracking\n");
            return -1;
        }
        arrayNames = newNames;
        arraySizes = newSizes;
        arrayTypes = newTypes;
        arrayFunctions = newFuncs;
    }
    
    // Store array info
    arrayNames[arrayCount] = strdupc(name);
    if (!arrayNames[arrayCount]) {
        fprintf(stderr, "Debug: Failed to allocate memory for array name\n");
        return -1;
    }
    arrayFunctions[arrayCount] = funcName ? strdupc(funcName) : strdupc("global");
    if (!arrayFunctions[arrayCount]) {
        fprintf(stderr, "Debug: Failed to allocate memory for array function name\n");
        free(arrayNames[arrayCount]);
        return -1;
    }
    arraySizes[arrayCount] = size;
    arrayTypes[arrayCount] = type;
    
    return arrayCount++;
}

// Array initializer tracking
typedef struct {
    ASTNode* initializer;
    int is_static;
} ArrayInitializerInfo;

static ArrayInitializerInfo* arrayInitializers = NULL;

// External function for writing array initializers
extern void writeArrayWithInitializers(FILE* outFile, const char* arrayName, int arraySize,
                                    DataType arrayType, ASTNode* initializer);

// Add an array declaration with initializers
int addArrayDeclarationWithInitializers(const char* name, int size, DataType type, 
                                      const char* funcName, ASTNode* initializer, 
                                      int is_static) {
    int arrayIndex = addArrayDeclaration(name, size, type, funcName);
    
    // Create or resize the initializers array if needed
    if (arrayInitializers == NULL) {
        arrayInitializers = (ArrayInitializerInfo*)malloc(sizeof(ArrayInitializerInfo) * (arrayIndex + 1));
    } else {
        arrayInitializers = (ArrayInitializerInfo*)realloc(
            arrayInitializers, sizeof(ArrayInitializerInfo) * (arrayIndex + 1));
    }
    
    if (!arrayInitializers) {
        reportError(-1, "Memory allocation failed for array initializer info");
        return -1;
    }
    
    // Store the initializer
    arrayInitializers[arrayIndex].initializer = initializer;
    arrayInitializers[arrayIndex].is_static = is_static;
    
    return arrayIndex;
}

// Hash table for string deduplication
typedef struct StringEntry {
    char* string;
    int index;
    struct StringEntry* next;
} StringEntry;

#define STRING_HASH_SIZE 64
StringEntry* stringHashTable[STRING_HASH_SIZE] = {NULL};

// Hash function for strings
static unsigned int hashString(const char* str) {
    unsigned int hash = 0;
    while (*str) {
        hash = hash * 31 + (*str++);
    }
    return hash % STRING_HASH_SIZE;
}

// Add string to hash table or get existing index
static int getStringIndex(const char* str, int newIndex) {
    // When redefining and we're already past the redefinition point,
    // don't try to deduplicate strings - assign sequential indices
    extern int redefineLocalsFound;
    extern int redefineStringStartIndex;
    
    if (redefineLocalsFound && stringLiteralCount > redefineStringStartIndex && 
        newIndex >= redefineStringStartIndex) {
        // For new strings after redefinition, just use the provided index
        unsigned int hash = hashString(str);
        StringEntry* entry = (StringEntry*)malloc(sizeof(StringEntry));
        if (entry) {
            entry->string = strdupc(str);
            entry->index = newIndex;
            entry->next = stringHashTable[hash];
            stringHashTable[hash] = entry;
        }
        return newIndex;
    }
    
    // Normal string deduplication for original strings
    unsigned int hash = hashString(str);
    StringEntry* entry = stringHashTable[hash];
    
    // Look for existing entry
    while (entry) {
        if (strcmp(entry->string, str) == 0) {
            return entry->index;
        }
        entry = entry->next;
    }
    
    // Add new entry
    entry = (StringEntry*)malloc(sizeof(StringEntry));
    if (entry) {
        entry->string = strdupc(str);
        entry->index = newIndex;
        entry->next = stringHashTable[hash];
        stringHashTable[hash] = entry;
    }
    
    return newIndex;
}

// Hash table for string label deduplication
typedef struct StringLabelEntry {
    char* label;
    struct StringLabelEntry* next;
} StringLabelEntry;

#define STRING_LABEL_HASH_SIZE 64
StringLabelEntry* stringLabelHashTable[STRING_LABEL_HASH_SIZE] = {NULL};

// Hash function for string labels
static unsigned int hashStringLabel(const char* label) {
    unsigned int hash = 0;
    while (*label) {
        hash = hash * 31 + (*label++);
    }
    return hash % STRING_LABEL_HASH_SIZE;
}

// Check if a string label already exists and add it if not
static int stringLabelExists(const char* label) {
    unsigned int hash = hashStringLabel(label);
    StringLabelEntry* entry = stringLabelHashTable[hash];
    
    while (entry) {
        if (strcmp(entry->label, label) == 0) {
            return 1; // Found
        }
        entry = entry->next;
    }
    
    // Add new entry
    entry = (StringLabelEntry*)malloc(sizeof(StringLabelEntry));
    if (entry) {
        entry->label = strdupc(label);
        entry->next = stringLabelHashTable[hash];
        stringLabelHashTable[hash] = entry;
    }
    
    return 0; // Not found, added now
}

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

// Generate string literals at specific location (after marker function)
void generateStringsAtMarker() {
    // Get access to the redefine flag from codegen.c
    extern int redefineLocalsFound;
    extern int redefineStringStartIndex;
    
    // Skip if strings were already generated and we're not redefining locations
    if ((stringMarkerFound && !redefineLocalsFound) || stringLiteralCount == 0) return;
    
    // Mark that strings have been generated
    stringMarkerFound = 1;
    
    fprintf(asmFile, "; String literals placed at _NCC_STRING_LOC%s\n", 
            redefineLocalsFound ? " (redefined)" : "");
    
    // Get sanitized filename prefix
    char* prefix = getSanitizedFilenamePrefix();
    if (!prefix) prefix = strdupc("unknown");
    
    // First build the hash table of existing string labels if redefining
    if (redefineLocalsFound && redefineStringStartIndex > 0) {
        for (int i = 0; i < redefineStringStartIndex; i++) {
            if (stringLiterals[i]) {
                char labelName[256];
                snprintf(labelName, sizeof(labelName), "%s_string_%d", prefix, i);
                stringLabelExists(labelName);
            }
        }
    }
    
    // Get the maximum string index from before the redefinition point (if redefining)
    int maxIndexBeforeRedefine = 0;
    if (redefineLocalsFound && redefineStringStartIndex > 0) {
        // We need to assume that string indices 0 to redefineStringStartIndex-1 exist
        maxIndexBeforeRedefine = redefineStringStartIndex;
    }
      // For non-redefine case, we process all strings from the beginning
    int startIdx = 0;
    
    if (redefineLocalsFound) {
        // In redefine case, clear any existing content deduplication
        // to ensure each string gets its own index
        for (int i = 0; i < STRING_HASH_SIZE; i++) {
            StringEntry* entry = stringHashTable[i];
            while (entry) {
                StringEntry* temp = entry->next;
                free(entry->string);
                free(entry);
                entry = temp;
            }
            stringHashTable[i] = NULL;
        }
        
        // Register all string indices up to the redefinition point
        for (int i = 0; i < redefineStringStartIndex; i++) {
            if (stringLiterals[i]) {
                getStringIndex(stringLiterals[i], i);
            }
        }
        
        // For redefine case, we only process new strings
        startIdx = redefineStringStartIndex;
    }
    
    // Generate strings after the redefine marker (if applicable)
    for (int i = startIdx; i < stringLiteralCount; i++) {
        // For each string, get or create its index
        int stringIdx = i;
        
        // Create the label name based on the index
        char labelName[256];
        snprintf(labelName, sizeof(labelName), "%s_string_%d", prefix, stringIdx);
        
        // Only skip if this specific label already exists (should never happen with new strings)
        if (redefineLocalsFound && stringLabelExists(labelName)) {
            continue;
        }
        
        // Output the string
        fprintf(asmFile, "%s: db ", labelName);
        
        // Output each byte of the string as a decimal value
        size_t len = strlen(stringLiterals[i]);
        for (size_t j = 0; j < len; j++) {
            fprintf(asmFile, "%d", (unsigned char)stringLiterals[i][j]);
            if (j < len - 1) {
                fprintf(asmFile, ", ");
            }
        }
        fprintf(asmFile, ", 0  ; null terminator\n");
    }
    
    free(prefix);
    fprintf(asmFile, "; String literal location marker%s\n", redefineLocalsFound ? " (redefined)" : "");
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
// Forward declaration
extern void generateArrayWithInitializers(ASTNode* node);

// Hash table for array deduplication
typedef struct ArrayEntry {
    char* name;
    struct ArrayEntry* next;
} ArrayEntry;

#define ARRAY_HASH_SIZE 64
ArrayEntry* arrayHashTable[ARRAY_HASH_SIZE] = {NULL};

// Hash function for array names
static unsigned int hashArrayName(const char* name) {
    unsigned int hash = 0;
    while (*name) {
        hash = hash * 31 + (*name++);
    }
    return hash % ARRAY_HASH_SIZE;
}

// Check if array name exists in hash table
static int arrayExists(const char* name) {
    unsigned int hash = hashArrayName(name);
    ArrayEntry* entry = arrayHashTable[hash];
    
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            return 1; // Found
        }
        entry = entry->next;
    }
    
    // Add new entry
    entry = (ArrayEntry*)malloc(sizeof(ArrayEntry));
    if (entry) {
        entry->name = strdupc(name);
        entry->next = arrayHashTable[hash];
        arrayHashTable[hash] = entry;
    }
    
    return 0; // Not found, added to table
}

// Generate array declarations at specific location (after marker function)
void generateArraysAtMarker() {
    // Get access to the redefine flag from codegen.c
    extern int redefineLocalsFound;
    extern int redefineArrayStartIndex;
    
    // Skip if arrays were already generated and we're not redefining locations
    if ((arrayMarkerFound && !redefineLocalsFound) || arrayCount == 0) return;
      
    // Mark that arrays have been generated
    arrayMarkerFound = 1;
      
    fprintf(asmFile, "; Array declarations placed at _NCC_ARRAY_LOC%s\n",
            redefineLocalsFound ? " (redefined)" : "");
    
    // Get sanitized filename prefix
    char* prefix = getSanitizedFilenamePrefix();
    if (!prefix) prefix = strdupc("unknown");
      // First, build the hash table of existing arrays if redefining
    if (redefineLocalsFound && redefineArrayStartIndex > 0) {        for (int i = 0; i < redefineArrayStartIndex; i++) {
            char fullName[256];
            // Use file_function_arrayName_index as the label
            snprintf(fullName, sizeof(fullName), "_%s_%s_%s_%d", prefix, 
                    arrayFunctions[i] ? arrayFunctions[i] : "global", 
                    arrayNames[i], i);
            arrayExists(fullName);
        }
    }
    
    // Determine starting index based on whether we're redefining
    int startIdx = redefineLocalsFound ? redefineArrayStartIndex : 0;    // Output only new, unique arrays
    for (int i = startIdx; i < arrayCount; i++) {
        char fullName[256];
        // Label format depends on whether it's a global array or local array
        if (strcmp(arrayFunctions[i], "global") == 0) {
            // For global arrays, use file_global_name_index
            snprintf(fullName, sizeof(fullName), "_%s_global_%s_%d", prefix, 
                    arrayNames[i], i);
        } else {
            // For local arrays, use file_function_name_index
            snprintf(fullName, sizeof(fullName), "_%s_%s_%s_%d", prefix, 
                    arrayFunctions[i], arrayNames[i], i);
        }

        // Skip if array was already defined
        if (redefineLocalsFound && arrayExists(fullName)) {
            continue;
        }
        
        fprintf(asmFile, "%s: ", fullName);
        
        // Check if this array has initializers
        if (arrayInitializers && i < arrayCount && arrayInitializers[i].initializer) {
            // Generate with initializers
            writeArrayWithInitializers(asmFile, arrayNames[i], arraySizes[i], 
                                      arrayTypes[i], arrayInitializers[i].initializer);
        } else {
            // Determine element size and directive
            const char* directive;
            int elementSize;
            
            switch (arrayTypes[i]) {
                case TYPE_CHAR:
                case TYPE_UNSIGNED_CHAR:
                case TYPE_BOOL:
                    directive = "db";
                    elementSize = 1;
                    break;
                default:
                    directive = "dw";
                    elementSize = 2;
                    break;
            }
            
            // Output array declaration with zeros
            fprintf(asmFile, "times %d %s 0 ; Array of %d bytes\n", 
                    arraySizes[i], directive, arraySizes[i] * elementSize);
        }
    }
    
    free(prefix);
}

// Generate data section with string literals and arrays
void generateStringLiteralsSection() {
    // Skip if nothing to generate or everything is already generated
    if ((stringLiteralCount == 0 || stringMarkerFound) && 
        (arrayCount == 0 || arrayMarkerFound)) return;
    
    fprintf(asmFile, "\n; Data section for strings and arrays\n");
    
    // Generate string literals if not already done
    if (!stringMarkerFound && stringLiteralCount > 0) {        fprintf(asmFile, "; String literals section\n");
        
        // Get sanitized filename prefix
        char* prefix = getSanitizedFilenamePrefix();
        if (!prefix) prefix = strdupc("unknown");
        
        for (int i = 0; i < stringLiteralCount; i++) {
            fprintf(asmFile, "%s_string_%d: db ", prefix, i);
            
            // Output each byte of the string as a decimal value
            size_t len = strlen(stringLiterals[i]);
            for (size_t j = 0; j < len; j++) {
                fprintf(asmFile, "%d", (unsigned char)stringLiterals[i][j]);
                if (j < len - 1) {
                    fprintf(asmFile, ", ");
                }            }
            fprintf(asmFile, ", 0  ; null terminator\n");
        }
        
        // Free the prefix
        free(prefix);
    }
    
    // Generate array declarations if not already done
    if (!arrayMarkerFound && arrayCount > 0) {
        fprintf(asmFile, "\n; Array declarations section\n");
        
        // Get sanitized filename prefix
        char* prefix = getSanitizedFilenamePrefix();
        if (!prefix) prefix = strdupc("unknown");        
        for (int i = 0; i < arrayCount; i++) {
            char fullName[256];
            // Label format depends on whether it's a global array or local array
            if (strcmp(arrayFunctions[i], "global") == 0) {
                // For global arrays, use file_global_name_index
                snprintf(fullName, sizeof(fullName), "_%s_global_%s_%d", prefix, 
                        arrayNames[i], i);
            } else {
                // For local arrays, use file_function_name_index
                snprintf(fullName, sizeof(fullName), "_%s_%s_%s_%d", prefix, 
                        arrayFunctions[i], arrayNames[i], i);
            }
            
            fprintf(asmFile, "%s: ", fullName);
            
            // Check if this array has initializers
            if (arrayInitializers && i < arrayCount && arrayInitializers[i].initializer) {
                // Generate with initializers
                writeArrayWithInitializers(asmFile, arrayNames[i], arraySizes[i], 
                                          arrayTypes[i], arrayInitializers[i].initializer);
            } else {
                // Determine element size and directive
                const char* directive;
                int elementSize;
                
                switch (arrayTypes[i]) {
                    case TYPE_CHAR:
                    case TYPE_UNSIGNED_CHAR:
                    case TYPE_BOOL:
                        directive = "db";
                        elementSize = 1;
                        break;
                    default:
                        directive = "dw";
                        elementSize = 2;
                        break;
                }
                
                // Output array declaration with zeros
                fprintf(asmFile, "times %d %s 0 ; Array of %d bytes\n", 
                        arraySizes[i], directive, arraySizes[i] * elementSize);
            }
        }
        
        free(prefix);
    }
}

void cleanupStringAndArrayTables() {
    // Free stringLiterals
    if (stringLiterals) {
        for (int i = 0; i < stringLiteralCount; i++) {
            free(stringLiterals[i]);
        }
        free(stringLiterals);
        stringLiterals = NULL;
    }
    stringLiteralCount = 0;

    // Free array tracking
    if (arrayNames) {
        for (int i = 0; i < arrayCount; i++) {
            free(arrayNames[i]);
        }
        free(arrayNames);
        arrayNames = NULL;
    }
    if (arraySizes) {
        free(arraySizes);
        arraySizes = NULL;
    }
    if (arrayTypes) {
        free(arrayTypes);
        arrayTypes = NULL;
    }
    if (arrayFunctions) {
        for (int i = 0; i < arrayCount; i++) {
            free(arrayFunctions[i]);
        }
        free(arrayFunctions);
        arrayFunctions = NULL;
    }
    arrayCount = 0;

    // Free stringHashTable
    for (int i = 0; i < STRING_HASH_SIZE; i++) {
        StringEntry* entry = stringHashTable[i];
        while (entry) {
            StringEntry* temp = entry->next;
            free(entry->string);
            free(entry);
            entry = temp;
        }
        stringHashTable[i] = NULL;
    }    // Free arrayHashTable
    for (int i = 0; i < ARRAY_HASH_SIZE; i++) {
        ArrayEntry* aEntry = arrayHashTable[i];
        while (aEntry) {
            ArrayEntry* temp = aEntry->next;
            free(aEntry->name);
            free(aEntry);
            aEntry = temp;
        }
        arrayHashTable[i] = NULL;
    }
    
    // Free stringLabelHashTable
    for (int i = 0; i < STRING_LABEL_HASH_SIZE; i++) {
        StringLabelEntry* entry = stringLabelHashTable[i];
        while (entry) {
            StringLabelEntry* temp = entry->next;
            free(entry->label);
            free(entry);
            entry = temp;
        }
        stringLabelHashTable[i] = NULL;
    }
    
    // Free array initializers tracking
    if (arrayInitializers) {
        free(arrayInitializers);
        arrayInitializers = NULL;
    }
}
