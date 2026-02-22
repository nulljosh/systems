/* NullOS Physical Memory Manager
 *
 * Bitmap-based physical page frame allocator.
 * 1 bit per 4 KB page. Manages all usable physical RAM.
 *
 * See: docs/PHASES.md Phase 3.1
 */

/* TODO: Implement physical memory manager
 *
 * Data:
 *   static uint32_t *frame_bitmap;
 *   static uint32_t total_frames;
 *   static uint32_t used_frames;
 *
 * Functions:
 *   pmm_init(multiboot_memory_map_t *mmap) - Parse memory map, init bitmap
 *   pmm_alloc_frame()    - Find first free frame, return physical address
 *   pmm_free_frame(addr) - Mark frame as free
 *   pmm_used_frames()    - Return count of allocated frames
 *   pmm_free_frames()    - Return count of free frames
 */
