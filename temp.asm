#width 16
#origin 0x0
_KERNEL_START:
_func_epilogue_0:
_after_diskload:
    push bp
    mov bp, sp
    call _clearScreen
    call _initUart
    mov ax, 0
    push ax
    mov ax, _str_0
    push ax
    call _writeLognl
    add sp, 4
    mov ax, 2
    push ax
    mov ax, _str_1
    push ax
    call _writeLog
    add sp, 4
    call _getCS
    push ax
    call _stoa_hex
    add sp, 2
    push ax
    call _writeDebug
    add sp, 2
    mov ax, _str_2
    push ax
    call _writeDebug
    add sp, 2
    mov ax, 2
    push ax
    mov ax, _str_3
    push ax
    call _writeLognl
    add sp, 4
    call _setGDT
    mov ax, 0
    push ax
    mov ax, _str_4
    push ax
    call _writeLognl
    add sp, 4
    mov ax, 2
    push ax
    mov ax, _str_5
    push ax
    call _writeLognl
    add sp, 4
    call _installIRQS
    mov ax, 0
    push ax
    mov ax, _str_6
    push ax
    call _writeLognl
    add sp, 4
    call _initPIT
    mov ax, 0
    push ax
    mov ax, _str_7
    push ax
    call _writeLognl
    add sp, 4
    mov ax, 2
    push ax
    mov ax, _str_8
    push ax
    call _writeLognl
    add sp, 4
    call _shell
    call _haltForever
_func_epilogue_1:
    mov sp, bp
    pop bp
    ret
_shell:
    push bp
    mov bp, sp
    mov ax, 2
    push ax
    mov ax, _str_9
    push ax
    call _writeLognl
    add sp, 4
    mov ax, 2
    push ax
    mov ax, _str_10
    push ax
    call _writeLognl
    add sp, 4
_loop_3:
    mov ax, 1
    test ax, ax
    jz _end_loop_3
    mov ax, _str_11
    push ax
    call _writeDebug
    add sp, 2
    ; Variable declaration: command
    mov ax, 1
    push ax
    call _getString
    add sp, 2
    mov ax, ax
    mov ax, _str_12
    push ax
    ; Load variable: command
    push ax
    call _strcmp
    add sp, 4
    mov bx, ax
    mov ax, 0
    mov cx, ax
    cmp bx, cx
    sete al
    xor ah, ah
    test ax, ax
    jz _else_4
    mov ax, _str_13
    push ax
    call _writeDebugnl
    add sp, 2
    mov ax, _str_14
    push ax
    call _writeDebugnl
    add sp, 2
    mov ax, _str_15
    push ax
    call _writeDebugnl
    add sp, 2
    mov ax, _str_16
    push ax
    call _writeDebugnl
    add sp, 2
    mov ax, _str_17
    push ax
    call _writeDebugnl
    add sp, 2
    jmp _end_if_4
_else_4:
    mov ax, _str_18
    push ax
    ; Load variable: command
    push ax
    call _strcmp
    add sp, 4
    mov bx, ax
    mov ax, 0
    mov cx, ax
    cmp bx, cx
    sete al
    xor ah, ah
    test ax, ax
    jz _else_5
    call _clearScreen
    jmp _end_if_5
_else_5:
    mov ax, _str_19
    push ax
    ; Load variable: command
    push ax
    call _strcmp
    add sp, 4
    mov bx, ax
    mov ax, 0
    mov cx, ax
    cmp bx, cx
    sete al
    xor ah, ah
    test ax, ax
    jz _else_6
    mov ax, 2
    push ax
    mov ax, _str_20
    push ax
    call _writeLognl
    add sp, 4
    ; Break statement - would need loop context
    jmp _end_if_6
_else_6:
    ; Load variable: command
    mov bx, ax
    mov ax, 0
    imul ax, 4
    add bx, ax
    mov ax, [bx]
    mov bx, ax
    mov ax, 101
    mov cx, ax
    cmp bx, cx
    sete al
    xor ah, ah
    mov bx, ax
    cmp bx, 0
    je .L_logic_short_8
    ; Load variable: command
    mov cx, ax
    mov ax, 1
    imul ax, 4
    add cx, ax
    mov ax, [cx]
    mov cx, ax
    mov ax, 99
    mov dx, ax
    cmp cx, dx
    sete al
    xor ah, ah
    cmp ax, 0
    setne ax
    jmp .L_logic_end_8
.L_logic_short_8:
    xor ax, ax
.L_logic_end_8:
    xor ah, ah
    mov bx, ax
    cmp bx, 0
    je .L_logic_short_9
    ; Load variable: command
    mov cx, ax
    mov ax, 2
    imul ax, 4
    add cx, ax
    mov ax, [cx]
    mov cx, ax
    mov ax, 104
    mov dx, ax
    cmp cx, dx
    sete al
    xor ah, ah
    cmp ax, 0
    setne ax
    jmp .L_logic_end_9
.L_logic_short_9:
    xor ax, ax
.L_logic_end_9:
    xor ah, ah
    mov bx, ax
    cmp bx, 0
    je .L_logic_short_10
    ; Load variable: command
    mov cx, ax
    mov ax, 3
    imul ax, 4
    add cx, ax
    mov ax, [cx]
    mov cx, ax
    mov ax, 111
    mov dx, ax
    cmp cx, dx
    sete al
    xor ah, ah
    cmp ax, 0
    setne ax
    jmp .L_logic_end_10
.L_logic_short_10:
    xor ax, ax
.L_logic_end_10:
    xor ah, ah
    test ax, ax
    jz _else_7
    ; Load variable: command
    mov bx, ax
    mov ax, 4
    imul ax, 4
    add bx, ax
    mov ax, [bx]
    mov bx, ax
    mov ax, 32
    mov cx, ax
    cmp bx, cx
    sete al
    xor ah, ah
    test ax, ax
    jz _else_11
    ; Load variable: command
    mov bx, ax
    mov ax, 5
    mov cx, ax
    mov ax, bx
    add ax, cx
    push ax
    call _writeDebugnl
    add sp, 2
    jmp _end_if_11
_else_11:
    mov ax, _str_21
    push ax
    call _writeDebug
    add sp, 2
_end_if_11:
    jmp _end_if_7
_else_7:
    ; Load variable: command
    mov bx, ax
    mov ax, 0
    imul ax, 4
    add bx, ax
    mov ax, [bx]
    mov bx, ax
    mov ax, 0
    mov cx, ax
    cmp bx, cx
    sete al
    xor ah, ah
    test ax, ax
    jz _else_12
    ; Continue statement - would need loop context
    jmp _end_if_12
_else_12:
    mov ax, 1
    push ax
    mov ax, _str_22
    push ax
    call _writeLog
    add sp, 4
    ; Load variable: command
    push ax
    call _writeDebugnl
    add sp, 2
_end_if_12:
_end_if_7:
_end_if_6:
_end_if_5:
_end_if_4:
    jmp _loop_3
_end_loop_3:
_func_epilogue_2:
    mov sp, bp
    pop bp
    ret
_strcmp:
    push bp
    mov bp, sp
_loop_14:
    ; Load variable: str1
    mov ax, [bp+4]
    mov ax, [ax]
    mov bx, ax
    cmp bx, 0
    je .L_logic_short_15
    ; Load variable: str1
    mov ax, [bp+4]
    mov ax, [ax]
    mov cx, ax
    ; Load variable: str2
    mov ax, [bp+6]
    mov ax, [ax]
    mov dx, ax
    cmp cx, dx
    sete al
    xor ah, ah
    cmp ax, 0
    setne ax
    jmp .L_logic_end_15
.L_logic_short_15:
    xor ax, ax
.L_logic_end_15:
    xor ah, ah
    test ax, ax
    jz _end_loop_14
    ; Load variable: str1
    mov ax, [bp+4]
    inc ax
    ; Load variable: str2
    mov ax, [bp+6]
    inc ax
    jmp _loop_14
_end_loop_14:
    ; Load variable: str1
    mov ax, [bp+4]
    mov ax, [ax]
    mov bx, ax
    ; Load variable: str2
    mov ax, [bp+6]
    mov ax, [ax]
    mov cx, ax
    mov ax, bx
    sub ax, cx
    jmp _func_epilogue_13
_func_epilogue_13:
    mov sp, bp
    pop bp
    ret
_clearScreen:
    push bp
    mov bp, sp
    ; Inline assembly
    mov ax, 0x0003
    ; Inline assembly
    int 0x10
_func_epilogue_16:
    mov sp, bp
    pop bp
    ret
_haltForever:
    ; Inline assembly
    cli
_loop_18:
    mov ax, 1
    test ax, ax
    jz _end_loop_18
    ; Inline assembly
    hlt
    jmp _loop_18
_end_loop_18:
_func_epilogue_17:
_writeChar:
    push bp
    mov bp, sp
    ; Inline assembly
    mov al, [bp+4]
    ; Inline assembly
    mov ah, 0x0E
    ; Inline assembly
    mov bx, 0x0007
    ; Inline assembly
    int 0x10
_func_epilogue_19:
    mov sp, bp
    pop bp
    ret
_getChar:
    push bp
    mov bp, sp
    ; Inline assembly
    mov ah, 0x00
    ; Inline assembly
    int 0x16
_func_epilogue_20:
    mov sp, bp
    pop bp
    ret
_getCharLetter:
    ; Inline assembly
    mov ah, 0x00
    ; Inline assembly
    int 0x16
    ; Inline assembly
    xor ah, ah
    ; Inline assembly
    ret
_func_epilogue_21:
_getString:
    push bp
    mov bp, sp
    ; Variable declaration: buffer
    sub sp, 256
    ; Variable declaration: ptr
    ; Load variable: buffer
    mov ax, [bp-256]
    mov ax, ax
    ; Variable declaration: i
    mov cx, 0
_for_loop_23:
    ; Load variable: i
    mov ax, cx
    mov bx, ax
    mov ax, 4
    ; sizeof - using placeholder value
    mov dx, ax
    cmp bx, dx
    setl al
    xor ah, ah
    test ax, ax
    jz _for_end_23
    ; Load variable: buffer
    mov ax, [bp-256]
    mov bx, ax
    ; Load variable: i
    mov ax, cx
    add bx, ax
    mov ax, 0
    mov [bx], ax
    xor ah, ah
_for_continue_23:
    ; Load variable: i
    mov ax, cx
    inc ax
    jmp _for_loop_23
_for_end_23:
    ; Variable declaration: c
    sub sp, 1
    sub sp, 15
_loop_24:
    mov ax, 1
    test ax, ax
    jz _end_loop_24
    call _getCharLetter
    mov [bp-257], ax
    ; Load variable: c
    mov ax, [bp-257]
    mov bx, ax
    mov ax, 13
    mov dx, ax
    cmp bx, dx
    sete al
    xor ah, ah
    test ax, ax
    jz _else_25
    ; Load variable: ptr
    mov bx, ax
    mov ax, 0
    mov [bx], ax
    xor ah, ah
    ; Break statement - would need loop context
    jmp _end_if_25
_else_25:
    ; Load variable: c
    mov ax, [bp-257]
    mov bx, ax
    mov ax, 8
    mov dx, ax
    cmp bx, dx
    sete al
    xor ah, ah
    test ax, ax
    jz _else_26
    ; Load variable: ptr
    mov bx, ax
    ; Load variable: buffer
    mov ax, [bp-256]
    mov dx, ax
    cmp bx, dx
    setg al
    xor ah, ah
    test ax, ax
    jz _end_if_27
    ; Load variable: ptr
    dec ax
    mov ax, 8
    push ax
    call _writeChar
    add sp, 2
    mov ax, 32
    push ax
    call _writeChar
    add sp, 2
    mov ax, 8
    push ax
    call _writeChar
    add sp, 2
_end_if_27:
    jmp _end_if_26
_else_26:
    ; Load variable: c
    mov ax, [bp-257]
    mov bx, ax
    mov ax, 27
    mov dx, ax
    cmp bx, dx
    sete al
    xor ah, ah
    test ax, ax
    jz _else_28
    ; Load variable: ptr
    mov bx, ax
    mov ax, 0
    mov [bx], ax
    xor ah, ah
    ; Break statement - would need loop context
    jmp _end_if_28
_else_28:
    ; Load variable: c
    mov ax, [bp-257]
    mov bx, ax
    mov ax, 32
    mov dx, ax
    cmp bx, dx
    setge al
    xor ah, ah
    mov bx, ax
    cmp bx, 0
    je .L_logic_short_30
    ; Load variable: c
    mov ax, [bp-257]
    mov dx, ax
    mov ax, 126
    mov si, ax
    cmp dx, si
    setle al
    xor ah, ah
    cmp ax, 0
    setne ax
    jmp .L_logic_end_30
.L_logic_short_30:
    xor ax, ax
.L_logic_end_30:
    xor ah, ah
    test ax, ax
    jz _end_if_29
    ; Load variable: ptr
    inc ax
    mov bx, ax
    ; Load variable: c
    mov ax, [bp-257]
    mov [bx], ax
    xor ah, ah
    ; Load variable: c
    mov ax, [bp-257]
    push ax
    call _writeChar
    add sp, 2
    ; Load variable: ptr
    mov bx, ax
    ; Load variable: buffer
    mov ax, [bp-256]
    mov dx, ax
    mov ax, 4
    ; sizeof - using placeholder value
    mov si, ax
    mov ax, dx
    add ax, si
    mov dx, ax
    mov ax, 1
    mov si, ax
    mov ax, dx
    sub ax, si
    mov dx, ax
    cmp bx, dx
    setge al
    xor ah, ah
    test ax, ax
    jz _end_if_31
    ; Break statement - would need loop context
_end_if_31:
_end_if_29:
    ; Variable declaration: btoathing
    ; Load variable: c
    mov ax, [bp-257]
    push ax
    call _btoa_hex
    add sp, 2
    mov dx, ax
    ; Load variable: btoathing
    mov ax, dx
    push ax
    call _writeUartString
    add sp, 2
_end_if_28:
_end_if_26:
_end_if_25:
    jmp _loop_24
_end_loop_24:
    ; Load variable: newline
    mov ax, [bp+4]
    test ax, ax
    jz _end_if_32
    mov ax, _str_23
    push ax
    call _writeDebug
    add sp, 2
_end_if_32:
    ; Load variable: buffer
    mov ax, [bp-256]
    jmp _func_epilogue_22
_func_epilogue_22:
    mov sp, bp
    pop bp
    ret
_writeStringnl:
    push bp
    mov bp, sp
    ; Load variable: str
    mov ax, [bp+4]
    push ax
    call _writeString
    add sp, 2
    mov ax, _str_24
    push ax
    call _writeString
    add sp, 2
_func_epilogue_33:
    mov sp, bp
    pop bp
    ret
_writeString:
    push bp
    mov bp, sp
_loop_35:
    ; Load variable: str
    mov ax, [bp+4]
    mov ax, [ax]
    test ax, ax
    jz _end_loop_35
    ; Load variable: str
    mov ax, [bp+4]
    inc ax
    mov ax, [ax]
    push ax
    call _writeChar
    add sp, 2
    jmp _loop_35
_end_loop_35:
_func_epilogue_34:
    mov sp, bp
    pop bp
    ret
_writeDebugnl:
    push bp
    mov bp, sp
    ; Load variable: str
    mov ax, [bp+4]
    push ax
    call _writeString
    add sp, 2
    mov ax, _str_25
    push ax
    call _writeString
    add sp, 2
    ; Load variable: str
    mov ax, [bp+4]
    push ax
    call _writeUartString
    add sp, 2
    mov ax, _str_26
    push ax
    call _writeUartString
    add sp, 2
_func_epilogue_36:
    mov sp, bp
    pop bp
    ret
_writeDebug:
    push bp
    mov bp, sp
    ; Load variable: str
    mov ax, [bp+4]
    push ax
    call _writeString
    add sp, 2
    ; Load variable: str
    mov ax, [bp+4]
    push ax
    call _writeUartString
    add sp, 2
_func_epilogue_37:
    mov sp, bp
    pop bp
    ret
_softReset:
    ; Inline assembly
    cli
    ; Inline assembly
    jmp 0xFFFF:0x0000
_func_epilogue_38:
_writeFarPtr:
    push bp
    mov bp, sp
    ; Inline assembly
    push es
    ; Inline assembly
    mov ax, [bp+4]
    mov es, ax
    ; Inline assembly
    mov ax, [bp+6]
    mov di, ax
    ; Inline assembly
    xor ax, ax
    ; Inline assembly
    mov al, [bp+8]
    mov [es:di], al
    ; Inline assembly
    pop es
_func_epilogue_39:
    mov sp, bp
    pop bp
    ret
_readFarPtr:
    push bp
    mov bp, sp
    ; Variable declaration: byte
    sub sp, 1
    sub sp, 15
    ; Inline assembly
    push es
    ; Inline assembly
    mov ax, [bp+4]
    mov es, ax
    ; Inline assembly
    mov ax, [bp+6]
    mov di, ax
    ; Inline assembly
    xor ax, ax
    ; Inline assembly
    mov al, [es:di]
    mov [bp-1], al
    ; Inline assembly
    pop es
    ; Load variable: byte
    mov ax, [bp-1]
    jmp _func_epilogue_40
_func_epilogue_40:
    mov sp, bp
    pop bp
    ret
_writePtr:
    push bp
    mov bp, sp
    ; Inline assembly
    mov ax, [bp+4]
    mov di, ax
    ; Inline assembly
    xor ax, ax
    ; Inline assembly
    mov al, [bp+6]
    mov [es:di], al
_func_epilogue_41:
    mov sp, bp
    pop bp
    ret
_readPtr:
    push bp
    mov bp, sp
    ; Variable declaration: byte
    sub sp, 1
    sub sp, 15
    ; Inline assembly
    mov ax, [bp+4]
    mov di, ax
    ; Inline assembly
    xor ax, ax
    ; Inline assembly
    mov al, [es:di]
    mov [bp-1], al
    ; Load variable: byte
    mov ax, [bp-1]
    jmp _func_epilogue_42
_func_epilogue_42:
    mov sp, bp
    pop bp
    ret
_enterVGAGraphicsMode:
    push bp
    mov bp, sp
    ; Inline assembly
    mov ax, 0x13
    ; Inline assembly
    int 0x10
_func_epilogue_43:
    mov sp, bp
    pop bp
    ret
_clearVGAScreen:
    push bp
    mov bp, sp
    mov ax, 64000
    push ax
    ; Load variable: c
    mov ax, [bp+4]
    push ax
    mov ax, 0
    push ax
    mov ax, 40960
    push ax
    call _memset_far
    add sp, 8
_func_epilogue_44:
    mov sp, bp
    pop bp
    ret
_drawPixel:
    push bp
    mov bp, sp
    ; Variable declaration: offset
    ; Load variable: y
    mov ax, [bp+6]
    mov bx, ax
    mov ax, 320
    mov cx, ax
    mov ax, bx
    mul cx
    mov bx, ax
    ; Load variable: x
    mov ax, [bp+4]
    mov cx, ax
    mov ax, bx
    add ax, cx
    mov ax, ax
    ; Inline assembly
    push es
    ; Inline assembly
    mov ax, 0xA000
    ; Inline assembly
    mov es, ax
    ; Inline assembly
    mov ax, ax
    mov di, ax
    ; Inline assembly
    xor ax, ax
    ; Inline assembly
    mov al, [bp+8]
    mov al, al
    ; Inline assembly
    stosb
    ; Inline assembly
    pop es
_func_epilogue_45:
    mov sp, bp
    pop bp
    ret
_drawRect:
    push bp
    mov bp, sp
_func_epilogue_46:
    mov sp, bp
    pop bp
    ret
_memset_far:
    push bp
    mov bp, sp
    ; Variable declaration: old_seg
    call _getESSegment
    mov ax, ax
    ; Load variable: seg
    mov ax, [bp+4]
    push ax
    call _setESSegment
    add sp, 2
    ; Inline assembly
    mov ax, [bp+6]
    mov di, ax
    ; Inline assembly
    mov ax, [bp+10]
    mov cx, ax
    ; Inline assembly
    xor ax, ax
    ; Inline assembly
    mov al, [bp+8]
    mov al, al
    ; Inline assembly
    rep stosb
    ; Load variable: old_seg
    push ax
    call _setESSegment
    add sp, 2
_func_epilogue_47:
    mov sp, bp
    pop bp
    ret
    ; Variable declaration: btoa_hex_buffer
#section .data
btoa_hex_buffer:
    #db 0
    #db 0
    #db 0
_btoa_hex:
    push bp
    mov bp, sp
    ; Inline assembly
    cli
    ; Variable declaration: digit
    sub sp, 1
    sub sp, 15
    ; Load variable: i
    mov ax, [bp+4]
    mov bx, ax
    mov ax, 4
    mov cx, ax
    mov ax, bx
    mov cl, cx
    shr ax, cl
    mov bx, ax
    mov ax, 15
    mov cx, ax
    mov ax, bx
    and ax, cx
    mov [bp-1], ax
    ; Load variable: btoa_hex_buffer
    mov ax, [btoa_hex_buffer]
    mov bx, ax
    mov ax, 0
    add bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, 10
    mov dx, ax
    cmp cx, dx
    setl al
    xor ah, ah
    mov [bx], ax
    xor ah, ah
    cmp ax, 0
    je .L_ternary_false_0
.L_ternary_true_0:
    mov ax, 48
    mov bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, bx
    add ax, cx
    jmp .L_ternary_end_0
.L_ternary_false_0:
    mov ax, 65
    mov bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, bx
    add ax, cx
    mov bx, ax
    mov ax, 10
    mov cx, ax
    mov ax, bx
    sub ax, cx
.L_ternary_end_0:
    ; Load variable: i
    mov ax, [bp+4]
    mov bx, ax
    mov ax, 15
    mov cx, ax
    mov ax, bx
    and ax, cx
    mov [bp-1], ax
    ; Load variable: btoa_hex_buffer
    mov ax, [btoa_hex_buffer]
    mov bx, ax
    mov ax, 1
    add bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, 10
    mov dx, ax
    cmp cx, dx
    setl al
    xor ah, ah
    mov [bx], ax
    xor ah, ah
    cmp ax, 0
    je .L_ternary_false_1
.L_ternary_true_1:
    mov ax, 48
    mov bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, bx
    add ax, cx
    jmp .L_ternary_end_1
.L_ternary_false_1:
    mov ax, 65
    mov bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, bx
    add ax, cx
    mov bx, ax
    mov ax, 10
    mov cx, ax
    mov ax, bx
    sub ax, cx
.L_ternary_end_1:
    ; Load variable: btoa_hex_buffer
    mov ax, [btoa_hex_buffer]
    mov bx, ax
    mov ax, 2
    add bx, ax
    mov ax, 0
    mov [bx], ax
    xor ah, ah
    ; Inline assembly
    sti
    ; Load variable: btoa_hex_buffer
    mov ax, [btoa_hex_buffer]
    jmp _func_epilogue_48
_func_epilogue_48:
    mov sp, bp
    pop bp
    ret
    ; Variable declaration: shex_buffer
#section .data
shex_buffer:
    #db 0
    #db 0
    #db 0
    #db 0
    #db 0
_stoa_hex:
    push bp
    mov bp, sp
    ; Inline assembly
    cli
    ; Variable declaration: digit
    sub sp, 1
    sub sp, 15
    ; Load variable: i
    mov ax, [bp+4]
    mov bx, ax
    mov ax, 12
    mov cx, ax
    mov ax, bx
    mov cl, cx
    shr ax, cl
    mov bx, ax
    mov ax, 15
    mov cx, ax
    mov ax, bx
    and ax, cx
    mov [bp-1], ax
    ; Load variable: shex_buffer
    mov ax, [shex_buffer]
    mov bx, ax
    mov ax, 0
    add bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, 10
    mov dx, ax
    cmp cx, dx
    setl al
    xor ah, ah
    mov [bx], ax
    xor ah, ah
    cmp ax, 0
    je .L_ternary_false_2
.L_ternary_true_2:
    mov ax, 48
    mov bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, bx
    add ax, cx
    jmp .L_ternary_end_2
.L_ternary_false_2:
    mov ax, 65
    mov bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, bx
    add ax, cx
    mov bx, ax
    mov ax, 10
    mov cx, ax
    mov ax, bx
    sub ax, cx
.L_ternary_end_2:
    ; Load variable: i
    mov ax, [bp+4]
    mov bx, ax
    mov ax, 8
    mov cx, ax
    mov ax, bx
    mov cl, cx
    shr ax, cl
    mov bx, ax
    mov ax, 15
    mov cx, ax
    mov ax, bx
    and ax, cx
    mov [bp-1], ax
    ; Load variable: shex_buffer
    mov ax, [shex_buffer]
    mov bx, ax
    mov ax, 1
    add bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, 10
    mov dx, ax
    cmp cx, dx
    setl al
    xor ah, ah
    mov [bx], ax
    xor ah, ah
    cmp ax, 0
    je .L_ternary_false_3
.L_ternary_true_3:
    mov ax, 48
    mov bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, bx
    add ax, cx
    jmp .L_ternary_end_3
.L_ternary_false_3:
    mov ax, 65
    mov bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, bx
    add ax, cx
    mov bx, ax
    mov ax, 10
    mov cx, ax
    mov ax, bx
    sub ax, cx
.L_ternary_end_3:
    ; Load variable: i
    mov ax, [bp+4]
    mov bx, ax
    mov ax, 4
    mov cx, ax
    mov ax, bx
    mov cl, cx
    shr ax, cl
    mov bx, ax
    mov ax, 15
    mov cx, ax
    mov ax, bx
    and ax, cx
    mov [bp-1], ax
    ; Load variable: shex_buffer
    mov ax, [shex_buffer]
    mov bx, ax
    mov ax, 2
    add bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, 10
    mov dx, ax
    cmp cx, dx
    setl al
    xor ah, ah
    mov [bx], ax
    xor ah, ah
    cmp ax, 0
    je .L_ternary_false_4
.L_ternary_true_4:
    mov ax, 48
    mov bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, bx
    add ax, cx
    jmp .L_ternary_end_4
.L_ternary_false_4:
    mov ax, 65
    mov bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, bx
    add ax, cx
    mov bx, ax
    mov ax, 10
    mov cx, ax
    mov ax, bx
    sub ax, cx
.L_ternary_end_4:
    ; Load variable: i
    mov ax, [bp+4]
    mov bx, ax
    mov ax, 15
    mov cx, ax
    mov ax, bx
    and ax, cx
    mov [bp-1], ax
    ; Load variable: shex_buffer
    mov ax, [shex_buffer]
    mov bx, ax
    mov ax, 3
    add bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, 10
    mov dx, ax
    cmp cx, dx
    setl al
    xor ah, ah
    mov [bx], ax
    xor ah, ah
    cmp ax, 0
    je .L_ternary_false_5
.L_ternary_true_5:
    mov ax, 48
    mov bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, bx
    add ax, cx
    jmp .L_ternary_end_5
.L_ternary_false_5:
    mov ax, 65
    mov bx, ax
    ; Load variable: digit
    mov ax, [bp-1]
    mov cx, ax
    mov ax, bx
    add ax, cx
    mov bx, ax
    mov ax, 10
    mov cx, ax
    mov ax, bx
    sub ax, cx
.L_ternary_end_5:
    ; Load variable: shex_buffer
    mov ax, [shex_buffer]
    mov bx, ax
    mov ax, 4
    add bx, ax
    mov ax, 0
    mov [bx], ax
    xor ah, ah
    ; Inline assembly
    sti
    ; Load variable: shex_buffer
    mov ax, [shex_buffer]
    jmp _func_epilogue_49
_func_epilogue_49:
    mov sp, bp
    pop bp
    ret
__GDT:
    ; Inline assembly
    gdt_start:
    ; Inline assembly
    #dw 0x0000
    ; Inline assembly
    #dw 0x0000
    ; Inline assembly
    #dw 0x0000
    ; Inline assembly
    #dw 0xFFFF
    ; Inline assembly
    #dw 0x0000
    ; Inline assembly
    #dw 0x9A00
    ; Inline assembly
    #dw 0xCF00
    ; Inline assembly
    #dw 0xFFFF
    ; Inline assembly
    #dw 0x0000
    ; Inline assembly
    #dw 0x9200
    ; Inline assembly
    #dw 0xCF00
    ; Inline assembly
    gdt_end:
    ; Inline assembly
    gdt_descriptor:
    ; Inline assembly
    #dw gdt_end - gdt_start - 1
    ; Inline assembly
    #dd gdt_start
_func_epilogue_50:
_setGDT:
    ; Inline assembly
    lgdt [gdt_descriptor]
    ; Inline assembly
    ret
_func_epilogue_51:
_setESSegment:
    push bp
    mov bp, sp
    ; Inline assembly
    mov ax, [bp+4]
    mov es, ax
_func_epilogue_52:
    mov sp, bp
    pop bp
    ret
_getESSegment:
    push bp
    mov bp, sp
    ; Variable declaration: es
    ; Inline assembly
    mov ax, es
    ; Inline assembly
    mov ax, ax
    mov ax, ax
    ; Load variable: es
    jmp _func_epilogue_53
_func_epilogue_53:
    mov sp, bp
    pop bp
    ret
_getCS:
    push bp
    mov bp, sp
    ; Variable declaration: cs
    ; Inline assembly
    mov ax, cs
    ; Inline assembly
    mov ax, ax
    mov ax, ax
    ; Load variable: cs
    jmp _func_epilogue_54
_func_epilogue_54:
    mov sp, bp
    pop bp
    ret
_initUart:
    push bp
    mov bp, sp
    ; Inline assembly
    mov dx, 0x3F8
    ; Inline assembly
    mov al, 0x80
    ; Inline assembly
    out dx, al
    ; Inline assembly
    mov al, 0x03
    ; Inline assembly
    out dx, al
    ; Inline assembly
    mov al, 0x03
    ; Inline assembly
    out dx, al
    ; Inline assembly
    mov al, 0x01
    ; Inline assembly
    out dx, al
    mov ax, _str_27
    push ax
    call _writeUartString
    add sp, 2
_func_epilogue_55:
    mov sp, bp
    pop bp
    ret
_writeUart:
    push bp
    mov bp, sp
    ; Inline assembly
    mov dx, 0x3F8
    ; Inline assembly
    mov al, [bp+4]
    ; Inline assembly
    out dx, al
_func_epilogue_56:
    mov sp, bp
    pop bp
    ret
_writeUartString:
    push bp
    mov bp, sp
_loop_58:
    ; Load variable: str
    mov ax, [bp+4]
    mov ax, [ax]
    test ax, ax
    jz _end_loop_58
    ; Load variable: str
    mov ax, [bp+4]
    inc ax
    mov ax, [ax]
    push ax
    call _writeUart
    add sp, 2
    jmp _loop_58
_end_loop_58:
_func_epilogue_57:
    mov sp, bp
    pop bp
    ret
_writeLognl:
    push bp
    mov bp, sp
    ; Load variable: level
    mov ax, [bp+6]
    push ax
    ; Load variable: str
    mov ax, [bp+4]
    push ax
    call _writeLog
    add sp, 4
    mov ax, _str_28
    push ax
    call _writeDebug
    add sp, 2
_func_epilogue_59:
    mov sp, bp
    pop bp
    ret
_writeLog:
    push bp
    mov bp, sp
    ; Variable declaration: prefix
    mov ax, 0
    ; Load variable: level
    mov ax, [bp+6]
    mov bx, ax
    mov ax, 0
    mov cx, ax
    cmp bx, cx
    sete al
    xor ah, ah
    test ax, ax
    jz _else_61
    mov ax, _str_29
    mov ax, ax
    jmp _end_if_61
_else_61:
    ; Load variable: level
    mov ax, [bp+6]
    mov bx, ax
    mov ax, 1
    mov cx, ax
    cmp bx, cx
    sete al
    xor ah, ah
    test ax, ax
    jz _else_62
    mov ax, _str_30
    mov ax, ax
    jmp _end_if_62
_else_62:
    ; Load variable: level
    mov ax, [bp+6]
    mov bx, ax
    mov ax, 2
    mov cx, ax
    cmp bx, cx
    sete al
    xor ah, ah
    test ax, ax
    jz _else_63
    mov ax, _str_31
    mov ax, ax
    jmp _end_if_63
_else_63:
    ; Load variable: level
    mov ax, [bp+6]
    mov bx, ax
    mov ax, 3
    mov cx, ax
    cmp bx, cx
    sete al
    xor ah, ah
    test ax, ax
    jz _else_64
    mov ax, _str_32
    mov ax, ax
    jmp _end_if_64
_else_64:
    mov ax, _str_33
    mov ax, ax
_end_if_64:
_end_if_63:
_end_if_62:
_end_if_61:
    ; Load variable: prefix
    test ax, ax
    jz _end_if_65
    ; Load variable: prefix
    push ax
    call _writeString
    add sp, 2
    ; Load variable: prefix
    push ax
    call _writeUartString
    add sp, 2
_end_if_65:
    ; Load variable: str
    mov ax, [bp+4]
    push ax
    call _writeString
    add sp, 2
    ; Load variable: str
    mov ax, [bp+4]
    push ax
    call _writeUartString
    add sp, 2
_func_epilogue_60:
    mov sp, bp
    pop bp
    ret
_outb:
    push bp
    mov bp, sp
    ; Inline assembly
    mov ax, [bp+4]
    mov dx, ax
    ; Inline assembly
    xor ax, ax
    ; Inline assembly
    mov al, [bp+6]
    mov al, al
    ; Inline assembly
    out dx, al
_func_epilogue_66:
    mov sp, bp
    pop bp
    ret
_inb:
    push bp
    mov bp, sp
    ; Variable declaration: value
    sub sp, 1
    sub sp, 15
    ; Inline assembly
    mov ax, [bp+4]
    mov dx, ax
    ; Inline assembly
    in al, dx
    ; Inline assembly
    mov al, al
    mov [bp-1], al
    ; Load variable: value
    mov ax, [bp-1]
    jmp _func_epilogue_67
_func_epilogue_67:
    mov sp, bp
    pop bp
    ret
_initPIT:
    push bp
    mov bp, sp
    ; Inline assembly
    cli
    mov ax, 52
    push ax
    mov ax, 67
    push ax
    call _outb
    add sp, 4
    mov ax, 1193
    mov bx, ax
    mov ax, 255
    mov cx, ax
    mov ax, bx
    and ax, cx
    push ax
    mov ax, 64
    push ax
    call _outb
    add sp, 4
    mov ax, 1193
    mov bx, ax
    mov ax, 8
    mov cx, ax
    mov ax, bx
    mov cl, cx
    shr ax, cl
    mov bx, ax
    mov ax, 255
    mov cx, ax
    mov ax, bx
    and ax, cx
    push ax
    mov ax, 64
    push ax
    call _outb
    add sp, 4
    mov ax, 32
    push ax
    mov ax, 32
    push ax
    call _outb
    add sp, 4
    mov ax, 32
    push ax
    mov ax, 160
    push ax
    call _outb
    add sp, 4
    ; Inline assembly
    sti
_func_epilogue_68:
    mov sp, bp
    pop bp
    ret
_sizeOfKernel:
    push bp
    mov bp, sp
    ; Variable declaration: size
    mov ax, 0
    ; Inline assembly
    mov ax, _KERNEL_START
    ; Inline assembly
    mov bx, _KERNEL_END
    ; Inline assembly
    sub bx, ax
    ; Inline assembly
    mov ax, bx
    mov ax, ax
    ; Load variable: size
    jmp _func_epilogue_69
_func_epilogue_69:
    mov sp, bp
    pop bp
    ret
_installIRQS:
    push bp
    mov bp, sp
    ; Inline assembly
    cli
    ; Inline assembly
    pusha
    ; Inline assembly
    mov bx, cs
    ; Inline assembly
    mov cx, _irq0_handler
    ; Inline assembly
    push es
    ; Inline assembly
    mov ax, 0x0000
    ; Inline assembly
    mov es, ax
    ; Inline assembly
    mov di, 0x20
    ; Inline assembly
    mov [es:di], cx
    ; Inline assembly
    mov [es:di+2], bx
    ; Inline assembly
    pop es
    ; Inline assembly
    popa
    ; Inline assembly
    sti
_func_epilogue_70:
    mov sp, bp
    pop bp
    ret
    ; Variable declaration: seconds
#section .data
seconds:
    #dw 0
    ; Variable declaration: tick
#section .data
tick:
    #dw 0
_getSeconds:
    push bp
    mov bp, sp
    ; Load variable: seconds
    mov ax, [seconds]
    jmp _func_epilogue_71
_func_epilogue_71:
    mov sp, bp
    pop bp
    ret
_getTick:
    push bp
    mov bp, sp
    ; Load variable: tick
    mov ax, [tick]
    jmp _func_epilogue_72
_func_epilogue_72:
    mov sp, bp
    pop bp
    ret
_setTick:
    push bp
    mov bp, sp
    ; Load variable: t
    mov ax, [bp+4]
    mov [tick], ax
_func_epilogue_73:
    mov sp, bp
    pop bp
    ret
_irq0_handler:
    ; Inline assembly
    cli
    ; Inline assembly
    pusha
    ; Inline assembly
    inc word [cs:_kernel_tick]
    ; Inline assembly
    cmp word [cs:_kernel_tick], 1000
    ; Inline assembly
    jne _irq0_handler_end
    ; Inline assembly
    mov word [cs:_kernel_tick], 0x0000
    ; Inline assembly
    inc word [cs:_kernel_seconds]
    ; Inline assembly
    _irq0_handler_end:
    ; Inline assembly
    popa
    ; Inline assembly
    mov al, 0x20
    ; Inline assembly
    out 0x20, al
    ; Inline assembly
    out 0xA0, al
    ; Inline assembly
    sti
    ; Inline assembly
    iret
_func_epilogue_74:
_NCC_GLOBAL_LOC:
_func_epilogue_75:
_NCC_ARRAY_LOC:
_func_epilogue_76:
_NCC_STRING_LOC:
_func_epilogue_77:
_str_33:
    #db 0x5B, 0x55, 0x4E, 0x4B, 0x57, 0x5D, 0x20, 0
_str_32:
    #db 0x5B, 0x57, 0x41, 0x52, 0x4E, 0x5D, 0x20, 0
_str_31:
    #db 0x5B, 0x49, 0x4E, 0x46, 0x4F, 0x5D, 0x20, 0
_str_30:
    #db 0x5B, 0x46, 0x41, 0x49, 0x4C, 0x5D, 0x20, 0
_str_29:
    #db 0x5B, 0x4F, 0x4B, 0x41, 0x59, 0x5D, 0x20, 0
_str_28:
    #db 0xD, 0xA, 0
_str_27:
    #db 0xD, 0
_str_26:
    #db 0xD, 0xA, 0
_str_25:
    #db 0xD, 0xA, 0
_str_24:
    #db 0xD, 0xA, 0
_str_23:
    #db 0xD, 0xA, 0
_str_22:
    #db 0x55, 0x6E, 0x6B, 0x6E, 0x6F, 0x77, 0x6E, 0x20, 0x63, 0x6F, 0x6D, 0x6D, 0x61, 0x6E, 0x64, 0x3A, 0x20, 0
_str_21:
    #db 0xD, 0xA, 0
_str_20:
    #db 0x45, 0x78, 0x69, 0x74, 0x69, 0x6E, 0x67, 0x20, 0x73, 0x68, 0x65, 0x6C, 0x6C, 0x2E, 0x2E, 0x2E, 0
_str_19:
    #db 0x65, 0x78, 0x69, 0x74, 0
_str_18:
    #db 0x63, 0x6C, 0x65, 0x61, 0x72, 0
_str_17:
    #db 0x20, 0x20, 0x65, 0x78, 0x69, 0x74, 0x20, 0x2D, 0x20, 0x45, 0x78, 0x69, 0x74, 0x20, 0x74, 0x68, 0x65, 0x20, 0x73, 0x68, 0x65, 0x6C, 0x6C, 0
_str_16:
    #db 0x20, 0x20, 0x65, 0x63, 0x68, 0x6F, 0x20, 0x3C, 0x74, 0x65, 0x78, 0x74, 0x3E, 0x20, 0x2D, 0x20, 0x45, 0x63, 0x68, 0x6F, 0x20, 0x74, 0x68, 0x65, 0x20, 0x74, 0x65, 0x78, 0x74, 0x20, 0x74, 0x6F, 0x20, 0x74, 0x68, 0x65, 0x20, 0x73, 0x63, 0x72, 0x65, 0x65, 0x6E, 0
_str_15:
    #db 0x20, 0x20, 0x63, 0x6C, 0x65, 0x61, 0x72, 0x20, 0x2D, 0x20, 0x43, 0x6C, 0x65, 0x61, 0x72, 0x20, 0x74, 0x68, 0x65, 0x20, 0x73, 0x63, 0x72, 0x65, 0x65, 0x6E, 0
_str_14:
    #db 0x20, 0x20, 0x68, 0x65, 0x6C, 0x70, 0x20, 0x2D, 0x20, 0x53, 0x68, 0x6F, 0x77, 0x20, 0x74, 0x68, 0x69, 0x73, 0x20, 0x68, 0x65, 0x6C, 0x70, 0x20, 0x6D, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0
_str_13:
    #db 0x41, 0x76, 0x61, 0x69, 0x6C, 0x61, 0x62, 0x6C, 0x65, 0x20, 0x63, 0x6F, 0x6D, 0x6D, 0x61, 0x6E, 0x64, 0x73, 0x3A, 0
_str_12:
    #db 0x68, 0x65, 0x6C, 0x70, 0
_str_11:
    #db 0x6E, 0x63, 0x63, 0x3E, 0x20, 0
_str_10:
    #db 0x54, 0x79, 0x70, 0x65, 0x20, 0x27, 0x68, 0x65, 0x6C, 0x70, 0x27, 0x20, 0x66, 0x6F, 0x72, 0x20, 0x61, 0x20, 0x6C, 0x69, 0x73, 0x74, 0x20, 0x6F, 0x66, 0x20, 0x63, 0x6F, 0x6D, 0x6D, 0x61, 0x6E, 0x64, 0x73, 0x2E, 0
_str_9:
    #db 0x57, 0x65, 0x6C, 0x63, 0x6F, 0x6D, 0x65, 0x20, 0x74, 0x6F, 0x20, 0x74, 0x68, 0x65, 0x20, 0x4E, 0x43, 0x43, 0x20, 0x42, 0x6F, 0x6F, 0x74, 0x6C, 0x6F, 0x61, 0x64, 0x65, 0x72, 0x20, 0x73, 0x68, 0x65, 0x6C, 0x6C, 0x21, 0
_str_8:
    #db 0x45, 0x6E, 0x74, 0x65, 0x72, 0x69, 0x6E, 0x67, 0x20, 0x73, 0x68, 0x65, 0x6C, 0x6C, 0x2E, 0x2E, 0x2E, 0
_str_7:
    #db 0x50, 0x49, 0x54, 0x20, 0x69, 0x6E, 0x69, 0x74, 0x69, 0x61, 0x6C, 0x69, 0x7A, 0x65, 0x64, 0x2E, 0
_str_6:
    #db 0x49, 0x6E, 0x73, 0x74, 0x61, 0x6C, 0x6C, 0x65, 0x64, 0x20, 0x69, 0x6E, 0x74, 0x65, 0x72, 0x72, 0x75, 0x70, 0x74, 0x73, 0x2E, 0
_str_5:
    #db 0x49, 0x6E, 0x73, 0x74, 0x61, 0x6C, 0x6C, 0x69, 0x6E, 0x67, 0x20, 0x49, 0x4E, 0x54, 0x73, 0x2E, 0x2E, 0x2E, 0
_str_4:
    #db 0x47, 0x44, 0x54, 0x20, 0x73, 0x65, 0x74, 0x2E, 0
_str_3:
    #db 0x53, 0x65, 0x74, 0x74, 0x69, 0x6E, 0x67, 0x20, 0x47, 0x44, 0x54, 0x2E, 0x2E, 0x2E, 0
_str_2:
    #db 0x3A, 0x30, 0x78, 0x30, 0x30, 0x30, 0x30, 0x29, 0xD, 0xA, 0
_str_1:
    #db 0x4E, 0x43, 0x43, 0x20, 0x42, 0x6F, 0x6F, 0x74, 0x6C, 0x6F, 0x61, 0x64, 0x65, 0x72, 0x20, 0x6C, 0x6F, 0x61, 0x64, 0x65, 0x64, 0x20, 0x69, 0x6E, 0x20, 0x61, 0x74, 0x20, 0x28, 0
_str_0:
    #db 0x55, 0x41, 0x52, 0x54, 0x20, 0x69, 0x6E, 0x69, 0x74, 0x69, 0x61, 0x6C, 0x69, 0x7A, 0x65, 0x64, 0x2E, 0
_KERNEL_END:
_func_epilogue_78:
