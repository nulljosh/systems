#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Token *tokens;
    int token_count;
    int pos;
} Parser;

void parser_init(Parser *parser, Token *tokens, int count);
ASTNode* parse_program(Parser *parser);

#endif // PARSER_H
