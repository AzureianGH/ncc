#include "lexer.h"
#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

// Global lexer state
static Lexer lexer;

// Keywords table for C99
static struct {
    const char* keyword;
    TokenType token;
} keywords[] = {
    {"auto", TOKEN_AUTO},
    {"break", TOKEN_BREAK},
    {"case", TOKEN_CASE},
    {"char", TOKEN_CHAR},
    {"const", TOKEN_CONST},
    {"continue", TOKEN_CONTINUE},
    {"default", TOKEN_DEFAULT},
    {"do", TOKEN_DO},
    {"double", TOKEN_DOUBLE},
    {"else", TOKEN_ELSE},
    {"enum", TOKEN_ENUM},
    {"extern", TOKEN_EXTERN},
    {"float", TOKEN_FLOAT},
    {"for", TOKEN_FOR},
    {"goto", TOKEN_GOTO},
    {"if", TOKEN_IF},
    {"int", TOKEN_INT},
    {"long", TOKEN_LONG},
    {"register", TOKEN_REGISTER},
    {"return", TOKEN_RETURN},
    {"short", TOKEN_SHORT},
    {"signed", TOKEN_SIGNED},
    {"sizeof", TOKEN_SIZEOF},
    {"static", TOKEN_STATIC},
    {"struct", TOKEN_STRUCT},
    {"switch", TOKEN_SWITCH},
    {"typedef", TOKEN_TYPEDEF},
    {"union", TOKEN_UNION},
    {"unsigned", TOKEN_UNSIGNED},
    {"void", TOKEN_VOID},
    {"volatile", TOKEN_VOLATILE},
    {"while", TOKEN_WHILE},
    
    // C99 keywords
    {"_Bool", TOKEN_BOOL},
    {"_Complex", TOKEN_COMPLEX},
    {"_Imaginary", TOKEN_IMAGINARY},
    {"inline", TOKEN_INLINE},
    {"restrict", TOKEN_RESTRICT},
    {"_Alignas", TOKEN_ALIGNAS},
    {"_Alignof", TOKEN_ALIGNOF},
    {"_Atomic", TOKEN_ATOMIC},
    {"_Static_assert", TOKEN_STATIC_ASSERT},
    {"_Thread_local", TOKEN_THREAD_LOCAL},
    {"_Generic", TOKEN_GENERIC},
    {"_Noreturn", TOKEN_NORETURN},
    
    // C99 boolean literals
    {"true", TOKEN_TRUE},
    {"false", TOKEN_FALSE},
    
    // Inline assembly
    {"asm", TOKEN_ASM},
    {"__asm__", TOKEN_ASM},
    {"__asm", TOKEN_ASM},
    
    // GCC attributes
    {"__attribute__", TOKEN_ATTRIBUTE},
    {"__packed__", TOKEN_PACKED},
    {"__aligned__", TOKEN_ALIGNED},
    {"__section__", TOKEN_SECTION},
    {"__naked__", TOKEN_NAKED},
    {"__deprecated__", TOKEN_DEPRECATED},
    
    {NULL, TOKEN_UNKNOWN}
};

// Character classification functions
int isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

int isDigit(char c) {
    return c >= '0' && c <= '9';
}

int isAlnum(char c) {
    return isAlpha(c) || isDigit(c);
}

int isHexDigit(char c) {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int isOctalDigit(char c) {
    return c >= '0' && c <= '7';
}

int isBinaryDigit(char c) {
    return c == '0' || c == '1';
}

int isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v';
}

int isNewline(char c) {
    return c == '\n';
}

// Utility functions
static char peek() {
    return *lexer.current;
}

static char peekNext() {
    if (lexer.current[0] == '\0') return '\0';
    return lexer.current[1];
}

static char advance() {
    if (*lexer.current == '\0') {
        return '\0';  // Don't advance past EOF
    }
    
    if (*lexer.current == '\n') {
        lexer.line++;
        lexer.column = 1;
    } else {
        lexer.column++;
    }
    return *lexer.current++;
}

static void skipWhitespace() {
    while (isWhitespace(peek())) {
        advance();
    }
}

static void skipLineComment() {
    // Skip //
    advance();
    advance();
    
    while (peek() != '\n' && peek() != '\0') {
        advance();
    }
}

static void skipBlockComment() {
    // Skip /*
    advance();
    advance();
    
    while (!(peek() == '*' && peekNext() == '/') && peek() != '\0') {
        advance();
    }
    
    if (peek() != '\0') {
        advance(); // *
        advance(); // /
    }
}

static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.value = createTokenString(lexer.start, (int)(lexer.current - lexer.start));
    token.length = (int)(lexer.current - lexer.start);
    token.line = lexer.line;
    token.column = lexer.column - token.length;
    token.filename = lexer.filename;
    
    // Clear flags
    memset(&token.flags, 0, sizeof(token.flags));
    memset(&token.numericValue, 0, sizeof(token.numericValue));
    
    return token;
}

static Token errorToken(const char* message) {
    Token token;
    token.type = TOKEN_UNKNOWN;
    token.value = createTokenString(message, strlen(message));
    token.length = strlen(message);
    token.line = lexer.line;
    token.column = lexer.column;
    token.filename = lexer.filename;
    
    lexerError(lexer.line, lexer.column, "%s", message);
    return token;
}

// Keyword lookup
TokenType getKeywordType(const char* str) {
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (strcmp(str, keywords[i].keyword) == 0) {
            return keywords[i].token;
        }
    }
    return TOKEN_IDENTIFIER;
}

int isKeyword(const char* str) {
    return getKeywordType(str) != TOKEN_IDENTIFIER;
}

// String utilities
char* createTokenString(const char* start, int length) {
    char* str = malloc(length + 1);
    if (!str) {
        fatalError("Out of memory creating token string");
        return NULL;
    }
    memcpy(str, start, length);
    str[length] = '\0';
    return str;
}

void freeTokenString(char* str) {
    if (str) {
        free(str);
    }
}

char* unescapeString(const char* str, int length) {
    char* result = malloc(length + 1);
    if (!result) {
        fatalError("Out of memory unescaping string");
        return NULL;
    }
    
    int i = 0, j = 0;
    while (i < length) {
        if (str[i] == '\\' && i + 1 < length) {
            switch (str[i + 1]) {
                case 'n': result[j++] = '\n'; i += 2; break;
                case 't': result[j++] = '\t'; i += 2; break;
                case 'r': result[j++] = '\r'; i += 2; break;
                case 'b': result[j++] = '\b'; i += 2; break;
                case 'f': result[j++] = '\f'; i += 2; break;
                case 'v': result[j++] = '\v'; i += 2; break;
                case '\\': result[j++] = '\\'; i += 2; break;
                case '\'': result[j++] = '\''; i += 2; break;
                case '\"': result[j++] = '\"'; i += 2; break;
                case '0': result[j++] = '\0'; i += 2; break;
                default:
                    result[j++] = str[i];
                    i++;
                    break;
            }
        } else {
            result[j++] = str[i++];
        }
    }
    result[j] = '\0';
    return result;
}

// Number parsing
int parseIntegerLiteral(const char* str, Token* token) {
    char* endptr;
    long long value;
    
    // Determine base
    int base = 10;
    if (str[0] == '0') {
        if (str[1] == 'x' || str[1] == 'X') {
            base = 16;
        } else if (str[1] == 'b' || str[1] == 'B') {
            base = 2; // Binary literals (GCC extension)
        } else if (isOctalDigit(str[1])) {
            base = 8;
        }
    }
    
    value = strtoll(str, &endptr, base);
    token->numericValue.intValue = value;
    
    // Check for suffixes
    while (*endptr) {
        switch (*endptr) {
            case 'u':
            case 'U':
                token->flags.isUnsigned = 1;
                break;
            case 'l':
            case 'L':
                if (token->flags.isLong) {
                    token->flags.isLongLong = 1;
                } else {
                    token->flags.isLong = 1;
                }
                break;
            default:
                return 0; // Invalid suffix
        }
        endptr++;
    }
    
    return 1;
}

int parseFloatLiteral(const char* str, Token* token) {
    char* endptr;
    double value = strtod(str, &endptr);
    token->numericValue.floatValue = value;
    
    // Check for suffixes
    while (*endptr) {
        switch (*endptr) {
            case 'f':
            case 'F':
                token->flags.isFloat = 1;
                break;
            case 'l':
            case 'L':
                token->flags.isLongDouble = 1;
                break;
            default:
                return 0; // Invalid suffix
        }
        endptr++;
    }
    
    if (!token->flags.isFloat && !token->flags.isLongDouble) {
        token->flags.isDouble = 1;
    }
    
    return 1;
}

int parseCharLiteral(const char* str, Token* token) {
    if (str[0] != '\'' || strlen(str) < 3) {
        return 0;
    }
    
    // Handle escape sequences
    if (str[1] == '\\') {
        switch (str[2]) {
            case 'n': token->numericValue.charValue = '\n'; break;
            case 't': token->numericValue.charValue = '\t'; break;
            case 'r': token->numericValue.charValue = '\r'; break;
            case 'b': token->numericValue.charValue = '\b'; break;
            case 'f': token->numericValue.charValue = '\f'; break;
            case 'v': token->numericValue.charValue = '\v'; break;
            case '\\': token->numericValue.charValue = '\\'; break;
            case '\'': token->numericValue.charValue = '\''; break;
            case '\"': token->numericValue.charValue = '\"'; break;
            case '0': token->numericValue.charValue = '\0'; break;
            default: token->numericValue.charValue = str[2]; break;
        }
    } else {
        token->numericValue.charValue = str[1];
    }
    
    return 1;
}

int parseStringLiteral(const char* str, Token* token) {
    // Basic string parsing - just store the value
    // Proper unescaping would be done later if needed
    return 1;
}

// Token scanning functions
static Token scanString() {
    while (peek() != '"' && peek() != '\0') {
        if (peek() == '\\') {
            advance(); // Skip escape character
            if (peek() != '\0') advance(); // Skip escaped character
        } else {
            advance();
        }
    }
    
    if (peek() == '\0') {
        return errorToken("Unterminated string");
    }
    
    advance(); // Closing quote
    return makeToken(TOKEN_STRING_LITERAL);
}

static Token scanChar() {
    while (peek() != '\'' && peek() != '\0') {
        if (peek() == '\\') {
            advance(); // Skip escape character
            if (peek() != '\0') advance(); // Skip escaped character
        } else {
            advance();
        }
    }
    
    if (peek() == '\0') {
        return errorToken("Unterminated character literal");
    }
    
    advance(); // Closing quote
    Token token = makeToken(TOKEN_CHAR_LITERAL);
    parseCharLiteral(token.value, &token);
    return token;
}

static Token scanNumber() {
    // Note: scanToken() consumed the first digit already, and lexer.start points to it
    // Recognize 0x..., 0b..., and octal when the first digit was '0'
    int firstWasZero = (lexer.start[0] == '0');
    if (firstWasZero) {
        if (peek() == 'x' || peek() == 'X') {
            advance(); // consume 'x'
            while (isHexDigit(peek())) advance();
        } else if (peek() == 'b' || peek() == 'B') {
            advance(); // consume 'b'
            while (isBinaryDigit(peek())) advance();
        } else {
            while (isOctalDigit(peek())) advance();
        }
    } else {
        while (isDigit(peek())) advance();
    }
    
    // Check for floating point
    int isFloat = 0;
    if (peek() == '.') {
        isFloat = 1;
        advance();
        while (isDigit(peek())) advance();
    }
    
    // Check for exponent
    if (peek() == 'e' || peek() == 'E') {
        isFloat = 1;
        advance();
        if (peek() == '+' || peek() == '-') advance();
        while (isDigit(peek())) advance();
    }
    
    // Handle suffixes
    while (isAlpha(peek())) {
        advance();
    }
    
    Token token;
    if (isFloat) {
        token = makeToken(TOKEN_FLOAT_LITERAL);
        parseFloatLiteral(token.value, &token);
        
        // Determine actual token type based on suffix
        if (token.flags.isFloat) {
            token.type = TOKEN_FLOAT_LITERAL;
        } else if (token.flags.isLongDouble) {
            token.type = TOKEN_LONG_DOUBLE_LITERAL;
        } else {
            token.type = TOKEN_DOUBLE_LITERAL;
        }
    } else {
        token = makeToken(TOKEN_INTEGER_LITERAL);
        parseIntegerLiteral(token.value, &token);
    }
    
    return token;
}

static Token scanIdentifier() {
    while (isAlnum(peek())) {
        advance();
    }
    
    Token token = makeToken(TOKEN_IDENTIFIER);
    TokenType keywordType = getKeywordType(token.value);
    if (keywordType != TOKEN_IDENTIFIER) {
        token.type = keywordType;
    }
    
    return token;
}

// Main tokenization function
static Token scanToken() {
    skipWhitespace();
    
    lexer.start = lexer.current;
    
    if (*lexer.current == '\0') {
        return makeToken(TOKEN_EOF);
    }
    
    char c = advance();
    
    // Double-check: if we somehow advanced past EOF, return EOF immediately
    if (c == '\0') {
        return makeToken(TOKEN_EOF);
    }
    
    // Handle identifiers and keywords
    if (isAlpha(c)) {
        return scanIdentifier();
    }
    
    // Handle numbers
    if (isDigit(c)) {
        return scanNumber();
    }
    
    switch (c) {
        case ' ':
        case '\r':
        case '\t':
        case '\f':
        case '\v':
            // Skip whitespace
            return scanToken();
        case '\n':
            // Line counting is already handled in advance() function
            // Check if we're at EOF after the newline
            if (*lexer.current == '\0') {
                return makeToken(TOKEN_EOF);
            }
            return scanToken();
        case '/':
            if (peek() == '/') {
                skipLineComment();
                return scanToken();
            } else if (peek() == '*') {
                skipBlockComment();
                return scanToken();
            } else if (peek() == '=') {
                advance();
                return makeToken(TOKEN_DIVIDE_ASSIGN);
            }
            return makeToken(TOKEN_DIVIDE);
        case '"':
            return scanString();
        case '\'':
            return scanChar();
        case '(':
            return makeToken(TOKEN_LEFT_PAREN);
        case ')':
            return makeToken(TOKEN_RIGHT_PAREN);
        case '[':
            return makeToken(TOKEN_LEFT_BRACKET);
        case ']':
            return makeToken(TOKEN_RIGHT_BRACKET);
        case '{':
            return makeToken(TOKEN_LEFT_BRACE);
        case '}':
            return makeToken(TOKEN_RIGHT_BRACE);
        case ';':
            return makeToken(TOKEN_SEMICOLON);
        case ',':
            return makeToken(TOKEN_COMMA);
        case '.':
            if (peek() == '.' && peekNext() == '.') {
                advance();
                advance();
                return makeToken(TOKEN_ELLIPSIS);
            }
            return makeToken(TOKEN_DOT);
        case '+':
            if (peek() == '+') {
                advance();
                return makeToken(TOKEN_INCREMENT);
            } else if (peek() == '=') {
                advance();
                return makeToken(TOKEN_PLUS_ASSIGN);
            }
            return makeToken(TOKEN_PLUS);
        case '-':
            if (peek() == '-') {
                advance();
                return makeToken(TOKEN_DECREMENT);
            } else if (peek() == '=') {
                advance();
                return makeToken(TOKEN_MINUS_ASSIGN);
            } else if (peek() == '>') {
                advance();
                return makeToken(TOKEN_ARROW);
            }
            return makeToken(TOKEN_MINUS);
        case '*':
            if (peek() == '=') {
                advance();
                return makeToken(TOKEN_MULTIPLY_ASSIGN);
            }
            return makeToken(TOKEN_MULTIPLY);
        case '%':
            if (peek() == '=') {
                advance();
                return makeToken(TOKEN_MODULO_ASSIGN);
            }
            return makeToken(TOKEN_MODULO);
        case '=':
            if (peek() == '=') {
                advance();
                return makeToken(TOKEN_EQUAL);
            }
            return makeToken(TOKEN_ASSIGN);
        case '!':
            if (peek() == '=') {
                advance();
                return makeToken(TOKEN_NOT_EQUAL);
            }
            return makeToken(TOKEN_LOGICAL_NOT);
        case '<':
            if (peek() == '<') {
                advance();
                if (peek() == '=') {
                    advance();
                    return makeToken(TOKEN_LEFT_SHIFT_ASSIGN);
                }
                return makeToken(TOKEN_LEFT_SHIFT);
            } else if (peek() == '=') {
                advance();
                return makeToken(TOKEN_LESS_EQUAL);
            }
            return makeToken(TOKEN_LESS);
        case '>':
            if (peek() == '>') {
                advance();
                if (peek() == '=') {
                    advance();
                    return makeToken(TOKEN_RIGHT_SHIFT_ASSIGN);
                }
                return makeToken(TOKEN_RIGHT_SHIFT);
            } else if (peek() == '=') {
                advance();
                return makeToken(TOKEN_GREATER_EQUAL);
            }
            return makeToken(TOKEN_GREATER);
        case '&':
            if (peek() == '&') {
                advance();
                return makeToken(TOKEN_LOGICAL_AND);
            } else if (peek() == '=') {
                advance();
                return makeToken(TOKEN_AND_ASSIGN);
            }
            return makeToken(TOKEN_BITWISE_AND);
        case '|':
            if (peek() == '|') {
                advance();
                return makeToken(TOKEN_LOGICAL_OR);
            } else if (peek() == '=') {
                advance();
                return makeToken(TOKEN_OR_ASSIGN);
            }
            return makeToken(TOKEN_BITWISE_OR);
        case '^':
            if (peek() == '=') {
                advance();
                return makeToken(TOKEN_XOR_ASSIGN);
            }
            return makeToken(TOKEN_BITWISE_XOR);
        case '~':
            return makeToken(TOKEN_BITWISE_NOT);
        case '?':
            return makeToken(TOKEN_QUESTION);
        case ':':
            return makeToken(TOKEN_COLON);
        case '#':
            if (peek() == '#') {
                advance();
                return makeToken(TOKEN_DOUBLE_HASH);
            }
            return makeToken(TOKEN_HASH);
    }
    
    return errorToken("Unexpected character");
}

// Public interface functions
void initLexer(const char* source) {
    initLexerWithOptions(source, "<unknown>");
}

void initLexerWithOptions(const char* source, const char* filename) {
    lexer.source = source;
    lexer.current = source;
    lexer.start = source;
    lexer.line = 1;
    lexer.column = 1;
    lexer.filename = filename ? strdup(filename) : strdup("<unknown>");
    lexer.errorCount = 0;
    lexer.lastError = NULL;
    
    // Set default options
    lexer.options.enableC99Extensions = 1;
    lexer.options.enableGNUExtensions = 1;
    lexer.options.enableInlineAsm = 1;
    lexer.options.enableWideStrings = 1;
    lexer.options.trigraphs = 0;
    lexer.options.digraphs = 1;
    
    // Initialize current token
    lexer.currentToken = scanToken();
    lexer.nextToken = scanToken();
}

void cleanupLexer(void) {
    if (lexer.filename) {
        free(lexer.filename);
        lexer.filename = NULL;
    }
    if (lexer.lastError) {
        free(lexer.lastError);
        lexer.lastError = NULL;
    }
    if (lexer.currentToken.value) {
        freeTokenString(lexer.currentToken.value);
        lexer.currentToken.value = NULL;
    }
    if (lexer.nextToken.value) {
        freeTokenString(lexer.nextToken.value);
        lexer.nextToken.value = NULL;
    }
}

Token nextToken(void) {
    Token current = lexer.currentToken;
    
    // If current token is already EOF, just return it without advancing
    if (current.type == TOKEN_EOF) {
        return current;
    }
    
    // If we're already at EOF, don't scan for more tokens
    if (lexer.nextToken.type == TOKEN_EOF) {
        lexer.currentToken = lexer.nextToken;
        return current;
    }
    
    lexer.currentToken = lexer.nextToken;
    lexer.nextToken = scanToken();
    return current;
}

Token peekToken(void) {
    return lexer.nextToken;
}

Token getCurrentToken(void) {
    return lexer.currentToken;
}

int matchToken(TokenType expected) {
    return lexer.currentToken.type == expected;
}

int consumeToken(TokenType expected) {
    if (matchToken(expected)) {
        nextToken();
        return 1;
    }
    return 0;
}

// Utility functions
const char* getTokenTypeName(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_UNKNOWN: return "UNKNOWN";
        case TOKEN_INTEGER_LITERAL: return "INTEGER";
        case TOKEN_FLOAT_LITERAL: return "FLOAT";
        case TOKEN_DOUBLE_LITERAL: return "DOUBLE";
        case TOKEN_LONG_DOUBLE_LITERAL: return "LONG_DOUBLE";
        case TOKEN_CHAR_LITERAL: return "CHAR";
        case TOKEN_STRING_LITERAL: return "STRING";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_IF: return "if";
        case TOKEN_ELSE: return "else";
        case TOKEN_WHILE: return "while";
        case TOKEN_FOR: return "for";
        case TOKEN_DO: return "do";
        case TOKEN_SWITCH: return "switch";
        case TOKEN_CASE: return "case";
        case TOKEN_DEFAULT: return "default";
        case TOKEN_BREAK: return "break";
        case TOKEN_CONTINUE: return "continue";
        case TOKEN_RETURN: return "return";
        case TOKEN_GOTO: return "goto";
        case TOKEN_SIZEOF: return "sizeof";
        case TOKEN_TYPEDEF: return "typedef";
        case TOKEN_STRUCT: return "struct";
        case TOKEN_UNION: return "union";
        case TOKEN_ENUM: return "enum";
        case TOKEN_STATIC: return "static";
        case TOKEN_EXTERN: return "extern";
        case TOKEN_AUTO: return "auto";
        case TOKEN_REGISTER: return "register";
        case TOKEN_CONST: return "const";
        case TOKEN_VOLATILE: return "volatile";
        case TOKEN_VOID: return "void";
        case TOKEN_CHAR: return "char";
        case TOKEN_SHORT: return "short";
        case TOKEN_INT: return "int";
        case TOKEN_LONG: return "long";
        case TOKEN_FLOAT: return "float";
        case TOKEN_DOUBLE: return "double";
        case TOKEN_SIGNED: return "signed";
        case TOKEN_UNSIGNED: return "unsigned";
        case TOKEN_INLINE: return "inline";
        case TOKEN_RESTRICT: return "restrict";
        case TOKEN_BOOL: return "_Bool";
        case TOKEN_COMPLEX: return "_Complex";
        case TOKEN_IMAGINARY: return "_Imaginary";
        case TOKEN_TRUE: return "true";
        case TOKEN_FALSE: return "false";
        case TOKEN_ASM: return "asm";
        default: return "UNKNOWN_TOKEN";
    }
}

const char* getTokenValue(Token* token) {
    return token->value;
}

void printToken(Token* token) {
    printf("Token: %s, Value: '%s', Line: %d, Column: %d\n",
           getTokenTypeName(token->type), token->value, token->line, token->column);
}

// Error handling
void lexerError(int line, int column, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char message[512];
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Lexer error: %s", message);
    lexer.lastError = strdup(buffer);
    lexer.errorCount++;
    
    // Report to error manager
    reportError(ERROR_ERROR, ERROR_LEXER, line, column, "%s", message);
}

void lexerWarning(int line, int column, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char message[512];
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    // Report to error manager  
    reportError(ERROR_WARNING, ERROR_LEXER, line, column, "%s", message);
}

int hasLexerErrors(void) {
    return lexer.errorCount > 0;
}

const char* getLastLexerError(void) {
    return lexer.lastError;
}
