/* NullOS Interrupt Service Routines
 *
 * Handles CPU exceptions (interrupts 0-31).
 * ISR stubs are generated via ISR_STUB macro.
 *
 * See: docs/PHASES.md Phase 2.3
 */

#include <kernel/isr.h>
#include <kernel/vga.h>
#include <kernel/paging.h>
#include <lib/string.h>
#include <lib/stdint.h>

/* CPU exception names */
static const char *exception_names[] = {
    "Division by zero",
    "Debug exception",
    "NMI interrupt",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Coprocessor segment overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack segment fault",
    "General protection fault",
    "Page fault",
    "Reserved",
    "Floating point exception",
    "Alignment check",
    "Machine check",
    "SIMD floating point exception",
};

/* Common exception handler - called from isr_common with int_no + err_code */
void isr_common_handler(uint32_t int_no, uint32_t err_code) {
    /* ISR 14: page fault -- delegate to paging subsystem */
    if (int_no == 14) {
        registers_t regs;
        memset(&regs, 0, sizeof(regs));
        regs.int_no   = int_no;
        regs.err_code = err_code;
        regs.eip      = 0;  /* EIP not recoverable from this calling convention */
        page_fault_handler(&regs);
        return;
    }

    const char *name = "Unknown";
    if (int_no < 20) {
        name = exception_names[int_no];
    }

    vga_puts("\n=== CPU EXCEPTION ===\n");
    vga_puts("Exception: ");
    vga_puts(name);
    vga_puts("\n");

    vga_puts("Int #: ");
    vga_putchar('0' + (int_no / 10));
    vga_putchar('0' + (int_no % 10));
    vga_puts("\n");

    vga_puts("Err:   0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint32_t n = (err_code >> i) & 0xF;
        vga_putchar(n < 10 ? '0' + n : 'A' + n - 10);
    }
    vga_puts("\n");

    vga_puts("=====================\n");
    while (1) { asm volatile("hlt"); }
}

void isr_init(void) { /* stubs registered in idt_init() */ }

/* ISR stubs: push dummy err_code (0), push int_no, jmp isr_common */
#define ISR_STUB(n) \
    __asm__( \
        ".global isr"#n"\n\t" \
        "isr"#n":\n\t" \
        "    cli\n\t" \
        "    push $0\n\t" \
        "    push $"#n"\n\t" \
        "    jmp isr_common\n" \
    )

/* Exceptions 8, 10-14, 17 push a real error code; the others push a dummy */
ISR_STUB(0);  ISR_STUB(1);  ISR_STUB(2);  ISR_STUB(3);
ISR_STUB(4);  ISR_STUB(5);  ISR_STUB(6);  ISR_STUB(7);
ISR_STUB(8);  ISR_STUB(9);  ISR_STUB(10); ISR_STUB(11);
ISR_STUB(12); ISR_STUB(13); ISR_STUB(14); ISR_STUB(15);
ISR_STUB(16); ISR_STUB(17); ISR_STUB(18); ISR_STUB(19);
ISR_STUB(20); ISR_STUB(21); ISR_STUB(22); ISR_STUB(23);
ISR_STUB(24); ISR_STUB(25); ISR_STUB(26); ISR_STUB(27);
ISR_STUB(28); ISR_STUB(29); ISR_STUB(30); ISR_STUB(31);

/*
 * isr_common - shared ISR entry/exit
 *
 * Stack on entry (from stub + CPU):
 *   [ESP+ 0] int_no      (stub pushed)
 *   [ESP+ 4] err_code    (stub pushed)
 *   [ESP+ 8] EIP         (CPU pushed)
 *   [ESP+12] CS
 *   [ESP+16] EFLAGS
 *
 * After pusha + push %ds:
 *   [ESP+ 0] DS
 *   [ESP+ 4..36] pusha (8 regs)
 *   [ESP+36] int_no
 *   [ESP+40] err_code
 */
__asm__(
    ".global isr_common\n\t"
    "isr_common:\n\t"
    "    pusha\n\t"
    "    push %ds\n\t"
    "    mov  $0x10, %ax\n\t"
    "    mov  %ax, %ds\n\t"
    "    mov  %ax, %es\n\t"
    "    mov  %ax, %fs\n\t"
    "    mov  %ax, %gs\n\t"
    /* pass err_code and int_no as cdecl args (right-to-left) */
    "    mov  40(%esp), %eax\n\t"   /* err_code */
    "    push %eax\n\t"
    "    mov  40(%esp), %eax\n\t"   /* int_no (was at 36, now +4 = 40) */
    "    push %eax\n\t"
    "    call isr_common_handler\n\t"
    "    add  $8, %esp\n\t"         /* discard pushed args */
    "    pop  %eax\n\t"             /* restore DS */
    "    mov  %eax, %ds\n\t"
    "    mov  %eax, %es\n\t"
    "    mov  %eax, %fs\n\t"
    "    mov  %eax, %gs\n\t"
    "    popa\n\t"
    "    add  $8, %esp\n\t"         /* skip int_no + err_code from stub */
    "    iret\n"
);
