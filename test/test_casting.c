#define BITS16
#include "test/bootloader.h"

// Global buffers for string conversions
char hex_buffer[16];
char dec_buffer[16];

void _after_diskload() {
    clearScreen();
    writeString("NCC Bootloader\r\n");
    writeString("Loading kernel...\r\n");
    
    // Test unsigned casting
    short testValue = -5;
    writeString("Signed value: ");
    stoa_dec(testValue, dec_buffer);
    writeString(dec_buffer);
    writeString("\r\n");
    
    writeString("Unsigned cast: ");
    stoa_dec((unsigned short)testValue, dec_buffer);
    writeString(dec_buffer);
    writeString("\r\n");
    
    // Test with large numbers approaching 16-bit limits
    short largeNeg = -32000;
    writeString("Large negative: ");
    stoa_dec(largeNeg, dec_buffer);
    writeString(dec_buffer);
    writeString("\r\n");
    
    writeString("Large unsigned: ");
    stoa_dec((unsigned short)largeNeg, dec_buffer);
    writeString(dec_buffer);
    writeString("\r\n");
    
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

// Convert short to hex string
void stoa_hex(short value, char* buffer)
{
    int i = 0;
    unsigned short temp = (unsigned short)value;    if (temp == 0) {
        buffer[i++] = '0';
    } else {
        while (temp > 0) {
            unsigned short digit = temp % 16;
            if (digit < 10) {
                buffer[i++] = digit + '0';
            } else {
                // Calculate A-F hex digit
                digit = digit - 10;
                buffer[i++] = digit + 'A';
            }
            temp /= 16;
        }
    }
    
    // Add 0x prefix
    buffer[i++] = 'x';
    buffer[i++] = '0';
    
    buffer[i] = '\0';
    
    // Reverse the buffer
    for (int j = 0; j < i / 2; j++) {
        char swap = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = swap;
    }
}

// Convert short to decimal string
void stoa_dec(short value, char* buffer)
{
    int i = 0;
    int isNegative = 0;

    unsigned short temp;    if (value < 0) {
        isNegative = 1;
        // Handle negation in steps to avoid issues with complex expressions
        short negVal = -value;
        temp = (unsigned short)negVal;
    } else {
        temp = (unsigned short)value;
    }
    
    do {
        // Handle digit conversion in steps
        unsigned short digit = temp % 10;
        buffer[i++] = digit + '0';
        temp /= 10;
    } while (temp > 0);

    if (isNegative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    // Reverse the buffer
    for (int j = 0; j < i / 2; j++) {
        char swap = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = swap;
    }
}
