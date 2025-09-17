#ifndef SKARD_SK_AST_H
#define SKARD_SK_AST_H

#include <stdlib.h>

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

    SK_AST_ARGS,

    // Statements
    SK_AST_BLOCK,
    SK_AST_IF,
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

struct sk_ast_args {
    struct sk_ast_node_array args;
};

struct sk_ast_block {
    struct sk_ast_node_array contents;
};

struct sk_ast_if {
    struct sk_ast_node *condition;
    struct sk_ast_node *then_branch;
    struct sk_ast_node *else_branch;
};

struct sk_ast_print {
    struct sk_ast_node *args;
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
        struct sk_ast_args args;
        struct sk_ast_block block;
        struct sk_ast_if ifn;
        struct sk_ast_print print;
        struct sk_ast_fn fn;
        struct sk_ast_program program;
        struct sk_ast_err err;
    } as;
};

void sk_ast_node_print(const struct sk_ast_node *node);

struct sk_ast_node_arena_block {
    struct sk_ast_node *nodes;
    size_t capacity;
    size_t count;
};

struct sk_ast_node_arena {
    struct sk_ast_node_arena_block *blocks;
    size_t capacity;
    size_t count;
    size_t current_block_index;
    size_t initial_block_capacity;
    size_t block_capacity;
};

void sk_ast_node_arena_init(struct sk_ast_node_arena *arena, size_t block_capacity);
void sk_ast_node_arena_free(struct sk_ast_node_arena *arena);
struct sk_ast_node *sk_ast_node_arena_alloc(struct sk_ast_node_arena *arena);

#endif //SKARD_SK_AST_H
