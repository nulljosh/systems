/* NullOS Kernel Heap Allocator
 *
 * Linked-list first-fit allocator on top of paging.
 * Provides kmalloc() and kfree() for dynamic kernel memory.
 *
 * See: docs/PHASES.md Phase 3.3
 */

/* TODO: Implement heap allocator
 *
 * Block header:
 *   typedef struct heap_block {
 *       uint32_t size;
 *       uint8_t  is_free;
 *       struct heap_block *next;
 *   } heap_block_t;
 *
 * Functions:
 *   heap_init(uint32_t start, uint32_t size) - Initialize heap region
 *   kmalloc(uint32_t size)         - Allocate size bytes
 *   kmalloc_aligned(uint32_t size) - Allocate page-aligned
 *   kfree(void *ptr)               - Free allocation
 *
 * Must handle splitting (on alloc) and coalescing (on free).
 */
