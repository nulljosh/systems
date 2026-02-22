# NullOS Build Instructions

## Prerequisites

The cross-compiler toolchain is required to build NullOS on macOS.

### Install Toolchain

```bash
# Install NASM assembler
brew install nasm

# Install i686-elf cross-compiler
# Method 1: Using tap (if available)
brew tap nativeos/i686-elf-toolchain
brew install i686-elf-gcc

# Method 2: Build from source (if tap unavailable)
# See: https://wiki.osdev.org/GCC_Cross-Compiler
# You'll need binutils 2.41+ and GCC 13+
# Target: i686-elf

# Install QEMU emulator
brew install qemu
```

### Verify Installation

```bash
nasm -version
i686-elf-gcc --version
qemu-system-i386 --version
```

## Building

### Full Build (Bootloader + Kernel)

```bash
cd /Users/joshua/Documents/Code/NullOS
make clean
make all
```

This produces `build/nullos.img` -- a 1.44 MB floppy image.

### Run in QEMU

```bash
make run
```

You should see:
```
NullOS Stage 1 Bootloader
Stage 2 loaded, jumping...
=== NullOS Kernel Boot ===
Kernel entry point reached at 0x100000
VGA text mode initialized (80x25)
```

### Debug with GDB

```bash
# Terminal 1: Start QEMU with GDB stub
make debug

# Terminal 2: Attach GDB
gdb build/kernel.elf -ex "target remote :1234"
(gdb) break kmain
(gdb) continue
```

### Build Individual Components

```bash
# Build bootloader only
make boot

# Build kernel only
make kernel

# Clean build artifacts
make clean
```

## Current Implementation Status

**Phase 1: Bootloader**  COMPLETE
-  Stage 1: Loads Stage 2 from disk
-  Stage 2: Enables A20 line, sets up GDT, enters protected mode
-  Jumps to kernel at 0x100000

**Phase 2: Kernel Basics** ⏳ IN PROGRESS
-  VGA text driver (80x25, hardware cursor, scrolling)
-  Basic string library (memory operations, string functions)
-  Kernel entry point (kmain) with initialization output
- ⏳ IDT and interrupt handling
- ⏳ PS/2 keyboard driver
- ⏳ Shell

**Phase 3: Memory Management**  TODO
- Physical memory manager (bitmap allocator)
- Paging and virtual memory
- Heap allocator (kmalloc/kfree)

**Phase 4: Multitasking**  TODO
- Process management
- Round-robin scheduler
- Context switching
- System calls

**Phase 5: Filesystem**  TODO
- ATA/IDE disk driver
- FAT12/16 filesystem
- VFS abstraction layer

## Project Structure

```
NullOS/
  src/
    boot/              # Bootloader (assembly)
    kernel/            # Kernel (C + assembly)
    drivers/           # Hardware drivers
    lib/               # Standard library (freestanding)
  include/             # Header files
  docs/                # Documentation
  Makefile             # Build system
  BUILD.md             # This file
```

## Troubleshooting

### "nasm not found"
Install NASM: `brew install nasm`

### "i686-elf-gcc: command not found"
Install cross-compiler. See Prerequisites above.

### "qemu-system-i386: command not found"
Install QEMU: `brew install qemu`

### Build fails with linker errors
Check that `linker.ld` exists and is correct. Kernel should be linked at 0x100000.

### QEMU shows blank screen
This might indicate:
1. Bootloader didn't jump to kernel properly
2. Kernel entry point (kmain) crashed or halted
3. VGA initialization failed

Use `make debug` with GDB to investigate.

## Resources

- [OSDev Wiki](https://wiki.osdev.org/) - OS development bible
- [Intel Software Developer Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [Nick Blundell - Writing a Simple OS from Scratch](https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf)
