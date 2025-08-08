#width 64
_main:
    push rbp
    mov rbp, rsp
    push rbx
    ; Variable declaration: fact
    mov eax, 5
    mov rdi, rax
    call _factorial
    mov eax, eax
    ; Inline assembly
    mov eax, 3333
    ; Load variable: fact
    jmp _func_epilogue_0
_func_epilogue_0:
    mov rsp, rbp
    sub rsp, 8
    pop rbx
    pop rbp
    ret
_factorial:
    push rbp
    mov rbp, rsp
    push rbx
    ; Load variable: n
    mov eax, edi
    mov rbx, rax
    mov eax, 0
    mov rcx, rax
    cmp rbx, rcx
    sete al
    movzx rax, al
    test rax, rax
    jz _end_if_2
    mov eax, 1
    jmp _func_epilogue_1
_end_if_2:
    ; Load variable: n
    mov eax, edi
    mov rbx, rax
    ; Load variable: n
    mov eax, edi
    mov rcx, rax
    mov eax, 1
    mov rdx, rax
    mov rax, rcx
    sub rax, rdx
    mov rdi, rax
    call _factorial
    mov rcx, rax
    mov rax, rbx
    imul rax, rcx
    jmp _func_epilogue_1
_func_epilogue_1:
    mov rsp, rbp
    sub rsp, 8
    pop rbx
    pop rbp
    ret
_naked:
_func_epilogue_3:
