#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    // Keywords
    TOKEN_INT,
    TOKEN_SHORT,
    TOKEN_UNSIGNED,
    TOKEN_CHAR,
    TOKEN_VOID,    TOKEN_FAR,
    TOKEN_ASM,
    TOKEN_STACKFRAME,
    TOKEN_FARCALLED,
    TOKEN_ATTRIBUTE,     // __attribute__
    TOKEN_NAKED,         // naked
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_RETURN,
    
    // Identifiers and literals
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_CHAR_LITERAL,
      // Operators
    TOKEN_PLUS,       // +
    TOKEN_MINUS,      // -
    TOKEN_STAR,       // *
    TOKEN_SLASH,      // /
    TOKEN_PERCENT,    // %
    TOKEN_ASSIGN,     // =
    TOKEN_EQ,         // ==
    TOKEN_NEQ,        // !=
    TOKEN_LT,         // <
    TOKEN_LTE,        // <=
    TOKEN_GT,         // >
    TOKEN_GTE,        // >=
    TOKEN_LEFT_SHIFT, // <<
    TOKEN_RIGHT_SHIFT,// >>
    TOKEN_AND,        // &&
    TOKEN_OR,         // ||
    TOKEN_NOT,        // !
    TOKEN_AMPERSAND,  // &
    TOKEN_PIPE,       // |
    TOKEN_INCREMENT,  // ++
    TOKEN_DECREMENT,  // --
    TOKEN_BITWISE_NOT,// ~
    TOKEN_XOR,        // ^
    TOKEN_ARROW,      // ->

    // Compound assignment operators
    TOKEN_PLUS_ASSIGN,  // +=
    TOKEN_MINUS_ASSIGN, // -=
    TOKEN_MUL_ASSIGN,   // *=
    TOKEN_DIV_ASSIGN,   // /=
    TOKEN_MOD_ASSIGN,   // %=  

    // Punctuation
    TOKEN_LBRACE,     // {
    TOKEN_RBRACE,     // }
    TOKEN_LPAREN,     // (
    TOKEN_RPAREN,     // )
    TOKEN_LBRACKET,   // [
    TOKEN_RBRACKET,   // ]
    TOKEN_SEMICOLON,  // ;
    TOKEN_COLON,      // :
    TOKEN_COMMA,      // ,
    TOKEN_DOT,        // .
    
    // Special tokens
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    char* value;
    int line;   // Line number for error reporting
    int column; // Column number for error reporting
    int pos;    // Position in source buffer for error reporting
} Token;

// Initialize lexer with source code
void initLexer(const char* src);

// Get the next token
Token getNextToken();

// Peek at the next token without consuming it
Token peekNextToken();

// Get the current token
Token getCurrentToken();

// Check if the current token is of a specific type
int tokenIs(TokenType type);

// Consume the current token if it matches the expected type
int consume(TokenType type);

// Consume the current token and return its value
char* consumeAndGetValue(TokenType type);

// Report a syntax error
void syntaxError(const char* message);

#endif // LEXER_H
