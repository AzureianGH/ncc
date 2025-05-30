# Contributing to NCC

Thank you for your interest in contributing to NCC! This document provides guidelines and information for contributors.

## Getting Started

1. Fork the repository
2. Clone your fork to your local machine
3. Set up the development environment
4. Build the compiler with `make`
5. Run tests to ensure everything is working correctly

## Development Workflow

### Building the Project

```bash
make
```

For a clean build:

```bash
make clean
make
```

### Running Tests

```bash
# Run a simple test
./bin/ncc test/simple.c -o test.asm
nasm -f bin test.asm -o test.com
```

### Code Organization

NCC is organized into several components:

- `include/`: Header files
- `src/`: Source files
- `test/`: Test files
- `bin/`: Build output directory
- `obj/`: Object files directory

Key source files:

- `lexer.c`: Tokenization
- `parser.c`: Syntax analysis
- `type_checker.c`: Semantic analysis
- `codegen.c`: Code generation
- Specialized modules for specific language features

## Contribution Guidelines

### Types of Contributions

We welcome various types of contributions:

- **Bug fixes**: Fixes for issues in the compiler
- **Feature additions**: New C language features or compiler options
- **Optimizations**: Improvements to compiler performance or output code
- **Documentation**: Enhancements to project documentation
- **Tests**: Additional test cases to ensure compiler reliability

### Coding Standards

- Use consistent indentation (4 spaces)
- Follow existing naming conventions:
  - `camelCase` for variables and functions
  - `UPPER_CASE` for constants and macros
  - Meaningful, descriptive names
- Add comments for complex logic
- Modularize code for readability and maintainability
- Minimize global state where possible

### Commit Messages

- Use clear, descriptive commit messages
- Start with a short summary line
- Provide more details in the body if necessary
- Reference issue numbers when applicable

Example:

```
Add support for switch-case statements

- Added lexer tokens for switch, case, default
- Implemented parser functions for switch statements
- Added AST nodes for switch and case blocks
- Implemented code generation for switch constructs

Fixes #42
```

### Pull Request Process

1. Create a new branch for your feature or fix
2. Make your changes with appropriate tests
3. Ensure all tests pass
4. Update documentation if necessary
5. Submit a pull request with a clear description
6. Address any feedback from code reviews

## Architecture Overview

Understanding the compiler architecture will help you contribute effectively:

1. **Lexical Analysis**: Converts source code to tokens
2. **Parsing**: Builds an Abstract Syntax Tree (AST)
3. **Type Checking**: Validates semantic correctness
4. **Code Generation**: Produces assembly output

For more details, see the [Architecture Guide](ARCHITECTURE.md).

## Adding New Features

When adding a new language feature:

1. **Lexer**: Add any new tokens to `include/lexer.h` and implement recognition in `src/lexer.c`
2. **Parser**: Add AST node types to `include/ast.h` and implement parsing functions in `src/parser.c`
3. **Type Checker**: Add type checking logic to `src/type_checker.c`
4. **Code Generator**: Implement code generation in `src/codegen.c`
5. **Tests**: Create test cases in the `test/` directory

For complex features, consider creating dedicated modules (like we did for structs with `struct_parser.c`, `struct_support.c`, etc.).

## Debugging Tips

- Use the `-v` flag for verbose output
- Examine the generated AST during parsing
- Check the assembly output for code generation issues
- Use small test cases to isolate problems

## Common Tasks

### Adding a New Operator

1. Add a token type in `include/lexer.h`
2. Add recognition in `src/lexer.c`
3. Add an operator type in `include/ast.h`
4. Update expression parsing in `src/parser.c`
5. Add type checking rules in `src/type_checker.c`
6. Implement code generation in `src/codegen.c`

### Adding a New Type

1. Add a data type enum in `include/ast.h`
2. Update `parseType()` in `src/parser.c`
3. Update type compatibility checks in `src/type_checker.c`
4. Add size information and code generation in `src/codegen.c`

## Reporting Bugs

When reporting bugs, please include:

1. A clear description of the issue
2. Steps to reproduce the problem
3. Expected vs. actual behavior
4. Compiler version or commit hash
5. Sample code that demonstrates the issue
6. Any relevant error messages

## Contact

Discord: `azureian`
Email: `alexmybestcat@gmail.com`

## Thank You

Your contributions help make NCC better! We appreciate your time and effort in improving this project.
