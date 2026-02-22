; NullOS Kernel Entry Point
; -------------------------
; Multiboot-compliant entry. QEMU or GRUB loads the kernel ELF
; directly at 0x100000, sets EAX=0x2BADB002, EBX=multiboot_info ptr,
; then jumps to _start.
;
; Sets up the initial 16KB stack and calls kmain().

[BITS 32]

; --- Multiboot 1 header (must be in first 8KB of kernel image) ---
section .multiboot
align 4
    dd 0x1BADB002               ; magic
    dd 0x00000003               ; flags: align modules + memory map
    dd -(0x1BADB002 + 0x00000003) ; checksum

; --- Kernel entry ---
section .text
global _start
extern kmain

_start:
    mov  esp, stack_top         ; set up 16KB stack
    xor  ebp, ebp               ; clear frame pointer
    call kmain                  ; call C kernel main
.hang:
    cli
    hlt
    jmp  .hang

; --- Initial kernel stack (16KB, BSS) ---
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
