#include "sk_ast.h"

#include <stdio.h>

#include "sk_utils.h"

void sk_ast_node_array_init(struct sk_ast_node_array *array)
{
    array->nodes = NULL;
    array->capacity = 0;
    array->count = 0;
}

void sk_ast_node_array_free(struct sk_ast_node_array *array)
{
    sk_free(array->nodes);

    sk_ast_node_array_init(array);
}

void sk_ast_node_array_add(struct sk_ast_node_array *array, struct sk_ast_node *node)
{
    if (array->count >= array->capacity) {
        array->capacity = sk_grow(array->capacity);
        array->nodes = sk_realloc(array->nodes, array->capacity);
    }

    array->nodes[array->count] = node;
    array->count++;
}

static void ast_node_print_impl(const struct sk_ast_node *node, int depth);

static void print_indent(int depth);

static void print_null_node(int depth);

static void print_expression(const struct sk_ast_node *node, int depth);
static void print_parenthesized_expression(const struct sk_ast_node *node);

static void print_block(const struct sk_ast_node *node, int depth);
static void print_print(const struct sk_ast_node *node, int depth);
static void print_fn(const struct sk_ast_node *node, int depth);
static void print_program(const struct sk_ast_node *node, int depth);

void sk_ast_node_print(const struct sk_ast_node *node)
{
    ast_node_print_impl(node, 0);
}

static void print_expression(const struct sk_ast_node *node, int depth)
{
    print_indent(depth);
    print_parenthesized_expression(node);
    printf("\n");
}

static void print_parenthesized_expression(const struct sk_ast_node *node)
{
    switch (node->type) {
        case SK_AST_LITERAL:
            printf("%.*s", (int) node->as.literal.token.length, node->as.literal.token.start);
            break;
        case SK_AST_UNARY:
            printf("(");
            printf("%.*s", (int) node->as.unary.operator.length, node->as.unary.operator.start);
            print_parenthesized_expression(node->as.unary.expression);
            printf(")");
            break;
        case SK_AST_BINARY:
            printf("(");
            print_parenthesized_expression(node->as.binary.left);
            printf(" %.*s ", (int) node->as.binary.operator.length, node->as.binary.operator.start);
            print_parenthesized_expression(node->as.binary.right);
            printf(")");
            break;
        default:
            fprintf(stderr, "Unexpected node type %d.", node->type);
            break;
    }
}

static void print_block(const struct sk_ast_node *node, int depth)
{
    print_indent(depth);
    printf("{}\n");

    for (size_t i = 0; i < node->as.block.contents.count; i++) {
        ast_node_print_impl(node->as.block.contents.nodes[i], depth + 1);
    }
}

static void print_print(const struct sk_ast_node *node, int depth)
{
    print_indent(depth);
    printf("print\n");
    print_expression(node->as.print.expression, depth + 1);
}

static void print_fn(const struct sk_ast_node *node, int depth)
{
    print_indent(depth);
    printf("fn %.*s()\n", (int)node->as.fn.name.length, node->as.fn.name.start);
    ast_node_print_impl(node->as.fn.body, depth + 1);
}

static void print_program(const struct sk_ast_node *node, int depth)
{
    print_indent(depth);
    printf("program:\n");

    for (size_t i = 0; i < node->as.program.declarations.count; i++) {
        ast_node_print_impl(node->as.program.declarations.nodes[i], depth + 1);
    }
}

static void ast_node_print_impl(const struct sk_ast_node *node, int depth)
{
    if (node == NULL) {
        print_null_node(depth);
        return;
    }

    switch (node->type) {
        case SK_AST_BLOCK:
            print_block(node, depth);
            break;
        case SK_AST_PRINT:
            print_print(node, depth);
            break;
        case SK_AST_FN:
            print_fn(node, depth);
            break;
        case SK_AST_PROGRAM:
            print_program(node, depth);
            break;
        default:
            break;
    }
}

static void print_null_node(int depth)
{
    print_indent(depth);
    printf("ERROR\n");
}

static void print_indent(int depth)
{
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
}

void sk_ast_node_arena_init(struct sk_ast_node_arena *arena, size_t block_capacity)
{
    arena->blocks = NULL;
    arena->capacity = 0;
    arena->count = 0;
    arena->current_block_index = 0;
    arena->block_capacity = block_capacity == 0 ? 8 : block_capacity;
}

void sk_ast_node_arena_free(struct sk_ast_node_arena *arena)
{
    for (size_t i = 0; i < arena->count; i++) {
        sk_free(arena->blocks[i].nodes);
    }

    sk_free(arena->blocks);

    sk_ast_node_arena_init(arena, arena->block_capacity);
}

// TODO: Consider improving this function.
struct sk_ast_node *sk_ast_node_arena_alloc(struct sk_ast_node_arena *arena)
{
    if (arena->current_block_index >= arena->count) {
        if (arena->count >= arena->capacity) {
            arena->capacity = sk_grow(arena->capacity);
            arena->blocks = sk_realloc(arena->blocks, arena->capacity);
        }

        struct sk_ast_node_arena_block *current_block = arena->blocks + arena->current_block_index;
        *current_block = (struct sk_ast_node_arena_block) {
            .nodes = NULL,
            .count = 0,
            .capacity = arena->block_capacity,
        };

        current_block->nodes = sk_realloc(current_block->nodes, current_block->capacity);
        arena->block_capacity = sk_grow(arena->block_capacity);
        arena->count++;
    }

    struct sk_ast_node_arena_block *current_block = arena->blocks + arena->current_block_index;
    struct sk_ast_node *result = &current_block->nodes[current_block->count++];

    if (current_block->count >= current_block->capacity) {
        arena->current_block_index++;
    }

    return result;
}
