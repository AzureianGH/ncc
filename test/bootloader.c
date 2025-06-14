[[naked]] void _bios_called() {
    __asm("cli");
    __asm("xor ax, ax");
    __asm("mov ds, ax");
    __asm("mov es, ax");

    __asm("mov ax, 0x9000");
    __asm("mov ss, ax");
    __asm("mov sp, 0xFFFE");
    __asm("sti");
    __asm("mov ah, 0x02");   // Read sectors
    __asm("mov al, 0x10");   // Number of sectors
    __asm("mov ch, 0x00");   // Cylinder
    __asm("mov cl, 0x02");   // Sector (start at sector 2)
    __asm("mov dh, 0x00");   // Head
    __asm("mov dl, 0x00");   // Drive 0
    __asm("mov bx, 0x8000"); // Buffer address
    __asm("int 0x13");       // BIOS interrupt
    __asm("jmp 0x0000:0x8000"); // Jump to loaded kernel
}

[[naked]] void pad() {
    __asm("#times (510 - $) #db 0");
    __asm("#dw 0xAA55");
}