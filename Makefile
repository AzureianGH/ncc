CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g -Wno-enum-conversion
# Add a flag for quiet mode (less verbose output)
QUIET_FLAG = -DQUIET_MODE
INCLUDES = -Iinclude

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

SOURCES = src/token_debug.c $(filter-out src/token_debug.c, $(wildcard $(SRC_DIR)/*.c))
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))
TARGET = $(BIN_DIR)/ncc

# Make sure the build directories exist
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

# Default target
all: $(TARGET)

# Link the object files to create the executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean up
clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*

# Create test directory and sample test file
test:
	@echo "Test unit 0: minimal.c         1/5 20%"
	$(TARGET) test/minimal.c -o test/minimal.asm
	@echo "Test unit 1: simple_for.c      2/5 40%"
	$(TARGET) test/simple_for.c -o test/simple_for.asm
	@echo "Test unit 2: simple_ptr.c      3/5 60%"
	$(TARGET) test/simple_ptr.c -o test/simple_ptr.asm
	@echo "Test unit 3: for_test.c        4/5 80%"
	$(TARGET) test/for_test.c -o test/for_test.asm
	@echo "Test unit 4: loops_and_strings.c 5/5 100%"
	$(TARGET) test/loops_and_strings.c -o test/loops_and_strings.asm
	@echo "All unit tests successful."

# Build a quiet version of the compiler (less verbose output)
quiet:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) $(QUIET_FLAG)"

.PHONY: all clean run test quiet