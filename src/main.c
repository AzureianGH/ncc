#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error_manager.h"
#include "preprocessor.h"

// Forward declarations from other files
typedef struct ASTNode ASTNode;
void initLexer(const char* src);
void initParser();
ASTNode* parseProgram();
void initCodeGen(const char* outputFilename, unsigned int originAddress);
void generateCode(ASTNode* root);
void finalizeCodeGen();
void printAST(ASTNode* node, int indent);
extern void cleanupPreprocessor();
extern void printLineMappings();

void printUsage(const char* programName) {
    fprintf(stderr, "NCC: Nathan's C Compiler\n");
    fprintf(stderr, "Usage: %s [options] <source file>\n", programName);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -o <file>    Output to <file> (default: output.asm)\n");
    fprintf(stderr, "  -d           Debug mode (print AST)\n");
    fprintf(stderr, "  -dl          Debug line tracking (show preprocessor line mappings)\n");
    fprintf(stderr, "  -I<path>     Add <path> to include search paths\n");
    fprintf(stderr, "  -disp <addr> Set origin displacement address\n");
    fprintf(stderr, "  -h           Display this help and exit\n");
}

int main(int argc, char* argv[]) {
    char* sourceFile = NULL;
    char* outputFile = "output.asm";
    int debugMode = 0;
    unsigned int originAddress = 0;  // Default load address (no ORG)
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        // Handle include path flag
        if (strncmp(argv[i], "-I", 2) == 0) {
            const char* path = argv[i] + 2; // Skip -I
            if (*path) {
                addIncludePath(path);
            } else if (i + 1 < argc) {
                // If the path is in the next argument
                addIncludePath(argv[++i]);
            }
        }
        // Handle origin displacement flag (hex or decimal)
        else if ((strcmp(argv[i], "-disp") == 0 || strcmp(argv[i], "-DISP") == 0) && i + 1 < argc) {
            // Parse number with auto-detect base (0x for hex)
            originAddress = (unsigned int)strtoul(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0) {
            debugMode = 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            printUsage(argv[0]);
            return 1;
        } else {
            sourceFile = argv[i];
        }
    }
    
    // Check if source file was specified
    if (!sourceFile) {
        fprintf(stderr, "Error: No source file specified\n");
        printUsage(argv[0]);
        return 1;
    }
    
    // Read the source file
    FILE* file = fopen(sourceFile, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open source file %s\n", sourceFile);
        return 1;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate memory for file contents
    char* sourceCode = (char*)malloc(fileSize + 1);
    if (!sourceCode) {
        fprintf(stderr, "Error: Could not allocate memory for source code\n");
        fclose(file);
        return 1;
    }
      // Read file contents
    size_t bytesRead = fread(sourceCode, 1, fileSize, file);
    sourceCode[bytesRead] = '\0';
    fclose(file);    // Initialize the preprocessor and process the source code
    initPreprocessor();
    
    // Add current directory to include paths
    addIncludePath(".");
    
    // Process source file
    char* processedSource = NULL;
    if (strstr(sourceFile, ".") != NULL) {
        // Treat as file if it has an extension
        processedSource = preprocessFile(sourceFile);
    } else {
        processedSource = preprocessSource(sourceCode);
    }
    
    if (processedSource) {
        free(sourceCode);
        sourceCode = processedSource;
    }

    // Initialize compiler components
    initErrorManager(sourceFile, sourceCode, !debugMode);
    initLexer(sourceCode);
    initParser();
    initCodeGen(outputFile, originAddress);
    
    // Parse the source code into an AST
    ASTNode* ast = parseProgram();
    
    // Generate code from the AST
    if (ast) {
        if (debugMode) {
            printAST(ast, 0);
        }
        generateCode(ast);
        if (debugMode) printf("Compilation successful. Output written to %s\n", outputFile);
    } else {
        fprintf(stderr, "Compilation failed\n");
        finalizeCodeGen();
        free(sourceCode);
        return 1;
    }
      // Clean up
    finalizeCodeGen();
    cleanupPreprocessor();
    free(sourceCode);
    
    return 0;
}