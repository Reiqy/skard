#ifndef SKARD_SK_TYPE_H
#define SKARD_SK_TYPE_H

#include <stdbool.h>
#include <stddef.h>

struct sk_type_array {
    struct sk_type *types;
    size_t capacity;
    size_t count;
};

enum sk_type_kind {
    SK_TYPE_INVALID,
    SK_TYPE_NOTHING,
    SK_TYPE_NUMBER,
    SK_TYPE_BOOLEAN,
    SK_TYPE_STRING,
    SK_TYPE_FUNCTION,
};

struct sk_function_type {
    struct sk_type_array parameters;
    struct sk_type *return_type;
};

struct sk_type {
    enum sk_type_kind kind;
    union {
        struct sk_function_type function;
    } as;
};

struct sk_type_arena_block {
    struct sk_type *types;
    size_t capacity;
    size_t count;
};

struct sk_type_arena {
    struct sk_type_arena_block *blocks;
    size_t capacity;
    size_t count;
    size_t current_block_index;
    size_t initial_block_capacity;
    size_t block_capacity;
};

void sk_type_arena_init(struct sk_type_arena *arena, size_t block_capacity);
void sk_type_arena_free(struct sk_type_arena *arena);
struct sk_type *sk_type_arena_alloc(struct sk_type_arena *arena);
struct sk_type *sk_type_arena_alloc_array(struct sk_type_arena *arena, size_t count);

bool sk_type_equal(const struct sk_type *left, const struct sk_type *right);

#endif // SKARD_SK_TYPE_H
