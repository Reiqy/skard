#include "sk_compiler.h"

#include <stdio.h>

#include "sk_parser.h"

static void emit(struct sk_compiler *compiler, uint8_t byte);
// static void emit2(struct sk_compiler *compiler, uint8_t byte1, uint8_t byte2);
static void emit_halt(struct sk_compiler *compiler);
static void emit_const(struct sk_compiler *compiler, struct sk_value constant);

static void compile_statement(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_expression(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_binary(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_unary(struct sk_compiler *compiler, struct sk_ast_node *node);
static void compile_literal(struct sk_compiler *compiler, struct sk_ast_node *node);

static void compile_number(struct sk_compiler *compiler, struct sk_ast_literal *literal);

bool sk_compiler_compile(struct sk_compiler *compiler, struct sk_ast_node *node, struct sk_chunk *chunk)
{
    compiler->current_chunk = chunk;

    compile_statement(compiler, node);
    emit_halt(compiler);
    return true;
}

static void emit(struct sk_compiler *compiler, uint8_t byte)
{
    sk_chunk_add(compiler->current_chunk, byte);
}

static void emit_halt(struct sk_compiler *compiler)
{
    emit(compiler, SK_OP_HALT);
}

static void emit_const(struct sk_compiler *compiler, struct sk_value constant)
{
    sk_chunk_add_const(compiler->current_chunk, constant);
}

static void compile_statement(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    if (node->type == SK_AST_PRINT) {
        compile_expression(compiler, node->as.print.expression);
        emit(compiler, SK_OP_DUMP);
    }
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
        case SK_AST_LITERAL:
            compile_literal(compiler, node);
            break;
        default:
            fprintf(stderr, "Unexpected\n");
    }
}

static void compile_binary(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    compile_expression(compiler, node->as.binary.left);
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
        default:
            fprintf(stderr, "Unexpected\n");
    }
}

static void compile_unary(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    compile_expression(compiler, node->as.unary.expression);

    switch (node->as.unary.operator.type) {
        case SK_TOKEN_PLUS:
            emit(compiler, SK_OP_NADD);
            break;
        case SK_TOKEN_MINUS:
            emit(compiler, SK_OP_NSUB);
            break;
        default:
            fprintf(stderr, "Unexpected\n");
    }
}

static void compile_literal(struct sk_compiler *compiler, struct sk_ast_node *node)
{
    compile_number(compiler, &node->as.literal);
}

static void compile_number(struct sk_compiler *compiler, struct sk_ast_literal *literal)
{
    sk_number number = sk_number_from_string(literal->token.start, literal->token.length);
    struct sk_value number_value = sk_number_value(number);
    emit_const(compiler, number_value);
}
