#ifndef SK_PARSER_H
#define SK_PARSER_H

#include <stdbool.h>

#include "sk_lexer.h"

struct sk_ast_node_array {
    struct sk_ast_node **nodes;
    size_t capacity;
    size_t count;
};

void sk_ast_node_array_init(struct sk_ast_node_array *array);
void sk_ast_node_array_free(struct sk_ast_node_array *array);
void sk_ast_node_array_add(struct sk_ast_node_array *array, struct sk_ast_node *node);

enum sk_ast_node_type {
    // Expressions
    SK_AST_LITERAL,
    SK_AST_UNARY,
    SK_AST_BINARY,

    // Statements
    SK_AST_BLOCK,
    SK_AST_PRINT,

    // Declarations
    SK_AST_FN,

    // Program
    SK_AST_PROGRAM,

    // Error
    SK_AST_ERR,
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

struct sk_ast_print {
    struct sk_ast_node *expression;
};

struct sk_ast_block {
    struct sk_ast_node_array contents;
};

struct sk_ast_fn {
    struct sk_token name;
    struct sk_ast_node *body;
};

struct sk_ast_program {
    struct sk_ast_node_array declarations;
};

struct sk_ast_err {
    const char *message;
};

struct sk_ast_node {
    enum sk_ast_node_type type;
    union {
        struct sk_ast_literal literal;
        struct sk_ast_unary unary;
        struct sk_ast_binary binary;
        struct sk_ast_print print;
        struct sk_ast_block block;
        struct sk_ast_fn fn;
        struct sk_ast_program program;
        struct sk_ast_err err;
    } as;
};

void sk_ast_node_print(const struct sk_ast_node *node);

struct sk_parser {
    struct sk_lexer lexer;
    struct sk_token current;
    struct sk_token previous;
    bool has_error;
    bool is_panic_mode;
};

void sk_parser_init(struct sk_parser *parser, const char *source);
struct sk_ast_node *sk_parser_parse(struct sk_parser *parser);

#endif //SK_PARSER_H
