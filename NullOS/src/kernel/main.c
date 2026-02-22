/* NullOS Kernel Entry Point
 *
 * Called from Stage 2 bootloader after switching to protected mode.
 * Initializes all kernel subsystems and launches the shell.
 */

#include <kernel/vga.h>
#include <kernel/idt.h>
#include <kernel/isr.h>
#include <kernel/irq.h>
#include <kernel/timer.h>
#include <kernel/keyboard.h>
#include <kernel/shell.h>
#include <lib/string.h>
#include <lib/printf.h>

/* Kernel entry point -- called from assembly at 0x100000 */
void kmain(void) {
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

    shell_run(); /* blocks -- does not return */
}
