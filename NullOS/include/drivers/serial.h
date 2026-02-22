/* NullOS Serial Port Header */

#ifndef _DRIVERS_SERIAL_H
#define _DRIVERS_SERIAL_H

#define COM1 0x3F8

void serial_init(void);
void serial_putchar(char c);
void serial_write(const char *str);

#endif /* _DRIVERS_SERIAL_H */
