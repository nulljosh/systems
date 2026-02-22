# NullOS Architecture

Technical reference for the system architecture, memory layout, boot sequence, and component interactions.

---

## Memory Map

### Real Mode Memory Layout (0x00000 - 0xFFFFF)

The first 1 MB of memory is structured by the BIOS and x86 architecture:

```
 Address Range          Size       Description
 ---------------------------------------------------------------
 0x00000 - 0x003FF      1 KB      Interrupt Vector Table (IVT)
 0x00400 - 0x004FF      256 B     BIOS Data Area (BDA)
 0x00500 - 0x07BFF      ~30 KB    Free (usable by bootloader)
 0x07C00 - 0x07DFF      512 B     Stage 1 Bootloader (loaded by BIOS)
 0x07E00 - 0x0FFFF      ~33 KB    Free (Stage 2 loaded here)
 0x10000 - 0x7FFFF      448 KB    Free (conventional memory)
 0x80000 - 0x9FFFF      128 KB    Extended BIOS Data Area (varies)
 0xA0000 - 0xBFFFF      128 KB    VGA Frame Buffer
   0xA0000 - 0xAFFFF                VGA Graphics Mode
   0xB0000 - 0xB7FFF                Monochrome Text Mode
   0xB8000 - 0xBFFFF                Color Text Mode (we use this)
 0xC0000 - 0xC7FFF      32 KB     VGA BIOS ROM
 0xC8000 - 0xEFFFF      160 KB    Mapped hardware / Option ROMs
 0xF0000 - 0xFFFFF      64 KB     System BIOS ROM
```

### Protected Mode Memory Layout (full 4 GB address space)

After switching to protected mode with paging enabled:

```
 Virtual Address          Maps To            Description
 ---------------------------------------------------------------
 0x00000000 - 0x000FFFFF  Identity mapped    Real mode area (IVT, BIOS, VGA)
 0x00100000 - 0x002FFFFF  Identity mapped    Kernel code + data (~2 MB)
 0x00300000 - 0x003FFFFF  Identity mapped    Kernel page tables
 0x00400000 - 0x00FFFFFF  Identity mapped    Kernel stacks, buffers

 0x01000000 - 0xBFFFFFFF  Allocated on demand  User-space processes

 0xC0000000 - 0xCFFFFFFF  Dynamic            Kernel heap (kmalloc)
 0xD0000000 - 0xDFFFFFFF  Reserved            Memory-mapped I/O (future)
 0xE0000000 - 0xEFFFFFFF  Reserved            Reserved for expansion
 0xF0000000 - 0xFFFFFFFF  Reserved            Recursive page table mapping
```

### Physical Memory Regions

Physical memory detected via BIOS `int 0x15, EAX=0xE820`:

```
 Region                     Description            Status
 ---------------------------------------------------------------
 0x00000000 - 0x0009FFFF    Conventional memory    Usable (640 KB)
 0x000A0000 - 0x000FFFFF    VGA + BIOS ROM         Reserved
 0x00100000 - 0x01FFFFFF    Extended memory         Usable (31 MB, typical QEMU)
 0xFEC00000 - 0xFECFFFFF    APIC MMIO              Reserved
 0xFEE00000 - 0xFEEFFFFF    Local APIC MMIO        Reserved
```

---

## Boot Sequence

```
 Power On
    |
    v
 +------------------+
 |  BIOS POST       |  Hardware self-test, device enumeration
 +------------------+
    |
    v
 +------------------+
 |  BIOS loads MBR  |  Reads sector 0 (512 bytes) to 0x7C00
 +------------------+
    |
    v
 +------------------+
 |  Stage 1         |  src/boot/stage1.asm
 |  (Boot Sector)   |
 |                  |  1. Set up segments (DS=ES=SS=0)
 |                  |  2. Set stack (SP=0x7C00)
 |                  |  3. Load Stage 2 via BIOS int 0x13
 |                  |  4. Jump to Stage 2
 +------------------+
    |
    v
 +------------------+
 |  Stage 2         |  src/boot/stage2.asm
 |                  |
 |                  |  1. Detect memory map (int 0x15, E820)
 |                  |  2. Enable A20 line
 |                  |  3. Load GDT (null + code + data segments)
 |                  |  4. Set CR0 bit 0 = 1 (enter protected mode)
 |                  |  5. Far jump to 32-bit code (flush pipeline)
 |                  |  6. Set segment registers to data selector
 |                  |  7. Set up protected-mode stack
 |                  |  8. Jump to kernel at 0x100000
 +------------------+
    |
    v
 +------------------+
 |  Kernel Entry    |  src/kernel/main.c -> kmain()
 |                  |
 |                  |  1. Initialize VGA (clear screen, cursor)
 |                  |  2. Print boot message
 |                  |  3. Set up IDT (256 entries)
 |                  |  4. Remap PIC (IRQs -> interrupts 32-47)
 |                  |  5. Register ISR handlers (exceptions 0-31)
 |                  |  6. Register IRQ handlers (timer, keyboard)
 |                  |  7. Configure PIT timer (100 Hz)
 |                  |  8. Enable interrupts (sti)
 |                  |  9. Initialize memory manager
 |                  | 10. Initialize scheduler
 |                  | 11. Launch shell
 +------------------+
    |
    v
 +------------------+
 |  Shell           |  src/kernel/shell.c
 |  (main loop)     |
 |                  |  while(1) {
 |                  |      read keyboard input
 |                  |      parse command
 |                  |      execute command
 |                  |  }
 +------------------+
```

### CPU Mode Transitions

```
 16-bit Real Mode          32-bit Protected Mode         User Mode
 (BIOS, Stage 1-2)        (Kernel, Ring 0)              (Processes, Ring 3)
     |                         |                             |
     |  CR0.PE = 1             |  iret to Ring 3             |
     |  + far jump             |  + TSS setup                |
     +------------------------>+----------------------------->|
                               |                             |
                               |<------- int 0x80 -----------|
                               |     (system call)           |
                               |                             |
                               |<------- IRQ (timer) --------|
                               |  (preemptive switch)        |
```

---

## Component Interaction

### Interrupt Flow

```
 Hardware Event (e.g., key press)
    |
    v
 +--------+
 |  PIC   |  8259 Programmable Interrupt Controller
 |        |  Translates IRQ line -> interrupt number
 +--------+
    |
    v
 +--------+
 |  CPU   |  Looks up interrupt number in IDT
 |        |  Pushes EFLAGS, CS, EIP onto stack
 |        |  Jumps to ISR handler address
 +--------+
    |
    v
 +------------------+
 |  ISR Stub (asm)  |  Saves all registers (pusha)
 |                  |  Pushes interrupt number
 |                  |  Calls C handler
 +------------------+
    |
    v
 +------------------+
 |  C Handler       |  e.g., keyboard_handler()
 |                  |  Reads scan code from port 0x60
 |                  |  Converts to ASCII
 |                  |  Pushes to input buffer
 +------------------+
    |
    v
 +------------------+
 |  ISR Stub (asm)  |  Sends EOI to PIC
 |                  |  Restores registers (popa)
 |                  |  Returns (iret)
 +------------------+
```

### Memory Management Stack

```
 +----------------------------------+
 |  kmalloc(size) / kfree(ptr)     |  Kernel Heap API
 |  (Phase 3.3)                     |  Returns arbitrary-sized blocks
 +----------------------------------+
                |
                v
 +----------------------------------+
 |  map_page(virt, phys, flags)     |  Paging Layer
 |  unmap_page(virt)                |  Manages page tables + CR3
 |  (Phase 3.2)                     |  4 KB granularity
 +----------------------------------+
                |
                v
 +----------------------------------+
 |  pmm_alloc_frame()               |  Physical Memory Manager
 |  pmm_free_frame(addr)            |  Bitmap allocator
 |  (Phase 3.1)                     |  Tracks all physical 4KB frames
 +----------------------------------+
                |
                v
 +----------------------------------+
 |  Physical RAM                    |  Detected via E820 at boot
 +----------------------------------+
```

### Process Scheduling

```
 Timer IRQ (every 10ms)
    |
    v
 +--------------------+
 |  timer_handler()   |  Increments global tick counter
 |                    |  Decrements current process time slice
 +--------------------+
    |
    | (time slice expired)
    v
 +--------------------+
 |  schedule()        |  Picks next READY process
 |                    |  Moves current to back of queue
 +--------------------+
    |
    v
 +--------------------+
 |  context_switch()  |  Save ESP/EBP/EIP of current process
 |  (assembly)        |  Load ESP/EBP/EIP of next process
 |                    |  Switch CR3 if different address space
 |                    |  iret -> resumes next process
 +--------------------+


 Process States:

   READY <----------+
     |              |
     | (scheduled)  | (preempted / yield)
     v              |
   RUNNING ---------+
     |
     | (waiting for I/O, sleep)
     v
   BLOCKED
     |
     | (I/O complete, wakeup)
     v
   READY
     |
     | (exit syscall)
     v
   TERMINATED
```

### File System Stack

```
 +----------------------------------+
 |  Shell Commands                  |  ls, cat, write, mkdir
 |  User Applications               |  sys_read(), sys_write()
 +----------------------------------+
                |
                v
 +----------------------------------+
 |  VFS (Virtual File System)       |  Unified API: vfs_read/write/open
 |                                  |  Dispatches to correct FS driver
 +----------------------------------+
                |
                v
 +----------------------------------+
 |  FAT12 Driver                    |  Parses FAT, directory entries
 |                                  |  Follows cluster chains
 +----------------------------------+
                |
                v
 +----------------------------------+
 |  ATA PIO Driver                  |  Reads/writes 512-byte sectors
 |                                  |  Talks to disk via I/O ports
 +----------------------------------+
                |
                v
 +----------------------------------+
 |  Physical Disk                   |  1.44 MB floppy image
 +----------------------------------+
```

---

## I/O Port Map

Key x86 I/O ports used by NullOS:

| Port(s) | Device | Usage |
|----------|--------|-------|
| `0x20-0x21` | Master PIC | Command / Data |
| `0x40-0x43` | PIT Timer | Channel 0-2 / Command |
| `0x60` | PS/2 Controller | Data (keyboard scancodes) |
| `0x64` | PS/2 Controller | Status / Command |
| `0x92` | System Control | Fast A20 gate |
| `0xA0-0xA1` | Slave PIC | Command / Data |
| `0x1F0-0x1F7` | Primary ATA | Disk I/O |
| `0x3D4-0x3D5` | VGA CRTC | Cursor position |
| `0x3F8-0x3FD` | COM1 Serial | Debug output |

---

## Privilege Levels

```
 Ring 0 (Kernel Mode)
 +------------------------------------------+
 |  Full hardware access                    |
 |  All instructions allowed (in, out, hlt) |
 |  Kernel code, drivers, interrupt handlers|
 +------------------------------------------+

 Ring 3 (User Mode)
 +------------------------------------------+
 |  Restricted hardware access              |
 |  Privileged instructions cause #GP fault |
 |  User processes, shell (future)          |
 |  Must use syscalls for kernel services   |
 +------------------------------------------+

 Transition mechanisms:
   Ring 3 -> Ring 0:  int 0x80 (syscall), hardware interrupt
   Ring 0 -> Ring 3:  iret with Ring 3 selectors on stack
```

---

## GDT Layout

| Index | Selector | Description | Base | Limit | DPL |
|-------|----------|-------------|------|-------|-----|
| 0 | `0x00` | Null descriptor | - | - | - |
| 1 | `0x08` | Kernel code | 0x0 | 4 GB | 0 |
| 2 | `0x10` | Kernel data | 0x0 | 4 GB | 0 |
| 3 | `0x18` | User code | 0x0 | 4 GB | 3 |
| 4 | `0x20` | User data | 0x0 | 4 GB | 3 |
| 5 | `0x28` | TSS | dynamic | 104 B | 0 |

---

## IDT Layout

| Range | Type | Description |
|-------|------|-------------|
| 0-31 | Exception | CPU exceptions (divide error, page fault, etc.) |
| 32-47 | IRQ | Hardware interrupts (timer, keyboard, disk, etc.) |
| 48-127 | Unused | Reserved for future use |
| 128 (0x80) | Syscall | Software interrupt for system calls |
| 129-255 | Unused | Reserved |
