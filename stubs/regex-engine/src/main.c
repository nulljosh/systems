#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: regex <pattern> <input>\n");
        return 1;
    }
    printf("Pattern: %s\nInput: %s\n", argv[1], argv[2]);
    /* TODO: parse, compile, match */
    return 0;
}
