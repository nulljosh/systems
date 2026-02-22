/* NullOS Kernel Heap Header */

#ifndef _KERNEL_HEAP_H
#define _KERNEL_HEAP_H

#include <lib/stdint.h>

#define HEAP_START 0xC0000000
#define HEAP_SIZE  0x10000000   /* 256 MB virtual range */

void  heap_init(uint32_t start, uint32_t size);
void *kmalloc(uint32_t size);
void *kmalloc_aligned(uint32_t size);
void  kfree(void *ptr);

#endif /* _KERNEL_HEAP_H */
