#include "sk_lexer.h"

#include <stdbool.h>
#include <string.h>

static struct sk_token make_token(struct sk_lexer *lexer, enum sk_token_type type);
static struct sk_token make_token_eof(struct sk_lexer *lexer);
static struct sk_token make_token_err(struct sk_lexer *lexer, const char *message);

static struct sk_token scan_number(struct sk_lexer *lexer);

static bool is_at_eof(struct sk_lexer *lexer);

static char advance(struct sk_lexer *lexer);
static char peek(struct sk_lexer *lexer);
static char peek_next(struct sk_lexer *lexer);

static bool is_alpha(char c);
static bool is_digit(char c);

static bool is_whitespace(char c);
static void skip_whitespace(struct sk_lexer *lexer);

void sk_lexer_init(struct sk_lexer *lexer, const char *source)
{
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
}

struct sk_token sk_lexer_next(struct sk_lexer *lexer)
{
    skip_whitespace(lexer);
    lexer->start = lexer->current;

    if (is_at_eof(lexer)) {
        return make_token_eof(lexer);
    }

    char c = advance(lexer);

    if (is_alpha(c)) {
        return make_token_err(lexer, "Not implemented yet.");
    }

    if (is_digit(c)) {
        return scan_number(lexer);
    }

    switch (c) {
        case '(':
            return make_token(lexer, SK_TOKEN_LPAREN);
        case ')':
            return make_token(lexer, SK_TOKEN_RPAREN);
        case '+':
            return make_token(lexer, SK_TOKEN_PLUS);
        case '-':
            return make_token(lexer, SK_TOKEN_MINUS);
        case '*':
            return make_token(lexer, SK_TOKEN_STAR);
        case '/':
            return make_token(lexer, SK_TOKEN_SLASH);
        default:
            break;
    }

    return make_token_err(lexer, "Unexpected character.");
}

static struct sk_token make_token(struct sk_lexer *lexer, enum sk_token_type type)
{
    struct sk_token token = {
        .type = type,
        .start = lexer->start,
        .length = lexer->current - lexer->start,
        .line = lexer->line,
    };

    return token;
}

static struct sk_token make_token_eof(struct sk_lexer *lexer)
{
    return make_token(lexer, SK_TOKEN_EOF);
}

static struct sk_token make_token_err(struct sk_lexer *lexer, const char *message)
{
    struct sk_token token = {
        .type = SK_TOKEN_ERR,
        .start = message,
        .length = strlen(message),
        .line = lexer->line,
    };

    return token;
}

static struct sk_token scan_number(struct sk_lexer *lexer)
{
    while (is_digit(peek(lexer))) {
        advance(lexer);
    }

    if (peek(lexer) == '.' && is_digit(peek_next(lexer))) {
        advance(lexer);

        while (is_digit(peek(lexer))) {
            advance(lexer);
        }
    }

    return make_token(lexer, SK_TOKEN_NUMBER);
}

static bool is_at_eof(struct sk_lexer *lexer)
{
    return peek(lexer) == '\0';
}

static char advance(struct sk_lexer *lexer)
{
    char c = peek(lexer);
    if (c == '\n') {
        lexer->line++;
    }

    lexer->current++;
    return c;
}

static char peek(struct sk_lexer *lexer)
{
    return *lexer->current;
}

static char peek_next(struct sk_lexer *lexer)
{
    if (is_at_eof(lexer)) {
        return '\0';
    }

    return lexer->current[1];
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static void skip_whitespace(struct sk_lexer *lexer)
{
    for (;;) {
        char c = peek(lexer);
        if (!is_whitespace(c)) {
            break;
        }

        advance(lexer);
    }
}
