/* NullOS Master System Header
 *
 * Common types, constants, and inline I/O functions
 * used throughout the kernel.
 */

#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <lib/stdint.h>
#include <lib/stddef.h>

/* --- I/O Port Access --- */

/* Write a byte to an I/O port */
static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Read a byte from an I/O port */
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Write a word to an I/O port */
static inline void outw(uint16_t port, uint16_t val)
{
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

/* Read a word from an I/O port */
static inline uint16_t inw(uint16_t port)
{
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Write a dword to an I/O port */
static inline void outl(uint16_t port, uint32_t val)
{
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

/* Read a dword from an I/O port */
static inline uint32_t inl(uint16_t port)
{
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* I/O wait (short delay for slow hardware) */
static inline void io_wait(void)
{
    outb(0x80, 0);
}

/* --- CPU Control --- */

/* Disable interrupts */
static inline void cli(void)
{
    asm volatile("cli");
}

/* Enable interrupts */
static inline void sti(void)
{
    asm volatile("sti");
}

/* Halt until next interrupt */
static inline void hlt(void)
{
    asm volatile("hlt");
}

/* --- Register structure for interrupt handlers --- */
typedef struct {
    uint32_t ds;                                        /* Data segment */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pusha */
    uint32_t int_no, err_code;                          /* Interrupt number and error code */
    uint32_t eip, cs, eflags, useresp, ss;              /* Pushed by CPU */
} registers_t;

/* Interrupt handler function type */
typedef void (*isr_handler_t)(registers_t *);
typedef void (*irq_handler_t)(void);

#endif /* _SYSTEM_H */
