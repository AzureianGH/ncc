#define BITS16
#include "test/bootloader.h"

void _after_diskload() {
    clearScreen();
    writeString("NCC Bootloader\r\n");
    writeString("Loading kernel...\r\n");      
    char* a = "Meow";
    char* b = "Woof";
    dothing(a, b);
    haltForever();
}

void dothing(char* a, char* b)
{
    writeString("Doing thing...\r\n");
    writeString("a: ");
    writeString(a);
    writeString("\r\n");
    writeString("b: ");
    writeString(b);
    writeString("\r\n");
}

void assert(int condition)
{
    if (!condition) {
        writeString("Assertion failed!\r\n");
        haltForever();
    }
}

void clearScreen()
{
    __asm("mov ax, 0x0003"); // Set video mode 3 (80x25 text mode)
    __asm("int 0x10");       // BIOS interrupt to set video mode
}

[[deprecated("This function will be removed soon."), naked]]
void haltForever()
{
    __asm("cli");           // Disable interrupts
    __asm("hlt");           // Halt CPU
    __asm("jmp _haltForever"); // Infinite loop
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

void enterVBEGraphicsMode()
{
    __asm("mov ax, 0x4F02"); // VBE function to set video mode
    __asm("mov bx, 0x118");  // Mode 0x118: 1024x768, 16-bit color
    __asm("int 0x10");       // BIOS interrupt to set video mode
}
