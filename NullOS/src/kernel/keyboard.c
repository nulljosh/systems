/* NullOS PS/2 Keyboard Driver
 *
 * Handles IRQ 1 (interrupt 33). Reads scancodes from port 0x60,
 * converts to ASCII via scancode set 1, stores in ring buffer.
 */

#include <kernel/keyboard.h>
#include <kernel/irq.h>
#include <system.h>

#define KB_DATA_PORT  0x60
#define KB_BUF_SIZE   256

/* Scancode set 1 -> ASCII (unshifted), US QWERTY */
static const char sc_table[128] = {
    0,    0,                          /* 0x00, 0x01 (none, ESC) */
    '1',  '2',  '3',  '4',  '5',     /* 0x02-0x06 */
    '6',  '7',  '8',  '9',  '0',     /* 0x07-0x0B */
    '-',  '=',  '\b', '\t',          /* 0x0C-0x0F */
    'q',  'w',  'e',  'r',  't',     /* 0x10-0x14 */
    'y',  'u',  'i',  'o',  'p',     /* 0x15-0x19 */
    '[',  ']',  '\n', 0,             /* 0x1A-0x1D (ctrl) */
    'a',  's',  'd',  'f',  'g',     /* 0x1E-0x22 */
    'h',  'j',  'k',  'l',  ';',     /* 0x23-0x27 */
    '\'', '`',  0,    '\\',          /* 0x28-0x2B (lshift) */
    'z',  'x',  'c',  'v',  'b',     /* 0x2C-0x30 */
    'n',  'm',  ',',  '.',  '/',      /* 0x31-0x35 */
    0,    '*',  0,    ' ',           /* 0x36-0x39 (rshift, kp*, alt, space) */
    0,    0,    0,    0,    0,        /* 0x3A-0x3E (caps, F1-F4) */
    0,    0,    0,    0,    0,        /* 0x3F-0x43 (F5-F9) */
    0,    0,    0,    0,    0,        /* 0x44-0x48 (F10, numlock, scroll, kp7, kp8) */
    0,    0,    0,    '-',  0,        /* 0x49-0x4D (kp9, kp-, kp4, kp5, kp6) */
    0,    '+',  0,    0,    0,        /* 0x4E-0x52 (kp+, kp1, kp2, kp3, kp0) */
    0,    0,    0,    0,    0,        /* 0x53-0x57 (kp., ?, ?, F11, F12) */
    0,    0,    0,    0,    0,        /* 0x58-0x5C */
    0,    0,    0,    0,    0,        /* 0x5D-0x61 */
    0,    0,    0,    0,    0,        /* 0x62-0x66 */
    0,    0,    0,    0,    0,        /* 0x67-0x6B */
    0,    0,    0,    0,    0,        /* 0x6C-0x70 */
    0,    0,    0,    0,    0,        /* 0x71-0x75 */
    0,    0,    0,    0,    0,        /* 0x76-0x7A */
    0,    0,    0,    0,    0,        /* 0x7B-0x7F */
};

/* Scancode set 1 -> ASCII (shifted), US QWERTY */
static const char sc_shift_table[128] = {
    0,    0,                          /* 0x00, 0x01 */
    '!',  '@',  '#',  '$',  '%',     /* 0x02-0x06 */
    '^',  '&',  '*',  '(',  ')',     /* 0x07-0x0B */
    '_',  '+',  '\b', '\t',          /* 0x0C-0x0F */
    'Q',  'W',  'E',  'R',  'T',     /* 0x10-0x14 */
    'Y',  'U',  'I',  'O',  'P',     /* 0x15-0x19 */
    '{',  '}',  '\n', 0,             /* 0x1A-0x1D */
    'A',  'S',  'D',  'F',  'G',     /* 0x1E-0x22 */
    'H',  'J',  'K',  'L',  ':',     /* 0x23-0x27 */
    '"',  '~',  0,    '|',           /* 0x28-0x2B */
    'Z',  'X',  'C',  'V',  'B',     /* 0x2C-0x30 */
    'N',  'M',  '<',  '>',  '?',     /* 0x31-0x35 */
    0,    '*',  0,    ' ',           /* 0x36-0x39 */
    0,    0,    0,    0,    0,        /* 0x3A-0x3E */
    0,    0,    0,    0,    0,        /* 0x3F-0x43 */
    0,    0,    0,    0,    0,        /* 0x44-0x48 */
    0,    0,    0,    '-',  0,        /* 0x49-0x4D */
    0,    '+',  0,    0,    0,        /* 0x4E-0x52 */
    0,    0,    0,    0,    0,        /* 0x53-0x57 */
    0,    0,    0,    0,    0,        /* 0x58-0x5C */
    0,    0,    0,    0,    0,        /* 0x5D-0x61 */
    0,    0,    0,    0,    0,        /* 0x62-0x66 */
    0,    0,    0,    0,    0,        /* 0x67-0x6B */
    0,    0,    0,    0,    0,        /* 0x6C-0x70 */
    0,    0,    0,    0,    0,        /* 0x71-0x75 */
    0,    0,    0,    0,    0,        /* 0x76-0x7A */
    0,    0,    0,    0,    0,        /* 0x7B-0x7F */
};

#define SC_LSHIFT  0x2A
#define SC_RSHIFT  0x36
#define BREAK_BIT  0x80

static volatile char kb_buf[KB_BUF_SIZE];
static volatile int  kb_read_idx  = 0;
static volatile int  kb_write_idx = 0;
static volatile int  shift_state  = 0;

static void keyboard_irq(void) {
    uint8_t sc = inb(KB_DATA_PORT);

    /* track shift key */
    if ((sc & ~BREAK_BIT) == SC_LSHIFT ||
        (sc & ~BREAK_BIT) == SC_RSHIFT) {
        shift_state = (sc & BREAK_BIT) ? 0 : 1;
        return;
    }

    /* only handle make codes */
    if (sc & BREAK_BIT) return;

    char ch = shift_state ? sc_shift_table[sc] : sc_table[sc];
    if (!ch) return;

    int next = (kb_write_idx + 1) % KB_BUF_SIZE;
    if (next != kb_read_idx) {
        kb_buf[kb_write_idx] = ch;
        kb_write_idx = next;
    }
}

void keyboard_init(void) {
    irq_install_handler(1, keyboard_irq);
}

int keyboard_haskey(void) {
    return kb_read_idx != kb_write_idx;
}

char keyboard_getchar(void) {
    while (!keyboard_haskey()) {
        asm volatile("hlt");
    }
    char c = kb_buf[kb_read_idx];
    kb_read_idx = (kb_read_idx + 1) % KB_BUF_SIZE;
    return c;
}
