// codegen.c — ARM64 assembly code generator for macOS (Darwin)
// Targets AArch64 AAPCS64 calling convention
// All values are 8 bytes (int, char, pointer, enum)

#include "codegen.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static void codegen_function(CodeGen *cg, ASTNode *node);
static void codegen_block(CodeGen *cg, ASTNode *node);
static void codegen_stmt(CodeGen *cg, ASTNode *node);
static void codegen_expr(CodeGen *cg, ASTNode *node);
static void codegen_lvalue(CodeGen *cg, ASTNode *node);
static void collect_strings(CodeGen *cg, ASTNode *node);
static int is_struct_type(const char *type);
static const char* struct_name_from_type(const char *type);
static int struct_size(CodeGen *cg, const char *struct_name);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void emit(CodeGen *cg, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(cg->out, fmt, ap);
    va_end(ap);
    fprintf(cg->out, "\n");
}

static int align16(int n) {
    return (n + 15) & ~15;
}

static int new_label(CodeGen *cg) {
    return cg->label_count++;
}

static int find_local(CodeGen *cg, const char *name) {
    for (int i = cg->local_count - 1; i >= 0; i--) {
        if (strcmp(cg->locals[i].name, name) == 0)
            return i;
    }
    return -1;
}

static int find_struct(CodeGen *cg, const char *name) {
    for (int i = 0; i < cg->struct_count; i++) {
        if (strcmp(cg->structs[i].name, name) == 0)
            return i;
    }
    return -1;
}

// Return byte offset of a field within a struct.
// Each field is 8 bytes wide, so field at index i is at offset i*8.
static int find_field_offset(CodeGen *cg, const char *struct_name, const char *field_name) {
    int si = find_struct(cg, struct_name);
    if (si < 0) {
        fprintf(stderr, "codegen error: unknown struct '%s'\n", struct_name);
        exit(1);
    }
    StructInfo *s = &cg->structs[si];
    int offset = 0;
    for (int i = 0; i < s->field_count; i++) {
        if (strcmp(s->field_names[i], field_name) == 0)
            return offset;
        
        // Calculate size of this field and add to offset
        const char *ftype = s->field_types[i];
        int field_size = 8; // Default for scalars
        if (is_struct_type(ftype)) {
            const char *nested_name = struct_name_from_type(ftype);
            field_size = struct_size(cg, nested_name);
        }
        offset += field_size;
    }
    fprintf(stderr, "codegen error: struct '%s' has no field '%s'\n",
            struct_name, field_name);
    exit(1);
}

// Get the type name of a field within a struct.
static const char* find_field_type(CodeGen *cg, const char *struct_name, const char *field_name) {
    int si = find_struct(cg, struct_name);
    if (si < 0) return NULL;
    StructInfo *s = &cg->structs[si];
    for (int i = 0; i < s->field_count; i++) {
        if (strcmp(s->field_names[i], field_name) == 0)
            return s->field_types[i];
    }
    return NULL;
}

// Search all enums for a value name. Return its integer value (index).
static int find_enum_value(CodeGen *cg, const char *name) {
    for (int i = 0; i < cg->enum_count; i++) {
        EnumInfo *e = &cg->enums[i];
        for (int j = 0; j < e->value_count; j++) {
            if (strcmp(e->values[j], name) == 0)
                return j;
        }
    }
    return -1;
}

// Register a string literal if not already present. Returns its label index.
static int register_string(CodeGen *cg, const char *value) {
    for (int i = 0; i < cg->string_count; i++) {
        if (strcmp(cg->strings[i].value, value) == 0)
            return cg->strings[i].label;
    }
    int lbl = cg->string_count;
    cg->strings[cg->string_count].value = strdup(value);
    cg->strings[cg->string_count].label = lbl;
    cg->string_count++;
    return lbl;
}

// Add a local variable. Returns its index.
static int add_local(CodeGen *cg, const char *name, const char *type,
                     int ptr_level, int array_size, int size_bytes) {
    cg->stack_offset -= size_bytes;
    int idx = cg->local_count++;
    cg->locals[idx].name = strdup(name);
    cg->locals[idx].offset = cg->stack_offset;
    cg->locals[idx].size = size_bytes;
    strncpy(cg->locals[idx].type, type ? type : "", 63);
    cg->locals[idx].type[63] = '\0';
    cg->locals[idx].ptr_level = ptr_level;
    cg->locals[idx].array_size = array_size;
    return idx;
}

// Check if a type string refers to a struct (starts with "struct ")
static int is_struct_type(const char *type) {
    return type && strncmp(type, "struct ", 7) == 0;
}

// Extract struct name from "struct Foo" -> "Foo"
static const char* struct_name_from_type(const char *type) {
    if (type && strncmp(type, "struct ", 7) == 0)
        return type + 7;
    return type;
}

// Get size of a struct by name (field_count * 8)
static int struct_size(CodeGen *cg, const char *struct_name) {
    int si = find_struct(cg, struct_name);
    if (si < 0) return 8;
    return cg->structs[si].total_size;
}

// Resolve the type of an expression node (best effort, used for member access chains)
// Returns a static buffer or a string from the local/struct tables.
static const char* resolve_expr_type(CodeGen *cg, ASTNode *node) {
    if (!node) return "int";
    switch (node->type) {
        case AST_IDENTIFIER: {
            int li = find_local(cg, node->data.identifier.name);
            if (li >= 0) return cg->locals[li].type;
            return "int";
        }
        case AST_MEMBER_ACCESS: {
            const char *obj_type = resolve_expr_type(cg, node->data.member_access.object);
            if (is_struct_type(obj_type)) {
                const char *sname = struct_name_from_type(obj_type);
                const char *ft = find_field_type(cg, sname, node->data.member_access.member);
                if (ft) return ft;
            }
            return "int";
        }
        case AST_INDEX: {
            const char *arr_type = resolve_expr_type(cg, node->data.index_access.array);
            // Indexing into a pointer/array strips one level — just return "int" for simplicity
            // unless we can detect it's a char array
            if (arr_type && strcmp(arr_type, "char") == 0) return "char";
            return "int";
        }
        case AST_UNARY_OP: {
            if (node->data.unary_op.op == '*') {
                // Dereference — underlying type
                return resolve_expr_type(cg, node->data.unary_op.operand);
            }
            return "int";
        }
        default:
            return "int";
    }
}

// Check if a local is a struct value (not a pointer to struct)
static int local_is_struct_value(CodeGen *cg, int li) {
    return is_struct_type(cg->locals[li].type) && cg->locals[li].ptr_level == 0
           && cg->locals[li].array_size < 0;
}

// arg register names for ARM64 AAPCS64
static const char *arg_regs[] = { "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7" };

// ---------------------------------------------------------------------------
// String literal collection — first pass over entire AST
// ---------------------------------------------------------------------------

static void collect_strings(CodeGen *cg, ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->data.program.item_count; i++)
                collect_strings(cg, node->data.program.items[i]);
            break;
        case AST_FUNCTION:
            for (int i = 0; i < node->data.function.param_count; i++)
                collect_strings(cg, node->data.function.params[i]);
            collect_strings(cg, node->data.function.body);
            break;
        case AST_BLOCK:
            for (int i = 0; i < node->data.block.stmt_count; i++)
                collect_strings(cg, node->data.block.statements[i]);
            break;
        case AST_RETURN:
            collect_strings(cg, node->data.return_stmt.value);
            break;
        case AST_IF:
            collect_strings(cg, node->data.if_stmt.condition);
            collect_strings(cg, node->data.if_stmt.then_body);
            collect_strings(cg, node->data.if_stmt.else_body);
            break;
        case AST_WHILE:
            collect_strings(cg, node->data.while_stmt.condition);
            collect_strings(cg, node->data.while_stmt.body);
            break;
        case AST_FOR:
            collect_strings(cg, node->data.for_stmt.init);
            collect_strings(cg, node->data.for_stmt.condition);
            collect_strings(cg, node->data.for_stmt.update);
            collect_strings(cg, node->data.for_stmt.body);
            break;
        case AST_VAR_DECL:
            collect_strings(cg, node->data.var_decl.init);
            break;
        case AST_ASSIGN:
            collect_strings(cg, node->data.assign.target);
            collect_strings(cg, node->data.assign.value);
            break;
        case AST_BINARY_OP:
            collect_strings(cg, node->data.binary_op.left);
            collect_strings(cg, node->data.binary_op.right);
            break;
        case AST_UNARY_OP:
            collect_strings(cg, node->data.unary_op.operand);
            break;
        case AST_CALL:
            for (int i = 0; i < node->data.call.arg_count; i++)
                collect_strings(cg, node->data.call.args[i]);
            break;
        case AST_STRING:
            register_string(cg, node->data.string_lit.value);
            break;
        case AST_INDEX:
            collect_strings(cg, node->data.index_access.array);
            collect_strings(cg, node->data.index_access.index);
            break;
        case AST_MEMBER_ACCESS:
            collect_strings(cg, node->data.member_access.object);
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// Emit string data section
// ---------------------------------------------------------------------------

static void emit_data_section(CodeGen *cg) {
    if (cg->string_count == 0) return;
    emit(cg, ".section __DATA,__data");
    for (int i = 0; i < cg->string_count; i++) {
        emit(cg, ".str%d:", cg->strings[i].label);
        emit(cg, "    .asciz \"%s\"", cg->strings[i].value);
    }
    emit(cg, "");
}

// ---------------------------------------------------------------------------
// lvalue codegen — leaves ADDRESS in %rax
// ---------------------------------------------------------------------------

static void codegen_lvalue(CodeGen *cg, ASTNode *node) {
    switch (node->type) {
        case AST_IDENTIFIER: {
            int li = find_local(cg, node->data.identifier.name);
            if (li < 0) {
                fprintf(stderr, "codegen error: undefined variable '%s'\n",
                        node->data.identifier.name);
                exit(1);
            }
            emit(cg, "    add x0, x29, #%d", cg->locals[li].offset);
            break;
        }

        case AST_INDEX: {
            // Compute base address
            ASTNode *arr = node->data.index_access.array;
            ASTNode *idx = node->data.index_access.index;

            // If the array expression is an identifier, check if it's an array local
            // (which already holds the address of element 0)
            if (arr->type == AST_IDENTIFIER) {
                int li = find_local(cg, arr->data.identifier.name);
                if (li >= 0 && cg->locals[li].array_size >= 0) {
                    // Array local: offset points to first element.
                    // Load the address of element 0.
                    emit(cg, "    add x0, x29, #%d", cg->locals[li].offset);
                } else {
                    // Pointer local: load the pointer value
                    codegen_expr(cg, arr);
                }
            } else {
                // General expression — evaluate to get base address
                codegen_expr(cg, arr);
            }

            // Save base address
            emit(cg, "    str x0, [sp, #-16]!");

            // Evaluate index
            codegen_expr(cg, idx);

            // rax = index, compute address = base + index * 8
            emit(cg, "    lsl x0, x0, #3");
            emit(cg, "    ldr x9, [sp], #16");
emit(cg, "    add x0, x0, x9");
            break;
        }

        case AST_MEMBER_ACCESS: {
            // Compute address of the object, then add field offset
            ASTNode *obj = node->data.member_access.object;
            const char *member = node->data.member_access.member;

            // Get the type of the object to find field offset
            const char *obj_type = resolve_expr_type(cg, obj);
            const char *sname = struct_name_from_type(obj_type);
            int offset = find_field_offset(cg, sname, member);

            // Get address of the object
            codegen_lvalue(cg, obj);

            // Add field offset
            if (offset != 0) {
                emit(cg, "    add x0, x0, #%d", offset);
            }
            break;
        }

        case AST_UNARY_OP: {
            if (node->data.unary_op.op == '*') {
                // *ptr — the lvalue address is the value of the pointer expression
                codegen_expr(cg, node->data.unary_op.operand);
            } else {
                fprintf(stderr, "codegen error: invalid lvalue (unary op '%c')\n",
                        node->data.unary_op.op);
                exit(1);
            }
            break;
        }

        default:
            fprintf(stderr, "codegen error: invalid lvalue (node type %d)\n", node->type);
            exit(1);
    }
}

// ---------------------------------------------------------------------------
// Expression codegen — leaves VALUE in %rax
// ---------------------------------------------------------------------------

static void codegen_expr(CodeGen *cg, ASTNode *node) {
    if (!node) {
        // Null expression (e.g. void return) — just zero rax
        emit(cg, "    mov x0, #0");
        return;
    }

    switch (node->type) {
        case AST_NUMBER:
            emit(cg, "    mov x0, #%d", node->data.number.value);
            break;

        case AST_CHAR_LIT:
            emit(cg, "    mov x0, #%d", (int)(unsigned char)node->data.char_lit.value);
            break;

        case AST_STRING: {
            int lbl = register_string(cg, node->data.string_lit.value);
            emit(cg, "    adrp x0, .str%d@PAGE", lbl);
            emit(cg, "    add x0, x0, .str%d@PAGEOFF", lbl);
            break;
        }

        case AST_IDENTIFIER: {
            // Check if it's an enum value first
            int ev = find_enum_value(cg, node->data.identifier.name);
            if (ev >= 0) {
                emit(cg, "    mov x0, #%d", ev);
                break;
            }

            int li = find_local(cg, node->data.identifier.name);
            if (li < 0) {
                fprintf(stderr, "codegen error: undefined variable '%s'\n",
                        node->data.identifier.name);
                exit(1);
            }

            // If it's an array, return the address of the first element
            if (cg->locals[li].array_size >= 0) {
                emit(cg, "    add x0, x29, #%d", cg->locals[li].offset);
            }
            // If it's a struct value, return the address (structs are passed by address)
            else if (local_is_struct_value(cg, li)) {
                emit(cg, "    add x0, x29, #%d", cg->locals[li].offset);
            }
            else {
                // Normal scalar: load value
                emit(cg, "    ldr x0, [x29, #%d]", cg->locals[li].offset);
            }
            break;
        }

        case AST_BINARY_OP: {
            const char *op = node->data.binary_op.op;

            // Short-circuit logical AND
            if (strcmp(op, "&&") == 0) {
                int lbl_false = new_label(cg);
                int lbl_end = new_label(cg);

                codegen_expr(cg, node->data.binary_op.left);
                emit(cg, "    cmp x0, #0");
                emit(cg, "    b.eq .L%d", lbl_false);

                codegen_expr(cg, node->data.binary_op.right);
                emit(cg, "    cmp x0, #0");
                emit(cg, "    b.eq .L%d", lbl_false);

                emit(cg, "    mov x0, #1");
                emit(cg, "    b .L%d", lbl_end);
                emit(cg, ".L%d:", lbl_false);
                emit(cg, "    mov x0, #0");
                emit(cg, ".L%d:", lbl_end);
                break;
            }

            // Short-circuit logical OR
            if (strcmp(op, "||") == 0) {
                int lbl_true = new_label(cg);
                int lbl_end = new_label(cg);

                codegen_expr(cg, node->data.binary_op.left);
                emit(cg, "    cmp x0, #0");
                emit(cg, "    b.ne .L%d", lbl_true);

                codegen_expr(cg, node->data.binary_op.right);
                emit(cg, "    cmp x0, #0");
                emit(cg, "    b.ne .L%d", lbl_true);

                emit(cg, "    mov x0, #0");
                emit(cg, "    b .L%d", lbl_end);
                emit(cg, ".L%d:", lbl_true);
                emit(cg, "    mov x0, #1");
                emit(cg, ".L%d:", lbl_end);
                break;
            }

            // General binary ops: eval left, push, eval right, pop left into rdi
            codegen_expr(cg, node->data.binary_op.left);
            emit(cg, "    str x0, [sp, #-16]!");
            codegen_expr(cg, node->data.binary_op.right);
            emit(cg, "    ldr x9, [sp], #16");
            // Now: left in %rdi, right in %rax

            if (strcmp(op, "+") == 0) {
emit(cg, "    add x0, x0, x9");
            }
            else if (strcmp(op, "-") == 0) {
                // left - right: x9 - x0
                emit(cg, "    sub x0, x9, x0");
            }
            else if (strcmp(op, "*") == 0) {
                // left * right: x9 * x0
                emit(cg, "    mul x0, x9, x0");
            }
            else if (strcmp(op, "/") == 0) {
                // left / right: x9 / x0
                emit(cg, "    sdiv x0, x9, x0");
            }
            else if (strcmp(op, "%") == 0) {
                // left % right: x9 % x0
                // ARM64: remainder = dividend - (quotient * divisor)
                emit(cg, "    sdiv x10, x9, x0");    // x10 = x9 / x0
                emit(cg, "    msub x0, x10, x0, x9");  // x0 = x9 - (x10 * x0)
            }
            else if (strcmp(op, "==") == 0) {
                emit(cg, "    cmp x9, x0");
                emit(cg, "    cset x0, eq");
            }
            else if (strcmp(op, "!=") == 0) {
                emit(cg, "    cmp x9, x0");
                emit(cg, "    cset x0, ne");
            }
            else if (strcmp(op, "<") == 0) {
                // left < right: x9 < x0
                emit(cg, "    cmp x9, x0");
                emit(cg, "    cset x0, lt");
            }
            else if (strcmp(op, ">") == 0) {
                emit(cg, "    cmp x9, x0");
                emit(cg, "    cset x0, gt");
            }
            else if (strcmp(op, "<=") == 0) {
                emit(cg, "    cmp x9, x0");
                emit(cg, "    cset x0, le");
            }
            else if (strcmp(op, ">=") == 0) {
                emit(cg, "    cmp x9, x0");
                emit(cg, "    cset x0, ge");
            }
            else {
                fprintf(stderr, "codegen error: unknown binary op '%s'\n", op);
                exit(1);
            }
            break;
        }

        case AST_UNARY_OP: {
            char op = node->data.unary_op.op;
            if (op == '-') {
                codegen_expr(cg, node->data.unary_op.operand);
                emit(cg, "    neg x0, x0");
            }
            else if (op == '!') {
                codegen_expr(cg, node->data.unary_op.operand);
                emit(cg, "    cmp x0, #0");
                emit(cg, "    cset x0, eq");
            }
            else if (op == '&') {
                // Address-of: get lvalue address
                codegen_lvalue(cg, node->data.unary_op.operand);
            }
            else if (op == '*') {
                // Dereference: evaluate pointer, then load from that address
                codegen_expr(cg, node->data.unary_op.operand);
                emit(cg, "    ldr x0, [x0]");
            }
            else {
                fprintf(stderr, "codegen error: unknown unary op '%c'\n", op);
                exit(1);
            }
            break;
        }

        case AST_CALL: {
            const char *fname = node->data.call.name;
            int argc = node->data.call.arg_count;

            // Evaluate arguments and push onto stack (left to right, we'll assign to regs after)
            // We need to be careful: evaluating args may clobber registers.
            // Strategy: evaluate each arg, push result. Then pop into arg registers.

            // For struct arguments, we need to copy the struct data, not just a pointer.
            // For now, we pass the address for struct values (simplified ABI).
            // Actually — the examples pass structs by value. In a real SysV ABI, small
            // structs go in registers. Our simplified model: pass struct by copying
            // all fields into the callee's parameter slots (which the callee treats as
            // a local struct). But since we use a simplified ABI where everything is
            // 8 bytes, we just pass the address and the callee copies from it.
            // Actually, let's just pass the struct address (leaq) and have the callee
            // copy the struct in codegen_function. This is what our simplified model does.

            for (int i = 0; i < argc; i++) {
                codegen_expr(cg, node->data.call.args[i]);
                emit(cg, "    str x0, [sp, #-16]!");
            }

            // Pop arguments into registers (first 6)
            // We pushed left-to-right, so we pop right-to-left into the correct regs
            for (int i = argc - 1; i >= 0; i--) {
                if (i < 6) {
                    emit(cg, "    ldr %s, [sp], #16", arg_regs[i]);
                } else {
                    // Args beyond 6 stay on stack — but we pushed them already.
                    // Actually, for SysV ABI, args beyond 6 go on the stack in
                    // right-to-left order. Our push order is left-to-right, which
                    // means arg[0] is at top. We need to rearrange.
                    // For simplicity (and since our examples use <= 6 args), just pop
                    // and discard the excess; they'll be on the stack in wrong order.
                    // TODO: handle >6 args properly if needed.
                    emit(cg, "    ldr x0, [sp], #16");
                }
            }

            // Ensure 16-byte stack alignment before call.
            // After our prologue's str %rbp, rsp is 16-byte aligned if we entered
            // aligned. Our sub for locals is aligned to 16. Each str/ldr pair
            // we've done above should be balanced. So we should be aligned here.
            // If we have an odd number of pushes on the stack (from nested expressions),
            // we could be misaligned. We handle this by noting that after all pops above,
            // the stack should be back where it was.

            emit(cg, "    bl _%s", fname);

            // Result is in %rax already
            break;
        }

        case AST_INDEX: {
            // array[index] — compute address, then load value
            codegen_lvalue(cg, node);
            emit(cg, "    ldr x0, [x0]");
            break;
        }

        case AST_MEMBER_ACCESS: {
            // obj.field — compute address, then load value
            // But if the field is itself a struct, we want the address, not a load.
            const char *obj_type = resolve_expr_type(cg, node->data.member_access.object);
            const char *sname = struct_name_from_type(obj_type);
            const char *ft = find_field_type(cg, sname, node->data.member_access.member);

            codegen_lvalue(cg, node);

            if (ft && is_struct_type(ft)) {
                // Leave address in rax (struct value — caller will use as address)
            } else {
                // Scalar field — load the value
                emit(cg, "    ldr x0, [x0]");
            }
            break;
        }

        default:
            fprintf(stderr, "codegen error: unhandled expression type %d\n", node->type);
            exit(1);
    }
}

// ---------------------------------------------------------------------------
// Statement codegen
// ---------------------------------------------------------------------------

static void codegen_stmt(CodeGen *cg, ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_RETURN: {
            if (node->data.return_stmt.value) {
                codegen_expr(cg, node->data.return_stmt.value);
            } else {
                emit(cg, "    mov x0, #0");
            }
            // Epilogue
            emit(cg, "    mov sp, x29");
    emit(cg, "    ldp x29, x30, [sp], #16");
    emit(cg, "    ret");
            break;
        }

        case AST_IF: {
            int lbl_else = new_label(cg);
            int lbl_end = new_label(cg);

            codegen_expr(cg, node->data.if_stmt.condition);
            emit(cg, "    cmp x0, #0");

            if (node->data.if_stmt.else_body) {
                emit(cg, "    b.eq .L%d", lbl_else);
            } else {
                emit(cg, "    b.eq .L%d", lbl_end);
            }

            // Then body
            if (node->data.if_stmt.then_body->type == AST_BLOCK)
                codegen_block(cg, node->data.if_stmt.then_body);
            else
                codegen_stmt(cg, node->data.if_stmt.then_body);

            if (node->data.if_stmt.else_body) {
                emit(cg, "    b .L%d", lbl_end);
                emit(cg, ".L%d:", lbl_else);

                ASTNode *eb = node->data.if_stmt.else_body;
                if (eb->type == AST_BLOCK)
                    codegen_block(cg, eb);
                else if (eb->type == AST_IF)
                    codegen_stmt(cg, eb);  // else if chain
                else
                    codegen_stmt(cg, eb);
            }

            emit(cg, ".L%d:", lbl_end);
            break;
        }

        case AST_WHILE: {
            int lbl_start = new_label(cg);
            int lbl_end = new_label(cg);

            // Push break label
            cg->break_labels[cg->break_depth++] = lbl_end;

            emit(cg, ".L%d:", lbl_start);
            codegen_expr(cg, node->data.while_stmt.condition);
            emit(cg, "    cmp x0, #0");
            emit(cg, "    b.eq .L%d", lbl_end);

            if (node->data.while_stmt.body->type == AST_BLOCK)
                codegen_block(cg, node->data.while_stmt.body);
            else
                codegen_stmt(cg, node->data.while_stmt.body);

            emit(cg, "    b .L%d", lbl_start);
            emit(cg, ".L%d:", lbl_end);

            cg->break_depth--;
            break;
        }

        case AST_FOR: {
            int lbl_start = new_label(cg);
            int lbl_end = new_label(cg);

            // Push break label
            cg->break_labels[cg->break_depth++] = lbl_end;

            // Init
            if (node->data.for_stmt.init)
                codegen_stmt(cg, node->data.for_stmt.init);

            emit(cg, ".L%d:", lbl_start);

            // Condition
            if (node->data.for_stmt.condition) {
                codegen_expr(cg, node->data.for_stmt.condition);
                emit(cg, "    cmp x0, #0");
                emit(cg, "    b.eq .L%d", lbl_end);
            }

            // Body
            if (node->data.for_stmt.body->type == AST_BLOCK)
                codegen_block(cg, node->data.for_stmt.body);
            else
                codegen_stmt(cg, node->data.for_stmt.body);

            // Update
            if (node->data.for_stmt.update)
                codegen_stmt(cg, node->data.for_stmt.update);

            emit(cg, "    b .L%d", lbl_start);
            emit(cg, ".L%d:", lbl_end);

            cg->break_depth--;
            break;
        }

        case AST_BREAK: {
            if (cg->break_depth <= 0) {
                fprintf(stderr, "codegen error: break outside of loop\n");
                exit(1);
            }
            emit(cg, "    b .L%d", cg->break_labels[cg->break_depth - 1]);
            break;
        }

        case AST_VAR_DECL: {
            const char *name = node->data.var_decl.name;
            const char *type = node->data.var_decl.var_type;
            int ptr_level = node->data.var_decl.ptr_level;
            int array_size = node->data.var_decl.array_size;

            int size_bytes;
            if (array_size >= 0) {
                // Array: allocate array_size * 8 bytes
                size_bytes = array_size * 8;
                if (size_bytes == 0) size_bytes = 8; // minimum
            } else if (is_struct_type(type) && ptr_level == 0) {
                // Struct value: allocate field_count * 8 bytes
                const char *sname = struct_name_from_type(type);
                size_bytes = struct_size(cg, sname);
            } else {
                // Scalar (int, char, pointer, enum): 8 bytes
                size_bytes = 8;
            }

            int li = add_local(cg, name, type, ptr_level, array_size, size_bytes);

            // Initialize if there's an initializer
            if (node->data.var_decl.init) {
                ASTNode *init = node->data.var_decl.init;

                if (is_struct_type(type) && ptr_level == 0) {
                    // Struct initialization: not handled via simple assignment
                    // (structs don't have direct initializers in our examples)
                    codegen_expr(cg, init);
                    emit(cg, "    str x0, [x29, #%d]", cg->locals[li].offset);
                } else {
                    codegen_expr(cg, init);
                    emit(cg, "    str x0, [x29, #%d]", cg->locals[li].offset);
                }
            }
            break;
        }

        case AST_ASSIGN: {
            ASTNode *target = node->data.assign.target;
            ASTNode *value = node->data.assign.value;

            // Evaluate the address of the target
            codegen_lvalue(cg, target);
            emit(cg, "    str x0, [sp, #-16]!");

            // Evaluate the value
            codegen_expr(cg, value);

            // Store value at target address
            emit(cg, "    ldr x9, [sp], #16");
            emit(cg, "    str x0, [x9]");
            break;
        }

        case AST_BLOCK:
            codegen_block(cg, node);
            break;

        // Expression statement (e.g., function call as statement)
        case AST_CALL:
        case AST_BINARY_OP:
        case AST_UNARY_OP:
        case AST_IDENTIFIER:
        case AST_NUMBER:
            codegen_expr(cg, node);
            break;

        case AST_STRUCT_DEF: {
            // Register the struct definition — no code emitted
            int si = cg->struct_count++;
            StructInfo *s = &cg->structs[si];
            s->name = strdup(node->data.struct_def.name);
            s->field_count = node->data.struct_def.field_count;
            
            // Calculate total size by summing field sizes
            s->total_size = 0;
            for (int i = 0; i < s->field_count; i++) {
                s->field_names[i] = strdup(node->data.struct_def.field_names[i]);
                // Build the full type string, including "struct " prefix if needed
                // The parser stores field types as-is (e.g. "int", "struct Point")
                s->field_types[i] = strdup(node->data.struct_def.field_types[i]);
                
                // Calculate field size
                const char *ftype = s->field_types[i];
                int field_size = 8; // Default for scalars
                if (is_struct_type(ftype)) {
                    // Nested struct - look up its size
                    const char *nested_name = struct_name_from_type(ftype);
                    field_size = struct_size(cg, nested_name);
                }
                s->total_size += field_size;
            }
            break;
        }

        case AST_ENUM_DEF: {
            // Register the enum definition — no code emitted
            int ei = cg->enum_count++;
            EnumInfo *e = &cg->enums[ei];
            e->name = strdup(node->data.enum_def.name);
            e->value_count = node->data.enum_def.value_count;
            for (int i = 0; i < e->value_count; i++) {
                e->values[i] = strdup(node->data.enum_def.values[i]);
            }
            break;
        }

        default:
            fprintf(stderr, "codegen error: unhandled statement type %d\n", node->type);
            exit(1);
    }
}

// ---------------------------------------------------------------------------
// Block codegen
// ---------------------------------------------------------------------------

static void codegen_block(CodeGen *cg, ASTNode *node) {
    if (!node) return;
    if (node->type != AST_BLOCK) {
        codegen_stmt(cg, node);
        return;
    }

    // Save local count for scope cleanup (block-scoped variables)
    int saved_local_count = cg->local_count;
    (void)cg->stack_offset; // stack not restored per block

    for (int i = 0; i < node->data.block.stmt_count; i++) {
        codegen_stmt(cg, node->data.block.statements[i]);
    }

    // Restore scope (variables declared in this block go out of scope)
    // Free names of locals that are going out of scope
    for (int i = saved_local_count; i < cg->local_count; i++) {
        free(cg->locals[i].name);
    }
    cg->local_count = saved_local_count;
    // Note: we do NOT restore stack_offset because the function prologue
    // allocates all space upfront. The stack offset monotonically decreases
    // and is used for allocation tracking. The actual rsp is set once in the
    // prologue. Restoring would cause overlapping allocations in loops.
}

// ---------------------------------------------------------------------------
// Count max stack space needed by walking the function body
// ---------------------------------------------------------------------------

// Walk a function body to pre-allocate all locals.
// We need to know the total stack space before emitting the prologue.
// Strategy: do a dry run of codegen_stmt for var_decls only, tracking max offset.
// But that's complex. Instead, we walk the AST and add up all var_decl sizes.
// For blocks inside loops, vars get re-allocated each iteration in a real compiler,
// but since we allocate in the prologue, we need the max. Since our block codegen
// doesn't restore stack_offset (the stack is set once), variables accumulate.
// This means our first-pass needs to mirror what codegen_stmt does.
//
// Simpler approach: we do a two-pass. First pass: walk all var_decls and compute
// total size. Second pass: actually generate code with the known frame size.
// But we'd need to assign offsets in the first pass too.
//
// Easiest approach: allocate a generous frame in the prologue (we can compute
// by walking), OR emit the prologue with a placeholder and patch later.
//
// Let's use the walk approach to count total bytes needed.

static int count_vars_size(CodeGen *cg, ASTNode *node) {
    if (!node) return 0;
    int total = 0;

    switch (node->type) {
        case AST_BLOCK:
            for (int i = 0; i < node->data.block.stmt_count; i++)
                total += count_vars_size(cg, node->data.block.statements[i]);
            break;
        case AST_VAR_DECL: {
            int array_size = node->data.var_decl.array_size;
            const char *type = node->data.var_decl.var_type;
            int ptr_level = node->data.var_decl.ptr_level;
            if (array_size >= 0) {
                int sz = array_size * 8;
                if (sz == 0) sz = 8;
                total += sz;
            } else if (is_struct_type(type) && ptr_level == 0) {
                const char *sname = struct_name_from_type(type);
                total += struct_size(cg, sname);
            } else {
                total += 8;
            }
            break;
        }
        case AST_IF:
            total += count_vars_size(cg, node->data.if_stmt.then_body);
            total += count_vars_size(cg, node->data.if_stmt.else_body);
            break;
        case AST_WHILE:
            total += count_vars_size(cg, node->data.while_stmt.body);
            break;
        case AST_FOR:
            total += count_vars_size(cg, node->data.for_stmt.init);
            total += count_vars_size(cg, node->data.for_stmt.body);
            break;
        default:
            break;
    }
    return total;
}

// ---------------------------------------------------------------------------
// Function codegen
// ---------------------------------------------------------------------------

static void codegen_function(CodeGen *cg, ASTNode *node) {
    // Reset per-function state
    // Free any leftover local names
    for (int i = 0; i < cg->local_count; i++) {
        free(cg->locals[i].name);
    }
    cg->local_count = 0;
    cg->stack_offset = 0;
    cg->break_depth = 0;

    const char *fname = node->data.function.name;
    int param_count = node->data.function.param_count;
    ASTNode **params = node->data.function.params;
    ASTNode *body = node->data.function.body;

    // Calculate total stack space needed:
    // 1. Parameters (each 8 bytes)
    int param_space = param_count * 8;

    // 2. Local variables in body
    int body_space = count_vars_size(cg, body);

    // Extra safety margin for struct parameter copies
    // (Struct parameters are passed by address; we copy their contents into local stack slots)
    for (int i = 0; i < param_count; i++) {
        if (params[i]->type == AST_VAR_DECL) {
            const char *ptype = params[i]->data.var_decl.var_type;
            int pptr = params[i]->data.var_decl.ptr_level;
            if (is_struct_type(ptype) && pptr == 0) {
                const char *sname = struct_name_from_type(ptype);
                int ssz = struct_size(cg, sname);
                // The parameter itself needs ssz bytes instead of 8
                param_space += (ssz - 8);
            }
        }
    }

    int total_stack = align16(param_space + body_space);
    if (total_stack == 0) total_stack = 16; // minimum frame

    // Emit function label
    emit(cg, ".globl _%s", fname);
    emit(cg, "_%s:", fname);

    // Prologue
    emit(cg, "    stp x29, x30, [sp, #-16]!");
    emit(cg, "    mov x29, sp");
    emit(cg, "    sub sp, sp, #%d", total_stack);

    // Copy parameters from registers to stack slots
    for (int i = 0; i < param_count; i++) {
        if (params[i]->type != AST_VAR_DECL) continue;

        const char *pname = params[i]->data.var_decl.name;
        const char *ptype = params[i]->data.var_decl.var_type;
        int pptr = params[i]->data.var_decl.ptr_level;
        int parray = params[i]->data.var_decl.array_size;

        if (is_struct_type(ptype) && pptr == 0) {
            // Struct parameter: the register holds the address of the struct.
            // We need to copy the struct's contents into a local stack slot.
            const char *sname = struct_name_from_type(ptype);
            int ssz = struct_size(cg, sname);
            int li = add_local(cg, pname, ptype, pptr, -1, ssz);

            // Copy struct data from source address (in arg register) to local stack
            if (i < 6) {
                // Source address is in arg_regs[i]
                // Save source pointer to x9 if we're using x0 as arg register
                if (i == 0) {
                    emit(cg, "    mov x9, x0");
                }
                const char *src_reg = (i == 0) ? "x9" : arg_regs[i];
                
                // Use a loop to copy 8 bytes at a time
                int nfields = ssz / 8;
                for (int f = 0; f < nfields; f++) {
                    emit(cg, "    ldr x0, [%s, #%d]", src_reg, f * 8);
                    emit(cg, "    str x0, [x29, #%d]", cg->locals[li].offset + f * 8);
                }
            }
        } else {
            // Scalar parameter
            int li = add_local(cg, pname, ptype, pptr, parray, 8);
            if (i < 6) {
                emit(cg, "    str %s, [x29, #%d]", arg_regs[i], cg->locals[li].offset);
            }
            // TODO: handle args beyond 6 (read from stack above rbp)
        }
    }

    // Generate body
    codegen_block(cg, body);

    // If the function might fall through without a return, emit an implicit return 0.
    // (Covers void functions and functions where control might reach end.)
    emit(cg, "    mov x0, #0");
    emit(cg, "    mov sp, x29");
    emit(cg, "    ldp x29, x30, [sp], #16");
    emit(cg, "    ret");
    emit(cg, "");
}

// ---------------------------------------------------------------------------
// Top-level program codegen
// ---------------------------------------------------------------------------

// Pre-pass: register all struct and enum definitions before generating code.
// This allows functions to reference structs/enums defined later in the file.
static void register_types(CodeGen *cg, ASTNode *program) {
    for (int i = 0; i < program->data.program.item_count; i++) {
        ASTNode *item = program->data.program.items[i];
        if (item->type == AST_STRUCT_DEF) {
            int si = cg->struct_count++;
            StructInfo *s = &cg->structs[si];
            s->name = strdup(item->data.struct_def.name);
            s->field_count = item->data.struct_def.field_count;
            
            // Calculate total size by summing field sizes
            s->total_size = 0;
            for (int j = 0; j < s->field_count; j++) {
                s->field_names[j] = strdup(item->data.struct_def.field_names[j]);
                s->field_types[j] = strdup(item->data.struct_def.field_types[j]);
                
                // Calculate field size
                const char *ftype = s->field_types[j];
                int field_size = 8; // Default for scalars
                if (is_struct_type(ftype)) {
                    // Nested struct - look up its size
                    const char *nested_name = struct_name_from_type(ftype);
                    field_size = struct_size(cg, nested_name);
                }
                s->total_size += field_size;
            }
        }
        else if (item->type == AST_ENUM_DEF) {
            int ei = cg->enum_count++;
            EnumInfo *e = &cg->enums[ei];
            e->name = strdup(item->data.enum_def.name);
            e->value_count = item->data.enum_def.value_count;
            for (int j = 0; j < e->value_count; j++) {
                e->values[j] = strdup(item->data.enum_def.values[j]);
            }
        }
    }
}

void codegen_init(CodeGen *cg, FILE *out) {
    memset(cg, 0, sizeof(CodeGen));
    cg->out = out;
    cg->label_count = 0;
    cg->local_count = 0;
    cg->stack_offset = 0;
    cg->struct_count = 0;
    cg->enum_count = 0;
    cg->string_count = 0;
    cg->break_depth = 0;
}

void codegen_program(CodeGen *cg, ASTNode *program) {
    if (!program || program->type != AST_PROGRAM) {
        fprintf(stderr, "codegen error: expected AST_PROGRAM\n");
        exit(1);
    }

    // Pass 0: Register all struct and enum definitions
    register_types(cg, program);

    // Pass 1: Collect all string literals
    collect_strings(cg, program);

    // Emit data section for string literals
    emit_data_section(cg);

    // Emit text section
    emit(cg, ".section __TEXT,__text");
    emit(cg, "");

    // Pass 2: Generate code for each function
    for (int i = 0; i < program->data.program.item_count; i++) {
        ASTNode *item = program->data.program.items[i];
        if (item->type == AST_FUNCTION) {
            codegen_function(cg, item);
        }
        // Struct and enum definitions are already registered in pass 0.
        // If codegen_stmt encounters them again (shouldn't happen since we
        // skip non-functions here), they'd be handled there too, but we
        // avoid double-registration by skipping them in this loop.
    }
}
