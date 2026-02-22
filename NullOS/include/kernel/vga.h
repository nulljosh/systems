/* NullOS VGA Text Mode Header */

#ifndef _KERNEL_VGA_H
#define _KERNEL_VGA_H

#include <lib/stdint.h>

/* VGA constants */
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000

/* VGA color codes */
enum vga_color {
    VGA_BLACK         = 0,
    VGA_BLUE          = 1,
    VGA_GREEN         = 2,
    VGA_CYAN          = 3,
    VGA_RED           = 4,
    VGA_MAGENTA       = 5,
    VGA_BROWN         = 6,
    VGA_LIGHT_GRAY    = 7,
    VGA_DARK_GRAY     = 8,
    VGA_LIGHT_BLUE    = 9,
    VGA_LIGHT_GREEN   = 10,
    VGA_LIGHT_CYAN    = 11,
    VGA_LIGHT_RED     = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_YELLOW        = 14,
    VGA_WHITE         = 15,
};

/* Make a VGA attribute byte from foreground and background colors */
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg)
{
    return fg | (bg << 4);
}

/* Make a VGA entry (character + attribute) */
static inline uint16_t vga_entry(unsigned char c, uint8_t color)
{
    return (uint16_t)c | ((uint16_t)color << 8);
}

void vga_init(void);
void vga_putchar(char c);
void vga_puts(const char *str);
void vga_set_color(uint8_t fg, uint8_t bg);
void vga_clear(void);
void vga_set_cursor(int row, int col);

#endif /* _KERNEL_VGA_H */
