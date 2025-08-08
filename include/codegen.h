#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <stdio.h>

// Constants
#define MAX_REGISTERS 16

// Target architectures
typedef enum {
    ARCH_X86_16,
    ARCH_X86_32,
    ARCH_X86_64
} TargetArch;

// Output formats
typedef enum {
    FORMAT_FLAT,     // Flat binary/assembly
    FORMAT_ELF,      // ELF object format
    FORMAT_COFF,     // COFF object format
    FORMAT_PE        // PE format
} OutputFormat;

// C Standards
typedef enum {
    STD_C89,
    STD_C99,
    STD_GNU89,
    STD_GNU99
} CStandard;

// Optimization levels
typedef enum {
    OPT_LEVEL_NONE = 0,
    OPT_LEVEL_BASIC = 1,
    OPT_LEVEL_ADVANCED = 2,
    OPT_LEVEL_AGGRESSIVE = 3
} OptimizationLevel;

// String literal management
typedef struct StringLiteral {
    char* label;
    char* value;
    struct StringLiteral* next;
} StringLiteral;

// Register allocation
typedef enum {
    // 16-bit registers
    REG_AX, REG_BX, REG_CX, REG_DX,
    REG_SI, REG_DI, REG_BP, REG_SP,
    REG_AL, REG_AH, REG_BL, REG_BH,
    REG_CL, REG_CH, REG_DL, REG_DH,
    
    // 32-bit registers
    REG_EAX, REG_EBX, REG_ECX, REG_EDX,
    REG_ESI, REG_EDI, REG_EBP, REG_ESP,
    
    // 64-bit registers
    REG_RAX, REG_RBX, REG_RCX, REG_RDX,
    REG_RSI, REG_RDI, REG_RBP, REG_RSP,
    REG_R8, REG_R9, REG_R10, REG_R11,
    REG_R12, REG_R13, REG_R14, REG_R15,
    
    // Floating point registers
    REG_ST0, REG_ST1, REG_ST2, REG_ST3,
    REG_ST4, REG_ST5, REG_ST6, REG_ST7,
    
    // SSE registers
    REG_XMM0, REG_XMM1, REG_XMM2, REG_XMM3,
    REG_XMM4, REG_XMM5, REG_XMM6, REG_XMM7,
    REG_XMM8, REG_XMM9, REG_XMM10, REG_XMM11,
    REG_XMM12, REG_XMM13, REG_XMM14, REG_XMM15,
    
    REG_INVALID
} Register;

// Code generation context
typedef struct CodeGenContext {
    FILE* outputFile;
    TargetArch targetArch;
    OutputFormat outputFormat;
    CStandard cStandard;
    OptimizationLevel optimization;
    
    // Target-specific settings
    int targetWidth;        // 16, 32, or 64 bits
    unsigned int originAddress;
    int systemMode;         // Bootloader mode
    
    // Assembly generation options
    int generateComments;
    int generateDebugInfo;
    int generateLineNumbers;
    
    // Warning and error settings
    int enableWarnings;
    int warningsAsErrors;
    
    // Current state
    int currentFunctionStackSize;
    int labelCounter;
    int tempCounter;
    
    // Symbol tables and scopes
    struct SymbolTable* globalSymbols;
    struct SymbolTable* currentScope;
    
    // Register allocation
    Register allocatedRegs[32];
    int regCount;
    
    // String literals
    struct StringLiteral** strings;
    int stringCount;
    
    // Global variables
    struct GlobalVar** globals;
    int globalCount;
    
    // Functions
    struct FunctionInfo** functions;
    int functionCount;
    
    // External symbols (for ELF)
    struct ExternalSymbol** externals;
    int externalCount;
    
    // Assembly sections
    struct AsmSection* currentSection;
    struct AsmSection** sections;
    int sectionCount;
    
} CodeGenContext;

// Global variable structure
typedef struct GlobalVar {
    char* name;
    TypeInfo* type;
    ASTNode* initializer;
    char* label;
    int isStatic;
    int isExtern;
} GlobalVar;

// Function information
typedef struct FunctionInfo {
    char* name;
    TypeInfo* returnType;
    Parameter** parameters;
    int paramCount;
    char* label;
    int isStatic;
    int isExtern;
    int isInline;
    int stackSize;
} FunctionInfo;

// External symbol (for ELF format)
typedef struct ExternalSymbol {
    char* name;
    int isFunction;
    TypeInfo* type;
} ExternalSymbol;

// Assembly section
typedef struct AsmSection {
    char* name;
    char* content;
    int size;
    int isCode;
    int isData;
    int isReadOnly;
} AsmSection;

// Inline assembly constraint
typedef struct AsmConstraint {
    char* constraint;
    char* operand;
    Register reg;
} AsmConstraint;

// Code generator state
typedef struct CodeGenerator {
    TargetArch targetArch;
    OutputFormat outputFormat;
    FILE* output;
    int targetWidth;
    int labelCount;
    int stringLiteralCount;
    int instructionCount;
    StringLiteral* stringLiterals;
    int registerInUse[MAX_REGISTERS];
    // Function-scoped epilogue label for unified returns
    char* currentFunctionEpilogueLabel;
    // Optimization and flags
    int optimizationLevel;   // 0..3
    int forceStrict16;       // when targetWidth==16, force 16-bit regs only
    // Special placement support
    int stringsEmitted;      // if we've already flushed string literals
    char* stringAnchorLabel; // label where to place strings if provided
} CodeGenerator;

// Function declarations
    
// Initialization and cleanup
void initCodeGen(const char* outputFilename, unsigned long long originAddress, TargetArch targetArch);
void initCodeGenSystemMode(const char* outputFilename, unsigned int originAddress, 
                          int setStackSegmentPointer, unsigned int stackSegment, unsigned int stackPointer);
void finalizeCodeGen(void);
void cleanupCodeGen(void);

// Configuration
void setTargetWidth(int width);
void setOutputFormat(OutputFormat format);
void setCStandard(CStandard standard);
void setOptimizationLevel(OptimizationLevel level, int debugMode);
void setWarningMode(int enableWarnings, int warningsAsErrors);
void setDebugMode(int generateDebugInfo);
void setForceStrict16(int enable);

// Main code generation
void generateCode(ASTNode* root);
void generateProgram(ASTNode* program);
void generateTranslationUnit(ASTNode* unit);

// Declaration generation
void generateDeclaration(ASTNode* decl);
void generateFunctionDeclaration(ASTNode* func);
void generateVariableDeclaration(ASTNode* var);
void generateStructDeclaration(ASTNode* structDecl);
void generateUnionDeclaration(ASTNode* unionDecl);
void generateEnumDeclaration(ASTNode* enumDecl);
void generateTypedefDeclaration(ASTNode* typedefDecl);

// Statement generation
void generateStatement(ASTNode* stmt);
void generateCompoundStatement(ASTNode* compound);
void generateExpressionStatement(ASTNode* expr);
void generateIfStatement(ASTNode* ifStmt);
void generateWhileStatement(ASTNode* whileStmt);
void generateDoWhileStatement(ASTNode* doWhile);
void generateForStatement(ASTNode* forStmt);
void generateSwitchStatement(ASTNode* switchStmt);
void generateReturnStatement(ASTNode* returnStmt);
void generateBreakStatement(ASTNode* breakStmt);
void generateContinueStatement(ASTNode* continueStmt);
void generateGotoStatement(ASTNode* gotoStmt);
void generateLabelStatement(ASTNode* labelStmt);

// Expression generation
void generateExpression(ASTNode* expr);
void generateBinaryOperation(ASTNode* binary);
void generateUnaryOperation(ASTNode* unary);
void generateTernaryOperation(ASTNode* ternary);
void generateFunctionCall(ASTNode* call);
void generateArrayAccess(ASTNode* access);
void generateMemberAccess(ASTNode* member);
void generateAssignment(ASTNode* assignment);
void generateCastExpression(ASTNode* cast);
void generateSizeofExpression(ASTNode* sizeofExpr);

// C99 specific generation
void generateCompoundLiteral(ASTNode* compound);
void generateDesignatedInitializer(ASTNode* designated);
void generateGenericSelection(ASTNode* generic);
void generateAlignofExpression(ASTNode* alignof);
void generateComplexOperation(ASTNode* complex);

// Inline assembly generation
void generateInlineAssembly(ASTNode* inlineAsm);
void generateSimpleInlineAsm(ASTNode* simpleAsm);
void generateExtendedInlineAsm(ASTNode* extendedAsm);
void processAsmConstraints(ASTNode* constraints);
void allocateAsmRegisters(AsmConstraint* constraints, int count);

// Literal generation
void generateIntegerLiteral(ASTNode* literal);
void generateFloatLiteral(ASTNode* literal);
void generateStringLiteral(ASTNode* literal);
void generateCharLiteral(ASTNode* literal);
void generateBoolLiteral(ASTNode* literal);

// Memory management
void generateAllocation(TypeInfo* type, int isLocal);
void generateDeallocation(TypeInfo* type, int isLocal);
void generateMemcpy(ASTNode* dest, ASTNode* src, int size);
void generateMemset(ASTNode* dest, int value, int size);

// Register allocation
Register allocateRegister(void);
void freeRegister(Register reg);
void spillRegister(Register reg);
void restoreRegister(Register reg);
const char* getRegisterName(Register reg, int width);

// Assembly output functions
void emitInstruction(const char* format, ...);
void emitLabel(const char* label);
void emitComment(const char* format, ...);
void emitDirective(const char* directive, const char* args);
void emitData(const char* format, ...);
void emitString(const char* str);

// NAS-specific assembly generation
void emitNASHeader(void);
void emitNASSection(const char* sectionName);
void emitNASWidth(int width);
void emitNASOrigin(unsigned int address);
void emitNASGlobal(const char* symbol);
void emitNASExtern(const char* symbol);
void emitNASExtend(const char* symbol);  // NAS-specific extern

// ELF format support
void generateELFHeader(void);
void generateELFSymbolTable(void);
void generateELFRelocations(void);
void generateELFSections(void);

// Symbol management
void declareGlobalSymbol(const char* name, TypeInfo* type);
void declareExternalSymbol(const char* name, TypeInfo* type, int isFunction);
void defineFunction(const char* name, TypeInfo* returnType, Parameter** params, int paramCount);
void defineGlobalVariable(const char* name, TypeInfo* type, ASTNode* initializer);
const char* getSymbolLabel(const char* name);

// Type utilities
int getTypeSize(TypeInfo* type, int targetWidth);
int getTypeAlignment(TypeInfo* type, int targetWidth);
const char* getTypeName(TypeInfo* type);
int isFloatingPointType(TypeInfo* type);
int isIntegerType(TypeInfo* type);
int isPointerType(TypeInfo* type);
int isStructType(TypeInfo* type);
int isArrayType(TypeInfo* type);

// Stack management
void pushValue(TypeInfo* type);
void popValue(TypeInfo* type);
int allocateStackSpace(int size);
void deallocateStackSpace(int size);
int getCurrentStackOffset(void);

// Jump and label management
const char* generateLabel(void);
const char* generateTempLabel(void);
void markLabel(const char* label);
void jumpTo(const char* label);
void conditionalJump(const char* label, const char* condition);

// Optimization passes
void optimizeDeadCode(ASTNode* root);
void optimizeConstantFolding(ASTNode* root);
void optimizeCommonSubexpressions(ASTNode* root);
void optimizeRegisterAllocation(void);
void optimizePeephole(void);

// Error handling
void codegenError(int line, int column, const char* format, ...);
void codegenWarning(int line, int column, const char* format, ...);
int hasCodegenErrors(void);
const char* getLastCodegenError(void);

// Debugging support
void generateDebugInfo(ASTNode* node);
void generateLineNumber(int line);
void generateSourceReference(const char* filename, int line);

// Standard library support
void generateRuntimeSupport(void);
void generateStartupCode(void);
void generateShutdownCode(void);

// Architecture-specific code generation
void generateArchSpecificPrologue(void);
void generateArchSpecificEpilogue(void);
void generateArchSpecificCall(ASTNode* call);
void generateArchSpecificReturn(ASTNode* returnStmt);

// Utility functions
const char* escapeString(const char* str);
char* generateUniqueLabel(const char* prefix);
void alignToSize(int size);
int calculatePadding(int currentSize, int alignment);
void finalizeCodeGen(void);

#endif // CODEGEN_H
