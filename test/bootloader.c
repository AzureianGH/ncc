#include "test/bootloader.h"

__attribute__((naked)) void biosInitialize() {
    __asm("cli");           // Disable interrupts

    __asm("xor ax, ax");
    __asm("mov ss, ax");    // Set stack segment to 0x0000
    __asm("mov sp, 0x7C00");// Set stack pointer below bootloader

    __asm("sti");           // Re-enable interrupts

    __asm("jmp _main");     // Jump to main function
}

__attribute__((naked)) void main()
{
    clearScreen();
    randomString();
    writeChar('A');
    haltForever();
}

__attribute__((naked)) void haltForever()
{
    __asm("cli");           // Disable interrupts
    __asm("hlt");           // Halt CPU
    __asm("jmp _haltForever"); // Infinite loop
}

void writeChar(char c)
{
    //get al from stack
    __asm("mov al, [bp+4]"); // Get character from stack
    __asm("mov ah, 0x0E"); // BIOS teletype function
    __asm("mov bx, 0x0007"); // Attribute (light gray on black)
    __asm("int 0x10");       // BIOS interrupt to write character
}

void randomString()
{
    char* str = "Hello, World!\n";
    writeChar(str[0]);
}

void clearScreen()
{
    __asm("mov ax, 0x0003"); // Set video mode 3 (80x25 text mode)
    __asm("int 0x10");       // BIOS interrupt to set video mode
}

__attribute__((naked)) void _NCC_STRING_LOC() {}

__attribute__((naked)) void _NCC_ARRAY_LOC() {}

__attribute__((naked)) void biosPadding()
{
    __asm("times 510 - ($ - $$) db 0");
    __asm("dw 0xAA55");
}