#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: mygit <command> [args]\n");
        return 1;
    }
    if (strcmp(argv[1], "init") == 0) {
        printf("Initialized empty repository\n");
        /* TODO: create .mygit directory structure */
    }
    return 0;
}
