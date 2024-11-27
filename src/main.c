#include <stdio.h>

#include "skard.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    struct sk_ast_node literal1 = {
        .type = SK_AST_LITERAL,
        .as.literal = (struct sk_ast_literal) {
            .token = (struct sk_token) {
                .type = SK_TOKEN_NUMBER,
                .start = "4.2",
                .length = 3,
                .line = 1
            },
        },
    };

    struct sk_ast_node literal2 = {
        .type = SK_AST_LITERAL,
        .as.literal = (struct sk_ast_literal) {
            .token = (struct sk_token) {
                .type = SK_TOKEN_NUMBER,
                .start = "6.9",
                .length = 3,
                .line = 1
            },
        },
    };

    struct sk_ast_node binary = {
        .type = SK_AST_BINARY,
        .as.binary = (struct sk_ast_binary) {
            .operator = (struct sk_token) {
                .type = SK_TOKEN_PLUS,
                .start = "+",
                .length = 1,
                .line = 1,
            },
            .left = &literal1,
            .right = &literal2,
        },
    };

    sk_value_print(sk_number_value(sk_ast_evaluate(&binary)));

    return 0;
}
