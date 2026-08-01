#ifndef SK_LEXER_H
#define SK_LEXER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum sk_token_type {
    SK_TOKEN_EOF,
    SK_TOKEN_ERR,

    SK_TOKEN_IDENTIFIER,
    SK_TOKEN_NUMBER,
    SK_TOKEN_STRING,

    SK_TOKEN_PRINT,
    SK_TOKEN_TRUE,
    SK_TOKEN_FALSE,
    SK_TOKEN_IF,
    SK_TOKEN_ELSE,
    SK_TOKEN_FOR,
    SK_TOKEN_WHILE,
    SK_TOKEN_FN,
    SK_TOKEN_RETURN,
    SK_TOKEN_LET,

    SK_TOKEN_LPAREN,
    SK_TOKEN_RPAREN,
    SK_TOKEN_LBRACE,
    SK_TOKEN_RBRACE,

    SK_TOKEN_COMMA,
    SK_TOKEN_COLON,

    SK_TOKEN_RARROW,

    SK_TOKEN_ASSIGN,
    SK_TOKEN_NOT,
    SK_TOKEN_PLUS,
    SK_TOKEN_MINUS,
    SK_TOKEN_STAR,
    SK_TOKEN_SLASH,
    SK_TOKEN_LESS,
    SK_TOKEN_LESS_EQ,
    SK_TOKEN_GREATER,
    SK_TOKEN_GREATER_EQ,
    SK_TOKEN_EQUAL,
    SK_TOKEN_NOT_EQUAL,
    SK_TOKEN_AND,
    SK_TOKEN_OR,
};

struct sk_token {
    enum sk_token_type type;
    const char *start;
    size_t length;
    const char *filename;
    size_t line;
    size_t column;
};

struct sk_token_set {
    uint64_t bits;
};

struct sk_token_set sk_token_set_new(void);
void sk_token_set_add(struct sk_token_set *set, enum sk_token_type type);
bool sk_token_set_has(const struct sk_token_set *set, enum sk_token_type type);

struct sk_lexer {
    const char *start;
    const char *current;
    const char *filename;
    size_t start_line;
    size_t line;
    size_t start_column;
    size_t column;
};

void sk_lexer_init(struct sk_lexer *lexer, const char *filename, const char *source);

struct sk_token sk_lexer_next(struct sk_lexer *lexer);

#endif // SK_LEXER_H
