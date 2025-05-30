# NCC (Nathan's C Compiler)

A lightweight C compiler targeting 8086/DOS systems, designed for simplicity, educational purposes, and retro computing.

## Overview

NCC is a compact C compiler that translates a significant subset of the C language into x86 assembly for 16-bit DOS environments. It's ideal for:

- Retro computing projects
- DOS development
- Bootloader and small OS development
- Learning compiler design and implementation
- Embedded systems with limited resources

## Features

- **Simplified C Subset**: Implements essential C language features
- **8086 Assembly Output**: Generates code for 16-bit x86 processors
- **Struct Support**: Full implementation of C structures
- **Small Memory Footprint**: Designed for resource-constrained environments
- **Fast Compilation**: Streamlined compilation process
- **Readable Assembly Output**: Generated assembly is human-readable and well-commented
- **DOS/8086 Targeting**: Perfect for vintage computing or embedded projects

## Getting Started

### Building the Compiler

```bash
make
```

### Basic Usage

```bash
./bin/ncc myprogram.c -o myprogram.asm
```

### Creating Executables

After compiling to assembly, use an assembler like NASM to create executable binaries:

```bash
nasm -f bin myprogram.asm -o myprogram.com
```

### Creating Operating Systems and Bootloaders

You can make bootloaders and full 16-bit OSes with specialized options:

```bash
# Using displacement address (manual approach)
ncc -disp 0x7C00 myos.c -o myos.asm

# Using bootloader mode (recommended approach)
ncc -sys myos.c -o myos.asm

# Using bootloader mode with custom stack setup
ncc -sys -ss 0000:7C00 myos.c -o myos.asm

# Assemble the bootloader
nasm -f bin myos.asm -o myprogram.bin
```

## Documentation

For detailed information about the compiler's architecture, implementation details, and how to extend or modify it, see [DOCUMENTATION.md](DOCUMENTATION.md).

## Examples

The `test/` directory contains various example programs demonstrating the compiler's capabilities, including:

- Basic control structures
- Function definitions and calls
- Struct usage
- Array manipulation
- Simple bootloader examples

## License

MIT License is used.

## Contributing

Contributions, bug reports, and feature requests are welcome! Feel free to submit pull requests or open issues on GitHub.

### Third-Party Tools

This project includes NASM (Netwide Assembler), which is distributed under the 2-Clause BSD License. See `LICENSE.nasm.txt` for details.
