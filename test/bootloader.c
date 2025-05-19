#include "test/bootloader.h"


[[naked]] void biosInitialize() {
    __asm("cli");
    __asm("xor ax, ax");
    __asm("mov ss, ax");
    __asm("mov sp, 0x7C00");
    __asm("sti");
    __asm("jmp _biosLoadDisk");
}

[[naked]] void biosLoadDisk()
{
    clearScreen();
    prebios_writeString("Loading kernel...\r\n");

    // Set up registers for BIOS interrupt 0x13
    __asm("mov ah, 0x02"); // Function: Read sectors
    __asm("mov al, 0x01"); // Number of sectors to read
    __asm("mov ch, 0x00"); // Cylinder number
    __asm("mov cl, 0x02"); // Sector number (starting from 1)
    __asm("mov dh, 0x00"); // Head number
    __asm("mov dl, 0x00"); // Drive number (0x00 for floppy)
    __asm("mov bx, 0x8000"); // Buffer address (0x8000:0000)
    __asm("int 0x13");       // BIOS interrupt to read disk

    // Check for errors
    __asm("jc disk_error"); // Jump if carry flag is set (error)

    // Jump to the loaded kernel
    __asm("jmp 0x8000");

    __asm("disk_error:");
    prebios_writeString("Disk read error!\r\n");
    __asm("cli");
    __asm("hlt");           // Halt CPU
}

void prebios_writeChar(char c)
{
    __asm("mov al, [bp+4]"); // Get character from stack
    __asm("mov ah, 0x0E"); // BIOS teletype function
    __asm("mov bx, 0x0007"); // Attribute (light gray on black)
    __asm("int 0x10");       // BIOS interrupt to write character
}

void prebios_writeString(char* str)
{
    while (*str) prebios_writeChar(*str++);
}

void clearScreen()
{
    __asm("mov ax, 0x0003"); // Set video mode 3 (80x25 text mode)
    __asm("int 0x10");       // BIOS interrupt to set video mode
}


[[naked]] void _NCC_STRING_LOC() {}

[[naked]] void _NCC_ARRAY_LOC() {}

[[naked]] void _NCC_GLOBAL_LOC() {}

[[naked]] void biosPadding()
{
    __asm("times 510 - ($ - $$) db 0");
    __asm("dw 0xAA55");
}

#define __NCC_REDEFINE_LOCALS

void biosLoadedEntry()
{
    writeString("Kernel loaded successfully!\r\n");
    haltForever();
}


void writeChar(char c)
{
    __asm("mov al, [bp+4]"); // Get character from stack
    __asm("mov ah, 0x0E"); // BIOS teletype function
    __asm("mov bx, 0x0007"); // Attribute (light gray on black)
    __asm("int 0x10");       // BIOS interrupt to write character
}

void writeString(char* str)
{
    while (*str) writeChar(*str++);
}

[[deprecated("This function will be removed soon."), naked]]
void haltForever()
{
    __asm("cli");           // Disable interrupts
    __asm("hlt");           // Halt CPU
    __asm("jmp _haltForever"); // Infinite loop
}

char* itoa_hex(int i) {
    char buffer[20];
    int j = 0;
    int isNegative = (i < 0);
    if (isNegative) {
        buffer[j++] = '-';
        i = -i;
    }
    int k = i;
    do {
        k /= 16;
        j++;
    } while (k > 0);
    buffer[j] = '\0';
    do {
        int digit = i % 16;
        buffer[--j] = (digit < 10) ? (digit + '0') : (digit - 10 + 'A');
        i /= 16;
    } while (i > 0);
    return isNegative ? buffer : buffer + (buffer[0] == '-');
}

[[naked]] void _NCC_STRING_LOC() {}

[[naked]] void _NCC_ARRAY_LOC() {}

[[naked]] void _NCC_GLOBAL_LOC() {}