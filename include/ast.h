#ifndef AST_H
#define AST_H

#include <stdio.h>

// Data types supported by the compiler
typedef enum {
    TYPE_INT,           // 16-bit signed integer
    TYPE_SHORT,         // 16-bit signed integer (alias)
    TYPE_UNSIGNED_INT,  // 16-bit unsigned integer
    TYPE_UNSIGNED_SHORT,// 16-bit unsigned integer (alias)
    TYPE_CHAR,          // 8-bit signed integer
    TYPE_UNSIGNED_CHAR, // 8-bit unsigned integer
    TYPE_VOID,          // void type
    TYPE_FAR_POINTER,   // Far pointer (segment:offset)
    TYPE_BOOL           // C23 bool type (1 byte)
} DataType;

// AST node types
typedef enum {
    NODE_PROGRAM,      // Program root
    NODE_FUNCTION,     // Function definition
    NODE_BLOCK,        // Code block
    NODE_DECLARATION,  // Variable declaration
    NODE_ASSIGNMENT,   // Assignment
    NODE_BINARY_OP,    // Binary operation
    NODE_UNARY_OP,     // Unary operation
    NODE_IDENTIFIER,   // Variable identifier
    NODE_LITERAL,      // Constant literal
    NODE_RETURN,       // Return statement
    NODE_IF,           // If statement
    NODE_WHILE,        // While loop
    NODE_FOR,          // For loop
    NODE_CALL,         // Function call
    NODE_ASM_BLOCK,    // Inline assembly block
    NODE_ASM,          // Inline assembly
    NODE_EXPRESSION    // Expression statement
} NodeType;

// Binary operators
typedef enum {
    OP_ADD,  // +
    OP_SUB,  // -
    OP_MUL,  // *
    OP_DIV,  // /
    OP_MOD,  // %
    OP_EQ,   // ==
    OP_NEQ,  // !=    
    OP_LAND, // logical AND (&&)
    OP_LOR,  // logical OR (||)
    OP_LT,   // <
    OP_LTE,  // <=
    OP_GT,   // >
    OP_GTE,  // >=
    OP_BITWISE_AND, // &
    OP_BITWISE_OR,  // |
    OP_BITWISE_XOR, // ^
    OP_LEFT_SHIFT,  // <<
    OP_RIGHT_SHIFT, // >>
    OP_PLUS_ASSIGN, // +=
    OP_MINUS_ASSIGN,// -=
    OP_MUL_ASSIGN,  // *=
    OP_DIV_ASSIGN,  // /=
    OP_MOD_ASSIGN   // %=
} OperatorType;

// Unary operators
typedef enum {
    UNARY_ADDRESS_OF,    // &x
    UNARY_DEREFERENCE,   // *x
    UNARY_NEGATE,        // -x
    UNARY_NOT,           // !x
    UNARY_BITWISE_NOT,   // ~x
    UNARY_SIZEOF,        // sizeof(x)
    PREFIX_INCREMENT,    // ++x
    PREFIX_DECREMENT,    // --x
    POSTFIX_INCREMENT,   // x++
    POSTFIX_DECREMENT    // x--
} UnaryOperatorType;

// Forward declaration
struct ASTNode;

// Type information structure
typedef struct {
    DataType type;
    int is_pointer;
    int is_far_pointer;
    int is_array;
    int array_size;
    int is_stackframe;  // Function uses stackframe with register preservation
    int is_far;         // Function is far called
} TypeInfo;

// Function information structure
typedef struct {
    TypeInfo return_type;
    int param_count;
    int is_stackframe;  // Uses stackframe with register preservation
    int is_far;        // Is a far function
    int is_naked;      // Function is naked (no prologue/epilogue)
} FunctionInfo;

// AST Node structure
typedef struct ASTNode {
    NodeType type;
    struct ASTNode* left;
    struct ASTNode* right;
    struct ASTNode* next; // For lists of nodes
    
    union {
        // For literals
        struct {
            DataType data_type;
            union {
                int int_value;
                char char_value;
                unsigned int uint_value;
                char* string_value;
                struct {
                    int segment;
                    int offset;
                };
            };
        } literal;
        
        // For identifiers
        char* identifier;
        
        // For variable declarations
        struct {
            char* var_name;
            TypeInfo type_info;
            struct ASTNode* initializer;
        } declaration;
          // For binary operations
        struct {
            OperatorType op;
        } operation;
        
        // For unary operations
        struct {
            UnaryOperatorType op;
        } unary_op;
        // For function definitions
        struct {
            char* func_name;
            FunctionInfo info;
            struct ASTNode* body;
            struct ASTNode* params;
        } function;
          // For inline assembly block (between braces)
        struct {
            char* code;
        } asm_block;
        
        // For inline assembly statement
        struct {
            char* code;
        } asm_stmt;
        
        // For function calls
        struct {
            char* func_name;
            struct ASTNode* args;
            int arg_count;
        } call;
          // For return statements
        struct {
            struct ASTNode* expr;
        } return_stmt;
          // For for loops
        struct {
            struct ASTNode* init;      // Initialization statement
            struct ASTNode* condition; // Loop condition
            struct ASTNode* update;    // Update statement
            struct ASTNode* body;      // Loop body
        } for_loop;
          // For while loops
        struct {
            struct ASTNode* condition; // Loop condition
            struct ASTNode* body;      // Loop body
        } while_loop;
          // For if statements
        struct {
            struct ASTNode* condition; // If condition
            struct ASTNode* if_body;   // If body (true branch)
            struct ASTNode* else_body; // Else body (false branch), NULL if no else
        } if_stmt;
        
        // For assignment statements
        struct {
            OperatorType op;  // The operation to perform (OP_PLUS_ASSIGN, etc.)
        } assignment;
    };
}ASTNode;

// Function to create a new AST node
ASTNode* createNode(NodeType type);

// Function to print the AST for debugging
void printAST(ASTNode* node, int indent);

// Get the size of a data type in bytes
int getTypeSize(DataType type);

#endif // AST_H