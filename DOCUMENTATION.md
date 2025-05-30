# NCC: Nathan's C Compiler

## Overview

NCC is a compact C compiler targeting 8086/DOS systems. It translates C code into x86 assembly, which can then be assembled into executables for DOS or similar environments. The compiler supports a significant subset of the C language including structs, arrays, pointers, control flow statements, and functions.

## Key Features

- **8086 Assembly Output**: Generates optimized assembly code for 16-bit x86 processors
- **C Standard Support**: Implements a useful subset of C features
- **Struct Support**: Full implementation of C structures, including member access
- **DOS Targeting**: Perfect for retro computing, embedded systems, or bootloader development
- **Small Footprint**: Minimalist implementation that's easy to understand and modify

## Getting Started

### Building the Compiler

```bash
# Clone the repository
git clone https://github.com/AzureianGH/ncc.git
cd ncc

# Build using make
make
```

### Compiling a C Program

```bash
# Compile a C file to assembly
./bin/ncc yourprogram.c -o yourprogram.asm

# To create an executable, use an assembler like NASM
nasm -f bin yourprogram.asm -o yourprogram.com

# To create a bootloader
./bin/ncc -sys yourbootloader.c -o yourbootloader.asm
nasm -f bin yourbootloader.asm -o yourbootloader.bin
```

## Compiler Architecture

NCC follows a traditional multi-pass compiler design:

1. **Lexical Analysis** - Tokenizes source code
2. **Parsing** - Builds an Abstract Syntax Tree (AST)
3. **Semantic Analysis** - Type checking and validation
4. **Code Generation** - Outputs assembly code

### Lexical Analysis

The lexer (`lexer.c`) breaks down the source code into tokens such as identifiers, keywords, operators, and literals. It handles preprocessor directives and comments. The tokenization process is the foundation for all subsequent analysis.

### Parsing

The parser (`parser.c` and related files) constructs an Abstract Syntax Tree (AST) that represents the program's structure. It implements recursive descent parsing for expressions, statements, declarations, and control structures.

### Semantic Analysis

Type checking (`type_checker.c`) verifies that operations are type-compatible and performs type conversions where necessary. This phase catches many semantic errors that aren't visible at the syntax level.

### Code Generation

The code generator (`codegen.c` and related files) translates the AST into 8086 assembly code. It handles:

- Register allocation
- Memory management
- Stack frame setup
- Function calls
- Efficient code patterns for common operations
- Bootloader initialization for system mode

#### System Mode (Bootloader) Support

For bootloader development, NCC provides specialized code generation via `initCodeGenSystemMode()` that:

- Sets the origin address to 0x7C00 (standard BIOS boot sector location)
- Optionally configures the stack segment and stack pointer
- Generates proper bootloader initialization code
- Handles entry point setup for BIOS loading

## Extending the Compiler

### Adding Language Features

To add a new language feature:

1. Add any necessary token types in `include/lexer.h`
2. Update the lexer in `src/lexer.c` to recognize the new syntax
3. Add corresponding AST node types in `include/ast.h`
4. Implement parsing logic in `src/parser.c` or a specialized file
5. Add type checking rules in `src/type_checker.c`
6. Implement code generation in `src/codegen.c` or a specialized file

### Example: Adding a New Control Structure

Let's say you want to add a `repeat-until` loop:

1. Add `TOKEN_REPEAT` and `TOKEN_UNTIL` to the token types
2. Update the lexer to recognize these keywords
3. Add `NODE_REPEAT_UNTIL` to the AST node types
4. Create a `parseRepeatUntilStatement()` function
5. Add code generation for the new construct

### Targeting a Different Architecture

To retarget NCC to a different architecture:

1. Create a new code generation module (e.g., `arm_codegen.c`)
2. Implement assembly generation for the target architecture
3. Update register allocation and calling conventions
4. Modify memory model assumptions if necessary
5. Update the build system to support the new target

## C Language Support

NCC currently supports:

- Basic types: `int`, `char`, `short`, `long`, `unsigned` variants
- Control structures: `if-else`, `while`, `do-while`, `for`
- Functions and function pointers
- Structs with member access
- Arrays and pointers
- Binary and unary operators
- Basic preprocessor directives

## Implementation Details

### Memory Management

NCC uses a simple approach to memory management with limited dynamic allocation during compilation. The AST nodes are allocated as needed and freed after code generation is complete.

### Symbol Tables

Symbol tables track variables, functions, and types throughout the compilation process. They store information about:

- Variable scope and storage duration
- Function signatures and return types
- Struct definitions and layouts

### Error Handling

The error manager provides detailed error messages with line and column information. It helps locate syntax errors, type mismatches, undefined symbols, and other common programming mistakes.

## Contributing

Contributions are welcome! Areas that could use improvement:

- More comprehensive standard library support
- Additional optimization passes
- Extended language features
- Better error reporting
- Support for additional target architectures

## License

MIT License is the used license.
