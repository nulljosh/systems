/* NullOS Serial Port Driver (COM1)
 *
 * Provides debug output via COM1 (port 0x3F8).
 * QEMU with -serial stdio pipes this to the terminal.
 *
 * See: docs/ARCHITECTURE.md I/O Port Map
 */

/* TODO: Implement serial driver
 *
 * COM1 ports:
 *   0x3F8: Data
 *   0x3F9: Interrupt Enable
 *   0x3FA: FIFO Control
 *   0x3FB: Line Control
 *   0x3FC: Modem Control
 *   0x3FD: Line Status
 *
 * Initialization:
 *   Disable interrupts, set baud rate (115200), 8N1 format,
 *   enable FIFO, set RTS/DSR.
 *
 * Functions:
 *   serial_init()              - Initialize COM1
 *   serial_putchar(char c)     - Write one character
 *   serial_write(const char *) - Write string
 *   serial_printf(fmt, ...)    - Formatted debug output
 */
