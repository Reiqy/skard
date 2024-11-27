#ifndef SK_PARSER_H
#define SK_PARSER_H

#include "sk_lexer.h"
#include "sk_value.h"

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

sk_number sk_ast_evaluate(struct sk_ast_node *node);

// struct sk_parser {
//
// };
//
// struct sk_ast_node sk_parser_parse

#endif //SK_PARSER_H
