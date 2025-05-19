#include "test/bootloader.h"

[[naked]] void biosInitialize() {
    __asm("cli");
    __asm("xor ax, ax");
    __asm("mov ss, ax");
    __asm("mov sp, 0x7C00");
    __asm("sti");
    __asm("jmp _main");
}

[[deprecated("This function will be removed soon."), naked]]
void haltForever()
{
    __asm("cli");           // Disable interrupts
    __asm("hlt");           // Halt CPU
    __asm("jmp _haltForever"); // Infinite loop
}

void main()
{
    clearScreen();
    writeString(itoa(1234));
    haltForever();
}

// int == 2 bytes wide
char* itoa(int i) {
    static char buffer[20];
    int j = 0;
    int isNegative = (i < 0);
    i = isNegative ? -i : i;
    int k = i;
    do {
        k /= 10;
        j++;
    } while (k > 0);
    buffer[j] = '\0';
    do {
        buffer[--j] = (i % 10) + '0';
        i /= 10;
    } while (i > 0);
    return isNegative ? (buffer[--j] = '-', buffer + j) : buffer;
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