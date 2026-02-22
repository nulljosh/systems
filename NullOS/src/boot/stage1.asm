; NullOS Stage 1 Bootloader
; -------------------------
; Loaded by BIOS to 0x7C00. Exactly 512 bytes.
; Loads Stage 2 from disk and jumps to it.
;
; See: docs/PHASES.md Phase 1.1

[BITS 16]
[ORG 0x7C00]

; Boot parameters
STAGE2_LOAD_ADDR equ 0x7E00     ; Load Stage 2 here (after Stage 1)
STAGE2_SECTORS equ 15            ; Load 15 sectors (7680 bytes) for Stage 2

start:
    ; Disable interrupts during boot
    cli

    ; Set up segment registers for real mode
    xor ax, ax
    mov ds, ax                  ; DS = 0
    mov es, ax                  ; ES = 0
    mov ss, ax                  ; SS = 0
    
    ; Set up stack (grows downward from 0x7C00)
    mov sp, 0x7C00
    
    ; Enable interrupts
    sti

    ; Save boot drive number in BL (DL is passed by BIOS)
    mov bl, dl

    ; Print boot message
    mov si, boot_msg
    call puts

    ; Load Stage 2 from disk
    mov ax, STAGE2_LOAD_ADDR
    mov es, ax                  ; Set ES to segment where Stage 2 goes
    xor bx, bx                  ; Offset within segment (0)
    
    mov ah, 0x02                ; BIOS read sectors function
    mov al, STAGE2_SECTORS      ; Number of sectors to read
    mov ch, 0                   ; Cylinder 0
    mov cl, 2                   ; Start at sector 2 (sector 1 is Stage 1)
    mov dh, 0                   ; Head 0
    mov dl, bl                  ; Drive number (from boot drive)
    
    int 0x13                    ; BIOS disk read
    
    jc read_error               ; Jump if carry flag set (error)

    ; Print success message
    mov si, loaded_msg
    call puts

    ; Jump to Stage 2
    jmp STAGE2_LOAD_ADDR

read_error:
    ; Print error message
    mov si, error_msg
    call puts
    
    ; Hang
    jmp $

; puts - Print string
; Input: DS:SI = string pointer
; String ends with null byte
puts:
    push ax
    push bx
    
    mov ah, 0x0E                ; BIOS TTY write
    mov bh, 0                   ; Page 0
    mov bl, 0x07                ; Default color
    
.puts_loop:
    lodsb                       ; Load byte from DS:SI into AL
    cmp al, 0                   ; Check for null terminator
    je .puts_done
    
    int 0x10                    ; BIOS video interrupt
    jmp .puts_loop
    
.puts_done:
    pop bx
    pop ax
    ret

; Boot messages
boot_msg:
    db "NullOS Stage 1 Bootloader", 13, 10, 0

loaded_msg:
    db "Stage 2 loaded, jumping...", 13, 10, 0

error_msg:
    db "ERROR: Failed to load Stage 2!", 13, 10, 0

; Pad to 510 bytes and add boot signature
times 510 - ($ - $$) db 0
dw 0xAA55
