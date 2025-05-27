// Pure asm lines in C naked function, no external data

[[naked]] void _bios_called() {
    __asm("cli");
    __asm("xor ax, ax");
    __asm("mov ds, ax");
    __asm("mov es, ax");

    __asm("mov ax, 0x9000");
    __asm("mov ss, ax");
    __asm("mov sp, 0xFFFF");
    __asm("sti");

    // Save DL to memory at 0x0500:0x0000 offset 0 for drive_number
    __asm("mov ax, 0x0050");
    __asm("mov ds, ax");
    __asm("mov byte [0x0000], dl");  // drive_number

    // Prepare Disk Address Packet at 0x0050:0x0001 (offset 1)
    __asm("mov byte [0x0001], 0x10");     // size = 16
    __asm("mov byte [0x0002], 0");        // reserved
    __asm("mov word [0x0003], 6");        // sectors to read = 6
    __asm("mov word [0x0005], 0x8000");   // offset load addr
    __asm("mov word [0x0007], 0x0000");   // segment load addr
    __asm("mov dword [0x0009], 1");       // LBA low dword = 1
    __asm("mov dword [0x000D], 0");       // LBA high dword = 0

    // Setup registers for INT 13h AH=42h call
    __asm("mov si, 0x0001");  // point SI to offset 1 (DAP)
    __asm("mov dl, byte [0x0000]"); // load drive_number back to DL
    __asm("mov ah, 0x42");
    __asm("int 0x13");

    // Jump to loaded kernel
    __asm("jmp 0x0000:0x8000");

    // Pad boot sector to 510 bytes then 0xAA55 signature
    __asm("times 510 - ($ - $$) db 0");
    __asm("dw 0xAA55");
}
