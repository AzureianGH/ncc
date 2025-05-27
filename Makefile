CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g -Wno-enum-conversion
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
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/* test/*.bin test/*.asm test/floppy.img test/floppy.iso iso_root

quiet:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) $(QUIET_FLAG)"

# Compile bootloader and kernel
build_bootloader:
	bin/ncc -disp 0x7C00 -Itest -O1 test/bootloader.c -o test/bootloader.asm
	nasm -f bin test/bootloader.asm -o test/bootloader.bin

build_kernel:
	bin/ncc -disp 0x8000 -Itest -O1 test/kernel.c -o test/kernel.asm
	nasm -f bin test/kernel.asm -o test/kernel.bin

# Create floppy image using bootloader as MBR and kernel as second sector
floppy: build_bootloader build_kernel
	dd if=/dev/zero of=test/floppy.img bs=512 count=2880
	dd if=test/bootloader.bin of=test/floppy.img conv=notrunc
	dd if=test/kernel.bin of=test/floppy.img bs=512 seek=1 conv=notrunc
	qemu-system-x86_64 -fda test/floppy.img -boot a -m 512

# Create ISO using bootloader as El Torito image, and kernel as a file on disk
iso: build_bootloader build_kernel
	mkdir -p iso_root/boot
	cp test/bootloader.bin iso_root/boot/bootloader.bin
	cp test/kernel.bin iso_root/boot/kernel.bin

	dd if=test/bootloader.bin of=test/bootloader_512.bin bs=512 count=1 conv=notrunc

	xor/xorriso.exe -as mkisofs \
		-o test/floppy.iso \
		-b boot/bootloader.bin \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		iso_root

	qemu-system-x86_64 -cdrom test/floppy.iso -boot d -m 512

.PHONY: all clean quiet build_bootloader build_kernel floppy iso
