#include <stdio.h>
#include "cpu.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: cpu <program.bin>\n");
        return 1;
    }
    cpu_t cpu;
    cpu_init(&cpu);
    printf("CPU simulator loaded: %s\n", argv[1]);
    /* TODO: load program, run */
    return 0;
}
