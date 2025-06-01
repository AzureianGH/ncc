CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g -Wno-enum-conversion -Wno-unused-but-set-variable -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter
QUIET_FLAG = -DQUIET_MODE
INCLUDES = -Iinclude

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

SOURCES = src/token_debug.c $(filter-out src/token_debug.c, $(wildcard $(SRC_DIR)/*.c))
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))
TARGET = $(BIN_DIR)/ncc

# Ensure necessary directories exist
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

# Default target
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/ncc.exe test/*.bin test/*.asm test/floppy.img test/floppy.iso iso_root

quiet:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) $(QUIET_FLAG)"

# Create floppy image using bootloader as MBR and kernel as second sector
test_os:
	bin/ncc -sys -ss 0x9000:0xFFFE -I.\test .\test\bootloader.c -o .\test\bootloader.bin
	bin/ncc -disp 0x8000 -I.\test .\test\kernel.c -o .\test\kernel.bin
	dd if=/dev/zero of=test\floppy.img bs=512 count=2880
	dd if=test\bootloader.bin of=test\floppy.img conv=notrunc
	dd if=test\kernel.bin of=test\floppy.img bs=512 seek=1 conv=notrunc
	qemu-system-x86_64 -fda test\floppy.img

.PHONY: all clean quiet build_bootloader build_kernel test_os
