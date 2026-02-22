/*
 * nullC -- Recursive Descent Parser
 *
 * Transforms a flat token array (from the lexer) into an AST.
 * Handles all 10 complexity levels: basic functions, arithmetic,
 * variables, control flow, loops, functions, recursion, pointers,
 * structs, strings, and enums.
 */

#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Forward declarations                                               */
/* ------------------------------------------------------------------ */

static ASTNode* parse_expression(Parser *p);
static ASTNode* parse_statement(Parser *p);
static ASTNode* parse_block(Parser *p);
static ASTNode* parse_var_decl(Parser *p, int expect_semi);

/* ------------------------------------------------------------------ */
/*  Helper functions                                                   */
/* ------------------------------------------------------------------ */

/* Return the current token without advancing, or NULL at end. */
static Token* peek(Parser *p) {
    if (p->pos >= p->token_count) return NULL;
    return &p->tokens[p->pos];
}

/* Return the token at pos+offset, or NULL if out of range. */
static Token* peek_ahead(Parser *p, int offset) {
    int idx = p->pos + offset;
    if (idx < 0 || idx >= p->token_count) return NULL;
    return &p->tokens[idx];
}

/* Return the current token and advance the position. */
static Token* advance(Parser *p) {
    if (p->pos >= p->token_count) return NULL;
    return &p->tokens[p->pos++];
}

/* Check whether the current token has the given type. */
static int match(Parser *p, TokenType type) {
    Token *t = peek(p);
    return t && t->type == type;
}

/* Check whether the current token has the given string value. */
static int match_value(Parser *p, const char *val) {
    Token *t = peek(p);
    return t && strcmp(t->value, val) == 0;
}

/* Advance and assert the token has the expected type.  Exit on failure. */
static Token* expect(Parser *p, TokenType type, const char *context) {
    Token *t = advance(p);
    if (!t || t->type != type) {
        fprintf(stderr, "Parse error at line %d col %d: expected %s, got '%s' (%s)\n",
                t ? t->line : -1,
                t ? t->column : -1,
                context,
                t ? t->value : "EOF",
                t ? token_type_str(t->type) : "none");
        exit(1);
    }
    return t;
}

/* Advance and assert the token has the expected string value. */
static Token* expect_value(Parser *p, const char *val, const char *context) {
    Token *t = advance(p);
    if (!t || strcmp(t->value, val) != 0) {
        fprintf(stderr, "Parse error at line %d col %d: expected '%s' (%s), got '%s'\n",
                t ? t->line : -1,
                t ? t->column : -1,
                val, context,
                t ? t->value : "EOF");
        exit(1);
    }
    return t;
}

/* Return non-zero if the current token is a type keyword (int, char, void). */
static int is_type_keyword(Parser *p) {
    Token *t = peek(p);
    if (!t || t->type != TOKEN_KEYWORD) return 0;
    return (strcmp(t->value, "int") == 0 ||
            strcmp(t->value, "char") == 0 ||
            strcmp(t->value, "void") == 0);
}

/* Return non-zero if the current token starts a type specifier
 * (type keyword, or "struct"/"enum" keyword). */
static int is_type_start(Parser *p) {
    if (is_type_keyword(p)) return 1;
    Token *t = peek(p);
    if (!t || t->type != TOKEN_KEYWORD) return 0;
    return (strcmp(t->value, "struct") == 0 ||
            strcmp(t->value, "enum") == 0);
}

/* ------------------------------------------------------------------ */
/*  Type parsing helpers                                               */
/* ------------------------------------------------------------------ */

/*
 * Parse a type specifier and return it as a malloc'd string.
 * Sets *ptr_level to the number of '*' tokens following the base type.
 *
 * Examples:
 *   "int"              -> "int",             ptr_level=0
 *   "char"             -> "char",            ptr_level=0
 *   "int" "*"          -> "int",             ptr_level=1
 *   "char" "*" "*"     -> "char",            ptr_level=2
 *   "struct" "Point"   -> "struct Point",    ptr_level=0
 *   "enum" "TokenType" -> "enum TokenType",  ptr_level=0
 */
static char* parse_type(Parser *p, int *ptr_level) {
    *ptr_level = 0;
    char type_buf[128];

    Token *t = advance(p);
    if (!t) {
        fprintf(stderr, "Parse error: expected type specifier, got EOF\n");
        exit(1);
    }

    if (strcmp(t->value, "struct") == 0 || strcmp(t->value, "enum") == 0) {
        /* "struct Name" or "enum Name" */
        Token *name = expect(p, TOKEN_IDENTIFIER, "type name after struct/enum");
        snprintf(type_buf, sizeof(type_buf), "%s %s", t->value, name->value);
    } else {
        /* Simple type: int, char, void */
        snprintf(type_buf, sizeof(type_buf), "%s", t->value);
    }

    /* Count pointer stars */
    while (match(p, TOKEN_OPERATOR) && match_value(p, "*")) {
        advance(p);
        (*ptr_level)++;
    }

    return strdup(type_buf);
}

/* ------------------------------------------------------------------ */
/*  Expression parsing -- precedence climbing (lowest to highest)      */
/* ------------------------------------------------------------------ */

/* 10. Primary: number | string | char_lit | identifier | '(' expr ')' */
static ASTNode* parse_primary(Parser *p) {
    Token *t = peek(p);
    if (!t) {
        fprintf(stderr, "Parse error: unexpected end of input in expression\n");
        exit(1);
    }

    /* Number literal */
    if (t->type == TOKEN_NUMBER) {
        advance(p);
        return ast_create_number(atoi(t->value));
    }

    /* String literal */
    if (t->type == TOKEN_STRING) {
        advance(p);
        return ast_create_string(t->value);
    }

    /* Character literal */
    if (t->type == TOKEN_CHAR_LIT) {
        advance(p);
        return ast_create_char_lit(t->value[0]);
    }

    /* Identifier (plain variable reference -- call/index/member handled in postfix) */
    if (t->type == TOKEN_IDENTIFIER) {
        advance(p);
        return ast_create_identifier(t->value);
    }

    /* Parenthesised expression */
    if (t->type == TOKEN_SEPARATOR && strcmp(t->value, "(") == 0) {
        advance(p);  /* eat '(' */
        ASTNode *expr = parse_expression(p);
        expect_value(p, ")", "closing parenthesis");
        return expr;
    }

    fprintf(stderr, "Parse error at line %d col %d: unexpected token '%s' in expression\n",
            t->line, t->column, t->value);
    exit(1);
    return NULL;
}

/* 9. Postfix: primary ( '(' args ')' | '[' expr ']' | '.' ident )* */
static ASTNode* parse_postfix(Parser *p) {
    ASTNode *node = parse_primary(p);

    while (1) {
        Token *t = peek(p);
        if (!t) break;

        /* Function call: '(' arg_list ')' */
        if (t->type == TOKEN_SEPARATOR && strcmp(t->value, "(") == 0) {
            /* The node should be an identifier holding the function name */
            if (node->type != AST_IDENTIFIER) {
                fprintf(stderr, "Parse error: call expression requires identifier\n");
                exit(1);
            }
            char *fn_name = strdup(node->data.identifier.name);
            ast_free(node);

            advance(p);  /* eat '(' */

            ASTNode **args = NULL;
            int arg_count = 0;

            if (!(peek(p) && peek(p)->type == TOKEN_SEPARATOR &&
                  strcmp(peek(p)->value, ")") == 0)) {
                /* At least one argument */
                int cap = 4;
                args = malloc(sizeof(ASTNode*) * cap);

                args[arg_count++] = parse_expression(p);
                while (match_value(p, ",")) {
                    advance(p);  /* eat ',' */
                    if (arg_count >= cap) {
                        cap *= 2;
                        args = realloc(args, sizeof(ASTNode*) * cap);
                    }
                    args[arg_count++] = parse_expression(p);
                }
            }

            expect_value(p, ")", "closing parenthesis of call");
            node = ast_create_call(fn_name, args, arg_count);
            free(fn_name);
            continue;
        }

        /* Array index: '[' expr ']' */
        if (t->type == TOKEN_SEPARATOR && strcmp(t->value, "[") == 0) {
            advance(p);  /* eat '[' */
            ASTNode *index = parse_expression(p);
            expect_value(p, "]", "closing bracket of index");
            node = ast_create_index(node, index);
            continue;
        }

        /* Member access: '.' ident */
        if (t->type == TOKEN_SEPARATOR && strcmp(t->value, ".") == 0) {
            advance(p);  /* eat '.' */
            Token *member = expect(p, TOKEN_IDENTIFIER, "member name after '.'");
            node = ast_create_member_access(node, member->value);
            continue;
        }

        break;
    }

    return node;
}

/* 8. Unary: ('*' | '&' | '-' | '!') unary | postfix */
static ASTNode* parse_unary(Parser *p) {
    Token *t = peek(p);
    if (t && t->type == TOKEN_OPERATOR) {
        if (strcmp(t->value, "*") == 0 ||
            strcmp(t->value, "&") == 0 ||
            strcmp(t->value, "-") == 0 ||
            strcmp(t->value, "!") == 0) {
            char op = t->value[0];
            advance(p);
            ASTNode *operand = parse_unary(p);
            return ast_create_unary_op(op, operand);
        }
    }
    return parse_postfix(p);
}

/* 7. Multiplicative: unary (('*' | '/') unary)* */
static ASTNode* parse_multiplicative(Parser *p) {
    ASTNode *left = parse_unary(p);

    while (match(p, TOKEN_OPERATOR) &&
           (match_value(p, "*") || match_value(p, "/"))) {
        Token *op = advance(p);
        ASTNode *right = parse_unary(p);
        left = ast_create_binary_op(op->value, left, right);
    }

    return left;
}

/* 6. Additive: multiplicative (('+' | '-') multiplicative)* */
static ASTNode* parse_additive(Parser *p) {
    ASTNode *left = parse_multiplicative(p);

    while (match(p, TOKEN_OPERATOR) &&
           (match_value(p, "+") || match_value(p, "-"))) {
        Token *op = advance(p);
        ASTNode *right = parse_multiplicative(p);
        left = ast_create_binary_op(op->value, left, right);
    }

    return left;
}

/* 5. Comparison: additive (('<' | '>' | '<=' | '>=') additive)* */
static ASTNode* parse_comparison(Parser *p) {
    ASTNode *left = parse_additive(p);

    while (match(p, TOKEN_OPERATOR) &&
           (match_value(p, "<") || match_value(p, ">") ||
            match_value(p, "<=") || match_value(p, ">="))) {
        Token *op = advance(p);
        ASTNode *right = parse_additive(p);
        left = ast_create_binary_op(op->value, left, right);
    }

    return left;
}

/* 4. Equality: comparison (('==' | '!=') comparison)* */
static ASTNode* parse_equality(Parser *p) {
    ASTNode *left = parse_comparison(p);

    while (match(p, TOKEN_OPERATOR) &&
           (match_value(p, "==") || match_value(p, "!="))) {
        Token *op = advance(p);
        ASTNode *right = parse_comparison(p);
        left = ast_create_binary_op(op->value, left, right);
    }

    return left;
}

/* 3. Logical AND: equality ('&&' equality)* */
static ASTNode* parse_logical_and(Parser *p) {
    ASTNode *left = parse_equality(p);

    while (match(p, TOKEN_OPERATOR) && match_value(p, "&&")) {
        Token *op = advance(p);
        ASTNode *right = parse_equality(p);
        left = ast_create_binary_op(op->value, left, right);
    }

    return left;
}

/* 2. Logical OR: logical_and ('||' logical_and)* */
static ASTNode* parse_logical_or(Parser *p) {
    ASTNode *left = parse_logical_and(p);

    while (match(p, TOKEN_OPERATOR) && match_value(p, "||")) {
        Token *op = advance(p);
        ASTNode *right = parse_logical_and(p);
        left = ast_create_binary_op(op->value, left, right);
    }

    return left;
}

/* 1. Expression entry point: logical_or */
static ASTNode* parse_expression(Parser *p) {
    return parse_logical_or(p);
}

/* ------------------------------------------------------------------ */
/*  Variable declaration parsing                                       */
/* ------------------------------------------------------------------ */

/*
 * Parse a variable declaration.
 *   type [*]* name [ '[' size ']' ] [ '=' init ] [';']
 *
 * When expect_semi is non-zero, the trailing ';' is consumed here.
 * When zero (e.g. for-loop init, function params), the caller handles it.
 */
static ASTNode* parse_var_decl(Parser *p, int expect_semi) {
    int ptr_level = 0;
    char *type = parse_type(p, &ptr_level);

    Token *name = expect(p, TOKEN_IDENTIFIER, "variable name");
    char *var_name = name->value;

    int array_size = -1;  /* -1 means not an array */
    ASTNode *init = NULL;

    /* Check for array declaration: '[' size ']' */
    if (match_value(p, "[")) {
        advance(p);  /* eat '[' */
        Token *size_tok = expect(p, TOKEN_NUMBER, "array size");
        array_size = atoi(size_tok->value);
        expect_value(p, "]", "closing bracket of array size");
    }

    /* Check for initialiser: '=' expression */
    if (match(p, TOKEN_OPERATOR) && match_value(p, "=")) {
        advance(p);  /* eat '=' */
        init = parse_expression(p);
    }

    if (expect_semi) {
        expect_value(p, ";", "semicolon after variable declaration");
    }

    ASTNode *node = ast_create_var_decl(var_name, type, ptr_level, array_size, init);
    free(type);
    return node;
}

/* ------------------------------------------------------------------ */
/*  Statement parsing                                                  */
/* ------------------------------------------------------------------ */

/* Parse a block: '{' statement* '}' */
static ASTNode* parse_block(Parser *p) {
    expect_value(p, "{", "opening brace of block");

    ASTNode *block = ast_create_block();

    while (peek(p) && !(peek(p)->type == TOKEN_SEPARATOR &&
           strcmp(peek(p)->value, "}") == 0)) {
        ASTNode *stmt = parse_statement(p);
        if (stmt) {
            ast_add_statement(block, stmt);
        }
    }

    expect_value(p, "}", "closing brace of block");
    return block;
}

/* Parse a body: either a braced block or a single statement */
static ASTNode* parse_body(Parser *p) {
    if (peek(p) && peek(p)->type == TOKEN_SEPARATOR &&
        strcmp(peek(p)->value, "{") == 0) {
        return parse_block(p);
    }
    /* Single statement — wrap in a block */
    ASTNode *block = ast_create_block();
    ASTNode *stmt = parse_statement(p);
    if (stmt) {
        ast_add_statement(block, stmt);
    }
    return block;
}

/* Parse: "return" [expression] ";" */
static ASTNode* parse_return(Parser *p) {
    advance(p);  /* eat "return" */

    ASTNode *value = NULL;
    if (!match_value(p, ";")) {
        value = parse_expression(p);
    }

    expect_value(p, ";", "semicolon after return");
    return ast_create_return(value);
}

/* Parse: "if" '(' expression ')' body ["else" ("if" ... | body)] */
static ASTNode* parse_if(Parser *p) {
    advance(p);  /* eat "if" */
    expect_value(p, "(", "opening parenthesis of if condition");
    ASTNode *cond = parse_expression(p);
    expect_value(p, ")", "closing parenthesis of if condition");
    ASTNode *then_body = parse_body(p);

    ASTNode *else_body = NULL;
    if (match(p, TOKEN_KEYWORD) && match_value(p, "else")) {
        advance(p);  /* eat "else" */
        if (match(p, TOKEN_KEYWORD) && match_value(p, "if")) {
            else_body = parse_if(p);
        } else {
            else_body = parse_body(p);
        }
    }

    return ast_create_if(cond, then_body, else_body);
}

/* Parse: "while" '(' expression ')' body */
static ASTNode* parse_while(Parser *p) {
    advance(p);  /* eat "while" */
    expect_value(p, "(", "opening parenthesis of while condition");
    ASTNode *cond = parse_expression(p);
    expect_value(p, ")", "closing parenthesis of while condition");
    ASTNode *body = parse_body(p);
    return ast_create_while(cond, body);
}

/*
 * Parse: "for" '(' init ';' condition ';' update ')' block
 *
 * init can be a var_decl (no trailing ';') or an expression/assignment.
 * update is an expression, possibly an assignment (no trailing ';').
 */
static ASTNode* parse_for(Parser *p) {
    advance(p);  /* eat "for" */
    expect_value(p, "(", "opening parenthesis of for");

    /* --- Init --- */
    ASTNode *init = NULL;
    if (is_type_start(p)) {
        /* Variable declaration without trailing semicolon */
        init = parse_var_decl(p, 0);
    } else if (!match_value(p, ";")) {
        /* Expression (possibly assignment) */
        init = parse_expression(p);
        if (match(p, TOKEN_OPERATOR) && match_value(p, "=")) {
            advance(p);  /* eat '=' */
            ASTNode *val = parse_expression(p);
            init = ast_create_assign(init, val);
        }
    }
    expect_value(p, ";", "first semicolon in for");

    /* --- Condition --- */
    ASTNode *cond = NULL;
    if (!match_value(p, ";")) {
        cond = parse_expression(p);
    }
    expect_value(p, ";", "second semicolon in for");

    /* --- Update --- */
    ASTNode *update = NULL;
    if (!match_value(p, ")")) {
        update = parse_expression(p);
        if (match(p, TOKEN_OPERATOR) && match_value(p, "=")) {
            advance(p);  /* eat '=' */
            ASTNode *val = parse_expression(p);
            update = ast_create_assign(update, val);
        }
    }
    expect_value(p, ")", "closing parenthesis of for");

    ASTNode *body = parse_body(p);
    return ast_create_for(init, cond, update, body);
}

/* Parse: "break" ";" */
static ASTNode* parse_break_stmt(Parser *p) {
    advance(p);  /* eat "break" */
    expect_value(p, ";", "semicolon after break");
    return ast_create_break();
}

/*
 * Determine whether the current position is a variable declaration.
 *
 * A var_decl starts with a type specifier followed (possibly after pointer
 * stars) by an identifier.  We need lookahead to distinguish
 * "int x = 5;" (var_decl) from "x = 5;" (expression statement).
 */
static int looking_at_var_decl(Parser *p) {
    if (!is_type_start(p)) return 0;

    Token *t = peek(p);
    int offset = 1;

    /* If struct/enum, skip the following name identifier */
    if (strcmp(t->value, "struct") == 0 || strcmp(t->value, "enum") == 0) {
        Token *next = peek_ahead(p, offset);
        if (next && next->type == TOKEN_IDENTIFIER) {
            offset++;
        }
    }

    /* Skip pointer stars */
    while (1) {
        Token *star = peek_ahead(p, offset);
        if (star && star->type == TOKEN_OPERATOR && strcmp(star->value, "*") == 0) {
            offset++;
        } else {
            break;
        }
    }

    /* The next token should be an identifier for this to be a var_decl */
    Token *maybe_name = peek_ahead(p, offset);
    return maybe_name && maybe_name->type == TOKEN_IDENTIFIER;
}

/*
 * Parse a single statement.
 *
 * Dispatches based on the leading keyword or falls through to
 * expression statement (which also handles assignment).
 */
static ASTNode* parse_statement(Parser *p) {
    Token *t = peek(p);
    if (!t || t->type == TOKEN_EOF) return NULL;

    /* Keyword-led statements */
    if (t->type == TOKEN_KEYWORD) {
        if (strcmp(t->value, "return") == 0) return parse_return(p);
        if (strcmp(t->value, "if") == 0)     return parse_if(p);
        if (strcmp(t->value, "while") == 0)  return parse_while(p);
        if (strcmp(t->value, "for") == 0)    return parse_for(p);
        if (strcmp(t->value, "break") == 0)  return parse_break_stmt(p);

        /* Variable declaration: type keyword or struct/enum followed by name */
        if (looking_at_var_decl(p)) {
            return parse_var_decl(p, 1);
        }
    }

    /*
     * Expression statement.
     * Handles plain expressions (function calls) and assignments like:
     *   x = 5;
     *   arr[0] = 10;
     *   r.top_left.x = 0;
     *   *pos = *pos + 1;
     *   dest[i] = src[i];
     */
    ASTNode *expr = parse_expression(p);

    /* Check for assignment */
    if (match(p, TOKEN_OPERATOR) && match_value(p, "=")) {
        advance(p);  /* eat '=' */
        ASTNode *value = parse_expression(p);
        expr = ast_create_assign(expr, value);
    }

    expect_value(p, ";", "semicolon after expression statement");
    return expr;
}

/* ------------------------------------------------------------------ */
/*  Top-level parsing: struct defs, enum defs, function defs           */
/* ------------------------------------------------------------------ */

/*
 * Parse struct definition:
 *   "struct" Name '{' (type [*]* name ';')* '}' ';'
 */
static ASTNode* parse_struct_def(Parser *p) {
    advance(p);  /* eat "struct" */
    Token *name = expect(p, TOKEN_IDENTIFIER, "struct name");
    char *struct_name = name->value;

    expect_value(p, "{", "opening brace of struct");

    char **field_names = NULL;
    char **field_types = NULL;
    int  *field_ptr_levels = NULL;
    int   field_count = 0;
    int   field_cap = 8;

    field_names = malloc(sizeof(char*) * field_cap);
    field_types = malloc(sizeof(char*) * field_cap);
    field_ptr_levels = malloc(sizeof(int) * field_cap);

    while (!(peek(p) && peek(p)->type == TOKEN_SEPARATOR &&
             strcmp(peek(p)->value, "}") == 0)) {
        int ptr_lvl = 0;
        char *ftype = parse_type(p, &ptr_lvl);
        Token *fname = expect(p, TOKEN_IDENTIFIER, "field name");

        if (field_count >= field_cap) {
            field_cap *= 2;
            field_names = realloc(field_names, sizeof(char*) * field_cap);
            field_types = realloc(field_types, sizeof(char*) * field_cap);
            field_ptr_levels = realloc(field_ptr_levels, sizeof(int) * field_cap);
        }

        field_names[field_count] = strdup(fname->value);
        field_types[field_count] = ftype;  /* already strdup'd by parse_type */
        field_ptr_levels[field_count] = ptr_lvl;
        field_count++;

        expect_value(p, ";", "semicolon after struct field");
    }

    expect_value(p, "}", "closing brace of struct");
    expect_value(p, ";", "semicolon after struct definition");

    ASTNode *node = ast_create_struct_def(struct_name, field_names, field_types,
                                          field_ptr_levels, field_count);

    /* Free temporary arrays -- ast_create_struct_def strdup's internally */
    for (int i = 0; i < field_count; i++) {
        free(field_names[i]);
        free(field_types[i]);
    }
    free(field_names);
    free(field_types);
    free(field_ptr_levels);

    return node;
}

/*
 * Parse enum definition:
 *   "enum" Name '{' value (',' value)* [','] '}' ';'
 */
static ASTNode* parse_enum_def(Parser *p) {
    advance(p);  /* eat "enum" */
    Token *name = expect(p, TOKEN_IDENTIFIER, "enum name");
    char *enum_name = name->value;

    expect_value(p, "{", "opening brace of enum");

    char **values = NULL;
    int value_count = 0;
    int value_cap = 8;
    values = malloc(sizeof(char*) * value_cap);

    while (!(peek(p) && peek(p)->type == TOKEN_SEPARATOR &&
             strcmp(peek(p)->value, "}") == 0)) {
        Token *val = expect(p, TOKEN_IDENTIFIER, "enum value");

        if (value_count >= value_cap) {
            value_cap *= 2;
            values = realloc(values, sizeof(char*) * value_cap);
        }
        values[value_count++] = strdup(val->value);

        /* Optional comma between values */
        if (match_value(p, ",")) {
            advance(p);
        }
    }

    expect_value(p, "}", "closing brace of enum");
    expect_value(p, ";", "semicolon after enum definition");

    ASTNode *node = ast_create_enum_def(enum_name, values, value_count);

    for (int i = 0; i < value_count; i++) {
        free(values[i]);
    }
    free(values);

    return node;
}

/*
 * Parse a function definition:
 *   type [*]* name '(' param_list ')' block
 *
 * param_list = ( type [*]* name (',' type [*]* name)* )?
 */
static ASTNode* parse_function_def(Parser *p) {
    int ret_ptr = 0;
    char *ret_type = parse_type(p, &ret_ptr);

    Token *name = expect(p, TOKEN_IDENTIFIER, "function name");
    char *fn_name = name->value;

    expect_value(p, "(", "opening parenthesis of function parameters");

    /* Parse parameter list */
    ASTNode **params = NULL;
    int param_count = 0;
    int param_cap = 4;
    params = malloc(sizeof(ASTNode*) * param_cap);

    if (!(peek(p) && peek(p)->type == TOKEN_SEPARATOR &&
          strcmp(peek(p)->value, ")") == 0)) {
        /* At least one parameter */
        int ptr_lvl = 0;
        char *ptype = parse_type(p, &ptr_lvl);
        Token *pname = expect(p, TOKEN_IDENTIFIER, "parameter name");
        params[param_count++] = ast_create_var_decl(pname->value, ptype, ptr_lvl, -1, NULL);
        free(ptype);

        while (match_value(p, ",")) {
            advance(p);  /* eat ',' */
            if (param_count >= param_cap) {
                param_cap *= 2;
                params = realloc(params, sizeof(ASTNode*) * param_cap);
            }
            ptr_lvl = 0;
            ptype = parse_type(p, &ptr_lvl);
            pname = expect(p, TOKEN_IDENTIFIER, "parameter name");
            params[param_count++] = ast_create_var_decl(pname->value, ptype, ptr_lvl, -1, NULL);
            free(ptype);
        }
    }

    expect_value(p, ")", "closing parenthesis of function parameters");

    /* Parse function body */
    ASTNode *body = parse_block(p);

    ASTNode *node = ast_create_function(fn_name, ret_type, ret_ptr,
                                        params, param_count, body);
    free(ret_type);
    /* Don't free params array -- ownership transferred to the AST node */
    return node;
}

/*
 * Determine whether the tokens at current position form a struct definition
 * (as opposed to a function returning a struct type).
 *
 * struct Foo { ... };   <- struct definition  (struct Name '{')
 * struct Foo func(...)  <- function returning struct Foo
 */
static int looking_at_struct_def(Parser *p) {
    Token *t0 = peek(p);
    if (!t0 || t0->type != TOKEN_KEYWORD || strcmp(t0->value, "struct") != 0)
        return 0;
    Token *t1 = peek_ahead(p, 1);
    if (!t1 || t1->type != TOKEN_IDENTIFIER) return 0;
    Token *t2 = peek_ahead(p, 2);
    return t2 && t2->type == TOKEN_SEPARATOR && strcmp(t2->value, "{") == 0;
}

/* Same logic for enum definitions: enum Name '{' */
static int looking_at_enum_def(Parser *p) {
    Token *t0 = peek(p);
    if (!t0 || t0->type != TOKEN_KEYWORD || strcmp(t0->value, "enum") != 0)
        return 0;
    Token *t1 = peek_ahead(p, 1);
    if (!t1 || t1->type != TOKEN_IDENTIFIER) return 0;
    Token *t2 = peek_ahead(p, 2);
    return t2 && t2->type == TOKEN_SEPARATOR && strcmp(t2->value, "{") == 0;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

void parser_init(Parser *parser, Token *tokens, int count) {
    parser->tokens = tokens;
    parser->token_count = count;
    parser->pos = 0;
}

/*
 * Parse the entire program.
 *
 * A program is a sequence of top-level items:
 *   - struct definitions
 *   - enum definitions
 *   - function definitions
 */
ASTNode* parse_program(Parser *p) {
    ASTNode *program = ast_create_program();

    while (peek(p) && peek(p)->type != TOKEN_EOF) {
        ASTNode *item = NULL;

        if (looking_at_struct_def(p)) {
            item = parse_struct_def(p);
        } else if (looking_at_enum_def(p)) {
            item = parse_enum_def(p);
        } else {
            item = parse_function_def(p);
        }

        if (item) {
            ast_add_item(program, item);
        }
    }

    return program;
}
