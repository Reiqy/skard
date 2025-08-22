#ifndef SK_LEXER_H
#define SK_LEXER_H

#include <stddef.h>

enum sk_token_type {
    SK_TOKEN_EOF, SK_TOKEN_ERR,

    SK_TOKEN_IDENTIFIER, SK_TOKEN_NUMBER,

    SK_TOKEN_PRINT,
    SK_TOKEN_IF, SK_TOKEN_ELSE,
    SK_TOKEN_FOR, SK_TOKEN_WHILE,

    SK_TOKEN_LPAREN, SK_TOKEN_RPAREN,

    SK_TOKEN_PLUS, SK_TOKEN_MINUS, SK_TOKEN_STAR, SK_TOKEN_SLASH,
};

struct sk_token {
    enum sk_token_type type;
    const char *start;
    size_t length;
    size_t line;
};

struct sk_lexer {
    const char *start;
    const char *current;
    size_t line;
};

void sk_lexer_init(struct sk_lexer *lexer, const char *source);

struct sk_token sk_lexer_next(struct sk_lexer *lexer);

#endif //SK_LEXER_H
