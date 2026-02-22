/* NullOS Paging (Virtual Memory)
 *
 * Two-level page table structure: Page Directory -> Page Tables -> Pages.
 * Each level has 1024 entries. Pages are 4 KB.
 *
 * See: docs/PHASES.md Phase 3.2
 */

/* TODO: Implement paging
 *
 * Page Directory Entry / Page Table Entry flags:
 *   #define PAGE_PRESENT   0x001
 *   #define PAGE_WRITE     0x002
 *   #define PAGE_USER      0x004
 *
 * Functions:
 *   paging_init()                              - Create kernel page directory, identity-map kernel
 *   map_page(uint32_t virt, uint32_t phys, uint32_t flags)
 *   unmap_page(uint32_t virt)
 *   page_fault_handler(registers_t *r)         - Handle ISR 14
 *   switch_page_directory(uint32_t *dir)       - Load CR3
 */
