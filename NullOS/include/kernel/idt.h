/* NullOS IDT Header */

#ifndef _KERNEL_IDT_H
#define _KERNEL_IDT_H

#include <lib/stdint.h>

/* IDT entry (interrupt gate descriptor) */
typedef struct {
    uint16_t offset_low;    /* Handler address bits 0-15 */
    uint16_t selector;      /* Code segment selector */
    uint8_t  zero;          /* Always 0 */
    uint8_t  type_attr;     /* Type and attributes */
    uint16_t offset_high;   /* Handler address bits 16-31 */
} __attribute__((packed)) idt_entry_t;

/* IDT pointer for lidt instruction */
typedef struct {
    uint16_t limit;         /* Size of IDT - 1 */
    uint32_t base;          /* Address of first entry */
} __attribute__((packed)) idt_ptr_t;

#define IDT_ENTRIES 256

void idt_init(void);
void idt_set_gate(int n, uint32_t handler);
void idt_set_gate(int n, uint32_t handler);

#endif /* _KERNEL_IDT_H */
