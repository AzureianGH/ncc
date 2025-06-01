
// Platform-specific includes
#ifdef _WIN32
    #include <direct.h>
    #include <windows.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>
    #define PATH_SEPARATOR '\\'
    #define MAX_PATH_LEN MAX_PATH
#else
#define _POSIX_C_SOURCE 200809L
    #include <unistd.h>
    #include <libgen.h>
    #include <limits.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>
    #define PATH_SEPARATOR '/'
    #define MAX_PATH_LEN 255
#endif

#include "error_manager.h"
#include "preprocessor.h"
#include "codegen.h"

// Forward declarations
typedef struct ASTNode ASTNode;
void initLexer(const char* src);
void initParser();
ASTNode* parseProgram();
void initCodeGen(const char* outputFilename, unsigned int originAddress);
void initCodeGenSystemMode(const char* outputFilename, unsigned int originAddress, 
                          int setStackSegmentPointer, unsigned int stackSegment, unsigned int stackPointer);
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
    fprintf(stderr, "  -O<level>    Set optimization level (0=none, 1=basic)\n");
    fprintf(stderr, "  -com         Target MS-DOS executable (ORG 0x100)\n");
    fprintf(stderr, "  -sys         Target bootloader (ORG 0x7C00)\n");
    fprintf(stderr, "  -ss SS:SP    Set stack segment and pointer (for bootloaders)\n");
#ifndef NO_NASM
    fprintf(stderr, "  -S           Stop after generating assembly (don't assemble)\n");
#endif
    fprintf(stderr, "  -h           Display this help and exit\n");
}

#ifndef NO_NASM
char* getExecutableDir() {
    static char buffer[MAX_PATH_LEN];
    
#ifdef _WIN32
    GetModuleFileName(NULL, buffer, MAX_PATH_LEN);
    char* lastSeparator = strrchr(buffer, PATH_SEPARATOR);
#else
    ssize_t len = readlink("/proc/self/exe", buffer, MAX_PATH_LEN - 1);
    if (len != -1) {
        buffer[len] = '\0';
    } else {
        // Fallback if /proc/self/exe doesn't exist
        strcpy(buffer, ".");
        return buffer;
    }
    char* lastSeparator = strrchr(buffer, PATH_SEPARATOR);
#endif
    
    if (lastSeparator) *lastSeparator = '\0';
    return buffer;
}
#endif

int main(int argc, char* argv[]) {
    char* sourceFile = NULL;
    char* outputFile = "output.asm";
    int debugMode = 0;
    int debugLineMode = 0;
    unsigned int originAddress = 0;
    int optimizationLevel = OPT_LEVEL_NONE;
#ifndef NO_NASM
    int stopAfterAsm = 0;
#endif
    int systemMode = 0;  // Flag for bootloader mode
    int setStackSegmentPointer = 0;
    unsigned int stackSegment = 0;
    unsigned int stackPointer = 0;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-I", 2) == 0) {
            const char* path = argv[i] + 2;
            if (*path) {
                addIncludePath(path);
            } else if (i + 1 < argc) {
                addIncludePath(argv[++i]);
            }
        } else if (strncmp(argv[i], "-O", 2) == 0) {
            if (isdigit(argv[i][2])) optimizationLevel = argv[i][2] - '0';
            else if (i + 1 < argc && isdigit(argv[i+1][0])) optimizationLevel = argv[++i][0] - '0';
        } else if ((strcmp(argv[i], "-disp") == 0 || strcmp(argv[i], "-DISP") == 0) && i + 1 < argc) {
            originAddress = (unsigned int)strtoul(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "-com") == 0 || strcmp(argv[i], "-COM") == 0) {
            originAddress = 0x100;
            systemMode = 0;  // COM mode, not system mode
        } else if (strcmp(argv[i], "-sys") == 0 || strcmp(argv[i], "-SYS") == 0) {
            originAddress = 0x7C00;  // Standard bootloader address
            systemMode = 1;  // Enable system mode (bootloader)
        } else if ((strcmp(argv[i], "-ss") == 0 || strcmp(argv[i], "-SS") == 0) && i + 1 < argc) {
            char* sssp = argv[++i];
            char* colon = strchr(sssp, ':');
            if (colon) {
                *colon = '\0';  // Split at colon
                stackSegment = (unsigned int)strtoul(sssp, NULL, 16);
                stackPointer = (unsigned int)strtoul(colon + 1, NULL, 16);
                setStackSegmentPointer = 1;
            } else {
                fprintf(stderr, "Error: -ss option requires SS:SP format in hexadecimal\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0) {
            debugMode = 1;
        } else if (strcmp(argv[i], "-dl") == 0) {
            debugLineMode = 1;
#ifndef NO_NASM
        } else if (strcmp(argv[i], "-S") == 0) {
            stopAfterAsm = 1;
#endif
        } else if (strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--version") == 0) {
            #ifdef _WIN32
            printf("ncc [ncc-win-x64] ntos(6.2025.1.0) - 1.00\n");
            printf("Copyright (C) 2025 Nathan's Compiler Collection\n");
            printf("This is free software; see the source for copying conditions.  There is NO\n");
            printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
            #else
            printf("ncc [ncc-linux-x64] any-linux(6.2025.1.0) - 1.00\n");
            printf("Copyright (C) 2025 Nathan's Compiler Collection\n");
            printf("This is free software; see the source for copying conditions.  There is NO\n");
            printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\r");
            #endif
            return 0;
        } 
        else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            printUsage(argv[0]);
            return 1;
        } else {
            sourceFile = argv[i];
        }
    }

    if (!sourceFile) {
        fprintf(stderr, "Error: No source file specified\n");
        printUsage(argv[0]);
        return 1;
    }

    FILE* file = fopen(sourceFile, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open source file %s\n", sourceFile);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* sourceCode = (char*)malloc(fileSize + 1);
    if (!sourceCode) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return 1;
    }
    fread(sourceCode, 1, fileSize, file);
    sourceCode[fileSize] = '\0';
    fclose(file);

    initPreprocessor();
    addIncludePath(".");

    char* processedSource = NULL;
    if (strchr(sourceFile, '.')) {
        processedSource = preprocessFile(sourceFile);
    } else {
        processedSource = preprocessSource(sourceCode);
    }

    if (processedSource) {
        free(sourceCode);
        sourceCode = processedSource;
    }

    initErrorManager(sourceFile, sourceCode, !debugMode);
    initLexer(sourceCode);
    initParser();

#ifndef NO_NASM
    // Use temp.asm if we're going to assemble
    const char* asmFile = stopAfterAsm ? outputFile : "temp.asm";
#else
    const char* asmFile = outputFile;
#endif
    
    // Initialize code generator based on mode
    if (systemMode) {
        initCodeGenSystemMode(asmFile, originAddress, setStackSegmentPointer, stackSegment, stackPointer);
    } else {
        initCodeGen(asmFile, originAddress);
    }
    
    setOptimizationLevel(optimizationLevel, debugMode);

    ASTNode* ast = parseProgram();
    if (!ast) {
        fprintf(stderr, "Compilation failed\n");
        finalizeCodeGen();
        free(sourceCode);
        return 1;
    }

    if (debugMode) printAST(ast, 0);
    generateCode(ast);
    finalizeCodeGen();
    cleanupPreprocessor();
    free(sourceCode);

#ifndef NO_NASM
    // Assemble if not stopping after ASM
    if (!stopAfterAsm) {
        char command[1024];
        char* exeDir = getExecutableDir();
        
#ifdef _WIN32
        snprintf(command, sizeof(command),
            "cmd /C \"\"%s%ctooling%cnasm%cnasm.exe\" -f bin temp.asm -o \"%s\"\"",
            exeDir, PATH_SEPARATOR, PATH_SEPARATOR, PATH_SEPARATOR, outputFile);
#else
        snprintf(command, sizeof(command),
                    "\"%s%ctooling%cnasm%cnasm\" -f bin temp.asm -o \"%s\"",
                    exeDir, PATH_SEPARATOR, PATH_SEPARATOR, PATH_SEPARATOR, outputFile);
#endif

        int result = system(command);
        if (result != 0) {
            fprintf(stderr, "NASM failed\n");
            return 1;
        }
        remove("temp.asm");
    }
#endif

    if (debugMode) printf("Compilation successful. Output written to %s\n", outputFile);
    return 0;
}
