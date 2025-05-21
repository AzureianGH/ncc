#define BITS16
#include "test/bootloader.h"
#include "test/stdarg.h"

void _after_diskload() {
    clearScreen();
    writeString("NCC Bootloader\r\n");
    writeString("Loading kernel...\r\n");
    writeString("Integer to string: ");
    writeString(stoa_hex(0x1234));

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

void enterVBEGraphicsMode()
{
    __asm("mov ax, 0x4F02"); // VBE function to set video mode
    __asm("mov bx, 0x118");  // Mode 0x118: 1024x768, 16-bit color
    __asm("int 0x10");       // BIOS interrupt to set video mode
}

int strlen(char* str)
{
    int len = 0;
    while (str && str[len]) len++;
    return len;
}

char* stoa(short i) {
    static char buffer[20];
    int j = 0;
    if (i < 0) {
        buffer[j++] = '-';
        i = -i;
    }
    if (i == 0) {
        buffer[j++] = '0';
    } else {
        int k = i;
        while (k > 0) {
            k /= 10;
            j++;
        }
        buffer[j] = '\0';
        while (i > 0) {
            buffer[--j] = i % 10 + '0';
            i /= 10;
        }
    }
    return buffer;
}
// int == 2 bytes
char* stoa_hex(short i)
{

    char buffer[7];
    // rudimentary hex conversion per nibble
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[6] = '\0';
    buffer[5] = (i & 0x0F) + '0';
    if (buffer[5] > '9') buffer[5] += 7; // convert to hex char
    buffer[4] = ((i >> 4) & 0x0F) + '0';
    if (buffer[4] > '9') buffer[4] += 7; // convert to hex char
    buffer[3] = ((i >> 8) & 0x0F) + '0';
    if (buffer[3] > '9') buffer[3] += 7; // convert to hex char
    buffer[2] = ((i >> 12) & 0x0F) + '0';
    if (buffer[2] > '9') buffer[2] += 7; // convert to hex char
    return buffer;
}
