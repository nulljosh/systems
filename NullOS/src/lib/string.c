/* NullOS String Library
 *
 * Freestanding implementations of common string/memory functions.
 * No libc available -- we implement everything from scratch.
 */

#include <lib/string.h>
#include <lib/stdint.h>

/* Copy memory block */
void *memcpy(void *dest, const void *src, uint32_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    for (uint32_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

/* Set memory block to value */
void *memset(void *dest, int val, uint32_t n) {
    uint8_t *d = (uint8_t *)dest;
    uint8_t v = (uint8_t)val;
    for (uint32_t i = 0; i < n; i++) {
        d[i] = v;
    }
    return dest;
}

/* Move memory block (handles overlapping regions) */
void *memmove(void *dest, const void *src, uint32_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    
    /* If source is before destination, copy backward to avoid overlap */
    if (s < d && s + n > d) {
        for (uint32_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    } else {
        for (uint32_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    }
    return dest;
}

/* Compare memory blocks */
int memcmp(const void *a, const void *b, uint32_t n) {
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    
    for (uint32_t i = 0; i < n; i++) {
        if (pa[i] != pb[i]) {
            return pa[i] - pb[i];
        }
    }
    return 0;
}

/* Get string length */
uint32_t strlen(const char *s) {
    uint32_t len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

/* Compare strings */
int strcmp(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) {
            return (uint8_t)*a - (uint8_t)*b;
        }
        a++;
        b++;
    }
    return (uint8_t)*a - (uint8_t)*b;
}

/* Compare strings (limited) */
int strncmp(const char *a, const char *b, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return (uint8_t)a[i] - (uint8_t)b[i];
        }
        if (!a[i]) {
            break;
        }
    }
    return 0;
}

/* Copy string */
char *strcpy(char *dest, const char *src) {
    uint32_t i = 0;
    while (src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = 0;
    return dest;
}

/* Copy string (limited) */
char *strncpy(char *dest, const char *src, uint32_t n) {
    uint32_t i = 0;
    while (i < n && src[i]) {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i] = 0;
        i++;
    }
    return dest;
}

/* Concatenate strings */
char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while (*src) *d++ = *src++;
    *d = '\0';
    return dest;
}

/* Find first occurrence of character in string */
char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    if ((char)c == '\0') return (char *)s;
    return (char *)0;
}

/* Convert string to integer */
int atoi(const char *s) {
    int result = 0, neg = 0;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') {
        result = result * 10 + (*s - '0');
        s++;
    }
    return neg ? -result : result;
}

/* Convert integer to string in given base */
void itoa(int value, char *buf, int base) {
    char tmp[32];
    int i = 0, neg = 0;
    uint32_t u;

    if (base == 10 && value < 0) {
        neg = 1;
        u = -(uint32_t)value;
    } else {
        u = (uint32_t)value;
    }

    if (u == 0) {
        tmp[i++] = '0';
    } else {
        while (u > 0) {
            int d = (int)(u % (uint32_t)base);
            tmp[i++] = (d < 10) ? '0' + d : 'a' + d - 10;
            u /= (uint32_t)base;
        }
    }

    int j = 0;
    if (neg) buf[j++] = '-';
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}
