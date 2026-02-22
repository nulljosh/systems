/* NullOS Shell
 *
 * Minimal command-line shell. Reads keyboard input,
 * parses commands, and executes built-in functions.
 */

#include <kernel/shell.h>
#include <kernel/keyboard.h>
#include <kernel/vga.h>
#include <kernel/timer.h>
#include <lib/string.h>
#include <lib/printf.h>
#include <system.h>

#define LINE_BUF_SIZE  256

static void shell_execute(const char *line) {
    if (strcmp(line, "help") == 0) {
        kprintf("Commands: help  clear  echo [text]  time  reboot\n");
    } else if (strcmp(line, "clear") == 0) {
        vga_clear();
    } else if (strncmp(line, "echo ", 5) == 0) {
        kprintf("%s\n", line + 5);
    } else if (strcmp(line, "echo") == 0) {
        kprintf("\n");
    } else if (strcmp(line, "time") == 0) {
        uint32_t ticks = timer_get_ticks();
        uint32_t secs  = ticks / 100;
        uint32_t ms    = (ticks % 100) * 10;
        kprintf("Uptime: %u.%02u s (%u ticks)\n", secs, ms / 10, ticks);
    } else if (strcmp(line, "reboot") == 0) {
        kprintf("Rebooting...\n");
        /* pulse CPU reset line via keyboard controller */
        outb(0x64, 0xFE);
        /* fallback: triple fault */
        asm volatile(
            "cli\n\t"
            "lidt 0\n\t"
            "int $0\n\t"
        );
    } else if (line[0] != '\0') {
        kprintf("Unknown command: %s\n", line);
    }
}

void shell_run(void) {
    char line[LINE_BUF_SIZE];

    kprintf("\nNullOS shell ready. Type 'help' for commands.\n\n");

    for (;;) {
        kprintf("null> ");

        int pos = 0;
        for (;;) {
            char c = keyboard_getchar();

            if (c == '\n') {
                vga_putchar('\n');
                break;
            } else if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    /* erase character: backspace, space, backspace */
                    vga_putchar('\b');
                    vga_putchar(' ');
                    vga_putchar('\b');
                }
            } else if (pos < LINE_BUF_SIZE - 1) {
                line[pos++] = c;
                vga_putchar(c);
            }
        }

        line[pos] = '\0';
        shell_execute(line);
    }
}
