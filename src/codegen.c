#include "codegen.h"
#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global code generator state
static CodeGenerator codegen;

// Simple per-function local variable tracking (stack offsets)
typedef struct LocalVarEntry {
    char* name;
    int offset; // positive byte offset from BP/RBP; address as [bp - offset]
    int isRegister; // 1 if this local lives in a register
    Register reg;   // home register when isRegister=1
} LocalVarEntry;

static LocalVarEntry* locals = NULL;
static int localCount = 0;
static int currentStackOffset = 0; // grows as locals allocated
static int savedRBXForThisFunction = 0; // x64 callee-saved preservation

// Simple per-function parameter tracking (positive offsets from BP/RBP)
typedef struct ParamEntry {
    char* name;
    int offset; // positive byte offset from BP/RBP; address as [bp + offset]
} ParamEntry;

static ParamEntry* params = NULL;
static int paramCountTracked = 0;

static void resetLocals(void) {
    for (int i = 0; i < localCount; i++) {
        // Free any reserved registers
        if (locals[i].isRegister) {
            if (locals[i].reg < MAX_REGISTERS) {
                codegen.registerInUse[locals[i].reg] = 0;
            }
        }
        free(locals[i].name);
    }
    free(locals);
    locals = NULL;
    localCount = 0;
    currentStackOffset = 0;

    // Reset parameters as well
    for (int i = 0; i < paramCountTracked; i++) {
        free(params[i].name);
    }
    free(params);
    params = NULL;
    paramCountTracked = 0;
}

static int addLocal(const char* name, int size) {
    currentStackOffset += size;
    int offset = currentStackOffset;
    locals = (LocalVarEntry*)realloc(locals, sizeof(LocalVarEntry) * (localCount + 1));
    locals[localCount].name = strdup(name);
    locals[localCount].offset = offset;
    locals[localCount].isRegister = 0;
    locals[localCount].reg = REG_INVALID;
    localCount++;
    return offset;
}

static int findLocalOffset(const char* name, int* outOffset) {
    for (int i = 0; i < localCount; i++) {
        if (strcmp(locals[i].name, name) == 0) {
            if (locals[i].isRegister) return 0; // not a stack local
            if (outOffset) *outOffset = locals[i].offset;
            return 1;
        }
    }
    return 0;
}

static int findLocalRegister(const char* name, Register* outReg) {
    for (int i = 0; i < localCount; i++) {
        if (strcmp(locals[i].name, name) == 0 && locals[i].isRegister) {
            if (outReg) *outReg = locals[i].reg;
            return 1;
        }
    }
    return 0;
}

static int findParamOffset(const char* name, int* outOffset) {
    for (int i = 0; i < paramCountTracked; i++) {
        if (strcmp(params[i].name, name) == 0) {
            if (outOffset) *outOffset = params[i].offset;
            return 1;
        }
    }
    return 0;
}

static int is64CalleeSaved(Register r) {
    // In SysV AMD64: RBX, RBP, R12-R15 are callee-saved
    return (r == REG_BX || r == REG_BP);
}

static int isAllocatableCallerSaved(Register r) {
    // Prefer RCX, RDX, RSI, RDI as they map to caller-saved under SysV AMD64
    return (r == REG_CX || r == REG_DX || r == REG_SI || r == REG_DI);
}

static int reserveRegister(Register r) {
    if (r < MAX_REGISTERS && !codegen.registerInUse[r]) {
        codegen.registerInUse[r] = 1;
        return 1;
    }
    return 0;
}

static int allocateCallerSavedForLocal(Register* outReg) {
    // Try preferred set: AX, then CX, DX, SI, DI (all caller-saved in SysV AMD64)
    Register candidates[] = { REG_AX, REG_CX, REG_DX, REG_SI, REG_DI };
    int n = sizeof(candidates)/sizeof(candidates[0]);
    for (int i = 0; i < n; i++) {
        if (reserveRegister(candidates[i])) {
            *outReg = candidates[i];
            return 1;
        }
    }
    return 0;
}

static int addRegLocal(const char* name, int size, Register* outReg) {
    (void)size;
    Register r;
    if (!allocateCallerSavedForLocal(&r)) return 0;
    locals = (LocalVarEntry*)realloc(locals, sizeof(LocalVarEntry) * (localCount + 1));
    locals[localCount].name = strdup(name);
    locals[localCount].offset = 0;
    locals[localCount].isRegister = 1;
    locals[localCount].reg = r;
    localCount++;
    if (outReg) *outReg = r;
    return 1;
}

static void addExistingRegLocal(const char* name, Register reg) {
    // Mark as in-use when within tracked range
    if (reg < MAX_REGISTERS) {
        codegen.registerInUse[reg] = 1;
    }
    locals = (LocalVarEntry*)realloc(locals, sizeof(LocalVarEntry) * (localCount + 1));
    locals[localCount].name = strdup(name);
    locals[localCount].offset = 0;
    locals[localCount].isRegister = 1;
    locals[localCount].reg = reg;
    localCount++;
}

static const char* ax_by_size(int size) {
    // If strict16 is set and target is 16-bit, cap at 16-bit names
    if (codegen.targetWidth == 16 && codegen.forceStrict16) {
        switch (size) {
            case 1: return "al";
            case 2: default: return "ax";
        }
    }
    switch (size) {
        case 1: return "al";
        case 2: return "ax";
        case 4: return "eax";
        case 8: return "rax";
        default: return getRegisterName(REG_AX, codegen.targetWidth);
    }
}

static const char* reg_by_size(Register r, int size) {
    if (codegen.targetWidth == 16 && codegen.forceStrict16) {
        // Allow 8-bit AL for AX family when size==1; otherwise 16-bit regs
        if (size == 1 && r == REG_AX) return "al";
        return getRegisterName(r, 16);
    }
    switch (size) {
        case 1:
            // Only AX family has 8-bit names here; fall back to 32-bit for others
            if (r == REG_AX) return "al";
            return getRegisterName(r, 32);
        case 2:
            if (r == REG_AX) return "ax";
            return getRegisterName(r, 32);
        case 4:
            return getRegisterName(r, 32);
        case 8:
        default:
            return getRegisterName(r, 64);
    }
}

// Error handling wrapper
static void codegenErrorSimple(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char message[512];
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    codegenError(0, 0, "%s", message);
}

// Register allocation
const char* getRegisterName(Register reg, int width) {
    if (codegen.targetWidth == 16 && codegen.forceStrict16) {
        width = 16;
    }
    switch (width) {
        case 16:
            switch (reg) {
                case REG_AX: return "ax";
                case REG_BX: return "bx";
                case REG_CX: return "cx";
                case REG_DX: return "dx";
                case REG_SI: return "si";
                case REG_DI: return "di";
                case REG_BP: return "bp";
                case REG_SP: return "sp";
                default: return "ax";
            }
        case 32:
            switch (reg) {
                case REG_AX: return "eax";
                case REG_BX: return "ebx";
                case REG_CX: return "ecx";
                case REG_DX: return "edx";
                case REG_SI: return "esi";
                case REG_DI: return "edi";
                case REG_BP: return "ebp";
                case REG_SP: return "esp";
                case REG_R8: return "r8d";
                case REG_R9: return "r9d";
                case REG_R10: return "r10d";
                case REG_R11: return "r11d";
                case REG_R12: return "r12d";
                case REG_R13: return "r13d";
                case REG_R14: return "r14d";
                case REG_R15: return "r15d";
                default: return "eax";
            }
        case 64:
            switch (reg) {
                case REG_AX: return "rax";
                case REG_BX: return "rbx";
                case REG_CX: return "rcx";
                case REG_DX: return "rdx";
                case REG_SI: return "rsi";
                case REG_DI: return "rdi";
                case REG_BP: return "rbp";
                case REG_SP: return "rsp";
                case REG_R8: return "r8";
                case REG_R9: return "r9";
                case REG_R10: return "r10";
                case REG_R11: return "r11";
                case REG_R12: return "r12";
                case REG_R13: return "r13";
                case REG_R14: return "r14";
                case REG_R15: return "r15";
                default: return "rax";
            }
        default:
            return "ax";
    }
}

Register allocateRegister(void) {
    for (int i = 0; i < MAX_REGISTERS; i++) {
        if (i == REG_AX) continue; // avoid using AX/RAX as a temp
        if (!codegen.registerInUse[i]) {
            codegen.registerInUse[i] = 1;
            return (Register)i;
        }
    }
    
    // If no registers available, spill one (simplified)
    codegenErrorSimple("Register allocation failed - all registers in use");
    return REG_AX; // fallback
}

void freeRegister(Register reg) {
    if (reg < MAX_REGISTERS) {
        codegen.registerInUse[reg] = 0;
    }
}

// Assembly output
void emitInstruction(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char instruction[256];
    vsnprintf(instruction, sizeof(instruction), format, args);
    va_end(args);
    
    fprintf(codegen.output, "    %s\n", instruction);
    codegen.instructionCount++;
}

void emitLabel(const char* label) {
    fprintf(codegen.output, "%s:\n", label);
}

void emitComment(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char comment[256];
    vsnprintf(comment, sizeof(comment), format, args);
    va_end(args);
    
    fprintf(codegen.output, "    ; %s\n", comment);
}

// NAS-specific directives
static void emitNasDirective(const char* directive, const char* value) {
    if (value) {
        fprintf(codegen.output, "#%s %s\n", directive, value);
    } else {
        fprintf(codegen.output, "#%s\n", directive);
    }
}

static void emitNasWidth(int width) {
    emitNasDirective("width", (width == 16) ? "16" : (width == 32) ? "32" : "64");
}

static void emitNasOrigin(unsigned long long origin) {
    fprintf(codegen.output, "#origin 0x%llx\n", origin);
}

static void emitNasSection(const char* name) {
    emitNasDirective("section", name);
}

static void emitNasGlobal(const char* name) {
    emitNasDirective("global", name);
}

static void emitNasExtern(const char* name) {
    emitNasDirective("extend", name); // NAS uses "extend" instead of "extern"
}

// --- String literal byte emission helpers ---
// Decode a C string token (including surrounding quotes) into raw bytes.
// Handles simple escapes: \n, \r, \t, \b, \f, \v, \\, \", \' and \0.
// Returns a newly allocated buffer and length via outLen. Caller frees.
static unsigned char* decodeStringTokenToBytes(const char* token, int* outLen) {
    if (!token) {
        *outLen = 0;
        return NULL;
    }
    size_t len = strlen(token);
    // Skip surrounding quotes if present
    size_t i = 0;
    size_t end = len;
    if (len >= 2 && token[0] == '"' && token[len - 1] == '"') {
        i = 1;
        end = len - 1;
    }
    // Allocate worst-case buffer (no escapes shrink)
    unsigned char* buf = (unsigned char*)malloc((end - i) + 1);
    int j = 0;
    while (i < end) {
        char c = token[i++];
        if (c == '\\' && i < end) {
            char e = token[i++];
            switch (e) {
                case 'n': buf[j++] = '\n'; break;
                case 'r': buf[j++] = '\r'; break;
                case 't': buf[j++] = '\t'; break;
                case 'b': buf[j++] = '\b'; break;
                case 'f': buf[j++] = '\f'; break;
                case 'v': buf[j++] = '\v'; break;
                case '\\': buf[j++] = '\\'; break;
                case '"': buf[j++] = '"'; break;
                case '\'': buf[j++] = '\''; break;
                case '0': buf[j++] = '\0'; break;
                default:  // Unknown escape, keep literally the escaped char
                    buf[j++] = (unsigned char)e;
                    break;
            }
        } else {
            buf[j++] = (unsigned char)c;
        }
    }
    *outLen = j;
    return buf;
}

static void emitDbBytes(const unsigned char* bytes, int len) {
    fprintf(codegen.output, "    #db ");
    for (int k = 0; k < len; k++) {
        fprintf(codegen.output, "0x%X", (unsigned int)bytes[k] & 0xFF);
        if (k != len - 1) {
            fprintf(codegen.output, ", ");
        }
    }
}

// String literal management
static char* addStringLiteral(const char* str) {
    char* label = malloc(32);
    snprintf(label, 32, "_str_%d", codegen.stringLiteralCount);
    codegen.stringLiteralCount++;
    
    // Store for later emission
    StringLiteral* literal = malloc(sizeof(StringLiteral));
    literal->label = strdup(label);
    literal->value = strdup(str);
    literal->next = codegen.stringLiterals;
    codegen.stringLiterals = literal;
    
    return label;
}

// Code generation functions
void generateExpression(ASTNode* node);
void generateStatement(ASTNode* node);

static void generateBinaryOp(ASTNode* node) {
    ASTNode* left = node->data.binary.left;
    ASTNode* right = node->data.binary.right;
    BinaryOperator op = node->data.binary.op;
    
    // Handle simple assignment to identifier
    if (op == BIN_ASSIGN && left && left->type == AST_IDENTIFIER) {
        // Evaluate RHS into AX
        generateExpression(right);
        int size = 4; // TODO: read from left->typeInfo
        const char* ax = ax_by_size(size);
        const char* bp = getRegisterName(REG_BP, codegen.targetWidth);
        int off;
        Register rhome;
        int poff;
        if (findParamOffset(left->data.identifier.name, &poff)) {
            emitInstruction("mov [%s+%d], %s", bp, poff, ax);
        } else
        if (findLocalRegister(left->data.identifier.name, &rhome)) {
            emitInstruction("mov %s, %s", reg_by_size(rhome, size), ax);
        } else if (findLocalOffset(left->data.identifier.name, &off)) {
            emitInstruction("mov [%s-%d], %s", bp, off, ax);
        } else {
            emitInstruction("mov [%s], %s", left->data.identifier.name, ax);
        }
        return;
    }

    // Handle compound assignments like +=, -=, etc. to identifiers
    if ((op == BIN_ADD_ASSIGN || op == BIN_SUB_ASSIGN || op == BIN_MUL_ASSIGN ||
         op == BIN_DIV_ASSIGN || op == BIN_MOD_ASSIGN || op == BIN_AND_ASSIGN ||
         op == BIN_OR_ASSIGN  || op == BIN_XOR_ASSIGN || op == BIN_LEFT_SHIFT_ASSIGN ||
         op == BIN_RIGHT_SHIFT_ASSIGN) && left && left->type == AST_IDENTIFIER) {
        int size = 4; // TODO: use left->typeInfo
        const char* ax = ax_by_size(size);
        const char* bp = getRegisterName(REG_BP, codegen.targetWidth);
        int off;
        Register rhome;
        int poff;
        int isParam = findParamOffset(left->data.identifier.name, &poff);
        int hasRegHome = findLocalRegister(left->data.identifier.name, &rhome);
        int isLocal = (!hasRegHome && !isParam) && findLocalOffset(left->data.identifier.name, &off);
        // Load current left value into a temp register or use home reg directly
        Register lreg = hasRegHome ? rhome : allocateRegister();
        const char* lregName = reg_by_size(lreg, size);
        if (!hasRegHome) {
            if (isParam) {
                emitInstruction("mov %s, [%s+%d]", lregName, bp, poff);
            } else if (isLocal) {
                emitInstruction("mov %s, [%s-%d]", lregName, bp, off);
            } else {
                emitInstruction("mov %s, [%s]", lregName, left->data.identifier.name);
            }
        }

        int inPlace = 0; // whether lreg updated in place (so no store-back needed for reg-home)

        // Fast-path: if RHS is a literal and we have a reg home, use immediate ops when possible
    if (hasRegHome && (right->type == AST_INTEGER_LITERAL || right->type == AST_CHAR_LITERAL || right->type == AST_BOOL_LITERAL)) {
            long long imm = right->data.literal.intValue;
            switch (op) {
                case BIN_ADD_ASSIGN:
                    emitInstruction("add %s, %lld", lregName, imm);
                    inPlace = 1;
                    break;
                case BIN_SUB_ASSIGN:
                    emitInstruction("sub %s, %lld", lregName, imm);
                    inPlace = 1;
                    break;
                case BIN_AND_ASSIGN:
                    emitInstruction("and %s, %lld", lregName, imm);
                    inPlace = 1;
                    break;
                case BIN_OR_ASSIGN:
                    emitInstruction("or %s, %lld", lregName, imm);
                    inPlace = 1;
                    break;
                case BIN_XOR_ASSIGN:
                    emitInstruction("xor %s, %lld", lregName, imm);
                    inPlace = 1;
                    break;
                case BIN_LEFT_SHIFT_ASSIGN:
                    emitInstruction("mov cl, %lld", imm & 0xFF);
                    emitInstruction("shl %s, cl", lregName);
                    inPlace = 1;
                    break;
                case BIN_RIGHT_SHIFT_ASSIGN:
                    emitInstruction("mov cl, %lld", imm & 0xFF);
                    emitInstruction("shr %s, cl", lregName);
                    inPlace = 1;
                    break;
                default:
                    break; // fall through to general path for MUL/DIV/MOD
            }
            if (inPlace) {
                // Result to AX as well unless home is already AX
                if (!(hasRegHome && rhome == REG_AX)) {
                    emitInstruction("mov %s, %s", ax, lregName);
                }
            }
        }

        if (!inPlace) {
            // General path: evaluate RHS into AX and operate with lreg
            generateExpression(right);

            if (op == BIN_ADD_ASSIGN || op == BIN_SUB_ASSIGN || op == BIN_AND_ASSIGN ||
                op == BIN_OR_ASSIGN || op == BIN_XOR_ASSIGN) {
                const char* rname = ax; // RHS in AX
                switch (op) {
                    case BIN_ADD_ASSIGN: emitInstruction("add %s, %s", lregName, rname); break;
                    case BIN_SUB_ASSIGN: emitInstruction("sub %s, %s", lregName, rname); break;
                    case BIN_AND_ASSIGN: emitInstruction("and %s, %s", lregName, rname); break;
                    case BIN_OR_ASSIGN:  emitInstruction("or %s, %s",  lregName, rname); break;
                    case BIN_XOR_ASSIGN: emitInstruction("xor %s, %s", lregName, rname); break;
                    default: break;
                }
                if (!(hasRegHome && rhome == REG_AX)) {
                    emitInstruction("mov %s, %s", ax, lregName);
                }
                inPlace = hasRegHome; // updated lreg directly
            } else if (op == BIN_LEFT_SHIFT_ASSIGN || op == BIN_RIGHT_SHIFT_ASSIGN) {
                // shift count must be in CL
                emitInstruction("mov cl, %s", getRegisterName(REG_AX, 8));
                if (op == BIN_LEFT_SHIFT_ASSIGN) {
                    emitInstruction("shl %s, cl", lregName);
                } else {
                    emitInstruction("shr %s, cl", lregName);
                }
                if (!(hasRegHome && rhome == REG_AX)) {
                    emitInstruction("mov %s, %s", ax, lregName);
                }
                inPlace = hasRegHome;
            } else if (op == BIN_MUL_ASSIGN) {
                // Use two-operand imul when possible: imul lreg, r/m
                emitInstruction("imul %s, %s", lregName, ax);
                emitInstruction("mov %s, %s", ax, lregName);
                inPlace = hasRegHome;
            } else if (op == BIN_DIV_ASSIGN || op == BIN_MOD_ASSIGN) {
                // Division requires AX:DX dividend; preserve lreg into AX first
                emitInstruction("mov %s, %s", ax, lregName);
                if (codegen.targetWidth == 16) {
                    emitInstruction("xor dx, dx");
                    emitInstruction("div %s", ax); // RHS still in AX; this is imperfect but placeholder
                } else {
                    const char* dxReg = getRegisterName(REG_DX, codegen.targetWidth);
                    emitInstruction("xor %s, %s", dxReg, dxReg);
                    emitInstruction("idiv %s", ax);
                }
                if (op == BIN_MOD_ASSIGN) {
                    if (codegen.targetWidth == 16) {
                        emitInstruction("mov ax, dx");
                    } else {
                        const char* dxReg = getRegisterName(REG_DX, codegen.targetWidth);
                        emitInstruction("mov %s, %s", ax, dxReg);
                    }
                }
                // Store back to lvalue below
                inPlace = 0;
            }
        }

        // Store back AX into left (skip if updated in place and home is a register)
        if (hasRegHome) {
            if (!inPlace) {
                // Only needed for ops where result produced in AX and home isn't AX already
                if (!(hasRegHome && rhome == REG_AX)) {
                    emitInstruction("mov %s, %s", lregName, ax);
                }
            }
        } else if (isParam) {
            emitInstruction("mov [%s+%d], %s", bp, poff, ax);
        } else if (isLocal) {
            emitInstruction("mov [%s-%d], %s", bp, off, ax);
        } else {
            emitInstruction("mov [%s], %s", left->data.identifier.name, ax);
        }

        if (!hasRegHome) freeRegister(lreg);
        return;
    }

    // General assignment to memory (e.g., *ptr = rhs; arr[i] = rhs;)
    if (op == BIN_ASSIGN && left) {
        // Compute address for lvalue into a temp register
        Register addrReg; int addrRegUsed = 0;
        const char* addrName = NULL;
        int handled = 0;
        if (left->type == AST_UNARY_OP && left->data.unary.op == UNARY_DEREFERENCE) {
            // Evaluate pointer expression -> AX holds address
            generateExpression(left->data.unary.operand);
            addrReg = allocateRegister();
            addrName = getRegisterName(addrReg, codegen.targetWidth);
            emitInstruction("mov %s, %s", addrName, getRegisterName(REG_AX, codegen.targetWidth));
            handled = 1; addrRegUsed = 1;
        } else if (left->type == AST_ARRAY_ACCESS) {
            // Evaluate array base to AX
            generateExpression(left->data.arrayAccess.array);
            Register baseReg = allocateRegister();
            const char* baseName = getRegisterName(baseReg, codegen.targetWidth);
            emitInstruction("mov %s, %s", baseName, getRegisterName(REG_AX, codegen.targetWidth));
            // Evaluate index -> AX
            generateExpression(left->data.arrayAccess.index);
            const char* indexName = getRegisterName(REG_AX, codegen.targetWidth);
            // Assume element size 1 for now
            emitInstruction("add %s, %s", baseName, indexName);
            addrReg = baseReg;
            addrName = baseName;
            handled = 1; addrRegUsed = 1;
        }

        if (handled) {
            // Evaluate RHS and store a byte (AL). TODO: width from typeInfo
            generateExpression(right);
            const char* ax8 = getRegisterName(REG_AX, 8);
            emitInstruction("mov [%s], %s", addrName, ax8);
            // Result of assignment is the stored value (in AX already). Zero-extend to target width
            if (codegen.targetWidth == 16 && codegen.forceStrict16) {
                emitInstruction("xor ah, ah");
            } else {
                emitInstruction("movzx %s, %s", getRegisterName(REG_AX, codegen.targetWidth), ax8);
            }
            if (addrRegUsed) freeRegister(addrReg);
            return;
        }
    }

    // Generate left operand
    generateExpression(left);
    Register leftReg = allocateRegister();
    const char* leftRegName = getRegisterName(leftReg, codegen.targetWidth);
    // Move result from AX into a temp only if temp is not AX (it never is now)
    emitInstruction("mov %s, %s", leftRegName, getRegisterName(REG_AX, codegen.targetWidth));
    
    // Short-circuit logical operators: handle before generating RHS
    if (op == BIN_LOGICAL_AND || op == BIN_LOGICAL_OR) {
        char lShort[32], lEnd[32];
        snprintf(lShort, sizeof lShort, ".L_logic_short_%d", codegen.labelCount);
        snprintf(lEnd, sizeof lEnd, ".L_logic_end_%d", codegen.labelCount);
        codegen.labelCount++;

        const char* resultReg = getRegisterName(REG_AX, codegen.targetWidth);
        const char* ax8 = getRegisterName(REG_AX, 8);

        if (op == BIN_LOGICAL_AND) {
            // if (left == 0) goto short (result 0); else evaluate right and set result = right != 0
            emitInstruction("cmp %s, 0", leftRegName);
            emitInstruction("je %s", lShort);
            // left is non-zero; evaluate right
            generateExpression(right);
            emitInstruction("cmp %s, 0", resultReg);
            emitInstruction("setne %s", ax8);
            emitInstruction("jmp %s", lEnd);
            emitLabel(lShort);
            emitInstruction("xor %s, %s", ax8, ax8); // al = 0
            emitLabel(lEnd);
            if (codegen.targetWidth == 16 && codegen.forceStrict16) {
                emitInstruction("xor ah, ah");
            } else {
                emitInstruction("movzx %s, %s", resultReg, ax8);
            }
        } else { // BIN_LOGICAL_OR
            // if (left != 0) goto short (result 1); else evaluate right and set result = right != 0
            emitInstruction("cmp %s, 0", leftRegName);
            emitInstruction("jne %s", lShort);
            // left is zero; evaluate right
            generateExpression(right);
            emitInstruction("cmp %s, 0", resultReg);
            emitInstruction("setne %s", ax8);
            emitInstruction("jmp %s", lEnd);
            emitLabel(lShort);
            emitInstruction("mov %s, 1", ax8); // al = 1
            emitLabel(lEnd);
            if (codegen.targetWidth == 16 && codegen.forceStrict16) {
                emitInstruction("xor ah, ah");
            } else {
                emitInstruction("movzx %s, %s", resultReg, ax8);
            }
        }

        freeRegister(leftReg);
        return;
    }
    
    // Generate right operand
    generateExpression(right);
    Register rightReg = allocateRegister();
    const char* rightRegName = getRegisterName(rightReg, codegen.targetWidth);
    emitInstruction("mov %s, %s", rightRegName, getRegisterName(REG_AX, codegen.targetWidth));
    
    // Perform operation
    const char* resultReg = getRegisterName(REG_AX, codegen.targetWidth);
    
    switch (op) {
        case BIN_ADD:
            emitInstruction("mov %s, %s", resultReg, leftRegName);
            emitInstruction("add %s, %s", resultReg, rightRegName);
            break;
        case BIN_SUB:
            emitInstruction("mov %s, %s", resultReg, leftRegName);
            emitInstruction("sub %s, %s", resultReg, rightRegName);
            break;
        case BIN_MUL:
            emitInstruction("mov %s, %s", resultReg, leftRegName);
            if (codegen.targetWidth == 16) {
                emitInstruction("mul %s", rightRegName);
            } else {
                emitInstruction("imul %s, %s", resultReg, rightRegName);
            }
            break;
        case BIN_DIV:
            emitInstruction("mov %s, %s", resultReg, leftRegName);
            if (codegen.targetWidth == 16) {
                emitInstruction("xor dx, dx"); // Clear DX for division
                emitInstruction("div %s", rightRegName);
            } else {
                const char* dxReg = getRegisterName(REG_DX, codegen.targetWidth);
                emitInstruction("xor %s, %s", dxReg, dxReg);
                emitInstruction("idiv %s", rightRegName);
            }
            break;
        case BIN_MOD:
            emitInstruction("mov %s, %s", resultReg, leftRegName);
            if (codegen.targetWidth == 16) {
                emitInstruction("xor dx, dx");
                emitInstruction("div %s", rightRegName);
                emitInstruction("mov ax, dx"); // Result is in DX
            } else {
                const char* dxReg = getRegisterName(REG_DX, codegen.targetWidth);
                emitInstruction("xor %s, %s", dxReg, dxReg);
                emitInstruction("idiv %s", rightRegName);
                emitInstruction("mov %s, %s", resultReg, dxReg);
            }
            break;
        case BIN_BITWISE_AND:
            emitInstruction("mov %s, %s", resultReg, leftRegName);
            emitInstruction("and %s, %s", resultReg, rightRegName);
            break;
        case BIN_BITWISE_OR:
            emitInstruction("mov %s, %s", resultReg, leftRegName);
            emitInstruction("or %s, %s", resultReg, rightRegName);
            break;
        case BIN_BITWISE_XOR:
            emitInstruction("mov %s, %s", resultReg, leftRegName);
            emitInstruction("xor %s, %s", resultReg, rightRegName);
            break;
        case BIN_LEFT_SHIFT:
            emitInstruction("mov %s, %s", resultReg, leftRegName);
            emitInstruction("mov cl, %s", getRegisterName(rightReg, 8)); // CL for shift count
            emitInstruction("shl %s, cl", resultReg);
            break;
        case BIN_RIGHT_SHIFT:
            emitInstruction("mov %s, %s", resultReg, leftRegName);
            emitInstruction("mov cl, %s", getRegisterName(rightReg, 8));
            emitInstruction("shr %s, cl", resultReg);
            break;
        case BIN_EQ:
        case BIN_NE:
        case BIN_LT:
        case BIN_LE:
        case BIN_GT:
        case BIN_GE:
            emitInstruction("cmp %s, %s", leftRegName, rightRegName);
            switch (op) {
                case BIN_EQ: emitInstruction("sete al"); break;
                case BIN_NE: emitInstruction("setne al"); break;
                case BIN_LT: emitInstruction("setl al"); break;
                case BIN_LE: emitInstruction("setle al"); break;
                case BIN_GT: emitInstruction("setg al"); break;
                case BIN_GE: emitInstruction("setge al"); break;
                default: break;
            }
            if (codegen.targetWidth == 16 && codegen.forceStrict16) {
                emitInstruction("xor ah, ah");
            } else {
                emitInstruction("movzx %s, al", resultReg);
            }
            break;
        default:
            codegenErrorSimple("Unsupported binary operator: %d", op);
            break;
    }
    
    freeRegister(leftReg);
    freeRegister(rightReg);
}

static void generateUnaryOp(ASTNode* node) {
    ASTNode* operand = node->data.unary.operand;
    UnaryOperator op = node->data.unary.op;
    
    generateExpression(operand);
    const char* reg = getRegisterName(REG_AX, codegen.targetWidth);
    
    switch (op) {
        case UNARY_MINUS:
            emitInstruction("neg %s", reg);
            break;
        case UNARY_PLUS:
            // No operation needed
            break;
        case UNARY_NOT:
            emitInstruction("test %s, %s", reg, reg);
            emitInstruction("setz al");
            if (codegen.targetWidth == 16 && codegen.forceStrict16) {
                emitInstruction("xor ah, ah");
            } else {
                emitInstruction("movzx %s, al", reg);
            }
            break;
        case UNARY_BITWISE_NOT:
            emitInstruction("not %s", reg);
            break;
        case UNARY_PRE_INCREMENT:
            emitInstruction("inc %s", reg);
            break;
        case UNARY_PRE_DECREMENT:
            emitInstruction("dec %s", reg);
            break;
        case UNARY_POST_INCREMENT:
            // Would need to handle differently - return original value
            emitInstruction("inc %s", reg);
            break;
        case UNARY_POST_DECREMENT:
            emitInstruction("dec %s", reg);
            break;
        case UNARY_ADDRESS_OF:
            // Would need symbol table to get address
            codegenErrorSimple("Address-of operator not fully implemented");
            break;
        case UNARY_DEREFERENCE:
            emitInstruction("mov %s, [%s]", reg, reg);
            break;
        default:
            codegenErrorSimple("Unsupported unary operator: %d", op);
            break;
    }
}

static void generateLiteral(ASTNode* node) {
    const char* reg = (codegen.targetWidth == 64) ? getRegisterName(REG_AX, 32)
                                                 : getRegisterName(REG_AX, codegen.targetWidth);
    
    switch (node->type) {
        case AST_INTEGER_LITERAL:
            emitInstruction("mov %s, %lld", reg, node->data.literal.intValue);
            break;
        case AST_FLOAT_LITERAL:
            // Floating point would need FPU instructions
            codegenErrorSimple("Floating point literals not fully implemented");
            break;
        case AST_CHAR_LITERAL:
            emitInstruction("mov %s, %d", reg, (int)node->data.literal.intValue);
            break;
        case AST_STRING_LITERAL: {
            char* label = addStringLiteral(node->data.literal.stringValue);
            emitInstruction("mov %s, %s", getRegisterName(REG_AX, codegen.targetWidth), label);
            free(label);
            break;
        }
        case AST_BOOL_LITERAL:
            emitInstruction("mov %s, %d", reg, node->data.literal.intValue ? 1 : 0);
            break;
        default:
            codegenErrorSimple("Unsupported literal type: %d", node->type);
            break;
    }
}

void generateFunctionCall(ASTNode* node) {
    // Optional: if function is an identifier and marked deprecated, warn
    if (node->data.call.function && node->data.call.function->type == AST_IDENTIFIER) {
        // We don't have symbol tables; attempt a heuristic: if there exists a function decl in AST it would be caught earlier.
        // Placeholder: do nothing here without symbol resolution.
    }
    // On x64 SysV, pass first 6 integer/pointer args in RDI, RSI, RDX, RCX, R8, R9; rest on stack.
    int wordSize = codegen.targetWidth / 8;
    int needAlignAdjust = 0;
    int argCount = node->data.call.argCount;
    Register argRegs64[] = { REG_DI, REG_SI, REG_DX, REG_CX, REG_R8, REG_R9 };
    int regArgCount = (codegen.targetWidth == 64) ? 6 : 0;

    if (codegen.targetWidth == 64) {
        // Determine number of stack args
        int stackArgs = argCount > regArgCount ? (argCount - regArgCount) : 0;
        // We'll push stack args (right-to-left) after evaluating them, but we also need alignment.
        // Each stack arg is 8 bytes. Stack before call must be 16B aligned.
        if (((stackArgs) % 2) != 0) {
            needAlignAdjust = wordSize; // 8 bytes pad
            const char* sp = getRegisterName(REG_SP, codegen.targetWidth);
            emitInstruction("sub %s, %d", sp, needAlignAdjust);
        }

        // Push stack args (those beyond the first 6), right-to-left
        for (int i = argCount - 1; i >= regArgCount; i--) {
            generateExpression(node->data.call.arguments[i]);
            emitInstruction("push %s", getRegisterName(REG_AX, 64));
        }

        // Load register args in order 0..5 (left-to-right)
        for (int i = 0; i < argCount && i < regArgCount; i++) {
            generateExpression(node->data.call.arguments[i]);
            // For integer types, zero-extend into 64-bit reg (use eax for move zero-extends to rax implicitly)
            // We don't have type info yet; zero/sign behavior TBD. Use 64-bit mov for now.
            emitInstruction("mov %s, %s", getRegisterName(argRegs64[i], 64), getRegisterName(REG_AX, 64));
        }
    } else {
        // Non-64-bit: push all args right-to-left
        for (int i = argCount - 1; i >= 0; i--) {
            generateExpression(node->data.call.arguments[i]);
            const char* reg = getRegisterName(REG_AX, codegen.targetWidth);
            emitInstruction("push %s", reg);
        }
    }
    
    // Generate function expression (should be identifier)
    if (node->data.call.function->type == AST_IDENTIFIER) {
        const char* funcName = node->data.call.function->data.identifier.name;
        if (funcName && funcName[0] != '_') {
            emitInstruction("call _%s", funcName);
        } else {
            emitInstruction("call %s", funcName);
        }
    } else {
        // Indirect call
        generateExpression(node->data.call.function);
        const char* reg = getRegisterName(REG_AX, codegen.targetWidth);
        emitInstruction("call %s", reg);
    }
    
    // Clean up stack (caller cleans up in most calling conventions)
    if (codegen.targetWidth == 64) {
        // Only stack-pushed args and any padding are cleaned here
        int stackArgs = argCount > regArgCount ? (argCount - regArgCount) : 0;
        int stackCleanup = stackArgs * wordSize + needAlignAdjust;
        if (stackCleanup) {
            const char* sp = getRegisterName(REG_SP, codegen.targetWidth);
            emitInstruction("add %s, %d", sp, stackCleanup);
        }
    } else if (node->data.call.argCount > 0 || needAlignAdjust) {
        int stackCleanup = argCount * wordSize + needAlignAdjust;
        const char* sp = getRegisterName(REG_SP, codegen.targetWidth);
        emitInstruction("add %s, %d", sp, stackCleanup);
    }
}

void generateArrayAccess(ASTNode* node) {
    // Generate array base address
    generateExpression(node->data.arrayAccess.array);
    Register baseReg = allocateRegister();
    const char* baseRegName = getRegisterName(baseReg, codegen.targetWidth);
    emitInstruction("mov %s, %s", baseRegName, getRegisterName(REG_AX, codegen.targetWidth));
    
    // Generate index
    generateExpression(node->data.arrayAccess.index);
    const char* indexReg = getRegisterName(REG_AX, codegen.targetWidth);
    
    // Calculate offset (assuming int elements for now)
    int elementSize = 4; // Would need type information
    if (elementSize > 1) {
        emitInstruction("imul %s, %d", indexReg, elementSize);
    }
    
    // Add to base address
    emitInstruction("add %s, %s", baseRegName, indexReg);
    
    // Load value
    emitInstruction("mov %s, [%s]", getRegisterName(REG_AX, codegen.targetWidth), baseRegName);
    
    freeRegister(baseReg);
}

void generateMemberAccess(ASTNode* node) {
    // Generate object address
    generateExpression(node->data.memberAccess.object);
    
    // For now, just treat as offset 0 (would need struct layout info)
    const char* reg = getRegisterName(REG_AX, codegen.targetWidth);
    
    if (node->data.memberAccess.isPointer) {
        // Pointer access: obj->member
        emitInstruction("mov %s, [%s]", reg, reg);
    }
    // Direct access: obj.member - would need member offset
    
    emitComment("Member access: %s", node->data.memberAccess.memberName);
}

void generateInlineAssembly(ASTNode* node) {
    emitComment("Inline assembly");

    const char* asmText = node->data.inlineAsm.assembly ? node->data.inlineAsm.assembly : "";

    // Minimal support for one operand mapped to AX:
    // - If single input operand present: load that C variable into AX before asm
    // - If single output operand present: replace %0 with AX and store AX into C variable after asm
    char replaced[1024];
    replaced[0] = '\0';
    // Pre-load input operand into AX for templates using %0 when only an input is provided
    if (node->data.inlineAsm.outputCount == 0 && node->data.inlineAsm.inputCount == 1) {
        const char* inName = node->data.inlineAsm.inputOperands ? node->data.inlineAsm.inputOperands[0] : NULL;
        const char* iconst = node->data.inlineAsm.inputConstraints ? node->data.inlineAsm.inputConstraints[0] : NULL;
        if (inName && *inName) {
            int size = 4;
            if (iconst && strchr(iconst, 'q')) size = 1; // use 'q' as byte in our minimal dialect
            const char* axs = ax_by_size(size);
            const char* bp = getRegisterName(REG_BP, codegen.targetWidth);
            int off, poff; Register rhome;
            if (findParamOffset(inName, &poff)) {
                emitInstruction("mov %s, [%s+%d]", axs, bp, poff);
            } else if (findLocalRegister(inName, &rhome)) {
                emitInstruction("mov %s, %s", axs, reg_by_size(rhome, size));
            } else if (findLocalOffset(inName, &off)) {
                emitInstruction("mov %s, [%s-%d]", axs, bp, off);
            } else {
                emitInstruction("mov %s, [%s]", axs, inName);
            }
        }
    }

    if (node->data.inlineAsm.outputCount == 1) {
        const char* outName = node->data.inlineAsm.outputOperands ? node->data.inlineAsm.outputOperands[0] : NULL;
        const char* oconst = node->data.inlineAsm.outputConstraints ? node->data.inlineAsm.outputConstraints[0] : NULL;
        // Replace %0 in the template with AX at appropriate width
        int rsize = 4; if (oconst && strchr(oconst, 'q')) rsize = 1; if (codegen.targetWidth==16 && rsize!=1) rsize=2;
        const char* ax = ax_by_size(rsize);
        const char* p = asmText;
        size_t len = 0;
        while (*p && len + 4 < sizeof(replaced)) {
            if (p[0] == '%' && p[1] == '0') {
                // Insert register name
                size_t rl = strlen(ax);
                if (len + rl >= sizeof(replaced) - 1) break;
                memcpy(&replaced[len], ax, rl);
                len += rl; p += 2;
            } else {
                replaced[len++] = *p++;
            }
        }
        replaced[len] = '\0';
        fprintf(codegen.output, "    %s\n", (replaced[0] ? replaced : asmText));

        // If we have an out variable, store AX into it
        if (outName && *outName) {
            int size = 4; // default width until types integrated
            if (oconst && strchr(oconst, 'q')) size = 1;
            const char* axs = ax_by_size(size);
            const char* bp = getRegisterName(REG_BP, codegen.targetWidth);
            int off, poff; Register rhome;
            if (findParamOffset(outName, &poff)) {
                emitInstruction("mov [%s+%d], %s", bp, poff, axs);
            } else if (findLocalRegister(outName, &rhome)) {
                emitInstruction("mov %s, %s", reg_by_size(rhome, size), axs);
            } else if (findLocalOffset(outName, &off)) {
                emitInstruction("mov [%s-%d], %s", bp, off, axs);
            } else {
                emitInstruction("mov [%s], %s", outName, axs);
            }
        }
        return;
    }

    // No mapped outputs: still replace %0 with AX if single input was preloaded
    // so templates like "mov di, %0" use AX.
    {
        const char* p = asmText; char replaced2[1024]; size_t len2 = 0;
        int rsize = 4;
        if (node->data.inlineAsm.inputCount == 1) {
            const char* iconst = node->data.inlineAsm.inputConstraints ? node->data.inlineAsm.inputConstraints[0] : NULL;
            if (iconst && strchr(iconst, 'q')) rsize = 1;
            if (codegen.targetWidth==16 && rsize!=1) rsize=2;
        } else if (codegen.targetWidth==16) {
            rsize = 2;
        }
        const char* ax = ax_by_size(rsize);
        int did = 0;
        while (*p && len2 + 4 < sizeof(replaced2)) {
            if (p[0] == '%' && p[1] == '0') {
                size_t rl = strlen(ax);
                if (len2 + rl >= sizeof(replaced2) - 1) break;
                memcpy(&replaced2[len2], ax, rl);
                len2 += rl; p += 2; did = 1;
            } else {
                replaced2[len2++] = *p++;
            }
        }
        replaced2[len2] = '\0';
        fprintf(codegen.output, "    %s\n", did ? replaced2 : asmText);
        return;
    }
}

void generateExpression(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_BINARY_OP:
            generateBinaryOp(node);
            break;
        case AST_TERNARY_OP: {
            // condition ? trueExpr : falseExpr
            const char* ax = ax_by_size(4);
            char lTrue[32], lFalse[32], lEnd[32];
            static int tcount = 0; int id = tcount++;
            snprintf(lTrue, sizeof lTrue, ".L_ternary_true_%d", id);
            snprintf(lFalse, sizeof lFalse, ".L_ternary_false_%d", id);
            snprintf(lEnd, sizeof lEnd, ".L_ternary_end_%d", id);
            // Evaluate condition
            generateExpression(node->data.ternary.condition);
            // Compare against zero and branch
            emitInstruction("cmp %s, 0", ax);
            emitInstruction("je %s", lFalse);
            // True branch
            emitLabel(lTrue);
            generateExpression(node->data.ternary.trueExpr);
            emitInstruction("jmp %s", lEnd);
            // False branch
            emitLabel(lFalse);
            generateExpression(node->data.ternary.falseExpr);
            // End
            emitLabel(lEnd);
            break;
        }
        case AST_UNARY_OP:
            generateUnaryOp(node);
            break;
        case AST_INTEGER_LITERAL:
        case AST_FLOAT_LITERAL:
        case AST_CHAR_LITERAL:
        case AST_STRING_LITERAL:
        case AST_BOOL_LITERAL:
            generateLiteral(node);
            break;
    case AST_IDENTIFIER:
            // Load variable value
            emitComment("Load variable: %s", node->data.identifier.name);
            {
        int size = 4; // TODO: read from node->typeInfo
        const char* ax = ax_by_size(size);
                const char* bp = getRegisterName(REG_BP, codegen.targetWidth);
                int off;
                int poff;
                Register rhome;
                if (findParamOffset(node->data.identifier.name, &poff)) {
                    emitInstruction("mov %s, [%s+%d]", ax, bp, poff);
                } else if (findLocalRegister(node->data.identifier.name, &rhome)) {
                    // If the home is AX/EAX already, skip the move
                    if (!(rhome == REG_AX)) {
                        emitInstruction("mov %s, %s", ax, reg_by_size(rhome, size));
                    }
                } else if (findLocalOffset(node->data.identifier.name, &off)) {
                    emitInstruction("mov %s, [%s-%d]", ax, bp, off);
                } else {
                    // Fallback to global symbol address
                    emitInstruction("mov %s, [%s]", ax, node->data.identifier.name);
                }
            }
            break;
        case AST_FUNCTION_CALL:
            generateFunctionCall(node);
            break;
        case AST_ARRAY_ACCESS:
            generateArrayAccess(node);
            break;
        case AST_MEMBER_ACCESS:
        case AST_POINTER_ACCESS:
            generateMemberAccess(node);
            break;
        case AST_SIZEOF:
            // Would need type information to calculate size
            emitInstruction("mov %s, 4", getRegisterName(REG_AX, codegen.targetWidth));
            emitComment("sizeof - using placeholder value");
            break;
        case AST_INLINE_ASM:
            generateInlineAssembly(node);
            break;
        default:
            codegenErrorSimple("Unsupported expression type: %d", node->type);
            break;
    }
}

void generateIfStatement(ASTNode* node) {
    char* elseLabel = malloc(32);
    char* endLabel = malloc(32);
    snprintf(elseLabel, 32, "_else_%d", codegen.labelCount);
    snprintf(endLabel, 32, "_end_if_%d", codegen.labelCount);
    codegen.labelCount++;
    
    // Generate condition
    generateExpression(node->data.ifStmt.condition);
    
    // Test condition
    const char* reg = getRegisterName(REG_AX, codegen.targetWidth);
    emitInstruction("test %s, %s", reg, reg);
    
    if (node->data.ifStmt.elseStmt) {
        emitInstruction("jz %s", elseLabel);
        generateStatement(node->data.ifStmt.thenStmt);
        emitInstruction("jmp %s", endLabel);
        emitLabel(elseLabel);
        generateStatement(node->data.ifStmt.elseStmt);
        emitLabel(endLabel);
    } else {
        emitInstruction("jz %s", endLabel);
        generateStatement(node->data.ifStmt.thenStmt);
        emitLabel(endLabel);
    }
    
    free(elseLabel);
    free(endLabel);
}

void generateWhileStatement(ASTNode* node) {
    char* loopLabel = malloc(32);
    char* endLabel = malloc(32);
    snprintf(loopLabel, 32, "_loop_%d", codegen.labelCount);
    snprintf(endLabel, 32, "_end_loop_%d", codegen.labelCount);
    codegen.labelCount++;
    
    emitLabel(loopLabel);
    
    // Generate condition
    generateExpression(node->data.whileStmt.condition);
    
    // Test condition
    const char* reg = getRegisterName(REG_AX, codegen.targetWidth);
    emitInstruction("test %s, %s", reg, reg);
    emitInstruction("jz %s", endLabel);
    
    // Generate body
    generateStatement(node->data.whileStmt.body);
    
    emitInstruction("jmp %s", loopLabel);
    emitLabel(endLabel);
    
    free(loopLabel);
    free(endLabel);
}

void generateForStatement(ASTNode* node) {
    char* loopLabel = malloc(32);
    char* continueLabel = malloc(32);
    char* endLabel = malloc(32);
    snprintf(loopLabel, 32, "_for_loop_%d", codegen.labelCount);
    snprintf(continueLabel, 32, "_for_continue_%d", codegen.labelCount);
    snprintf(endLabel, 32, "_for_end_%d", codegen.labelCount);
    codegen.labelCount++;
    
    // Initialization
    if (node->data.forStmt.init) {
        if (node->data.forStmt.init->type == AST_VARIABLE_DECL) {
            generateStatement(node->data.forStmt.init);
        } else {
            generateExpression(node->data.forStmt.init);
        }
    }
    
    emitLabel(loopLabel);
    
    // Condition
    if (node->data.forStmt.condition) {
        generateExpression(node->data.forStmt.condition);
        const char* reg = getRegisterName(REG_AX, codegen.targetWidth);
        emitInstruction("test %s, %s", reg, reg);
        emitInstruction("jz %s", endLabel);
    }
    
    // Body
    generateStatement(node->data.forStmt.body);
    
    emitLabel(continueLabel);
    
    // Update
    if (node->data.forStmt.update) {
        generateExpression(node->data.forStmt.update);
    }
    
    emitInstruction("jmp %s", loopLabel);
    emitLabel(endLabel);
    
    free(loopLabel);
    free(continueLabel);
    free(endLabel);
}

void generateReturnStatement(ASTNode* node) {
    if (node->data.returnStmt.expression) {
        generateExpression(node->data.returnStmt.expression);
        // Result is in AX/EAX/RAX
    }
    
    // Jump to the unified function epilogue
    if (codegen.currentFunctionEpilogueLabel) {
        emitInstruction("jmp %s", codegen.currentFunctionEpilogueLabel);
    } else {
        // Fallback if called outside of a proper function context
        const char* bp = getRegisterName(REG_BP, codegen.targetWidth);
        const char* sp = getRegisterName(REG_SP, codegen.targetWidth);
        emitInstruction("mov %s, %s", sp, bp);
        emitInstruction("pop %s", bp);
        emitInstruction("ret");
    }
}

void generateCompoundStatement(ASTNode* node) {
    for (int i = 0; i < node->data.compound.count; i++) {
        generateStatement(node->data.compound.statements[i]);
    }
}

void generateVariableDeclaration(ASTNode* node) {
    emitComment("Variable declaration: %s", node->data.varDecl.name);

    // If we're not inside a function, this is a global (file-scope) declaration.
    if (!codegen.currentFunctionEpilogueLabel) {
        // Handle externs: declare but do not define
        if (node->data.varDecl.storageClass & STORAGE_EXTERN) {
            if (node->data.varDecl.name) {
                emitNasExtern(node->data.varDecl.name);
            }
            return;
        }

        // Emit into data (simple model; no .bss split yet)
        emitNasSection(".data");

        const char* name = node->data.varDecl.name ? node->data.varDecl.name : "_anon_global";
        emitLabel(name);

        int size = getTypeSize(node->data.varDecl.type, codegen.targetWidth);
        if (size <= 0) size = (codegen.targetWidth / 8);

        // Helper lambdas (as static inline local functions are not available in C89)
        // Emit a sized constant
        if (node->data.varDecl.initializer) {
            ASTNode* init = node->data.varDecl.initializer;
            // Special-case: char array initialized with string literal
            if (node->data.varDecl.type && node->data.varDecl.type->baseType == TYPE_ARRAY &&
                node->data.varDecl.type->elementType && node->data.varDecl.type->elementType->baseType == TYPE_CHAR &&
                init->type == AST_STRING_LITERAL) {
                // Emit the bytes and trailing 0; pad to array size if specified
                const char* s = init->data.literal.stringValue ? init->data.literal.stringValue : "\"\"";
                int strLen = 0;
                unsigned char* bytes = decodeStringTokenToBytes(s, &strLen);
                if (!bytes) {
                    bytes = (unsigned char*)malloc(1);
                    strLen = 0;
                }
                // Emit numeric bytes and trailing NUL
                if (strLen > 0) {
                    emitDbBytes(bytes, strLen);
                    fprintf(codegen.output, ", 0\n");
                } else {
                    fprintf(codegen.output, "    #db 0\n");
                }
                int used = strLen + 1; // include NUL
                int total = size;
                for (int i = used; i < total; i++) {
                    fprintf(codegen.output, "    #db 0\n");
                }
                free(bytes);
                return;
            }

            // Simple scalar constant initializers
            if (init->type == AST_INTEGER_LITERAL || init->type == AST_CHAR_LITERAL || init->type == AST_BOOL_LITERAL) {
                long long imm = init->data.literal.intValue;
                if (size == 1) {
                    fprintf(codegen.output, "    #db %lld\n", imm & 0xFF);
                } else if (size == 2) {
                    fprintf(codegen.output, "    #dw %lld\n", imm & 0xFFFF);
                } else if (size == 4) {
                    fprintf(codegen.output, "    #dd %lld\n", imm & 0xFFFFFFFFLL);
                } else {
                    // 8 or larger: emit dq for first word then zero-fill remainder if any
                    fprintf(codegen.output, "    #dq %lld\n", imm);
                    for (int i = 8; i < size; i++) {
                        fprintf(codegen.output, "    #db 0\n");
                    }
                }
                return;
            }

            // Pointer to string literal: emit pointer to pooled string
            if (init->type == AST_STRING_LITERAL && node->data.varDecl.type && node->data.varDecl.type->baseType == TYPE_POINTER) {
                char* lbl = addStringLiteral(init->data.literal.stringValue ? init->data.literal.stringValue : "");
                if (size == 2) {
                    fprintf(codegen.output, "    #dw %s\n", lbl);
                } else if (size == 4) {
                    fprintf(codegen.output, "    #dd %s\n", lbl);
                } else {
                    fprintf(codegen.output, "    #dq %s\n", lbl);
                }
                free(lbl);
                return;
            }

            // Fallback: unsupported initializer at global scope; zero-initialize
        }

        // No initializer or unsupported one: zero-fill
        for (int i = 0; i < size; i++) {
            fprintf(codegen.output, "    #db 0\n");
        }
        return;
    }

    // Otherwise, local (function-scope) declaration
    int size = getTypeSize(node->data.varDecl.type, codegen.targetWidth);
    // Simple heuristic: try to place small scalars in a caller-saved register
    Register homeReg;
    int placedInReg = 0;
    if ((codegen.targetWidth == 64 && codegen.optimizationLevel >= 1 && (size == 1 || size == 2 || size == 4)) ||
        (codegen.targetWidth == 32 && codegen.optimizationLevel >= 1 && size == 4) ||
        (codegen.targetWidth == 16 && codegen.optimizationLevel >= 2 && size == 2)) {
        placedInReg = addRegLocal(node->data.varDecl.name, size, &homeReg);
    }
    int offset = 0;
    if (!placedInReg) {
        // Stack local path
        const char* sp = getRegisterName(REG_SP, codegen.targetWidth);
        emitInstruction("sub %s, %d", sp, size);
        offset = addLocal(node->data.varDecl.name, size);
        // Maintain 16-byte alignment of RSP after each allocation (SysV AMD64)
        int mis = currentStackOffset % 16;
        if (mis != 0) {
            int pad = 16 - mis;
            emitInstruction("sub %s, %d", sp, pad);
            currentStackOffset += pad;
        }
    }

    // Initialize if there's an initializer
    if (node->data.varDecl.initializer) {
        if (placedInReg && (node->data.varDecl.initializer->type == AST_INTEGER_LITERAL ||
                            node->data.varDecl.initializer->type == AST_CHAR_LITERAL ||
                            node->data.varDecl.initializer->type == AST_BOOL_LITERAL)) {
            long long imm = node->data.varDecl.initializer->data.literal.intValue;
            emitInstruction("mov %s, %lld", reg_by_size(homeReg, size), imm);
        } else {
            generateExpression(node->data.varDecl.initializer);
            const char* val = ax_by_size(size);
            if (placedInReg) {
                emitInstruction("mov %s, %s", reg_by_size(homeReg, size), val);
            } else {
                const char* bp = getRegisterName(REG_BP, codegen.targetWidth);
                emitInstruction("mov [%s-%d], %s", bp, offset, val);
            }
        }
    }
}

void generateFunctionDeclaration(ASTNode* node) {
    // Function label (prefix underscore for C-style symbol names)
    const char* rawName = node->data.funcDecl.name;
    if (rawName && rawName[0] != '_') {
        char prefixed[512];
        snprintf(prefixed, sizeof(prefixed), "_%s", rawName);
        emitLabel(prefixed);
        // Capture anchor for special marker names
        if (strcmp(prefixed, "__NCC_STRING_LOC") == 0 || strcmp(prefixed, "_NCC_STRING_LOC") == 0) {
            // Set anchor and emit strings here if any and not yet emitted
            codegen.stringAnchorLabel = strdup(prefixed);
        }
    } else {
        emitLabel(rawName);
        if (rawName && (strcmp(rawName, "__NCC_STRING_LOC") == 0 || strcmp(rawName, "_NCC_STRING_LOC") == 0)) {
            codegen.stringAnchorLabel = strdup(rawName);
        }
    }
    
    // Function prologue (skip if [[naked]])
    const char* bp = getRegisterName(REG_BP, codegen.targetWidth);
    const char* sp = getRegisterName(REG_SP, codegen.targetWidth);
    int isNaked = node->data.funcDecl.isNaked;
    if (!isNaked) {
        emitInstruction("push %s", bp);
        emitInstruction("mov %s, %s", bp, sp);
        // Preserve callee-saved RBX on x64 (we frequently use it for temps)
        savedRBXForThisFunction = 0;
        if (codegen.targetWidth == 64) {
            emitInstruction("push rbx");
            savedRBXForThisFunction = 1;
        }
    }
    // Reset locals for this function scope
    resetLocals();
    // After establishing frame, record parameter homes: on x64 SysV use registers for first 6 args
    if (node->data.funcDecl.paramCount > 0 && node->data.funcDecl.parameters) {
        int slotSize = codegen.targetWidth / 8;
        int firstParamOff;
        if (codegen.targetWidth == 64) {
            // Return addr at +8; if any args spilled to stack (beyond 6), they start at +16
            firstParamOff = 16;
        } else if (codegen.targetWidth == 32) {
            firstParamOff = 8;
        } else {
            firstParamOff = 4;
        }
        params = (ParamEntry*)malloc(sizeof(ParamEntry) * node->data.funcDecl.paramCount);
        paramCountTracked = 0;

        if (codegen.targetWidth == 64) {
            Register argRegs64[] = { REG_DI, REG_SI, REG_DX, REG_CX, REG_R8, REG_R9 };
            int regArgCount = 6;
            int off = firstParamOff;
            for (int i = 0; i < node->data.funcDecl.paramCount; i++) {
                Parameter* p = node->data.funcDecl.parameters[i];
                if (!p || !p->name) continue;
                if (i < regArgCount) {
                    // Home in the incoming argument register
                    addExistingRegLocal(p->name, argRegs64[i]);
                } else {
                    // Stack arg at [bp+off]
                    params[paramCountTracked].name = strdup(p->name);
                    params[paramCountTracked].offset = off;
                    paramCountTracked++;
                    off += slotSize;
                }
            }
        } else {
            int off = firstParamOff;
            for (int i = 0; i < node->data.funcDecl.paramCount; i++) {
                Parameter* p = node->data.funcDecl.parameters[i];
                if (!p || !p->name) continue;
                params[paramCountTracked].name = strdup(p->name);
                params[paramCountTracked].offset = off;
                paramCountTracked++;
                off += slotSize;
            }
        }
    }
    // Prepare unified epilogue label for this function
    char epilogueLabelBuf[32];
    snprintf(epilogueLabelBuf, sizeof(epilogueLabelBuf), "_func_epilogue_%d", codegen.labelCount++);
    char* savedEpilogue = codegen.currentFunctionEpilogueLabel;
    codegen.currentFunctionEpilogueLabel = strdup(epilogueLabelBuf);
    
    // Generate function body
    if (node->data.funcDecl.body) {
        generateStatement(node->data.funcDecl.body);
    }

    // Unified function epilogue (handles both implicit and explicit returns)
    emitLabel(codegen.currentFunctionEpilogueLabel);
    if (!isNaked) {
        if (savedRBXForThisFunction && codegen.targetWidth == 64) {
            // Reset stack to base, step to saved rbx, restore, then pop rbp
            emitInstruction("mov %s, %s", sp, bp);
            emitInstruction("sub %s, 8", sp);
            emitInstruction("pop rbx");
            emitInstruction("pop %s", bp);
            emitInstruction("ret");
        } else {
            emitInstruction("mov %s, %s", sp, bp);
            emitInstruction("pop %s", bp);
            emitInstruction("ret");
        }
    } else {
        // Naked: absolutely nothing
    }

    // Restore prior context
    free(codegen.currentFunctionEpilogueLabel);
    codegen.currentFunctionEpilogueLabel = savedEpilogue;
    // Ensure locals are cleared for next function (already reset at start)
    resetLocals();
    savedRBXForThisFunction = 0;

    // After emitting the marker function body, if this is the string anchor and not yet emitted, flush strings
    if (!codegen.stringsEmitted) {
        const char* nameToCheck = rawName && rawName[0] ? rawName : NULL;
        if (nameToCheck && (
            strcmp(nameToCheck, "_NCC_STRING_LOC") == 0 || strcmp(nameToCheck, "__NCC_STRING_LOC") == 0)) {
            // Emit pending string literals right here
            StringLiteral* current = codegen.stringLiterals;
            while (current) {
                emitLabel(current->label);
                int blen = 0;
                unsigned char* b = decodeStringTokenToBytes(current->value, &blen);
                if (b && blen > 0) {
                    emitDbBytes(b, blen);
                    fprintf(codegen.output, ", 0\n");
                } else {
                    fprintf(codegen.output, "    #db 0\n");
                }
                if (b) free(b);
                current = current->next;
            }
            codegen.stringsEmitted = 1;
        }
    }
}

void generateStatement(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_EXPRESSION_STMT:
            generateExpression(node->data.returnStmt.expression);
            break;
        case AST_IF_STMT:
            generateIfStatement(node);
            break;
        case AST_WHILE_STMT:
            generateWhileStatement(node);
            break;
        case AST_FOR_STMT:
            generateForStatement(node);
            break;
        case AST_RETURN_STMT:
            generateReturnStatement(node);
            break;
        case AST_COMPOUND_STMT:
            generateCompoundStatement(node);
            break;
        case AST_VARIABLE_DECL:
            generateVariableDeclaration(node);
            break;
        case AST_FUNCTION_DECL:
            generateFunctionDeclaration(node);
            break;
        case AST_BREAK_STMT:
            emitComment("Break statement - would need loop context");
            break;
        case AST_CONTINUE_STMT:
            emitComment("Continue statement - would need loop context");
            break;
        case AST_INLINE_ASM:
            generateInlineAssembly(node);
            break;
        default:
            codegenErrorSimple("Unsupported statement type: %d", node->type);
            break;
    }
}

// Public interface
void initCodeGenerator(TargetArch arch, OutputFormat format, FILE* output, unsigned long long originAddress) {
    memset(&codegen, 0, sizeof(CodeGenerator));
    
    codegen.targetArch = arch;
    codegen.outputFormat = format;
    codegen.output = output;
    
    switch (arch) {
        case ARCH_X86_16: codegen.targetWidth = 16; break;
        case ARCH_X86_32: codegen.targetWidth = 32; break;
        case ARCH_X86_64: codegen.targetWidth = 64; break;
        default: codegen.targetWidth = 32; break;
    }
    
    codegen.labelCount = 0;
    codegen.stringLiteralCount = 0;
    codegen.instructionCount = 0;
    codegen.stringLiterals = NULL;
    codegen.stringsEmitted = 0;
    codegen.stringAnchorLabel = NULL;
    
    // Initialize options
    codegen.optimizationLevel = 0;
    codegen.forceStrict16 = 0;

    // Initialize register allocation
    for (int i = 0; i < MAX_REGISTERS; i++) {
        codegen.registerInUse[i] = 0;
    }
    
    // Emit NAS directives
    emitNasWidth(codegen.targetWidth);
    emitNasOrigin(originAddress);

    if (format == FORMAT_ELF) {
        emitNasSection(".text");
    }
}

void cleanupCodeGenerator(void) {
    // Clean up string literals
    StringLiteral* current = codegen.stringLiterals;
    while (current) {
        StringLiteral* next = current->next;
        free(current->label);
        free(current->value);
        free(current);
        current = next;
    }
    
    memset(&codegen, 0, sizeof(CodeGenerator));
}

void generateCode(ASTNode* ast) {
    if (!ast) return;
    
    // Generate main program
    if (ast->type == AST_PROGRAM) {
        for (int i = 0; i < ast->data.compound.count; i++) {
            generateStatement(ast->data.compound.statements[i]);
        }
    } else {
        generateStatement(ast);
    }
    
    // Emit string literals section (only if not already emitted via _NCC_STRING_LOC)
    if (codegen.stringLiterals && !codegen.stringsEmitted) {
        emitNasSection(".data");
        
        StringLiteral* current = codegen.stringLiterals;
        while (current) {
            emitLabel(current->label);
            int blen = 0;
            unsigned char* b = decodeStringTokenToBytes(current->value, &blen);
            if (b && blen > 0) {
                emitDbBytes(b, blen);
                fprintf(codegen.output, ", 0\n");
            } else {
                fprintf(codegen.output, "    #db 0\n");
            }
            if (b) free(b);
            current = current->next;
        }
        codegen.stringsEmitted = 1;
    }
}

void emitAssembly(const char* instruction) {
    fprintf(codegen.output, "    %s\n", instruction);
    codegen.instructionCount++;
}

void emitAssemblyLabel(const char* label) {
    emitLabel(label);
}

int getTargetWidth(void) {
    return codegen.targetWidth;
}

TargetArch getTargetArch(void) {
    return codegen.targetArch;
}

OutputFormat getOutputFormat(void) {
    return codegen.outputFormat;
}

// Simple finalizeCodeGen function
void finalizeCodeGen(void) {
    if (codegen.output && codegen.output != stdout) {
        fclose(codegen.output);
        codegen.output = NULL;
    }
}

// Wrapper functions for main.c compatibility
void initCodeGen(const char* outputFilename, unsigned long long originAddress, TargetArch targetArch) {
    FILE* output = stdout;
    if (outputFilename && strcmp(outputFilename, "-") != 0) {
        output = fopen(outputFilename, "w");
        if (!output) {
            fprintf(stderr, "Error: Cannot open output file %s\n", outputFilename);
            exit(1);
        }
    }
    initCodeGenerator(targetArch, FORMAT_FLAT, output, originAddress);
    // Store origin address somewhere if needed
    
}

void setTargetWidth(int width) {
    switch (width) {
        case 16: codegen.targetArch = ARCH_X86_16; break;
        case 32: codegen.targetArch = ARCH_X86_32; break;
        case 64: codegen.targetArch = ARCH_X86_64; break;
        default: codegen.targetArch = ARCH_X86_32; break;
    }
}

void setDebugMode(int generateDebugInfo) {
    // Store debug mode somewhere if needed
    (void)generateDebugInfo; // Silence unused parameter warning
}

// Additional wrapper functions
void setOptimizationLevel(OptimizationLevel level, int debugMode) {
    // Store optimization level somewhere if needed  
    codegen.optimizationLevel = (int)level;
    (void)debugMode;
}

void setOutputFormat(OutputFormat format) {
    codegen.outputFormat = format;
}

void setCStandard(CStandard standard) {
    // Store C standard somewhere if needed
    (void)standard;
}

void setWarningMode(int enableWarnings, int warningsAsErrors) {
    // Store warning mode somewhere if needed
    (void)enableWarnings;
    (void)warningsAsErrors;
}

void setForceStrict16(int enable) {
    codegen.forceStrict16 = enable ? 1 : 0;
}
