[BITS 32]

section .asm

global restore_general_purpose_registers
global task_return
global user_registers

; void restore_general_purpose_registers(struct registers* regs);
restore_general_purpose_registers:
    push ebp
    mov ebp, esp

    mov ebx, [ebp+8]

    mov edi, [ebx]
    mov esi, [ebx+4]
    mov ebp, [ebx+8]
    mov edx, [ebx+16]
    mov ecx, [ebx+20]
    mov eax, [ebx+24]
    mov ebx, [ebx+12]
    
    pop ebp
    ret

; void user_registers();
user_registers:
    mov ax, 0x23

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ret

; void task_return(struct registers* regs);
task_return:
    mov ebp, esp

    ; Access the register structure passed from C
    mov ebx, [ebp+4]

    ; Push the user data/stack selector
    push dword [ebx+44]

    ; Push the user stack pointer
    push dword [ebx+40]

    ; Push EFLAGS with interrupts enabled
    pushf
    pop eax
    or eax, 0x200
    push eax

    ; Push user code selector
    push dword [ebx+32]

    ; Push instruction pointer
    push dword [ebx+28]

    ; Set user-mode data segment registers
    mov ax, [ebx+44]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Restore general-purpose registers
    push dword [ebp+4]
    call restore_general_purpose_registers
    add esp, 4

    ; Return into user mode
    iretd
