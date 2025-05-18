#include "preprocessor.h"
#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Forward declarations
static int evaluateExpression(const char** expr);
static int evaluateTerm(const char** expr);
static int evaluateFactor(const char** expr);

// Skip whitespace in an expression
static void skipWhitespace(const char** expr) {
    while (**expr && isspace(**expr)) {
        (*expr)++;
    }
}

// Check if a macro is defined in the preprocessor expression
static int evaluateDefinedOperator(const char** expr) {
    skipWhitespace(expr);
    
    int hasParen = 0;
    if (**expr == '(') {
        (*expr)++;
        skipWhitespace(expr);
        hasParen = 1;
    }
    
    // Extract the macro name
    char macroName[MAX_MACRO_NAME_LEN];
    int nameLen = 0;
    
    while (**expr && (isalnum(**expr) || **expr == '_')) {
        if (nameLen < MAX_MACRO_NAME_LEN - 1) {
            macroName[nameLen++] = **expr;
        }
        (*expr)++;
    }
    macroName[nameLen] = '\0';
    
    if (hasParen) {
        skipWhitespace(expr);
        if (**expr == ')') {
            (*expr)++;
        } else {
            fprintf(stderr, "Error: Missing closing parenthesis in defined() operator\n");
        }
    }
    
    // Check if the macro is defined
    return isMacroDefined(macroName);
}

// Get the size of a type
static int evaluateSizeofOperator(const char** expr) {
    skipWhitespace(expr);
    
    if (**expr != '(') {
        fprintf(stderr, "Error: Expected opening parenthesis after sizeof\n");
        return 0;
    }
    (*expr)++;
    skipWhitespace(expr);
    
    // Extract the type name
    char typeName[MAX_MACRO_NAME_LEN];
    int nameLen = 0;
    
    while (**expr && **expr != ')') {
        if (nameLen < MAX_MACRO_NAME_LEN - 1) {
            typeName[nameLen++] = **expr;
        }
        (*expr)++;
    }
    typeName[nameLen] = '\0';
    
    if (**expr == ')') {
        (*expr)++;
    } else {
        fprintf(stderr, "Error: Missing closing parenthesis in sizeof() operator\n");
    }
    
    // Determine size based on type name
    if (strcmp(typeName, "char") == 0 || strcmp(typeName, "unsigned char") == 0) {
        return 1;
    } else if (strcmp(typeName, "short") == 0 || strcmp(typeName, "unsigned short") == 0) {
        return 2;
    } else if (strcmp(typeName, "int") == 0 || strcmp(typeName, "unsigned int") == 0 ||
              strcmp(typeName, "long") == 0 || strcmp(typeName, "unsigned long") == 0) {
        return 2;  // 16-bit integers in the target architecture
    } else if (strstr(typeName, "*") != NULL) {
        return 2;  // Pointers are 16-bit (near pointers)
    } else if (strcmp(typeName, "void") == 0) {
        return 0;
    } else {
        fprintf(stderr, "Warning: Unknown type '%s' in sizeof(), assuming 2 bytes\n", typeName);
        return 2;
    }
}

// Parse a number
static int parseNumber(const char** expr) {
    int value = 0;
    int base = 10;
    
    // Check for hexadecimal format
    if (**expr == '0' && ((*expr)[1] == 'x' || (*expr)[1] == 'X')) {
        (*expr) += 2;
        base = 16;
    }
    
    // Parse the number
    while (1) {
        if (base == 10 && isdigit(**expr)) {
            value = value * 10 + (**expr - '0');
            (*expr)++;
        } else if (base == 16 && isxdigit(**expr)) {
            char c = tolower(**expr);
            if (isdigit(c)) {
                value = value * 16 + (c - '0');
            } else {
                value = value * 16 + (c - 'a' + 10);
            }
            (*expr)++;
        } else {
            break;
        }
    }
    
    return value;
}

// Parse an expression factor (number, identifier, or subexpression)
static int evaluateFactor(const char** expr) {
    skipWhitespace(expr);
    
    if (**expr == '(') {
        (*expr)++;
        int value = evaluateExpression(expr);
        skipWhitespace(expr);
        if (**expr == ')') {
            (*expr)++;
        } else {
            fprintf(stderr, "Error: Missing closing parenthesis in expression\n");
        }
        return value;
    } else if (isdigit(**expr)) {
        return parseNumber(expr);
    } else if (strncmp(*expr, "defined", 7) == 0 && (isspace((*expr)[7]) || (*expr)[7] == '(')) {
        (*expr) += 7;
        return evaluateDefinedOperator(expr);
    } else if (strncmp(*expr, "sizeof", 6) == 0 && (isspace((*expr)[6]) || (*expr)[6] == '(')) {
        (*expr) += 6;
        return evaluateSizeofOperator(expr);
    } else if (isalpha(**expr) || **expr == '_') {
        // Identifiers are treated as macros
        char macroName[MAX_MACRO_NAME_LEN];
        int nameLen = 0;
        
        while (**expr && (isalnum(**expr) || **expr == '_')) {
            if (nameLen < MAX_MACRO_NAME_LEN - 1) {
                macroName[nameLen++] = **expr;
            }
            (*expr)++;
        }
        macroName[nameLen] = '\0';
        
        // Get the macro value and convert to integer
        const char* value = getMacroValue(macroName);
        if (value) {
            return atoi(value);
        } else {
            // Undefined macros are treated as 0
            return 0;
        }
    } else if (**expr == '!') {
        (*expr)++;
        return !evaluateFactor(expr);
    } else if (**expr == '~') {
        (*expr)++;
        return ~evaluateFactor(expr);
    } else if (**expr == '-') {
        (*expr)++;
        return -evaluateFactor(expr);
    }
    
    fprintf(stderr, "Error: Unexpected character in preprocessor expression: %c\n", **expr);
    return 0;
}

// Parse a term (factor * factor, factor / factor, factor % factor)
static int evaluateTerm(const char** expr) {
    int left = evaluateFactor(expr);
    
    while (1) {
        skipWhitespace(expr);
        
        if (**expr == '*') {
            (*expr)++;
            left *= evaluateFactor(expr);
        } else if (**expr == '/') {
            (*expr)++;
            int right = evaluateFactor(expr);
            if (right == 0) {
                fprintf(stderr, "Error: Division by zero in preprocessor expression\n");
                return 0;
            }
            left /= right;
        } else if (**expr == '%') {
            (*expr)++;
            int right = evaluateFactor(expr);
            if (right == 0) {
                fprintf(stderr, "Error: Modulo by zero in preprocessor expression\n");
                return 0;
            }
            left %= right;
        } else {
            break;
        }
    }
    
    return left;
}

// Parse an additive expression (term + term, term - term)
static int evaluateAddExpr(const char** expr) {
    int left = evaluateTerm(expr);
    
    while (1) {
        skipWhitespace(expr);
        
        if (**expr == '+') {
            (*expr)++;
            left += evaluateTerm(expr);
        } else if (**expr == '-') {
            (*expr)++;
            left -= evaluateTerm(expr);
        } else {
            break;
        }
    }
    
    return left;
}

// Parse a shift expression (addExpr << addExpr, addExpr >> addExpr)
static int evaluateShiftExpr(const char** expr) {
    int left = evaluateAddExpr(expr);
    
    while (1) {
        skipWhitespace(expr);
        
        if (**expr == '<' && (*expr)[1] == '<') {
            (*expr) += 2;
            left <<= evaluateAddExpr(expr);
        } else if (**expr == '>' && (*expr)[1] == '>') {
            (*expr) += 2;
            left >>= evaluateAddExpr(expr);
        } else {
            break;
        }
    }
    
    return left;
}

// Parse a relational expression (shiftExpr < shiftExpr, etc.)
static int evaluateRelExpr(const char** expr) {
    int left = evaluateShiftExpr(expr);
    
    while (1) {
        skipWhitespace(expr);
        
        if (**expr == '<' && (*expr)[1] == '=') {
            (*expr) += 2;
            left = left <= evaluateShiftExpr(expr);
        } else if (**expr == '>' && (*expr)[1] == '=') {
            (*expr) += 2;
            left = left >= evaluateShiftExpr(expr);
        } else if (**expr == '<' && (*expr)[1] != '<') {
            (*expr)++;
            left = left < evaluateShiftExpr(expr);
        } else if (**expr == '>' && (*expr)[1] != '>') {
            (*expr)++;
            left = left > evaluateShiftExpr(expr);
        } else {
            break;
        }
    }
    
    return left;
}

// Parse an equality expression (relExpr == relExpr, relExpr != relExpr)
static int evaluateEqExpr(const char** expr) {
    int left = evaluateRelExpr(expr);
    
    while (1) {
        skipWhitespace(expr);
        
        if (**expr == '=' && (*expr)[1] == '=') {
            (*expr) += 2;
            left = left == evaluateRelExpr(expr);
        } else if (**expr == '!' && (*expr)[1] == '=') {
            (*expr) += 2;
            left = left != evaluateRelExpr(expr);
        } else {
            break;
        }
    }
    
    return left;
}

// Parse a bitwise AND expression (eqExpr & eqExpr)
static int evaluateAndExpr(const char** expr) {
    int left = evaluateEqExpr(expr);
    
    while (1) {
        skipWhitespace(expr);
        
        if (**expr == '&' && (*expr)[1] != '&') {
            (*expr)++;
            left &= evaluateEqExpr(expr);
        } else {
            break;
        }
    }
    
    return left;
}

// Parse a bitwise XOR expression (andExpr ^ andExpr)
static int evaluateXorExpr(const char** expr) {
    int left = evaluateAndExpr(expr);
    
    while (1) {
        skipWhitespace(expr);
        
        if (**expr == '^') {
            (*expr)++;
            left ^= evaluateAndExpr(expr);
        } else {
            break;
        }
    }
    
    return left;
}

// Parse a bitwise OR expression (xorExpr | xorExpr)
static int evaluateOrExpr(const char** expr) {
    int left = evaluateXorExpr(expr);
    
    while (1) {
        skipWhitespace(expr);
        
        if (**expr == '|' && (*expr)[1] != '|') {
            (*expr)++;
            left |= evaluateXorExpr(expr);
        } else {
            break;
        }
    }
    
    return left;
}

// Parse a logical AND expression (orExpr && orExpr)
static int evaluateLogAndExpr(const char** expr) {
    int left = evaluateOrExpr(expr);
    
    while (1) {
        skipWhitespace(expr);
        
        if (**expr == '&' && (*expr)[1] == '&') {
            (*expr) += 2;
            int right = evaluateOrExpr(expr);
            left = left && right;  // Short-circuit evaluation
        } else {
            break;
        }
    }
    
    return left;
}

// Parse a logical OR expression (logAndExpr || logAndExpr)
static int evaluateLogOrExpr(const char** expr) {
    int left = evaluateLogAndExpr(expr);
    
    while (1) {
        skipWhitespace(expr);
        
        if (**expr == '|' && (*expr)[1] == '|') {
            (*expr) += 2;
            int right = evaluateLogAndExpr(expr);
            left = left || right;  // Short-circuit evaluation
        } else {
            break;
        }
    }
    
    return left;
}

// Parse a conditional expression (logOrExpr ? expr : expr)
static int evaluateCondExpr(const char** expr) {
    int condition = evaluateLogOrExpr(expr);
    
    skipWhitespace(expr);
    if (**expr == '?') {
        (*expr)++;
        int trueValue = evaluateExpression(expr);
        
        skipWhitespace(expr);
        if (**expr == ':') {
            (*expr)++;
            int falseValue = evaluateCondExpr(expr);
            return condition ? trueValue : falseValue;
        } else {
            fprintf(stderr, "Error: Missing ':' in conditional expression\n");
            return condition ? trueValue : 0;
        }
    }
    
    return condition;
}

// Parse a full expression
static int evaluateExpression(const char** expr) {
    return evaluateCondExpr(expr);
}

// Evaluate a preprocessing expression (for #if directive)
int evaluatePreprocessorExpression(const char* expr) {
    return evaluateExpression(&expr);
}
