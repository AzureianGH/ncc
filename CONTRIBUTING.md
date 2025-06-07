# Contributing to NCC

Thank you for your interest in contributing to NCC (Nathan's Compiler Collection)! This document provides comprehensive guidelines for contributors who want to help improve this educational C compiler for retro computing and DOS systems.

## Getting Started

### Prerequisites

- **Build Tools**: GCC or compatible C compiler, Make
- **Testing Tools**: QEMU (optional, for bootloader testing)
- **Platform**: Linux/Unix environment or Windows with MSYS2/WSL
- **Knowledge**: Basic understanding of C, assembly language, and compiler theory

### Initial Setup

1. **Fork and Clone**
   ```bash
   git clone https://github.com/AzureianGH/ncc.git
   cd ncc
   ```

2. **Build the Compiler**
   ```bash
   make clean
   make
   ```

3. **Verify Installation**
   ```bash
   ./bin/ncc -h  # Should display help message
   ```

4. **Run Basic Tests**
   ```bash
   make test_com    # Test MS-DOS compilation
   make test_os     # Test bootloader compilation (requires QEMU)
   ```

## Project Architecture

NCC follows a traditional compiler pipeline with specialized modules:

### Core Pipeline
1. **Preprocessor** (`preprocessor.c`) - Handles `#include`, `#define`, macro expansion
2. **Lexer** (`lexer.c`) - Tokenizes source code into language tokens
3. **Parser** (`parser.c`) - Builds Abstract Syntax Tree (AST)
4. **Type Checker** (`type_checker.c`) - Validates semantic correctness
5. **Code Generator** (`codegen.c`) - Emits x86 assembly code
6. **Assembler** (NAS) - Converts assembly to machine code

### Directory Structure

```
include/           # Header files for all modules
├── ast.h         # AST node definitions
├── lexer.h       # Token definitions and lexer interface
├── parser.h      # Parser function declarations
├── codegen.h     # Code generation interface
└── ...           # Specialized module headers

src/              # Implementation files
├── main.c        # Command-line interface and driver
├── lexer.c       # Tokenization logic
├── parser.c      # Core parsing functions
├── codegen.c     # Assembly generation
├── *_codegen.c   # Specialized code generators
├── *_parser.c    # Specialized parsers
└── ...           # Support modules

test/             # Test programs and examples
├── bootloader.c  # Bootloader example
├── kernel.c      # Simple kernel example
├── testcom.c     # MS-DOS program example
└── *.h          # Test headers and utilities

bin/              # Build outputs
└── tooling/      # Integrated tools (NAS assembler)
```

### Specialized Modules

- **Struct Support**: `struct_parser.c`, `struct_codegen.c`, `struct_support.c`
- **Loop Constructs**: `while_loop.c`, `for_loop.c`, `do_while_loop.c`
- **Control Flow**: `if_statement.c`, `if_statement_codegen.c`
- **Expression Handling**: `unary_ops.c`, `compound_expressions.h`
- **Memory Management**: `array_ops.c`, `string_literals.c`
- **Debugging**: `token_debug.c`, `struct_debug.c`, `ast_cleanup.c`

## Development Workflow

### Building and Testing

**Standard Build:**
```bash
make                 # Build with standard flags
make clean && make   # Clean build
```

**Quiet Build:**
```bash
make quiet          # Build with minimal output
```

**Testing Commands:**
```bash
# Test MS-DOS .COM file generation
make test_com

# Test bootloader + kernel (requires QEMU)
make test_os

# Test with NAS assembler
make test_nas

# Debug assembly output
make dbg_nas
```

### Development Tools

**Debugging Options:**
```bash
./bin/ncc -d program.c           # Print AST
./bin/ncc -dl program.c          # Debug line mappings
./bin/ncc -S program.c           # Stop after assembly generation
```

**Compilation Targets:**
```bash
./bin/ncc -com program.c         # MS-DOS executable (ORG 0x100)
./bin/ncc -sys bootloader.c      # Bootloader (ORG 0x7C00)
./bin/ncc -disp 0x8000 kernel.c # Custom origin address
```

## Contributing Guidelines

### Types of Contributions Welcome

#### 🐛 Bug Fixes
- Compiler crashes or incorrect code generation
- Memory leaks or resource management issues
- Incorrect AST construction or type checking
- Assembly output errors

#### ✨ Feature Additions
- **Language Features**: New C constructs (switch/case, additional operators, etc.)
- **Target Support**: New compilation targets or output formats
- **Optimization**: Code generation improvements
- **Tooling**: Enhanced debugging, profiling, or analysis tools

#### 📚 Documentation
- Code comments and documentation
- Usage examples and tutorials
- Architecture explanations
- API documentation

#### 🧪 Testing
- Additional test cases for edge conditions
- Performance benchmarks
- Integration tests for new features
- Regression test suites

### Coding Standards

#### Code Style
```c
// Use consistent 4-space indentation
if (condition) {
    doSomething();
    doSomethingElse();
}

// Naming conventions
int variableName;           // camelCase for variables
void functionName();        // camelCase for functions
#define CONSTANT_NAME 42    // UPPER_CASE for constants
typedef struct TypeName;    // PascalCase for types

// Meaningful names
int tokenCount;             // Good
int tc;                     // Bad

// Comments for complex logic
// Parse expression with operator precedence
ASTNode* parseExpression(int minPrec) {
    // Implementation...
}
```

#### File Organization
- **Headers**: Declare interfaces in `include/` directory
- **Implementation**: Define functions in `src/` directory  
- **Tests**: Add test cases in `test/` directory
- **Modularity**: Create separate files for major features

#### Memory Management
```c
// Always clean up allocated memory
ASTNode* node = malloc(sizeof(ASTNode));
if (!node) {
    error("Memory allocation failed");
    return NULL;
}
// ... use node ...
free(node);

// Use the AST cleanup system
cleanupAST(rootNode);  // Automatically handles tree cleanup
```

### Commit Guidelines

#### Commit Message Format
```
<type>(<scope>): <short description>

<detailed description if needed>

<issue references>
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Build system or auxiliary tool changes

**Examples:**
```
feat(parser): add support for switch-case statements

- Added SWITCH, CASE, DEFAULT tokens to lexer
- Implemented parseSwitch() function in parser
- Added AST nodes for switch constructs
- Updated type checker for switch validation

Fixes #42

fix(codegen): correct stack alignment for function calls

The previous implementation didn't properly align the stack
for function parameters, causing issues with multi-parameter
function calls.

refactor(struct): extract struct parsing into separate module

Moved struct-related parsing functions to struct_parser.c
for better code organization and maintainability.
```

### Pull Request Process

1. **Create Feature Branch**
   ```bash
   git checkout -b feature/switch-case-support
   git checkout -b fix/memory-leak-parser
   ```

2. **Make Changes**
   - Write clean, well-documented code
   - Add appropriate test cases
   - Ensure all tests pass
   - Update documentation if needed

3. **Test Thoroughly**
   ```bash
   make clean && make        # Clean build
   make test_com            # Test MS-DOS compilation
   make test_os             # Test bootloader compilation
   ./bin/ncc -d test.c      # Test AST generation
   ```

4. **Submit Pull Request**
   - Clear title and description
   - Reference related issues
   - Include testing instructions
   - Request specific reviewers if needed

5. **Address Feedback**
   - Respond to code review comments
   - Make requested changes
   - Update tests as needed

## Adding New Features

### Language Feature Development

#### 1. Adding New Operators

**Lexer Changes** (`src/lexer.c`, `include/lexer.h`):
```c
// Add token type
typedef enum {
    // ...existing tokens...
    TOKEN_POWER,        // **
    TOKEN_SHIFT_LEFT,   // <<
} TokenType;

// Add recognition logic
if (current == '*' && peek() == '*') {
    advance(); advance();
    return createToken(TOKEN_POWER);
}
```

**Parser Changes** (`src/parser.c`, `include/ast.h`):
```c
// Add AST node type
typedef enum {
    // ...existing types...
    AST_POWER_OP,
    AST_SHIFT_LEFT_OP,
} ASTNodeType;

// Update expression parsing with precedence
ASTNode* parseExpression(int minPrec) {
    // Handle new operator with appropriate precedence
}
```

**Type Checker** (`src/type_checker.c`):
```c
void checkBinaryOp(ASTNode* node) {
    switch (node->type) {
        case AST_POWER_OP:
            // Validate operand types
            break;
    }
}
```

**Code Generator** (`src/codegen.c`):
```c
void generateBinaryOp(ASTNode* node) {
    switch (node->type) {
        case AST_POWER_OP:
            // Generate assembly for power operation
            break;
    }
}
```

#### 2. Adding New Statements

For complex statements like `switch-case`, create dedicated modules:

**Files to Create:**
- `src/switch_parser.c` - Parsing logic
- `src/switch_codegen.c` - Code generation
- `include/switch_support.h` - Interface definitions

**Integration Points:**
```c
// In parser.c
ASTNode* parseStatement() {
    switch (currentToken.type) {
        case TOKEN_SWITCH:
            return parseSwitch();  // Delegate to specialized parser
        // ...
    }
}

// In codegen.c
void generateStatement(ASTNode* node) {
    switch (node->type) {
        case AST_SWITCH:
            generateSwitch(node);  // Delegate to specialized generator
        // ...
    }
}
```

#### 3. Adding New Data Types

**Type System** (`include/ast.h`):
```c
typedef enum {
    TYPE_INT,
    TYPE_CHAR,
    TYPE_FLOAT,    // New type
    TYPE_DOUBLE,   // New type
} DataType;
```

**Parser Integration**:
```c
DataType parseType() {
    if (match(TOKEN_FLOAT)) return TYPE_FLOAT;
    if (match(TOKEN_DOUBLE)) return TYPE_DOUBLE;
    // ...
}
```

**Code Generation**:
```c
int getTypeSize(DataType type) {
    switch (type) {
        case TYPE_FLOAT: return 4;
        case TYPE_DOUBLE: return 8;
        // ...
    }
}
```

### Testing New Features

#### Create Test Cases
```c
// test/switch_test.c
void main() {
    int x = 2;
    switch (x) {
        case 1:
            printstring("One$");
            break;
        case 2:
            printstring("Two$");
            break;
        default:
            printstring("Other$");
            break;
    }
}
```

#### Add to Build System
```makefile
# In Makefile
test_switch:
	bin/ncc -com test/switch_test.c -o test/switch.com
```

## Debugging and Troubleshooting

### Common Issues

#### Compiler Crashes
1. **Enable Debug Mode**: `./bin/ncc -d program.c`
2. **Check AST Structure**: Verify AST nodes are properly formed
3. **Memory Issues**: Use valgrind or similar tools
4. **Step Through Code**: Use GDB to debug the compiler itself

#### Incorrect Code Generation
1. **Examine Assembly**: Use `-S` flag to see generated assembly
2. **Compare Expected**: Check against known-good assembly
3. **Test Incrementally**: Start with simple cases
4. **Use Disassembler**: `ndisasm -b 16 output.bin`

#### Parser Errors
1. **Token Debug**: Use token debugging functions
2. **Check Grammar**: Verify parsing logic matches C grammar
3. **Precedence Issues**: Check operator precedence tables
4. **Recovery**: Implement error recovery mechanisms

### Debug Tools

```bash
# AST visualization
./bin/ncc -d program.c

# Line mapping debug
./bin/ncc -dl program.c

# Assembly output only
./bin/ncc -S program.c

# Disassemble binary output
ndisasm -b 16 output.bin > disassembly.txt

# Test in QEMU with debugging
qemu-system-x86_64 -fda floppy.img -monitor stdio
```

### Performance Profiling

```bash
# Time compilation
time ./bin/ncc large_program.c

# Memory usage
valgrind --tool=massif ./bin/ncc program.c

# Profile hot functions
perf record ./bin/ncc program.c
perf report
```

## Code Review Guidelines

### For Contributors

- **Self-Review**: Review your own code before submitting
- **Test Coverage**: Ensure new code is tested
- **Documentation**: Update docs for user-facing changes
- **Backwards Compatibility**: Avoid breaking existing functionality

### For Reviewers

- **Focus on**: Logic correctness, performance, maintainability
- **Check**: Memory management, error handling, edge cases
- **Verify**: Tests pass, documentation is updated
- **Be Constructive**: Provide helpful feedback and suggestions

## Release Process

### Version Numbering
- **Major**: Breaking changes or significant features
- **Minor**: New features, backwards compatible
- **Patch**: Bug fixes and small improvements

### Release Checklist
- [ ] All tests pass
- [ ] Documentation updated
- [ ] Performance benchmarks run
- [ ] Example programs verified
- [ ] CHANGELOG.md updated

## Getting Help

### Communication Channels
- **Discord**: `azureian`
- **Email**: `alexmybestcat@gmail.com`
- **GitHub**: [@AzureianGH](https://github.com/AzureianGH)

### Before Asking for Help
1. Search existing issues and discussions
2. Check the documentation and examples
3. Try debugging with compiler flags (`-d`, `-dl`, `-S`)
4. Create a minimal test case that reproduces the issue

### Reporting Bugs

**Include:**
- **Environment**: OS, compiler version, build flags
- **Steps to Reproduce**: Exact commands and inputs
- **Expected vs Actual**: What should happen vs what does happen
- **Sample Code**: Minimal example that shows the problem
- **Error Messages**: Complete error output
- **Additional Context**: Relevant configuration or setup details

**Template:**
```markdown
## Bug Description
Brief description of the issue

## Steps to Reproduce
1. Compile with: `./bin/ncc -com program.c`
2. Run the output
3. Observe incorrect behavior

## Expected Behavior
Program should print "Hello World"

## Actual Behavior
Program crashes with segmentation fault

## Environment
- OS: Ubuntu 20.04
- GCC version: 9.4.0
- NCC commit: abc123

## Sample Code
```c
void main() {
    // Minimal code that reproduces the issue
}
```

## Error Output
```
Error message or compiler output
```
```

## Thank You

Thank you for contributing to NCC! Your efforts help make this educational compiler better for everyone interested in retro computing and systems programming. Every contribution, whether it's a bug fix, new feature, or documentation improvement, is valuable to the project.

Remember: NCC is designed to be an educational tool that demonstrates compiler construction principles while targeting fascinating retro computing platforms. Keep this mission in mind as you contribute, and don't hesitate to ask questions or suggest improvements to make the codebase more understandable and maintainable.

na
