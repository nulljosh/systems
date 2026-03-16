#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>

typedef struct {
    uint8_t memory[4096];
    uint8_t V[16];
    uint16_t I;
    uint16_t pc;
    uint16_t stack[16];
    uint8_t sp;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint8_t display[64 * 32];
    uint8_t keypad[16];
    int draw_flag;
} chip8_t;

void chip8_init(chip8_t *c8);
int chip8_load(chip8_t *c8, const char *path);
void chip8_cycle(chip8_t *c8);

#endif
