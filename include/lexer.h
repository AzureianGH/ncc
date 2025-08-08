#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdint.h>

// C99 Token Types
typedef enum {
    // End of file/stream
    TOKEN_EOF,
    TOKEN_UNKNOWN,
    
    // Literals
    TOKEN_INTEGER_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_DOUBLE_LITERAL,
    TOKEN_LONG_DOUBLE_LITERAL,
    TOKEN_CHAR_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_WIDE_STRING_LITERAL,    // L"string"
    
    // Identifiers
    TOKEN_IDENTIFIER,
    
    // C99 Keywords
    TOKEN_AUTO,
    TOKEN_BREAK,
    TOKEN_CASE,
    TOKEN_CHAR,
    TOKEN_CONST,
    TOKEN_CONTINUE,
    TOKEN_DEFAULT,
    TOKEN_DO,
    TOKEN_DOUBLE,
    TOKEN_ELSE,
    TOKEN_ENUM,
    TOKEN_EXTERN,
    TOKEN_FLOAT,
    TOKEN_FOR,
    TOKEN_GOTO,
    TOKEN_IF,
    TOKEN_INT,
    TOKEN_LONG,
    TOKEN_REGISTER,
    TOKEN_RETURN,
    TOKEN_SHORT,
    TOKEN_SIGNED,
    TOKEN_SIZEOF,
    TOKEN_STATIC,
    TOKEN_STRUCT,
    TOKEN_SWITCH,
    TOKEN_TYPEDEF,
    TOKEN_UNION,
    TOKEN_UNSIGNED,
    TOKEN_VOID,
    TOKEN_VOLATILE,
    TOKEN_WHILE,
    
    // C99 specific keywords
    TOKEN_BOOL,               // _Bool
    TOKEN_COMPLEX,            // _Complex
    TOKEN_IMAGINARY,          // _Imaginary
    TOKEN_INLINE,             // inline
    TOKEN_RESTRICT,           // restrict
    TOKEN_ALIGNAS,            // _Alignas
    TOKEN_ALIGNOF,            // _Alignof
    TOKEN_ATOMIC,             // _Atomic
    TOKEN_STATIC_ASSERT,      // _Static_assert
    TOKEN_THREAD_LOCAL,       // _Thread_local
    TOKEN_GENERIC,            // _Generic
    TOKEN_NORETURN,           // _Noreturn
    
    // Boolean literals (C99)
    TOKEN_TRUE,               // true
    TOKEN_FALSE,              // false
    
    // Operators
    TOKEN_PLUS,               // +
    TOKEN_MINUS,              // -
    TOKEN_MULTIPLY,           // *
    TOKEN_DIVIDE,             // /
    TOKEN_MODULO,             // %
    TOKEN_ASSIGN,             // =
    TOKEN_PLUS_ASSIGN,        // +=
    TOKEN_MINUS_ASSIGN,       // -=
    TOKEN_MULTIPLY_ASSIGN,    // *=
    TOKEN_DIVIDE_ASSIGN,      // /=
    TOKEN_MODULO_ASSIGN,      // %=
    TOKEN_INCREMENT,          // ++
    TOKEN_DECREMENT,          // --
    
    // Comparison operators
    TOKEN_EQUAL,              // ==
    TOKEN_NOT_EQUAL,          // !=
    TOKEN_LESS,               // <
    TOKEN_LESS_EQUAL,         // <=
    TOKEN_GREATER,            // >
    TOKEN_GREATER_EQUAL,      // >=
    
    // Logical operators
    TOKEN_LOGICAL_AND,        // &&
    TOKEN_LOGICAL_OR,         // ||
    TOKEN_LOGICAL_NOT,        // !
    
    // Bitwise operators
    TOKEN_BITWISE_AND,        // &
    TOKEN_BITWISE_OR,         // |
    TOKEN_BITWISE_XOR,        // ^
    TOKEN_BITWISE_NOT,        // ~
    TOKEN_LEFT_SHIFT,         // <<
    TOKEN_RIGHT_SHIFT,        // >>
    TOKEN_AND_ASSIGN,         // &=
    TOKEN_OR_ASSIGN,          // |=
    TOKEN_XOR_ASSIGN,         // ^=
    TOKEN_LEFT_SHIFT_ASSIGN,  // <<=
    TOKEN_RIGHT_SHIFT_ASSIGN, // >>=
    
    // Punctuation
    TOKEN_SEMICOLON,          // ;
    TOKEN_COMMA,              // ,
    TOKEN_DOT,                // .
    TOKEN_ARROW,              // ->
    TOKEN_QUESTION,           // ?
    TOKEN_COLON,              // :
    TOKEN_ELLIPSIS,           // ... (C99 variadic)
    
    // Brackets
    TOKEN_LEFT_PAREN,         // (
    TOKEN_RIGHT_PAREN,        // )
    TOKEN_LEFT_BRACKET,       // [
    TOKEN_RIGHT_BRACKET,      // ]
    TOKEN_LEFT_BRACE,         // {
    TOKEN_RIGHT_BRACE,        // }
    
    // Preprocessor tokens
    TOKEN_HASH,               // #
    TOKEN_DOUBLE_HASH,        // ##
    
    // Special tokens for inline assembly
    TOKEN_ASM,                // asm or __asm__
    TOKEN_VOLATILE_ASM,       // volatile (in asm context)
    
    // Attributes (GNU/C99 extensions)
    TOKEN_ATTRIBUTE,          // __attribute__
    TOKEN_PACKED,             // __packed__
    TOKEN_ALIGNED,            // __aligned__
    TOKEN_SECTION,            // __section__
    TOKEN_NAKED,              // __naked__ (for inline asm functions)
    TOKEN_DEPRECATED,         // __deprecated__
    
    TOKEN_COUNT
} TokenType;

// Token structure
typedef struct Token {
    TokenType type;
    char* value;              // Text representation
    int length;               // Length of token
    
    // Source location
    int line;
    int column;
    char* filename;
    
    // For numeric literals
    union {
        long long intValue;
        double floatValue;
        char charValue;
    } numericValue;
    
    // Token flags
    struct {
        unsigned int isUnsigned : 1;
        unsigned int isLong : 1;
        unsigned int isLongLong : 1;
        unsigned int isFloat : 1;
        unsigned int isDouble : 1;
        unsigned int isLongDouble : 1;
        unsigned int isWide : 1;      // For wide strings/chars
        unsigned int isHexFloat : 1;  // C99 hexadecimal float
    } flags;
} Token;

// Lexer state
typedef struct Lexer {
    const char* source;
    const char* current;
    const char* start;
    int line;
    int column;
    char* filename;
    
    // Current token
    Token currentToken;
    Token nextToken;
    
    // Lexer options
    struct {
        unsigned int enableC99Extensions : 1;
        unsigned int enableGNUExtensions : 1;
        unsigned int enableInlineAsm : 1;
        unsigned int enableWideStrings : 1;
        unsigned int trigraphs : 1;
        unsigned int digraphs : 1;
    } options;
    
    // Error handling
    int errorCount;
    char* lastError;
} Lexer;

// Function declarations
void initLexer(const char* source);
void initLexerWithOptions(const char* source, const char* filename);
void cleanupLexer(void);

// Token operations
Token nextToken(void);
Token peekToken(void);
Token getCurrentToken(void);
int matchToken(TokenType expected);
int consumeToken(TokenType expected);
void ungetToken(Token token);

// Utility functions
const char* getTokenTypeName(TokenType type);
const char* getTokenValue(Token* token);
void printToken(Token* token);
int isKeyword(const char* str);
TokenType getKeywordType(const char* str);

// Number parsing
int parseIntegerLiteral(const char* str, Token* token);
int parseFloatLiteral(const char* str, Token* token);
int parseCharLiteral(const char* str, Token* token);
int parseStringLiteral(const char* str, Token* token);

// C99 specific parsing
int parseHexFloat(const char* str, Token* token);
int parseComplexLiteral(const char* str, Token* token);
int parseUTF8String(const char* str, Token* token);
int parseUniversalCharName(const char* str, Token* token);

// String utilities
char* unescapeString(const char* str, int length);
char* createTokenString(const char* start, int length);
void freeTokenString(char* str);

// Error handling
void lexerError(int line, int column, const char* format, ...);
void lexerWarning(int line, int column, const char* format, ...);
int hasLexerErrors(void);
const char* getLastLexerError(void);

// Lexer state management
void saveLexerState(void);
void restoreLexerState(void);
void markPosition(void);
void resetToMark(void);

// Character classification
int isAlpha(char c);
int isDigit(char c);
int isAlnum(char c);
int isHexDigit(char c);
int isOctalDigit(char c);
int isBinaryDigit(char c);
int isWhitespace(char c);
int isNewline(char c);

// Trigraph and digraph support
char processTrigraph(const char* str);
TokenType processDigraph(const char* str);

// Preprocessor integration
void setPreprocessorMode(int enabled);
int isPreprocessorDirective(const char* line);

#endif // LEXER_H
