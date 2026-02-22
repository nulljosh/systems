; NullOS A20 Line Enablement
; --------------------------
; Enables the A20 address line so memory above 1 MB is accessible.
; Tries multiple methods for compatibility.
;
; See: docs/PHASES.md Phase 1.2

[BITS 16]

; TODO: Implement A20 enable
;
; a20_enable:
;   ; Method 1: BIOS (int 0x15, AX=0x2401)
;   ; Method 2: Keyboard controller (ports 0x64/0x60)
;   ; Method 3: Fast A20 (port 0x92)
;   ;
;   ; After each method, verify A20 is enabled:
;   ;   Write 0x00 to 0x000000
;   ;   Write 0xFF to 0x100000 (wraps to 0x000000 if A20 disabled)
;   ;   Compare: if different, A20 is enabled
;   ret
