// Attribute parsing functions
#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include "token_debug.h"  // For getTokenName
#include "error_manager.h" // For reportError

// Parse function attributes like __attribute__((naked))
void parseFunctionAttributes(FunctionInfo* funcInfo) {
    if (tokenIs(TOKEN_ATTRIBUTE)) {
        consume(TOKEN_ATTRIBUTE);
        // Expect first '('
        expect(TOKEN_LPAREN);
        
        // Expect second '('
        expect(TOKEN_LPAREN);
        
        // Parse attributes until double right parenthesis
        while (!tokenIs(TOKEN_RPAREN) && !tokenIs(TOKEN_RBRACKET)) {
            if (tokenIs(TOKEN_NAKED)) {
                consume(TOKEN_NAKED);
                funcInfo->is_naked = 1;
            } else if (tokenIs(TOKEN_IDENTIFIER)) {
                // Skip unknown attributes
                consume(TOKEN_IDENTIFIER);
            } else {
                break;
            }
            
            // Check for comma separating multiple attributes
            if (tokenIs(TOKEN_COMMA)) {
                consume(TOKEN_COMMA);
            } else {
                break;
            }
        }
        
        // Expect closing ')' twice
        expect(TOKEN_RPAREN);
        expect(TOKEN_RPAREN);
    }
}
