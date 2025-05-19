#include "test/bootloader.h"

void _after_diskload() {
    clearScreen();
    enterVGAGraphicsMode();
    
    clearGraphics(0x01); // Clear screen with a color (0x01 = blue)
    haltForever();
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

void enterVGAGraphicsMode()
{
    __asm("mov ax, 0x0013"); // Set video mode 0x13 (320x200, 256 colors)
    __asm("int 0x10");       // BIOS interrupt to set video mode
}

void clearGraphics(unsigned char color)
{
    __asm("mov ax, 0xA000"); // Segment for VGA graphics
    __asm("mov es, ax");     // Set ES to VGA segment
    __asm("xor di, di");     // Start at offset 0
    __asm("mov cx, byte %0" : : "r"(color));  // Load color into AL
    __asm("mov ax, cx");
    __asm("mov cx, 64000");  // Number of pixels (320x200)
    __asm("rep stosb");      // Fill with color
}