#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <stdio.h>

typedef struct {
    char *name;
    int offset;       // negative offset from rbp
    int size;         // size in bytes (8 for everything)
    char type[64];
    int ptr_level;
    int array_size;   // -1 if not array
} Local;

typedef struct {
    char *name;
    char *field_names[32];
    char *field_types[32];
    int field_count;
    int total_size;
} StructInfo;

typedef struct {
    char *name;
    char *values[64];
    int value_count;
} EnumInfo;

typedef struct {
    char *value;
    int label;
} StringLit;

typedef struct {
    FILE *out;
    int label_count;

    Local locals[512];
    int local_count;
    int stack_offset;

    StructInfo structs[32];
    int struct_count;

    EnumInfo enums[16];
    int enum_count;

    StringLit strings[256];
    int string_count;

    // Loop context for break
    int break_labels[32];
    int break_depth;
} CodeGen;

void codegen_init(CodeGen *cg, FILE *out);
void codegen_program(CodeGen *cg, ASTNode *program);

#endif // CODEGEN_H
