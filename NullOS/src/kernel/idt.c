/* NullOS Interrupt Descriptor Table
 *
 * Sets up the 256-entry IDT for handling CPU exceptions,
 * hardware interrupts (IRQs), and software interrupts (syscalls).
 *
 * See: docs/PHASES.md Phase 2.2
 */

#include <kernel/idt.h>
#include <lib/stdint.h>
#include <lib/string.h>

/* IDT entry structure (8 bytes) */
struct idt_entry {
    uint16_t offset_low;   /* Handler address bits 0-15 */
    uint16_t selector;     /* Code segment selector (0x08) */
    uint8_t zero;          /* Always 0 */
    uint8_t type_attr;     /* Type and attributes */
    uint16_t offset_high;  /* Handler address bits 16-31 */
} __attribute__((packed));

/* IDT descriptor */
struct idt_descriptor {
    uint16_t limit;        /* IDT size - 1 */
    uint32_t base;         /* IDT base address */
} __attribute__((packed));

/* IDT table (256 entries for 256 interrupts) */
static struct idt_entry idt[256];

/* IDT descriptor */
static struct idt_descriptor idt_desc = {
    .limit = sizeof(idt) - 1,
    .base = (uint32_t)&idt,
};

/* Forward declaration of interrupt handlers (defined in isr.asm or isr.c) */
extern void isr0(void);   /* Division by zero */
extern void isr1(void);   /* Debug exception */
extern void isr2(void);   /* NMI */
extern void isr3(void);   /* Breakpoint */
extern void isr4(void);   /* Overflow */
extern void isr5(void);   /* Bound range exceeded */
extern void isr6(void);   /* Invalid opcode */
extern void isr7(void);   /* Device not available */
extern void isr8(void);   /* Double fault */
extern void isr9(void);   /* Coprocessor segment overrun */
extern void isr10(void);  /* Invalid TSS */
extern void isr11(void);  /* Segment not present */
extern void isr12(void);  /* Stack segment fault */
extern void isr13(void);  /* General protection fault */
extern void isr14(void);  /* Page fault */
extern void isr15(void);  /* Reserved */
extern void isr16(void);  /* Floating point exception */
extern void isr17(void);  /* Alignment check */
extern void isr18(void);  /* Machine check */
extern void isr19(void);  /* SIMD floating point */

/* Interrupt gate type */
#define IDT_TYPE_INTERRUPT_GATE 0x8E  /* Present, Ring 0, Interrupt gate */
#define IDT_TYPE_TRAP_GATE      0x8F  /* Present, Ring 0, Trap gate */

/* Set an IDT entry */
void idt_set_gate(int n, uint32_t handler) {
    if (n >= 256) return;
    
    idt[n].offset_low = handler & 0xFFFF;
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
    idt[n].selector = 0x08;              /* Code segment selector */
    idt[n].zero = 0;
    idt[n].type_attr = IDT_TYPE_INTERRUPT_GATE;
}

/* Initialize the IDT */
void idt_init(void) {
    /* Clear the IDT */
    memset(idt, 0, sizeof(idt));
    
    /* Set up CPU exception handlers (0-19) */
    idt_set_gate(0, (uint32_t)isr0);
    idt_set_gate(1, (uint32_t)isr1);
    idt_set_gate(2, (uint32_t)isr2);
    idt_set_gate(3, (uint32_t)isr3);
    idt_set_gate(4, (uint32_t)isr4);
    idt_set_gate(5, (uint32_t)isr5);
    idt_set_gate(6, (uint32_t)isr6);
    idt_set_gate(7, (uint32_t)isr7);
    idt_set_gate(8, (uint32_t)isr8);
    idt_set_gate(9, (uint32_t)isr9);
    idt_set_gate(10, (uint32_t)isr10);
    idt_set_gate(11, (uint32_t)isr11);
    idt_set_gate(12, (uint32_t)isr12);
    idt_set_gate(13, (uint32_t)isr13);
    idt_set_gate(14, (uint32_t)isr14);
    idt_set_gate(15, (uint32_t)isr15);
    idt_set_gate(16, (uint32_t)isr16);
    idt_set_gate(17, (uint32_t)isr17);
    idt_set_gate(18, (uint32_t)isr18);
    idt_set_gate(19, (uint32_t)isr19);
    
    /* Hardware IRQs (32-47) will be set up by irq_init() */
    
    /* Load the IDT into the CPU */
    asm volatile("lidt %0" : : "m"(idt_desc));
}
