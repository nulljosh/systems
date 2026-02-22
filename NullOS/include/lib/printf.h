/* NullOS Printf Header */

#ifndef _LIB_PRINTF_H
#define _LIB_PRINTF_H

#include <lib/stdarg.h>

int kprintf(const char *fmt, ...);
int ksprintf(char *buf, const char *fmt, ...);
int kvprintf(const char *fmt, va_list args);

#endif /* _LIB_PRINTF_H */
