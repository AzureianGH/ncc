#define BITS16
#include "test/bootloader.h"
#include "test/stdarg.h"
#include "test/color.h"

[[naked]] void _KERNEL_START() {}

void _after_diskload() {
    clearScreen();
    initUart();
    writeLog("UART initialized.\r\n", 0);
    writeLog("NCC Bootloader loaded in at (", 2);
    writeDebug(stoa_hex(getCS()));
    writeDebug(":8000)\r\n");
    writeLog("Installing INTs...\r\n", 2);
    installIRQS();
    initPIT();
    writeLog("PIT initialized.\r\n", 0);
    
    
    haltForever();
}

void clearScreen()
{
    __asm("mov ax, 0x0003"); // Set video mode 3 (80x25 text mode)
    __asm("int 0x10");       // BIOS interrupt to set video mode
}

[[deprecated("This function will be removed soon."), naked]]
void haltForever()
{
    writeLog("Halt forever called.\r\n", 0);
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
    __asm("mov [es:di], %0" : : "q"(byte)); // becomes mov al, byte; mov [es:di], al
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
    memset_far(0xA000, 0, c, 64000);
}

void drawPixel(unsigned short x, unsigned short y, unsigned char color)
{
    writeFarPtr(0xA000, y * 320 + x, color);
}

void drawRect(unsigned short x, unsigned short y, unsigned short width, unsigned short height, unsigned char color)
{
    for (unsigned short i = 0; i < height; i++) {
        for (unsigned short j = 0; j < width; j++) {
            drawPixel(x + j, y + i, color);
        }
    }
}

void memset_far(unsigned short seg, unsigned short offset, unsigned char value, unsigned short size)
{
    uint16_t old_seg = getESSegment();
    setESSegment(seg);

    __asm("mov di, %0" : : "r"(offset));
    __asm("mov cx, %0" : : "r"(size));
    __asm("xor ax, ax");
    __asm("mov al, %0" : : "q"(value));  // Use the constraint system properly
    __asm("rep stosb");

    setESSegment(old_seg);
}

// Thread-safe version of stoa_hex that works with interrupts
char* stoa_hex(unsigned short i)
{
    // Use per-call buffer instead of static to avoid interrupt corruption
    // Each call gets its own buffer space from the stack
    char buffer[5]; // 4 hex digits + null terminator
    
    // Disable interrupts while we're creating the string
    __asm("cli");
    
    // Convert each nibble to a hex digit using direct computation instead of indexing
    unsigned char digit;
    
    // First digit (most significant nibble)
    digit = (i >> 12) & 0xF;
    buffer[0] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
    
    // Second digit
    digit = (i >> 8) & 0xF;
    buffer[1] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
    
    // Third digit
    digit = (i >> 4) & 0xF;
    buffer[2] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
    
    // Fourth digit (least significant nibble)
    digit = i & 0xF;
    buffer[3] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
    
    buffer[4] = '\0'; // Null-terminate the string
    
    // We need to copy the buffer to a static location to return it
    static char return_buffer[5];
    return_buffer[0] = buffer[0];
    return_buffer[1] = buffer[1];
    return_buffer[2] = buffer[2];
    return_buffer[3] = buffer[3];
    return_buffer[4] = buffer[4];
    
    // Re-enable interrupts
    __asm("sti");
    
    return return_buffer;
}

char* stoa_dec(unsigned short i)
{
    // Use per-call buffer instead of static to avoid interrupt corruption
    // Each call gets its own buffer space from the stack
    char buffer[6]; // 5 digits + null terminator
    
    // Disable interrupts while we're creating the string
    __asm("cli");
    
    // Convert each digit to a decimal character using direct computation instead of indexing
    unsigned char digit;
    
    // First digit (most significant digit)
    digit = (i / 10000) % 10;
    buffer[0] = '0' + digit;
    
    // Second digit
    digit = (i / 1000) % 10;
    buffer[1] = '0' + digit;
    
    // Third digit
    digit = (i / 100) % 10;
    buffer[2] = '0' + digit;
    
    // Fourth digit
    digit = (i / 10) % 10;
    buffer[3] = '0' + digit;
    
    // Fifth digit (least significant digit)
    digit = i % 10;
    buffer[4] = '0' + digit;
    
    buffer[5] = '\0'; // Null-terminate the string
    
    // We need to copy the buffer to a static location to return it
    static char return_buffer[6];
    return_buffer[0] = buffer[0];
    return_buffer[1] = buffer[1];
    return_buffer[2] = buffer[2];
    return_buffer[3] = buffer[3];
    return_buffer[4] = buffer[4];
    return_buffer[5] = buffer[5];
    
    // Re-enable interrupts
    __asm("sti");
    
    return return_buffer;
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

uint16_t getCS()
{
    uint16_t cs;
    __asm("mov ax, cs");
    __asm("mov %0, ax" : : "=r"(cs));
    return cs;
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

void outb(unsigned short port, unsigned char value)
{
    __asm("mov dx, %0" : : "r"(port));
    __asm("xor ax, ax");
    __asm("mov al, %0" : : "q"(value));
    __asm("out dx, al");
}

unsigned char inb(unsigned short port)
{
    unsigned char value;
    __asm("mov dx, %0" : : "r"(port));
    __asm("in al, dx");
    __asm("mov %0, al" : : "=q"(value));
    return value;
}


void initPIT()
{
    __asm("cli");
    //1193182 / 1000 = 1193
    outb(0x43, 0x34); // Set mode 2 (rate generator)
    outb(0x40, 1193 & 0xFF); // Set low byte
    outb(0x40, (1193 >> 8) & 0xFF); // Set high byte
    outb(0x20, 0x20); // Send EOI to PIC
    outb(0xA0, 0x20); // Send EOI to slave PIC
    __asm("sti");
}

uint16_t sizeOfKernel()
{
    uint16_t size = 0;
    __asm("mov ax, __KERNEL_START");
    __asm("mov bx, __KERNEL_END");
    __asm("sub bx, ax");
    __asm("mov %0, bx" : : "=r"(size));
    return size;
}

/// IRQ HANDLERS
/// ############
/// ############
/// ############
/// ############
/// ############

void installIRQS()
{
    __asm("cli");
    __asm("pusha");
    __asm("mov bx, cs");            // Segment
    __asm("mov cx, _irq0_handler"); // Offset
    __asm("push es");
    __asm("mov ax, 0x0000");
    __asm("mov es, ax");
    __asm("mov di, 0x20");       // 4 * 0x08 = 0x20
    __asm("mov [es:di], cx");      // Offset first
    __asm("mov [es:di+2], bx");    // Segment next
    __asm("pop es");
    __asm("popa");
    __asm("sti");
}

uint16_t seconds = 0;
uint16_t tick = 0;

uint16_t getTick()
{
    return tick;
}

void setTick(uint16_t t)
{
    tick = t;
}


[[naked]] void irq0_handler() {
    __asm("cli");
    __asm("pusha");

    __asm("inc word [cs:_kernel_tick]"); // Increment tick
    // check if its 0xFFFF
    __asm("cmp word [cs:_kernel_tick], 1000");
    __asm("jne _irq0_handler_end");

    __asm("mov word [cs:_kernel_tick], 0x0000"); // Reset tick to 0
    __asm("inc word [cs:_kernel_seconds]"); // Increment seconds

    __asm("_irq0_handler_end:");


    __asm("popa");
    __asm("mov al, 0x20");
    __asm("out 0x20, al");
    __asm("out 0xA0, al");
    __asm("sti");
    __asm("iret");
}
[[naked]] void _NCC_GLOBAL_LOC() {}
[[naked]] void _NCC_ARRAY_LOC() {}
[[naked]] void _NCC_STRING_LOC() {}
[[naked]] void _KERNEL_END() {}