#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define NUM_REGS 8
#define MEM_SIZE 65536

typedef struct {
    uint16_t regs[NUM_REGS];
    uint16_t pc;
    uint16_t sp;
    uint8_t flags;
    uint8_t memory[MEM_SIZE];
    int halted;
} cpu_t;

void cpu_init(cpu_t *cpu);
void cpu_step(cpu_t *cpu);
void cpu_run(cpu_t *cpu);

#endif
