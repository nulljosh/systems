/* Freestanding stddef.h for NullOS */

#ifndef _LIB_STDDEF_H
#define _LIB_STDDEF_H

#include <lib/stdint.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#define offsetof(type, member) ((size_t)&((type *)0)->member)

typedef int32_t  ptrdiff_t;

#endif /* _LIB_STDDEF_H */
