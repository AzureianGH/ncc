#include "test/bootloader.h"


[[naked]] void biosInitialize() {
    __asm("cli");
    __asm("xor ax, ax");
    __asm("mov ss, ax");
    __asm("mov sp, 0x7C00");
    __asm("sti");
    __asm("jmp _main");
}

[[naked]]
[[deprecated("This function will be removed soon.")]]
void haltForever()
{
    __asm("cli");           // Disable interrupts
    __asm("hlt");           // Halt CPU
    __asm("jmp _haltForever"); // Infinite loop
}

void main()
{
    clearScreen();
    char test[] = "Hello, World!\r\n";
    char* smaller = "Hello, Earth!\r\n";
    writeString(test); // Hello, World!
    strcpy(test, smaller);
    writeString(test); // Hello, Earth!
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
    for (int i = 0; i < strlen(str); i++) {
        writeChar(str[i]);
    }
}

void clearScreen()
{
    __asm("mov ax, 0x0003"); // Set video mode 3 (80x25 text mode)
    __asm("int 0x10");       // BIOS interrupt to set video mode
}

int strlen(char* s) {
    char* p = s;
    while (*p) p++;
    return p - s;
}


void strcpy(char* dest, char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}


[[naked]] void _NCC_STRING_LOC() {}

[[naked]] void _NCC_ARRAY_LOC() {}

[[naked]] void _NCC_GLOBAL_LOC() {}

[[naked]] void biosPadding()
{
    __asm("times 510 - ($ - $$) db 0");
    __asm("dw 0xAA55");
}