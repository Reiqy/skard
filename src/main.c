#include <stdio.h>

#include "skard.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    struct sk_lexer lexer;
    sk_lexer_init(&lexer, "  2.3   \n 4.2 10 \t 32 asd\n (4.3 + 2.14) * (2 -3.132+28/2)\0");

    struct sk_token token;
    while ((token = sk_lexer_next(&lexer)).type != SK_TOKEN_EOF) {
        printf("LINE: %zu | TYPE: %d | TOKEN: %.*s\n", token.line, token.type, (int)token.length, token.start);
    }

    return 0;
}
