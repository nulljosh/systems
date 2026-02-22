; NullOS Stage 2 Bootloader
; -------------------------
; Handles real mode setup, A20 enablement, GDT load,
; protected mode switch, and jump to kernel.
;
; See: docs/PHASES.md Phase 1.2

[BITS 16]
[ORG 0x7E00]

KERNEL_LOAD_ADDR equ 0x100000  ; Kernel load address

start_stage2:
    ; Print Stage 2 message
    mov si, stage2_msg
    call puts_real
    
    ; Enable A20 line
    call a20_enable
    
    ; Load the GDT
    lgdt [gdt_descriptor]
    
    ; Disable interrupts and enter protected mode
    cli
    
    ; Set protected mode bit in CR0
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Far jump to flush pipeline and enter 32-bit mode
    jmp 0x08:protected_mode_entry
    
a20_enable:
    ; Try BIOS method first
    mov ax, 0x2401
    int 0x15
    
    ; If that fails, try keyboard controller
    cmp ah, 0
    je .a20_enabled
    
    ; Keyboard controller method
    call a20_keyboard
    
.a20_enabled:
    ret

a20_keyboard:
    ; Wait for keyboard controller to be ready
.wait_write:
    in al, 0x64
    test al, 2
    jnz .wait_write
    
    ; Send "write output port" command
    mov al, 0xD1
    out 0x64, al
    
.wait_write2:
    in al, 0x64
    test al, 2
    jnz .wait_write2
    
    ; Send enable A20 bit
    mov al, 0xDF
    out 0x60, al
    
    ret

; puts_real - Print string in real mode
; Input: DS:SI = string
puts_real:
    push ax
    push bx
    
    mov ah, 0x0E
    mov bh, 0
    mov bl, 0x07
    
.loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .loop
    
.done:
    pop bx
    pop ax
    ret

; GDT definition
gdt_start:
    ; Null descriptor
    dd 0x00000000
    dd 0x00000000
    
    ; Code segment (0x08)
    ; Base: 0x00000000, Limit: 0xFFFFF (4GB)
    ; Flags: Present, Ring 0, Code/Data
    ; Access: Execute, Read, Ring 0
    dw 0xFFFF           ; Limit (bits 0-15)
    dw 0x0000           ; Base (bits 0-15)
    db 0x00             ; Base (bits 16-23)
    db 0x9A             ; Access byte (10011010)
    db 0xCF             ; Flags and Limit high (11001111)
    db 0x00             ; Base (bits 24-31)
    
    ; Data segment (0x10)
    ; Base: 0x00000000, Limit: 0xFFFFF (4GB)
    ; Flags: Present, Ring 0, Code/Data
    ; Access: Read/Write, Ring 0
    dw 0xFFFF           ; Limit (bits 0-15)
    dw 0x0000           ; Base (bits 0-15)
    db 0x00             ; Base (bits 16-23)
    db 0x92             ; Access byte (10010010)
    db 0xCF             ; Flags and Limit high (11001111)
    db 0x00             ; Base (bits 24-31)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT size - 1
    dd gdt_start                 ; GDT base address

stage2_msg:
    db "NullOS Stage 2: Entering Protected Mode", 13, 10, 0

; ============= 32-BIT PROTECTED MODE =============
[BITS 32]

protected_mode_entry:
    ; Set data segment registers to 0x10 (data segment)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Set up 32-bit stack in high memory
    mov esp, 0x90000
    
    ; Print protected mode message
    mov esi, protected_msg
    call puts_prot
    
    ; Jump to kernel entry point at 0x100000
    jmp 0x100000

; puts_prot - Print string in protected mode
; Since we don't have full kernel VGA driver yet, use simple loop-based approach
; This is temporary - will be replaced by kernel VGA driver
puts_prot:
    ; For now, just return - we'll do proper output once kernel boots
    ret

protected_msg:
    db "Protected Mode OK", 0
