#include "type_support.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>

// Forward declarations
extern FILE* asmFile;
extern void generateExpression(ASTNode* node);

// Get size in bytes for a data type (implementation moved to ast.c)
// This function is now just a wrapper for the one in ast.c
int getTypeSizeEx(DataType type) {
    // Call the original function from ast.c
    return getTypeSize(type);
}

// Check if a type is an unsigned type
int isUnsignedType(DataType type) {
    return (type == TYPE_UNSIGNED_INT || type == TYPE_UNSIGNED_SHORT || type == TYPE_UNSIGNED_CHAR);
}

// Generate code for type conversion
void generateTypeConversion(DataType fromType, DataType toType) {
    if (fromType == toType) {
        // No conversion needed
        return;
    }
      // Handle char to int conversion (sign extension)
    if (fromType == TYPE_CHAR && toType == TYPE_INT) {
        fprintf(asmFile, "    ; Convert char to int (sign extension)\n");
        fprintf(asmFile, "    cbw ; Convert byte in AL to word in AX\n");
    }
    // Handle char to unsigned int (zero extension)
    else if (fromType == TYPE_CHAR && (toType == TYPE_UNSIGNED_INT || toType == TYPE_UNSIGNED_SHORT)) {
        fprintf(asmFile, "    ; Convert char to unsigned int (zero extension)\n");
        fprintf(asmFile, "    and ax, 0x00FF ; Zero extend AL to AX\n");
    }    // Truncate int to char (simple assignment keeps lower byte)
    else if ((fromType == TYPE_INT || fromType == TYPE_UNSIGNED_INT || fromType == TYPE_UNSIGNED_SHORT) && 
             (toType == TYPE_CHAR || toType == TYPE_UNSIGNED_CHAR)) {
        fprintf(asmFile, "    ; Truncate int to char (keeping lower byte)\n");
        // The result is already in AL, no need for additional instructions
    }
}

// Generate appropriate division instruction based on type
void generateDivision(ASTNode* left, ASTNode* right, int isMod) {
    // Determine if we're dealing with unsigned types
    int isUnsigned = 0; // By default, assume signed
    
    // In a more robust compiler, we'd check the specific types of both operands
    // and decide based on language rules. For simplicity, we'll assume int
    
    // Generate left operand, save result on stack
    generateExpression(left);
    fprintf(asmFile, "    push ax ; Save left operand\n");
    
    // Generate right operand, result in ax
    generateExpression(right);
    fprintf(asmFile, "    mov bx, ax ; Move divisor to BX\n");
    fprintf(asmFile, "    pop ax ; Restore left operand (dividend)\n");
    
    if (isUnsigned) {
        // Unsigned division
        fprintf(asmFile, "    mov dx, 0 ; Clear DX for unsigned division\n");
        fprintf(asmFile, "    div bx ; Unsigned division\n");
    } else {
        // Signed division
        fprintf(asmFile, "    cwd ; Sign extend AX into DX:AX\n");
        fprintf(asmFile, "    idiv bx ; Signed division\n");
    }
    
    // If this is a modulo operation, move the remainder (DX) to AX
    if (isMod) {
        fprintf(asmFile, "    mov ax, dx ; Move remainder to AX\n");
    }
    // For division, the quotient is already in AX
}
