/* NullOS Serial Port Driver (COM1)
 *
 * Provides debug output via COM1 (port 0x3F8).
 * QEMU with -serial stdio pipes this to the terminal.
 */

#include <drivers/serial.h>
#include <system.h>

#define COM_DATA   (COM1 + 0)
#define COM_IER    (COM1 + 1)
#define COM_FCR    (COM1 + 2)
#define COM_LCR    (COM1 + 3)
#define COM_MCR    (COM1 + 4)
#define COM_LSR    (COM1 + 5)

#define LCR_DLAB   0x80
#define LSR_THRE   0x20

static int serial_ready = 0;

void serial_init(void) {
    /* Disable interrupts */
    outb(COM_IER, 0x00);

    /* Enable DLAB to set baud divisor */
    outb(COM_LCR, LCR_DLAB);
    outb(COM_DATA, 0x01); /* divisor low byte: 1 -> 115200 baud */
    outb(COM_IER, 0x00);  /* divisor high byte */

    /* 8 bits, no parity, 1 stop bit */
    outb(COM_LCR, 0x03);

    /* Enable FIFO, clear queues, 14-byte threshold */
    outb(COM_FCR, 0xC7);

    /* IRQs disabled, set RTS/DSR */
    outb(COM_MCR, 0x03);

    serial_ready = 1;
}

void serial_putchar(char c) {
    if (!serial_ready)
        return;

    if (c == '\n')
        serial_putchar('\r');

    while ((inb(COM_LSR) & LSR_THRE) == 0) {
    }

    outb(COM_DATA, (uint8_t)c);
}

void serial_write(const char *str) {
    if (!str)
        return;

    while (*str) {
        serial_putchar(*str++);
    }
}
