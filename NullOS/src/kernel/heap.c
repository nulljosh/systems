/* NullOS Kernel Heap Allocator
 *
 * Linked-list first-fit allocator on top of paging.
 * Provides kmalloc() and kfree() for dynamic kernel memory.
 *
 * See: docs/PHASES.md Phase 3.3
 */

#include <kernel/heap.h>
#include <kernel/paging.h>
#include <kernel/memory.h>
#include <lib/string.h>
#include <lib/printf.h>

/* Block header for the linked-list allocator */
typedef struct heap_block {
    uint32_t           size;    /* payload size (not including header) */
    uint8_t            is_free;
    struct heap_block *next;
} __attribute__((packed)) heap_block_t;

#define HEADER_SIZE    ((uint32_t)sizeof(heap_block_t))
#define MIN_BLOCK_SIZE 16u

static heap_block_t *heap_head;
static uint32_t      heap_limit;       /* virtual end of heap region */
static uint32_t      heap_mapped_end;  /* how far pages have been mapped */

/* Map physical frames to cover [heap_mapped_end, target) */
static void ensure_mapped(uint32_t target) {
    while (heap_mapped_end < target) {
        uint32_t frame = pmm_alloc_frame();
        if (frame == 0) {
            kprintf("Heap: out of physical memory\n");
            return;
        }
        map_page(heap_mapped_end, frame, PAGE_PRESENT | PAGE_WRITE);
        heap_mapped_end += PAGE_SIZE;
    }
}

void heap_init(uint32_t start, uint32_t size) {
    heap_mapped_end = start;
    heap_limit      = start + size;

    /* Map the first page so we can write the initial block header */
    ensure_mapped(start + PAGE_SIZE);

    heap_head          = (heap_block_t *)start;
    heap_head->size    = PAGE_SIZE - HEADER_SIZE;
    heap_head->is_free = 1;
    heap_head->next    = (heap_block_t *)0;

    kprintf("Heap: initialized at 0x%x, max size %u MB\n",
            start, size / (1024u * 1024u));
}

void *kmalloc(uint32_t size) {
    if (size == 0)
        return (void *)0;

    /* Align payload to 4 bytes */
    size = (size + 3u) & ~3u;

    /* First-fit search through existing blocks */
    heap_block_t *block = heap_head;
    heap_block_t *prev  = (heap_block_t *)0;

    while (block) {
        if (block->is_free && block->size >= size) {
            /* Split if the leftover is large enough to hold a new header + data */
            if (block->size >= size + HEADER_SIZE + MIN_BLOCK_SIZE) {
                heap_block_t *split = (heap_block_t *)((uint32_t)block + HEADER_SIZE + size);
                /* Make sure the split header itself is mapped */
                ensure_mapped((uint32_t)split + HEADER_SIZE);
                split->size    = block->size - size - HEADER_SIZE;
                split->is_free = 1;
                split->next    = block->next;
                block->size    = size;
                block->next    = split;
            }
            block->is_free = 0;
            return (void *)((uint32_t)block + HEADER_SIZE);
        }
        prev  = block;
        block = block->next;
    }

    /* No suitable free block -- extend the heap */
    uint32_t new_addr;
    if (prev)
        new_addr = (uint32_t)prev + HEADER_SIZE + prev->size;
    else
        new_addr = (uint32_t)heap_head;

    if (new_addr + HEADER_SIZE + size > heap_limit) {
        kprintf("Heap: out of virtual address space\n");
        return (void *)0;
    }

    /* Map enough pages to cover the new block */
    ensure_mapped(new_addr + HEADER_SIZE + size);

    heap_block_t *nb = (heap_block_t *)new_addr;
    nb->size         = size;
    nb->is_free      = 0;
    nb->next         = (heap_block_t *)0;

    if (prev)
        prev->next = nb;

    return (void *)((uint32_t)nb + HEADER_SIZE);
}

void *kmalloc_aligned(uint32_t size) {
    if (size == 0)
        return (void *)0;

    /* Over-allocate to guarantee we can return a page-aligned pointer */
    void *ptr = kmalloc(size + PAGE_SIZE);
    if (!ptr)
        return (void *)0;

    uint32_t raw     = (uint32_t)ptr;
    uint32_t aligned = (raw + PAGE_SIZE - 1u) & ~(PAGE_SIZE - 1u);
    return (void *)aligned;
}

void kfree(void *ptr) {
    if (!ptr)
        return;

    heap_block_t *block = (heap_block_t *)((uint32_t)ptr - HEADER_SIZE);
    block->is_free = 1;

    /* Coalesce forward: merge with next block if it is also free */
    if (block->next && block->next->is_free) {
        block->size += HEADER_SIZE + block->next->size;
        block->next  = block->next->next;
    }

    /* Coalesce backward: find previous block and merge if free */
    if (heap_head != block) {
        heap_block_t *p = heap_head;
        while (p && p->next != block)
            p = p->next;
        if (p && p->is_free) {
            p->size += HEADER_SIZE + block->size;
            p->next  = block->next;
        }
    }
}
