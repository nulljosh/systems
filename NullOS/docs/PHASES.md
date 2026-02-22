# NullOS Development Phases

A detailed breakdown of each development phase, including technical requirements, implementation details, estimated scope, and learning resources.

---

## Phase 1: Bootloader

**Goal**: Write a custom two-stage bootloader that transitions the CPU from 16-bit real mode to 32-bit protected mode and loads the kernel.

**Files**: `src/boot/stage1.asm`, `src/boot/stage2.asm`, `src/boot/gdt.asm`, `src/boot/a20.asm`

**Estimated LOC**: 300-400 lines of assembly

### 1.1 Stage 1 -- Boot Sector

The BIOS loads exactly 512 bytes from the first sector of the boot device to memory address `0x7C00` and jumps to it. This is our Stage 1.

**Requirements**:
- Must be exactly 512 bytes
- Must end with the magic number `0x55AA` at bytes 510-511
- Must fit all logic in 510 bytes (2 bytes reserved for signature)

**Implementation**:
1. Set up segment registers (`DS`, `ES`, `SS` = 0) and stack pointer (`SP` = `0x7C00`, grows downward)
2. Use BIOS interrupt `int 0x13` (function `0x02`) to read additional sectors from disk
3. Load Stage 2 from sectors 2-N to a known address (e.g., `0x7E00` or `0x1000`)
4. Jump to Stage 2

**BIOS Disk Read** (`int 0x13`, `AH=0x02`):
```
AH = 0x02       ; Read sectors
AL = count       ; Number of sectors to read
CH = cylinder    ; Cylinder number
CL = sector      ; Sector number (1-based)
DH = head        ; Head number
DL = drive       ; Drive number (0x00=floppy, 0x80=HDD)
ES:BX = buffer   ; Destination address
```

**Key constraint**: The boot sector has no room for error handling or fancy features. Keep it minimal -- load Stage 2 and jump.

### 1.2 Stage 2 -- Setup and Mode Switch

Stage 2 has more space (loaded from multiple sectors) and handles the complex transition work.

**Implementation steps**:

1. **Enable A20 Line** (`a20.asm`)
   - The A20 address line is disabled by default for 8086 compatibility
   - Without it, memory above 1 MB wraps around to 0
   - Methods (try in order):
     - **BIOS method**: `int 0x15`, `AX=0x2401`
     - **Keyboard controller method**: Send `0xD1` (write) then `0xDF` (enable) to port `0x64`/`0x60`
     - **Fast A20**: Write bit 1 to port `0x92` (may not work on all hardware)
   - Verify: Write to `0x000000`, check if `0x100000` is different

2. **Set Up GDT** (`gdt.asm`)
   - The GDT defines memory segments for protected mode
   - Minimum three entries:
     - **Null descriptor** (index 0): Required, all zeros
     - **Code segment** (index 1, selector `0x08`): Base=0, Limit=4GB, Execute/Read
     - **Data segment** (index 2, selector `0x10`): Base=0, Limit=4GB, Read/Write
   - This creates a "flat memory model" -- segments cover all 4 GB, paging handles real protection later
   - Load with `lgdt [gdt_descriptor]`

   **GDT Entry Format** (8 bytes each):
   ```
   Bits 0-15:   Limit (low)
   Bits 16-31:  Base (low)
   Bits 32-39:  Base (middle)
   Bits 40-47:  Access byte
   Bits 48-51:  Limit (high)
   Bits 52-55:  Flags
   Bits 56-63:  Base (high)
   ```

   **Access Byte**:
   ```
   Bit 7:    Present (1)
   Bits 5-6: Privilege level (00 = Ring 0)
   Bit 4:    Descriptor type (1 = code/data)
   Bit 3:    Executable (1 = code, 0 = data)
   Bit 2:    Direction/Conforming
   Bit 1:    Read/Write
   Bit 0:    Accessed (set by CPU)
   ```

3. **Switch to Protected Mode**
   - Disable interrupts (`cli`)
   - Set bit 0 of `CR0` (`or eax, 1` / `mov cr0, eax`)
   - Far jump to flush the pipeline: `jmp 0x08:protected_mode_entry`
   - In 32-bit code: set `DS`, `ES`, `FS`, `GS`, `SS` to `0x10` (data segment)
   - Set up a new stack in high memory
   - Jump to kernel entry point at `0x100000`

### 1.3 Testing

```bash
make boot
qemu-system-i386 -fda build/boot.bin
```

**Success criteria**:
- QEMU boots without "No bootable device" error
- Stage 1 loads Stage 2 (verify with debug print via BIOS `int 0x10`)
- A20 is enabled (test with memory wrap check)
- CPU enters protected mode (32-bit registers available)
- Jumps to kernel address without triple-fault

### 1.4 References

- [OSDev: Boot Sequence](https://wiki.osdev.org/Boot_Sequence)
- [OSDev: A20 Line](https://wiki.osdev.org/A20_Line)
- [OSDev: GDT](https://wiki.osdev.org/GDT)
- [OSDev: Protected Mode](https://wiki.osdev.org/Protected_Mode)
- Nick Blundell, "Writing a Simple OS from Scratch", Chapters 1-3
- Intel SDM, Volume 3A, Chapter 9 (Processor Management and Initialization)

---

## Phase 2: Kernel Basics

**Goal**: Build the kernel foundation -- screen output, interrupt handling, keyboard input, and a minimal shell.

**Files**: `src/kernel/main.c`, `src/kernel/vga.c`, `src/kernel/idt.c`, `src/kernel/isr.c`, `src/kernel/irq.c`, `src/kernel/timer.c`, `src/kernel/keyboard.c`, `src/kernel/shell.c`, `src/lib/string.c`, `src/lib/printf.c`

**Estimated LOC**: 1200-1600 lines of C + 200 lines of assembly (ISR stubs)

### 2.1 VGA Text Mode Driver

The VGA text buffer is memory-mapped at `0xB8000`. Each character cell is 2 bytes:
- Byte 0: ASCII character
- Byte 1: Attribute (foreground color [0:3] + background color [4:6] + blink [7])

**Functions to implement**:
```c
void vga_init(void);                        // Clear screen, set cursor to (0,0)
void vga_putchar(char c);                   // Print one character, handle \n, scrolling
void vga_puts(const char *str);             // Print a string
void vga_set_color(uint8_t fg, uint8_t bg); // Set text colors
void vga_clear(void);                       // Clear the screen
void vga_set_cursor(int row, int col);      // Move hardware cursor
```

**Color codes** (4-bit):
```
0x0 Black     0x8 Dark Gray
0x1 Blue      0x9 Light Blue
0x2 Green     0xA Light Green
0x3 Cyan      0xB Light Cyan
0x4 Red       0xC Light Red
0x5 Magenta   0xD Light Magenta
0x6 Brown     0xE Yellow
0x7 Light Gray 0xF White
```

**Scrolling**: When the cursor reaches row 25, `memmove` all rows up by one and clear the last row.

**Hardware cursor**: Write to VGA I/O ports `0x3D4`/`0x3D5` to move the blinking cursor.

### 2.2 Interrupt Descriptor Table (IDT)

The IDT maps interrupt numbers (0-255) to handler functions. Similar structure to the GDT but for interrupts.

**IDT Entry** (8 bytes):
```
Bits 0-15:   Handler offset (low)
Bits 16-31:  Code segment selector (0x08)
Bits 32-39:  Zero
Bits 40-47:  Type/Attributes (0x8E = 32-bit interrupt gate, Ring 0)
Bits 48-63:  Handler offset (high)
```

**Implementation**:
1. Create a 256-entry IDT array
2. Write ISR stub functions in assembly (save registers, call C handler, restore, `iret`)
3. Load with `lidt [idt_descriptor]`
4. Re-enable interrupts (`sti`)

### 2.3 Exception Handlers (ISRs 0-31)

The CPU reserves interrupts 0-31 for exceptions:

| # | Exception | Description |
|---|-----------|-------------|
| 0 | Division Error | `div` by zero |
| 6 | Invalid Opcode | Undefined instruction |
| 8 | Double Fault | Exception during exception handling |
| 13 | General Protection Fault | Segment violation, privilege error |
| 14 | Page Fault | Access to unmapped page (critical for Phase 3) |

Each ISR stub must:
1. Push a dummy error code (if the CPU doesn't push one)
2. Push the interrupt number
3. Save all general-purpose registers (`pusha`)
4. Call the C handler: `void isr_handler(registers_t *regs)`
5. Restore registers (`popa`)
6. Clean up the stack (`add esp, 8`)
7. Return from interrupt (`iret`)

### 2.4 Hardware Interrupts (IRQs)

The 8259 PIC (Programmable Interrupt Controller) maps hardware IRQs to interrupt numbers. By default, IRQs 0-7 map to interrupts 8-15 (which conflicts with CPU exceptions). We must remap them.

**PIC Remapping**: Send ICW1-ICW4 initialization words to master PIC (`0x20`/`0x21`) and slave PIC (`0xA0`/`0xA1`):
- Remap IRQ 0-7 to interrupts 32-39
- Remap IRQ 8-15 to interrupts 40-47

**Key IRQs**:
| IRQ | Interrupt | Device |
|-----|-----------|--------|
| 0 | 32 | PIT Timer |
| 1 | 33 | Keyboard |
| 12 | 44 | PS/2 Mouse |
| 14 | 46 | Primary ATA |

Each IRQ handler must send an EOI (End of Interrupt) to the PIC after handling:
- `outb(0x20, 0x20)` for master PIC
- `outb(0xA0, 0x20)` for slave PIC (IRQs 8-15)

### 2.5 PIT Timer (IRQ 0)

The Programmable Interval Timer generates periodic interrupts. Configure it for ~100 Hz (tick every 10ms):

```c
// Channel 0, lobyte/hibyte, rate generator
outb(0x43, 0x36);
uint16_t divisor = 1193180 / 100;  // 1.193 MHz base frequency
outb(0x40, divisor & 0xFF);        // Low byte
outb(0x40, (divisor >> 8) & 0xFF); // High byte
```

Use the timer tick to maintain a global `ticks` counter for scheduling (Phase 4) and `sleep()`.

### 2.6 Keyboard Driver (IRQ 1)

Read scancodes from port `0x60`. Convert scan codes to ASCII using a lookup table.

**Scan code set 1** (make codes for common keys):
```
0x1E = 'a', 0x30 = 'b', 0x2E = 'c', ...
0x1C = Enter, 0x0E = Backspace, 0x39 = Space
0x2A = Left Shift (press), 0xAA = Left Shift (release)
```

Handle:
- Key press (make code, bit 7 = 0) and release (break code, bit 7 = 1)
- Shift state for uppercase and symbols
- Buffered input: store characters in a ring buffer, read from shell

### 2.7 Shell

A minimal command-line shell that reads keyboard input and executes commands.

**Built-in commands**:
- `help` -- List available commands
- `clear` -- Clear screen
- `echo <text>` -- Print text
- `time` -- Show uptime (from PIT ticks)
- `reboot` -- Triple-fault to reboot
- `meminfo` -- Show memory info (placeholder until Phase 3)

**Prompt**: `null> `

### 2.8 References

- [OSDev: VGA Text Mode](https://wiki.osdev.org/Text_UI)
- [OSDev: IDT](https://wiki.osdev.org/IDT)
- [OSDev: PIC](https://wiki.osdev.org/PIC)
- [OSDev: PS/2 Keyboard](https://wiki.osdev.org/PS/2_Keyboard)
- James Molloy's Kernel Tutorial, Parts 1-6
- Bran's Kernel Development Tutorial

---

## Phase 3: Memory Management

**Goal**: Implement physical memory allocation, virtual memory via paging, and a kernel heap allocator.

**Files**: `src/kernel/memory.c`, `src/kernel/paging.c`, `src/kernel/heap.c`

**Estimated LOC**: 800-1200 lines of C

### 3.1 Physical Page Allocator

Manages all physical RAM in 4 KB pages using a bitmap.

**Detection**: Use BIOS `int 0x15`, `EAX=0xE820` (called in Stage 2 before entering protected mode) to get a memory map. Pass it to the kernel via a known memory address.

**Bitmap allocator**:
- 1 bit per 4 KB page. For 128 MB RAM, that is 32,768 pages = 4 KB bitmap
- `pmm_alloc()` -- scan bitmap for first free bit, mark it used, return physical address
- `pmm_free(addr)` -- clear the bit for the given page
- Mark kernel pages, BIOS area, and VGA memory as used during init

**Data structures**:
```c
static uint32_t *frame_bitmap;     // Bitmap array
static uint32_t total_frames;      // Total 4KB frames
static uint32_t used_frames;       // Currently allocated frames

uint32_t pmm_alloc_frame(void);    // Returns physical address of free frame
void pmm_free_frame(uint32_t addr);
```

### 3.2 Paging

Map virtual addresses to physical addresses using the x86 two-level page table structure.

**x86 Paging**:
- `CR3` register points to the Page Directory (1024 entries, each 4 bytes)
- Each Page Directory Entry (PDE) points to a Page Table (1024 entries)
- Each Page Table Entry (PTE) points to a 4 KB physical page
- Virtual address breakdown: `[Directory:10 bits][Table:10 bits][Offset:12 bits]`
- Total addressable: 1024 x 1024 x 4096 = 4 GB

**PDE/PTE Flags** (lower 12 bits):
```
Bit 0: Present
Bit 1: Read/Write (0 = read-only, 1 = read/write)
Bit 2: User/Supervisor (0 = kernel only, 1 = user accessible)
Bit 3: Write-Through
Bit 4: Cache Disable
Bit 5: Accessed
Bit 6: Dirty (PTE only)
Bit 7: Page Size (PDE only, 0 = 4KB pages)
```

**Implementation**:
1. Create a kernel page directory and identity-map the first 4 MB (kernel + VGA)
2. Set `CR3` to the page directory's physical address
3. Set bit 31 of `CR0` to enable paging
4. Implement `map_page(virtual, physical, flags)` and `unmap_page(virtual)`
5. Handle Page Fault (ISR 14) -- the error code and `CR2` register tell you what happened

**Page Fault Error Code**:
```
Bit 0: Present (0 = not present, 1 = protection violation)
Bit 1: Write (0 = read, 1 = write)
Bit 2: User (0 = kernel mode, 1 = user mode)
```

### 3.3 Kernel Heap (kmalloc / kfree)

Build a dynamic memory allocator on top of paging. A simple approach is a linked-list first-fit allocator.

**Heap region**: Reserve a virtual address range (e.g., `0xC0000000` - `0xCFFFFFFF`) for the kernel heap. Allocate physical pages on demand.

**Block header**:
```c
typedef struct heap_block {
    uint32_t size;              // Size of the data area (not including header)
    uint8_t  is_free;           // 1 if free, 0 if allocated
    struct heap_block *next;    // Next block in the list
} heap_block_t;
```

**Functions**:
```c
void  heap_init(uint32_t start, uint32_t size);
void *kmalloc(uint32_t size);           // Allocate size bytes
void *kmalloc_aligned(uint32_t size);   // Allocate page-aligned
void  kfree(void *ptr);                // Free allocation
```

**Splitting and coalescing**: When allocating, split large free blocks. When freeing, merge adjacent free blocks to prevent fragmentation.

### 3.4 References

- [OSDev: Memory Map (x86)](https://wiki.osdev.org/Detecting_Memory_(x86))
- [OSDev: Page Frame Allocation](https://wiki.osdev.org/Page_Frame_Allocation)
- [OSDev: Paging](https://wiki.osdev.org/Paging)
- [OSDev: Writing a Memory Manager](https://wiki.osdev.org/Writing_a_memory_manager)
- James Molloy's Tutorial, Parts 7-8 (Paging, Heap)
- Intel SDM, Volume 3A, Chapter 4 (Paging)

---

## Phase 4: Process Management

**Goal**: Implement processes with independent address spaces, context switching, and a preemptive round-robin scheduler.

**Files**: `src/kernel/process.c`, `src/kernel/scheduler.c`, `src/kernel/syscall.c`

**Estimated LOC**: 600-900 lines of C + 100 lines of assembly

### 4.1 Process Control Block (PCB)

Each process needs:

```c
typedef struct process {
    uint32_t pid;               // Process ID
    uint32_t esp;               // Saved stack pointer
    uint32_t ebp;               // Saved base pointer
    uint32_t eip;               // Saved instruction pointer
    uint32_t *page_directory;   // Process's own page directory
    process_state_t state;      // RUNNING, READY, BLOCKED, TERMINATED
    uint32_t kernel_stack;      // Top of this process's kernel stack
    struct process *next;       // Linked list for scheduler queue
} process_t;

typedef enum {
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;
```

### 4.2 Context Switching

The context switch saves one process's registers and restores another's. This happens in assembly because we need precise control over the stack.

**What gets saved**:
- General-purpose registers (EAX, EBX, ECX, EDX, ESI, EDI)
- Stack pointer (ESP) and base pointer (EBP)
- Instruction pointer (EIP) -- via the return address on the stack
- EFLAGS register
- Page directory (CR3)

**Switch sequence**:
1. Timer interrupt fires (IRQ 0)
2. CPU pushes EFLAGS, CS, EIP onto the current process's stack
3. ISR stub pushes all general registers
4. Save current ESP into the current process's PCB
5. Call scheduler to pick the next process
6. Load next process's ESP from its PCB
7. Switch CR3 if page directories differ
8. Pop all general registers from the new process's stack
9. `iret` restores EIP, CS, EFLAGS -- execution resumes in the new process

### 4.3 Scheduler

A round-robin scheduler with a fixed time quantum (e.g., 20ms = 2 timer ticks at 100 Hz).

**Data structures**:
```c
static process_t *current_process;  // Currently running
static process_t *ready_queue;      // Linked list of READY processes

void scheduler_init(void);
void schedule(void);                // Called from timer interrupt
process_t *process_create(void (*entry)(void));
void process_exit(void);
```

**Algorithm**:
1. On each timer tick, decrement the current process's time slice
2. When time slice expires, move current process to back of ready queue (state = READY)
3. Pop next process from front of ready queue (state = RUNNING)
4. Context switch to the new process

### 4.4 System Calls

User-mode processes communicate with the kernel via software interrupts. Use `int 0x80` (following Linux convention).

**Syscall convention**:
- `EAX` = syscall number
- `EBX`, `ECX`, `EDX`, `ESI`, `EDI` = arguments
- Return value in `EAX`

**Initial syscalls**:

| Number | Name | Description |
|--------|------|-------------|
| 0 | `sys_exit` | Terminate process |
| 1 | `sys_write` | Write to screen |
| 2 | `sys_read` | Read from keyboard buffer |
| 3 | `sys_fork` | Clone process (stretch goal) |
| 4 | `sys_getpid` | Return current PID |
| 5 | `sys_sleep` | Sleep for N milliseconds |

### 4.5 User Mode (Ring 3)

Transition from Ring 0 (kernel) to Ring 3 (user) to run user processes:
1. Set up TSS (Task State Segment) with the kernel stack pointer
2. Add Ring 3 code and data segments to the GDT
3. Use `iret` to jump to user code with the new privilege level
4. Syscalls (`int 0x80`) switch back to Ring 0 via the IDT

### 4.6 References

- [OSDev: Multitasking](https://wiki.osdev.org/Brendan%27s_Multi-tasking_Tutorial)
- [OSDev: Context Switching](https://wiki.osdev.org/Context_Switching)
- [OSDev: System Calls](https://wiki.osdev.org/System_Calls)
- [OSDev: TSS](https://wiki.osdev.org/TSS)
- James Molloy's Tutorial, Part 9 (Multitasking)
- Intel SDM, Volume 3A, Chapter 7 (Task Management)

---

## Phase 5: File System

**Goal**: Implement disk I/O, a Virtual File System layer, and a simple on-disk file system.

**Files**: `src/drivers/ata.c`, plus new files for VFS and the filesystem implementation

**Estimated LOC**: 1000-1500 lines of C

### 5.1 ATA/IDE Disk Driver

Read and write sectors using ATA PIO (Programmed I/O) mode. This is the simplest disk access method.

**ATA PIO Ports** (primary bus):
```
0x1F0: Data (16-bit)
0x1F1: Error / Features
0x1F2: Sector Count
0x1F3: LBA Low
0x1F4: LBA Mid
0x1F5: LBA High
0x1F6: Drive / Head (+ LBA mode bit)
0x1F7: Status / Command
```

**Read sector sequence**:
1. Select drive: `outb(0x1F6, 0xE0 | (lba >> 24))` (LBA mode, master drive)
2. Set sector count: `outb(0x1F2, 1)`
3. Set LBA address bytes to ports `0x1F3`, `0x1F4`, `0x1F5`
4. Send read command: `outb(0x1F7, 0x20)`
5. Poll status register until BSY clears and DRQ sets
6. Read 256 words (512 bytes) from port `0x1F0` using `insw`

### 5.2 Virtual File System (VFS)

An abstraction layer so the kernel doesn't care which filesystem is on disk.

**VFS node**:
```c
typedef struct vfs_node {
    char name[128];
    uint32_t flags;         // FILE, DIRECTORY, etc.
    uint32_t size;
    uint32_t inode;

    // Function pointers (polymorphism in C)
    read_fn_t  read;
    write_fn_t write;
    open_fn_t  open;
    close_fn_t close;
    readdir_fn_t readdir;
    finddir_fn_t finddir;

    struct vfs_node *parent;
} vfs_node_t;
```

**Operations**:
```c
uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
vfs_node_t *vfs_open(const char *path);
void vfs_close(vfs_node_t *node);
```

### 5.3 File System: FAT12

FAT12 is the standard filesystem for 1.44 MB floppy disks. It is simple to implement and well-documented.

**FAT12 Structure**:
```
Sector 0:          Boot sector (BPB - BIOS Parameter Block)
Sectors 1-9:       FAT (File Allocation Table) -- copy 1
Sectors 10-18:     FAT -- copy 2
Sectors 19-32:     Root directory (224 entries)
Sectors 33+:       Data area (clusters)
```

**Directory entry** (32 bytes):
```
Offset 0:   Filename (8 bytes, space-padded)
Offset 8:   Extension (3 bytes)
Offset 11:  Attributes (0x10 = directory, 0x20 = archive)
Offset 26:  First cluster (2 bytes)
Offset 28:  File size (4 bytes)
```

**FAT chain**: Each FAT entry (12 bits) points to the next cluster in the chain. `0xFFF` = end of file.

**Implementation plan**:
1. Parse the BPB from sector 0
2. Read the FAT into memory
3. Read the root directory
4. Implement `fat12_read_file()` by following the cluster chain
5. Implement `fat12_list_dir()` for directory listing
6. Write support (create files, append data)

### 5.4 Shell Integration

New shell commands:
- `ls` -- List files in current directory
- `cat <file>` -- Print file contents
- `mkdir <name>` -- Create directory
- `write <file> <data>` -- Write data to file
- `stat <file>` -- Show file metadata

### 5.5 References

- [OSDev: ATA PIO Mode](https://wiki.osdev.org/ATA_PIO_Mode)
- [OSDev: VFS](https://wiki.osdev.org/VFS)
- [OSDev: FAT](https://wiki.osdev.org/FAT)
- [Microsoft FAT Specification](https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/fatgen103.doc)
- James Molloy's Tutorial, Part 10 (VFS)

---

## Summary

| Phase | Focus | Est. LOC | Key Deliverable |
|-------|-------|----------|-----------------|
| 1 | Bootloader | 300-400 asm | CPU in protected mode, kernel loaded |
| 2 | Kernel | 1200-1600 C | VGA output, interrupts, keyboard, shell |
| 3 | Memory | 800-1200 C | Page allocator, paging, kmalloc/kfree |
| 4 | Processes | 600-900 C | Multitasking with round-robin scheduler |
| 5 | Filesystem | 1000-1500 C | FAT12 read/write, VFS, disk I/O |
| **Total** | | **~4000-5600** | **A functional teaching OS** |

Each phase builds on the previous one. Do not skip ahead -- later phases depend on the infrastructure from earlier ones.
