#include "sk_compiler.h"

#include <stdio.h>

#include "sk_parser.h"

static void compiler_error(struct sk_compiler *compiler, const char *msg);

static bool declare_local(struct sk_compiler *compiler, struct sk_token name, size_t *slot);

static bool resolve_local(const struct sk_compiler *compiler, struct sk_token name, size_t *slot);

static void emit(struct sk_compiler *compiler, uint8_t byte);
static void emit2(struct sk_compiler *compiler, uint8_t byte1, uint8_t byte2);
static void emit3(struct sk_compiler *compiler, uint8_t byte1, uint8_t byte2, uint8_t byte3);

static void emit_halt(struct sk_compiler *compiler);
static void emit_const(struct sk_compiler *compiler, struct sk_value constant);
static size_t emit_jmp(struct sk_compiler *compiler, uint8_t instruction);
static void emit_jmp_back(struct sk_compiler *compiler, size_t target_offset);

static void patch_jmp(struct sk_compiler *compiler, size_t offset);

static void compile_program(struct sk_compiler *compiler, struct sk_ast_node *node);

static void compile_declaration(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_function(struct sk_compiler *compiler, struct sk_ast_node *node);

static void compile_statement(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_block(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_let_statement(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_assign_statement(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_if_statement(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_while_statement(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_print_statement(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_return_statement(struct sk_compiler *compiler, struct sk_ast_node *node);

static void compile_expression_or_nothing(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_expression(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_binary(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_and(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_or(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_unary(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_identifier(struct sk_compiler *compiler, struct sk_ast_node *node);

static void compile_literal(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_number(struct sk_compiler *compiler, struct sk_ast_literal *literal);
static void compile_string(struct sk_compiler *compiler, struct sk_ast_literal *literal);

bool sk_compiler_compile(struct sk_compiler *compiler, struct sk_ast_node *node, struct sk_chunk *chunk)
{
    compiler->current_chunk = chunk;
    compiler->has_error = false;

    sk_hashmap_init(&compiler->locals_map);
    compiler->locals_count = 0;

    compile_program(compiler, node);
    chunk->locals_count = compiler->locals_count;
    emit_halt(compiler);

    bool success = !compiler->has_error;

    sk_hashmap_free(&compiler->locals_map);

    return success;
}

static void compiler_error(struct sk_compiler *compiler, const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    compiler->has_error = true;
}

static bool declare_local(struct sk_compiler *compiler, struct sk_token name, size_t *slot)
{
    void *existing = NULL;

    if (sk_hashmap_get(&compiler->locals_map, name.start, name.length, &existing)) {
        // TODO: Shadowing.
        compiler_error(compiler, "Local variable already declared.");
        return false;
    }

    if (compiler->locals_count >= SK_COMPILER_MAX_LOCALS) {
        compiler_error(compiler, "Too many local variables.");
        return false;
    }

    struct sk_compiler_local *local = &compiler->locals[compiler->locals_count];

    *slot = compiler->locals_count;
    *local = (struct sk_compiler_local) {
        .name = name,
        .slot = *slot,
    };

    compiler->locals_count++;

    sk_hashmap_set(&compiler->locals_map, name.start, name.length, local);

    return true;
}

static bool resolve_local(const struct sk_compiler *compiler, struct sk_token name, size_t *slot)
{
    void *value = NULL;

    if (!sk_hashmap_get(&compiler->locals_map, name.start, name.length, &value)) {
        return false;
    }

    const struct sk_compiler_local *local = value;
    *slot = local->slot;
    return true;
}

static void emit(struct sk_compiler *compiler, uint8_t byte)
{
    sk_chunk_add(compiler->current_chunk, byte);
}

static void emit2(struct sk_compiler *compiler, uint8_t byte1, uint8_t byte2)
{
    emit(compiler, byte1);
    emit(compiler, byte2);
}

static void emit3(struct sk_compiler *compiler, uint8_t byte1, uint8_t byte2, uint8_t byte3)
{
    emit(compiler, byte1);
    emit(compiler, byte2);
    emit(compiler, byte3);
}

static void emit_halt(struct sk_compiler *compiler)
{
    emit(compiler, SK_OP_HALT);
}

static void emit_const(struct sk_compiler *compiler, struct sk_value constant)
{
    sk_chunk_add_const(compiler->current_chunk, constant);
}

static size_t emit_jmp(struct sk_compiler *compiler, uint8_t instruction)
{
    emit3(compiler, instruction, 0xFF, 0xFF);
    return compiler->current_chunk->count - 2;
}

static void emit_jmp_back(struct sk_compiler *compiler, size_t target_offset)
{
    emit(compiler, SK_OP_JMP_BACK);

    size_t offset = compiler->current_chunk->count - target_offset + 2;
    if (offset > UINT16_MAX) {
        compiler_error(compiler, "Too long jump.");
        return;
    }

    emit(compiler, (offset >> 8) & 0xFF);
    emit(compiler, offset & 0xFF);
}

static void patch_jmp(struct sk_compiler *compiler, size_t offset)
{
    size_t jmp_offset = compiler->current_chunk->count - offset - 2;

    if (jmp_offset > UINT16_MAX) {
        compiler_error(compiler, "Too long jump.");
    }

    compiler->current_chunk->code[offset] = (jmp_offset >> 8) & 0xFF;
    compiler->current_chunk->code[offset + 1] = jmp_offset & 0xFF;
}

static void compile_program(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    struct sk_ast_program *program = &node->as.program;
    for (size_t i = 0; i < program->declarations.count; i++) {
        compile_declaration(compiler, program->declarations.nodes[i]);
        // TODO: Only a single declaration is allowed for now.
        break;
    }
}

static void compile_declaration(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    if (node->type == SK_AST_FN) {
        compile_function(compiler, node);
    }
}

static void compile_function(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    struct sk_ast_fn *fn = &node->as.fn;
    compile_block(compiler, fn->body);
}

static void compile_statement(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    switch (node->type) {
        case SK_AST_BLOCK:
            compile_block(compiler, node);
            break;
        case SK_AST_LET:
            compile_let_statement(compiler, node);
            break;
        case SK_AST_ASSIGN:
            compile_assign_statement(compiler, node);
            break;
        case SK_AST_IF:
            compile_if_statement(compiler, node);
            break;
        case SK_AST_WHILE:
            compile_while_statement(compiler, node);
            break;
        case SK_AST_PRINT:
            compile_print_statement(compiler, node);
            break;
        case SK_AST_RETURN:
            compile_return_statement(compiler, node);
            break;
        default:
            compiler_error(compiler, "Unsupported statement.");
            break;
    }
}

static void compile_block(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    struct sk_ast_block *block = &node->as.block;
    for (size_t i = 0; i < block->contents.count; i++) {
        compile_statement(compiler, block->contents.nodes[i]);
    }
}

static void compile_let_statement(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    struct sk_ast_let *let = &node->as.let;

    compile_expression(compiler, let->expression);
    if (compiler->has_error) {
        return;
    }

    size_t slot;
    if (!declare_local(compiler, let->name, &slot)) {
        return;
    }

    emit2(compiler, SK_OP_STORE_LOCAL, (uint8_t)slot);
}

static void compile_assign_statement(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    struct sk_ast_assign *assign = &node->as.assign;

    compile_expression(compiler, assign->expression);
    if (compiler->has_error) {
        return;
    }

    size_t slot;
    if (!resolve_local(compiler, assign->name, &slot)) {
        compiler_error(compiler, "Unknown local variable.");
        return;
    }

    emit2(compiler, SK_OP_STORE_LOCAL, (uint8_t)slot);
}

static void compile_if_statement(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    compile_expression(compiler, node->as.ifn.condition);
    size_t then_branch_jmp = emit_jmp(compiler, SK_OP_JMP_FALSE);

    emit(compiler, SK_OP_POP);

    compile_statement(compiler, node->as.ifn.then_branch);

    size_t else_branch_jmp = emit_jmp(compiler, SK_OP_JMP);

    patch_jmp(compiler, then_branch_jmp);

    emit(compiler, SK_OP_POP);

    if (node->as.ifn.else_branch != NULL) {
        compile_statement(compiler, node->as.ifn.else_branch);
    }

    patch_jmp(compiler, else_branch_jmp);
}

static void compile_while_statement(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    size_t loop_start = compiler->current_chunk->count;

    compile_expression(compiler, node->as.whilen.condition);

    size_t exit_jmp = emit_jmp(compiler, SK_OP_JMP_FALSE);

    emit(compiler, SK_OP_POP);
    compile_statement(compiler, node->as.whilen.body);

    emit_jmp_back(compiler, loop_start);

    patch_jmp(compiler, exit_jmp);
    emit(compiler, SK_OP_POP);
}

static void compile_print_statement(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    struct sk_ast_print *print = &node->as.print;
    struct sk_ast_args *args = &print->args->as.args;
    for (size_t i = args->args.count; i > 0; i--) {
        compile_expression(compiler, args->args.nodes[i - 1]);
    }

    emit(compiler, SK_OP_PRINT);
}

static void compile_return_statement(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    struct sk_ast_return *returnn = &node->as.returnn;
    struct sk_ast_node *expression = returnn->expression;
    compile_expression_or_nothing(compiler, expression);
    emit(compiler, SK_OP_RETURN);
}

static void compile_expression_or_nothing(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    if (node == NULL) {
        emit(compiler, SK_OP_NOTHING);
        return;
    }

    compile_expression(compiler, node);
}

static void compile_expression(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    switch (node->type) {
        case SK_AST_BINARY:
            compile_binary(compiler, node);
            break;
        case SK_AST_UNARY:
            compile_unary(compiler, node);
            break;
        case SK_AST_IDENTIFIER:
            compile_identifier(compiler, node);
            break;
        case SK_AST_LITERAL:
            compile_literal(compiler, node);
            break;
        default:
            compiler_error(compiler, "Unsupported expression.");
            break;
    }
}

static void compile_binary(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    compile_expression(compiler, node->as.binary.left);

    if (node->as.binary.operator.type == SK_TOKEN_AND) {
        compile_and(compiler, node);
        return;
    }

    if (node->as.binary.operator.type == SK_TOKEN_OR) {
        compile_or(compiler, node);
        return;
    }

    compile_expression(compiler, node->as.binary.right);

    switch (node->as.binary.operator.type) {
        case SK_TOKEN_PLUS:
            emit(compiler, SK_OP_NADD);
            break;
        case SK_TOKEN_MINUS:
            emit(compiler, SK_OP_NSUB);
            break;
        case SK_TOKEN_STAR:
            emit(compiler, SK_OP_NMUL);
            break;
        case SK_TOKEN_SLASH:
            emit(compiler, SK_OP_NDIV);
            break;
        case SK_TOKEN_LESS:
            emit(compiler, SK_OP_NLESS);
            break;
        case SK_TOKEN_LESS_EQ:
            emit2(compiler, SK_OP_NGREATER, SK_OP_NOT);
            break;
        case SK_TOKEN_GREATER:
            emit(compiler, SK_OP_NGREATER);
            break;
        case SK_TOKEN_GREATER_EQ:
            emit2(compiler, SK_OP_NLESS, SK_OP_NOT);
            break;
        case SK_TOKEN_EQUAL:
            emit(compiler, SK_OP_NEQUAL);
            break;
        case SK_TOKEN_NOT_EQUAL:
            emit2(compiler, SK_OP_NEQUAL, SK_OP_NOT);
            break;
        default:
            compiler_error(compiler, "Unsupported binary operator.");
            break;
    }
}

static void compile_and(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    size_t jmp_offset = emit_jmp(compiler, SK_OP_JMP_FALSE);

    emit(compiler, SK_OP_POP);
    compile_expression(compiler, node->as.binary.right);

    patch_jmp(compiler, jmp_offset);
}

static void compile_or(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    size_t jmp_offset = emit_jmp(compiler, SK_OP_JMP_TRUE);

    emit(compiler, SK_OP_POP);
    compile_expression(compiler, node->as.binary.right);

    patch_jmp(compiler, jmp_offset);
}

static void compile_unary(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    compile_expression(compiler, node->as.unary.expression);

    switch (node->as.unary.operator.type) {
        case SK_TOKEN_PLUS:
            // Unary plus preserves the operand; no bytecode is needed.
            break;
        case SK_TOKEN_MINUS:
            emit(compiler, SK_OP_NNEG);
            break;
        case SK_TOKEN_NOT:
            emit(compiler, SK_OP_NOT);
            break;
        default:
            compiler_error(compiler, "Unsupported unary operator.");
            break;
    }
}

static void compile_identifier(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    struct sk_ast_identifier *identifier = &node->as.identifier;

    size_t slot;
    if (!resolve_local(compiler, identifier->token, &slot)) {
        compiler_error(compiler, "Unknown local variable.");
        return;
    }

    emit2(compiler, SK_OP_LOAD_LOCAL, (uint8_t)slot);
}

static void compile_literal(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    switch (node->as.literal.token.type) {
        case SK_TOKEN_TRUE:
            emit(compiler, SK_OP_TRUE);
            break;
        case SK_TOKEN_FALSE:
            emit(compiler, SK_OP_FALSE);
            break;
        case SK_TOKEN_NUMBER:
            compile_number(compiler, &node->as.literal);
            break;
        case SK_TOKEN_STRING:
            compile_string(compiler, &node->as.literal);
            break;
        default:
            compiler_error(compiler, "Unsupported literal.");
            break;
    }
}

static void compile_number(struct sk_compiler *compiler, struct sk_ast_literal *literal)
{
    sk_number number = sk_number_from_string(literal->token.start, literal->token.length);
    struct sk_value number_value = sk_number_value(number);
    emit_const(compiler, number_value);
}

static void compile_string(struct sk_compiler *compiler, struct sk_ast_literal *literal)
{
    struct sk_value string_value = sk_object_value(
        sk_object_string_from_chars(literal->token.start + 1, literal->token.length - 2));
    emit_const(compiler, string_value);
}
