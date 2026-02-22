/* NullOS Minimal Printf
 *
 * Supports: %d %i %u %x %X %s %c %p %%.
 * Width + zero-padding: %08x, %10d, etc.
 * Outputs via vga_putchar (kprintf) or char buffer (ksprintf).
 */

#include <lib/printf.h>
#include <lib/stdarg.h>
#include <lib/stdint.h>
#include <kernel/vga.h>

/* --- number formatting -------------------------------------------------- */

static void uint_to_str(uint32_t val, char *buf, int base, int upper) {
    const char *lo = "0123456789abcdef";
    const char *hi = "0123456789ABCDEF";
    const char *digits = upper ? hi : lo;
    int i = 0;
    char tmp[32];

    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (val > 0) {
        tmp[i++] = digits[val % (uint32_t)base];
        val /= (uint32_t)base;
    }
    /* reverse */
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

static void int_to_str(int32_t val, char *buf) {
    if (val < 0) {
        buf[0] = '-';
        uint_to_str((uint32_t)(-val), buf + 1, 10, 0);
    } else {
        uint_to_str((uint32_t)val, buf, 10, 0);
    }
}

/* --- output contexts ----------------------------------------------------- */

typedef struct {
    char   *buf;
    int     pos;
} str_ctx;

static void vga_emit(char c, void *ctx) {
    (void)ctx;
    vga_putchar(c);
}

static void str_emit(char c, void *ctx) {
    str_ctx *s = (str_ctx *)ctx;
    s->buf[s->pos++] = c;
}

/* --- core formatter ------------------------------------------------------ */

static int fmt_core(void (*emit)(char, void *), void *ctx,
                    const char *fmt, va_list ap)
{
    int written = 0;

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            emit(*p, ctx);
            written++;
            continue;
        }

        p++; /* skip '%' */

        /* flags */
        int zero_pad = 0;
        if (*p == '0') { zero_pad = 1; p++; }

        /* width */
        int width = 0;
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p++;
        }

        char nbuf[32];
        const char *s;
        int len;

        switch (*p) {
        case 'd':
        case 'i': {
            int v = va_arg(ap, int);
            int_to_str(v, nbuf);
            s = nbuf;
            len = 0;
            while (s[len]) len++;
            char pad = zero_pad ? '0' : ' ';
            /* sign must come before zero padding */
            if (zero_pad && s[0] == '-') {
                emit('-', ctx); written++;
                s++; len--;
                width--;
            }
            while (len < width) { emit(pad, ctx); written++; width--; }
            while (*s) { emit(*s++, ctx); written++; }
            break;
        }
        case 'u': {
            uint32_t v = va_arg(ap, uint32_t);
            uint_to_str(v, nbuf, 10, 0);
            s = nbuf;
            len = 0;
            while (s[len]) len++;
            char pad = zero_pad ? '0' : ' ';
            while (len < width) { emit(pad, ctx); written++; width--; }
            while (*s) { emit(*s++, ctx); written++; }
            break;
        }
        case 'x':
        case 'X': {
            uint32_t v = va_arg(ap, uint32_t);
            uint_to_str(v, nbuf, 16, (*p == 'X'));
            s = nbuf;
            len = 0;
            while (s[len]) len++;
            char pad = zero_pad ? '0' : ' ';
            while (len < width) { emit(pad, ctx); written++; width--; }
            while (*s) { emit(*s++, ctx); written++; }
            break;
        }
        case 'p': {
            uint32_t v = (uint32_t)(uintptr_t)va_arg(ap, void *);
            uint_to_str(v, nbuf, 16, 0);
            s = nbuf;
            len = 0;
            while (s[len]) len++;
            emit('0', ctx); emit('x', ctx); written += 2;
            /* zero-pad to 8 digits */
            int pwidth = 8;
            while (len < pwidth) { emit('0', ctx); written++; pwidth--; }
            while (*s) { emit(*s++, ctx); written++; }
            break;
        }
        case 's': {
            s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            len = 0;
            while (s[len]) len++;
            char pad = zero_pad ? '0' : ' ';
            while (len < width) { emit(pad, ctx); written++; width--; }
            while (*s) { emit(*s++, ctx); written++; }
            break;
        }
        case 'c': {
            char c = (char)va_arg(ap, int);
            while (1 < width) { emit(' ', ctx); written++; width--; }
            emit(c, ctx); written++;
            break;
        }
        case '%':
            emit('%', ctx); written++;
            break;
        default:
            emit('%', ctx); emit(*p, ctx); written += 2;
            break;
        }
    }

    return written;
}

/* --- public API ---------------------------------------------------------- */

int kvprintf(const char *fmt, va_list args) {
    return fmt_core(vga_emit, 0, fmt, args);
}

int kprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = fmt_core(vga_emit, 0, fmt, ap);
    va_end(ap);
    return n;
}

int ksprintf(char *buf, const char *fmt, ...) {
    str_ctx ctx = { buf, 0 };
    va_list ap;
    va_start(ap, fmt);
    int n = fmt_core(str_emit, &ctx, fmt, ap);
    va_end(ap);
    buf[ctx.pos] = '\0';
    return n;
}
