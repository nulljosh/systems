#include "chip8.h"
#include <stdio.h>
#include <string.h>

void chip8_init(chip8_t *c8) {
    memset(c8, 0, sizeof(chip8_t));
    c8->pc = 0x200;
    /* TODO: load fontset into memory */
}

int chip8_load(chip8_t *c8, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    size_t n = fread(&c8->memory[0x200], 1, 4096 - 0x200, f);
    fclose(f);
    return n > 0 ? 0 : -1;
}

void chip8_cycle(chip8_t *c8) {
    /* TODO: fetch, decode, execute opcode */
    (void)c8;
}
