
// Platform-specific includes
#ifdef _WIN32
    #include <direct.h>
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #define TokenType Win32TokenType  // Avoid conflict with our TokenType
    #include <windows.h>
    #undef TokenType  // Restore our TokenType
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
#include "preprocessor_simple.h"
#include "codegen.h"
#include "lexer.h"
#include "parser_simple.h"

// Forward declarations
typedef struct ASTNode ASTNode;
void initLexer(const char* src);
void initParser();
ASTNode* parseProgram();
void initCodeGen(const char* outputFilename, unsigned long long originAddress, TargetArch targetArch);
void generateCode(ASTNode* root);
void finalizeCodeGen();
void printAST(ASTNode* node, int indent);
extern void cleanupPreprocessor();

void printUsage(const char* programName) {
    fprintf(stderr, "NCC: Nathan's Compiler Collection\n");
    fprintf(stderr, "Usage: %s [options] <source file>\n", programName);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -o <file>      Output to <file> (default: output.asm)\n");
    fprintf(stderr, "  -d             Debug mode (print AST)\n");
    fprintf(stderr, "  -dl            Debug line tracking (show preprocessor line mappings)\n");
    fprintf(stderr, "  -I<path>       Add <path> to include search paths\n");
    fprintf(stderr, "  -disp <addr>   Set origin displacement address\n");
    fprintf(stderr, "  -O<level>      Set optimization level (0=none, 1=basic, 2=advanced)\n");
    fprintf(stderr, "  -com           Target MS-DOS executable (ORG 0x100)\n");
    fprintf(stderr, "  -sys           Target bootloader (ORG 0x7C00)\n");
    fprintf(stderr, "  -m16           Generate 16-bit code (default)\n");
    fprintf(stderr, "  -m32           Generate 32-bit code\n");
    fprintf(stderr, "  -m64           Generate 64-bit code\n");
    fprintf(stderr, "  -force-16      In 16-bit mode, restrict to 16-bit registers only (no 32-bit ops)\n");
    fprintf(stderr, "  -felf          Generate ELF format with externals\n");
    fprintf(stderr, "  -fflat         Generate flat assembly (default)\n");
    fprintf(stderr, "  -std=c99       Use C99 standard (default)\n");
    fprintf(stderr, "  -std=c89       Use C89/C90 standard\n");
    fprintf(stderr, "  -std=gnu99     Use GNU C99 extensions\n");
    fprintf(stderr, "  -Wall          Enable all warnings\n");
    fprintf(stderr, "  -Werror        Treat warnings as errors\n");
    fprintf(stderr, "  -g             Generate debug information\n");
    fprintf(stderr, "  -k, --keep-going  Do not stop after the first error\n");
#ifndef NO_nas
    fprintf(stderr, "  -S             Stop after generating assembly (don't assemble)\n");
#endif
    fprintf(stderr, "  -h, --help     Display this help and exit\n");
    fprintf(stderr, "  --version      Show version information\n");
}

#ifndef NO_nas
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
    unsigned long long originAddress = 0;
    int optimizationLevel = OPT_LEVEL_NONE;
#ifndef NO_nas
    int stopAfterAsm = 0;
#endif
    int systemMode = 0;  // Flag for bootloader mode
    int setStackSegmentPointer = 0;
    unsigned int stackSegment = 0;
    unsigned int stackPointer = 0;
    
    // New C99 compiler options
    int targetWidth = 16;        // 16, 32, or 64 bit
    int outputFormat = 0;        // 0=flat, 1=ELF
    int cStandard = 99;          // 89, 99, or extensions
    int enableWarnings = 0;
    int warningsAsErrors = 0;
    int generateDebugInfo = 0;
    int forceStrict16 = 0;
    int keepGoing = 0; // by default, stop on first error

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-I", 2) == 0) {
            const char* path = argv[i] + 2;
            if (*path) {
                addIncludePath(path);
            } else if (i + 1 < argc) {
                addIncludePath(argv[++i]);
            }
        } else if (strncmp(argv[i], "-O", 2) == 0) {
            if (isdigit((int)argv[i][2])) optimizationLevel = argv[i][2] - '0';
            else if (i + 1 < argc && isdigit((int)argv[i+1][0])) optimizationLevel = argv[++i][0] - '0';
        } else if ((strcmp(argv[i], "-disp") == 0 || strcmp(argv[i], "-DISP") == 0) && i + 1 < argc) {
            originAddress = strtoul(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "-com") == 0 || strcmp(argv[i], "-COM") == 0) {
            originAddress = 0x100;
            systemMode = 0;  // COM mode, not system mode
            targetWidth = 16;
        } else if (strcmp(argv[i], "-sys") == 0 || strcmp(argv[i], "-SYS") == 0) {
            originAddress = 0x7C00;  // Standard bootloader address
            targetWidth = 16;
        } else if (strcmp(argv[i], "-m16") == 0) {
            targetWidth = 16;
        } else if (strcmp(argv[i], "-force-16") == 0) {
            forceStrict16 = 1;
        } else if (strcmp(argv[i], "-m32") == 0) {
            targetWidth = 32;
        } else if (strcmp(argv[i], "-m64") == 0) {
            targetWidth = 64;
        } else if (strcmp(argv[i], "-felf") == 0) {
            outputFormat = 1;  // ELF format
        } else if (strcmp(argv[i], "-fflat") == 0) {
            outputFormat = 0;  // Flat format
        } else if (strcmp(argv[i], "-std=c89") == 0) {
            cStandard = 89;
        } else if (strcmp(argv[i], "-std=c99") == 0) {
            cStandard = 99;
        } else if (strcmp(argv[i], "-std=gnu99") == 0) {
            cStandard = 199;  // GNU extensions
        } else if (strcmp(argv[i], "-Wall") == 0) {
            enableWarnings = 1;
        } else if (strcmp(argv[i], "-Werror") == 0) {
            warningsAsErrors = 1;
        } else if (strcmp(argv[i], "-g") == 0) {
            generateDebugInfo = 1;
        } else if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--keep-going") == 0) {
            keepGoing = 1;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0) {
            debugMode = 1;
        } else if (strcmp(argv[i], "-dl") == 0) {
            debugLineMode = 1;
#ifndef NO_nas
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
            printf("ncc [ncc-win-x64] ntos(6.2025.1.4) - 2.0 (C99 Edition)\n");
            printf("Copyright (C) 2025 Nathan's Compiler Collection\n");
            printf("This is free software; see the source for copying conditions.  There is NO\n");
            printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
            #else
            printf("ncc [ncc-linux-x64] any-linux(6.2025.1.4) - 2.0 (C99 Edition)\n");
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

    // Always print errors (quiet=0). We can add a -q later if needed.
    initErrorManager(sourceFile, sourceCode, 0);
    // Configure stop-on-first-error (default on unless -k)
    setStopOnFirstError(!keepGoing);
    initLexer(sourceCode);
    
    initParser();

#ifndef NO_nas
    // Use temp.asm if we're going to assemble
    const char* asmFile = stopAfterAsm ? outputFile : "temp.asm";
#else
    const char* asmFile = outputFile;
#endif
    
    // Initialize code generator based on mode and architecture
    if (systemMode) {
        // For now, use simple initialization
        initCodeGen(asmFile, originAddress, ARCH_X86_16);
    } else {
        initCodeGen(asmFile, originAddress, targetWidth == 16 ? ARCH_X86_16 :
                    targetWidth == 32 ? ARCH_X86_32 : ARCH_X86_64);
    }
    
    // Set compiler options with proper type conversion
    setOptimizationLevel((OptimizationLevel)optimizationLevel, debugMode);
    setTargetWidth(targetWidth);
    setOutputFormat((OutputFormat)outputFormat);
    setCStandard((CStandard)cStandard);
    setWarningMode(enableWarnings, warningsAsErrors);
    setDebugMode(generateDebugInfo);
    if (targetWidth == 16) setForceStrict16(forceStrict16);

    ASTNode* ast = parseProgram();
    // If parser produced any errors, print them and halt
    if (hasErrors()) {
        printAllErrors();
        printErrorsSummary();
        finalizeCodeGen();
        free(sourceCode);
        return 1;
    }
    if (!ast) {
        fprintf(stderr, "Compilation failed\n");
        finalizeCodeGen();
        free(sourceCode);
        return 1;
    }

    if (debugMode) printAST(ast, 0);
    generateCode(ast);
    // Stop if any errors were reported during code generation
    if (hasErrors()) {
        printAllErrors();
        printErrorsSummary();
        finalizeCodeGen();
        free(sourceCode);
        return 1;
    }
    finalizeCodeGen();
    cleanupPreprocessor();
    free(sourceCode);

#ifndef NO_nas
    // Assemble if not stopping after ASM
    if (!stopAfterAsm) {
        char command[1024];
        char* exeDir = getExecutableDir();
        const char* widthFlag = (targetWidth == 32) ? "-m32" : (targetWidth == 64) ? "-m64" : "-m16";
        const char* formatFlag = (outputFormat == 1) ? "-f elf" : "-f bin";
        
#ifdef _WIN32
        if (debugMode)
        {
            snprintf(command, sizeof(command),
            "cmd /C \"\"%s%ctooling%cnas.exe\" %s -v %s temp.asm -o \"%s\"\"",
            exeDir, PATH_SEPARATOR, PATH_SEPARATOR, widthFlag, formatFlag, outputFile);
        }
        else
        {
            snprintf(command, sizeof(command),
            "cmd /C \"\"%s%ctooling%cnas.exe\" %s %s temp.asm -o \"%s\"\"",
            exeDir, PATH_SEPARATOR, PATH_SEPARATOR, widthFlag, formatFlag, outputFile);
        }
#else
        if (debugMode)
        {
            snprintf(command, sizeof(command),
                    "\"%s%ctooling%cnas\" %s -v %s temp.asm -o \"%s\"",
                    exeDir, PATH_SEPARATOR, PATH_SEPARATOR, widthFlag, formatFlag, outputFile);
        }
        else
        {
            snprintf(command, sizeof(command),
                    "\"%s%ctooling%cnas\" %s %s temp.asm -o \"%s\"",
                    exeDir, PATH_SEPARATOR, PATH_SEPARATOR, widthFlag, formatFlag, outputFile);
        }
#endif

        int result = system(command);
        if (result != 0) {
            fprintf(stderr, "NAS failed\n");
            return 1;
        }
        remove("temp.asm");
    }
#endif

    if (debugMode) printf("Compilation successful. Output written to %s\n", outputFile);
    if (hasWarnings()) {
        printErrorsSummary();
    }
    return 0;
}
