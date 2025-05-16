// Function that performs a hardware port operation


#define __stackframe

__stackframe void write_port(int value) {
    __asm("mov dx, 0x3F8");   // Load port number
    __asm("mov al, byte [bp+4]");   // Load value
    __asm("out dx, al");            // Output to port
}

__stackframe void write_char_to_screen(int value) {
    __asm("mov ah, 0x0E");         // BIOS teletype function
    __asm("mov al, byte [bp+4]");  // Load character to print
    __asm("int 0x10");             // Call BIOS interrupt
}

int start() {
    write_port('e');
    write_char_to_screen('a');
    __asm("jmp $");
    return 0;
}
