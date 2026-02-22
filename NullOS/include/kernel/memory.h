/* NullOS Physical Memory Manager Header */

#ifndef _KERNEL_MEMORY_H
#define _KERNEL_MEMORY_H

#include <lib/stdint.h>

#define PAGE_SIZE 4096

void     pmm_init(void *memory_map, uint32_t map_length);
uint32_t pmm_alloc_frame(void);
void     pmm_free_frame(uint32_t addr);
uint32_t pmm_used_frames(void);
uint32_t pmm_free_frames(void);
uint32_t pmm_total_frames(void);

#endif /* _KERNEL_MEMORY_H */
