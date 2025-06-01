#ifndef AST_H
#define AST_H

#include <stdio.h>

// Data types supported by the compiler
typedef enum {
    TYPE_INT,           // 16-bit signed integer
    TYPE_SHORT,         // 16-bit signed integer (alias)
    TYPE_UNSIGNED_INT,  // 16-bit unsigned integer
    TYPE_UNSIGNED_SHORT,// 16-bit unsigned integer (alias)
    TYPE_LONG,          // 32-bit signed integer
    TYPE_UNSIGNED_LONG, // 32-bit unsigned integer
    TYPE_CHAR,          // 8-bit signed integer
    TYPE_UNSIGNED_CHAR, // 8-bit unsigned integer
    TYPE_VOID,          // void type
    TYPE_FAR_POINTER,   // Far pointer (segment:offset)
    TYPE_BOOL,          // C23 bool type (1 byte)
    TYPE_STRUCT         // struct type
} DataType;

// AST node types
typedef enum {    NODE_PROGRAM,      // Program root
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
    NODE_DO_WHILE,     // Do-while loop
    NODE_FOR,          // For loop
    NODE_CALL,         // Function call
    NODE_ASM_BLOCK,    // Inline assembly block
    NODE_ASM,          // Inline assembly
    NODE_EXPRESSION,   // Expression statement
    NODE_TERNARY,      // Ternary conditional expression (? :)
    NODE_STRUCT_DEF,   // Struct definition
    NODE_MEMBER_ACCESS // Struct member access
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
    OP_MOD_ASSIGN,  // %=
    OP_LEFT_SHIFT_ASSIGN,  // <<=
    OP_RIGHT_SHIFT_ASSIGN, // >>=
    OP_DOT,         // . (struct member access)
    OP_ARROW,       // -> (struct member access through pointer)
    OP_COMMA        // , (comma operator)
} OperatorType;

// Unary operators
typedef enum {
    UNARY_ADDRESS_OF,    // &x
    UNARY_DEREFERENCE,   // *x
    UNARY_NEGATE,        // -x
    UNARY_NOT,           // !x
    UNARY_BITWISE_NOT,   // ~x
    UNARY_SIZEOF,        // sizeof(x)
    UNARY_CAST,          // (type)x
    PREFIX_INCREMENT,    // ++x
    PREFIX_DECREMENT,    // --x
    POSTFIX_INCREMENT,   // x++
    POSTFIX_DECREMENT    // x--
} UnaryOperatorType;

// Forward declaration
struct ASTNode;

// Struct info forward declaration
typedef struct StructInfo StructInfo;

// Type information structure
typedef struct TypeInfo {
    DataType type;
    int is_pointer;
    int is_far_pointer;
    int is_array;
    int array_size;
    int is_stackframe;  // Function uses stackframe with register preservation
    int is_far;         // Function is far called
    int is_static;      // Has static storage duration
    StructInfo* struct_info; // Pointer to struct info when type is TYPE_STRUCT
} TypeInfo;

// Struct member declaration
typedef struct StructMember {
    char* name;                     // Member name
    TypeInfo type_info;             // Member type
    int offset;                     // Byte offset within struct
    struct StructMember* next;      // Next member in linked list
} StructMember;

// Struct definition information
struct StructInfo {
    char* name;                     // Name of the struct
    StructMember* members;          // List of struct members
    int size;                       // Total size of the struct in bytes
};

// Function information structure
typedef struct {
    TypeInfo return_type;
    int param_count;
    int is_stackframe;  // Uses stackframe with register preservation
    int is_far;        // Is a far function
    int is_naked;      // Function is naked (no prologue/epilogue)
    int is_static;     // Function has internal linkage
    int is_deprecated; // Function is deprecated (1 if true)
    char* deprecation_msg; // Deprecation message (NULL if not deprecated)
    int is_variadic;   // Function accepts variable arguments (...)
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
            DataType cast_type;   // For UNARY_CAST operations
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
            char* code;               // The assembly instruction template
            struct ASTNode** operands; // Array of operand expressions
            char** constraints;        // Array of constraint strings (e.g., "r", "m")
            int operand_count;         // Number of operands
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
        } for_loop;        // For while loops
        struct {
            struct ASTNode* condition; // Loop condition
            struct ASTNode* body;      // Loop body
        } while_loop;
        
        // For do-while loops
        struct {
            struct ASTNode* condition; // Loop condition
            struct ASTNode* body;      // Loop body
        } do_while_loop;
        
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
          // For ternary conditional expressions (condition ? expr_if_true : expr_if_false)
        struct {
            struct ASTNode* condition;  // Condition expression
            struct ASTNode* true_expr;  // Expression if condition is true
            struct ASTNode* false_expr; // Expression if condition is false
        } ternary;
        
        // For struct definition (struct name { members })
        struct {
            char* struct_name;          // Name of the struct type
            StructInfo* info;           // Struct information
            struct ASTNode* members;    // List of member declarations
        } struct_def;
        
        // For struct member access (expr.member or expr->member)
        struct {
            OperatorType op;            // OP_DOT or OP_ARROW
            char* member_name;          // Name of the accessed member
        } member_access;
    };
}ASTNode;

// Function to create a new AST node
ASTNode* createNode(NodeType type);

// Function to print the AST for debugging
void printAST(ASTNode* node, int indent);

// Get the size of a data type in bytes
int getTypeSize(DataType type);

char * strdupc (const char *s);

#endif // AST_H