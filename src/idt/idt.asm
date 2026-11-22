section .asm

extern int21h_handler
extern no_interrupt_handler
extern isr80h_handler
global isr80h_wrapper
global int21h
global idt_load
global no_interrupt

idt_load:
    push ebp
    mov ebp, esp
    mov ebx, [ebp+8]
    lidt [ebx]
    pop ebp
    ret

int21h:
    pushad
    push ds
    push es
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    cld
    call int21h_handler
    pop es
    pop ds
    popad
    iretd

no_interrupt:
    
    pushad
    call no_interrupt_handler
    popad
    
    iret

isr80h_wrapper:
    ; Save all general-purpose registers
    pushad
    cld

    ; Push address of interrupt frame
    push esp

    ; EAX contains the command number
    push eax

    call isr80h_handler

    ; Preserve C return value
    mov dword [tmp_res], eax

    ; Remove function arguments
    add esp, 8

    ; Restore original registers
    popad

    ; Restore return value into EAX
    mov eax, [tmp_res]

    ; Return to user space
    iretd


section .data

tmp_res:
    dd 0
