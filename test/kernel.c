#define BITS16
#include "test/bootloader.h"
#include "test/stdarg.h"
#include "test/color.h"

[[naked]] void _KERNEL_START() {}

void _after_diskload()
{
    clearScreen();
    initUart();
    writeLog("UART initialized.\r\n", 0);
    writeLog("NCC Bootloader loaded in at (", 2);
    writeDebug(stoa_hex(seconds));
    writeDebug(":8000)\r\n");
    writeLog("Setting GDT...\r\n", 2);
    setGDT();
    writeLog("GDT set.\r\n", 0);
    writeLog("Installing INTs...\r\n", 2);
    installIRQS();
    initPIT();
    writeLog("PIT initialized.\r\n", 0);

    while (true)
    {
        writeDebug("shell > ");
        char* string = getString(true);
        writeLog("You entered: ", 2);
        writeDebug(string);
        writeDebug("\r\n"); 
    }
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
    __asm("cli");
    while (true)
    {
        __asm("hlt");
    }
}

void writeChar(char c)
{
    __asm("mov al, [bp+4]"); // Get character from stack
    __asm("mov ah, 0x0E");   // BIOS teletype function
    __asm("mov bx, 0x0007"); // Attribute (light gray on black)
    __asm("int 0x10");       // BIOS interrupt to write character
}

uint16_t getChar()
{
    __asm("mov ah, 0x00"); // BIOS keyboard read function
    __asm("int 0x16");     // BIOS interrupt to read character
    // AL = ASCII code
    // AH = Scan code
}

[[naked]] char getCharLetter()
{
    __asm("mov ah, 0x00"); // BIOS keyboard read function
    __asm("int 0x16");     // BIOS interrupt to read character
    __asm("mov bl, al");
    __asm("xor ax, ax");
    __asm("mov al, bl"); // Move ASCII code to AH for output
    __asm("ret");        // Return to caller
}

char *getString(bool newline)
{
    char buffer[256]; // Buffer for input string
    char *ptr = buffer;

    //clear buffer
    for (int i = 0; i < sizeof(buffer); i++)
    {
        buffer[i] = '\0'; // Initialize buffer to null characters
    }
    char c;

    while (true)
    {
        c = getCharLetter();
        if (c == '\r')
        {                // Enter key
            *ptr = '\0'; // Null-terminate the string
            break;
        }
        else if (c == '\b') // Backspace or Delete
        { // Backspace
            if (ptr > buffer)
            { // Only remove if there's something to remove
                ptr--; // Move pointer back
                writeChar('\b'); // Echo backspace character
                writeChar(' ');  // Overwrite with space
                writeChar('\b'); // Move back again
            }
        }
        else if (c == 0x1B) // Escape key
        {
            *ptr = '\0'; // Null-terminate the string
            break;       // Exit input loop
        }
        else
        {
            //only allow printable characters
            if (c >= 32 && c <= 126) // Printable ASCII range
            {
                *ptr++ = c; // Add character to buffer
                writeChar(c); // Echo character to screen
                if (ptr >= buffer + sizeof(buffer) - 1)
                { // Prevent buffer overflow
                    break;
                }
            }
            char* btoathing = btoa_hex(c);
            writeUartString(btoathing); // Send character to UART
        }
    }
    if (newline)
    {
        writeDebug("\r\n"); // Write newline to UART
    }
    return buffer; // Return the input string
}

void writeString(char *str)
{
    while (*str)
        writeChar(*str++);
}

void writeDebug(char *str)
{
    writeString(str);
    writeUartString(str);
}

[[naked]] void softReset()
{
    __asm("cli");
    __asm("jmp 0xFFFF:0x0000");
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
    __asm("int 0x10");     // BIOS interrupt to set video mode
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
    for (unsigned short i = 0; i < height; i++)
    {
        for (unsigned short j = 0; j < width; j++)
        {
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
    __asm("mov al, %0" : : "q"(value)); // Use the constraint system properly
    __asm("rep stosb");

    setESSegment(old_seg);
}

static char btoa_hex_buffer[3]; // 2 hex digits + null terminator

char* btoa_hex(unsigned char i)
{

    // Disable interrupts while we're creating the string
    __asm("cli");

    // Convert each nibble to a hex digit
    unsigned char digit;

    digit = (i >> 4) & 0xF;
    btoa_hex_buffer[0] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);

    digit = i & 0xF;
    btoa_hex_buffer[1] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);

    btoa_hex_buffer[2] = '\0';

    // Re-enable interrupts
    __asm("sti");

    return btoa_hex_buffer;
}

static char shex_buffer[5]; // 4 hex digits + null terminator

char *stoa_hex(unsigned short i)
{
    // Disable interrupts while we're creating the string
    __asm("cli");

    // Convert each nibble to a hex digit
    unsigned char digit;

    digit = (i >> 12) & 0xF;
    shex_buffer[0] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);

    digit = (i >> 8) & 0xF;
    shex_buffer[1] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);

    digit = (i >> 4) & 0xF;
    shex_buffer[2] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);

    digit = i & 0xF;
    shex_buffer[3] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);

    shex_buffer[4] = '\0';

    // Re-enable interrupts
    __asm("sti");

    return shex_buffer;
}

[[naked]] void __GDT()
{
    __asm("gdt_start:");

    __asm("#dw 0x0000");
    __asm("#dw 0x0000");
    __asm("#dw 0x0000");

    __asm("#dw 0xFFFF");
    __asm("#dw 0x0000");
    __asm("#dw 0x9A00");
    __asm("#dw 0xCF00");

    __asm("#dw 0xFFFF");
    __asm("#dw 0x0000");
    __asm("#dw 0x9200");
    __asm("#dw 0xCF00");

    __asm("gdt_end:");

    __asm("gdt_descriptor:");
    __asm("#dw gdt_end - gdt_start - 1");
    __asm("#dd gdt_start");
}

[[naked]] void setGDT()
{
    __asm("lgdt [gdt_descriptor]");
    __asm("ret");
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
    __asm("mov al, 0x80");  // Enable DLAB (Divisor Latch Access Bit)
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
    __asm("mov dx, 0x3F8");  // COM1 port
    __asm("mov al, [bp+4]"); // Get character from stack
    __asm("out dx, al");
}

void writeUartString(char *str)
{
    while (*str)
    {
        writeUart(*str++);
    }
}

void writeLog(char *str, unsigned char level)
{
    char *prefix = NULL;
    if (level == 0)
    {
        prefix = "[OKAY] ";
    }
    else if (level == 1)
    {
        prefix = "[FAIL] ";
    }
    else if (level == 2)
    {
        prefix = "[INFO] ";
    }
    else if (level == 3)
    {
        prefix = "[WARN] ";
    }
    else
    {
        prefix = "[UNKW] ";
    }
    if (prefix)
    {
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
    // 1193182 / 1000 = 1193
    outb(0x43, 0x34);               // Set mode 2 (rate generator)
    outb(0x40, 1193 & 0xFF);        // Set low byte
    outb(0x40, (1193 >> 8) & 0xFF); // Set high byte
    outb(0x20, 0x20);               // Send EOI to PIC
    outb(0xA0, 0x20);               // Send EOI to slave PIC
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
    __asm("mov di, 0x20");      // 4 * 0x08 = 0x20
    __asm("mov [es:di], cx");   // Offset first
    __asm("mov [es:di+2], bx"); // Segment next
    __asm("pop es");
    __asm("popa");
    __asm("sti");
}

uint16_t seconds = 0;
uint16_t tick = 0;

uint16_t getSeconds()
{
    return seconds;
}

uint16_t getTick()
{
    return tick;
}

void setTick(uint16_t t)
{
    tick = t;
}

[[naked]] void irq0_handler()
{
    __asm("cli");
    __asm("pusha");

    __asm("inc word [cs:_kernel_tick]"); // Increment tick
    // check if its 0xFFFF
    __asm("cmp word [cs:_kernel_tick], 1000");
    __asm("jne _irq0_handler_end");

    __asm("mov word [cs:_kernel_tick], 0x0000"); // Reset tick to 0
    __asm("inc word [cs:_kernel_seconds]");      // Increment seconds

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