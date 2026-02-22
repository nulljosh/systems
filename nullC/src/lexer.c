#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static const char *keywords[] = {
    "int", "return", "if", "else", "while", "for",
    "void", "char", "struct", "enum", "break", NULL
};

static int is_keyword(const char *str) {
    for (int i = 0; keywords[i]; i++) {
        if (strcmp(str, keywords[i]) == 0) return 1;
    }
    return 0;
}

void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->pos = 0;
    lexer->length = strlen(source);
    lexer->line = 1;
    lexer->column = 1;
}

static char peek(Lexer *lexer) {
    if (lexer->pos >= lexer->length) return '\0';
    return lexer->source[lexer->pos];
}

static char advance(Lexer *lexer) {
    if (lexer->pos >= lexer->length) return '\0';
    char c = lexer->source[lexer->pos++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static void skip_whitespace_and_comments(Lexer *lexer) {
    while (1) {
        while (isspace(peek(lexer))) {
            advance(lexer);
        }

        // Check for comments
        if (peek(lexer) == '/' && lexer->pos + 1 < lexer->length) {
            char next = lexer->source[lexer->pos + 1];

            // Line comment //
            if (next == '/') {
                advance(lexer);  // /
                advance(lexer);  // /
                while (peek(lexer) != '\n' && peek(lexer) != '\0') {
                    advance(lexer);
                }
                continue;
            }

            // Block comment /* */
            if (next == '*') {
                advance(lexer);  // /
                advance(lexer);  // *
                while (!(peek(lexer) == '*' && lexer->pos + 1 < lexer->length &&
                         lexer->source[lexer->pos + 1] == '/')) {
                    if (peek(lexer) == '\0') break;
                    advance(lexer);
                }
                if (peek(lexer) == '*') {
                    advance(lexer);  // *
                    advance(lexer);  // /
                }
                continue;
            }
        }

        break;
    }
}

static Token make_token(TokenType type, const char *value, int line, int col) {
    Token token;
    token.type = type;
    token.value = strdup(value);
    token.line = line;
    token.column = col;
    return token;
}

/* Resolve a single backslash escape character.
 * Given the character after the backslash, return the actual character. */
static char resolve_escape(char c) {
    switch (c) {
        case 'n':  return '\n';
        case 't':  return '\t';
        case '\\': return '\\';
        case '0':  return '\0';
        case '"':  return '"';
        case '\'': return '\'';
        default:   return c;
    }
}

static Token lex_string(Lexer *lexer, int start_line, int start_col) {
    advance(lexer);  // skip opening "

    // Collect string content into a dynamic buffer
    size_t cap = 64;
    size_t len = 0;
    char *buf = malloc(cap);

    while (peek(lexer) != '"' && peek(lexer) != '\0') {
        char c = advance(lexer);
        if (c == '\\') {
            // Escape sequence
            c = resolve_escape(advance(lexer));
        }
        if (len + 1 >= cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
        buf[len++] = c;
    }

    if (peek(lexer) == '"') {
        advance(lexer);  // skip closing "
    }

    buf[len] = '\0';
    Token token = make_token(TOKEN_STRING, buf, start_line, start_col);
    free(buf);
    return token;
}

static Token lex_char_lit(Lexer *lexer, int start_line, int start_col) {
    advance(lexer);  // skip opening '

    char c;
    if (peek(lexer) == '\\') {
        advance(lexer);  // skip backslash
        c = resolve_escape(advance(lexer));
    } else {
        c = advance(lexer);
    }

    if (peek(lexer) == '\'') {
        advance(lexer);  // skip closing '
    }

    // Store the character as a single-char string
    char str[2] = {c, '\0'};
    return make_token(TOKEN_CHAR_LIT, str, start_line, start_col);
}

Token lexer_next_token(Lexer *lexer) {
    skip_whitespace_and_comments(lexer);

    int start_line = lexer->line;
    int start_col = lexer->column;

    char c = peek(lexer);

    if (c == '\0') {
        return make_token(TOKEN_EOF, "", start_line, start_col);
    }

    // String literals
    if (c == '"') {
        return lex_string(lexer, start_line, start_col);
    }

    // Character literals
    if (c == '\'') {
        return lex_char_lit(lexer, start_line, start_col);
    }

    // Numbers
    if (isdigit(c)) {
        size_t start = lexer->pos;
        while (isdigit(peek(lexer))) advance(lexer);
        size_t len = lexer->pos - start;
        char *num = strndup(&lexer->source[start], len);
        Token token = make_token(TOKEN_NUMBER, num, start_line, start_col);
        free(num);
        return token;
    }

    // Identifiers and keywords
    if (isalpha(c) || c == '_') {
        size_t start = lexer->pos;
        while (isalnum(peek(lexer)) || peek(lexer) == '_') advance(lexer);
        size_t len = lexer->pos - start;
        char *word = strndup(&lexer->source[start], len);
        TokenType type = is_keyword(word) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
        Token token = make_token(type, word, start_line, start_col);
        free(word);
        return token;
    }

    // Multi-character and single-character operators
    if (strchr("+-*/=<>!&|", c)) {
        advance(lexer);
        char next = peek(lexer);

        // Two-character operators
        if (c == '=' && next == '=') { advance(lexer); return make_token(TOKEN_OPERATOR, "==", start_line, start_col); }
        if (c == '!' && next == '=') { advance(lexer); return make_token(TOKEN_OPERATOR, "!=", start_line, start_col); }
        if (c == '<' && next == '=') { advance(lexer); return make_token(TOKEN_OPERATOR, "<=", start_line, start_col); }
        if (c == '>' && next == '=') { advance(lexer); return make_token(TOKEN_OPERATOR, ">=", start_line, start_col); }
        if (c == '&' && next == '&') { advance(lexer); return make_token(TOKEN_OPERATOR, "&&", start_line, start_col); }
        if (c == '|' && next == '|') { advance(lexer); return make_token(TOKEN_OPERATOR, "||", start_line, start_col); }

        // Single-character operator
        char str[2] = {c, '\0'};
        return make_token(TOKEN_OPERATOR, str, start_line, start_col);
    }

    // Separators: ( ) { } [ ] ; , .
    if (strchr("(){}[];,.", c)) {
        advance(lexer);
        char str[2] = {c, '\0'};
        return make_token(TOKEN_SEPARATOR, str, start_line, start_col);
    }

    // Unknown character
    advance(lexer);
    char str[2] = {c, '\0'};
    return make_token(TOKEN_UNKNOWN, str, start_line, start_col);
}

void token_free(Token *token) {
    if (token->value) {
        free(token->value);
        token->value = NULL;
    }
}

const char* token_type_str(TokenType type) {
    switch (type) {
        case TOKEN_EOF:        return "EOF";
        case TOKEN_KEYWORD:    return "KEYWORD";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMBER:     return "NUMBER";
        case TOKEN_STRING:     return "STRING";
        case TOKEN_CHAR_LIT:   return "CHAR_LIT";
        case TOKEN_OPERATOR:   return "OPERATOR";
        case TOKEN_SEPARATOR:  return "SEPARATOR";
        case TOKEN_UNKNOWN:    return "UNKNOWN";
        default:               return "???";
    }
}
