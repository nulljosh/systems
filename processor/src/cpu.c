#include "cpu.h"
#include <string.h>

void cpu_init(cpu_t *cpu) {
    memset(cpu, 0, sizeof(cpu_t));
}

void cpu_step(cpu_t *cpu) {
    /* TODO: fetch, decode, execute */
    (void)cpu;
}

void cpu_run(cpu_t *cpu) {
    while (!cpu->halted) {
        cpu_step(cpu);
    }
}
