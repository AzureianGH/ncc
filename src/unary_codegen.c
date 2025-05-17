#include "codegen.h"
#include "ast.h"
#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations from codegen.c
extern FILE* asmFile;
extern int getVariableOffset(const char* name);
extern int isParameter(const char* name);
extern int getNextLabelId();
extern int labelCounter;

// Generate code for unary operations
void generateUnaryOp(ASTNode* node) {
    if (!node || node->type != NODE_UNARY_OP) return;
    
    switch (node->unary_op.op) {
        case UNARY_DEREFERENCE:
            // Generate code to evaluate the pointer expression
            generateExpression(node->right);
            
            // Check if this is a far pointer (segment in DX, offset in AX)
            if (node->right->type == NODE_LITERAL && node->right->literal.data_type == TYPE_FAR_POINTER) {
                // For far pointers, we need to save DS, switch to the segment, dereference, then restore DS
                fprintf(asmFile, "    ; Dereferencing far pointer\n");
                fprintf(asmFile, "    push ds ; Save current DS\n");
                fprintf(asmFile, "    mov bx, ax ; Move offset to BX\n");
                fprintf(asmFile, "    mov ds, dx ; Set DS to segment\n");
                
                int labelId = getNextLabelId();
                
                // Check for null pointer
                fprintf(asmFile, "    cmp bx, 0 ; Check for null pointer\n");
                fprintf(asmFile, "    je null_ptr_deref_%d\n", labelId);
                fprintf(asmFile, "    xor ah, ah ; Clear high byte for char\n");
                fprintf(asmFile, "    mov al, [bx] ; Load byte (char) from memory\n");
                fprintf(asmFile, "    jmp ptr_deref_end_%d\n", labelId);
                fprintf(asmFile, "null_ptr_deref_%d:\n", labelId);
                fprintf(asmFile, "    ; Null pointer dereference detected\n");
                fprintf(asmFile, "    mov ax, 0 ; Return 0 for null deref\n");
                fprintf(asmFile, "ptr_deref_end_%d:\n", labelId);
                
                fprintf(asmFile, "    pop ds ; Restore DS\n");
            }            // For identifiers that might be pointers
            else if (node->right->type == NODE_IDENTIFIER) {
                fprintf(asmFile, "    ; Dereferencing pointer\n");
                fprintf(asmFile, "    mov bx, ax ; Move pointer address to BX\n");
                
                int labelId = getNextLabelId();
                
                // Add null pointer check
                fprintf(asmFile, "    cmp bx, 0 ; Check for null pointer\n");
                fprintf(asmFile, "    je null_ptr_deref_%d\n", labelId);
                fprintf(asmFile, "    xor ah, ah ; Clear high byte for char\n");
                fprintf(asmFile, "    mov al, [bx] ; Load byte (char) from memory\n");
                fprintf(asmFile, "    jmp ptr_deref_end_%d\n", labelId);
                fprintf(asmFile, "null_ptr_deref_%d:\n", labelId);
                fprintf(asmFile, "    ; Null pointer dereference detected\n");
                fprintf(asmFile, "    mov ax, 0 ; Return 0 for null deref\n");
                fprintf(asmFile, "ptr_deref_end_%d:\n", labelId);
            }
            // For other expressions resulting in a pointer
            else {
                fprintf(asmFile, "    ; Dereferencing pointer\n");
                fprintf(asmFile, "    mov bx, ax ; Move address to BX\n");
                
                int labelId = getNextLabelId();
                
                // Add null pointer check
                fprintf(asmFile, "    cmp bx, 0 ; Check for null pointer\n");
                fprintf(asmFile, "    je null_ptr_deref_%d\n", labelId);
                fprintf(asmFile, "    xor ah, ah ; Clear high byte for char\n");
                fprintf(asmFile, "    mov al, [bx] ; Load byte (char) from memory\n");
                fprintf(asmFile, "    jmp ptr_deref_end_%d\n", labelId);
                fprintf(asmFile, "null_ptr_deref_%d:\n", labelId);
                fprintf(asmFile, "    ; Null pointer dereference detected\n");
                fprintf(asmFile, "    mov ax, 0 ; Return 0 for null deref\n");
                fprintf(asmFile, "ptr_deref_end_%d:\n", labelId);
            }
            break;
            
        case UNARY_ADDRESS_OF:
            // Check if operand is an identifier (the common case)
            if (node->right->type == NODE_IDENTIFIER) {
                char* name = node->right->identifier;
                
                if (isParameter(name)) {
                    // Parameter - compute address from BP + offset
                    fprintf(asmFile, "    ; Address of parameter %s\n", name);
                    fprintf(asmFile, "    lea ax, [bp+%d] ; Load address of parameter\n", 
                          -getVariableOffset(name));
                } else {
                    // Local variable - compute address from BP - offset
                    fprintf(asmFile, "    ; Address of variable %s\n", name);
                    fprintf(asmFile, "    lea ax, [bp-%d] ; Load address of local variable\n", 
                          getVariableOffset(name));
                }
            }
            else {
                // This would be more complex address calculations
                // For nested expressions - not fully implemented
                fprintf(asmFile, "    ; Complex address-of operation not fully supported\n");
            }
            break;
            
        case UNARY_NEGATE:
            // Generate code for the operand
            generateExpression(node->right);
            
            // Negate the result
            fprintf(asmFile, "    neg ax ; Negate value\n");
            break;
            
        case UNARY_NOT:
            // Generate code for the operand
            generateExpression(node->right);
            
            // Logical NOT (0 -> 1, non-zero -> 0)
            fprintf(asmFile, "    test ax, ax ; Test if AX is zero\n");
            fprintf(asmFile, "    setz al ; Set AL to 1 if AX is zero, 0 otherwise\n");
            fprintf(asmFile, "    movzx ax, al ; Zero-extend AL to AX\n");
            break;
              case UNARY_BITWISE_NOT:
            // Generate code for the operand
            generateExpression(node->right);
            
            // Bitwise NOT
            fprintf(asmFile, "    not ax ; Bitwise NOT\n");
            break;

        case PREFIX_INCREMENT:
            // For prefix increment (++x), we need to:
            // 1. Generate the address of the operand
            // 2. Load its value
            // 3. Increment the value
            // 4. Store it back
            // 5. Leave the incremented value in AX as the result
            if (node->right->type == NODE_IDENTIFIER) {
                char* name = node->right->identifier;
                int offset = getVariableOffset(name);
                
                if (isParameter(name)) {
                    fprintf(asmFile, "    ; Prefix increment of parameter %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp+%d] ; Load parameter value\n", -offset);
                    fprintf(asmFile, "    inc ax ; Increment value\n");
                    fprintf(asmFile, "    mov [bp+%d], ax ; Store incremented value back\n", -offset);
                } else {
                    fprintf(asmFile, "    ; Prefix increment of variable %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp-%d] ; Load variable value\n", offset);
                    fprintf(asmFile, "    inc ax ; Increment value\n");
                    fprintf(asmFile, "    mov [bp-%d], ax ; Store incremented value back\n", offset);
                }            } else if (node->right->type == NODE_UNARY_OP && 
                      node->right->unary_op.op == UNARY_DEREFERENCE) {
                // Handle the case of ++(*ptr)
                fprintf(asmFile, "    ; Prefix increment of dereferenced pointer\n");
                generateExpression(node->right->right);  // Get the pointer value
                fprintf(asmFile, "    mov bx, ax ; Move pointer to BX\n");
                fprintf(asmFile, "    mov ax, [bx] ; Load value from memory\n");
                fprintf(asmFile, "    inc ax ; Increment value\n");
                fprintf(asmFile, "    mov [bx], ax ; Store incremented value back\n");
            } else {
                reportWarning(-1, "Complex prefix increment not fully supported");
                generateExpression(node->right);
            }
            break;
            
        case PREFIX_DECREMENT:
            // For prefix decrement (--x), similar to prefix increment but decrement
            if (node->right->type == NODE_IDENTIFIER) {
                char* name = node->right->identifier;
                int offset = getVariableOffset(name);
                
                if (isParameter(name)) {
                    fprintf(asmFile, "    ; Prefix decrement of parameter %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp+%d] ; Load parameter value\n", -offset);
                    fprintf(asmFile, "    dec ax ; Decrement value\n");
                    fprintf(asmFile, "    mov [bp+%d], ax ; Store decremented value back\n", -offset);
                } else {
                    fprintf(asmFile, "    ; Prefix decrement of variable %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp-%d] ; Load variable value\n", offset);
                    fprintf(asmFile, "    dec ax ; Decrement value\n");
                    fprintf(asmFile, "    mov [bp-%d], ax ; Store decremented value back\n", offset);
                }            } else if (node->right->type == NODE_UNARY_OP && 
                      node->right->unary_op.op == UNARY_DEREFERENCE) {
                // Handle the case of --(*ptr)
                fprintf(asmFile, "    ; Prefix decrement of dereferenced pointer\n");
                generateExpression(node->right->right);  // Get the pointer value
                fprintf(asmFile, "    mov bx, ax ; Move pointer to BX\n");
                fprintf(asmFile, "    mov ax, [bx] ; Load value from memory\n");
                fprintf(asmFile, "    dec ax ; Decrement value\n");
                fprintf(asmFile, "    mov [bx], ax ; Store decremented value back\n");
            } else {
                reportWarning(-1, "Complex prefix decrement not fully supported");
                generateExpression(node->right);
            }
            break;
            
        case POSTFIX_INCREMENT:
            // For postfix increment (x++), we need to:
            // 1. Generate the address of the operand
            // 2. Load its value
            // 3. Store the original value in a temporary (AX)
            // 4. Increment and store back the value
            // 5. Leave the original value in AX as the result
            if (node->right->type == NODE_IDENTIFIER) {
                char* name = node->right->identifier;
                int offset = getVariableOffset(name);
                
                if (isParameter(name)) {
                    fprintf(asmFile, "    ; Postfix increment of parameter %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp+%d] ; Load parameter value\n", -offset);
                    fprintf(asmFile, "    mov bx, ax ; Save original value to BX\n");
                    fprintf(asmFile, "    inc bx ; Increment value\n");
                    fprintf(asmFile, "    mov [bp+%d], bx ; Store incremented value back\n", -offset);
                    // AX still contains original value
                } else {
                    fprintf(asmFile, "    ; Postfix increment of variable %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp-%d] ; Load variable value\n", offset);
                    fprintf(asmFile, "    mov bx, ax ; Save original value to BX\n");
                    fprintf(asmFile, "    inc bx ; Increment value\n");
                    fprintf(asmFile, "    mov [bp-%d], bx ; Store incremented value back\n", offset);
                    // AX still contains original value
                }            } else if (node->right->type == NODE_UNARY_OP && 
                      node->right->unary_op.op == UNARY_DEREFERENCE) {                // Handle the case of (*ptr)++
                fprintf(asmFile, "    ; Postfix increment of dereferenced pointer\n");
                generateExpression(node->right->right);  // Get the pointer value
                fprintf(asmFile, "    mov bx, ax ; Move pointer to BX\n");
                fprintf(asmFile, "    mov ax, [bx] ; Load value from memory\n");                fprintf(asmFile, "    mov cx, ax ; Save original value to CX\n");
                fprintf(asmFile, "    inc ax ; Increment value\n");
                fprintf(asmFile, "    mov [bx], ax ; Store incremented value back\n");
                fprintf(asmFile, "    mov ax, cx ; Restore original value to AX\n");
                // AX contains original value
            } else {
                reportWarning(-1, "Complex postfix increment not fully supported");
                generateExpression(node->right);
            }
            break;
            
        case POSTFIX_DECREMENT:
            // For postfix decrement (x--), similar to postfix increment but decrement
            if (node->right->type == NODE_IDENTIFIER) {
                char* name = node->right->identifier;
                int offset = getVariableOffset(name);
                
                if (isParameter(name)) {
                    fprintf(asmFile, "    ; Postfix decrement of parameter %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp+%d] ; Load parameter value\n", -offset);
                    fprintf(asmFile, "    mov bx, ax ; Save original value to BX\n");
                    fprintf(asmFile, "    dec bx ; Decrement value\n");
                    fprintf(asmFile, "    mov [bp+%d], bx ; Store decremented value back\n", -offset);
                    // AX still contains original value
                } else {
                    fprintf(asmFile, "    ; Postfix decrement of variable %s\n", name);
                    fprintf(asmFile, "    mov ax, [bp-%d] ; Load variable value\n", offset);
                    fprintf(asmFile, "    mov bx, ax ; Save original value to BX\n");
                    fprintf(asmFile, "    dec bx ; Decrement value\n");
                    fprintf(asmFile, "    mov [bp-%d], bx ; Store decremented value back\n", offset);
                    // AX still contains original value
                }            } else if (node->right->type == NODE_UNARY_OP && 
                      node->right->unary_op.op == UNARY_DEREFERENCE) {
                // Handle the case of (*ptr)--
                fprintf(asmFile, "    ; Postfix decrement of dereferenced pointer\n");
                generateExpression(node->right->right);  // Get the pointer value
                fprintf(asmFile, "    mov bx, ax ; Move pointer to BX\n");
                fprintf(asmFile, "    mov ax, [bx] ; Load value from memory\n");
                fprintf(asmFile, "    mov cx, ax ; Save original value to CX\n");
                fprintf(asmFile, "    dec cx ; Decrement value\n");
                fprintf(asmFile, "    mov [bx], cx ; Store decremented value back\n");
                // AX still contains original value
            } else {
                reportWarning(-1, "Complex postfix decrement not fully supported");
                generateExpression(node->right);
            }
            break;
            
        default:
            reportWarning(-1, "Unsupported unary operator: %d", node->unary_op.op);
            break;
    }
}
