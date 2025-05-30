# NCC User Guide

This guide provides instructions for using NCC to compile C programs for 8086/DOS environments.

## Installation

### Building from Source

```bash
# Clone the repository
git clone https://github.com/AzureianGH/ncc.git
cd ncc

# Build the compiler
make
```

The compiler executable will be created in the `bin/` directory.

## Basic Usage

```bash
./bin/ncc [options] input.c -o output.asm
```

### Command Line Options

- `-o <file>`: Specify output file (default: input.asm)
- `-I <path>`: Add include path
- `-D <name>[=value]`: Define preprocessor macro
- `-v`: Verbose mode (shows compilation details)
- `-h` or `--help`: Display help information

## Compilation Process

1. Write your C code in a .c file
2. Compile it to assembly using NCC
3. Assemble the resulting .asm file with NASM or another assembler
4. Link or directly run the resulting binary

Example workflow:

```bash
# Compile to assembly
./bin/ncc program.c -o program.asm

# Assemble with NASM
nasm -f bin program.asm -o program.com

# Run in DOSBox or similar environment
dosbox program.com
```

## Supported C Features

NCC supports a subset of the C language, including:

### Basic Types

- `int`: 16-bit signed integer
- `short`: 16-bit signed integer (alias for int)
- `long`: 32-bit signed integer
- `char`: 8-bit signed integer
- `unsigned` variants of the above types
- `void`: Used for functions that don't return a value
- `bool`: C23 boolean type (1 byte)

### Operators

- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical: `&&`, `||`, `!`
- Bitwise: `&`, `|`, `^`, `~`, `<<`, `>>`
- Assignment: `=`, `+=`, `-=`, `*=`, `/=`, `%=`
- Other: `sizeof`, pointer operators (`*`, `&`), ternary (`?:`)

### Control Structures

- `if`-`else`
- `while`
- `do`-`while`
- `for`
- `return`

### Data Structures

- Arrays
- Pointers
- Structs with member access (`.` and `->`)
#### NOTE: Structs are broken right now.

### Functions

- Function definitions and calls
- Function pointers
- Parameters and return values

### Other Features

- Basic preprocessor directives
- Inline assembly with `__asm`

## Language Limitations

NCC has some limitations compared to a full C compiler:

- No floating-point support
- Limited standard library
- No complex initializers
- No unions or bitfields
- No variable-length arrays
- Limited optimization

## Example Programs

### Hello World

```c
/* hello.c */
#include <stdio.h>

int main() {
    printf("Hello, world!\n");
    return 0;
}
```

### Structure Example

```c
/* struct.c */
struct Point {
    int x;
    int y;
};

int main() {
    struct Point p;
    p.x = 10;
    p.y = 20;
    
    struct Point* pp = &p;
    pp->x = 30;
    
    return p.x + p.y;  // Returns 50
}
```

### Function Example

```c
/* function.c */
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int main() {
    return factorial(5);  // Returns 120
}
```

## DOS Programming Tips

When targeting DOS with NCC:

- Keep memory usage minimal
- Use far pointers for accessing memory outside the current segment
- Be aware of 16-bit integer limitations (values from -32768 to 32767)
- For graphics, use direct memory access or BIOS/DOS interrupts
- For file operations, use DOS interrupts or implement custom I/O

## Troubleshooting

### Common Errors

- **Syntax errors**: Check your C syntax against the supported features
- **Undefined symbols**: Ensure all functions and variables are declared before use
- **Type mismatches**: Verify type compatibility in expressions and assignments
- **Memory limitations**: DOS programs have limited memory; simplify if necessary

### Getting Help

If you encounter issues:

1. Check error messages for line numbers and descriptions
2. Examine the generated assembly for problems
3. Simplify your code to isolate the issue
4. Review documentation for supported features
5. Submit an issue on GitHub with a minimal example

## Advanced Features

### Inline Assembly

NCC supports inline assembly for direct hardware access:

```c
void setVideoMode(int mode) {
    asm {
        mov ah, 0
        mov al, [bp+4]  ; Function parameter
        int 0x10        ; BIOS video interrupt
    }
}
```

### Memory Models

For larger programs, use far pointers to access memory across segments:

```c
char far* videoMemory = (char far*)0xB8000000;  // CGA/EGA/VGA text mode memory

void writeChar(int x, int y, char c, char attr) {
    int offset = y * 80 + x;
    videoMemory[offset*2] = c;
    videoMemory[offset*2+1] = attr;
}
```

## Conclusion

NCC provides a streamlined way to create DOS programs in C. While it doesn't support every C feature, it offers enough functionality for many DOS development tasks while maintaining simplicity and efficiency.
