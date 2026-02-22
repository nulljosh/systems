#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

typedef enum {
    TOKEN_EOF,
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_CHAR_LIT,
    TOKEN_OPERATOR,
    TOKEN_SEPARATOR,
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char *value;
    int line;
    int column;
} Token;

typedef struct {
    const char *source;
    size_t pos;
    size_t length;
    int line;
    int column;
} Lexer;

void lexer_init(Lexer *lexer, const char *source);
Token lexer_next_token(Lexer *lexer);
void token_free(Token *token);
const char* token_type_str(TokenType type);

#endif // LEXER_H
