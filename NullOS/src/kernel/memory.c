/* NullOS Physical Memory Manager
 *
 * Bitmap-based physical page frame allocator.
 * 1 bit per 4 KB page. Manages all usable physical RAM.
 *
 * See: docs/PHASES.md Phase 3.1
 */

#include <kernel/memory.h>
#include <lib/string.h>
#include <lib/printf.h>

/* Bitmap: 1 bit per 4KB frame. Bit set = frame in use.
 * 32768 uint32_t * 32 bits * 4096 bytes = 4 GB addressable */
#define BITMAP_SIZE 32768

static uint32_t frame_bitmap[BITMAP_SIZE];
static uint32_t total_frame_count;
static uint32_t used_frame_count;

/* Set a frame as used */
static void frame_set(uint32_t frame) {
    frame_bitmap[frame / 32] |= (1u << (frame % 32));
}

/* Clear a frame (mark as free) */
static void frame_clear(uint32_t frame) {
    frame_bitmap[frame / 32] &= ~(1u << (frame % 32));
}

/* Test if a frame is used */
static int frame_test(uint32_t frame) {
    return (int)(frame_bitmap[frame / 32] & (1u << (frame % 32)));
}

/* Find first free frame; returns frame index or (uint32_t)-1 on failure */
static uint32_t frame_first_free(void) {
    uint32_t words = (total_frame_count + 31) / 32;
    for (uint32_t i = 0; i < words; i++) {
        if (frame_bitmap[i] != 0xFFFFFFFFu) {
            for (uint32_t j = 0; j < 32; j++) {
                if (!(frame_bitmap[i] & (1u << j))) {
                    uint32_t frame = i * 32 + j;
                    if (frame < total_frame_count)
                        return frame;
                }
            }
        }
    }
    return (uint32_t)-1;
}

/* Linker-provided symbol -- marks end of kernel image */
extern uint32_t _kernel_end;

/* Multiboot memory map entry layout (packed) */
typedef struct {
    uint32_t size;       /* size of this entry excluding this field */
    uint32_t base_lo;
    uint32_t base_hi;
    uint32_t length_lo;
    uint32_t length_hi;
    uint32_t type;       /* 1 = usable */
} __attribute__((packed)) mmap_entry_t;

void pmm_init(void *memory_map, uint32_t map_length) {
    /* Mark all frames as used to start */
    memset(frame_bitmap, 0xFF, sizeof(frame_bitmap));
    total_frame_count = 0;
    used_frame_count  = 0;

    if (memory_map == (void *)0 || map_length == 0) {
        /* Fallback: assume 32 MB, treat 1MB-32MB as usable */
        total_frame_count = (32u * 1024u * 1024u) / PAGE_SIZE;
        used_frame_count  = total_frame_count;
        uint32_t start_frame = 0x100000 / PAGE_SIZE;
        for (uint32_t i = start_frame; i < total_frame_count; i++) {
            frame_clear(i);
            used_frame_count--;
        }
        kprintf("PMM: fallback mode, 32 MB assumed, %u frames free\n",
                pmm_free_frames());
        return;
    }

    mmap_entry_t *entry = (mmap_entry_t *)memory_map;
    mmap_entry_t *end   = (mmap_entry_t *)((uint32_t)memory_map + map_length);

    /* First pass: find highest usable address to size the bitmap */
    uint32_t max_addr = 0;
    mmap_entry_t *e = entry;
    while (e < end) {
        if (e->base_hi == 0 && e->length_hi == 0) {  /* only handle 32-bit range */
            uint32_t region_end = e->base_lo + e->length_lo;
            if (e->type == 1 && region_end > max_addr)
                max_addr = region_end;
        }
        e = (mmap_entry_t *)((uint32_t)e + e->size + sizeof(e->size));
    }

    total_frame_count = max_addr / PAGE_SIZE;
    if (total_frame_count > BITMAP_SIZE * 32)
        total_frame_count = BITMAP_SIZE * 32;

    used_frame_count = total_frame_count;

    /* Second pass: free usable regions */
    e = entry;
    while (e < end) {
        if (e->type == 1 && e->base_hi == 0 && e->length_hi == 0) {
            uint32_t base = e->base_lo;
            uint32_t len  = e->length_lo;

            /* Align base up to page boundary */
            if (base & (PAGE_SIZE - 1)) {
                uint32_t aligned = (base + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
                uint32_t wasted  = aligned - base;
                if (wasted < len)
                    len -= wasted;
                else
                    len = 0;
                base = aligned;
            }

            for (uint32_t addr = base; addr + PAGE_SIZE <= base + len; addr += PAGE_SIZE) {
                uint32_t frame = addr / PAGE_SIZE;
                if (frame < total_frame_count) {
                    frame_clear(frame);
                    used_frame_count--;
                }
            }
        }
        e = (mmap_entry_t *)((uint32_t)e + e->size + sizeof(e->size));
    }

    /* Reserve first 1 MB (BIOS, VGA, bootloader) */
    for (uint32_t i = 0; i < 256 && i < total_frame_count; i++) {
        if (!frame_test(i)) {
            frame_set(i);
            used_frame_count++;
        }
    }

    /* Reserve kernel image (0x100000 to _kernel_end) */
    uint32_t kend        = (uint32_t)&_kernel_end;
    uint32_t kstart_frame = 0x100000 / PAGE_SIZE;
    uint32_t kend_frame   = (kend + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t i = kstart_frame; i < kend_frame && i < total_frame_count; i++) {
        if (!frame_test(i)) {
            frame_set(i);
            used_frame_count++;
        }
    }

    kprintf("PMM: %u MB detected, %u frames total, %u free\n",
            max_addr / (1024u * 1024u),
            total_frame_count,
            total_frame_count - used_frame_count);
}

uint32_t pmm_alloc_frame(void) {
    uint32_t frame = frame_first_free();
    if (frame == (uint32_t)-1) {
        kprintf("PMM: out of memory!\n");
        return 0;
    }
    frame_set(frame);
    used_frame_count++;
    return frame * PAGE_SIZE;
}

void pmm_free_frame(uint32_t addr) {
    uint32_t frame = addr / PAGE_SIZE;
    if (frame < total_frame_count && frame_test(frame)) {
        frame_clear(frame);
        used_frame_count--;
    }
}

uint32_t pmm_used_frames(void) {
    return used_frame_count;
}

uint32_t pmm_free_frames(void) {
    return total_frame_count - used_frame_count;
}

uint32_t pmm_total_frames(void) {
    return total_frame_count;
}
