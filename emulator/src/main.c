#include <stdio.h>
#include "chip8.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: chip8 <rom.ch8>\n");
        return 1;
    }
    chip8_t c8;
    chip8_init(&c8);
    if (chip8_load(&c8, argv[1]) != 0) {
        fprintf(stderr, "Failed to load ROM\n");
        return 1;
    }
    printf("Running: %s\n", argv[1]);
    /* TODO: main loop with display and input */
    return 0;
}
