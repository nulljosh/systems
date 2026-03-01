/* NullOS Kernel Entry Point
 *
 * Called from entry.asm after multiboot loads the kernel.
 * EAX = multiboot magic, EBX = multiboot info pointer.
 */

#include <kernel/vga.h>
#include <kernel/idt.h>
#include <kernel/isr.h>
#include <kernel/irq.h>
#include <kernel/timer.h>
#include <kernel/keyboard.h>
#include <kernel/shell.h>
#include <kernel/memory.h>
#include <kernel/paging.h>
#include <kernel/heap.h>
#include <lib/string.h>
#include <lib/printf.h>

#define MULTIBOOT_MAGIC 0x2BADB002u

/* Multiboot info structure (only the fields we need) */
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} __attribute__((packed)) multiboot_info_t;

/* Kernel entry point -- called from assembly with multiboot args */
void kmain(uint32_t magic, multiboot_info_t *mbi) {
    vga_init();
    kprintf("=== NullOS Kernel Boot ===\n");
    kprintf("Kernel entry point reached at 0x100000\n");
    kprintf("VGA text mode initialized (80x25)\n\n");

    kprintf("Initializing interrupts...\n");
    idt_init();
    kprintf("  IDT loaded\n");

    isr_init();
    kprintf("  ISRs initialized\n");

    irq_init();
    kprintf("  IRQs initialized and remapped\n");

    asm volatile("sti");
    kprintf("  Interrupts enabled\n\n");

    timer_init(100);
    kprintf("  Timer: 100 Hz\n");

    keyboard_init();
    kprintf("  Keyboard: PS/2 IRQ1\n\n");

    /* Phase 3: Memory management */
    kprintf("Initializing memory management...\n");

    if (magic == MULTIBOOT_MAGIC && (mbi->flags & (1u << 6))) {
        pmm_init((void *)mbi->mmap_addr, mbi->mmap_length);
    } else {
        kprintf("  WARNING: No multiboot memory map -- using fallback\n");
        pmm_init((void *)0, 0);
    }

    paging_init();
    heap_init(HEAP_START, HEAP_SIZE);

    kprintf("Memory management initialized.\n\n");

    shell_run(); /* blocks -- does not return */
}
