#include "sk_checker.h"

#include <stdio.h>
#include <string.h>

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

static struct sk_type *resolve_type_expr(struct sk_checker *checker, const struct sk_ast_type *type_expr);
static struct sk_type *resolve_type_name_expr(struct sk_checker *checker, const struct sk_ast_type_name *type_expr);
static bool token_equals(const struct sk_token *token, const char *text);
static struct sk_type *make_type(struct sk_checker *checker, enum sk_type_kind kind);
static void checker_error(struct sk_checker *checker, const char *message);

static void collect_declaration(struct sk_checker *checker, const struct sk_ast_node *node);
static void collect_function(struct sk_checker *checker, const struct sk_ast_node *node);

bool sk_checker_check(struct sk_checker *checker, const struct sk_ast_node *root)
{
    checker->has_error = false;

    if (root == NULL || root->type != SK_AST_PROGRAM) {
        checker_error(checker, "Expected a program node.");
        return false;
    }

    const struct sk_ast_program *program = &root->as.program;
    for (size_t i = 0; i < program->declarations.count; i++) {
        const struct sk_ast_node *declaration = program->declarations.nodes[i];

        if (declaration == NULL) {
            checker_error(checker, "Missing top level declaration.");
            continue;
        }

        collect_declaration(checker, declaration);
    }

    return !checker->has_error;
}

static struct sk_type *resolve_type_expr(struct sk_checker *checker, const struct sk_ast_type *type_expr)
{
    if (type_expr == NULL) {
        checker_error(checker, "Missing type expression.");
        return make_type(checker, SK_TYPE_INVALID);
    }

    switch (type_expr->kind) {
        case SK_AST_TYPE_NAME:
            return resolve_type_name_expr(checker, &type_expr->as.name);
        default:
            checker_error(checker, "Unsupported type expression.");
            return make_type(checker, SK_TYPE_INVALID);
    }
}

static struct sk_type *resolve_type_name_expr(struct sk_checker *checker, const struct sk_ast_type_name *type_expr)
{
    if (type_expr != NULL) {
        if (token_equals(&type_expr->name, "Number")) {
            return make_type(checker, SK_TYPE_NUMBER);
        }

        if (token_equals(&type_expr->name, "Boolean")) {
            return make_type(checker, SK_TYPE_BOOLEAN);
        }

        if (token_equals(&type_expr->name, "String")) {
            return make_type(checker, SK_TYPE_STRING);
        }

        if (token_equals(&type_expr->name, "Nothing")) {
            return make_type(checker, SK_TYPE_NOTHING);
        }
    }

    checker_error(checker, "Unknown type name.");
    return make_type(checker, SK_TYPE_INVALID);
}

static void collect_declaration(struct sk_checker *checker, const struct sk_ast_node *node)
{
    switch (node->type) {
        case SK_AST_FN:
            collect_function(checker, node);
            break;
        default:
            checker_error(checker, "Unsupported top level declaration.");
            break;
    }
}

static void collect_function(struct sk_checker *checker, const struct sk_ast_node *node)
{
    const struct sk_ast_fn *function = &node->as.fn;
    struct sk_type *function_type = make_type(checker, SK_TYPE_FUNCTION);

    function_type->as.function.parameters.count = function->parameters.count;
    function_type->as.function.parameters.capacity = function->parameters.count;
    function_type->as.function.parameters.types = sk_type_arena_alloc_array(
        &checker->type_arena,
        function->parameters.count);

    for (size_t i = 0; i < function->parameters.count; i++) {
        struct sk_type *resolved_type = resolve_type_expr(checker, &function->parameters.parameters[i].type);
        function_type->as.function.parameters.types[i] = *resolved_type;
    }

    if (function->has_return_type) {
        function_type->as.function.return_type = resolve_type_expr(checker, &function->return_type);
    } else {
        function_type->as.function.return_type = make_type(checker, SK_TYPE_NOTHING);
    }

    const struct sk_symbol symbol = {
        .name = function->name,
        .type = SK_SYMBOL_FN_OVERLOADS,
        .as.fn_overloads = {
            .overloads = {
                .type = function_type,
            },
        },
    };

    if (!sk_symbol_table_add(&checker->symbols, symbol)) {
        checker_error(checker, "Function already declared.");
    }
}

static bool token_equals(const struct sk_token *token, const char *text)
{
    const size_t length = strlen(text);
    return token->length == length && memcmp(token->start, text, length) == 0;
}

static struct sk_type *make_type(struct sk_checker *checker, enum sk_type_kind kind)
{
    struct sk_type *type = sk_type_arena_alloc(&checker->type_arena);
    *type = (struct sk_type) {
        .kind = kind,
    };
    return type;
}

static void checker_error(struct sk_checker *checker, const char *message)
{
    fprintf(stderr, "Semantic error: %s\n", message);
    checker->has_error = true;
}
