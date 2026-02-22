#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"

char* read_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = malloc(size + 1);
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);
    return buffer;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.c> [-o output] [--ast] [--asm]\n", argv[0]);
        return 1;
    }

    int print_ast = 0;
    int asm_only = 0;
    char *output_name = NULL;
    char *input_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ast") == 0) {
            print_ast = 1;
        } else if (strcmp(argv[i], "--asm") == 0) {
            asm_only = 1;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_name = argv[++i];
        } else {
            input_file = argv[i];
        }
    }

    if (!input_file) {
        fprintf(stderr, "Error: No input file\n");
        return 1;
    }

    char *source = read_file(input_file);
    if (!source) return 1;

    // Tokenize
    Lexer lexer;
    lexer_init(&lexer, source);

    Token tokens[4096];
    int token_count = 0;

    Token token;
    do {
        token = lexer_next_token(&lexer);
        tokens[token_count++] = token;
    } while (token.type != TOKEN_EOF && token_count < 4096);

    // Parse
    Parser parser;
    parser_init(&parser, tokens, token_count);
    ASTNode *ast = parse_program(&parser);

    if (print_ast) {
        printf("=== AST: %s ===\n\n", input_file);
        ast_print(ast, 0);
        printf("\n");
    }

    // Generate assembly
    // Derive .s filename from input
    char asm_file[256];
    if (asm_only && output_name) {
        snprintf(asm_file, sizeof(asm_file), "%s", output_name);
    } else {
        // Replace .c with .s
        strncpy(asm_file, input_file, sizeof(asm_file) - 1);
        char *dot = strrchr(asm_file, '.');
        if (dot) strcpy(dot, ".s");
        else strcat(asm_file, ".s");
    }

    FILE *out = fopen(asm_file, "w");
    if (!out) {
        fprintf(stderr, "Error: Cannot create '%s'\n", asm_file);
        ast_free(ast);
        for (int i = 0; i < token_count; i++) token_free(&tokens[i]);
        free(source);
        return 1;
    }

    CodeGen cg;
    codegen_init(&cg, out);
    codegen_program(&cg, ast);
    fclose(out);

    if (!print_ast) {
        printf("Generated: %s\n", asm_file);
    }

    // Run peephole optimization
    char optimized_asm[256];
    snprintf(optimized_asm, sizeof(optimized_asm), "%s.opt", asm_file);
    char peephole_cmd[512];
    snprintf(peephole_cmd, sizeof(peephole_cmd), "./peephole %s %s 2>&1", asm_file, optimized_asm);
    system(peephole_cmd);
    
    // Use optimized assembly if it exists
    FILE *opt_check = fopen(optimized_asm, "r");
    if (opt_check) {
        fclose(opt_check);
        // Replace original with optimized
        snprintf(peephole_cmd, sizeof(peephole_cmd), "mv %s %s", optimized_asm, asm_file);
        system(peephole_cmd);
    }

    if (!asm_only) {
        // Assemble and link
        char bin_name[256];
        if (output_name) {
            snprintf(bin_name, sizeof(bin_name), "%s", output_name);
        } else {
            strncpy(bin_name, input_file, sizeof(bin_name) - 1);
            char *dot = strrchr(bin_name, '.');
            if (dot) *dot = '\0';
            // Strip directory
            char *slash = strrchr(bin_name, '/');
            if (slash) {
                memmove(bin_name, slash + 1, strlen(slash + 1) + 1);
            }
        }

        char cmd[512];
        snprintf(cmd, sizeof(cmd), "cc -o %s %s 2>&1", bin_name, asm_file);
        printf("Compiling: %s -> %s\n", asm_file, bin_name);
        int ret = system(cmd);
        if (ret != 0) {
            fprintf(stderr, "Error: Assembly/linking failed\n");
        } else {
            printf("Success: ./%s\n", bin_name);
        }
    }

    // Cleanup
    ast_free(ast);
    for (int i = 0; i < token_count; i++) token_free(&tokens[i]);
    free(source);

    return 0;
}
