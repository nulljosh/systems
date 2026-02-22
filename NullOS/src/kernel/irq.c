/* NullOS Hardware Interrupt Handlers
 *
 * Remaps the 8259 PIC and handles IRQs 0-15 (interrupts 32-47).
 *
 * See: docs/PHASES.md Phase 2.4
 */

#include <kernel/irq.h>
#include <kernel/idt.h>
#include <kernel/vga.h>
#include <lib/stdint.h>

/* PIC I/O ports */
#define PIC_MASTER_CMD   0x20
#define PIC_MASTER_DATA  0x21
#define PIC_SLAVE_CMD    0xA0
#define PIC_SLAVE_DATA   0xA1

/* PIC commands */
#define ICW1_ICW4        0x01  /* ICW4 not needed */
#define ICW1_SINGLE      0x02  /* Single (not cascade) mode */
#define ICW1_INTERVAL4   0x04  /* Interval of 4 (not 8) */
#define ICW1_LEVEL       0x08  /* Level triggered (not edge) */
#define ICW1_INIT        0x10  /* Initialization */

#define ICW4_8086        0x01  /* 8086 mode (not MCS-80/85) */
#define ICW4_AUTO        0x02  /* Auto (not normal) EOI */
#define ICW4_BUF_MASTER  0x04  /* Buffered mode/master */
#define ICW4_BUF_SLAVE   0x08  /* Buffered mode/slave */
#define ICW4_SFNM        0x10  /* Special fully nested (not normal) */

#define PIC_EOI          0x20  /* End of Interrupt */

/* Forward declarations of IRQ handlers (defined in assembly stubs) */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

/* IRQ handler callback table */
static irq_handler_t irq_handlers[16] = {0};

/* Remap the PIC interrupt vectors */
static void irq_remap_pic(void) {
    /* Start initialization sequence */
    outb(PIC_MASTER_CMD, ICW1_INIT | ICW1_ICW4);
    outb(PIC_SLAVE_CMD, ICW1_INIT | ICW1_ICW4);
    
    /* ICW2 - Set vector offsets */
    outb(PIC_MASTER_DATA, 32);   /* Master PIC -> IRQ 0-7 map to INT 32-39 */
    outb(PIC_SLAVE_DATA, 40);    /* Slave PIC -> IRQ 8-15 map to INT 40-47 */
    
    /* ICW3 - Cascade configuration */
    outb(PIC_MASTER_DATA, 0x04); /* Master: slave on IR2 */
    outb(PIC_SLAVE_DATA, 0x02);  /* Slave: cascade identity 2 */
    
    /* ICW4 - 8086 mode */
    outb(PIC_MASTER_DATA, ICW4_8086);
    outb(PIC_SLAVE_DATA, ICW4_8086);
    
    /* Restore masks (disable all for now) */
    outb(PIC_MASTER_DATA, 0xFF);
    outb(PIC_SLAVE_DATA, 0xFF);
}

/* Initialize IRQ handling */
void irq_init(void) {
    /* Remap PIC */
    irq_remap_pic();
    
    /* Register all IRQ handlers with IDT (interrupts 32-47) */
    idt_set_gate(32, (uint32_t)irq0);
    idt_set_gate(33, (uint32_t)irq1);
    idt_set_gate(34, (uint32_t)irq2);
    idt_set_gate(35, (uint32_t)irq3);
    idt_set_gate(36, (uint32_t)irq4);
    idt_set_gate(37, (uint32_t)irq5);
    idt_set_gate(38, (uint32_t)irq6);
    idt_set_gate(39, (uint32_t)irq7);
    idt_set_gate(40, (uint32_t)irq8);
    idt_set_gate(41, (uint32_t)irq9);
    idt_set_gate(42, (uint32_t)irq10);
    idt_set_gate(43, (uint32_t)irq11);
    idt_set_gate(44, (uint32_t)irq12);
    idt_set_gate(45, (uint32_t)irq13);
    idt_set_gate(46, (uint32_t)irq14);
    idt_set_gate(47, (uint32_t)irq15);
    
    /* Enable IRQ0 (timer) and IRQ1 (keyboard) for now */
    uint8_t master_mask = inb(PIC_MASTER_DATA);
    outb(PIC_MASTER_DATA, master_mask & ~0x03);  /* Enable IRQ0 and IRQ1 */
}

/* Send End-Of-Interrupt signal */
static void irq_send_eoi(int irq) {
    if (irq >= 8) {
        outb(PIC_SLAVE_CMD, PIC_EOI);
    }
    outb(PIC_MASTER_CMD, PIC_EOI);
}

/* Common IRQ handler - called from assembly stubs */
void irq_common_handler(int irq) {
    /* Send EOI */
    irq_send_eoi(irq);
    
    /* Call registered handler if available */
    if (irq_handlers[irq]) {
        irq_handlers[irq]();
    } else {
        /* Default: print message */
        vga_puts("IRQ ");
        vga_putchar('0' + irq / 10);
        vga_putchar('0' + irq % 10);
        vga_puts(" fired\n");
    }
}

/* Install a custom IRQ handler */
void irq_install_handler(int irq, irq_handler_t handler) {
    if (irq >= 0 && irq < 16) {
        irq_handlers[irq] = handler;
    }
}

/* Uninstall a custom IRQ handler */
void irq_uninstall_handler(int irq) {
    if (irq >= 0 && irq < 16) {
        irq_handlers[irq] = NULL;
    }
}

/* IRQ stubs (simplified - generated from macro) */
#define IRQ_STUB(n) \
    __asm__( \
        ".global irq"#n"\n\t" \
        "irq"#n":\n\t" \
        "    cli\n\t" \
        "    push $"#n"\n\t" \
        "    jmp irq_common\n" \
    )

/* Generate IRQ 0-15 stubs */
IRQ_STUB(0);
IRQ_STUB(1);
IRQ_STUB(2);
IRQ_STUB(3);
IRQ_STUB(4);
IRQ_STUB(5);
IRQ_STUB(6);
IRQ_STUB(7);
IRQ_STUB(8);
IRQ_STUB(9);
IRQ_STUB(10);
IRQ_STUB(11);
IRQ_STUB(12);
IRQ_STUB(13);
IRQ_STUB(14);
IRQ_STUB(15);

/* Common IRQ entry point */
__asm__(
    ".global irq_common\n\t"
    "irq_common:\n\t"
    "    pusha\n\t"
    "    push %ds\n\t"
    "    mov $0x10, %ax\n\t"
    "    mov %ax, %ds\n\t"
    "    mov %ax, %es\n\t"
    "    mov %ax, %fs\n\t"
    "    mov %ax, %gs\n\t"
    "    mov 36(%esp), %eax\n\t"    /* peek at irq# (offset 36: 1 ds + 8 pusha regs)*4) */
    "    push %eax\n\t"              /* push as C argument */
    "    call irq_common_handler\n\t"
    "    add $4, %esp\n\t"           /* clean up pushed arg */
    "    pop %eax\n\t"               /* restore old DS (no pusha corruption) */
    "    mov %eax, %ds\n\t"
    "    mov %eax, %es\n\t"
    "    mov %eax, %fs\n\t"
    "    mov %eax, %gs\n\t"
    "    popa\n\t"
    "    add $4, %esp\n\t"           /* skip irq# pushed by stub */
    "    iret\n"
);
