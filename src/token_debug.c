#include "lexer.h"
#include <stdio.h>

// Function         case TOKEN_XOR: return "TOKEN_XOR (^)";
const char* getTokenName(TokenType type) {
    switch (type) {
        // Keywords
        case TOKEN_INT: return "TOKEN_INT";
        case TOKEN_SHORT: return "TOKEN_SHORT";
        case TOKEN_UNSIGNED: return "TOKEN_UNSIGNED";
        case TOKEN_CHAR: return "TOKEN_CHAR";
        case TOKEN_VOID: return "TOKEN_VOID";
        case TOKEN_FAR: return "TOKEN_FAR";
        case TOKEN_ASM: return "TOKEN_ASM";
        case TOKEN_STACKFRAME: return "TOKEN_STACKFRAME";
        case TOKEN_FARCALLED: return "TOKEN_FARCALLED";
        case TOKEN_IF: return "TOKEN_IF";
        case TOKEN_ELSE: return "TOKEN_ELSE";
        case TOKEN_WHILE: return "TOKEN_WHILE";
        case TOKEN_FOR: return "TOKEN_FOR";
        case TOKEN_RETURN: return "TOKEN_RETURN";
        case TOKEN_BOOL: return "TOKEN_BOOL";
        case TOKEN_TRUE: return "TOKEN_TRUE";        case TOKEN_FALSE: return "TOKEN_FALSE";
        case TOKEN_ATTR_OPEN: return "TOKEN_ATTR_OPEN";
        case TOKEN_ATTR_CLOSE: return "TOKEN_ATTR_CLOSE";
        case TOKEN_ELLIPSIS: return "TOKEN_ELLIPSIS (...)";

        // Identifiers and literals
        case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
        case TOKEN_NUMBER: return "TOKEN_NUMBER";
        case TOKEN_STRING: return "TOKEN_STRING";
        case TOKEN_CHAR_LITERAL: return "TOKEN_CHAR_LITERAL";
        
        // Operators
        case TOKEN_PLUS: return "TOKEN_PLUS (+)";
        case TOKEN_MINUS: return "TOKEN_MINUS (-)";
        case TOKEN_STAR: return "TOKEN_STAR (*)";
        case TOKEN_SLASH: return "TOKEN_SLASH (/)";
        case TOKEN_PERCENT: return "TOKEN_PERCENT (%)";
        case TOKEN_ASSIGN: return "TOKEN_ASSIGN (=)";
        case TOKEN_EQ: return "TOKEN_EQ (==)";
        case TOKEN_NEQ: return "TOKEN_NEQ (!=)";
        case TOKEN_LT: return "TOKEN_LT (<)";
        case TOKEN_LTE: return "TOKEN_LTE (<=)";
        case TOKEN_GT: return "TOKEN_GT (>)";
        case TOKEN_GTE: return "TOKEN_GTE (>=)";
        case TOKEN_AND: return "TOKEN_AND (&&)";
        case TOKEN_OR: return "TOKEN_OR (||)";
        case TOKEN_NOT: return "TOKEN_NOT (!)";
        case TOKEN_AMPERSAND: return "TOKEN_AMPERSAND (&)";
        case TOKEN_PIPE: return "TOKEN_PIPE (|)";
        case TOKEN_INCREMENT: return "TOKEN_INCREMENT (++)";
        case TOKEN_DECREMENT: return "TOKEN_DECREMENT (--)";        
        case TOKEN_BITWISE_NOT: return "TOKEN_BITWISE_NOT (~)";
        case TOKEN_XOR: return "TOKEN_XOR (^)";
        case TOKEN_ARROW: return "TOKEN_ARROW (->)";
        case TOKEN_QUESTION: return "TOKEN_QUESTION (?)";
        // Compound assignment operators
        case TOKEN_PLUS_ASSIGN: return "TOKEN_PLUS_ASSIGN (+=)";
        case TOKEN_MINUS_ASSIGN: return "TOKEN_MINUS_ASSIGN (-=)";
        case TOKEN_MUL_ASSIGN: return "TOKEN_MUL_ASSIGN (*=)";
        case TOKEN_DIV_ASSIGN: return "TOKEN_DIV_ASSIGN (/=)";
        case TOKEN_MOD_ASSIGN: return "TOKEN_MOD_ASSIGN (%=)";
        
        // Punctuation
        case TOKEN_LBRACE: return "TOKEN_LBRACE ({)";
        case TOKEN_RBRACE: return "TOKEN_RBRACE (})";
        case TOKEN_LPAREN: return "TOKEN_LPAREN (()";
        case TOKEN_RPAREN: return "TOKEN_RPAREN ())";
        case TOKEN_LBRACKET: return "TOKEN_LBRACKET ([)";
        case TOKEN_RBRACKET: return "TOKEN_RBRACKET (])";
        case TOKEN_SEMICOLON: return "TOKEN_SEMICOLON (;)";
        case TOKEN_COLON: return "TOKEN_COLON (:)";
        case TOKEN_COMMA: return "TOKEN_COMMA (,)";
        case TOKEN_DOT: return "TOKEN_DOT (.)";
        
        // Special tokens
        case TOKEN_EOF: return "TOKEN_EOF";
        
        default: return "UNKNOWN_TOKEN";
    }
}
