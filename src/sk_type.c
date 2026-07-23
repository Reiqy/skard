#include "sk_type.h"

#include "sk_memory.h"

static struct sk_type_arena_block *type_arena_add_block(struct sk_type_arena *arena, size_t capacity);

void sk_type_arena_init(struct sk_type_arena *arena, size_t block_capacity)
{
    arena->blocks = NULL;
    arena->capacity = 0;
    arena->count = 0;
    arena->current_block_index = 0;
    arena->initial_block_capacity = block_capacity == 0 ? 8 : block_capacity;
    arena->block_capacity = arena->initial_block_capacity;
}

void sk_type_arena_free(struct sk_type_arena *arena)
{
    for (size_t i = 0; i < arena->count; i++) {
        sk_free(arena->blocks[i].types);
    }

    sk_free(arena->blocks);
    sk_type_arena_init(arena, arena->initial_block_capacity);
}

struct sk_type *sk_type_arena_alloc(struct sk_type_arena *arena)
{
    return sk_type_arena_alloc_array(arena, 1);
}

struct sk_type *sk_type_arena_alloc_array(struct sk_type_arena *arena, size_t count)
{
    if (count == 0) {
        return NULL;
    }

    if (arena->current_block_index >= arena->count ||
        arena->blocks[arena->current_block_index].count + count > arena->blocks[arena->current_block_index].capacity) {
        if (arena->current_block_index < arena->count) {
            arena->current_block_index++;
        }

        size_t capacity = arena->block_capacity;
        if (capacity < count) {
            capacity = count;
        }

        type_arena_add_block(arena, capacity);
    }

    struct sk_type_arena_block *current_block = arena->blocks + arena->current_block_index;
    struct sk_type *result = current_block->types + current_block->count;
    current_block->count += count;

    if (current_block->count >= current_block->capacity) {
        arena->current_block_index++;
    }

    return result;
}

static struct sk_type_arena_block *type_arena_add_block(struct sk_type_arena *arena, size_t capacity)
{
    if (arena->count >= arena->capacity) {
        arena->capacity = sk_grow(arena->capacity);
        arena->blocks = sk_realloc(arena->blocks, arena->capacity);
    }

    struct sk_type_arena_block *block = arena->blocks + arena->current_block_index;
    *block = (struct sk_type_arena_block) {
        .types = sk_allocs(capacity * sizeof(struct sk_type)),
        .capacity = capacity,
        .count = 0,
    };

    arena->block_capacity = sk_grow(capacity);
    arena->count = arena->current_block_index + 1;
    return block;
}

static bool function_type_equal(const struct sk_type *left, const struct sk_type *right);

bool sk_type_equal(const struct sk_type *left, const struct sk_type *right)
{
    if (left == NULL || right == NULL || left->kind != right->kind) {
        return false;
    }

    switch (left->kind) {
        case SK_TYPE_NOTHING:
        case SK_TYPE_NUMBER:
        case SK_TYPE_BOOLEAN:
        case SK_TYPE_STRING:
            return true;

        case SK_TYPE_FUNCTION:
            return function_type_equal(left, right);

        default:
            return false;
    }
}

static bool function_type_equal(const struct sk_type *left, const struct sk_type *right)
{
    if (left->kind != SK_TYPE_FUNCTION || right->kind != SK_TYPE_FUNCTION) {
        return false;
    }

    const struct sk_function_type left_function = left->as.function;
    const struct sk_function_type right_function = right->as.function;
    if (left_function.parameters.count != right_function.parameters.count) {
        return false;
    }

    const size_t parameter_count = left_function.parameters.count;
    for (size_t i = 0; i < parameter_count; i++) {
        if (!sk_type_equal(&left_function.parameters.types[i], &right_function.parameters.types[i])) {
            return false;
        }
    }

    return sk_type_equal(left_function.return_type, right_function.return_type);
}
