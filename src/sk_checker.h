#ifndef SKARD_SK_CHECKER_H
#define SKARD_SK_CHECKER_H

#include <stdbool.h>
#include <stddef.h>

#include "sk_hashmap.h"
#include "sk_lexer.h"
#include "sk_type.h"

enum sk_symbol_type {
    SK_SYMBOL_FN_OVERLOADS,
};

struct sk_symbol_function {
    struct sk_type *type;
};

struct sk_symbol_fn_overloads {
    // TODO: Currently only a single overload is supported.
    struct sk_symbol_function overloads;
};

struct sk_symbol {
    struct sk_token name;
    enum sk_symbol_type type;
    union {
        struct sk_symbol_fn_overloads fn_overloads;
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

void sk_symbol_arena_init(struct sk_symbol_arena *arena, size_t block_capacity);
void sk_symbol_arena_free(struct sk_symbol_arena *arena);
struct sk_symbol *sk_symbol_arena_alloc(struct sk_symbol_arena *arena);

struct sk_symbol_table {
    struct sk_symbol_arena arena;
    size_t count;
    struct sk_hashmap symbols_map;
};

void sk_symbol_table_init(struct sk_symbol_table *table);
void sk_symbol_table_free(struct sk_symbol_table *table);
bool sk_symbol_table_add(struct sk_symbol_table *table, struct sk_symbol symbol);

struct sk_checker {
    bool has_error;
    struct sk_type_arena type_arena;
    struct sk_symbol_table symbols;
};

void sk_checker_init(struct sk_checker *checker);
void sk_checker_free(struct sk_checker *checker);
bool sk_checker_check(struct sk_checker *checker);

#endif // SKARD_SK_CHECKER_H
