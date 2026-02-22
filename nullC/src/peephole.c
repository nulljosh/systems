/* Peephole optimizer for ARM64 assembly */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINES 10000
#define MAX_LINE_LEN 256

typedef struct {
    char lines[MAX_LINES][MAX_LINE_LEN];
    int count;
} AsmCode;

void trim(char *str) {
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end + 1) = '\0';
}

int is_store_to_stack(const char *line) {
    char instr[32];
    if (sscanf(line, " %s", instr) == 1) {
        return strcmp(instr, "str") == 0 && strstr(line, "[sp,");
    }
    return 0;
}

int is_load_from_stack(const char *line) {
    char instr[32];
    if (sscanf(line, " %s", instr) == 1) {
        return strcmp(instr, "ldr") == 0 && strstr(line, "[sp]");
    }
    return 0;
}

int is_nop_sequence(const char *line1, const char *line2) {
    // Pattern: str xN, [sp, #-16]! immediately followed by ldr xN, [sp], #16
    // This is a true no-op (store then load same register with nothing in between)
    char reg1[16], reg2[16];
    
    if (sscanf(line1, " str %[^,]", reg1) == 1 &&
        sscanf(line2, " ldr %[^,]", reg2) == 1 &&
        strcmp(reg1, reg2) == 0 &&
        strstr(line1, "[sp, #-16]!") &&
        strstr(line2, "[sp], #16")) {
        return 1;
    }
    return 0;
}

int optimize(AsmCode *code) {
    AsmCode optimized = {0};
    int removed = 0;
    
    for (int i = 0; i < code->count; i++) {
        // Check for true no-op: str xN / ldr xN with nothing between
        if (i < code->count - 1 && 
            is_nop_sequence(code->lines[i], code->lines[i+1])) {
            // Skip both instructions
            i += 1;
            removed++;
            continue;
        }
        
        // Keep this line
        strcpy(optimized.lines[optimized.count++], code->lines[i]);
    }
    
    // Copy optimized back
    memcpy(code, &optimized, sizeof(AsmCode));
    return removed;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.s> <output.s>\n", argv[0]);
        return 1;
    }
    
    FILE *in = fopen(argv[1], "r");
    if (!in) {
        fprintf(stderr, "Cannot open %s\n", argv[1]);
        return 1;
    }
    
    AsmCode code = {0};
    while (fgets(code.lines[code.count], MAX_LINE_LEN, in) && code.count < MAX_LINES) {
        trim(code.lines[code.count]);
        code.count++;
    }
    fclose(in);
    
    int removed = optimize(&code);
    
    FILE *out = fopen(argv[2], "w");
    if (!out) {
        fprintf(stderr, "Cannot create %s\n", argv[2]);
        return 1;
    }
    
    for (int i = 0; i < code.count; i++) {
        fprintf(out, "%s\n", code.lines[i]);
    }
    fclose(out);
    
    if (removed > 0) {
        printf("Peephole: removed %d redundant instruction pair(s)\n", removed);
    }
    
    return 0;
}
