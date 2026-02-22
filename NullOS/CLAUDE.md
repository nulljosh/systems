# NullOS - Development Guide

A custom x86 operating system built from scratch in assembly and C.

## Quick Reference

```bash
# Build the OS image
make

# Run in QEMU
make run

# Run with debug output (serial console)
make debug

# Clean build artifacts
make clean

# Build and run in one step
make all run
```

## Toolchain Requirements

| Tool | Purpose | Install (macOS) |
|------|---------|-----------------|
| `nasm` | x86 assembler | `brew install nasm` |
| `i686-elf-gcc` | Cross-compiler (32-bit ELF) | `brew install i686-elf-gcc` |
| `i686-elf-ld` | Cross-linker | Included with gcc |
| `qemu-system-i386` | x86 emulator | `brew install qemu` |
| `make` | Build automation | Pre-installed on macOS |

### Installing the Cross-Compiler

The cross-compiler is critical. Do NOT use the system `gcc` -- it targets macOS Mach-O, not bare-metal ELF.

```bash
brew tap nativeos/i686-elf-toolchain
brew install i686-elf-gcc
```

If the tap is unavailable, build from source:
```bash
# See https://wiki.osdev.org/GCC_Cross-Compiler
# Target: i686-elf
# Binutils 2.41+, GCC 13+
```

## Architecture

- **Target**: i686 (32-bit x86, protected mode)
- **Boot**: Custom two-stage bootloader (no GRUB)
- **Memory model**: Flat memory model with paging
- **Output**: `build/nullos.img` -- raw floppy disk image (1.44 MB)

## Project Structure

```
NullOS/
  src/
    boot/
      stage1.asm        # First-stage bootloader (512 bytes, MBR)
      stage2.asm        # Second-stage (real mode -> protected mode)
      gdt.asm           # Global Descriptor Table setup
      a20.asm           # A20 line enablement
    kernel/
      main.c            # Kernel entry point (kmain)
      vga.c             # VGA text-mode driver (80x25)
      idt.c             # Interrupt Descriptor Table
      isr.c             # Interrupt Service Routines
      irq.c             # Hardware interrupt handlers
      timer.c           # PIT timer (IRQ0)
      keyboard.c        # PS/2 keyboard driver (IRQ1)
      shell.c           # Basic command shell
      memory.c          # Physical page allocator
      paging.c          # Virtual memory / paging
      heap.c            # kmalloc / kfree
      process.c         # Process management
      scheduler.c       # Round-robin scheduler
      syscall.c         # System call interface
    drivers/
      serial.c          # Serial port (COM1) for debug output
      pci.c             # PCI bus enumeration
      ata.c             # ATA/IDE disk driver
    lib/
      string.c          # string.h implementation (memcpy, strlen, etc.)
      printf.c          # Minimal printf implementation
  include/
    kernel/
      vga.h
      idt.h
      isr.h
      irq.h
      timer.h
      keyboard.h
      shell.h
      memory.h
      paging.h
      heap.h
      process.h
      scheduler.h
      syscall.h
    drivers/
      serial.h
      pci.h
      ata.h
    lib/
      string.h
      stdint.h
      stddef.h
      stdarg.h
      printf.h
    system.h            # Master include (types, constants)
  scripts/
    build.sh            # Full build pipeline
    run.sh              # Launch QEMU
    debug.sh            # Launch QEMU with GDB stub
  docs/
    PHASES.md           # Phase-by-phase development plan
    ARCHITECTURE.md     # Memory map, boot sequence, component overview
  linker.ld             # Linker script (kernel loaded at 0x100000)
  Makefile              # Build system
  README.md             # Project overview
  roadmap.svg           # Visual roadmap
```

## Build System

The Makefile handles a two-stage build:

1. **Bootloader** -- NASM assembles `stage1.asm` and `stage2.asm` into raw binaries
2. **Kernel** -- `i686-elf-gcc` compiles all C files, links with `linker.ld`, produces ELF then flat binary
3. **Image** -- `dd` concatenates bootloader + kernel into `build/nullos.img`

Key Makefile targets:

| Target | Description |
|--------|-------------|
| `make` | Build `build/nullos.img` |
| `make run` | Build + launch QEMU |
| `make debug` | Build + launch QEMU with GDB stub on :1234 |
| `make clean` | Remove `build/` directory |
| `make boot` | Build bootloader only |
| `make kernel` | Build kernel only |

## Phase-by-Phase Implementation Notes

### Phase 1: Bootloader

Focus files: `src/boot/stage1.asm`, `src/boot/stage2.asm`, `src/boot/gdt.asm`, `src/boot/a20.asm`

The bootloader is pure NASM assembly. No C, no linker script needed yet.

**Stage 1** (exactly 512 bytes):
- BIOS loads this from the first sector of the disk to `0x7C00`
- Sets up segment registers and stack
- Loads Stage 2 from disk sectors 2+ using BIOS `int 0x13`
- Jumps to Stage 2
- Must end with `0x55AA` boot signature

**Stage 2**:
- Still in 16-bit real mode
- Enables the A20 line (keyboard controller method + fast A20 fallback)
- Sets up the GDT with three segments: null, code (0x08), data (0x10)
- Switches to 32-bit protected mode (`cr0 |= 1`)
- Jumps to the kernel entry point

**Testing Phase 1**:
```bash
make boot
qemu-system-i386 -fda build/boot.img
# Should see "NullOS" or a boot message in QEMU
```

### Phase 2: Kernel Basics

Focus files: `src/kernel/main.c`, `src/kernel/vga.c`, `src/kernel/idt.c`, `src/kernel/keyboard.c`

The kernel is compiled as freestanding C (`-ffreestanding -nostdlib`). No libc.

**Entry point**: `kmain()` in `main.c`, called from Stage 2's protected-mode jump.

Key rules for freestanding C:
- No `#include <stdio.h>` -- we write our own everything
- No `malloc`, no `printf` -- until we implement them
- Must provide `__stack_chk_fail` stub if using `-fstack-protector`
- Compile with: `-m32 -ffreestanding -fno-builtin -nostdlib -nostdinc -Wall -Wextra -O2`

### Phase 3: Memory Management

Focus files: `src/kernel/memory.c`, `src/kernel/paging.c`, `src/kernel/heap.c`

Physical allocator uses a bitmap (1 bit per 4KB page). Paging maps virtual to physical addresses. Heap provides `kmalloc()`/`kfree()` on top of paging.

### Phase 4: Process Management

Focus files: `src/kernel/process.c`, `src/kernel/scheduler.c`, `src/kernel/syscall.c`

Each process gets its own page directory, kernel stack, and saved register state. Context switch saves/restores ESP, EBP, EIP, and segment registers. Scheduler is round-robin with a linked list of process control blocks.

### Phase 5: File System

Focus files: `src/drivers/ata.c`, plus new VFS and FS implementation files.

Start with reading raw sectors via ATA PIO. Build a VFS abstraction layer, then implement a simple filesystem (FAT12 is the easiest real-world option for a floppy image).

## Debugging

### Serial output (recommended)
```c
// In kernel code:
serial_write("Debug: reached kmain\n");
```
QEMU flags: `-serial stdio` pipes COM1 to your terminal.

### GDB
```bash
make debug
# In another terminal:
gdb build/kernel.elf -ex "target remote :1234"
```

### QEMU Monitor
Press `Ctrl+Alt+2` in QEMU window for the monitor console:
```
info registers    # Dump CPU registers
info mem          # Memory mappings
xp /16x 0x7c00   # Examine memory at address
```

## Code Style

- C: K&R style, 4-space indent, `snake_case` for functions/variables, `UPPER_CASE` for constants
- ASM: NASM Intel syntax, labels with `_` prefix for exported symbols, `.` prefix for local labels
- Every function gets a one-line comment explaining its purpose
- Header files use include guards: `#ifndef _KERNEL_VGA_H` / `#define _KERNEL_VGA_H`

## Key References

- [OSDev Wiki](https://wiki.osdev.org/) -- the OS development bible
- [Intel 64 and IA-32 Architectures Software Developer Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [James Molloy's Kernel Tutorial](http://www.jamesmolloy.co.uk/tutorial_html/)
- [Bran's Kernel Development Tutorial](http://www.osdever.net/bkerndev/Docs/title.htm)
- [Nick Blundell - Writing a Simple OS from Scratch](https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf)
