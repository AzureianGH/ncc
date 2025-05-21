#include "lexer.h"
#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Source code to tokenize
static const char* source;

// Current position in the source
static size_t position = 0;

// Current line and column for error reporting
static int line = 1;
static int column = 1;

// Current token
static Token currentToken;

// Initialize lexer with source code
void initLexer(const char* src) {
    source = src;
    position = 0;
    line = 1;
    column = 1;
    
    // Initialize first token
    currentToken = getNextToken();
}

// Skip whitespace and comments
static void skipWhitespace() {
    while (source[position]) {
        // Skip spaces, tabs, newlines
        if (isspace(source[position])) {
            if (source[position] == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            position++;
        }
        // Skip single line comments
        else if (source[position] == '/' && source[position + 1] == '/') {
            position += 2;
            column += 2;
            while (source[position] && source[position] != '\n') {
                position++;
                column++;
            }
        }
        // Skip multi-line comments
        else if (source[position] == '/' && source[position + 1] == '*') {
            position += 2;
            column += 2;
            while (source[position] && !(source[position] == '*' && source[position + 1] == '/')) {
                if (source[position] == '\n') {
                    line++;
                    column = 1;
                } else {
                    column++;
                }
                position++;
            }
            if (source[position]) {
                position += 2; // Skip */
                column += 2;
            }
        } else {
            break;
        }
    }
}

// Check if a string is a keyword
static int isKeyword(const char* str) {
    if (strcmp(str, "int") == 0) return TOKEN_INT;
    if (strcmp(str, "short") == 0) return TOKEN_SHORT;
    if (strcmp(str, "unsigned") == 0) return TOKEN_UNSIGNED;
    if (strcmp(str, "char") == 0) return TOKEN_CHAR;
    if (strcmp(str, "void") == 0) return TOKEN_VOID;
    if (strcmp(str, "__far") == 0) return TOKEN_FAR;
    if (strcmp(str, "__asm") == 0) return TOKEN_ASM;
    if (strcmp(str, "__stackframe") == 0) return TOKEN_STACKFRAME;
    if (strcmp(str, "__farcalled") == 0) return TOKEN_FARCALLED;
    if (strcmp(str, "__attribute__") == 0) return TOKEN_ATTRIBUTE;
    if (strcmp(str, "naked") == 0) return TOKEN_NAKED;
    if (strcmp(str, "static") == 0) return TOKEN_STATIC;
    if (strcmp(str, "deprecated") == 0) return TOKEN_DEPRECATED;    
    if (strcmp(str, "if") == 0) return TOKEN_IF;
    if (strcmp(str, "else") == 0) return TOKEN_ELSE;    
    if (strcmp(str, "while") == 0) return TOKEN_WHILE;
    if (strcmp(str, "do") == 0) return TOKEN_DO;
    if (strcmp(str, "for") == 0) return TOKEN_FOR;
    if (strcmp(str, "return") == 0) return TOKEN_RETURN;
    if (strcmp(str, "bool") == 0) return TOKEN_BOOL;
    if (strcmp(str, "true") == 0) return TOKEN_TRUE;
    if (strcmp(str, "false") == 0) return TOKEN_FALSE;
    if (strcmp(str, "sizeof") == 0) return TOKEN_SIZEOF;
    return TOKEN_IDENTIFIER;
}

// Get the next token
Token getNextToken() {
    Token token;
    token.value = NULL;
    token.line = line;
    token.column = column;
    token.pos = position;  // Store current position in source

    skipWhitespace();

    if (source[position] == '\0') {
        token.type = TOKEN_EOF;
        return token;
    }

    // Handle keywords and identifiers
    if (isalpha(source[position]) || source[position] == '_') {
        size_t start = position;
        int startColumn = column;
        
        while (isalnum(source[position]) || source[position] == '_') {
            position++;
            column++;
        }
        
        size_t length = position - start;
        token.value = (char*)malloc(length + 1);
        strncpy(token.value, &source[start], length);
        token.value[length] = '\0';

        token.type = isKeyword(token.value);
        token.line = line;
        token.column = startColumn;
        
        return token;
    }

    // Handle numbers
    if (isdigit(source[position])) {
        size_t start = position;
        int startColumn = column;
        
        // Check for hex numbers (0x...)
        if (source[position] == '0' && (source[position + 1] == 'x' || source[position + 1] == 'X')) {
            position += 2;
            column += 2;
            while (isxdigit(source[position])) {
                position++;
                column++;
            }
        } else {
            // Regular decimal numbers
            while (isdigit(source[position])) {
                position++;
                column++;
            }
        }
        
        size_t length = position - start;
        token.value = (char*)malloc(length + 1);
        strncpy(token.value, &source[start], length);
        token.value[length] = '\0';
        token.type = TOKEN_NUMBER;
        token.line = line;
        token.column = startColumn;
        return token;
    }
      // Handle string literals
    if (source[position] == '"') {
        int startColumn = column;
        int startPos = position;
        position++;  // Skip opening quote
        column++;
        
        size_t start = position;
        while (source[position] && source[position] != '"' && source[position] != '\n') {
            // Handle escape sequences
            if (source[position] == '\\' && source[position + 1]) {
                position += 2;
                column += 2;
            } else {
                position++;
                column++;
            }
        }
        
        size_t length = position - start;
        token.value = (char*)malloc(length + 1);
        strncpy(token.value, &source[start], length);
        token.value[length] = '\0';
        token.type = TOKEN_STRING;
        
        if (source[position] == '"') {
            position++;  // Skip closing quote
            column++;
        } else {
            reportError(startPos, "Unterminated string literal");
            exit(1);
        }
        
        token.line = line;
        token.column = startColumn;
        return token;
    }    // Handle character literals
    if (source[position] == '\'') {
        int startColumn = column;
        int startPos = position;
        position++;  // Skip opening quote
        column++;
        
        char charValue = 0;
        
        // Handle escape sequences
        if (source[position] == '\\') {
            position++;
            column++;
            
            // Process escape sequences
            switch (source[position]) {
                case 'n':  charValue = '\n'; break;
                case 't':  charValue = '\t'; break;
                case 'r':  charValue = '\r'; break;
                case '0':  charValue = '\0'; break;
                case '\\': charValue = '\\'; break;
                case '\'': charValue = '\''; break;
                case '"':  charValue = '"';  break;
                case 'x': {
                    // Handle hex escape sequence \xHH
                    if (isxdigit(source[position + 1]) && isxdigit(source[position + 2])) {
                        char hex[3] = {source[position + 1], source[position + 2], '\0'};
                        charValue = (char)strtol(hex, NULL, 16);
                        position += 2;
                        column += 2;
                    } else {
                        reportError(startPos, "Invalid hex escape sequence, expected \\xHH format");
                        exit(1);
                    }
                    break;
                }
                default:
                    charValue = source[position];
            }
            position++;
            column++;
        } else if (source[position] && source[position] != '\'' && source[position] != '\n') {
            charValue = source[position];
            position++;
            column++;
        } else {
            reportError(startPos, "Invalid character literal");
            exit(1);
        }
        
        token.type = TOKEN_CHAR_LITERAL;
        token.value = malloc(2);
        token.value[0] = charValue;
        token.value[1] = '\0';
        
        if (source[position] == '\'') {
            position++;  // Skip closing quote
            column++;
        } else {
            reportError(startPos, "Unterminated character literal");
            exit(1);
        }
        
        token.line = line;
        token.column = startColumn;
        return token;
    }
    
    // Handle operators and punctuation
    int startColumn = column;
    // C23 attributes
    if (source[position] == '[' && source[position+1] == '[') {
        token.type = TOKEN_ATTR_OPEN;
        position += 2;
        column += 2;        token.line = line;
        token.column = startColumn;
        return token;
    }
    // Handle ellipsis (...)
    if (source[position] == '.' && source[position+1] == '.' && source[position+2] == '.') {
        token.type = TOKEN_ELLIPSIS;
        position += 3;
        column += 3;
        token.line = line;
        token.column = startColumn;
        return token;
    }
    if (source[position] == ']' && source[position+1] == ']') {
        token.type = TOKEN_ATTR_CLOSE;
        position += 2;
        column += 2;
        token.line = line;
        token.column = startColumn;
        return token;
    }
    switch (source[position]) {
        case '{':
            token.type = TOKEN_LBRACE;
            position++;
            column++;
            break;
        case '}':
            token.type = TOKEN_RBRACE;
            position++;
            column++;
            break;
        case '(':
            token.type = TOKEN_LPAREN;
            position++;
            column++;
            break;
        case ')':
            token.type = TOKEN_RPAREN;
            position++;
            column++;
            break;
        case '[':
            token.type = TOKEN_LBRACKET;
            position++;
            column++;
            break;
        case ']':
            token.type = TOKEN_RBRACKET;
            position++;
            column++;
            break;
        case ';':
            token.type = TOKEN_SEMICOLON;
            position++;
            column++;
            break;        
        case ':':
            token.type = TOKEN_COLON;
            position++;
            column++;
            break;
        case '?':
            token.type = TOKEN_QUESTION;
            position++;
            column++;
            break;
        case ',':
            token.type = TOKEN_COMMA;
            position++;
            column++;
            break;
        case '.':
            token.type = TOKEN_DOT;
            position++;
            column++;
            break;
        case '=':
            position++;
            column++;            if (source[position] == '=') {
                token.type = TOKEN_EQ;
                position++;
                column++;
            } else {
                token.type = TOKEN_ASSIGN;
            }
            break;        case '+':
            position++;
            column++;
            if (source[position] == '=') {
                token.type = TOKEN_PLUS_ASSIGN;  // Now proper += token
                position++;
                column++;
            } else if (source[position] == '+') {
                token.type = TOKEN_INCREMENT;  // ++ operator
                position++;
                column++;
            } else {
                token.type = TOKEN_PLUS;
            }
            break;        case '-':
            position++;
            column++;
            if (source[position] == '=') {
                token.type = TOKEN_MINUS_ASSIGN;  // Now proper -= token
                position++;
                column++;
            } else if (source[position] == '>') {
                token.type = TOKEN_ARROW;
                position++;
                column++;
            } else if (source[position] == '-') {
                token.type = TOKEN_DECREMENT;  // -- operator
                position++;
                column++;
            } else {
                token.type = TOKEN_MINUS;
            }
            break;case '*':
            position++;
            column++;
            if (source[position] == '=') {
                token.type = TOKEN_MUL_ASSIGN;  // *= operator
                position++;
                column++;
            } else {
                token.type = TOKEN_STAR;
            }
            break;        case '/':
            position++;
            column++;
            if (source[position] == '=') {
                token.type = TOKEN_DIV_ASSIGN;  // /= operator
                position++;
                column++;
            } else {
                token.type = TOKEN_SLASH;
            }
            break;        case '%':
            position++;
            column++;
            if (source[position] == '=') {
                token.type = TOKEN_MOD_ASSIGN;  // %= operator
                position++;
                column++;
            } else {
                token.type = TOKEN_PERCENT;
            }
            break;
        case '!':
            position++;
            column++;            if (source[position] == '=') {
                token.type = TOKEN_NEQ;
                position++;
                column++;
            } else {
                token.type = TOKEN_NOT;
            }
            break;        case '<':
            position++;
            column++;
            if (source[position] == '=') {
                token.type = TOKEN_LTE;
                position++;
                column++;
            } else if (source[position] == '<') {
                token.type = TOKEN_LEFT_SHIFT;
                position++;
                column++;
            } else {
                token.type = TOKEN_LT;
            }
            break;        case '>':
            position++;
            column++;
            if (source[position] == '=') {
                token.type = TOKEN_GTE;
                position++;
                column++;
            } else if (source[position] == '>') {
                token.type = TOKEN_RIGHT_SHIFT;
                position++;
                column++;
            } else {
                token.type = TOKEN_GT;
            }
            break;
        case '&':
            position++;
            column++;
            if (source[position] == '&') {
                token.type = TOKEN_AND;
                position++;
                column++;
            } else {
                token.type = TOKEN_AMPERSAND;
            }
            break;
        case '|':
            position++;
            column++;
            if (source[position] == '|') {
                token.type = TOKEN_OR;
                position++;
                column++;
            } else {
                token.type = TOKEN_PIPE;
            }
            break;
            
        case '^':
            position++;
            column++;
            token.type = TOKEN_XOR;
            break;
            
        case '~':
            position++;
            column++;
            token.type = TOKEN_BITWISE_NOT;
            break;
            
        default:
            int errorPos = position;
            char errorChar = source[position];
            position++;  // Skip the unrecognized character
            column++;
            reportWarning(errorPos, "Unexpected character '%c'", errorChar);
            return getNextToken();  // Try again with the next character
    }
    
    token.line = line;
    token.column = startColumn;
    
    return token;
}

// Peek at the next token without consuming it
Token peekNextToken() {
    size_t oldPos = position;
    int oldLine = line;
    int oldColumn = column;
    Token oldToken = currentToken;
    
    Token nextToken = getNextToken();
    
    position = oldPos;
    line = oldLine;
    column = oldColumn;
    currentToken = oldToken;
    
    return nextToken;
}

// Get the current token
Token getCurrentToken() {
    return currentToken;
}

// Check if the current token is of a specific type
int tokenIs(TokenType type) {
    return currentToken.type == type;
}

// Consume the current token if it matches the expected type
int consume(TokenType type) {
    if (tokenIs(type)) {
        if (currentToken.value) {
            free(currentToken.value);
        }
        currentToken = getNextToken();
        return 1;
    }
    return 0;
}

// Consume the current token and return its value
char* consumeAndGetValue(TokenType type) {
    if (tokenIs(type)) {
        char* value = currentToken.value ? strdup(currentToken.value) : NULL;
        currentToken = getNextToken();
        return value;
    }
    return NULL;
}

// Report a syntax error
void syntaxError(const char* message) {
    // Calculate position in source buffer for error reporting
    int position = currentToken.pos;
    
    // Use the error manager to report the error
    reportError(position, "Syntax error: %s", message);
    exit(1);
}

// Get current token position for backtracking
int getTokenPosition() {
    return position;
}

// Set token position for backtracking
void setTokenPosition(int pos) {
    position = pos;
    // Must reset current token to ensure next getCurrentToken starts from the new position
    currentToken.type = TOKEN_EOF; // Use TOKEN_EOF as a marker for reset
}