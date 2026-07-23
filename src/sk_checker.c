#include "sk_checker.h"

#include "sk_memory.h"

static struct sk_symbol_arena_block *symbol_arena_add_block(struct sk_symbol_arena *arena, size_t capacity);

void sk_symbol_arena_init(struct sk_symbol_arena *arena, size_t block_capacity)
{
    arena->blocks = NULL;
    arena->capacity = 0;
    arena->count = 0;
    arena->current_block_index = 0;
    arena->initial_block_capacity = block_capacity == 0 ? 8 : block_capacity;
    arena->block_capacity = arena->initial_block_capacity;
}

void sk_symbol_arena_free(struct sk_symbol_arena *arena)
{
    for (size_t i = 0; i < arena->count; i++) {
        sk_free(arena->blocks[i].symbols);
    }

    sk_free(arena->blocks);
    sk_symbol_arena_init(arena, arena->initial_block_capacity);
}

struct sk_symbol *sk_symbol_arena_alloc(struct sk_symbol_arena *arena)
{
    if (arena->current_block_index >= arena->count) {
        symbol_arena_add_block(arena, arena->block_capacity);
    }

    struct sk_symbol_arena_block *current_block = arena->blocks + arena->current_block_index;
    struct sk_symbol *result = &current_block->symbols[current_block->count++];

    if (current_block->count >= current_block->capacity) {
        arena->current_block_index++;
    }

    return result;
}

static struct sk_symbol_arena_block *symbol_arena_add_block(struct sk_symbol_arena *arena, size_t capacity)
{
    if (arena->count >= arena->capacity) {
        arena->capacity = sk_grow(arena->capacity);
        arena->blocks = sk_realloc(arena->blocks, arena->capacity);
    }

    struct sk_symbol_arena_block *block = arena->blocks + arena->current_block_index;
    *block = (struct sk_symbol_arena_block) {
        .symbols = sk_allocs(capacity * sizeof(struct sk_symbol)),
        .capacity = capacity,
        .count = 0,
    };

    arena->block_capacity = sk_grow(capacity);
    arena->count = arena->current_block_index + 1;
    return block;
}

void sk_symbol_table_init(struct sk_symbol_table *table)
{
    sk_symbol_arena_init(&table->arena, 256);
    table->count = 0;
    sk_hashmap_init(&table->symbols_map);
}

void sk_symbol_table_free(struct sk_symbol_table *table)
{
    sk_symbol_arena_free(&table->arena);
    sk_hashmap_free(&table->symbols_map);
    sk_symbol_table_init(table);
}

bool sk_symbol_table_add(struct sk_symbol_table *table, struct sk_symbol symbol)
{
    void *existing = NULL;
    if (sk_hashmap_get(&table->symbols_map, symbol.name.start, symbol.name.length, &existing)) {
        return false;
    }

    struct sk_symbol *stored = sk_symbol_arena_alloc(&table->arena);
    *stored = symbol;
    table->count++;

    sk_hashmap_set(&table->symbols_map, stored->name.start, stored->name.length, stored);

    return true;
}

void sk_checker_init(struct sk_checker *checker)
{
    checker->has_error = false;
    sk_type_arena_init(&checker->type_arena, 256);
    sk_symbol_table_init(&checker->symbols);
}

void sk_checker_free(struct sk_checker *checker)
{
    sk_symbol_table_free(&checker->symbols);
    sk_type_arena_free(&checker->type_arena);
    checker->has_error = false;
}

bool sk_checker_check(struct sk_checker *checker)
{
    return !checker->has_error;
}
