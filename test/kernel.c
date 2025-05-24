#define BITS16
#include "test/bootloader.h"
#include "test/stdarg.h"
#include "test/color.h"

void _after_diskload() {
    clearScreen();
    initUart();
    writeLog("UART initialized.\r\n", 0);
    writeLog("NCC Bootloader\r\n", 2);
    writeLog("Enabling VGA...\r\n", 2);
    enterVGAGraphicsMode();
    clearVGAScreen(VGA_COLOR_WHITE);
    
}

void assert(int condition)
{
    if (!condition) {
        writeString("Assertion failed!\r\n");
        haltForever();
    }
}

void vocalAssert(int condition)
{
    if (!condition) {
        writeString("Assertion failed!\r\n");
        haltForever();
    }
    else {
        writeString("Assertion passed!\r\n");
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

void writeDebug(char* str)
{
    writeString(str);
    writeUartString(str);
}

void writeFarPtr(unsigned short seg, unsigned short offset, unsigned char byte)
{
    __asm("push es");
    __asm("mov es, %0" : : "r"(seg));
    __asm("mov di, %0" : : "r"(offset));
    __asm("xor ax, ax");
    __asm("mov [es:di], %0" : : "q"(byte));
    __asm("pop es");
}

unsigned char readFarPtr(unsigned short seg, unsigned short offset)
{
    unsigned char byte;
    __asm("push es");
    __asm("mov es, %0" : : "r"(seg));
    __asm("mov di, %0" : : "r"(offset));
    __asm("xor ax, ax");
    __asm("mov %0, [es:di]" : : "=q"(byte));
    __asm("pop es");
    return byte;
}

void writePtr(unsigned short ptr, unsigned char byte)
{
    __asm("mov di, %0" : : "r"(ptr));
    __asm("xor ax, ax");
    __asm("mov [es:di], %0" : : "q"(byte));
}

unsigned char readPtr(char ptr)
{
    unsigned char byte;
    __asm("mov di, %0" : : "r"(ptr));
    __asm("xor ax, ax");
    __asm("mov %0, [es:di]" : : "=q"(byte));
    return byte;
}

void enterVGAGraphicsMode()
{
    __asm("mov ax, 0x13"); // Set video mode 13h (320x200, 256 colors)
    __asm("int 0x10");       // BIOS interrupt to set video mode
}

void clearVGAScreen(char c)
{
    memset_far(0xA000, 0, c, 32000);
}

void drawPixel(unsigned short x, unsigned short y, unsigned char color)
{
    writeFarPtr(0xA000, y * 320 + x, color);
}

void memset_near(unsigned short dest, unsigned char value, unsigned int size)
{
    unsigned short* d = dest;
    for (unsigned short i = 0; i < size; i++) {
        d[i] = value;
    }
}

void memset_far(unsigned short seg, unsigned short offset, unsigned char value, unsigned int size)
{
    uint16_t old_seg = getESSegment();
    setESSegment(seg);

    // Ensure size does not exceed VGA memory limit (64 KB)
    if (offset + size > 0xFFFF) {
        size = 0xFFFF - offset + 1;
    }

    for (unsigned int i = 0; i < size; i++) {
        writeFarPtr(seg, offset + i, value);
    }

    setESSegment(old_seg);
}

int strlen(char* str)
{
    int len = 0;
    while (str && str[len]) len++;
    return len;
}

// int == 2 bytes
char* stoa_hex(unsigned i)
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

void setESSegment(unsigned int es)
{
    __asm("mov es, %0" : : "r"(es));
}

unsigned int getESSegment()
{
    unsigned int es;
    __asm("mov ax, es");
    __asm("mov %0, ax" : : "=r"(es));
    return es;
}

void initUart()
{
    __asm("mov dx, 0x3F8"); // COM1 port
    __asm("mov al, 0x80"); // Enable DLAB (Divisor Latch Access Bit)
    __asm("out dx, al");
    __asm("mov al, 0x03"); // Set baud rate to 9600 (divisor = 3)
    __asm("out dx, al");
    __asm("mov al, 0x03"); // 8 bits, no parity, 1 stop bit
    __asm("out dx, al");
    __asm("mov al, 0x01"); // Enable interrupts
    __asm("out dx, al");

    writeUartString("\r"); // clear junk
}

void writeUart(char c)
{
    __asm("mov dx, 0x3F8"); // COM1 port
    __asm("mov al, [bp+4]"); // Get character from stack
    __asm("out dx, al");
}

void writeUartString(char* str)
{
    while (*str) {
        writeUart(*str++);
    }
}

void writeLog(char* str, unsigned char level)
{
    char* prefix = NULL;
    if (level == 0) {
        prefix = "[OKAY] ";
    } else if (level == 1) {
        prefix = "[FAIL] ";
    } else if (level == 2) {
        prefix = "[INFO] ";
    } else if (level == 3) {
        prefix = "[WARN] ";
    } else {
        prefix = "[UNKW] ";
    }
    if (prefix) {
        writeString(prefix);
        writeUartString(prefix);
    }
    writeString(str);
    writeUartString(str);
}