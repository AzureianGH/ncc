// Attribute parsing functions
#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include "token_debug.h"  // For getTokenName
#include "error_manager.h" // For reportError


char * strdupc (const char *s)
{
  size_t len = strlen (s) + 1;
  void *new = malloc (
len);
  if (new == NULL)
    return NULL;
  return (char *) memcpy (new, s, len);
}

// Parse function attributes like __attribute__((naked))
void parseFunctionAttributes(FunctionInfo* funcInfo) {
    // Process multiple attributes in sequence
    while (tokenIs(TOKEN_ATTRIBUTE) || tokenIs(TOKEN_ATTR_OPEN)) {
        if (tokenIs(TOKEN_ATTRIBUTE)) {
            // Legacy __attribute__ syntax
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
                } else if (tokenIs(TOKEN_DEPRECATED)) {
                    consume(TOKEN_DEPRECATED);
                    funcInfo->is_deprecated = 1;
                    
                    // Check for optional message
                    if (tokenIs(TOKEN_LPAREN)) {
                        consume(TOKEN_LPAREN);
                        if (tokenIs(TOKEN_STRING)) {
                            // Free previous message if there was one
                            if (funcInfo->deprecation_msg) {
                                free(funcInfo->deprecation_msg);
                            }
                            funcInfo->deprecation_msg = strdupc(getCurrentToken().value);
                            consume(TOKEN_STRING);
                        }
                        expect(TOKEN_RPAREN);
                    }
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
        else if (tokenIs(TOKEN_ATTR_OPEN)) {
            // Parse C23 attribute syntax [[naked]] or [[deprecated("message")]]
            consume(TOKEN_ATTR_OPEN);
            
            // Allow multiple attributes within a single [[attribute1, attribute2]] block
            while (!tokenIs(TOKEN_ATTR_CLOSE)) {
                if (tokenIs(TOKEN_NAKED)) {
                    consume(TOKEN_NAKED);
                    funcInfo->is_naked = 1;
                } else if (tokenIs(TOKEN_DEPRECATED)) {
                    consume(TOKEN_DEPRECATED);
                    funcInfo->is_deprecated = 1;
                    
                    // Check for optional message in parentheses
                    if (tokenIs(TOKEN_LPAREN)) {
                        consume(TOKEN_LPAREN);
                        if (tokenIs(TOKEN_STRING)) {
                            // Free previous message if there was one
                            if (funcInfo->deprecation_msg) {
                                free(funcInfo->deprecation_msg);
                            }
                            funcInfo->deprecation_msg = strdupc(getCurrentToken().value);
                            consume(TOKEN_STRING);
                        }
                        expect(TOKEN_RPAREN);
                    }
                } else if (tokenIs(TOKEN_IDENTIFIER)) {
                    // Skip unknown attributes
                    consume(TOKEN_IDENTIFIER);
                } else {
                    break;
                }
                
                // Continue parsing attributes if comma is present
                if (tokenIs(TOKEN_COMMA)) {
                    consume(TOKEN_COMMA);
                } else {
                    break;
                }
            }
            expect(TOKEN_ATTR_CLOSE);
        }
    }
}
