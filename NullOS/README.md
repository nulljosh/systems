# NullOS

Custom x86 operating system built from scratch in assembly and C. No GRUB, no libraries, no training wheels -- just raw metal.

## Toolchain

| Tool | Purpose | Install |
|------|---------|---------|
| `nasm` | x86 assembler | `brew install nasm` |
| `i686-elf-gcc` | Cross-compiler (32-bit ELF) | `brew tap nativeos/i686-elf-toolchain && brew install i686-elf-gcc` |
| `qemu-system-i386` | x86 emulator | `brew install qemu` |
| `make` | Build automation | Pre-installed on macOS |

> **Important:** Do not use the system `gcc` -- it targets macOS Mach-O, not bare-metal ELF. The `i686-elf-gcc` cross-compiler is required.

## Build & Run

```bash
# Build the OS image
make

# Run in QEMU
make run

# Run with GDB debug stub on :1234
make debug

# Clean build artifacts
make clean
```

## Architecture Overview

### Memory Map

```
0x00000000 ┌──────────────────────────┐
           │ Real Mode IVT            │
0x00000500 ├──────────────────────────┤
           │ BIOS Data Area           │
0x00007C00 ├──────────────────────────┤
           │ Stage 1 Bootloader (512B)│  ← BIOS loads MBR here
0x00007E00 ├──────────────────────────┤
           │ Stage 2 Bootloader       │  ← Stage 1 loads this
0x00010000 ├──────────────────────────┤
           │ Stack (grows down)       │
0x0009FC00 ├──────────────────────────┤
           │ Extended BIOS Data Area  │
0x000B8000 ├──────────────────────────┤
           │ VGA Text Mode Buffer     │  ← 80×25 character display
0x00100000 ├──────────────────────────┤
           │ Kernel (loaded here)     │  ← kmain() entry point
           │ .text, .rodata, .data    │
           │ .bss, heap               │
0x00400000 ├──────────────────────────┤
           │ Free memory              │
           └──────────────────────────┘
```

### Boot Sequence

```mermaid
graph TD
    A[Power On] --> B[BIOS POST]
    B --> C[BIOS loads MBR<br/>to 0x7C00]
    C --> D[Stage 1 Bootloader]
    D --> E[Load Stage 2<br/>from disk via INT 0x13]
    E --> F[Stage 2 Bootloader]
    F --> G[Enable A20 Line]
    G --> H[Set up GDT<br/>null / code 0x08 / data 0x10]
    H --> I[Switch to Protected Mode<br/>CR0 bit 0 = 1]
    I --> J[Far jump to 32-bit code]
    J --> K[Load kernel from disk<br/>to 0x100000]
    K --> L["kmain()"]
    L --> M[Init VGA driver]
    M --> N[Init IDT + ISRs]
    N --> O[Init keyboard driver]
    O --> P[Shell loop]

    style A fill:#333,stroke:#555,color:#fff
    style D fill:#1a1a2e,stroke:#555,color:#fff
    style F fill:#1a1a2e,stroke:#555,color:#fff
    style I fill:#16213e,stroke:#555,color:#fff
    style L fill:#0f3460,stroke:#555,color:#fff
    style P fill:#0f3460,stroke:#555,color:#fff
```

### Design Decisions

- **Flat memory model** -- GDT maps code and data segments across the full 4 GB address space
- **Ring 0 only** initially -- no user-mode separation until Phase 4
- **Custom bootloader** -- no GRUB dependency, full control over the boot process
- **Freestanding C** -- compiled with `-ffreestanding -nostdlib -nostdinc`, no libc

## Realistic Timeline

| Phase | What | Time Estimate |
|-------|------|---------------|
| Phase 1: Bootloader | Two-stage bootloader, GDT, A20 line, protected mode | 1 focused weekend |
| Phase 2: Kernel Basics | VGA text driver, keyboard input, basic shell | 2-3 sessions |
| Phase 3: Memory | Physical/virtual memory manager, paging, heap | 2-3 weeks |
| Phase 4: Multitasking | Process scheduler, context switching, syscalls | 3-4 weeks |
| Phase 5: Filesystem | FAT12/16 driver, VFS layer, disk I/O | 4-6 weeks |

**"Hello World" bootable OS in QEMU: ~1 week of evening sessions**

**Full OS with memory + multitasking + filesystem: months of steady work**

## Project Structure

```
NullOS/
  src/
    boot/
      stage1.asm          # First-stage bootloader (512 bytes, MBR)
      stage2.asm          # Second-stage (real mode → protected mode)
      gdt.asm             # Global Descriptor Table setup
      a20.asm             # A20 line enablement
    kernel/
      main.c              # Kernel entry point (kmain)
      vga.c               # VGA text-mode driver (80×25)
      idt.c               # Interrupt Descriptor Table
      keyboard.c          # PS/2 keyboard driver (IRQ1)
      shell.c             # Basic command shell
      memory.c            # Physical page allocator
      paging.c            # Virtual memory / paging
      heap.c              # kmalloc / kfree
      process.c           # Process management
      scheduler.c         # Round-robin scheduler
    drivers/
      serial.c            # Serial port (COM1) for debug output
      ata.c               # ATA/IDE disk driver
    lib/
      string.c            # string.h implementation
      printf.c            # Minimal printf
  include/                # Header files
  linker.ld               # Linker script (kernel at 0x100000)
  Makefile                # Build system
```

## Resources

- [OSDev Wiki](https://wiki.osdev.org/) -- the OS development bible
- [Intel Software Developer Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [Nick Blundell -- Writing a Simple OS from Scratch](https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf)
- [James Molloy's Kernel Tutorial](http://www.jamesmolloy.co.uk/tutorial_html/)

## License

MIT

## Project Map

```svg
<svg viewBox="0 0 680 420" width="680" height="420" xmlns="http://www.w3.org/2000/svg" style="font-family:monospace;background:#f8fafc;border-radius:12px">
  <!-- Title -->
  <text x="340" y="28" text-anchor="middle" font-size="13" font-weight="bold" fill="#1e293b">NullOS — Custom x86 Operating System</text>

  <!-- Root node -->
  <rect x="270" y="44" width="140" height="34" rx="8" fill="#0071e3"/>
  <text x="340" y="66" text-anchor="middle" font-size="11" fill="white" font-weight="bold">NullOS/</text>

  <!-- Dashed lines from root to level 1 -->
  <line x1="290" y1="78" x2="80" y2="120" stroke="#94a3b8" stroke-width="1.5" stroke-dasharray="4,3"/>
  <line x1="315" y1="78" x2="200" y2="120" stroke="#94a3b8" stroke-width="1.5" stroke-dasharray="4,3"/>
  <line x1="340" y1="78" x2="340" y2="120" stroke="#94a3b8" stroke-width="1.5" stroke-dasharray="4,3"/>
  <line x1="365" y1="78" x2="460" y2="120" stroke="#94a3b8" stroke-width="1.5" stroke-dasharray="4,3"/>
  <line x1="390" y1="78" x2="590" y2="120" stroke="#94a3b8" stroke-width="1.5" stroke-dasharray="4,3"/>

  <!-- src/ folder -->
  <rect x="40" y="120" width="80" height="30" rx="6" fill="#6366f1"/>
  <text x="80" y="138" text-anchor="middle" font-size="11" fill="white">src/</text>

  <!-- include/ folder -->
  <rect x="150" y="120" width="100" height="30" rx="6" fill="#6366f1"/>
  <text x="200" y="138" text-anchor="middle" font-size="11" fill="white">include/</text>

  <!-- build/ folder -->
  <rect x="300" y="120" width="80" height="30" rx="6" fill="#6366f1"/>
  <text x="340" y="138" text-anchor="middle" font-size="11" fill="white">build/</text>

  <!-- docs/ folder -->
  <rect x="420" y="120" width="80" height="30" rx="6" fill="#6366f1"/>
  <text x="460" y="138" text-anchor="middle" font-size="11" fill="white">docs/</text>

  <!-- scripts/ folder -->
  <rect x="550" y="120" width="90" height="30" rx="6" fill="#6366f1"/>
  <text x="595" y="138" text-anchor="middle" font-size="11" fill="white">scripts/</text>

  <!-- src/ children -->
  <line x1="80" y1="150" x2="50" y2="195" stroke="#94a3b8" stroke-width="1"/>
  <line x1="80" y1="150" x2="80" y2="195" stroke="#94a3b8" stroke-width="1"/>
  <line x1="80" y1="150" x2="115" y2="195" stroke="#94a3b8" stroke-width="1"/>

  <rect x="14" y="195" width="72" height="28" rx="5" fill="#e0e7ff" stroke="#818cf8" stroke-width="1.5"/>
  <text x="50" y="211" text-anchor="middle" font-size="10" fill="#3730a3">boot/</text>
  <rect x="46" y="195" width="72" height="28" rx="5" fill="#e0e7ff" stroke="#818cf8" stroke-width="1.5"/>
  <text x="82" y="211" text-anchor="middle" font-size="10" fill="#3730a3">kernel/</text>
  <rect x="90" y="195" width="72" height="28" rx="5" fill="#e0e7ff" stroke="#818cf8" stroke-width="1.5"/>
  <text x="126" y="211" text-anchor="middle" font-size="10" fill="#3730a3">drivers/</text>

  <!-- include/ children -->
  <line x1="200" y1="150" x2="180" y2="195" stroke="#94a3b8" stroke-width="1"/>
  <line x1="200" y1="150" x2="220" y2="195" stroke="#94a3b8" stroke-width="1"/>

  <rect x="148" y="195" width="72" height="28" rx="5" fill="#e0f2fe" stroke="#7dd3fc" stroke-width="1.5"/>
  <text x="184" y="211" text-anchor="middle" font-size="10" fill="#0c4a6e">system.h</text>
  <rect x="222" y="195" width="60" height="28" rx="5" fill="#e0e7ff" stroke="#818cf8" stroke-width="1.5"/>
  <text x="252" y="211" text-anchor="middle" font-size="10" fill="#3730a3">lib/</text>

  <!-- docs/ children -->
  <line x1="460" y1="150" x2="440" y2="195" stroke="#94a3b8" stroke-width="1"/>
  <line x1="460" y1="150" x2="480" y2="195" stroke="#94a3b8" stroke-width="1"/>

  <rect x="400" y="195" width="96" height="28" rx="5" fill="#dcfce7" stroke="#86efac" stroke-width="1.5"/>
  <text x="448" y="211" text-anchor="middle" font-size="10" fill="#166534">ARCHITECTURE</text>
  <rect x="498" y="195" width="72" height="28" rx="5" fill="#dcfce7" stroke="#86efac" stroke-width="1.5"/>
  <text x="534" y="211" text-anchor="middle" font-size="10" fill="#166534">PHASES.md</text>

  <!-- Root-level config files -->
  <line x1="340" y1="78" x2="200" y2="300" stroke="#94a3b8" stroke-width="1" stroke-dasharray="4,3"/>
  <line x1="340" y1="78" x2="340" y2="300" stroke="#94a3b8" stroke-width="1" stroke-dasharray="4,3"/>
  <line x1="340" y1="78" x2="480" y2="300" stroke="#94a3b8" stroke-width="1" stroke-dasharray="4,3"/>

  <rect x="140" y="300" width="100" height="28" rx="5" fill="#e0f2fe" stroke="#7dd3fc" stroke-width="1.5"/>
  <text x="190" y="316" text-anchor="middle" font-size="10" fill="#0c4a6e">Makefile</text>
  <rect x="262" y="300" width="100" height="28" rx="5" fill="#e0f2fe" stroke="#7dd3fc" stroke-width="1.5"/>
  <text x="312" y="316" text-anchor="middle" font-size="10" fill="#0c4a6e">linker.ld</text>
  <rect x="384" y="300" width="100" height="28" rx="5" fill="#dcfce7" stroke="#86efac" stroke-width="1.5"/>
  <text x="434" y="316" text-anchor="middle" font-size="10" fill="#166534">BUILD.md</text>

  <!-- Legend -->
  <rect x="30" y="375" width="12" height="12" rx="2" fill="#0071e3"/>
  <text x="48" y="386" font-size="9" fill="#64748b">Root</text>
  <rect x="90" y="375" width="12" height="12" rx="2" fill="#6366f1"/>
  <text x="108" y="386" font-size="9" fill="#64748b">Folder</text>
  <rect x="158" y="375" width="12" height="12" rx="2" fill="#e0e7ff" stroke="#818cf8" stroke-width="1"/>
  <text x="176" y="386" font-size="9" fill="#64748b">Source</text>
  <rect x="236" y="375" width="12" height="12" rx="2" fill="#e0f2fe" stroke="#7dd3fc" stroke-width="1"/>
  <text x="254" y="386" font-size="9" fill="#64748b">Config</text>
  <rect x="316" y="375" width="12" height="12" rx="2" fill="#dcfce7" stroke="#86efac" stroke-width="1"/>
  <text x="334" y="386" font-size="9" fill="#64748b">Docs</text>
</svg>
```
