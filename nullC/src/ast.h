#ifndef AST_H
#define AST_H

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_BLOCK,
    AST_RETURN,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_BREAK,
    AST_VAR_DECL,
    AST_ASSIGN,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_NUMBER,
    AST_IDENTIFIER,
    AST_CALL,
    AST_STRING,
    AST_CHAR_LIT,
    AST_INDEX,
    AST_MEMBER_ACCESS,
    AST_STRUCT_DEF,
    AST_ENUM_DEF
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    union {
        struct { struct ASTNode **items; int item_count; } program;

        struct {
            char *name;
            char *return_type;
            int return_ptr_level;
            struct ASTNode **params;   // AST_VAR_DECL nodes
            int param_count;
            struct ASTNode *body;
        } function;

        struct { struct ASTNode **statements; int stmt_count; } block;
        struct { struct ASTNode *value; } return_stmt;

        struct {
            struct ASTNode *condition;
            struct ASTNode *then_body;
            struct ASTNode *else_body;  // NULL or another AST_IF or AST_BLOCK
        } if_stmt;

        struct { struct ASTNode *condition; struct ASTNode *body; } while_stmt;

        struct {
            struct ASTNode *init;       // var_decl or assign
            struct ASTNode *condition;
            struct ASTNode *update;     // assign expression
            struct ASTNode *body;
        } for_stmt;

        struct {
            char *name;
            char *var_type;      // "int", "char", "struct Point", "enum TokenType"
            int ptr_level;       // 0=value, 1=pointer, 2=pointer-to-pointer
            int array_size;      // -1=not array, 0+=fixed size
            struct ASTNode *init;
        } var_decl;

        struct {
            struct ASTNode *target;  // identifier, index, or member_access
            struct ASTNode *value;
        } assign;

        struct {
            char op[4];  // "+", "-", "==", "!=", "<=", ">=", "&&", "||", etc.
            struct ASTNode *left;
            struct ASTNode *right;
        } binary_op;

        struct {
            char op;  // '*' (deref), '&' (addr-of), '-' (negate), '!' (not)
            struct ASTNode *operand;
        } unary_op;

        struct { int value; } number;
        struct { char *name; } identifier;

        struct {
            char *name;
            struct ASTNode **args;
            int arg_count;
        } call;

        struct { char *value; } string_lit;
        struct { char value; } char_lit;

        struct {
            struct ASTNode *array;  // expression (identifier, deref, etc.)
            struct ASTNode *index;  // expression
        } index_access;

        struct {
            struct ASTNode *object;
            char *member;
        } member_access;

        struct {
            char *name;
            char **field_names;
            char **field_types;
            int *field_ptr_levels;
            int field_count;
        } struct_def;

        struct {
            char *name;
            char **values;
            int value_count;
        } enum_def;
    } data;
} ASTNode;

// Constructors
ASTNode* ast_create_program(void);
ASTNode* ast_create_function(char *name, char *ret_type, int ret_ptr, ASTNode **params, int param_count, ASTNode *body);
ASTNode* ast_create_block(void);
ASTNode* ast_create_return(ASTNode *value);
ASTNode* ast_create_if(ASTNode *cond, ASTNode *then_body, ASTNode *else_body);
ASTNode* ast_create_while(ASTNode *cond, ASTNode *body);
ASTNode* ast_create_for(ASTNode *init, ASTNode *cond, ASTNode *update, ASTNode *body);
ASTNode* ast_create_break(void);
ASTNode* ast_create_var_decl(char *name, char *type, int ptr_level, int array_size, ASTNode *init);
ASTNode* ast_create_assign(ASTNode *target, ASTNode *value);
ASTNode* ast_create_binary_op(const char *op, ASTNode *left, ASTNode *right);
ASTNode* ast_create_unary_op(char op, ASTNode *operand);
ASTNode* ast_create_number(int value);
ASTNode* ast_create_identifier(char *name);
ASTNode* ast_create_call(char *name, ASTNode **args, int arg_count);
ASTNode* ast_create_string(char *value);
ASTNode* ast_create_char_lit(char value);
ASTNode* ast_create_index(ASTNode *array, ASTNode *index);
ASTNode* ast_create_member_access(ASTNode *object, char *member);
ASTNode* ast_create_struct_def(char *name, char **field_names, char **field_types, int *field_ptr_levels, int count);
ASTNode* ast_create_enum_def(char *name, char **values, int count);

void ast_add_item(ASTNode *program, ASTNode *item);
void ast_add_statement(ASTNode *block, ASTNode *stmt);
void ast_print(ASTNode *node, int indent);
void ast_free(ASTNode *node);

#endif // AST_H
