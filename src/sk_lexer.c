#include "sk_lexer.h"

#include <stdbool.h>
#include <string.h>

void sk_lexer_init(struct sk_lexer *lexer, const char *source)
{
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
}

static size_t get_token_length(const struct sk_lexer *lexer);

static struct sk_token make_token(const struct sk_lexer *lexer, enum sk_token_type type);
static struct sk_token make_token_eof(const struct sk_lexer *lexer);
static struct sk_token make_token_err(const struct sk_lexer *lexer, const char *message);

static bool is_at_eof(const struct sk_lexer *lexer);

static bool is_alpha(char c);
static bool is_digit(char c);
static bool is_whitespace(char c);

static char advance(struct sk_lexer *lexer);
static char peek(const struct sk_lexer *lexer);
static char peek_next(const struct sk_lexer *lexer);

static bool match(struct sk_lexer *lexer, char expected);

static void skip_whitespace(struct sk_lexer *lexer);

enum sk_token_type check_keyword(const struct sk_lexer *lexer, int offset, const char *keyword, enum sk_token_type type);
enum sk_token_type classify_identifier(const struct sk_lexer *lexer);

static struct sk_token scan_number(struct sk_lexer *lexer);
static struct sk_token scan_identifier(struct sk_lexer *lexer);

struct sk_token sk_lexer_next(struct sk_lexer *lexer)
{
    skip_whitespace(lexer);
    lexer->start = lexer->current;

    if (is_at_eof(lexer)) {
        return make_token_eof(lexer);
    }

    char c = advance(lexer);

    if (is_alpha(c)) {
        return scan_identifier(lexer);
    }

    if (is_digit(c)) {
        return scan_number(lexer);
    }

    switch (c) {
        case '(':
            return make_token(lexer, SK_TOKEN_LPAREN);
        case ')':
            return make_token(lexer, SK_TOKEN_RPAREN);
        case '{':
            return make_token(lexer, SK_TOKEN_LBRACE);
        case '}':
            return make_token(lexer, SK_TOKEN_RBRACE);
        case '+':
            return make_token(lexer, SK_TOKEN_PLUS);
        case '-':
            return make_token(lexer, SK_TOKEN_MINUS);
        case '*':
            return make_token(lexer, SK_TOKEN_STAR);
        case '/':
            return make_token(lexer, SK_TOKEN_SLASH);
        case '<':
            return make_token(lexer, match(lexer, '=') ? SK_TOKEN_LESS_EQ : SK_TOKEN_LESS);
        case '>':
            return make_token(lexer, match(lexer, '=') ? SK_TOKEN_GREATER : SK_TOKEN_GREATER_EQ);
        case '=':
            return make_token(lexer, match(lexer, '=') ? SK_TOKEN_ASSIGN : SK_TOKEN_EQUAL);
        case '!':
            return make_token(lexer, match(lexer, '=') ? SK_TOKEN_NOT : SK_TOKEN_NOT_EQUAL);
        default:
            break;
    }

    return make_token_err(lexer, "Unexpected character.");
}

static size_t get_token_length(const struct sk_lexer *lexer)
{
    return lexer->current - lexer->start;
}

static struct sk_token make_token(const struct sk_lexer *lexer, enum sk_token_type type)
{
    struct sk_token token = {
        .type = type,
        .start = lexer->start,
        .length = get_token_length(lexer),
        .line = lexer->line,
    };

    return token;
}

static struct sk_token make_token_eof(const struct sk_lexer *lexer)
{
    return make_token(lexer, SK_TOKEN_EOF);
}

static struct sk_token make_token_err(const struct sk_lexer *lexer, const char *message)
{
    struct sk_token token = {
        .type = SK_TOKEN_ERR,
        .start = message,
        .length = strlen(message),
        .line = lexer->line,
    };

    return token;
}

static bool is_at_eof(const struct sk_lexer *lexer)
{
    return peek(lexer) == '\0';
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

static char advance(struct sk_lexer *lexer)
{
    char c = peek(lexer);
    if (c == '\n') {
        lexer->line++;
    }

    lexer->current++;
    return c;
}

static char peek(const struct sk_lexer *lexer)
{
    return *lexer->current;
}

static char peek_next(const struct sk_lexer *lexer)
{
    if (is_at_eof(lexer)) {
        return '\0';
    }

    return lexer->current[1];
}

static bool match(struct sk_lexer *lexer, char expected)
{
    if (is_at_eof(lexer)) {
        return false;
    }

    if (peek(lexer) != expected) {
        return false;
    }

    advance(lexer);
    return true;
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

enum sk_token_type check_keyword(const struct sk_lexer *lexer, int offset, const char *keyword, enum sk_token_type type)
{
    size_t keyword_length = strlen(keyword);
    size_t token_length = get_token_length(lexer);
    bool is_length_same = token_length == keyword_length;
    if (is_length_same && memcmp(lexer->start + offset, keyword + offset, keyword_length - offset) == 0) {
        return type;
    }

    return SK_TOKEN_IDENTIFIER;
}

enum sk_token_type classify_identifier(const struct sk_lexer *lexer)
{
    switch (lexer->start[0]) {
        case 'e':
            return check_keyword(lexer, 1, "else", SK_TOKEN_ELSE);
        case 'f':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'n':
                        return check_keyword(lexer, 2, "fn", SK_TOKEN_FN);
                    case 'o':
                        return check_keyword(lexer, 2, "for", SK_TOKEN_FOR);
                    default:
                        break;
                }
            }

            break;
        case 'i':
            return check_keyword(lexer, 1, "if", SK_TOKEN_IF);
        case 'p':
            return check_keyword(lexer, 1, "print", SK_TOKEN_PRINT);
        case 'r':
            return check_keyword(lexer, 1, "return", SK_TOKEN_RETURN);
        case 'w':
            return check_keyword(lexer, 1, "while", SK_TOKEN_WHILE);
        default:
            break;
    }

    return SK_TOKEN_IDENTIFIER;
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

static struct sk_token scan_identifier(struct sk_lexer *lexer)
{
    while (is_alpha(peek(lexer)) || is_digit(peek(lexer))) {
        advance(lexer);
    }

    return make_token(lexer, classify_identifier(lexer));
}
