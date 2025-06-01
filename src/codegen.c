#include "codegen.h"
#include "ast.h"
#include "string_literals.h"
#include "error_manager.h"
#include "global_variables.h"
#include "type_checker.h"
#include "struct_support.h"
#include "struct_codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// External declarations from type_checker.c
extern TypeInfo* getTypeInfo(const char* name);
extern TypeInfo* getTypeInfoFromExpression(ASTNode* expr);

// Define the optimization state
OptimizationState optimizationState = {
    .level = OPT_LEVEL_NONE,
    .mergeStrings = 0
};

// Output file for assembly code
FILE* asmFile = NULL;

// String literals table
int stringLiteralCount = 0;
char** stringLiterals = NULL;
// Track where the redefined strings start
int redefineStringStartIndex = 0;

// Array declarations table
int arrayCount = 0;
char** arrayNames = NULL;
int* arraySizes = NULL;
DataType* arrayTypes = NULL;
char** arrayFunctions = NULL; // Track function names for arrays
// Track where the redefined arrays start
int redefineArrayStartIndex = 0;

// Flags to track if marker functions were found
int stringMarkerFound = 0;
int arrayMarkerFound = 0;
int globalMarkerFound = 0;

// Flag to track if the location should be redefined (when __NCC_REDEFINE_LOCALS is defined)
int redefineLocalsFound = 0;

// Label counter for unique labels
int labelCounter = 0;

// Current function being generated
static char* currentFunction = NULL;
static int currentFunctionIsNaked = 0;

// Variable tracking for stack offsets
#define MAX_LOCALS 64
typedef struct {
    char* name;
    int offset;
} LocalVariable;

static LocalVariable localVars[MAX_LOCALS];
static int localVarCount = 0;
static int stackSize = 0;

// Origin address for ORG directive
static unsigned int originAddress = 0;

// Get the current name of the function being generated
const char* getCurrentFunctionName() {
    return currentFunction ? currentFunction : "global";
}

// Clear local variables when entering a new function
void clearLocalVars() {
    for (int i = 0; i < localVarCount; i++) {
        if (localVars[i].name) {
            free(localVars[i].name);
        }
    }
    localVarCount = 0;
    stackSize = 0;
}

// Get the stack offset for a local variable, return 0 if not found (global)
int getLocalVarOffset(const char* name) {
    for (int i = 0; i < localVarCount; i++) {
        if (localVars[i].name && strcmp(localVars[i].name, name) == 0) {
            return localVars[i].offset;
        }
    }
    return 0;  // Not a local variable
}

// gcc disable unused variable warnings
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif


// Add a local variable and return its stack offset
int addLocalVariable(const char* name, int size) {
    if (localVarCount >= MAX_LOCALS) {
        fprintf(stderr, "Error: Too many local variables\n");
        exit(1);
    }
      
    // For long/unsigned long, allocate 4 bytes
    // For other types, use word-sized (2-byte) allocations on the stack for x86
    // This ensures proper alignment and simplifies address calculations
    int allocationSize = size == 4 ? 4 : 2; // Allocate 4 bytes for long types, 2 bytes for others
    
    stackSize += allocationSize;
    localVars[localVarCount].name = strdupc(name);
    localVars[localVarCount].offset = stackSize;
    localVarCount++;
    
    return stackSize;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

// Get the stack offset for a variable
int getVariableOffset(const char* name) {
    for (int i = 0; i < localVarCount; i++) {
        if (strcmp(localVars[i].name, name) == 0) {
            return localVars[i].offset;
        }
    }
      // Variable not found - might be a global
    return 0;
}

// Check if a variable is a parameter (parameters have negative offsets)
int isParameter(const char* name) {
    for (int i = 0; i < localVarCount; i++) {
        if (strcmp(localVars[i].name, name) == 0) {
            return localVars[i].offset < 0;
        }
    }
    return 0;
}

// Initialize code generator
void initCodeGen(const char* outputFilename, unsigned int orgAddr) {
    asmFile = fopen(outputFilename, "w");
    if (!asmFile) {
        fprintf(stderr, "Error: Could not open output file %s\n", outputFilename);
        exit(1);
    }
    labelCounter = 0;
    
    // Initialize string tracking
    stringLiteralCount = 0;
    stringLiterals = NULL;
    stringMarkerFound = 0;
    redefineStringStartIndex = 0;
      
    // Initialize array tracking
    arrayCount = 0;
    arrayNames = NULL;
    arraySizes = NULL;
    arrayTypes = NULL;
    arrayFunctions = NULL; // Track function names for arrays
    arrayMarkerFound = 0;
    redefineArrayStartIndex = 0;
    
    // Store origin address
    originAddress = orgAddr;
    
    // Write origin directive to ASM file
    fprintf(asmFile, "org 0x%X\n\n", orgAddr);
}

// Initialize code generator for system mode (bootloader)
void initCodeGenSystemMode(const char* outputFilename, unsigned int orgAddr,
                          int setStackSegmentPointer, unsigned int stackSegment, unsigned int stackPointer) {
    asmFile = fopen(outputFilename, "w");
    if (!asmFile) {
        fprintf(stderr, "Error: Could not open output file %s\n", outputFilename);
        exit(1);
    }
    labelCounter = 0;
    
    // Initialize string tracking
    stringLiteralCount = 0;
    stringLiterals = NULL;
    stringMarkerFound = 0;
    redefineStringStartIndex = 0;
      
    // Initialize array tracking
    arrayCount = 0;
    arrayNames = NULL;
    arraySizes = NULL;
    arrayTypes = NULL;
    arrayFunctions = NULL; // Track function names for arrays
    arrayMarkerFound = 0;
    redefineArrayStartIndex = 0;
    
    // Store origin address
    originAddress = orgAddr;
    
    // Write origin directive to ASM file
    fprintf(asmFile, "org 0x%X\n\n", orgAddr);
    
    // Generate bootloader initialization code
    fprintf(asmFile, "; System mode initialization code\n");
    fprintf(asmFile, "cli                      ; Disable interrupts\n");
    fprintf(asmFile, "xor ax, ax               ; Clear AX register\n");
    
    if (setStackSegmentPointer) {
        // Set up stack segment and pointer as specified
        fprintf(asmFile, "mov ax, 0x%04X         ; Set CS to specified value\n", stackSegment);
        fprintf(asmFile, "mov ss, ax             ; Set stack segment\n");
        fprintf(asmFile, "xor ax, ax             ; Clear AX register\n");
        fprintf(asmFile, "mov ax, 0x%04X         ; Set SP to specified value\n", stackPointer);
        fprintf(asmFile, "mov sp, ax             ; Set stack pointer\n");
    }
    
    fprintf(asmFile, "sti                      ; Re-enable interrupts\n");
    fprintf(asmFile, "\n; Begin program code\n");
}

// Close code generator
void finalizeCodeGen() {
    if (asmFile) {
        // Generate any global variables that weren't emitted at a marker
        generateRemainingGlobals(asmFile);
        
        // Generate string literals section before closing
        generateStringLiteralsSection();
        
        // Free string literals
        for (int i = 0; i < stringLiteralCount; i++) {
            if (stringLiterals[i]) {
                free(stringLiterals[i]);
            }
        }
        free(stringLiterals);
        
        // Free array tracking
        for (int i = 0; i < arrayCount; i++) {
            if (arrayNames[i]) {
                free(arrayNames[i]);
            }
        }
        free(arrayNames);
        free(arraySizes);
        free(arrayTypes);
        
        // Clean up global variables
        cleanupGlobals();
        
        fclose(asmFile);
        asmFile = NULL;
    }
}

// Check if a node represents a pointer type
int isPointerType(ASTNode* node) {
    if (!node) return 0;
    
    // Direct check for literals - string literals are pointers
    if (node->type == NODE_LITERAL) {
        // String literals are pointers
        if (node->literal.data_type == TYPE_CHAR && node->literal.string_value) {
            return 1;
        }
        // Far pointers
        if (node->literal.data_type == TYPE_FAR_POINTER) {
            return 1;
        }
    }
    
    // For identifiers, check the symbol table
    if (node->type == NODE_IDENTIFIER) {
        TypeInfo* typeInfo = getTypeInfo(node->identifier);
        return typeInfo && typeInfo->is_pointer;
    }
    
    // For expressions, use the type inferred from the expression
    TypeInfo* typeInfo = getTypeInfoFromExpression(node);
    return typeInfo && typeInfo->is_pointer;
}

// Generate a unique label ID
int getNextLabelId() {
    return labelCounter++;
}

// Generate a label with prefix
char* generateLabel(const char* prefix) {
    char* label = malloc(strlen(prefix) + 10);
    sprintf(label, "%s%d", prefix, getNextLabelId());
    return label;
}

void generateGlobalDeclaration(ASTNode* node);
void generateFunction(ASTNode* node);
void generateBlock(ASTNode* node);
void generateStatement(ASTNode* node);
void generateVariableDeclaration(ASTNode* node);
void generateExpression(ASTNode* node);
void generateBinaryOp(ASTNode* node);
void generateTernaryExpression(ASTNode* node);
void generateReturnStatement(ASTNode* node);
void generateFunctionCall(ASTNode* node);
void generateAsmBlock(ASTNode* node);
void generateAsmStmt(ASTNode* node);
void generateForLoop(ASTNode* node);

// Generate the header for the program
void generateProgramHeader() {
    fprintf(asmFile, "; 8086 Assembly generated by NCC Compiler\n");
    fprintf(asmFile, "bits 16\n");
    // ORG directive for load address
    fprintf(asmFile, "org 0x%X\n\n", originAddress);
     // No program entry point boilerplate for flat binary
}

// Main code generation function
void generateCode(ASTNode* root) {
    if (!root) {
        fprintf(stderr, "Error: Empty AST\n");
        return;
    }
      // Debug message removed to reduce output verbosity
    
    // Start with program header
    generateProgramHeader();
    
    // Process the AST - starting from the program node
    if (root->type == NODE_PROGRAM) {
        // Debug message removed to reduce output verbosity
          // Process all top-level declarations and functions
        ASTNode* current = root->left;
          if (!current) {
            fprintf(stderr, "Warning: Program node has no children (empty program)\n");
        }
        
        int nodeCount = 0;
        while (current) {
            nodeCount++;
              switch (current->type) {
                case NODE_FUNCTION:
                    // Debug message removed to reduce output verbosity
                    generateFunction(current);
                    break;
                case NODE_DECLARATION:
                    // Debug message removed to reduce output verbosity 
                    generateGlobalDeclaration(current);
                    break;
                default:
                    fprintf(stderr, "Warning: Unsupported top-level node type: %d\n", current->type);
            }
            current = current->next;
        }
    }
}

// Forward declaration from preprocessor.h
extern int isMacroDefined(const char* name);

// Generate code for a function
void generateFunction(ASTNode* node) {
    if (!node || node->type != NODE_FUNCTION) return;
    
    char* funcName = node->function.func_name;
    
    // Check if we need to redefine locals flag
    if (isMacroDefined("__NCC_REDEFINE_LOCALS") && !redefineLocalsFound) {
        // Found the directive, so we need to reset markers to allow redefinition
        redefineLocalsFound = 1;
        stringMarkerFound = 0;
        arrayMarkerFound = 0;
        globalMarkerFound = 0;
        
        // Mark the current indices as the starting points for redefinition
        redefineStringStartIndex = stringLiteralCount;
        redefineArrayStartIndex = arrayCount;
        markRedefineGlobalsStart(); // Mark global start index
        
        fprintf(asmFile, "; Detected __NCC_REDEFINE_LOCALS - marker locations will be updated\n");
    }
    
    // Check if this is a special marker function
    if (strcmp(funcName, "_NCC_STRING_LOC") == 0) {
        // This is where string literals should go
        generateStringsAtMarker();
        
        // Output only a comment for the marker when redefining locations
        if (!redefineLocalsFound) {
            fprintf(asmFile, "; String literal location marker\n");
            fprintf(asmFile, "_%s:\n", funcName);
        } else {
            fprintf(asmFile, "; String literal location marker (redefined)\n");
        }
        return;
    }
    else if (strcmp(funcName, "_NCC_ARRAY_LOC") == 0) {
        // This is where arrays should go
        generateArraysAtMarker();
        
        // Output only a comment for the marker when redefining locations
        if (!redefineLocalsFound) {
            fprintf(asmFile, "; Array location marker\n");
            fprintf(asmFile, "_%s:\n", funcName);
        } else {
            fprintf(asmFile, "; Array location marker (redefined)\n");
        }
        return;
    }
    else if (strcmp(funcName, "_NCC_GLOBAL_LOC") == 0) {
        // This is where global variables should go
        generateGlobalsAtMarker(asmFile);
        
        // Output only a comment for the marker when redefining locations
        if (!redefineLocalsFound) {
            fprintf(asmFile, "; Global variable location marker\n");
            fprintf(asmFile, "_%s:\n", funcName);
        } else {
            fprintf(asmFile, "; Global variable location marker (redefined)\n");
        }
        return;
    }
      // Regular function processing
    // Clear any local variables from previous functions
    clearLocalVars();
    funcName = node->function.func_name;
    currentFunction = funcName;
    currentFunctionIsNaked = node->function.info.is_naked;
    
    fprintf(asmFile, "; Function: %s\n", funcName);
    
    // Handle static functions - they get a special prefix to make them file-local
    if (node->function.info.is_static) {
        // Get sanitized filename for prefix
        const char* filename = getCurrentSourceFilename();
        char* prefix = strdupc(filename);
        
        // Remove extension and sanitize for label use
        char* dot = strrchr(prefix, '.');
        if (dot) *dot = '\0';
        
        // Replace invalid characters with underscore
        for (char* c = prefix; *c; c++) {
            if (!isalnum(*c) && *c != '_') {
                *c = '_';
            }
        }
        
        fprintf(asmFile, "_%s_%s: ; static function (file-local)\n", prefix, funcName);
        free(prefix);
    } else {
        fprintf(asmFile, "_%s:\n", funcName);  // Prepend underscore to function names
    }
      // Check if this is a naked function (no prologue/epilogue)
    if (currentFunctionIsNaked) {
        fprintf(asmFile, "    ; Naked function - no prologue generated\n");
    }
    // Check if this function uses stackframe
    else if (node->function.info.is_stackframe) {
        fprintf(asmFile, "    ; Setup stackframe with register preservation\n");
        fprintf(asmFile, "    push bp\n");
        fprintf(asmFile, "    mov bp, sp\n");
        fprintf(asmFile, "    push bx\n");
        fprintf(asmFile, "    push cx\n");
        fprintf(asmFile, "    push dx\n");
        fprintf(asmFile, "    push si\n");
        fprintf(asmFile, "    push di\n");
        
        // We'll calculate the actual space needed after processing all declarations
        fprintf(asmFile, "    ; Space for local variables will be allocated later\n\n");
    } else {
        // Standard function prologue
        fprintf(asmFile, "    push bp\n");
        fprintf(asmFile, "    mov bp, sp\n\n");
    }
      // Add function parameters to local variable table
    // Parameters start at bp+4 (return address is at bp+2)
    int paramOffset = 4;
    ASTNode* param = node->function.params;
    while (param) {
        // Parameters are accessed via positive offsets from bp
        if (param->type == NODE_DECLARATION) {
            // Store parameters in reverse order for easier access
            localVars[localVarCount].name = strdupc(param->declaration.var_name);
            localVars[localVarCount].offset = -paramOffset; // Negative offset means it's a parameter
            localVarCount++;
              // Parameters always take 2 bytes on the stack in 16-bit mode
            paramOffset += 2;
        }
        param = param->next;
    }
    
    // For variadic functions, add a comment about how to access additional arguments
    if (node->function.info.is_variadic) {
        fprintf(asmFile, "    ; This is a variadic function with %d fixed parameters\n", node->function.info.param_count);
        fprintf(asmFile, "    ; Variable arguments start at [bp+%d]\n", paramOffset);
        fprintf(asmFile, "    ; Use the va_XXX macros from stdarg.h to access variable arguments\n");
        fprintf(asmFile, "    ; Example: va_list args; va_start(args, last_param); value = va_arg(args, type);\n");
        
        // If we're in debug mode, add detailed stack layout information
        #ifndef QUIET_MODE
        fprintf(asmFile, "    ; Stack layout for varargs:\n");
        fprintf(asmFile, "    ; [bp+0] = Previous BP\n");
        fprintf(asmFile, "    ; [bp+2] = Return address\n");
        fprintf(asmFile, "    ; [bp+4] = First parameter\n");
        
        int offset = 4; // Start at first parameter
        for (int i = 0; i < node->function.info.param_count; i++) {
            fprintf(asmFile, "    ; [bp+%d] = Parameter %d\n", offset, i);
            offset += 2; // Assuming 2 bytes per parameter
        }
        
        fprintf(asmFile, "    ; [bp+%d] = First variable argument\n", offset);
        fprintf(asmFile, "    ; [bp+%d] = Second variable argument\n", offset + 2);
        fprintf(asmFile, "    ; ... and so on\n");
        #endif
    }
    
    // Generate code for function body
    if (node->function.body) {
        generateBlock(node->function.body);
    }
    
    // Generate function exit label
    fprintf(asmFile, "\n_%s_exit:\n", funcName);
      // Function epilogue
    if (node->function.info.is_naked) {
        fprintf(asmFile, "    ; Naked function - no epilogue generated\n");
        // No ret instruction for naked functions - user must provide it
    }
    else if (node->function.info.is_stackframe) {
        fprintf(asmFile, "    ; Restore stackframe with registers\n");
        
        // If we allocated space for locals, deallocate it here
        if (stackSize > 0) {
            fprintf(asmFile, "    add sp, %d ; Remove space for local variables\n", stackSize);
        }
        
        fprintf(asmFile, "    mov sp, bp\n");
        fprintf(asmFile, "    pop di\n");
        fprintf(asmFile, "    pop si\n");
        fprintf(asmFile, "    pop dx\n");
        fprintf(asmFile, "    pop cx\n");
        fprintf(asmFile, "    pop bx\n");
        fprintf(asmFile, "    pop bp\n");
        fprintf(asmFile, "    ret\n");
    } else {
        fprintf(asmFile, "    ; Standard function epilogue\n");
        fprintf(asmFile, "    mov sp, bp\n");
        fprintf(asmFile, "    pop bp\n");
        fprintf(asmFile, "    ret\n");
    }
      fprintf(asmFile, "\n");
    
    currentFunction = NULL;
    currentFunctionIsNaked = 0;
}

// Generate code for a block of statements
void generateBlock(ASTNode* node) {
    if (!node || node->type != NODE_BLOCK) return;
    
    ASTNode* statement = node->left;
    
    while (statement) {
        generateStatement(statement);
        statement = statement->next;
    }
}

// Generate code for a statement
void generateStatement(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_DECLARATION:
            generateVariableDeclaration(node);
            break;
              case NODE_ASSIGNMENT:
            fprintf(asmFile, "    ; Assignment statement\n");
            // Generate code for right-hand side
            if (node->assignment.op != 0 && node->left->type == NODE_IDENTIFIER) {                // Compound assignment: compute old value and RHS
                // Load current LHS value
                int varOffset = getVariableOffset(node->left->identifier);
                if (isParameter(node->left->identifier)) {
                    fprintf(asmFile, "    mov ax, [bp+%d] ; Load parameter %s for compound assignment\n", 
                            -varOffset, node->left->identifier);
                } else if (varOffset > 0) {
                    fprintf(asmFile, "    mov ax, [bp-%d] ; Load local variable %s for compound assignment\n", 
                            varOffset, node->left->identifier);
                } else {
                    // Load from global variable
                    // Get sanitized filename prefix
                    const char* filename = getCurrentSourceFilename();
                    char* prefix = (char*)malloc(strlen(filename) + 1);
                    if (prefix) {
                        strcpy(prefix, filename);
                        char* dot = strrchr(prefix, '.');
                        if (dot) *dot = '\0';
                        for (char* c = prefix; *c; c++) {
                            if (!isalnum(*c) && *c != '_') {
                                *c = '_';
                            }
                        }
                        
                        fprintf(asmFile, "    ; Loading global variable %s for compound assignment\n", node->left->identifier);
                        fprintf(asmFile, "    mov ax, [_%s_%s] ; Load global variable\n", 
                                prefix, node->left->identifier);
                        free(prefix);
                    } else {
                        fprintf(asmFile, "    ; Loading global variable %s for compound assignment\n", node->left->identifier);
                        fprintf(asmFile, "    mov ax, [_%s] ; Load global variable (fallback)\n", node->left->identifier);
                    }
                }
                fprintf(asmFile, "    push ax ; Save old value\n");
                // Evaluate RHS
                generateExpression(node->right);
                fprintf(asmFile, "    push ax ; Save RHS value\n");
                // Pop into registers: BX=rhs, AX=old
                fprintf(asmFile, "    pop bx ; RHS value\n");
                fprintf(asmFile, "    pop ax ; Old LHS value\n");
                // Apply operation
                switch (node->assignment.op) {
                    case OP_PLUS_ASSIGN:
                        fprintf(asmFile, "    add ax, bx ; +=\n");
                        break;
                    case OP_MINUS_ASSIGN:
                        fprintf(asmFile, "    sub ax, bx ; -=\n");
                        break;
                    case OP_MUL_ASSIGN:
                        fprintf(asmFile, "    imul bx ; *=\n");
                        break;                    
                        
                    case OP_DIV_ASSIGN:
                        {
                            // Check if variable is unsigned
                            TypeInfo* typeInfo = getTypeInfo(node->left->identifier);
                            if (typeInfo && (typeInfo->type == TYPE_UNSIGNED_INT || 
                                          typeInfo->type == TYPE_UNSIGNED_SHORT ||
                                          typeInfo->type == TYPE_UNSIGNED_CHAR)) {
                                fprintf(asmFile, "    xor dx, dx ; Zero extend AX into DX:AX for unsigned division\n");
                                fprintf(asmFile, "    div bx ; /= (unsigned)\n");
                            } else {
                                fprintf(asmFile, "    cwd ; Sign extend AX into DX:AX for division\n");
                                fprintf(asmFile, "    idiv bx ; /=\n");
                            }
                        }
                        break;                    case OP_MOD_ASSIGN:
                        {
                            // Check if variable is unsigned
                            TypeInfo* typeInfo = getTypeInfo(node->left->identifier);
                            if (typeInfo && (typeInfo->type == TYPE_UNSIGNED_INT || 
                                          typeInfo->type == TYPE_UNSIGNED_SHORT ||
                                          typeInfo->type == TYPE_UNSIGNED_CHAR)) {
                                fprintf(asmFile, "    xor dx, dx ; Zero extend AX into DX:AX for unsigned mod\n");
                                fprintf(asmFile, "    div bx ; (unsigned)\n");
                                fprintf(asmFile, "    mov ax, dx ; remainder in DX\n");
                            } else {
                                fprintf(asmFile, "    cwd ; Sign extend AX into DX:AX for mod\n");
                                fprintf(asmFile, "    idiv bx ;\n");
                                fprintf(asmFile, "    mov ax, dx ; remainder in DX\n");
                            }
                        }
                        break;
                    case OP_LEFT_SHIFT_ASSIGN:
                        fprintf(asmFile, "    mov cx, bx ; Set shift count in CX\n");
                        fprintf(asmFile, "    shl ax, cl ; Shift left (<<= operator)\n");
                        break;
                    case OP_RIGHT_SHIFT_ASSIGN:
                        fprintf(asmFile, "    mov cx, bx ; Set shift count in CX\n");
                        fprintf(asmFile, "    sar ax, cl ; Shift right (arithmetic) (>>= operator)\n");
                        break;
                    default:
                        break;
                }
            } else if (node->assignment.op == 0) {
                // Simple assignment: evaluate RHS
                generateExpression(node->right);
            } else {
                // Other targets or ops: fallback to simple RHS
                generateExpression(node->right);
            }            // Store result in AX to the left-hand side
            if (node->left->type == NODE_IDENTIFIER) {
                // Check if this is a parameter or local variable
                int varOffset = getVariableOffset(node->left->identifier);
                if (isParameter(node->left->identifier)) {
                    // Parameters have positive offsets from bp
                    fprintf(asmFile, "    mov [bp+%d], ax ; Store in parameter %s\n", 
                            -varOffset, node->left->identifier);
                } else if (varOffset > 0) {
                    // Local variables have negative offsets from bp
                    fprintf(asmFile, "    mov [bp-%d], ax ; Store in local variable %s\n", 
                            varOffset, node->left->identifier);
                } else {
                    // Must be a global variable
                    // Get sanitized filename prefix
                    const char* filename = getCurrentSourceFilename();
                    char* prefix = (char*)malloc(strlen(filename) + 1);
                    if (prefix) {
                        strcpy(prefix, filename);
                        char* dot = strrchr(prefix, '.');
                        if (dot) *dot = '\0';
                        for (char* c = prefix; *c; c++) {
                            if (!isalnum(*c) && *c != '_') {
                                *c = '_';
                            }
                        }
                        
                        fprintf(asmFile, "    mov [_%s_%s], ax ; Store in global variable %s\n", 
                                prefix, node->left->identifier, node->left->identifier);
                        free(prefix);
                    } else {
                        // Fallback if memory allocation fails
                        fprintf(asmFile, "    mov [_%s], ax ; Store in global variable %s (fallback)\n", 
                                node->left->identifier, node->left->identifier);
                    }
                }
            }else if (node->left->type == NODE_UNARY_OP && node->left->unary_op.op == UNARY_DEREFERENCE) {
                // Pointer assignment (e.g., *ptr = value)
                // Save the right-hand side result temporarily
                fprintf(asmFile, "    push ax ; Save right-hand side value\n");
                
                // Generate code to evaluate the pointer expression
                generateExpression(node->left->right);
                
                // Check if this is a far pointer (segment in DX, offset in AX)
                if (node->left->right->type == NODE_LITERAL && node->left->right->literal.data_type == TYPE_FAR_POINTER) {
                    fprintf(asmFile, "    ; Far pointer assignment\n");
                    fprintf(asmFile, "    push ds ; Save current DS\n");
                    fprintf(asmFile, "    mov bx, ax ; Move offset to BX\n");
                    fprintf(asmFile, "    mov ds, dx ; Set DS to segment\n");
                    fprintf(asmFile, "    pop ax ; Restore right-hand side value\n");
                    
                    // Get type info to determine proper store size
                    TypeInfo* typeInfo = getTypeInfoFromExpression(node->left->right);
                    if (typeInfo && (typeInfo->type == TYPE_CHAR || 
                                    typeInfo->type == TYPE_UNSIGNED_CHAR || 
                                    typeInfo->type == TYPE_BOOL)) {
                        fprintf(asmFile, "    mov [bx], al ; Store byte value through far pointer\n");
                    } else {
                        fprintf(asmFile, "    mov [bx], ax ; Store word value through far pointer\n");
                    }
                    
                    fprintf(asmFile, "    pop ds ; Restore DS\n");
                } else {
                    // Regular near pointer
                    fprintf(asmFile, "    mov bx, ax ; Move pointer address to BX\n");
                    
                    // Restore the value and store through the pointer
                    fprintf(asmFile, "    pop ax ; Restore right-hand side value\n");
                    
                    // Get type info to determine proper store size
                    TypeInfo* typeInfo = getTypeInfoFromExpression(node->left->right);
                    if (typeInfo && (typeInfo->type == TYPE_CHAR || 
                                    typeInfo->type == TYPE_UNSIGNED_CHAR || 
                                    typeInfo->type == TYPE_BOOL)) {
                        fprintf(asmFile, "    mov [bx], al ; Store byte value through pointer\n");
                    } else {
                        fprintf(asmFile, "    mov [bx], ax ; Store word value through pointer\n");
                    }
                }
            }
            else {
                reportWarning(-1, "Unsupported assignment target");
            }
            break;
            
        case NODE_RETURN:
            generateReturnStatement(node);
            break;
            
        case NODE_EXPRESSION:
            // Generate code for the expression, but we don't need the result
            generateExpression(node->left);
            break;
              case NODE_ASM_BLOCK:
            generateAsmBlock(node);
            break;
            
        case NODE_ASM:
            generateAsmStmt(node);
            break;
            
        case NODE_FOR:
            generateForLoop(node);
            break;
              case NODE_WHILE:
            generateWhileLoop(node);
            break;
            
        case NODE_DO_WHILE:
            generateDoWhileLoop(node);
            break;
            
        case NODE_IF:
            generateIfStatement(node);
            break;
              default:
            reportWarning(-1, "Unsupported statement type: %d", node->type);
            break;
    }
}

// Generate code for variable declaration
void generateVariableDeclaration(ASTNode* node) {
    if (!node || node->type != NODE_DECLARATION) return;
    
    // Check if this is an array with a fixed size
    if (node->declaration.type_info.is_array && node->declaration.type_info.array_size > 0) {
        // Check if this array has initializers
        if (node->declaration.initializer) {
            fprintf(asmFile, "    ; Array variable with initializers: %s[%d]\n", 
                    node->declaration.var_name, 
                    node->declaration.type_info.array_size);
            // Register the array with its initializers for generation at the right location
            generateArrayWithInitializers(node);
        } else {
            // Register the array for generation with zeros
            fprintf(asmFile, "    ; Array variable without initializers: %s[%d]\n", 
                    node->declaration.var_name, 
                    node->declaration.type_info.array_size);
            addArrayDeclaration(node->declaration.var_name,
                                node->declaration.type_info.array_size,
                                node->declaration.type_info.type,
                                currentFunction);
        }
        
        // Set up a pointer to the array
        fprintf(asmFile, "    ; Setting up pointer to array %s[%d]\n", 
                node->declaration.var_name, 
                node->declaration.type_info.array_size);
                
        // Get sanitized filename prefix
        const char* filename = getCurrentSourceFilename();
        char* prefix = (char*)malloc(strlen(filename) + 1);
        if (prefix) {
            strcpy(prefix, filename);
            char* dot = strrchr(prefix, '.');
            if (dot) *dot = '\0';
            for (char* c = prefix; *c; c++) {
                if (!isalnum(*c) && *c != '_') {
                    *c = '_';
                }
            }
            
            // Get the array index from addArrayDeclaration (should be the most recent one)
            int arrIndex = arrayCount - 1;
            
            // Generate pointer to array with full unique label (file_function_name_index)
            fprintf(asmFile, "    mov ax, _%s_%s_%s_%d ; Address of array\n", 
                    prefix, currentFunction ? currentFunction : "global", 
                    node->declaration.var_name, arrIndex);
            free(prefix);
        } else {
            // Fallback with at least function name and index if we couldn't get a prefix
            int arrIndex = arrayCount - 1;
            fprintf(asmFile, "    mov ax, _%s_%s_%d ; Address of array (fallback)\n", 
                    currentFunction ? currentFunction : "global", 
                    node->declaration.var_name, arrIndex);
        }
        fprintf(asmFile, "    push ax ; Store pointer to array\n");
        
        // Add to local variable table
        addLocalVariable(node->declaration.var_name, 2);  // Pointer size is 2 bytes
        
        return;
    }
    
    // For non-array local variables, proceed as before
    fprintf(asmFile, "    ; Local variable declaration: %s\n", node->declaration.var_name);    // Determine variable size based on type
    int varSize = 2; // Default for int, short
    if (node->declaration.type_info.type == TYPE_CHAR || 
        node->declaration.type_info.type == TYPE_UNSIGNED_CHAR) {
        varSize = 1;
    } else if (node->declaration.type_info.type == TYPE_LONG || 
              node->declaration.type_info.type == TYPE_UNSIGNED_LONG) {
        varSize = 4; // 32-bit size for long types
    } else if (node->declaration.type_info.type == TYPE_STRUCT && 
              node->declaration.type_info.struct_info) {
        // For structs, use the calculated struct size
        varSize = node->declaration.type_info.struct_info->size;
    }    // If there's an initializer, generate code for the assignment
    if (node->declaration.initializer) {
        // For struct initializers, we need to handle each member
        if (node->declaration.type_info.type == TYPE_STRUCT && 
            node->declaration.type_info.struct_info) {
            
            StructInfo* structInfo = node->declaration.type_info.struct_info;
            
            // Check if we have a compound initializer (brace-enclosed)
            if (node->declaration.initializer->next) {
                // This is a compound initializer with values for each field
                ASTNode* initValue = node->declaration.initializer;
                StructMember* member = structInfo->members;
                
                // Calculate the total struct size to reserve space
                int structSize = structInfo->size;
                
                // Reserve space for the struct on stack
                if (structSize >= 2) {
                    fprintf(asmFile, "    sub sp, %d  ; Reserve space for struct\n", structSize);
                } else {
                    fprintf(asmFile, "    push 0      ; Reserve space for small struct\n");
                }
                
                // Initialize each member with the corresponding initializer
                int offset = 0;
                while (initValue && member) {
                    // Generate the member initializer value
                    generateExpression(initValue);
                    
                    // Determine base pointer and offset for storing member
                    int memberOffset = member->offset;
                    
                    // Store the value in the appropriate member location
                    if (member->type_info.type == TYPE_CHAR || 
                        member->type_info.type == TYPE_UNSIGNED_CHAR || 
                        member->type_info.type == TYPE_BOOL) {
                        fprintf(asmFile, "    mov byte [bp-%d-%d], al  ; Initialize struct member %s\n", 
                                stackSize, memberOffset, member->name);
                    } else if (member->type_info.type == TYPE_LONG || 
                              member->type_info.type == TYPE_UNSIGNED_LONG) {
                        // Assume 32-bit value in dx:ax
                        fprintf(asmFile, "    mov word [bp-%d-%d], ax  ; Initialize struct member %s low word\n", 
                                stackSize, memberOffset, member->name);
                        fprintf(asmFile, "    mov word [bp-%d-%d], dx  ; Initialize struct member %s high word\n", 
                                stackSize, memberOffset + 2, member->name);
                    } else {
                        // Default case for 16-bit values
                        fprintf(asmFile, "    mov word bp-%d-%d], ax  ; Initialize struct member %s\n", 
                                stackSize, memberOffset, member->name);
                    }
                    
                    // Move to next member and initializer
                    member = member->next;
                    initValue = initValue->next;
                }
            } else {
                // Single-value initializer - not directly supported for structs
                // Just reserve space
                fprintf(asmFile, "    ; Warning: Single value initializer not supported for struct, leaving uninitialized\n");
                
                int structSize = structInfo->size;
                int wordsToPush = (structSize + 1) / 2; // Round up to nearest word
                
                for (int i = 0; i < wordsToPush; i++) {
                    fprintf(asmFile, "    push 0 ; Uninitialized struct space\n");
                }
            }
        }
        // For regular values
        else {
            // Generate the value
            generateExpression(node->declaration.initializer);
            
            // For long values, need to handle 32-bit initialization
            if (node->declaration.type_info.type == TYPE_LONG || 
                node->declaration.type_info.type == TYPE_UNSIGNED_LONG) {
                // For now, we'll initialize with lower 16 bits in AX and assume upper 16 bits are zero
                // This would need to be expanded for full 32-bit literal support
                fprintf(asmFile, "    push 0 ; Push high word (upper 16 bits)\n");
                fprintf(asmFile, "    push ax ; Push low word (lower 16 bits)\n");
            } else {
                // For regular values, just push AX
                fprintf(asmFile, "    push ax ; Initialize local variable\n");
            }
        }
    } else {
        // Reserve space by pushing zeros
        if (node->declaration.type_info.type == TYPE_STRUCT && 
            node->declaration.type_info.struct_info) {
            // For structs, push enough zeros to reserve the required space
            int structSize = node->declaration.type_info.struct_info->size;
            int wordsToPush = (structSize + 1) / 2; // Round up to nearest word
            
            fprintf(asmFile, "    ; Reserving %d bytes for uninit struct %s\n", 
                   structSize, node->declaration.var_name);
            
            for (int i = 0; i < wordsToPush; i++) {
                fprintf(asmFile, "    push 0 ; Uninitialized struct space\n");
            }
        }
        else if (node->declaration.type_info.type == TYPE_LONG || 
                node->declaration.type_info.type == TYPE_UNSIGNED_LONG) {
            // For 32-bit long, push two 16-bit zeros
            fprintf(asmFile, "    push 0 ; Uninitialized long variable (high word)\n");
            fprintf(asmFile, "    push 0 ; Uninitialized long variable (low word)\n");
        } else {
            // Just reserve space by pushing a zero
            fprintf(asmFile, "    push 0 ; Uninitialized local variable\n");
        }
    }
    
    // Add to local variable table
    addLocalVariable(node->declaration.var_name, varSize);
}

// Generate code for global variable declaration
void generateGlobalDeclaration(ASTNode* node) {
    if (!node || node->type != NODE_DECLARATION) return;
    
    // Check if this is an array with a fixed size
    if (node->declaration.type_info.is_array && node->declaration.type_info.array_size > 0) {
        // Register the array for generation at the proper location
        if (node->declaration.initializer) {
            // Register array with initializers
            generateArrayWithInitializers(node);
        } else {
            // Register array without initializers
            addArrayDeclaration(node->declaration.var_name,
                               node->declaration.type_info.array_size,
                               node->declaration.type_info.type,
                               "global"); // Global function context
        }
        
        // No code is emitted here - arrays will be generated at the marker or at the end
        return;
    }
    
    // Add global declaration to be generated at the _NCC_GLOBAL_LOC marker if present
    addGlobalDeclaration(node);
}



// Generate all collected global variables
// Generate code for an expression
void generateExpression(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_STRUCT_DEF:
            // No code generation needed for struct definitions
            // Struct definitions only affect the type system
            break;

        case NODE_MEMBER_ACCESS:
            {
                TypeInfo* baseType = getTypeInfoFromExpression(node->left);
                if (!baseType) {
                    reportError(-1, "Cannot access member of unknown type");
                    return;
                }
                
                // Handle struct pointer dereference (->)
                if (node->member_access.op == OP_ARROW) {
                    // Ensure we're working with a struct pointer
                    if (baseType->type != TYPE_STRUCT || !baseType->is_pointer) {
                        reportError(-1, "Cannot use -> operator on non-struct-pointer");
                        return;
                    }
                    
                    // Generate code to load the struct pointer into BX
                    generateExpression(node->left);
                    fprintf(asmFile, "    mov bx, ax    ; Load struct pointer into BX\n");
                    
                    // Calculate the member offset within the struct
                    int offset = getMemberOffset(baseType->struct_info, node->member_access.member_name);
                    if (offset < 0) {
                        reportError(-1, "Struct %s has no member named %s", 
                                  baseType->struct_info->name, node->member_access.member_name);
                        return;
                    }
                    
                    // Get the member's type to determine load size
                    TypeInfo* memberType = getMemberType(baseType->struct_info, node->member_access.member_name);
                    
                    // Load the member value from the pointer + offset into AX
                    if (memberType->type == TYPE_CHAR || memberType->type == TYPE_UNSIGNED_CHAR || memberType->type == TYPE_BOOL) {
                        // Byte-sized member
                        fprintf(asmFile, "    mov al, [bx+%d]  ; Load byte-sized struct member\n", offset);
                        fprintf(asmFile, "    xor ah, ah       ; Clear high byte for byte-sized member\n");
                    } else {
                        // Word-sized or larger member (handle larger types separately)
                        fprintf(asmFile, "    mov ax, [bx+%d]  ; Load struct member\n", offset);
                    }
                } 
                // Handle direct struct access (.)
                else if (node->member_access.op == OP_DOT) {
                    // Ensure we're working with a struct
                    if (baseType->type != TYPE_STRUCT) {
                        reportError(-1, "Cannot use . operator on non-struct type");
                        return;
                    }
                    
                    // Generate code to get the struct address into BX
                    // For locals/parameters, we already have memory access
                    // For globals, we need to use the symbol address
                    generateAddressOf(node->left);
                    fprintf(asmFile, "    mov bx, ax    ; Load struct address into BX\n");
                    
                    // Calculate the member offset within the struct
                    int offset = getMemberOffset(baseType->struct_info, node->member_access.member_name);
                    if (offset < 0) {
                        reportError(-1, "Struct %s has no member named %s", 
                                  baseType->struct_info->name, node->member_access.member_name);
                        return;
                    }
                    
                    // Get the member's type to determine load size
                    TypeInfo* memberType = getMemberType(baseType->struct_info, node->member_access.member_name);
                    
                    // Load the member value from the address + offset into AX
                    if (memberType->type == TYPE_CHAR || memberType->type == TYPE_UNSIGNED_CHAR || memberType->type == TYPE_BOOL) {
                        // Byte-sized member
                        fprintf(asmFile, "    mov al, [bx+%d]  ; Load byte-sized struct member\n", offset);
                        fprintf(asmFile, "    xor ah, ah       ; Clear high byte for byte-sized member\n");
                    } else {
                        // Word-sized or larger member (handle larger types separately)
                        fprintf(asmFile, "    mov ax, [bx+%d]  ; Load struct member\n", offset);
                    }
                }
                break;
            }
        case NODE_LITERAL:
            // Handle different literal types
            if (node->literal.data_type == TYPE_FAR_POINTER) {
                // Load segment into dx, offset into ax for far pointers
                fprintf(asmFile, "    mov dx, 0x%04X ; Segment\n", node->literal.segment);
                fprintf(asmFile, "    mov ax, 0x%04X ; Offset\n", node->literal.offset);
            } else if (node->literal.data_type == TYPE_CHAR && node->literal.string_value) {                // Add string to the string literals table and get its index
                int strIndex = addStringLiteral(node->literal.string_value);
                
                if (strIndex >= 0) {
                    // Get sanitized filename prefix
                    const char* filename = getCurrentSourceFilename();
                    char* prefix = (char*)malloc(strlen(filename) + 1);
                    if (prefix) {
                        strcpy(prefix, filename);
                        char* dot = strrchr(prefix, '.');
                        if (dot) *dot = '\0';
                        for (char* c = prefix; *c; c++) {
                            if (!isalnum(*c) && *c != '_') {
                                *c = '_';
                            }
                        }
                        
                        // Load the address of the string into AX
                        fprintf(asmFile, "    ; String literal: %s\n", node->literal.string_value);
                        fprintf(asmFile, "    mov ax, %s_string_%d ; Address of string\n", prefix, strIndex);
                        free(prefix);
                    } else {
                        fprintf(asmFile, "    ; String literal: %s\n", node->literal.string_value);
                        fprintf(asmFile, "    mov ax, string_%d ; Address of string (fallback)\n", strIndex);
                    }
                } else {
                    fprintf(asmFile, "    ; Error processing string literal: %s\n", 
                            node->literal.string_value ? node->literal.string_value : "(null)");
                    fprintf(asmFile, "    mov ax, 0 ; Using null pointer as fallback\n");
                }            } else if (node->literal.data_type == TYPE_CHAR && !node->literal.string_value) {
                // For character literals, load the ASCII value into al (8-bit)
                // Then zero-extend to ax for consistent value handling
                fprintf(asmFile, "    mov al, %d ; Load character value (ASCII: '%c')\n", 
                       (unsigned char)node->literal.char_value, node->literal.char_value);
                fprintf(asmFile, "    mov ah, 0 ; Zero-extend to 16-bit\n");
            } else if (node->literal.data_type == TYPE_BOOL) {
                // For boolean literals, load 0 or 1 into ax
                fprintf(asmFile, "    mov ax, %d ; Load boolean value (%s)\n", 
                       node->literal.int_value, node->literal.int_value ? "true" : "false");
            }            else if (node->literal.data_type == TYPE_LONG || node->literal.data_type == TYPE_UNSIGNED_LONG) {
                // For 32-bit long literals, load low 16 bits into AX and high 16 bits into DX
                // Note: This assumes the literal can fit into 32 bits
                int lowWord = node->literal.int_value & 0xFFFF;
                int highWord = (node->literal.int_value >> 16) & 0xFFFF;
                
                fprintf(asmFile, "    mov ax, %d ; Load long literal (low word)\n", lowWord);
                fprintf(asmFile, "    mov dx, %d ; Load long literal (high word)\n", highWord);
            } else {
                // For regular numbers, load the value into ax
                fprintf(asmFile, "    mov ax, %d ; Load literal\n", node->literal.int_value);
            }
            break;
          case NODE_IDENTIFIER:
            // Check if this is a parameter or local variable
            if (isParameter(node->identifier)) {
                // Check if it's a long type
                TypeInfo* typeInfo = getTypeInfo(node->identifier);
                if (typeInfo && (typeInfo->type == TYPE_LONG || typeInfo->type == TYPE_UNSIGNED_LONG)) {
                    // For 32-bit types, load low word into AX and high word into DX
                    fprintf(asmFile, "    ; Loading long parameter %s\n", node->identifier);
                    fprintf(asmFile, "    mov ax, [bp+%d] ; Load low word\n", 
                            -getVariableOffset(node->identifier));
                    fprintf(asmFile, "    mov dx, [bp+%d] ; Load high word\n", 
                            -getVariableOffset(node->identifier) + 2);
                } else {
                    // Parameters have positive offsets from bp
                    fprintf(asmFile, "    mov ax, [bp+%d] ; Load parameter %s\n", 
                            -getVariableOffset(node->identifier), node->identifier);
                }
            } else {
            // Local variables have negative offsets from bp
                 int varOffset = getVariableOffset(node->identifier);
                   if (varOffset == 0) {
                    // Global variable or array
                    const char* filename = getCurrentSourceFilename();
                    char* prefix = (char*)malloc(strlen(filename) + 1);
                    if (prefix) {
                        strcpy(prefix, filename);
                        char* dot = strrchr(prefix, '.'); if (dot) *dot = '\0';
                        for (char* c = prefix; *c; c++) if (!isalnum(*c) && *c != '_') *c = '_';
                    }                    // Determine if this global is an array
                    TypeInfo* tinfo = getTypeInfo(node->identifier);
                    if (tinfo && tinfo->is_array) {
                        extern int arrayCount; extern char** arrayNames; extern char** arrayFunctions;
                        int idx = -1;
                        for (int ai = 0; ai < arrayCount; ai++) {
                            if (strcmp(arrayNames[ai], node->identifier) == 0 && strcmp(arrayFunctions[ai], "global") == 0) {
                                idx = ai; break;
                            }
                        }                        if (idx >= 0 && prefix) {
                            fprintf(asmFile, "    mov ax, _%s_global_%s_%d ; Address of global array\n", prefix, node->identifier, idx);
                            free(prefix);
                            break;
                        }
                    }
                    // Fallback for scalar global
                    if (prefix) {
                        fprintf(asmFile, "    ; Loading global variable %s\n", node->identifier);
                        fprintf(asmFile, "    mov ax, [_%s_%s] ; Load global variable\n", prefix, node->identifier);
                        free(prefix);
                    } else {
                        fprintf(asmFile, "    ; Loading global variable %s\n", node->identifier);
                        fprintf(asmFile, "    mov ax, [_%s] ; Load global variable (fallback)\n", node->identifier);
                    }
                 } else {
                     // Check if it's a long type
                     TypeInfo* typeInfo = getTypeInfo(node->identifier);
                     if (typeInfo && (typeInfo->type == TYPE_LONG || typeInfo->type == TYPE_UNSIGNED_LONG)) {
                         // For 32-bit types, load low word into AX and high word into DX
                         fprintf(asmFile, "    ; Loading long variable %s\n", node->identifier);
                         fprintf(asmFile, "    mov ax, [bp-%d] ; Load low word\n", varOffset);
                         fprintf(asmFile, "    mov dx, [bp-%d] ; Load high word\n", varOffset - 2);
                     } else {
                         // Ensure we use word-aligned offsets for local variables
                         fprintf(asmFile, "    mov ax, [bp-%d] ; Load local variable %s\n", 
                                 varOffset, node->identifier);
                     }
                 }
             }
             break;
              case NODE_BINARY_OP:
            // Generate code for binary operation
            generateBinaryOp(node);
            break;
              case NODE_UNARY_OP:
            // Generate code for unary operation
            generateUnaryOp(node);
            break;
            
        case NODE_TERNARY:
            // Generate code for ternary conditional expression
            generateTernaryExpression(node);
            break;
            
        case NODE_CALL:
            // Generate function call
            generateFunctionCall(node);
            break;
        case NODE_ASSIGNMENT:
            // Assignment used as expression: generate code and leave assigned value in AX
            generateStatement(node);
            break;        default:
            reportWarning(-1, "Unsupported expression type: %d", node->type);
            break;
    }
}

// Generate code for binary operations
void generateBinaryOp(ASTNode* node) {
    // Short-circuit logical operators
    if (node->operation.op == OP_LAND) {
        char* falseLabel = generateLabel("land_false");
        char* endLabel = generateLabel("land_end");
        generateExpression(node->left);
        fprintf(asmFile, "    test ax, ax ; logical AND left test\n");
        fprintf(asmFile, "    jz %s ; left false, skip right\n", falseLabel);
        generateExpression(node->right);
        fprintf(asmFile, "    test ax, ax ; logical AND right test\n");
        fprintf(asmFile, "    jz %s ; right false, result false\n", falseLabel);
        fprintf(asmFile, "    mov ax, 1 ; both true -> true\n");
        fprintf(asmFile, "    jmp %s\n", endLabel);
        fprintf(asmFile, "%s:\n", falseLabel);
        fprintf(asmFile, "    mov ax, 0 ; false\n");
        fprintf(asmFile, "%s:\n", endLabel);
        free(falseLabel);
        free(endLabel);
        return;
    }
    if (node->operation.op == OP_LOR) {
        char* trueLabel = generateLabel("lor_true");
        char* endLabel = generateLabel("lor_end");
        generateExpression(node->left);
        fprintf(asmFile, "    test ax, ax ; logical OR left test\n");
        fprintf(asmFile, "    jnz %s ; left true, result true\n", trueLabel);
        generateExpression(node->right);
        fprintf(asmFile, "    test ax, ax ; logical OR right test\n");
        fprintf(asmFile, "    jnz %s ; right true -> true\n", trueLabel);
        fprintf(asmFile, "    mov ax, 0 ; both false -> false\n");
        fprintf(asmFile, "    jmp %s\n", endLabel);
        fprintf(asmFile, "%s:\n", trueLabel);
        fprintf(asmFile, "    mov ax, 1 ; true\n");
        fprintf(asmFile, "%s:\n", endLabel);
        free(trueLabel);
        free(endLabel);
        return;    }
    
    // Check if we're operating on long types
    TypeInfo* leftType = getTypeInfoFromExpression(node->left);
    TypeInfo* rightType = getTypeInfoFromExpression(node->right);
    int isLongOperation = (leftType && (leftType->type == TYPE_LONG || leftType->type == TYPE_UNSIGNED_LONG)) ||
                        (rightType && (rightType->type == TYPE_LONG || rightType->type == TYPE_UNSIGNED_LONG));
                        
    if (isLongOperation) {
        // For 32-bit operations, we need to handle the upper 16 bits (DX register)
        fprintf(asmFile, "    ; 32-bit long operation detected\n");
        
        // Generate left operand (result in DX:AX)
        generateExpression(node->left);
        fprintf(asmFile, "    push dx ; Save left operand high word\n");
        fprintf(asmFile, "    push ax ; Save left operand low word\n");
        
        // Generate right operand (result in DX:AX)
        generateExpression(node->right);
        fprintf(asmFile, "    mov cx, dx ; Right operand high word to CX\n");
        fprintf(asmFile, "    mov bx, ax ; Right operand low word to BX\n");
        
        // Restore left operand to DX:AX
        fprintf(asmFile, "    pop ax ; Restore left operand low word\n");
        fprintf(asmFile, "    pop dx ; Restore left operand high word\n");
    } else {
        // Regular 16-bit operation
        // Generate left operand and save
        generateExpression(node->left);
        fprintf(asmFile, "    push ax ; Save left operand\n");
        
        // Generate right operand, result in ax
        generateExpression(node->right);
        
        // Move right operand to bx and restore left operand to ax
        fprintf(asmFile, "    mov bx, ax ; Right operand to bx\n");
        fprintf(asmFile, "    pop ax ; Restore left operand\n");
    }    // Perform operation based on operator type
    switch (node->operation.op) {
        case OP_ADD:
            // Check if this is a long operation
            if (isLongOperation) {
                fprintf(asmFile, "    ; 32-bit addition\n");
                fprintf(asmFile, "    add ax, bx ; Add low words\n");
                fprintf(asmFile, "    adc dx, cx ; Add high words with carry\n");
            }
            // Check for pointer arithmetic (pointer + integer)
            else if (isPointerType(node->left)) {
                // Get element size for scaling
                TypeInfo* typeInfo = getTypeInfoFromExpression(node->left);
                if (typeInfo) {
                    int elemSize = (typeInfo->type == TYPE_CHAR || 
                                   typeInfo->type == TYPE_UNSIGNED_CHAR || 
                                   typeInfo->type == TYPE_BOOL) ? 1 : 2;
                    
                    if (elemSize > 1) {
                        // Scale the offset by element size
                        fprintf(asmFile, "    ; Pointer arithmetic: scale by element size %d\n", elemSize);
                        fprintf(asmFile, "    shl bx, 1 ; Scale index by 2 for word elements\n");
                    }
                }
                fprintf(asmFile, "    add ax, bx ; Addition\n");
            } else if (isPointerType(node->right)) {
                // Case of integer + pointer (need to scale integer)
                TypeInfo* typeInfo = getTypeInfoFromExpression(node->right);
                if (typeInfo) {
                    int elemSize = (typeInfo->type == TYPE_CHAR || 
                                   typeInfo->type == TYPE_UNSIGNED_CHAR || 
                                   typeInfo->type == TYPE_BOOL) ? 1 : 2;
                    
                    if (elemSize > 1) {
                        // Scale the offset by element size
                        fprintf(asmFile, "    ; Pointer arithmetic: scale by element size %d\n", elemSize);
                        fprintf(asmFile, "    shl ax, 1 ; Scale index by 2 for word elements\n");
                    }
                    
                    // Swap operands to ensure pointer is in AX
                    fprintf(asmFile, "    xchg ax, bx ; Swap to put pointer in AX\n");
                }
                fprintf(asmFile, "    add ax, bx ; Addition\n");
            } else {
                fprintf(asmFile, "    add ax, bx ; Addition\n");
            }
            break;
              case OP_SUB:
            // Check if this is a long operation
            if (isLongOperation) {
                fprintf(asmFile, "    ; 32-bit subtraction\n");
                fprintf(asmFile, "    sub ax, bx ; Subtract low words\n");
                fprintf(asmFile, "    sbb dx, cx ; Subtract high words with borrow\n");
            }
            // Check for pointer arithmetic (pointer - integer or pointer - pointer)
            else if (isPointerType(node->left)) {
                if (isPointerType(node->right)) {
                    // Pointer - Pointer: yields a count of elements between pointers
                    fprintf(asmFile, "    ; Pointer difference\n");
                    fprintf(asmFile, "    sub ax, bx ; Calculate raw byte difference\n");
                    
                    // Divide by element size to get element count
                    TypeInfo* typeInfo = getTypeInfoFromExpression(node->left);
                    if (typeInfo && (typeInfo->type != TYPE_CHAR && 
                                    typeInfo->type != TYPE_UNSIGNED_CHAR && 
                                    typeInfo->type != TYPE_BOOL)) {
                        fprintf(asmFile, "    sar ax, 1 ; Divide by 2 for word elements\n");
                    }
                } else {
                    // Pointer - Integer: scale integer by element size
                    TypeInfo* typeInfo = getTypeInfoFromExpression(node->left);
                    if (typeInfo && (typeInfo->type != TYPE_CHAR && 
                                    typeInfo->type != TYPE_UNSIGNED_CHAR && 
                                    typeInfo->type != TYPE_BOOL)) {
                        fprintf(asmFile, "    ; Pointer arithmetic: scale by element size\n");
                        fprintf(asmFile, "    shl bx, 1 ; Scale index by 2 for word elements\n");
                    }
                    fprintf(asmFile, "    sub ax, bx ; Subtraction\n");
                }
            } else {
                fprintf(asmFile, "    sub ax, bx ; Subtraction\n");
            }
            break;
              case OP_MUL:
            if (isLongOperation) {
                // 32-bit multiplication is complex - needs multiple steps
                fprintf(asmFile, "    ; 32-bit multiplication\n");
                
                // First save our operands 
                fprintf(asmFile, "    push dx ; Save left high word\n");
                fprintf(asmFile, "    push ax ; Save left low word\n");
                fprintf(asmFile, "    push cx ; Save right high word\n");
                fprintf(asmFile, "    push bx ; Save right low word\n");
                
                // First multiplication: left-low * right-low (result in DX:AX)
                fprintf(asmFile, "    mov ax, [esp+2] ; Load left low word\n");
                fprintf(asmFile, "    mov bx, [esp] ; Load right low word\n");
                fprintf(asmFile, "    mul bx ; Unsigned multiply, result in DX:AX\n");
                fprintf(asmFile, "    push dx ; Save high result of low*low\n");
                fprintf(asmFile, "    push ax ; Save low result\n");
                
                // Second mult: left-high * right-low (result added to high word)
                fprintf(asmFile, "    mov ax, [esp+8] ; Load left high word\n");
                fprintf(asmFile, "    mov bx, [esp+4] ; Load right low word\n");
                fprintf(asmFile, "    mul bx ; Multiply, result in DX:AX\n");
                fprintf(asmFile, "    add [esp+2], ax ; Add to high word of result\n");
                
                // Third mult: left-low * right-high (result added to high word)
                fprintf(asmFile, "    mov ax, [esp+6] ; Load left low word\n");
                fprintf(asmFile, "    mov bx, [esp+4] ; Load right high word\n");
                fprintf(asmFile, "    mul bx ; Multiply, result in DX:AX\n");
                fprintf(asmFile, "    add [esp+2], ax ; Add to high word of result\n");
                
                // Get final result - ignoring overflows from high word * high word
                fprintf(asmFile, "    pop ax ; Get low word of result\n");
                fprintf(asmFile, "    pop dx ; Get high word of result\n");
                
                // Clean up the stack
                fprintf(asmFile, "    add esp, 4 ; Clean up saved values\n");
            } else {
                fprintf(asmFile, "    imul bx ; Multiplication (signed)\n");
            }
            break;        case OP_DIV:
            {
                if (isLongOperation) {
                    // 32-bit division is complex
                    fprintf(asmFile, "    ; 32-bit division\n");
                    
                    TypeInfo* typeInfo = getTypeInfoFromExpression(node->left);
                    if (typeInfo && (typeInfo->type == TYPE_UNSIGNED_LONG)) {
                        // We already have dividend in DX:AX and divisor in CX:BX
                        // Use a custom division routine for 32-bit / 32-bit
                        
                        // For now, we'll only support division by 16-bit divisors
                        // which is the common case when dividing by constants
                        fprintf(asmFile, "    ; 32-bit unsigned division by 16-bit divisor\n");
                        fprintf(asmFile, "    push cx ; Save divisor high word\n");
                        
                        // Check if high word of divisor is zero
                        fprintf(asmFile, "    test cx, cx ; Check if high word of divisor is zero\n");
                        fprintf(asmFile, "    jnz div32_complex ; Jump if we need a complex division\n");
                        
                        // Simple case: 32-bit / 16-bit = 16-bit
                        fprintf(asmFile, "    div bx ; Divide DX:AX by BX\n");
                        fprintf(asmFile, "    xor dx, dx ; Clear high word of result\n");
                        fprintf(asmFile, "    jmp div32_done\n");
                        
                        // Complex case handler stub - would need a full algorithm
                        fprintf(asmFile, "div32_complex:\n");
                        fprintf(asmFile, "    ; Complex 32-bit division not fully implemented\n");
                        fprintf(asmFile, "    ; Returning dividend as result\n");
                        
                        fprintf(asmFile, "div32_done:\n");
                        fprintf(asmFile, "    add sp, 2 ; Clean up stack\n");
                    } else {
                        // Signed division - similar approach but using IDIV
                        fprintf(asmFile, "    ; 32-bit signed division\n");
                        fprintf(asmFile, "    push cx ; Save divisor high word\n");
                        
                        // Check if high word of divisor is zero
                        fprintf(asmFile, "    test cx, cx ; Check if high word of divisor is zero\n");
                        fprintf(asmFile, "    jnz idiv32_complex ; Jump if we need a complex division\n");
                        
                        // Simple case: 32-bit / 16-bit = 16-bit
                        fprintf(asmFile, "    idiv bx ; Divide DX:AX by BX\n");
                        fprintf(asmFile, "    cwd ; Sign extend result\n");
                        fprintf(asmFile, "    jmp idiv32_done\n");
                        
                        // Complex case handler stub - would need a full algorithm
                        fprintf(asmFile, "idiv32_complex:\n");
                        fprintf(asmFile, "    ; Complex 32-bit division not fully implemented\n");
                        fprintf(asmFile, "    ; Returning dividend as result\n");
                        
                        fprintf(asmFile, "idiv32_done:\n");
                        fprintf(asmFile, "    add sp, 2 ; Clean up stack\n");
                    }
                } else {
                    TypeInfo* typeInfo = getTypeInfoFromExpression(node->left);
                    if (typeInfo && (typeInfo->type == TYPE_UNSIGNED_INT || 
                                    typeInfo->type == TYPE_UNSIGNED_SHORT || 
                                    typeInfo->type == TYPE_UNSIGNED_CHAR)) {
                        fprintf(asmFile, "    xor dx, dx ; Zero extend AX into DX:AX for unsigned division\n");
                        fprintf(asmFile, "    div bx ; Division (unsigned)\n");
                    } else {
                        fprintf(asmFile, "    cwd ; Sign extend AX into DX:AX for division\n");
                        fprintf(asmFile, "    idiv bx ; Division (signed)\n");
                    }
                }
            }
            break;        case OP_MOD:
            {
                if (isLongOperation) {
                    // 32-bit modulus is complex
                    fprintf(asmFile, "    ; 32-bit modulus\n");
                    
                    TypeInfo* typeInfo = getTypeInfoFromExpression(node->left);
                    if (typeInfo && (typeInfo->type == TYPE_UNSIGNED_LONG)) {
                        // Similar to division, but we want remainder instead
                        // We already have dividend in DX:AX and divisor in CX:BX
                        
                        fprintf(asmFile, "    ; 32-bit unsigned modulus\n");
                        fprintf(asmFile, "    push cx ; Save divisor high word\n");
                        
                        // Check if high word of divisor is zero
                        fprintf(asmFile, "    test cx, cx ; Check if high word of divisor is zero\n");
                        fprintf(asmFile, "    jnz mod32_complex ; Jump if we need a complex modulus\n");
                        
                        // Simple case: 32-bit % 16-bit
                        fprintf(asmFile, "    div bx ; Divide DX:AX by BX\n");
                        fprintf(asmFile, "    mov ax, dx ; Remainder is in DX\n");
                        fprintf(asmFile, "    xor dx, dx ; Clear high word of result\n");
                        fprintf(asmFile, "    jmp mod32_done\n");
                        
                        // Complex case handler stub
                        fprintf(asmFile, "mod32_complex:\n");
                        fprintf(asmFile, "    ; Complex 32-bit modulus not fully implemented\n");
                        fprintf(asmFile, "    ; Returning 0 as result\n");
                        fprintf(asmFile, "    xor ax, ax\n");
                        fprintf(asmFile, "    xor dx, dx\n");
                        
                        fprintf(asmFile, "mod32_done:\n");
                        fprintf(asmFile, "    add sp, 2 ; Clean up stack\n");
                    } else {
                        // Signed modulus 
                        fprintf(asmFile, "    ; 32-bit signed modulus\n");
                        fprintf(asmFile, "    push cx ; Save divisor high word\n");
                        
                        // Check if high word of divisor is zero
                        fprintf(asmFile, "    test cx, cx ; Check if high word of divisor is zero\n");
                        fprintf(asmFile, "    jnz imod32_complex ; Jump if we need a complex modulus\n");
                        
                        // Simple case: 32-bit % 16-bit
                        fprintf(asmFile, "    idiv bx ; Divide DX:AX by BX\n");
                        fprintf(asmFile, "    mov ax, dx ; Remainder is in DX\n");
                        fprintf(asmFile, "    cwd ; Sign extend result\n");
                        fprintf(asmFile, "    jmp imod32_done\n");
                        
                        // Complex case handler stub
                        fprintf(asmFile, "imod32_complex:\n");
                        fprintf(asmFile, "    ; Complex 32-bit modulus not fully implemented\n");
                        fprintf(asmFile, "    ; Returning 0 as result\n");
                        fprintf(asmFile, "    xor ax, ax\n");
                        fprintf(asmFile, "    xor dx, dx\n");
                        
                        fprintf(asmFile, "imod32_done:\n");
                        fprintf(asmFile, "    add sp, 2 ; Clean up stack\n");
                    }
                } else {
                    TypeInfo* typeInfo = getTypeInfoFromExpression(node->left);
                    if (typeInfo && (typeInfo->type == TYPE_UNSIGNED_INT || 
                                    typeInfo->type == TYPE_UNSIGNED_SHORT || 
                                    typeInfo->type == TYPE_UNSIGNED_CHAR)) {
                        fprintf(asmFile, "    xor dx, dx ; Zero extend AX into DX:AX for unsigned mod\n");
                        fprintf(asmFile, "    div bx ; Division (unsigned)\n");
                        fprintf(asmFile, "    mov ax, dx ; Remainder is in DX\n");
                    } else {
                        fprintf(asmFile, "    cwd ; Sign extend AX into DX:AX for signed mod\n");
                        fprintf(asmFile, "    idiv bx ; Division (signed)\n");
                        fprintf(asmFile, "    mov ax, dx ; Remainder is in DX\n");
                    }
                }
            }
            break;
              // Comparison operators for 8086 (without setcc instructions)
        case OP_EQ:
            fprintf(asmFile, "    cmp ax, bx ; Equal comparison\n");
            fprintf(asmFile, "    mov ax, 0  ; Assume false\n");
            fprintf(asmFile, "    je eq_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp eq_end_%d\n", labelCounter);
            fprintf(asmFile, "eq_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1  ; Set true\n");
            fprintf(asmFile, "eq_end_%d:\n", labelCounter++);
            break;
        case OP_NEQ:
            fprintf(asmFile, "    cmp ax, bx ; Not equal comparison\n");
            fprintf(asmFile, "    mov ax, 0  ; Assume false\n");
            fprintf(asmFile, "    jne neq_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp neq_end_%d\n", labelCounter);
            fprintf(asmFile, "neq_true_%d:\n", labelCounter);
                       fprintf(asmFile, "    mov ax, 1  ; Set true\n");
            fprintf(asmFile, "neq_end_%d:\n", labelCounter++);
            break;
        case OP_LT:
            fprintf(asmFile, "    cmp ax, bx ; Less than comparison\n");
            fprintf(asmFile, "    mov ax, 0  ; Assume false\n");
            fprintf(asmFile, "    jl lt_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp lt_end_%d\n", labelCounter);
            fprintf(asmFile, "lt_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1  ; Set true\n");
            fprintf(asmFile, "lt_end_%d:\n", labelCounter++);
            break;
        case OP_LTE:
            fprintf(asmFile, "    cmp ax, bx ; Less than or equal comparison\n");
            fprintf(asmFile, "    mov ax, 0  ; Assume false\n");
            fprintf(asmFile, "    jle lte_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp lte_end_%d\n", labelCounter);
            fprintf(asmFile, "lte_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1  ; Set true\n");
            fprintf(asmFile, "lte_end_%d:\n", labelCounter++);
            break;
        case OP_GT:
            fprintf(asmFile, "    cmp ax, bx ; Greater than comparison\n");
            fprintf(asmFile, "    mov ax, 0  ; Assume false\n");
            fprintf(asmFile, "    jg gt_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp gt_end_%d\n", labelCounter);
            fprintf(asmFile, "gt_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1  ; Set true\n");
            fprintf(asmFile, "gt_end_%d:\n", labelCounter++);
            break;
        case OP_GTE:
            fprintf(asmFile, "    cmp ax, bx ; Greater than or equal comparison\n");
            fprintf(asmFile, "    mov ax, 0  ; Assume false\n");
            fprintf(asmFile, "    jge gte_true_%d\n", labelCounter);
            fprintf(asmFile, "    jmp gte_end_%d\n", labelCounter);
            fprintf(asmFile, "gte_true_%d:\n", labelCounter);
            fprintf(asmFile, "    mov ax, 1  ; Set true\n");
            fprintf(asmFile, "gte_end_%d:\n", labelCounter++);
            break;
              case OP_BITWISE_AND:
            fprintf(asmFile, "    and ax, bx ; Bitwise AND\n");
            break;
            
        case OP_BITWISE_OR:
            fprintf(asmFile, "    or ax, bx ; Bitwise OR\n");
            break;
            
        case OP_BITWISE_XOR:
            fprintf(asmFile, "    xor ax, bx ; Bitwise XOR\n");
            break;
            
        case OP_LEFT_SHIFT:
            fprintf(asmFile, "    mov cx, bx ; Set shift count in CX\n");
            fprintf(asmFile, "    shl ax, cl ; Shift left\n");
            break;
              case OP_RIGHT_SHIFT:
            fprintf(asmFile, "    mov cx, bx ; Set shift count in CX\n");
            fprintf(asmFile, "    sar ax, cl ; Shift right (arithmetic, preserves sign)\n");
            break;
            
        case OP_COMMA:
            // For comma operator, left operand is already evaluated (result discarded)
            // and its value is in AX. Now evaluate right operand and its result becomes
            // the overall result.
            fprintf(asmFile, "    ; Comma operator - left operand already evaluated\n");
            fprintf(asmFile, "    ; The right operand's value becomes the result\n");
            generateExpression(node->right);
            break;
            
        default:
            reportWarning(-1, "Unsupported binary operator: %d", node->operation.op);
            break;
    }
}

// Generate code for a ternary conditional expression
void generateTernaryExpression(ASTNode* node) {
    if (!node || node->type != NODE_TERNARY) return;
    
    // Generate unique labels
    char* falseLabel = generateLabel("ternary_false");
    char* endLabel = generateLabel("ternary_end");
    
    fprintf(asmFile, "    ; Ternary conditional expression (condition ? true_expr : false_expr)\n");
    
    // Generate condition
    generateExpression(node->ternary.condition);
    
    // Test the condition, if false jump to false branch
    fprintf(asmFile, "    test ax, ax ; Test condition result\n");
    fprintf(asmFile, "    jz %s ; Jump to false branch if condition is false\n", falseLabel);
    
    // Generate true expression
    generateExpression(node->ternary.true_expr);
    
    // Jump to end (skip false expression)
    fprintf(asmFile, "    jmp %s ; Skip false branch\n", endLabel);
    
    // False branch
    fprintf(asmFile, "%s: ; False branch\n", falseLabel);
    
    // Generate false expression
    generateExpression(node->ternary.false_expr);
    
    // End label
    fprintf(asmFile, "%s: ; End of ternary expression\n", endLabel);
    
    // Free the allocated labels
    free(falseLabel);
    free(endLabel);
}

// Generate code for a function call
void generateFunctionCall(ASTNode* node) {
    if (!node || node->type != NODE_CALL) return;
    
    fprintf(asmFile, "    ; Function call to %s\n", node->call.func_name);
    
    // Count arguments and store them in an array
    int argCount = 0;
    ASTNode* arg = node->call.args;
    ASTNode* args[32]; // Maximum 32 arguments
    
    // Collect arguments in an array first
    while (arg) {
        args[argCount++] = arg;
        arg = arg->next;
    }
      // Push arguments in reverse order (right-to-left) as per C calling convention
    for (int i = argCount - 1; i >= 0; i--) {
        // Get argument type information to check if it's a long type
        TypeInfo* typeInfo = getTypeInfoFromExpression(args[i]);
        
        // Generate code for each argument
        generateExpression(args[i]);
        
        if (typeInfo && (typeInfo->type == TYPE_LONG || typeInfo->type == TYPE_UNSIGNED_LONG)) {
            // For 32-bit long values, push high word (DX) then low word (AX)
            fprintf(asmFile, "    push dx ; Argument %d (high word)\n", i + 1);
            fprintf(asmFile, "    push ax ; Argument %d (low word)\n", i + 1);
        } else {
            // For normal values, just push AX
            fprintf(asmFile, "    push ax ; Argument %d\n", i + 1);
        }
    }
    
    // Call the function
    fprintf(asmFile, "    call _%s\n", node->call.func_name);
      // Clean up stack (caller-cleanup convention)
    if (argCount > 0) {
        // Calculate how many bytes to clean up based on argument types
        int bytesToCleanup = 0;
        
        for (int i = 0; i < argCount; i++) {
            TypeInfo* typeInfo = getTypeInfoFromExpression(args[i]);
            if (typeInfo && (typeInfo->type == TYPE_LONG || typeInfo->type == TYPE_UNSIGNED_LONG)) {
                bytesToCleanup += 4; // 32-bit args take 4 bytes
            } else {
                bytesToCleanup += 2; // Normal args take 2 bytes
            }
        }
        
        fprintf(asmFile, "    add sp, %d ; Remove arguments\n", bytesToCleanup);
    }
}

// Generate code for a return statement
void generateReturnStatement(ASTNode* node) {
    if (!node || node->type != NODE_RETURN) return;
    
    fprintf(asmFile, "    ; Return statement\n");    // Generate code for return value if present
    if (node->return_stmt.expr) {
        // Check if the return value is a long type
        TypeInfo* returnType = getTypeInfoFromExpression(node->return_stmt.expr);
        
        generateExpression(node->return_stmt.expr);
        
        if (returnType && (returnType->type == TYPE_LONG || returnType->type == TYPE_UNSIGNED_LONG)) {
            // For 32-bit return values, the value is in DX:AX
            fprintf(asmFile, "    ; Returning 32-bit long value in DX:AX\n");
        } else {
            // For regular types, return value is in AX
            fprintf(asmFile, "    ; Return value in AX\n");
        }
    }
    
    // For naked functions, don't generate automatic control flow
    if (!currentFunctionIsNaked) {
        // Jump to function epilogue
        fprintf(asmFile, "    jmp _%s_exit\n", currentFunction);
    } else {
        fprintf(asmFile, "    ; Naked function - no automatic jump to epilogue generated\n");
    }
}

// Generate code for an inline assembly block
void generateAsmBlock(ASTNode* node) {
    if (!node || node->type != NODE_ASM_BLOCK || !node->asm_block.code) return;
    
    fprintf(asmFile, "    ; Inline assembly block\n");
    fprintf(asmFile, "%s\n", node->asm_block.code);
}

// Generate code for an inline assembly statement
void generateAsmStmt(ASTNode* node) {
    if (!node || node->type != NODE_ASM || !node->asm_stmt.code) return;
    
    fprintf(asmFile, "    ; Inline assembly statement\n");
    
    // If there are no operands, just output the code directly
    if (node->asm_stmt.operand_count == 0) {
        fprintf(asmFile, "    %s\n", node->asm_stmt.code);
        return;
    }
    
    // With operands, we need to:
    // 1. First generate code to load the input operands into registers
    // 2. Substitute %0, %1, etc. with appropriate register names
    // 3. After the assembly code, store output operands back to their variables
    fprintf(asmFile, "    ; Inline assembly with %d operands\n", node->asm_stmt.operand_count);
    
    // Arrays to store register assignments and operand types
    char** registers = (char**)malloc(sizeof(char*) * node->asm_stmt.operand_count);
    int* isOutput = (int*)malloc(sizeof(int) * node->asm_stmt.operand_count);
    
    if (!registers || !isOutput) {
        fprintf(stderr, "Memory allocation failed for assembly registers\n");
        if (registers) free(registers);
        if (isOutput) free(isOutput);
        return;
    }
      // Commonly used registers based on constraint type
    const char* word_reg_choices[] = {"ax", "bx", "cx", "dx", "si", "di"};
    const char* byte_reg_choices[] = {"al", "bl", "cl", "dl"}; // Byte-sized registers
    int reg_index = 0;
    
    // Process operands and assign registers
    for (int i = 0; i < node->asm_stmt.operand_count; i++) {
        // Check constraint type
        char* constraint = node->asm_stmt.constraints[i];
        
        // Determine if this is an output operand (constraint starts with =)
        isOutput[i] = (constraint[0] == '=');
        
        // Skip the = for output operands to get the actual constraint
        if (isOutput[i]) {
            constraint++;
        }
          // Process input operands - generate code to load them
        // For "q" constraint, we'll handle the loading specially to avoid the mov al, al issue
        if (!isOutput[i] && (*constraint != 'q')) {
            // Generate code to evaluate the operand
            generateExpression(node->asm_stmt.operands[i]);
        }
          // Assign register based on constraint
        if (*constraint == 'r' && (constraint[1] == 'b' || constraint[1] == '\0')) {
            // Check if it's a byte register constraint ("rb") or word register ("r")
            int is_byte_register = (constraint[1] == 'b');
            
            if (is_byte_register) {
                // Byte register constraint ("rb")
                if (reg_index < 4) { // Only 4 byte registers available
                    registers[i] = strdupc(byte_reg_choices[reg_index++]);
                } else {
                    // Fall back to al if we run out of preferred registers
                    registers[i] = strdupc("al");
                }
                
                // For input operands, move result to the assigned byte register
                if (!isOutput[i]) {
                    // AL is the default result register's low byte
                    if (strcmp(registers[i], "al") != 0) {
                        fprintf(asmFile, "    mov %s, al ; Load byte input operand %d into register\n", 
                                registers[i], i);
                    }
                }
            } else {
                // Standard word register constraint ("r")
                if (reg_index < 6) {
                    registers[i] = strdupc(word_reg_choices[reg_index++]);
                } else {
                    // Run out of preferred registers, just use ax
                    registers[i] = strdupc("ax");
                }
                
                // For input operands, move result to the assigned register
                if (!isOutput[i] && strcmp(registers[i], "ax") != 0) {
                    fprintf(asmFile, "    mov %s, ax ; Load word input operand %d into register\n", 
                            registers[i], i);
                }
            }        } else if (*constraint == 'q') {
            // 'q' is a GCC constraint that means a,b,c,d registers (in any size)
            // In our case, we'll use it specifically for byte registers (al, bl, cl, dl)
            if (reg_index < 4) { // Only 4 byte registers available
                registers[i] = strdupc(byte_reg_choices[reg_index++]);
            } else {
                // Fall back to al if we run out of preferred registers
                registers[i] = strdupc("al");
            }
            
            // For input operands, handle byte-sized parameters properly
            if (!isOutput[i]) {
                // Check if this is a parameter or variable that we can directly access
                if (node->asm_stmt.operands[i]->type == NODE_IDENTIFIER) {
                    char* varName = node->asm_stmt.operands[i]->identifier;
                    int offset = getVariableOffset(varName);
                    
                    if (isParameter(varName)) {
                        // For parameters, load the byte directly into the byte register
                        fprintf(asmFile, "    mov %s, byte [bp+%d] ; Load byte parameter directly\n", 
                                registers[i], -offset);
                    } else if (offset > 0) {
                        // For local variables, load the byte directly into the byte register
                        fprintf(asmFile, "    mov %s, byte [bp-%d] ; Load byte local variable directly\n", 
                                registers[i], offset);
                    } else {
                        // For other variables (likely globals), use standard approach
                        generateExpression(node->asm_stmt.operands[i]);
                        // If the assigned register is not al, move from al to the assigned register
                        if (strcmp(registers[i], "al") != 0) {
                            fprintf(asmFile, "    mov %s, al ; Load byte input operand %d into register ('q' constraint)\n", 
                                    registers[i], i);
                        }
                    }
                } else {
                    // For complex expressions, use the standard approach
                    generateExpression(node->asm_stmt.operands[i]);
                    // If the assigned register is not al, move from al to the assigned register
                    if (strcmp(registers[i], "al") != 0) {
                        fprintf(asmFile, "    mov %s, al ; Load byte input operand %d into register ('q' constraint)\n", 
                                registers[i], i);
                    }
                }
            }
        } else {
            // Default to ax for unknown constraints
            registers[i] = strdupc("ax");
        }
    }
    
    // Now process the assembly code string, replacing %0, %1, etc.
    char* asmCode = strdupc(node->asm_stmt.code);
    char* result = (char*)malloc(strlen(asmCode) * 2); // Allocate double space for substitutions
    if (!result) {
        fprintf(stderr, "Memory allocation failed for assembly code processing\n");
        free(asmCode);
        for (int i = 0; i < node->asm_stmt.operand_count; i++) {
            free(registers[i]);
        }
        free(registers);
        free(isOutput);
        return;
    }
    
    result[0] = '\0';
    
    // Process the string, replacing %0, %1, etc. with register names
    char* p = asmCode;
    while (*p) {
        if (*p == '%' && *(p+1) >= '0' && *(p+1) <= '9') {
            int operand_num = *(p+1) - '0';
            if (operand_num < node->asm_stmt.operand_count) {
                strcat(result, registers[operand_num]);
                p += 2;
            } else {
                // Invalid operand number, just copy the %
                char tmp[2] = {*p, '\0'};
                strcat(result, tmp);
                p++;
            }
        } else {
            char tmp[2] = {*p, '\0'};
            strcat(result, tmp);
            p++;
        }
    }
    
    // Output the processed assembly code
    fprintf(asmFile, "    %s\n", result);
    
    // After executing the assembly code, store output operands back to their variables
    for (int i = 0; i < node->asm_stmt.operand_count; i++) {
        if (isOutput[i]) {
            // This is an output operand - store the register value back to the variable
            ASTNode* operand = node->asm_stmt.operands[i];
            
            if (operand->type == NODE_IDENTIFIER) {
                // Get variable name and offset
                char* varName = operand->identifier;
                int offset = getVariableOffset(varName);
                
                // Check if it's a parameter or local variable
                if (isParameter(varName)) {
                    fprintf(asmFile, "    mov [bp+%d], %s ; Store output operand %d to parameter %s\n", 
                            -offset, registers[i], i, varName);
                } else {
                    fprintf(asmFile, "    mov [bp-%d], %s ; Store output operand %d to local variable %s\n", 
                            offset, registers[i], i, varName);
                }
            } else {
                // For complex output expressions (dereferencing pointers, etc.)
                fprintf(asmFile, "    ; Warning: Complex output operand not fully supported\n");
            }
        }
    }
    
    // Clean up
    free(asmCode);
    free(result);
    free(isOutput);
    for (int i = 0; i < node->asm_stmt.operand_count; i++) {
        free(registers[i]);
    }
    free(registers);
}