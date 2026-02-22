/* NullOS Paging Header */

#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H

#include <lib/stdint.h>
#include <system.h>

/* Page flags */
#define PAGE_PRESENT  0x001
#define PAGE_WRITE    0x002
#define PAGE_USER     0x004

void paging_init(void);
void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
void unmap_page(uint32_t virtual_addr);
void switch_page_directory(uint32_t *dir);
void page_fault_handler(registers_t *regs);

#endif /* _KERNEL_PAGING_H */
