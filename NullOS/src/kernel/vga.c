/* NullOS VGA Text Mode Driver
 *
 * 80x25 color text mode. Frame buffer at 0xB8000.
 * Each cell: 2 bytes (character + attribute).
 *
 * See: docs/PHASES.md Phase 2.1
 */

#include <kernel/vga.h>
#include <lib/string.h>
#include <lib/stdint.h>
#include <system.h>

/* VGA framebuffer pointer */
#define VGA_BUF ((volatile uint16_t *)VGA_MEMORY)

/* VGA state */
static int vga_row = 0;
static int vga_col = 0;
static uint8_t vga_color = (VGA_WHITE << 4) | VGA_BLACK;

/* Make color attribute byte */
static inline uint8_t vga_make_color(uint8_t fg, uint8_t bg) {
    return (uint8_t)((bg << 4) | fg);
}

/* Update hardware cursor */
static void vga_update_cursor(void) {
    uint16_t pos = vga_row * VGA_WIDTH + vga_col;
    
    /* Set high byte of cursor position */
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)(pos >> 8));

    /* Set low byte of cursor position */
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)pos);
}

/* Clear the screen */
void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUF[i] = vga_entry(' ', vga_color);
    }
    vga_row = 0;
    vga_col = 0;
    vga_update_cursor();
}

/* Initialize VGA driver */
void vga_init(void) {
    vga_color = vga_make_color(VGA_WHITE, VGA_BLACK);
    vga_clear();
}

/* Set text color */
void vga_set_color(uint8_t fg, uint8_t bg) {
    vga_color = vga_make_color(fg, bg);
}

/* Put a single character */
void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else {
        VGA_BUF[vga_row * VGA_WIDTH + vga_col] = vga_entry(c, vga_color);
        vga_col++;
    }
    
    /* Handle line wrap */
    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }
    
    /* Handle scrolling */
    if (vga_row >= VGA_HEIGHT) {
        /* Scroll up by moving rows */
        for (int i = 0; i < VGA_HEIGHT - 1; i++) {
            for (int j = 0; j < VGA_WIDTH; j++) {
                VGA_BUF[i * VGA_WIDTH + j] = 
                    VGA_BUF[(i + 1) * VGA_WIDTH + j];
            }
        }
        
        /* Clear bottom row */
        for (int j = 0; j < VGA_WIDTH; j++) {
            VGA_BUF[(VGA_HEIGHT - 1) * VGA_WIDTH + j] = 
                vga_entry(' ', vga_color);
        }
        
        vga_row = VGA_HEIGHT - 1;
    }
    
    vga_update_cursor();
}

/* Put a string */
void vga_puts(const char *str) {
    while (*str) {
        vga_putchar(*str);
        str++;
    }
}
