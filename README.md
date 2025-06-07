# NCC (Nathan's Compiler Collection)

A lightweight C compiler targeting real-mode/DOS systems, designed for simplicity, educational purposes, and retro computing. NCC compiles C code to x86 assembly and can generate bootloaders, MS-DOS executables, and bare-metal programs for 16-bit systems.

## Features

- **Full C Compiler Pipeline**: Lexical analysis, parsing, type checking, and code generation
- **Multiple Target Formats**:
  - MS-DOS executables (.COM files)
  - Bootloader binaries (512-byte MBR compatible)
  - Custom origin addresses for bare-metal programming
- **Advanced Language Support**:
  - Structs with nested field access
  - Arrays and array initialization
  - Control flow (if/else, while, do-while, for loops)
  - Function calls and parameters
  - Inline assembly with `__asm()` 
  - Preprocessor with includes and macros
  - Unary and compound expressions
- **Development Features**:
  - AST debugging and visualization
  - Line number tracking through preprocessing
  - Built-in assembler (NAS - Nathan's Assembler)
  - Optimization passes
  - Comprehensive error reporting

## Quick Start

### Building NCC

```bash
make
```

For a clean build:
```bash
make clean && make
```

### Basic Usage

```bash
# Compile to assembly
./bin/ncc source.c -o output.asm

# Compile MS-DOS .COM executable
./bin/ncc -com program.c -o program.com

# Compile bootloader
./bin/ncc -sys bootloader.c -o boot.bin

# Custom origin address
./bin/ncc -disp 0x8000 kernel.c -o kernel.bin
```

## Command Line Options

| Option | Description |
|--------|-------------|
| `-o <file>` | Output filename (default: output.asm) |
| `-com` | Target MS-DOS executable (ORG 0x100) |
| `-sys` | Target bootloader (ORG 0x7C00) |
| `-disp <addr>` | Set custom origin displacement address |
| `-I<path>` | Add include search path |
| `-O<level>` | Optimization level (0=none, 1=basic) |
| `-S` | Stop after assembly generation (don't assemble) |
| `-d` | Debug mode (print AST) |
| `-dl` | Debug line tracking |
| `-h` | Display help |

## Example Programs

### Hello World (MS-DOS)

```c
void main() {
    printstring("Hello, World!$");
}

void printstring(char* str) {
    __asm("mov dx, [bp+4]");  // Get string parameter
    __asm("mov ah, 0x09");    // DOS print string function
    __asm("int 0x21");        // Call DOS interrupt
}
```

Compile with:
```bash
./bin/ncc -com hello.c -o hello.com
```

### Simple Bootloader

```c
void main() {
    // Clear screen and print message
    __asm("mov ah, 0x06");   // Scroll up function
    __asm("mov al, 0");      // Clear entire screen
    __asm("mov bh, 0x07");   // Attribute
    __asm("mov cx, 0");      // Upper left
    __asm("mov dx, 0x184F"); // Lower right  
    __asm("int 0x10");       // BIOS video interrupt
    
    // Print boot message
    // ... (additional boot code)
    
    // Infinite loop
    while(1) {}
}
```

Compile with:
```bash
./bin/ncc -sys boot.c -o boot.bin
```

## Testing

### Run Test Suite

```bash
# Test MS-DOS program
make test_com

# Test bootloader in QEMU
make test_os
```

### Manual Testing

```bash
# Compile and run in QEMU
./bin/ncc -sys bootloader.c -o boot.bin
qemu-system-x86_64 -fda boot.bin
```

## Architecture

NCC is structured as a traditional compiler with these phases:

1. **Preprocessor** (`preprocessor.c`) - Handle includes and macros
2. **Lexer** (`lexer.c`) - Tokenize source code
3. **Parser** (`parser.c`) - Build abstract syntax tree
4. **Type Checker** (`type_checker.c`) - Semantic analysis
5. **Code Generator** (`codegen.c`) - Emit x86 assembly
6. **Assembler** (NAS) - Convert assembly to machine code

### Key Components

- **AST Management** - Full abstract syntax tree with cleanup
- **Struct Support** - Complete struct parsing and code generation  
- **Loop Constructs** - While, do-while, and for loop implementations
- **Expression Handling** - Unary operations and compound expressions
- **Memory Management** - String literals and array operations
- **Error Handling** - Comprehensive error reporting system

## Supported C Features

### Data Types
- `int`, `char`, `void`
- Pointers and pointer arithmetic
- Arrays (single and multi-dimensional)
- Structs with nested access

### Control Flow
- `if`/`else` statements
- `while` loops
- `do-while` loops  
- `for` loops
- Function calls

### Operators
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logical: `&&`, `||`, `!`
- Assignment: `=`, `+=`, `-=`, etc.
- Unary: `++`, `--`, `&`, `*`

### Advanced Features
- Inline assembly with `__asm()`
- Preprocessor directives (`#include`, `#define`)
- Function parameters and local variables
- Global variables and initialization
- String literals and character constants

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed contribution guidelines.

## License

NCC is designed for educational and retro computing purposes. See the individual source files for specific licensing information.

## System Requirements

- GCC or compatible C compiler (for building NCC)
- Make build system
- QEMU (optional, for testing bootloaders)
- Linux/Unix environment or Windows with MSYS2/WSL

## Getting Help

- Check the test files in `test/` for usage examples
- Use `-d` flag to debug AST generation
- Use `-dl` flag to trace preprocessor line mappings
- Examine generated assembly with `-S` flag