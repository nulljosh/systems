#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Helper: print `indent` levels of indentation (2 spaces each)
// ---------------------------------------------------------------------------
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------

ASTNode* ast_create_program(void) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_PROGRAM;
    node->data.program.items = NULL;
    node->data.program.item_count = 0;
    return node;
}

ASTNode* ast_create_function(char *name, char *ret_type, int ret_ptr,
                             ASTNode **params, int param_count, ASTNode *body) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_FUNCTION;
    node->data.function.name = strdup(name);
    node->data.function.return_type = strdup(ret_type);
    node->data.function.return_ptr_level = ret_ptr;
    node->data.function.param_count = param_count;
    node->data.function.body = body;

    if (param_count > 0 && params) {
        node->data.function.params = malloc(sizeof(ASTNode*) * param_count);
        if (!node->data.function.params) {
            fprintf(stderr, "ast: out of memory\n"); exit(1);
        }
        for (int i = 0; i < param_count; i++) {
            node->data.function.params[i] = params[i];
        }
    } else {
        node->data.function.params = NULL;
    }

    return node;
}

ASTNode* ast_create_block(void) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_BLOCK;
    node->data.block.statements = NULL;
    node->data.block.stmt_count = 0;
    return node;
}

ASTNode* ast_create_return(ASTNode *value) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_RETURN;
    node->data.return_stmt.value = value;
    return node;
}

ASTNode* ast_create_if(ASTNode *cond, ASTNode *then_body, ASTNode *else_body) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_IF;
    node->data.if_stmt.condition = cond;
    node->data.if_stmt.then_body = then_body;
    node->data.if_stmt.else_body = else_body;
    return node;
}

ASTNode* ast_create_while(ASTNode *cond, ASTNode *body) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_WHILE;
    node->data.while_stmt.condition = cond;
    node->data.while_stmt.body = body;
    return node;
}

ASTNode* ast_create_for(ASTNode *init, ASTNode *cond, ASTNode *update, ASTNode *body) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_FOR;
    node->data.for_stmt.init = init;
    node->data.for_stmt.condition = cond;
    node->data.for_stmt.update = update;
    node->data.for_stmt.body = body;
    return node;
}

ASTNode* ast_create_break(void) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_BREAK;
    return node;
}

ASTNode* ast_create_var_decl(char *name, char *type, int ptr_level,
                             int array_size, ASTNode *init) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_VAR_DECL;
    node->data.var_decl.name = strdup(name);
    node->data.var_decl.var_type = strdup(type);
    node->data.var_decl.ptr_level = ptr_level;
    node->data.var_decl.array_size = array_size;
    node->data.var_decl.init = init;
    return node;
}

ASTNode* ast_create_assign(ASTNode *target, ASTNode *value) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_ASSIGN;
    node->data.assign.target = target;
    node->data.assign.value = value;
    return node;
}

ASTNode* ast_create_binary_op(const char *op, ASTNode *left, ASTNode *right) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_BINARY_OP;
    strncpy(node->data.binary_op.op, op, 3);
    node->data.binary_op.op[3] = '\0';
    node->data.binary_op.left = left;
    node->data.binary_op.right = right;
    return node;
}

ASTNode* ast_create_unary_op(char op, ASTNode *operand) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_UNARY_OP;
    node->data.unary_op.op = op;
    node->data.unary_op.operand = operand;
    return node;
}

ASTNode* ast_create_number(int value) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_NUMBER;
    node->data.number.value = value;
    return node;
}

ASTNode* ast_create_identifier(char *name) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_IDENTIFIER;
    node->data.identifier.name = strdup(name);
    return node;
}

ASTNode* ast_create_call(char *name, ASTNode **args, int arg_count) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_CALL;
    node->data.call.name = strdup(name);
    node->data.call.arg_count = arg_count;

    if (arg_count > 0 && args) {
        node->data.call.args = malloc(sizeof(ASTNode*) * arg_count);
        if (!node->data.call.args) {
            fprintf(stderr, "ast: out of memory\n"); exit(1);
        }
        for (int i = 0; i < arg_count; i++) {
            node->data.call.args[i] = args[i];
        }
    } else {
        node->data.call.args = NULL;
    }

    return node;
}

ASTNode* ast_create_string(char *value) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_STRING;
    node->data.string_lit.value = strdup(value);
    return node;
}

ASTNode* ast_create_char_lit(char value) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_CHAR_LIT;
    node->data.char_lit.value = value;
    return node;
}

ASTNode* ast_create_index(ASTNode *array, ASTNode *index) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_INDEX;
    node->data.index_access.array = array;
    node->data.index_access.index = index;
    return node;
}

ASTNode* ast_create_member_access(ASTNode *object, char *member) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_MEMBER_ACCESS;
    node->data.member_access.object = object;
    node->data.member_access.member = strdup(member);
    return node;
}

ASTNode* ast_create_struct_def(char *name, char **field_names, char **field_types,
                               int *field_ptr_levels, int count) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_STRUCT_DEF;
    node->data.struct_def.name = strdup(name);
    node->data.struct_def.field_count = count;

    if (count > 0) {
        node->data.struct_def.field_names = malloc(sizeof(char*) * count);
        node->data.struct_def.field_types = malloc(sizeof(char*) * count);
        node->data.struct_def.field_ptr_levels = malloc(sizeof(int) * count);
        if (!node->data.struct_def.field_names ||
            !node->data.struct_def.field_types ||
            !node->data.struct_def.field_ptr_levels) {
            fprintf(stderr, "ast: out of memory\n"); exit(1);
        }
        for (int i = 0; i < count; i++) {
            node->data.struct_def.field_names[i] = strdup(field_names[i]);
            node->data.struct_def.field_types[i] = strdup(field_types[i]);
            node->data.struct_def.field_ptr_levels[i] = field_ptr_levels[i];
        }
    } else {
        node->data.struct_def.field_names = NULL;
        node->data.struct_def.field_types = NULL;
        node->data.struct_def.field_ptr_levels = NULL;
    }

    return node;
}

ASTNode* ast_create_enum_def(char *name, char **values, int count) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) { fprintf(stderr, "ast: out of memory\n"); exit(1); }
    node->type = AST_ENUM_DEF;
    node->data.enum_def.name = strdup(name);
    node->data.enum_def.value_count = count;

    if (count > 0 && values) {
        node->data.enum_def.values = malloc(sizeof(char*) * count);
        if (!node->data.enum_def.values) {
            fprintf(stderr, "ast: out of memory\n"); exit(1);
        }
        for (int i = 0; i < count; i++) {
            node->data.enum_def.values[i] = strdup(values[i]);
        }
    } else {
        node->data.enum_def.values = NULL;
    }

    return node;
}

// ---------------------------------------------------------------------------
// Dynamic array helpers
// ---------------------------------------------------------------------------

void ast_add_item(ASTNode *program, ASTNode *item) {
    if (!program || program->type != AST_PROGRAM || !item) return;

    int count = program->data.program.item_count;
    ASTNode **new_items = realloc(program->data.program.items,
                                  sizeof(ASTNode*) * (count + 1));
    if (!new_items) { fprintf(stderr, "ast: out of memory\n"); exit(1); }

    new_items[count] = item;
    program->data.program.items = new_items;
    program->data.program.item_count = count + 1;
}

void ast_add_statement(ASTNode *block, ASTNode *stmt) {
    if (!block || block->type != AST_BLOCK || !stmt) return;

    int count = block->data.block.stmt_count;
    ASTNode **new_stmts = realloc(block->data.block.statements,
                                   sizeof(ASTNode*) * (count + 1));
    if (!new_stmts) { fprintf(stderr, "ast: out of memory\n"); exit(1); }

    new_stmts[count] = stmt;
    block->data.block.statements = new_stmts;
    block->data.block.stmt_count = count + 1;
}

// ---------------------------------------------------------------------------
// Pretty printer
// ---------------------------------------------------------------------------

void ast_print(ASTNode *node, int indent) {
    if (!node) return;

    print_indent(indent);

    switch (node->type) {
    case AST_PROGRAM:
        printf("Program\n");
        for (int i = 0; i < node->data.program.item_count; i++) {
            ast_print(node->data.program.items[i], indent + 1);
        }
        break;

    case AST_FUNCTION: {
        printf("Function: %s(", node->data.function.name);
        for (int i = 0; i < node->data.function.param_count; i++) {
            if (i > 0) printf(", ");
            ASTNode *p = node->data.function.params[i];
            if (p && p->type == AST_VAR_DECL) {
                printf("%s", p->data.var_decl.var_type);
                for (int j = 0; j < p->data.var_decl.ptr_level; j++) {
                    printf("*");
                }
                printf(" %s", p->data.var_decl.name);
            }
        }
        printf(") -> ");
        for (int i = 0; i < node->data.function.return_ptr_level; i++) {
            printf("*");
        }
        printf("%s\n", node->data.function.return_type);
        ast_print(node->data.function.body, indent + 1);
        break;
    }

    case AST_BLOCK:
        printf("Block\n");
        for (int i = 0; i < node->data.block.stmt_count; i++) {
            ast_print(node->data.block.statements[i], indent + 1);
        }
        break;

    case AST_RETURN:
        printf("Return\n");
        if (node->data.return_stmt.value) {
            ast_print(node->data.return_stmt.value, indent + 1);
        }
        break;

    case AST_IF:
        printf("If\n");
        print_indent(indent + 1); printf("Condition:\n");
        ast_print(node->data.if_stmt.condition, indent + 2);
        print_indent(indent + 1); printf("Then:\n");
        ast_print(node->data.if_stmt.then_body, indent + 2);
        if (node->data.if_stmt.else_body) {
            print_indent(indent + 1); printf("Else:\n");
            ast_print(node->data.if_stmt.else_body, indent + 2);
        }
        break;

    case AST_WHILE:
        printf("While\n");
        print_indent(indent + 1); printf("Condition:\n");
        ast_print(node->data.while_stmt.condition, indent + 2);
        print_indent(indent + 1); printf("Body:\n");
        ast_print(node->data.while_stmt.body, indent + 2);
        break;

    case AST_FOR:
        printf("For\n");
        print_indent(indent + 1); printf("Init:\n");
        ast_print(node->data.for_stmt.init, indent + 2);
        print_indent(indent + 1); printf("Condition:\n");
        ast_print(node->data.for_stmt.condition, indent + 2);
        print_indent(indent + 1); printf("Update:\n");
        ast_print(node->data.for_stmt.update, indent + 2);
        print_indent(indent + 1); printf("Body:\n");
        ast_print(node->data.for_stmt.body, indent + 2);
        break;

    case AST_BREAK:
        printf("Break\n");
        break;

    case AST_VAR_DECL:
        printf("VarDecl: %s", node->data.var_decl.var_type);
        for (int i = 0; i < node->data.var_decl.ptr_level; i++) {
            printf("*");
        }
        printf(" %s", node->data.var_decl.name);
        if (node->data.var_decl.array_size >= 0) {
            printf("[%d]", node->data.var_decl.array_size);
        }
        printf("\n");
        if (node->data.var_decl.init) {
            print_indent(indent + 1); printf("Init:\n");
            ast_print(node->data.var_decl.init, indent + 2);
        }
        break;

    case AST_ASSIGN:
        printf("Assign\n");
        print_indent(indent + 1); printf("Target:\n");
        ast_print(node->data.assign.target, indent + 2);
        print_indent(indent + 1); printf("Value:\n");
        ast_print(node->data.assign.value, indent + 2);
        break;

    case AST_BINARY_OP:
        printf("BinaryOp: %s\n", node->data.binary_op.op);
        ast_print(node->data.binary_op.left, indent + 1);
        ast_print(node->data.binary_op.right, indent + 1);
        break;

    case AST_UNARY_OP:
        printf("UnaryOp: %c\n", node->data.unary_op.op);
        ast_print(node->data.unary_op.operand, indent + 1);
        break;

    case AST_NUMBER:
        printf("Number: %d\n", node->data.number.value);
        break;

    case AST_IDENTIFIER:
        printf("Identifier: %s\n", node->data.identifier.name);
        break;

    case AST_CALL:
        printf("Call: %s\n", node->data.call.name);
        for (int i = 0; i < node->data.call.arg_count; i++) {
            ast_print(node->data.call.args[i], indent + 1);
        }
        break;

    case AST_STRING:
        printf("String: \"%s\"\n", node->data.string_lit.value);
        break;

    case AST_CHAR_LIT:
        printf("Char: '%c'\n", node->data.char_lit.value);
        break;

    case AST_INDEX:
        printf("Index\n");
        print_indent(indent + 1); printf("Array:\n");
        ast_print(node->data.index_access.array, indent + 2);
        print_indent(indent + 1); printf("Index:\n");
        ast_print(node->data.index_access.index, indent + 2);
        break;

    case AST_MEMBER_ACCESS:
        printf("Member: .%s\n", node->data.member_access.member);
        ast_print(node->data.member_access.object, indent + 1);
        break;

    case AST_STRUCT_DEF:
        printf("StructDef: %s\n", node->data.struct_def.name);
        for (int i = 0; i < node->data.struct_def.field_count; i++) {
            print_indent(indent + 1);
            printf("%s", node->data.struct_def.field_types[i]);
            for (int j = 0; j < node->data.struct_def.field_ptr_levels[i]; j++) {
                printf("*");
            }
            printf(" %s\n", node->data.struct_def.field_names[i]);
        }
        break;

    case AST_ENUM_DEF:
        printf("EnumDef: %s\n", node->data.enum_def.name);
        for (int i = 0; i < node->data.enum_def.value_count; i++) {
            print_indent(indent + 1);
            printf("%s\n", node->data.enum_def.values[i]);
        }
        break;
    }
}

// ---------------------------------------------------------------------------
// Recursive cleanup
// ---------------------------------------------------------------------------

void ast_free(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
    case AST_PROGRAM:
        for (int i = 0; i < node->data.program.item_count; i++) {
            ast_free(node->data.program.items[i]);
        }
        free(node->data.program.items);
        break;

    case AST_FUNCTION:
        free(node->data.function.name);
        free(node->data.function.return_type);
        for (int i = 0; i < node->data.function.param_count; i++) {
            ast_free(node->data.function.params[i]);
        }
        free(node->data.function.params);
        ast_free(node->data.function.body);
        break;

    case AST_BLOCK:
        for (int i = 0; i < node->data.block.stmt_count; i++) {
            ast_free(node->data.block.statements[i]);
        }
        free(node->data.block.statements);
        break;

    case AST_RETURN:
        ast_free(node->data.return_stmt.value);
        break;

    case AST_IF:
        ast_free(node->data.if_stmt.condition);
        ast_free(node->data.if_stmt.then_body);
        ast_free(node->data.if_stmt.else_body);
        break;

    case AST_WHILE:
        ast_free(node->data.while_stmt.condition);
        ast_free(node->data.while_stmt.body);
        break;

    case AST_FOR:
        ast_free(node->data.for_stmt.init);
        ast_free(node->data.for_stmt.condition);
        ast_free(node->data.for_stmt.update);
        ast_free(node->data.for_stmt.body);
        break;

    case AST_BREAK:
        // nothing to free
        break;

    case AST_VAR_DECL:
        free(node->data.var_decl.name);
        free(node->data.var_decl.var_type);
        ast_free(node->data.var_decl.init);
        break;

    case AST_ASSIGN:
        ast_free(node->data.assign.target);
        ast_free(node->data.assign.value);
        break;

    case AST_BINARY_OP:
        ast_free(node->data.binary_op.left);
        ast_free(node->data.binary_op.right);
        break;

    case AST_UNARY_OP:
        ast_free(node->data.unary_op.operand);
        break;

    case AST_NUMBER:
        // nothing to free
        break;

    case AST_IDENTIFIER:
        free(node->data.identifier.name);
        break;

    case AST_CALL:
        free(node->data.call.name);
        for (int i = 0; i < node->data.call.arg_count; i++) {
            ast_free(node->data.call.args[i]);
        }
        free(node->data.call.args);
        break;

    case AST_STRING:
        free(node->data.string_lit.value);
        break;

    case AST_CHAR_LIT:
        // nothing to free (char is a value type)
        break;

    case AST_INDEX:
        ast_free(node->data.index_access.array);
        ast_free(node->data.index_access.index);
        break;

    case AST_MEMBER_ACCESS:
        ast_free(node->data.member_access.object);
        free(node->data.member_access.member);
        break;

    case AST_STRUCT_DEF:
        free(node->data.struct_def.name);
        for (int i = 0; i < node->data.struct_def.field_count; i++) {
            free(node->data.struct_def.field_names[i]);
            free(node->data.struct_def.field_types[i]);
        }
        free(node->data.struct_def.field_names);
        free(node->data.struct_def.field_types);
        free(node->data.struct_def.field_ptr_levels);
        break;

    case AST_ENUM_DEF:
        free(node->data.enum_def.name);
        for (int i = 0; i < node->data.enum_def.value_count; i++) {
            free(node->data.enum_def.values[i]);
        }
        free(node->data.enum_def.values);
        break;
    }

    free(node);
}
