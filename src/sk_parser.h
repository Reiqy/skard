#ifndef SK_PARSER_H
#define SK_PARSER_H

#include "sk_lexer.h"

enum sk_ast_node_type {
    SK_AST_LITERAL,
    SK_AST_UNARY,
    SK_AST_BINARY,
};

struct sk_ast_literal {
    struct sk_token token;
};

struct sk_ast_unary {
    struct sk_token operator;
    struct sk_ast_node *expression;
};

struct sk_ast_binary {
    struct sk_token operator;
    struct sk_ast_node *left;
    struct sk_ast_node *right;
};

struct sk_ast_node {
    enum sk_ast_node_type type;
    union {
        struct sk_ast_literal literal;
        struct sk_ast_unary unary;
        struct sk_ast_binary binary;
    } as;
};

void sk_ast_node_print(const struct sk_ast_node *node);

struct sk_parser {
    struct sk_lexer lexer;
    struct sk_token current;
    struct sk_token previous;
};

void sk_parser_init(struct sk_parser *parser, const char *source);
struct sk_ast_node *sk_parser_parse(struct sk_parser *parser);

#endif //SK_PARSER_H
