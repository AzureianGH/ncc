#ifndef AST_H
#define AST_H

#include <stddef.h>
#include <stdint.h>

// Forward declarations
typedef struct ASTNode ASTNode;
typedef struct Symbol Symbol;
typedef struct SymbolTable SymbolTable;

// C99 Data Types
typedef enum {
    TYPE_VOID,
    TYPE_CHAR,
    TYPE_SHORT,
    TYPE_INT,
    TYPE_LONG,
    TYPE_LONG_LONG,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_LONG_DOUBLE,
    TYPE_BOOL,              // C99 _Bool
    TYPE_COMPLEX_FLOAT,     // C99 complex types
    TYPE_COMPLEX_DOUBLE,
    TYPE_COMPLEX_LONG_DOUBLE,
    TYPE_POINTER,
    TYPE_ARRAY,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_ENUM,
    TYPE_FUNCTION,
    TYPE_TYPEDEF,
    TYPE_AUTO,              // C99 type inference placeholder
    TYPE_UNKNOWN
} DataType;

// Type qualifiers
typedef enum {
    QUAL_NONE = 0,
    QUAL_CONST = 1 << 0,
    QUAL_VOLATILE = 1 << 1,
    QUAL_RESTRICT = 1 << 2   // C99 restrict qualifier
} TypeQualifier;

// Storage class specifiers
typedef enum {
    STORAGE_NONE = 0,
    STORAGE_AUTO = 1 << 0,
    STORAGE_REGISTER = 1 << 1,
    STORAGE_STATIC = 1 << 2,
    STORAGE_EXTERN = 1 << 3,
    STORAGE_TYPEDEF = 1 << 4,
    STORAGE_INLINE = 1 << 5   // C99 inline
} StorageClass;

// Function specifiers
typedef enum {
    FUNC_NONE = 0,
    FUNC_INLINE = 1 << 0,     // C99 inline
    FUNC_NORETURN = 1 << 1    // C99 _Noreturn
} FunctionSpecifier;

// Type information
typedef struct TypeInfo {
    DataType baseType;
    TypeQualifier qualifiers;
    StorageClass storageClass;
    FunctionSpecifier funcSpecifiers;
    
    // For arrays
    int arraySize;
    struct TypeInfo* elementType;
    
    // For pointers
    struct TypeInfo* pointsTo;
    
    // For functions
    struct TypeInfo* returnType;
    struct TypeInfo** paramTypes;
    int paramCount;
    int isVariadic;           // C99 variadic functions
    
    // For structs/unions
    char* structName;
    struct Symbol** members;
    int memberCount;
    
    // For enums
    char* enumName;
    struct Symbol** enumValues;
    int enumCount;
    
    // Size and alignment info
    int size;
    int alignment;
} TypeInfo;

// AST Node Types - Full C99 Support
typedef enum {
    // Literals
    AST_INTEGER_LITERAL,
    AST_FLOAT_LITERAL,
    AST_DOUBLE_LITERAL,
    AST_LONG_DOUBLE_LITERAL,
    AST_CHAR_LITERAL,
    AST_STRING_LITERAL,
    AST_WIDE_STRING_LITERAL,   // C99 wide strings
    AST_BOOL_LITERAL,          // C99 true/false
    
    // Identifiers
    AST_IDENTIFIER,
    
    // Expressions
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_TERNARY_OP,           // condition ? true : false
    AST_ASSIGNMENT,
    AST_COMPOUND_ASSIGNMENT,  // +=, -=, etc.
    AST_FUNCTION_CALL,
    AST_ARRAY_ACCESS,
    AST_MEMBER_ACCESS,        // struct.member
    AST_POINTER_ACCESS,       // struct->member
    AST_CAST,
    AST_SIZEOF,
    AST_ALIGNOF,              // C99 _Alignof
    AST_COMPOUND_LITERAL,     // C99 (int[]){1,2,3}
    AST_DESIGNATED_INIT,      // C99 .member = value
    AST_VA_ARG,               // C99 va_arg
    
    // Statements
    AST_EXPRESSION_STMT,
    AST_COMPOUND_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_DO_WHILE_STMT,
    AST_FOR_STMT,
    AST_SWITCH_STMT,
    AST_CASE_STMT,
    AST_DEFAULT_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_RETURN_STMT,
    AST_GOTO_STMT,
    AST_LABEL_STMT,
    
    // Declarations
    AST_VARIABLE_DECL,
    AST_FUNCTION_DECL,
    AST_STRUCT_DECL,
    AST_UNION_DECL,
    AST_ENUM_DECL,
    AST_TYPEDEF_DECL,
    
    // Program structure
    AST_PROGRAM,
    AST_TRANSLATION_UNIT,
    
    // C99 specific
    AST_INLINE_ASM,           // Inline assembly
    AST_PRAGMA,               // #pragma directives
    AST_STATIC_ASSERT,        // C99 _Static_assert
    AST_GENERIC_SELECTION,    // C99 _Generic
    
    // Array and struct initializers
    AST_INITIALIZER_LIST,
    AST_DESIGNATED_INITIALIZER,
    
    // Variable length arrays (C99)
    AST_VLA_DECL,
    
    AST_UNKNOWN
} ASTNodeType;

// Binary operators
typedef enum {
    // Arithmetic
    BIN_ADD, BIN_SUB, BIN_MUL, BIN_DIV, BIN_MOD,
    
    // Bitwise
    BIN_BITWISE_AND, BIN_BITWISE_OR, BIN_BITWISE_XOR,
    BIN_LEFT_SHIFT, BIN_RIGHT_SHIFT,
    
    // Logical
    BIN_LOGICAL_AND, BIN_LOGICAL_OR,
    
    // Comparison
    BIN_EQ, BIN_NE, BIN_LT, BIN_LE, BIN_GT, BIN_GE,
    
    // Assignment
    BIN_ASSIGN,
    BIN_ADD_ASSIGN, BIN_SUB_ASSIGN, BIN_MUL_ASSIGN,
    BIN_DIV_ASSIGN, BIN_MOD_ASSIGN,
    BIN_AND_ASSIGN, BIN_OR_ASSIGN, BIN_XOR_ASSIGN,
    BIN_LEFT_SHIFT_ASSIGN, BIN_RIGHT_SHIFT_ASSIGN,
    
    // Other
    BIN_COMMA
} BinaryOperator;

// Unary operators
typedef enum {
    // Arithmetic
    UNARY_PLUS, UNARY_MINUS, UNARY_NOT, UNARY_BITWISE_NOT,
    
    // Memory
    UNARY_ADDRESS_OF, UNARY_DEREFERENCE,
    
    // Increment/Decrement
    UNARY_PRE_INCREMENT, UNARY_POST_INCREMENT,
    UNARY_PRE_DECREMENT, UNARY_POST_DECREMENT,
    
    // Type operations
    UNARY_SIZEOF, UNARY_ALIGNOF
} UnaryOperator;

// AST Node structure
struct ASTNode {
    ASTNodeType type;
    TypeInfo* typeInfo;
    
    // Source location info
    int line;
    int column;
    char* filename;
    
    // Node-specific data
    union {
        // Literals
        struct {
            long long intValue;
            double floatValue;
            char* stringValue;
            int stringLength;
        } literal;
        
        // Identifiers
        struct {
            char* name;
            Symbol* symbol;
        } identifier;
        
        // Binary operations
        struct {
            BinaryOperator op;
            ASTNode* left;
            ASTNode* right;
        } binary;
        
        // Unary operations
        struct {
            UnaryOperator op;
            ASTNode* operand;
        } unary;
        
        // Ternary (conditional) operation
        struct {
            ASTNode* condition;
            ASTNode* trueExpr;
            ASTNode* falseExpr;
        } ternary;
        
        // Function calls
        struct {
            ASTNode* function;
            ASTNode** arguments;
            int argCount;
        } call;
        
        // Array access
        struct {
            ASTNode* array;
            ASTNode* index;
        } arrayAccess;
        
        // Member access
        struct {
            ASTNode* object;
            char* memberName;
            int isPointer;  // -> vs .
        } memberAccess;
        
        // Statements
        struct {
            ASTNode** statements;
            int count;
        } compound;
        
        struct {
            ASTNode* condition;
            ASTNode* thenStmt;
            ASTNode* elseStmt;
        } ifStmt;
        
        struct {
            ASTNode* condition;
            ASTNode* body;
        } whileStmt;
        
        struct {
            ASTNode* init;
            ASTNode* condition;
            ASTNode* update;
            ASTNode* body;
        } forStmt;
        
        struct {
            ASTNode* expression;
            ASTNode** cases;
            int caseCount;
        } switchStmt;
        
        struct {
            ASTNode* value;
            ASTNode* stmt;
        } caseStmt;
        
        struct {
            ASTNode* expression;
        } returnStmt;
        
        // Declarations
        struct {
            char* name;
            TypeInfo* type;
            ASTNode* initializer;
            StorageClass storageClass;
        } varDecl;
        
        struct {
            char* name;
            TypeInfo* returnType;
            struct Parameter** parameters;
            int paramCount;
            ASTNode* body;
            StorageClass storageClass;
            FunctionSpecifier specifiers;
            // Attributes
            int isNaked;
            int isDeprecated;
            char* deprecatedMessage;
        } funcDecl;
        
        // Inline assembly
        struct {
            char* assembly;
            char** outputConstraints;
            char** inputConstraints;
            char** clobbers;
            // Operand names (identifiers) corresponding to constraints (best-effort parsing)
            char** outputOperands;
            char** inputOperands;
            int isVolatile;
            int outputCount;
            int inputCount;
            int clobberCount;
        } inlineAsm;
        
        // Initializer lists
        struct {
            ASTNode** elements;
            int count;
        } initList;
        
        // Designated initializers
        struct {
            ASTNode** designators;  // .member or [index]
            ASTNode* value;
            int designatorCount;
        } designatedInit;
    } data;
    
    // Child nodes (for generic traversal)
    ASTNode** children;
    int childCount;
};

// Function parameter
typedef struct Parameter {
    char* name;
    TypeInfo* type;
} Parameter;

// Function declarations
ASTNode* createASTNode(ASTNodeType type);
void freeASTNode(ASTNode* node);
void printAST(ASTNode* node, int indent);
ASTNode* copyASTNode(ASTNode* node);

// Type system functions
TypeInfo* createTypeInfo(DataType baseType);
void freeTypeInfo(TypeInfo* type);
TypeInfo* copyTypeInfo(TypeInfo* type);
int getTypeSize(TypeInfo* type, int targetWidth);
int getTypeAlignment(TypeInfo* type, int targetWidth);
int areTypesCompatible(TypeInfo* type1, TypeInfo* type2);
TypeInfo* getCommonType(TypeInfo* type1, TypeInfo* type2);

// C99 specific type functions
int isIntegerType(TypeInfo* type);
int isFloatingType(TypeInfo* type);
int isArithmeticType(TypeInfo* type);
int isScalarType(TypeInfo* type);
int isComplexType(TypeInfo* type);
int isRealType(TypeInfo* type);

// AST utility functions
void addChild(ASTNode* parent, ASTNode* child);
ASTNode* findNodeByType(ASTNode* root, ASTNodeType type);
void visitAST(ASTNode* node, void (*visitor)(ASTNode*, void*), void* context);

// AST cleanup
void cleanupAST(ASTNode* root);

#endif // AST_H
