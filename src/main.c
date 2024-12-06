#include "skard.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    struct sk_parser parser;
    sk_parser_init(&parser, "# 1 + (2 - 3) * 4 + 5 / (6 - 8) * 9");
    // sk_parser_init(&parser, "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9");

    struct sk_ast_node *node = sk_parser_parse(&parser);
    sk_ast_node_print(node);

    return 0;
}
