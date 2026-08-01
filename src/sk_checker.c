#include "sk_checker.h"

#include <string.h>

#include "sk_log.h"
#include "sk_memory.h"

static struct sk_symbol_arena_block *symbol_arena_add_block(struct sk_symbol_arena *arena, size_t capacity);
static void sk_symbol_arena_init(struct sk_symbol_arena *arena, size_t block_capacity);
static void sk_symbol_arena_free(struct sk_symbol_arena *arena);
static struct sk_symbol *sk_symbol_arena_alloc(struct sk_symbol_arena *arena);
static struct sk_symbol *sk_symbol_table_add(
    struct sk_symbol_arena *arena,
    struct sk_symbol_table *table,
    struct sk_symbol symbol);
static struct sk_symbol *sk_symbol_table_replace(
    struct sk_symbol_arena *arena,
    struct sk_symbol_table *table,
    struct sk_symbol symbol);

static void sk_symbol_arena_init(struct sk_symbol_arena *arena, const size_t block_capacity)
{
    arena->blocks = NULL;
    arena->capacity = 0;
    arena->count = 0;
    arena->current_block_index = 0;
    arena->initial_block_capacity = block_capacity == 0 ? 8 : block_capacity;
    arena->block_capacity = arena->initial_block_capacity;
}

static void sk_symbol_arena_free(struct sk_symbol_arena *arena)
{
    for (size_t i = 0; i < arena->count; i++) {
        sk_free(arena->blocks[i].symbols);
    }

    sk_free(arena->blocks);
    sk_symbol_arena_init(arena, arena->initial_block_capacity);
}

static struct sk_symbol *sk_symbol_arena_alloc(struct sk_symbol_arena *arena)
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

static struct sk_symbol_arena_block *symbol_arena_add_block(struct sk_symbol_arena *arena, const size_t capacity)
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
    table->count = 0;
    sk_hashmap_init(&table->symbols_map);
}

void sk_symbol_table_free(struct sk_symbol_table *table)
{
    sk_hashmap_free(&table->symbols_map);
    sk_symbol_table_init(table);
}

static struct sk_symbol *sk_symbol_table_add(
    struct sk_symbol_arena *arena,
    struct sk_symbol_table *table,
    const struct sk_symbol symbol)
{
    void *existing = NULL;
    if (sk_hashmap_get(&table->symbols_map, symbol.name.start, symbol.name.length, &existing)) {
        return NULL;
    }

    struct sk_symbol *stored = sk_symbol_arena_alloc(arena);
    *stored = symbol;
    table->count++;

    sk_hashmap_set(&table->symbols_map, stored->name.start, stored->name.length, stored);

    return stored;
}

static struct sk_symbol *sk_symbol_table_replace(
    struct sk_symbol_arena *arena,
    struct sk_symbol_table *table,
    const struct sk_symbol symbol)
{
    void *existing = NULL;
    const bool is_new = !sk_hashmap_get(&table->symbols_map, symbol.name.start, symbol.name.length, &existing);

    struct sk_symbol *stored = sk_symbol_arena_alloc(arena);
    *stored = symbol;

    if (is_new) {
        table->count++;
    }

    sk_hashmap_set(&table->symbols_map, stored->name.start, stored->name.length, stored);
    return stored;
}

void sk_scope_init(struct sk_scope *scope)
{
    scope->parent = NULL;
    sk_symbol_table_init(&scope->symbols);
}

void sk_scope_free(struct sk_scope *scope)
{
    sk_symbol_table_free(&scope->symbols);
    scope->parent = NULL;
}

void sk_checker_init(struct sk_checker *checker)
{
    checker->has_error = false;
    sk_type_arena_init(&checker->type_arena, 256);
    sk_symbol_arena_init(&checker->symbol_arena, 256);
    sk_scope_init(&checker->global_scope);
    checker->current_scope = &checker->global_scope;
    checker->current_function_type = NULL;
    checker->next_local_slot = 0;
    checker->next_fnptr = 0;
}

void sk_checker_free(struct sk_checker *checker)
{
    sk_scope_free(&checker->global_scope);
    checker->current_scope = NULL;
    checker->current_function_type = NULL;
    sk_symbol_arena_free(&checker->symbol_arena);
    sk_type_arena_free(&checker->type_arena);
    checker->has_error = false;
}

static struct sk_type *resolve_type_expr(struct sk_checker *checker, const struct sk_ast_type *type_expr);
static struct sk_type *resolve_type_name_expr(struct sk_checker *checker, const struct sk_ast_type_name *type_expr);
static bool token_equals(const struct sk_token *token, const char *text);
static struct sk_type *make_type(struct sk_checker *checker, enum sk_type_kind kind);
static const struct sk_token *token_for_node(const struct sk_ast_node *node);
static void checker_error(struct sk_checker *checker, const struct sk_token *token, const char *message);
static void checker_type_error(struct sk_checker *checker, const struct sk_token *token, const char *message);
static struct sk_scope *checker_push_scope(struct sk_checker *checker);
static void checker_pop_scope(struct sk_checker *checker);
static struct sk_symbol *checker_add_symbol(struct sk_checker *checker, struct sk_symbol symbol);

static struct sk_symbol *lookup_symbol(const struct sk_scope *scope, const struct sk_token *name);
static void check_node(struct sk_checker *checker, struct sk_ast_node *node);
static struct sk_type *check_expression(
    struct sk_checker *checker,
    struct sk_ast_node *node,
    const struct sk_type *expected_type);
static struct sk_type *check_literal(struct sk_checker *checker, const struct sk_ast_node *node);
static struct sk_type *check_identifier(struct sk_checker *checker, struct sk_ast_node *node);
static struct sk_type *check_unary(struct sk_checker *checker, const struct sk_ast_node *node);
static struct sk_type *check_binary(struct sk_checker *checker, const struct sk_ast_node *node);
static struct sk_type *check_call(struct sk_checker *checker, const struct sk_ast_node *node);
static void check_block(struct sk_checker *checker, const struct sk_ast_node *node);
static void check_let(
    struct sk_checker *checker,
    struct sk_ast_node *node,
    struct sk_token name,
    bool has_type,
    const struct sk_ast_type *type_expr,
    bool has_initializer,
    struct sk_ast_node *expression);
static struct sk_type *check_assignment(struct sk_checker *checker, struct sk_ast_node *node);
static void check_if(struct sk_checker *checker, const struct sk_ast_node *node);
static void check_while(struct sk_checker *checker, const struct sk_ast_node *node);
static void check_return(struct sk_checker *checker, const struct sk_ast_node *node);
static void check_print(struct sk_checker *checker, const struct sk_ast_node *node);
static void check_expr_stmt(struct sk_checker *checker, const struct sk_ast_node *node);

static void collect_declarations(struct sk_checker *checker, const struct sk_ast_program *program);
static void collect_declaration(struct sk_checker *checker, struct sk_ast_node *node);
static void collect_function(struct sk_checker *checker, struct sk_ast_node *node);
static void check_declarations(struct sk_checker *checker, const struct sk_ast_program *program);
static void check_declaration(struct sk_checker *checker, struct sk_ast_node *node);
static void check_function_parameters(struct sk_checker *checker, const struct sk_ast_fn *function);
static void check_function(struct sk_checker *checker, struct sk_ast_node *node);

bool sk_checker_check(struct sk_checker *checker, const struct sk_ast_node *root)
{
    checker->has_error = false;

    if (root == NULL || root->type != SK_AST_PROGRAM) {
        checker_error(checker, NULL, "Expected a program node.");
        return false;
    }

    const struct sk_ast_program *program = &root->as.program;
    collect_declarations(checker, program);
    check_declarations(checker, program);

    return !checker->has_error;
}

static void collect_declarations(struct sk_checker *checker, const struct sk_ast_program *program)
{
    for (size_t i = 0; i < program->declarations.count; i++) {
        struct sk_ast_node *declaration = program->declarations.nodes[i];

        if (declaration == NULL) {
            checker_error(checker, NULL, "Missing top level declaration.");
            continue;
        }

        collect_declaration(checker, declaration);
    }
}

static struct sk_type *resolve_type_expr(struct sk_checker *checker, const struct sk_ast_type *type_expr)
{
    if (type_expr == NULL) {
        checker_error(checker, NULL, "Missing type expression.");
        return make_type(checker, SK_TYPE_INVALID);
    }

    switch (type_expr->kind) {
        case SK_AST_TYPE_NAME:
            return resolve_type_name_expr(checker, &type_expr->as.name);
        default:
            checker_error(checker, NULL, "Unsupported type expression.");
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

    checker_error(checker, &type_expr->name, "Unknown type name.");
    return make_type(checker, SK_TYPE_INVALID);
}

static void collect_declaration(struct sk_checker *checker, struct sk_ast_node *node)
{
    switch (node->type) {
        case SK_AST_FN:
            collect_function(checker, node);
            break;
        default:
            checker_error(checker, token_for_node(node), "Unsupported top level declaration.");
            break;
    }
}

static void collect_function(struct sk_checker *checker, struct sk_ast_node *node)
{
    struct sk_ast_fn *function = &node->as.fn;
    struct sk_type *function_type = make_type(checker, SK_TYPE_FUNCTION);

    function_type->as.function.parameters.count = function->parameters.count;
    function_type->as.function.parameters.capacity = function->parameters.count;
    function_type->as.function.parameters.types = sk_type_arena_alloc_array(
        &checker->type_arena,
        function->parameters.count);

    for (size_t i = 0; i < function->parameters.count; i++) {
        const struct sk_type *resolved_type = resolve_type_expr(checker, &function->parameters.parameters[i].type);
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
                .fnptr = 0,
            },
        },
    };

    struct sk_symbol *stored = checker_add_symbol(checker, symbol);
    if (stored == NULL) {
        checker_error(checker, &function->name, "Function already declared.");
    } else {
        stored->as.fn_overloads.overloads.fnptr = checker->next_fnptr++;
        function->symbol = stored;
    }
}

static void check_declarations(struct sk_checker *checker, const struct sk_ast_program *program)
{
    for (size_t i = 0; i < program->declarations.count; i++) {
        struct sk_ast_node *declaration = program->declarations.nodes[i];

        if (declaration != NULL) {
            check_declaration(checker, declaration);
        }
    }
}

static void check_declaration(struct sk_checker *checker, struct sk_ast_node *node)
{
    switch (node->type) {
        case SK_AST_FN:
            check_function(checker, node);
            break;
        default:
            break;
    }
}

static void check_function(struct sk_checker *checker, struct sk_ast_node *node)
{
    struct sk_ast_fn *function = &node->as.fn;
    const struct sk_symbol *symbol = lookup_symbol(&checker->global_scope, &function->name);
    const struct sk_type *previous_function_type = checker->current_function_type;

    if (symbol != NULL && symbol->type == SK_SYMBOL_FN_OVERLOADS) {
        checker->current_function_type = symbol->as.fn_overloads.overloads.type;
    }

    checker->next_local_slot = 0;
    checker_push_scope(checker);
    check_function_parameters(checker, function);
    check_node(checker, function->body);
    function->locals_count = checker->next_local_slot;
    checker_pop_scope(checker);
    checker->current_function_type = previous_function_type;
}

static void check_function_parameters(struct sk_checker *checker, const struct sk_ast_fn *function)
{
    for (size_t i = 0; i < function->parameters.count; i++) {
        const struct sk_ast_parameter *parameter = &function->parameters.parameters[i];
        check_let(checker, NULL, parameter->name, true, &parameter->type, false, NULL);
    }
}

static bool token_equals(const struct sk_token *token, const char *text)
{
    const size_t length = strlen(text);
    return token->length == length && memcmp(token->start, text, length) == 0;
}

static struct sk_type *make_type(struct sk_checker *checker, const enum sk_type_kind kind)
{
    struct sk_type *type = sk_type_arena_alloc(&checker->type_arena);
    *type = (struct sk_type) {
        .kind = kind,
    };
    return type;
}

static const struct sk_token *token_for_node(const struct sk_ast_node *node)
{
    static const struct sk_token fallback = {
        .filename = "<checker>",
    };

    if (node == NULL) {
        return &fallback;
    }

    switch (node->type) {
        case SK_AST_LITERAL:
            return &node->as.literal.token;
        case SK_AST_IDENTIFIER:
            return &node->as.identifier.token;
        case SK_AST_UNARY:
            return &node->as.unary.operator;
        case SK_AST_BINARY:
            return &node->as.binary.operator;
        case SK_AST_CALL:
            return token_for_node(node->as.call.callee);
        case SK_AST_ASSIGN:
            return &node->as.assign.name;
        case SK_AST_BLOCK:
            return node->as.block.contents.count == 0 ? &fallback : token_for_node(node->as.block.contents.nodes[0]);
        case SK_AST_LET:
            return &node->as.let.name;
        case SK_AST_IF:
            return token_for_node(node->as.ifn.condition);
        case SK_AST_WHILE:
            return token_for_node(node->as.whilen.condition);
        case SK_AST_RETURN:
            return node->as.returnn.expression == NULL ? &node->as.returnn.keyword
                                                       : token_for_node(node->as.returnn.expression);
        case SK_AST_PRINT:
            return node->as.print.args.count == 0 ? &node->as.print.keyword
                                                  : token_for_node(node->as.print.args.nodes[0]);
        case SK_AST_EXPR_STMT:
            return token_for_node(node->as.expr_stmt.expression);
        case SK_AST_FN:
            return &node->as.fn.name;
        case SK_AST_PROGRAM:
            return node->as.program.declarations.count == 0 ? &fallback
                                                            : token_for_node(node->as.program.declarations.nodes[0]);
        case SK_AST_TYPE:
            return &node->as.type.as.name.name;
        case SK_AST_ERR:
            return &fallback;
    }

    return &fallback;
}

static void checker_error(struct sk_checker *checker, const struct sk_token *token, const char *message)
{
    if (token == NULL) {
        token = token_for_node(NULL);
    }

    sk_error(token->filename, token->line, token->column, message);
    checker->has_error = true;
}

static void checker_type_error(struct sk_checker *checker, const struct sk_token *token, const char *message)
{
    checker_error(checker, token, message);
}

static struct sk_scope *checker_push_scope(struct sk_checker *checker)
{
    struct sk_scope *scope = sk_alloc(struct sk_scope);
    sk_scope_init(scope);
    scope->parent = checker->current_scope;
    checker->current_scope = scope;
    return scope;
}

static void checker_pop_scope(struct sk_checker *checker)
{
    if (checker->current_scope == NULL || checker->current_scope == &checker->global_scope) {
        return;
    }

    struct sk_scope *scope = checker->current_scope;
    checker->current_scope = scope->parent;
    sk_scope_free(scope);
    sk_free(scope);
}

static struct sk_symbol *checker_add_symbol(struct sk_checker *checker, const struct sk_symbol symbol)
{
    return sk_symbol_table_add(&checker->symbol_arena, &checker->current_scope->symbols, symbol);
}

static struct sk_symbol *lookup_symbol(const struct sk_scope *scope, const struct sk_token *name)
{
    for (const struct sk_scope *current = scope; current != NULL; current = current->parent) {
        void *value = NULL;
        if (sk_hashmap_get(&current->symbols.symbols_map, name->start, name->length, &value)) {
            return value;
        }
    }

    return NULL;
}

static void check_node(struct sk_checker *checker, struct sk_ast_node *node)
{
    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case SK_AST_LITERAL:
        case SK_AST_IDENTIFIER:
        case SK_AST_UNARY:
        case SK_AST_BINARY:
        case SK_AST_CALL:
        case SK_AST_ASSIGN:
            check_expression(checker, node, NULL);
            break;
        case SK_AST_BLOCK:
            check_block(checker, node);
            break;
        case SK_AST_LET:
            check_let(
                checker,
                node,
                node->as.let.name,
                node->as.let.has_type,
                &node->as.let.type,
                node->as.let.has_initializer,
                node->as.let.expression);
            break;
        case SK_AST_IF:
            check_if(checker, node);
            break;
        case SK_AST_WHILE:
            check_while(checker, node);
            break;
        case SK_AST_RETURN:
            check_return(checker, node);
            break;
        case SK_AST_PRINT:
            check_print(checker, node);
            break;
        case SK_AST_EXPR_STMT:
            check_expr_stmt(checker, node);
            break;
        default:
            break;
    }
}

static struct sk_type *check_expression(
    struct sk_checker *checker,
    struct sk_ast_node *node,
    const struct sk_type *expected_type)
{
    if (node == NULL) {
        checker_error(checker, NULL, "Missing expression.");
        return make_type(checker, SK_TYPE_INVALID);
    }

    struct sk_type *actual_type = NULL;
    switch (node->type) {
        case SK_AST_LITERAL:
            actual_type = check_literal(checker, node);
            break;
        case SK_AST_IDENTIFIER:
            actual_type = check_identifier(checker, node);
            break;
        case SK_AST_UNARY:
            actual_type = check_unary(checker, node);
            break;
        case SK_AST_BINARY:
            actual_type = check_binary(checker, node);
            break;
        case SK_AST_CALL:
            actual_type = check_call(checker, node);
            break;
        case SK_AST_ASSIGN:
            actual_type = check_assignment(checker, node);
            break;
        default:
            checker_error(checker, token_for_node(node), "Expected expression.");
            return make_type(checker, SK_TYPE_INVALID);
    }

    if (expected_type != NULL && actual_type->kind != SK_TYPE_INVALID && expected_type->kind != SK_TYPE_UNKNOWN &&
        !sk_type_equal(actual_type, expected_type)) {
        checker_type_error(checker, token_for_node(node), "Expression type does not match expected type.");
        return make_type(checker, SK_TYPE_INVALID);
    }

    return actual_type;
}

static void check_expr_stmt(struct sk_checker *checker, const struct sk_ast_node *node)
{
    check_expression(checker, node->as.expr_stmt.expression, NULL);
}

static struct sk_type *check_literal(struct sk_checker *checker, const struct sk_ast_node *node)
{
    switch (node->as.literal.token.type) {
        case SK_TOKEN_NUMBER:
            return make_type(checker, SK_TYPE_NUMBER);
        case SK_TOKEN_STRING:
            return make_type(checker, SK_TYPE_STRING);
        case SK_TOKEN_TRUE:
        case SK_TOKEN_FALSE:
            return make_type(checker, SK_TYPE_BOOLEAN);
        default:
            checker_error(checker, &node->as.literal.token, "Unsupported literal.");
            return make_type(checker, SK_TYPE_INVALID);
    }
}

static struct sk_type *check_identifier(struct sk_checker *checker, struct sk_ast_node *node)
{
    struct sk_symbol *symbol = lookup_symbol(checker->current_scope, &node->as.identifier.token);
    if (symbol == NULL) {
        checker_error(checker, &node->as.identifier.token, "Unknown identifier.");
        return make_type(checker, SK_TYPE_INVALID);
    }

    node->as.identifier.symbol = symbol;

    if (symbol->type == SK_SYMBOL_FN_OVERLOADS) {
        return symbol->as.fn_overloads.overloads.type;
    }

    if (symbol->type != SK_SYMBOL_LOCAL) {
        checker_error(checker, &node->as.identifier.token, "Expected a value.");
        return make_type(checker, SK_TYPE_INVALID);
    }

    if (symbol->as.local.type->kind == SK_TYPE_UNKNOWN) {
        checker_error(checker, &node->as.identifier.token, "Cannot use a variable before its type is inferred.");
        return make_type(checker, SK_TYPE_INVALID);
    }

    return symbol->as.local.type;
}

static struct sk_type *check_unary(struct sk_checker *checker, const struct sk_ast_node *node)
{
    struct sk_type *operand_type = check_expression(checker, node->as.unary.expression, NULL);
    if (operand_type->kind == SK_TYPE_INVALID) {
        return operand_type;
    }

    switch (node->as.unary.operator.type) {
        case SK_TOKEN_PLUS:
        case SK_TOKEN_MINUS:
            if (operand_type->kind != SK_TYPE_NUMBER) {
                checker_type_error(checker, &node->as.unary.operator, "Unary numeric operator requires Number.");
                return make_type(checker, SK_TYPE_INVALID);
            }
            return make_type(checker, SK_TYPE_NUMBER);
        case SK_TOKEN_NOT:
            if (operand_type->kind != SK_TYPE_BOOLEAN) {
                checker_type_error(checker, &node->as.unary.operator, "Not operator requires Boolean.");
                return make_type(checker, SK_TYPE_INVALID);
            }
            return make_type(checker, SK_TYPE_BOOLEAN);
        default:
            checker_error(checker, &node->as.unary.operator, "Unsupported unary operator.");
            return make_type(checker, SK_TYPE_INVALID);
    }
}

static struct sk_type *check_binary(struct sk_checker *checker, const struct sk_ast_node *node)
{
    const struct sk_type *left_type = check_expression(checker, node->as.binary.left, NULL);
    const struct sk_type *right_type = check_expression(checker, node->as.binary.right, NULL);
    if (left_type->kind == SK_TYPE_INVALID || right_type->kind == SK_TYPE_INVALID) {
        return make_type(checker, SK_TYPE_INVALID);
    }

    switch (node->as.binary.operator.type) {
        case SK_TOKEN_PLUS:
        case SK_TOKEN_MINUS:
        case SK_TOKEN_STAR:
        case SK_TOKEN_SLASH:
            if (left_type->kind != SK_TYPE_NUMBER || right_type->kind != SK_TYPE_NUMBER) {
                checker_type_error(checker, &node->as.binary.operator, "Arithmetic operator requires Number operands.");
                return make_type(checker, SK_TYPE_INVALID);
            }
            return make_type(checker, SK_TYPE_NUMBER);
        case SK_TOKEN_LESS:
        case SK_TOKEN_LESS_EQ:
        case SK_TOKEN_GREATER:
        case SK_TOKEN_GREATER_EQ:
            if (left_type->kind != SK_TYPE_NUMBER || right_type->kind != SK_TYPE_NUMBER) {
                checker_type_error(checker, &node->as.binary.operator, "Comparison operator requires Number operands.");
                return make_type(checker, SK_TYPE_INVALID);
            }
            return make_type(checker, SK_TYPE_BOOLEAN);
        case SK_TOKEN_EQUAL:
        case SK_TOKEN_NOT_EQUAL:
            if (!sk_type_equal(left_type, right_type)) {
                checker_type_error(checker, &node->as.binary.operator, "Equality operands must have the same type.");
                return make_type(checker, SK_TYPE_INVALID);
            }
            return make_type(checker, SK_TYPE_BOOLEAN);
        case SK_TOKEN_AND:
        case SK_TOKEN_OR:
            if (left_type->kind != SK_TYPE_BOOLEAN || right_type->kind != SK_TYPE_BOOLEAN) {
                checker_type_error(checker, &node->as.binary.operator, "Logical operator requires Boolean operands.");
                return make_type(checker, SK_TYPE_INVALID);
            }
            return make_type(checker, SK_TYPE_BOOLEAN);
        default:
            checker_error(checker, &node->as.binary.operator, "Unsupported binary operator.");
            return make_type(checker, SK_TYPE_INVALID);
    }
}

static struct sk_type *check_call(struct sk_checker *checker, const struct sk_ast_node *node)
{
    struct sk_type *callee_type = check_expression(checker, node->as.call.callee, NULL);
    if (callee_type->kind == SK_TYPE_INVALID) {
        return callee_type;
    }
    if (callee_type->kind != SK_TYPE_FUNCTION) {
        checker_type_error(checker, token_for_node(node->as.call.callee), "Can only call functions.");
        return make_type(checker, SK_TYPE_INVALID);
    }

    const struct sk_function_type *function_type = &callee_type->as.function;
    const struct sk_ast_node_array *args = &node->as.call.args;
    if (args->count != function_type->parameters.count) {
        checker_type_error(checker, token_for_node(node->as.call.callee), "Incorrect number of arguments.");
    }

    const size_t count = args->count < function_type->parameters.count ? args->count : function_type->parameters.count;
    for (size_t i = 0; i < count; i++) {
        check_expression(checker, args->nodes[i], &function_type->parameters.types[i]);
    }

    return function_type->return_type;
}

static void check_block(struct sk_checker *checker, const struct sk_ast_node *node)
{
    checker_push_scope(checker);

    for (size_t i = 0; i < node->as.block.contents.count; i++) {
        check_node(checker, node->as.block.contents.nodes[i]);
    }

    checker_pop_scope(checker);
}

static void check_let(
    struct sk_checker *checker,
    struct sk_ast_node *node,
    const struct sk_token name,
    const bool has_type,
    const struct sk_ast_type *type_expr,
    const bool has_initializer,
    struct sk_ast_node *expression)
{
    struct sk_type *declared_type = has_type ? resolve_type_expr(checker, type_expr)
                                             : make_type(checker, SK_TYPE_UNKNOWN);
    struct sk_type *type = declared_type;

    if (has_initializer) {
        struct sk_type *inferred_type = check_expression(checker, expression, has_type ? declared_type : NULL);
        if (!has_type && inferred_type->kind != SK_TYPE_INVALID) {
            type = inferred_type;
        }
    }

    if (checker->next_local_slot >= SK_MAX_LOCAL_SLOTS) {
        checker_error(checker, &name, "Too many local variables.");
        return;
    }

    const struct sk_symbol symbol = {
        .name = name,
        .type = SK_SYMBOL_LOCAL,
        .as.local = {
            .type = type,
            .slot = checker->next_local_slot,
        },
    };

    struct sk_symbol *stored = sk_symbol_table_replace(
        &checker->symbol_arena,
        &checker->current_scope->symbols,
        symbol);

    checker->next_local_slot++;
    if (node != NULL) {
        node->as.let.symbol = stored;
    }
}

static struct sk_type *check_assignment(struct sk_checker *checker, struct sk_ast_node *node)
{
    struct sk_symbol *symbol = lookup_symbol(checker->current_scope, &node->as.assign.name);
    if (symbol == NULL) {
        checker_error(checker, &node->as.assign.name, "Unknown identifier.");
        check_expression(checker, node->as.assign.expression, NULL);
        return make_type(checker, SK_TYPE_INVALID);
    }

    node->as.assign.symbol = symbol;

    if (symbol->type != SK_SYMBOL_LOCAL) {
        checker_error(checker, &node->as.assign.name, "Expected a local value.");
        check_expression(checker, node->as.assign.expression, NULL);
        return make_type(checker, SK_TYPE_INVALID);
    }

    struct sk_type *inferred_type = check_expression(
        checker,
        node->as.assign.expression,
        symbol->as.local.type->kind == SK_TYPE_UNKNOWN ? NULL : symbol->as.local.type);
    if (symbol->as.local.type->kind == SK_TYPE_UNKNOWN && inferred_type->kind != SK_TYPE_INVALID) {
        symbol->as.local.type = inferred_type;
    }

    return symbol->as.local.type;
}

static void check_if(struct sk_checker *checker, const struct sk_ast_node *node)
{
    const struct sk_type *boolean_type = make_type(checker, SK_TYPE_BOOLEAN);
    check_expression(checker, node->as.ifn.condition, boolean_type);
    check_node(checker, node->as.ifn.then_branch);
    check_node(checker, node->as.ifn.else_branch);
}

static void check_while(struct sk_checker *checker, const struct sk_ast_node *node)
{
    const struct sk_type *boolean_type = make_type(checker, SK_TYPE_BOOLEAN);
    check_expression(checker, node->as.whilen.condition, boolean_type);
    check_node(checker, node->as.whilen.body);
}

static void check_return(struct sk_checker *checker, const struct sk_ast_node *node)
{
    if (checker->current_function_type == NULL || checker->current_function_type->kind != SK_TYPE_FUNCTION) {
        checker_error(checker, &node->as.returnn.keyword, "Return outside of function.");
        return;
    }

    const struct sk_type *return_type = checker->current_function_type->as.function.return_type;
    if (node->as.returnn.expression == NULL) {
        if (return_type->kind != SK_TYPE_NOTHING) {
            checker_type_error(checker, &node->as.returnn.keyword, "Return requires a value.");
        }
        return;
    }

    check_expression(checker, node->as.returnn.expression, return_type);
}

static void check_print(struct sk_checker *checker, const struct sk_ast_node *node)
{
    if (node->as.print.args.count == 0) {
        checker_error(checker, &node->as.print.keyword, "Print requires a format string.");
        return;
    }

    for (size_t i = 0; i < node->as.print.args.count; i++) {
        check_expression(checker, node->as.print.args.nodes[i], NULL);
    }
}
