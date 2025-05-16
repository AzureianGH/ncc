# NCC - A C Compiler Targeting the 8086 CPU with NASM Output

NCC is a C compiler that targets the 8086 CPU and emits NASM assembly code, suitable for creating 16-bit DOS programs or flat binaries.

## Features

- Targets Intel 8086 CPU architecture
- Generates NASM assembly code
- Supports the following data types:
  - `int` (16-bit signed integer)
  - `short` (16-bit signed integer, alias of int)
  - `unsigned int` (16-bit unsigned integer)
  - `unsigned short` (16-bit unsigned integer, alias of unsigned int)
  - `char` (8-bit signed integer)
  - `unsigned char` (8-bit unsigned integer)
- Special features:
  - `FAR` keyword for far pointers (segment:offset)
  - Inline assembly with `__asm` blocks
  - `stackframe` attribute for function stack management

## Building the Compiler

```bash
make
```

This will create the `ncc` executable in the `bin` directory.

## Using the Compiler

```bash
./bin/ncc [options] <source-file>
```

### Options

- `-o <file>` : Specify output assembly file (default: output.asm)
- `-d` : Debug mode (print AST)
- `-h` : Display help and exit

## Example

```c
// Sample program
int main() {
    int x = 10;
    int y = 20;
    int z = x + y;
    return z;
}
```

Compile with:

```bash
./bin/ncc test/sample.c -o test/sample.asm
```

Then assemble with NASM:

```bash
nasm -f bin test/sample.asm -o sample.com
```

## Special Features

### Far Pointers

```c
// Far pointer declaration (segment:offset)
int*^ screen = 0xB800:0000;

// Regular pointer
int* ptr = &x;

// Function taking a far pointer
void writeScreen(int*^ buffer, int value);
```

### Inline Assembly

```c
void interruptExample() {
    __asm {
        mov ah, 0x4C
        int 0x21
    };
}
```

### Stackframe Attribute

The `stackframe` attribute automatically sets up and tears down a function's stack frame.

```c
stackframe int factorial(int n) {
    if (n <= 1)
        return 1;
    else
        return n * factorial(n - 1);
}
```

This is equivalent to:

```c
int factorial(int n) {
    // Automatically inserted by compiler:
    // push bp
    // mov bp, sp

    if (n <= 1)
        return 1;
    else
        return n * factorial(n - 1);

    // Automatically inserted by compiler:
    // mov sp, bp
    // pop bp
    // ret
}
```

## Limitations

- No support for floating-point types
- Limited optimizations
- Limited standard library

## License

This software is provided as-is with no warranty.
