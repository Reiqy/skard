#ifndef SKARD_SK_CHECKER_H
#define SKARD_SK_CHECKER_H

#include <stdbool.h>
#include <stddef.h>

#include "sk_ast.h"
#include "sk_hashmap.h"
#include "sk_lexer.h"
#include "sk_type.h"
#include "sk_value.h"

#define SK_MAX_LOCAL_SLOTS 256

enum sk_symbol_type {
    SK_SYMBOL_FN_OVERLOADS,
    SK_SYMBOL_LOCAL,
};

struct sk_symbol_function {
    struct sk_type *type;
    sk_fnptr fnptr;
};

struct sk_symbol_fn_overloads {
    // TODO: Currently only a single overload is supported.
    struct sk_symbol_function overloads;
};

struct sk_symbol_local {
    struct sk_type *type;
    size_t slot;
};

struct sk_symbol {
    struct sk_token name;
    enum sk_symbol_type type;
    union {
        struct sk_symbol_fn_overloads fn_overloads;
        struct sk_symbol_local local;
    } as;
};

struct sk_symbol_arena_block {
    struct sk_symbol *symbols;
    size_t capacity;
    size_t count;
};

struct sk_symbol_arena {
    struct sk_symbol_arena_block *blocks;
    size_t capacity;
    size_t count;
    size_t current_block_index;
    size_t initial_block_capacity;
    size_t block_capacity;
};

struct sk_symbol_table {
    size_t count;
    struct sk_hashmap symbols_map;
};

void sk_symbol_table_init(struct sk_symbol_table *table);
void sk_symbol_table_free(struct sk_symbol_table *table);

struct sk_scope {
    struct sk_scope *parent;
    struct sk_symbol_table symbols;
};

void sk_scope_init(struct sk_scope *scope);
void sk_scope_free(struct sk_scope *scope);

struct sk_checker {
    bool has_error;
    struct sk_type_arena type_arena;
    struct sk_symbol_arena symbol_arena;
    struct sk_scope global_scope;
    struct sk_scope *current_scope;
    const struct sk_type *current_function_type;
    size_t next_local_slot;
    sk_fnptr next_fnptr;
};

void sk_checker_init(struct sk_checker *checker);
void sk_checker_free(struct sk_checker *checker);
bool sk_checker_check(struct sk_checker *checker, const struct sk_ast_node *root);

#endif // SKARD_SK_CHECKER_H
