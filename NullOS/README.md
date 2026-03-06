# NullOS

Custom 32-bit x86 hobby OS written in C and NASM.

## Verified Status (March 6, 2026)

This is what was verified in this workspace:

- `make` succeeds and produces `build/kernel/kernel.elf`.
- `make image` succeeds and produces `build/nullos.img`.
- `make run` currently fails on this machine because `qemu-system-i386` is not installed.
- Headless serial output support is now implemented in `src/drivers/serial.c`, and `kprintf` mirrors output to COM1 after `serial_init()`.

## Toolchain (macOS)

Required:

- `nasm`
- `make`
- `i686-elf-gcc`, `i686-elf-ld`, `i686-elf-objcopy`
- QEMU (`qemu-system-i386` or `qemu-system-x86_64`)

Install example:

```bash
brew install nasm qemu
brew tap nativeos/i686-elf-toolchain
brew install i686-elf-gcc
```

Verify:

```bash
nasm --version
i686-elf-gcc --version
qemu-system-i386 --version
```

If `qemu-system-i386` is missing but `qemu-system-x86_64` exists, you can usually run with that binary instead.

## Build

```bash
make
```

Outputs:

- Kernel ELF: `build/kernel/kernel.elf`
- Optional floppy image: `build/nullos.img` (via `make image`)

## Run In QEMU

Kernel direct boot (Multiboot):

```bash
qemu-system-i386 -kernel build/kernel/kernel.elf -serial stdio -nographic -m 32M
```

Equivalent project target:

```bash
make run
```

Floppy boot path:

```bash
make run-floppy
```

## What Is Implemented

Boot/runtime path currently in code:

- Multiboot kernel entry (`src/kernel/entry.asm`)
- VGA text console (`src/kernel/vga.c`)
- Interrupt setup: IDT/ISR/IRQ (`src/kernel/idt.c`, `src/kernel/isr.c`, `src/kernel/irq.c`)
- PIT timer (`src/kernel/timer.c`)
- PS/2 keyboard input (`src/kernel/keyboard.c`)
- Basic shell commands (`help`, `clear`, `echo`, `time`, `meminfo`, `reboot`) in `src/kernel/shell.c`
- Physical memory manager bitmap allocator (`src/kernel/memory.c`)
- Paging + page fault handler (`src/kernel/paging.c`)
- Kernel heap allocator (`src/kernel/heap.c`)
- Minimal printf/string support (`src/lib/printf.c`, `src/lib/string.c`)
- Serial COM1 driver for headless logs (`src/drivers/serial.c`)

## Not Implemented Yet (Stubs/TODO)

- Process management (`src/kernel/process.c`)
- Scheduler (`src/kernel/scheduler.c`)
- Syscall layer (`src/kernel/syscall.c`)
- ATA disk driver (`src/drivers/ata.c`)
- PCI enumeration (`src/drivers/pci.c`)

## Known Limitations

- QEMU run was not executable in this environment because no QEMU system binary is installed.
- No user mode or multitasking yet.
- Storage and PCI are placeholders.
- No filesystem yet.

## License

MIT
