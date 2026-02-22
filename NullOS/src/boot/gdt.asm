; NullOS Global Descriptor Table
; ------------------------------
; Defines flat memory model segments for protected mode.
; Three entries: null, kernel code (0x08), kernel data (0x10).
;
; See: docs/PHASES.md Phase 1.2, docs/ARCHITECTURE.md GDT Layout

; TODO: Implement GDT
;
; gdt_start:
;   .null:    dq 0x0000000000000000    ; Null descriptor (required)
;   .code:    ; Base=0, Limit=4GB, Code, Execute/Read, Ring 0
;   .data:    ; Base=0, Limit=4GB, Data, Read/Write, Ring 0
; gdt_end:
;
; gdt_descriptor:
;   dw gdt_end - gdt_start - 1    ; Size
;   dd gdt_start                   ; Offset
;
; Constants for segment selectors:
; CODE_SEG equ gdt_start.code - gdt_start  ; 0x08
; DATA_SEG equ gdt_start.data - gdt_start  ; 0x10
