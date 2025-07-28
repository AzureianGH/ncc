[[naked]] void _bios_called() {
    __asm("cli");
    __asm("xor ax, ax");
    __asm("mov ds, ax");
    __asm("mov es, ax");

    __asm("mov ax, 0x9000");
    __asm("mov ss, ax");
    __asm("mov sp, 0xFFFE");
    __asm("sti");

    __asm("mov ax, 0x7000");
    __asm("mov es, ax");
    __asm("xor bx, bx");       // bx = 0x0000

    __asm("mov ah, 0x02");     // Read sectors
    __asm("mov al, 0x10");     // Number of sectors
    __asm("mov ch, 0x00");     // Cylinder
    __asm("mov cl, 0x02");     // Sector (start at sector 2)
    __asm("mov dh, 0x00");     // Head
    __asm("mov dl, 0x00");     // Drive 0
    __asm("int 0x13");         // BIOS interrupt

    // set segments so they're at 0x7000
    __asm("mov ax, 0x7000");
    __asm("mov ds, ax");       // Set data segment
    __asm("mov es, ax");       // Set extra segment
    

    __asm("jmp 0x7000:0x0000"); // Jump to loaded kernel
}



[[naked]] void pad() {
    __asm("#times (510 - $) #db 0");
    __asm("#dw 0xAA55");
}