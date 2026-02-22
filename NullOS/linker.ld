/* NullOS Linker Script
 *
 * The kernel is loaded at 1 MB (0x100000) in physical memory.
 * This is the standard location above the BIOS/real-mode area.
 *
 * Memory layout after boot:
 *   0x00000000 - 0x000FFFFF  Real mode IVT, BIOS data, bootloader
 *   0x00100000 - ...         Kernel starts here
 */

ENTRY(_start)

SECTIONS
{
    /* Kernel starts at 1 MB */
    . = 0x100000;

    /* Code section */
    .text ALIGN(4K) :
    {
        *(.multiboot)       /* Multiboot header (if used) */
        *(.text)            /* All code */
    }

    /* Read-only data */
    .rodata ALIGN(4K) :
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* Initialized read-write data */
    .data ALIGN(4K) :
    {
        *(.data)
    }

    /* Uninitialized data (zeroed by kernel startup) */
    .bss ALIGN(4K) :
    {
        _bss_start = .;
        *(.bss)
        *(COMMON)
        _bss_end = .;
    }

    /* Track end of kernel for memory allocator */
    _kernel_end = .;

    /* Discard unnecessary sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame)
    }
}
