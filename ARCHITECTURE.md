# NCC Architectural Guide

This document provides a deeper understanding of NCC's internal architecture, aimed at developers who want to modify or extend the compiler.

## Compiler Pipeline

NCC follows a traditional compiler pipeline:

```
Source Code → Lexer → Parser → Type Checker → Code Generator → Assembly Output
```

Each stage transforms the input into a more structured representation until we get the final assembly code.

## 1. Lexical Analysis (`lexer.c`, `lexer.h`)

The lexer reads the source code character by character and groups them into tokens. Key components:

- **Token Structure**: Defined in `lexer.h`, each token has a type, value, and position
- **Lexer State**: Tracks current position, line number, and buffered characters
- **Token Recognition**: Identifies keywords, identifiers, literals, and operators

### How to Modify:

- To add a new keyword: Add a token type in `lexer.h` and update the keyword recognition in `lexer.c`
- To support new operators: Add token types and implement detection in the main lexing loop

## 2. Parsing (`parser.c`, `parser.h` and specialized parsers)

The parser builds an Abstract Syntax Tree (AST) representing the structure of the program:

- **AST Nodes**: Defined in `ast.h`, representing different language constructs
- **Recursive Descent**: The parsing approach used for expressions and statements
- **Declaration Parsing**: Handles variables, functions, and struct definitions
- **Expression Parsing**: Implements operator precedence and associativity

### How to Modify:

- To add a new statement type: Create a new AST node type and implement a parsing function
- To add a new expression: Update the relevant parsing function in the expression hierarchy
- For complex features: Create a dedicated parsing module (e.g., `struct_parser.c`)

## 3. Semantic Analysis (`type_checker.c`)

The type checker validates the AST and ensures operations are type-compatible:

- **Symbol Table**: Tracks variables, functions, and types
- **Type Information**: Stores information about each symbol's type
- **Type Compatibility**: Checks if operations between types are valid
- **Type Conversion**: Handles implicit and explicit type conversions

### How to Modify:

- To support new types: Add type entries in `ast.h` and update type checking rules
- To add type validation: Implement type checking for new language constructs
- For complex type systems: Create dedicated modules for specialized type checking

## 4. Code Generation (`codegen.c` and specialized generators)

The code generator transforms the AST into assembly code:

- **Assembly Output**: Generates 8086 assembly instructions
- **Register Allocation**: Manages CPU registers during code generation
- **Memory Management**: Handles stack allocation for local variables
- **Function Calls**: Implements calling conventions for the target architecture

### How to Modify:

- To optimize generated code: Modify the instruction selection patterns
- To add new code patterns: Implement dedicated generators for complex constructs
- To target new architectures: Create alternative code generators

## Key Data Structures

### Abstract Syntax Tree (AST)

The AST is the central data structure in NCC, defined in `ast.h`:

```c
typedef struct ASTNode {
    NodeType type;
    struct ASTNode* left;
    struct ASTNode* right;
    struct ASTNode* next;
    
    union {
        // Different node-specific data structures
        // ...
    };
} ASTNode;
```

Each node type has specific fields in the union for its unique data.

### Type Information

Type information is stored in the `TypeInfo` structure:

```c
typedef struct TypeInfo {
    DataType type;
    int is_pointer;
    int is_far_pointer;
    int is_array;
    int array_size;
    int is_stackframe;
    int is_far;
    int is_static;
    StructInfo* struct_info;
} TypeInfo;
```

This structure tracks the complete type information for variables and expressions.

## Adding a New Feature

Let's walk through the process of adding a new feature, using a hypothetical `switch-case` statement as an example:

1. **Define AST Node Types**:
   ```c
   // In ast.h
   typedef enum {
       // ...existing types...
       NODE_SWITCH,
       NODE_CASE,
       NODE_DEFAULT,
   } NodeType;
   
   // Add to the union in ASTNode
   struct {
       struct ASTNode* expression;
       struct ASTNode* cases;
   } switch_stmt;
   
   struct {
       int value;
       struct ASTNode* body;
   } case_stmt;
   ```

2. **Add Token Types**:
   ```c
   // In lexer.h
   typedef enum {
       // ...existing tokens...
       TOKEN_SWITCH,
       TOKEN_CASE,
       TOKEN_DEFAULT,
   } TokenType;
   ```

3. **Update Lexer**:
   ```c
   // In lexer.c
   if (strcmp(identifier, "switch") == 0) return TOKEN_SWITCH;
   if (strcmp(identifier, "case") == 0) return TOKEN_CASE;
   if (strcmp(identifier, "default") == 0) return TOKEN_DEFAULT;
   ```

4. **Implement Parsing**:
   ```c
   // In parser.c or a dedicated file
   ASTNode* parseSwitchStatement() {
       ASTNode* node = createNode(NODE_SWITCH);
       consume(TOKEN_SWITCH);
       expect(TOKEN_LPAREN);
       node->switch_stmt.expression = parseExpression();
       expect(TOKEN_RPAREN);
       expect(TOKEN_LBRACE);
       
       // Parse case statements
       // ...
       
       expect(TOKEN_RBRACE);
       return node;
   }
   ```

5. **Implement Type Checking**:
   ```c
   // In type_checker.c
   case NODE_SWITCH:
       // Verify switch expression is integral
       TypeInfo* type = getTypeInfoFromExpression(node->switch_stmt.expression);
       if (!isIntegralType(type)) {
           reportError("Switch expression must have integral type");
       }
       // Check case statements...
       break;
   ```

6. **Implement Code Generation**:
   ```c
   // In codegen.c or a dedicated file
   case NODE_SWITCH:
       generateExpression(node->switch_stmt.expression);
       // Generate comparison and jump instructions for each case
       // ...
       break;
   ```

## Target Architecture Considerations

NCC currently targets 8086/DOS systems, which affects several aspects of the compiler:

- **16-bit Registers**: Code generation uses 16-bit registers (AX, BX, CX, DX, SI, DI, BP, SP)
- **Segmented Memory Model**: Far pointers use segment:offset addressing
- **Stack Organization**: Local variables are accessed via BP-relative addressing
- **Calling Conventions**: Function calls follow specific register and stack usage patterns

To target a different architecture:

1. Create a new code generator module
2. Implement appropriate register allocation strategies
3. Adjust memory addressing modes
4. Update calling conventions
5. Modify the assembly syntax output

## Memory Model

NCC's memory model is designed for 16-bit DOS environments:

- **Near Pointers**: 16-bit offset within the current segment
- **Far Pointers**: 32-bit segment:offset pairs
- **Stack Variables**: Accessed via BP-relative addressing
- **Global Variables**: Accessed via symbolic names
- **Struct Layout**: Sequential member arrangement with appropriate alignment

## Optimization Opportunities

The current compiler focuses on correctness over optimization. Potential optimizations include:

- **Constant Folding**: Evaluate constant expressions at compile time
- **Strength Reduction**: Replace expensive operations with cheaper equivalents
- **Dead Code Elimination**: Remove unreachable or unused code
- **Register Allocation**: Improve register usage to minimize memory access
- **Peephole Optimization**: Local instruction pattern improvements

## Testing and Debugging

When modifying the compiler:

1. Create small test cases that isolate the feature you're implementing
2. Test each phase independently (lexing, parsing, type checking, code generation)
3. Use the `-v` verbose flag to see detailed compiler output
4. Examine the generated assembly to verify correctness
5. Test compiled programs in a DOS environment or emulator

## Conclusion

NCC is designed to be understandable and modifiable. By following the architectural patterns already established in the codebase, you can extend the compiler with new features or adapt it to different targets while maintaining its simplicity and clarity.
