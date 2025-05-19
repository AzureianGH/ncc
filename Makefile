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
test_os:
	bin/ncc -disp 0x7C00 -I.\test .\test\bootloader.c -o .\test\bootloader.asm
	nasm -f bin .\test\bootloader.asm -o .\test\bootloader.bin

	bin/ncc -disp 0x8000 -I.\test .\test\kernel.c -o .\test\kernel.asm
	nasm -f bin .\test\kernel.asm -o .\test\kernel.bin

	dd if=/dev/zero of=test\floppy.img bs=512 count=2880
	dd if=test\bootloader.bin of=test\floppy.img conv=notrunc
	dd if=test\kernel.bin of=test\floppy.img bs=512 seek=1 conv=notrunc

	qemu-system-x86_64 -fda test\floppy.img

# Build a quiet version of the compiler (less verbose output)
quiet:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) $(QUIET_FLAG)"

.PHONY: all clean run test quiet