#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "string_literals.h"
#include "error_manager.h"

// Optimization levels
#define OPT_LEVEL_NONE 0    // -O0: No optimization
#define OPT_LEVEL_BASIC 1   // -O1: Basic optimizations (string merging)

// Optimization state
typedef struct {
    int level;              // Current optimization level
    int mergeStrings;       // Whether to merge identical strings
} OptimizationState;

extern OptimizationState optimizationState;

// Initialize code generator with optional origin displacement
void initCodeGen(const char* outputFilename, unsigned int orgAddress);

// Initialize code generator with system mode (bootloader) settings
void initCodeGenSystemMode(const char* outputFilename, unsigned int orgAddress, 
                          int setStackSegmentPointer, unsigned int stackSegment, unsigned int stackPointer);

// Set the optimization level
void setOptimizationLevel(int level, int quietMode);

// Get the current function name
const char* getCurrentFunctionName();

// Close code generator
void finalizeCodeGen();

// Generate code from the AST
void generateCode(ASTNode* root);

// Generate code for a function
void generateFunction(ASTNode* node);

// Generate code for a global variable declaration
void generateGlobalDeclaration(ASTNode* node);

// Generate code for a local variable declaration
void generateVariableDeclaration(ASTNode* node);

// Generate code for an array with initializers
void generateArrayWithInitializers(ASTNode* node);

// Generate code for a block of statements
void generateBlock(ASTNode* node);

// Generate code for a statement
void generateStatement(ASTNode* node);

// Generate code for an expression
void generateExpression(ASTNode* node);

// Generate code for a binary operation
void generateBinaryOp(ASTNode* node);

// Generate code for a unary operation
void generateUnaryOp(ASTNode* node);

// Check if a node represents a pointer type
int isPointerType(ASTNode* node);

// Generate code for a function call
void generateFunctionCall(ASTNode* node);

// Generate code for a return statement
void generateReturnStatement(ASTNode* node);

// Generate code for an inline assembly block
void generateAsmBlock(ASTNode* node);

// Generate code for a for loop
void generateForLoop(ASTNode* node);

// Generate code for a while loop
void generateWhileLoop(ASTNode* node);

// Generate code for a do-while loop
void generateDoWhileLoop(ASTNode* node);

// Generate code for an if statement
void generateIfStatement(ASTNode* node);

// Generate code for a program header
void generateProgramHeader();

#endif // CODEGEN_H