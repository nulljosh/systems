#include "malloc.h"
#include <unistd.h>
#include <string.h>

typedef struct block {
    size_t size;
    int free;
    struct block *next;
} block_t;

#define BLOCK_SIZE sizeof(block_t)

static block_t *head = NULL;

void *my_malloc(size_t size) {
    /* TODO: implement first-fit allocation */
    (void)size;
    return NULL;
}

void my_free(void *ptr) {
    /* TODO: mark block as free, coalesce */
    (void)ptr;
}

void *my_realloc(void *ptr, size_t size) {
    (void)ptr; (void)size;
    return NULL;
}

void *my_calloc(size_t nmemb, size_t size) {
    void *p = my_malloc(nmemb * size);
    if (p) memset(p, 0, nmemb * size);
    return p;
}
