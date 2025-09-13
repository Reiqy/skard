#ifndef SK_PARSER_H
#define SK_PARSER_H

#include <stdbool.h>

#include "sk_ast.h"
#include "sk_lexer.h"

struct sk_parser {
    struct sk_ast_node_arena arena;
    struct sk_lexer lexer;
    struct sk_token current;
    struct sk_token previous;
    bool has_error;
    bool is_panic;
};

void sk_parser_init(struct sk_parser *parser, const char *source);
void sk_parser_free(struct sk_parser *parser);
struct sk_ast_node *sk_parser_parse(struct sk_parser *parser);

#endif //SK_PARSER_H
