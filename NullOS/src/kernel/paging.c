/* NullOS Paging (Virtual Memory)
 *
 * Two-level page table structure: Page Directory -> Page Tables -> Pages.
 * Each level has 1024 entries. Pages are 4 KB.
 *
 * See: docs/PHASES.md Phase 3.2
 */

#include <kernel/paging.h>
#include <kernel/memory.h>
#include <kernel/heap.h>
#include <lib/string.h>
#include <lib/printf.h>

/* Kernel page directory: 1024 entries, each pointing to a page table */
static uint32_t page_directory[1024] __attribute__((aligned(4096)));

/* Static page table for identity-mapping first 4 MB */
static uint32_t first_page_table[1024] __attribute__((aligned(4096)));

void paging_init(void) {
    memset(page_directory, 0, sizeof(page_directory));

    /* Identity map first 4 MB (1024 pages x 4 KB) */
    for (uint32_t i = 0; i < 1024; i++) {
        first_page_table[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE;
    }
    page_directory[0] = ((uint32_t)first_page_table) | PAGE_PRESENT | PAGE_WRITE;

    /* Load page directory into CR3 */
    asm volatile("mov %0, %%cr3" : : "r"(page_directory) : "memory");

    /* Enable paging: set PG bit (bit 31) in CR0 */
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;
    asm volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");

    kprintf("Paging: enabled, first 4 MB identity-mapped\n");
}

void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t dir_idx   = virtual_addr >> 22;
    uint32_t table_idx = (virtual_addr >> 12) & 0x3FFu;

    uint32_t *table;
    if (page_directory[dir_idx] & PAGE_PRESENT) {
        table = (uint32_t *)(page_directory[dir_idx] & 0xFFFFF000u);
    } else {
        /* Allocate a new page table from the PMM */
        uint32_t table_phys = pmm_alloc_frame();
        if (table_phys == 0) {
            kprintf("Paging: failed to allocate page table\n");
            return;
        }
        table = (uint32_t *)table_phys;
        memset(table, 0, PAGE_SIZE);
        page_directory[dir_idx] = table_phys | PAGE_PRESENT | PAGE_WRITE |
                                  (flags & PAGE_USER);
    }

    table[table_idx] = (physical_addr & 0xFFFFF000u) | (flags & 0xFFFu) | PAGE_PRESENT;

    /* Flush TLB entry for this address */
    asm volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
}

void unmap_page(uint32_t virtual_addr) {
    uint32_t dir_idx   = virtual_addr >> 22;
    uint32_t table_idx = (virtual_addr >> 12) & 0x3FFu;

    if (!(page_directory[dir_idx] & PAGE_PRESENT))
        return;

    uint32_t *table = (uint32_t *)(page_directory[dir_idx] & 0xFFFFF000u);
    table[table_idx] = 0;

    asm volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
}

void switch_page_directory(uint32_t *dir) {
    asm volatile("mov %0, %%cr3" : : "r"(dir) : "memory");
}

void page_fault_handler(registers_t *regs) {
    uint32_t fault_addr;
    asm volatile("mov %%cr2, %0" : "=r"(fault_addr));

    int present = (int)(regs->err_code & 0x1u);
    int write   = (int)(regs->err_code & 0x2u);
    int user    = (int)(regs->err_code & 0x4u);

    /* Demand-paging for the kernel heap region */
    if (!present && fault_addr >= HEAP_START && fault_addr < HEAP_START + HEAP_SIZE) {
        uint32_t frame = pmm_alloc_frame();
        if (frame == 0) {
            kprintf("Page fault: out of physical memory at 0x%x\n", fault_addr);
            while (1) asm volatile("hlt");
        }
        uint32_t page = fault_addr & 0xFFFFF000u;
        map_page(page, frame, PAGE_PRESENT | PAGE_WRITE);
        return;
    }

    /* Unrecoverable page fault */
    kprintf("\n=== PAGE FAULT ===\n");
    kprintf("Address: 0x%x\n", fault_addr);
    kprintf("Error:   %s %s %s\n",
            present ? "protection" : "not-present",
            write   ? "write"      : "read",
            user    ? "user-mode"  : "kernel-mode");
    kprintf("EIP:     0x%x\n", regs->eip);
    kprintf("==================\n");
    while (1) asm volatile("hlt");
}
