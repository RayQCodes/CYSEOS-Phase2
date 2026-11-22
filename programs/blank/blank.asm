[BITS 32]

; Values match src/isr80h/isr80h.h and src/status.h.
%define SYS_PRINT 1
%define SYS_GETKEY 2
%define SYS_READ 3
%define SYS_DELETE 4
%define SYS_WRITE 5
%define SYS_UNREGISTERED 999
%define ERR_INVALID_ARGUMENT -2
%define ERR_INVALID_SYSCALL -7
%define ERR_ACCESS_DENIED -9
%define MAX_LINE_LENGTH 31

section .asm
global _start

_start:
    cld
    mov edx, welcome
    call print_text

.prompt:
    mov edx, prompt
    call print_text
    xor edi, edi
    xor ebp, ebp

.read_char:
    pause
    mov eax, SYS_GETKEY
    int 0x80
    test eax, eax
    jz .read_char
    cmp al, 0x0D
    je .run_command
    cmp al, 0x08
    je .backspace
    cmp al, ' '
    jb .read_char
    cmp edi, MAX_LINE_LENGTH
    jae .line_too_long

    mov [command_line + edi], al
    inc edi
    mov [echo_char], al
    mov edx, echo_char
    call print_text
    jmp .read_char

.line_too_long:
    mov ebp, 1
    jmp .read_char

.backspace:
    test edi, edi
    jz .read_char
    dec edi
    mov edx, erase_char
    call print_text
    jmp .read_char

.run_command:
    mov byte [command_line + edi], 0
    mov edx, newline
    call print_text
    test ebp, ebp
    jnz .bad_command
    test edi, edi
    jz .prompt
    call parse_command
    cmp eax, -1
    je .bad_command

    ; The parser sets EAX to a syscall ID and EBX to its scalar argument.
    mov ecx, eax
    int 0x80
    mov esi, eax

    mov edx, invalid_syscall
    cmp esi, ERR_INVALID_SYSCALL
    jz .show_result
    cmp esi, ERR_INVALID_ARGUMENT
    je .show_argument
    cmp esi, ERR_ACCESS_DENIED
    je .show_argument
    test esi, esi
    jns .show_argument
    mov edx, syscall_failed
    jmp .show_result

.show_argument:
    mov edx, valid_syscall
    call print_text
    mov edx, valid_argument_prefix
    cmp esi, ERR_INVALID_ARGUMENT
    jne .show_record_id
    mov edx, invalid_argument_prefix
.show_record_id:
    call print_text
    mov edx, edi
    call print_text
    mov edx, invalid_argument
    cmp esi, ERR_INVALID_ARGUMENT
    je .show_result
    mov edx, newline
    call print_text

    test esi, esi
    mov edx, pass_prefix
    jns .show_permission_name
    mov edx, fail_prefix

.show_permission_name:
    call print_text
    mov edx, read_permission
    cmp ecx, SYS_READ
    je .show_permission_status
    mov edx, write_permission
    cmp ecx, SYS_WRITE
    je .show_permission_status
    mov edx, delete_permission

.show_permission_status:
    call print_text
    mov edx, success
    test esi, esi
    jns .show_result
    mov edx, access_denied
    jmp .show_result

.bad_command:
    mov edx, bad_command
.show_result:
    call print_text
    jmp .prompt

print_text:
    push edx
    mov eax, SYS_PRINT
    int 0x80
    add esp, 4
    ret

parse_command:
    mov esi, command_line
    mov edi, read_prefix
    mov ecx, read_prefix_length
    repe cmpsb
    mov eax, SYS_READ
    je .number

    mov esi, command_line
    mov edi, write_prefix
    mov ecx, write_prefix_length
    repe cmpsb
    mov eax, SYS_WRITE
    je .number

    mov esi, command_line
    mov edi, delete_prefix
    mov ecx, delete_prefix_length
    repe cmpsb
    mov eax, SYS_DELETE
    je .number

    mov esi, command_line
    mov edi, invalid_prefix
    mov ecx, invalid_prefix_length
    repe cmpsb
    mov eax, SYS_UNREGISTERED
    jne .bad

.number:
    mov edi, esi
    xor ebx, ebx
    movzx edx, byte [esi]
    sub edx, '0'
    cmp edx, 9
    ja .bad
.digit:
    imul ebx, ebx, 10
    jo .bad
    add ebx, edx
    jo .bad
    inc esi
    movzx edx, byte [esi]
    sub edx, '0'
    cmp edx, 9
    jbe .digit
    cmp byte [esi], ')'
    jne .bad
    cmp byte [esi + 1], 0
    jne .bad
    ; Keep the parsed record text in EDI for reporting the kernel result.
    mov byte [esi], 0
    ret

.bad:
    mov eax, -1
    ret

section .data

welcome db "CYSEOS Secure System Call Interface", 10
        db "Records 10 and 20. Try read(10), write(20), or delete(20).", 10
        db "Try read(999) or invalid(10) to see validation fail.", 10, 0
prompt db "> ", 0
newline db 10, 0
erase_char db 8, 0
valid_syscall db "[PASS] Valid syscall ID", 10, 0
valid_argument_prefix db "[PASS] Valid argument: Record ", 0
invalid_argument_prefix db "[FAIL] Invalid argument: Record ", 0
pass_prefix db "[PASS] ", 0
fail_prefix db "[FAIL] ", 0
read_permission db "READ", 0
write_permission db "WRITE", 0
delete_permission db "DELETE", 0
success db " permission granted", 10
        db "RESULT: SUCCESS (simulated request permitted)", 10, 0
access_denied db " permission denied", 10
              db "RESULT: ACCESS DENIED", 10, 0
invalid_argument db 10
                 db "RESULT: INVALID ARGUMENT", 10, 0
invalid_syscall db "[FAIL] Unregistered syscall ID", 10
                db "RESULT: INVALID SYSCALL ID", 10, 0
syscall_failed db "RESULT: SYSCALL FAILED", 10, 0
bad_command db "SYNTAX ERROR (use command(record ID))", 10, 0

invalid_prefix db "invalid("
invalid_prefix_length equ $ - invalid_prefix
read_prefix db "read("
read_prefix_length equ $ - read_prefix
write_prefix db "write("
write_prefix_length equ $ - write_prefix
delete_prefix db "delete("
delete_prefix_length equ $ - delete_prefix

echo_char db 0, 0
command_line times MAX_LINE_LENGTH + 1 db 0
