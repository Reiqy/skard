#include "sk_parser.h"

#include <stdio.h>
#include <stdbool.h>

#include "sk_utils.h"

static struct sk_ast_node *ast_literal_new(struct sk_parser *parser, struct sk_token token);
static struct sk_ast_node *ast_unary_new(struct sk_parser *parser, struct sk_token operator,
                                         struct sk_ast_node *expression);
static struct sk_ast_node *ast_binary_new(struct sk_parser *parser, struct sk_token operator, struct sk_ast_node *left,
                                          struct sk_ast_node *right);

static struct sk_ast_node *ast_args_new(struct sk_parser *parser);

static struct sk_ast_node *ast_block_new(struct sk_parser *parser);
static struct sk_ast_node *ast_if_new(
    struct sk_parser *parser,
    struct sk_ast_node *condition,
    struct sk_ast_node *then_branch,
    struct sk_ast_node *else_branch);
static struct sk_ast_node *ast_print_new(struct sk_parser *parser, struct sk_ast_node *args);

static struct sk_ast_node *ast_fn_new(struct sk_parser *parser, struct sk_token name, struct sk_ast_node *body);

static struct sk_ast_node *ast_program_new(struct sk_parser *parser);

static struct sk_ast_node *ast_literal_new(struct sk_parser *parser, struct sk_token token)
{
    struct sk_ast_node *literal = sk_ast_node_arena_alloc(&parser->arena);
    *literal = (struct sk_ast_node){
        .type = SK_AST_LITERAL,
        .as.literal = (struct sk_ast_literal){
            .token = token,
        },
    };

    return literal;
}

static struct sk_ast_node *ast_unary_new(struct sk_parser *parser, struct sk_token operator,
                                         struct sk_ast_node *expression)
{
    struct sk_ast_node *unary = sk_ast_node_arena_alloc(&parser->arena);
    *unary = (struct sk_ast_node){
        .type = SK_AST_UNARY,
        .as.unary = (struct sk_ast_unary){
            .operator = operator,
            .expression = expression,
        },
    };

    return unary;
}

static struct sk_ast_node *ast_binary_new(struct sk_parser *parser, struct sk_token operator, struct sk_ast_node *left,
                                          struct sk_ast_node *right)
{
    struct sk_ast_node *binary = sk_ast_node_arena_alloc(&parser->arena);
    *binary = (struct sk_ast_node){
        .type = SK_AST_BINARY,
        .as.binary = (struct sk_ast_binary){
            .operator = operator,
            .left = left,
            .right = right,
        },
    };

    return binary;
}

static struct sk_ast_node *ast_args_new(struct sk_parser *parser)
{
    struct sk_ast_args inner = {0};
    struct sk_ast_node *block = sk_ast_node_arena_alloc(&parser->arena);
    *block = (struct sk_ast_node){
        .type = SK_AST_ARGS,
        .as.args = inner,
    };

    sk_ast_node_array_init(&block->as.args.args);

    return block;
}

static struct sk_ast_node *ast_block_new(struct sk_parser *parser)
{
    struct sk_ast_block inner = {0};
    struct sk_ast_node *block = sk_ast_node_arena_alloc(&parser->arena);
    *block = (struct sk_ast_node){
        .type = SK_AST_BLOCK,
        .as.block = inner,
    };

    sk_ast_node_array_init(&block->as.block.contents);

    return block;
}

static struct sk_ast_node *ast_if_new(
    struct sk_parser *parser,
    struct sk_ast_node *condition,
    struct sk_ast_node *then_branch,
    struct sk_ast_node *else_branch)
{
    struct sk_ast_node *ifn = sk_ast_node_arena_alloc(&parser->arena);
    *ifn = (struct sk_ast_node){
        .type = SK_AST_IF,
        .as.ifn = (struct sk_ast_if){
            .condition = condition,
            .then_branch = then_branch,
            .else_branch = else_branch,
        }
    };

    return ifn;
}

static struct sk_ast_node *ast_print_new(struct sk_parser *parser, struct sk_ast_node *args)
{
    struct sk_ast_node *print = sk_ast_node_arena_alloc(&parser->arena);
    *print = (struct sk_ast_node){
        .type = SK_AST_PRINT,
        .as.print = (struct sk_ast_print){
            .args = args,
        },
    };

    return print;
}

static struct sk_ast_node *ast_fn_new(struct sk_parser *parser, struct sk_token name, struct sk_ast_node *body)
{
    struct sk_ast_node *fn = sk_ast_node_arena_alloc(&parser->arena);
    *fn = (struct sk_ast_node){
        .type = SK_AST_FN,
        .as.fn = (struct sk_ast_fn){
            .name = name,
            .body = body,
        },
    };

    return fn;
}

static struct sk_ast_node *ast_program_new(struct sk_parser *parser)
{
    struct sk_ast_program inner = {0};
    struct sk_ast_node *program = sk_ast_node_arena_alloc(&parser->arena);
    *program = (struct sk_ast_node){
        .type = SK_AST_PROGRAM,
        .as.program = inner,
    };

    sk_ast_node_array_init(&program->as.program.declarations);

    return program;
}

static void error(struct sk_parser *parser, const struct sk_token *token, const char *message);
static void synchronize(struct sk_parser *parser);

static bool check(const struct sk_parser *parser, enum sk_token_type type);
static bool match(struct sk_parser *parser, enum sk_token_type type);
static void advance(struct sk_parser *parser);
static void consume(struct sk_parser *parser, enum sk_token_type type, const char *message);

static struct sk_ast_node *parse_declaration(struct sk_parser *parser, bool is_statement_allowed);
static struct sk_ast_node *parse_fn_declaration(struct sk_parser *parser);

static struct sk_ast_node *parse_args(struct sk_parser *parser);

static struct sk_ast_node *parse_statement(struct sk_parser *parser);
static struct sk_ast_node *parse_block(struct sk_parser *parser);
static struct sk_ast_node *parse_if_statement(struct sk_parser *parser);
static struct sk_ast_node *parse_print_statement(struct sk_parser *parser);

enum precedence {
    PREC_NONE,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_ADDITIVE,
    PREC_MULTIPLICATIVE,
    PREC_UNARY,
};

static enum precedence get_precedence(enum sk_token_type token_type);

static struct sk_ast_node *parse_expression(struct sk_parser *parser);
static struct sk_ast_node *parse_pratt(struct sk_parser *parser, enum precedence precedence);
static struct sk_ast_node *parse_prefix(struct sk_parser *parser);
static struct sk_ast_node *parse_infix(struct sk_parser *parser, struct sk_ast_node *left);
static struct sk_ast_node *parse_grouping(struct sk_parser *parser);
static struct sk_ast_node *parse_binary(struct sk_parser *parser, struct sk_ast_node *left);
static struct sk_ast_node *parse_unary(struct sk_parser *parser);
static struct sk_ast_node *parse_literal(struct sk_parser *parser);

void sk_parser_init(struct sk_parser *parser, const char *source)
{
    sk_ast_node_arena_init(&parser->arena, 256);
    sk_lexer_init(&parser->lexer, source);
    parser->is_panic = false;
    parser->has_error = false;
}

void sk_parser_free(struct sk_parser *parser)
{
    sk_ast_node_arena_free(&parser->arena);
}

struct sk_ast_node *sk_parser_parse(struct sk_parser *parser)
{
    advance(parser);

    struct sk_ast_node *program = ast_program_new(parser);
    while (!match(parser, SK_TOKEN_EOF)) {
        const bool is_statement_allowed = false;
        struct sk_ast_node *declaration = parse_declaration(parser, is_statement_allowed);
        if (declaration != NULL) {
            sk_ast_node_array_add(&program->as.program.declarations, declaration);
        }

        synchronize(parser);
    }

    return program;
}

static void error(struct sk_parser *parser, const struct sk_token *token, const char *message)
{
    if (parser->is_panic) {
        return;
    }

    parser->is_panic = true;
    parser->has_error = true;

    fprintf(stderr, "[line %zu] Error: %s\n", token->line, message);
}

static void synchronize(struct sk_parser *parser)
{
    if (!parser->is_panic) {
        return;
    }

    parser->is_panic = false;
    while (parser->current.type != SK_TOKEN_EOF) {
        switch (parser->current.type) {
            case SK_TOKEN_FN:
                return;

            default:
                break;
        }

        advance(parser);
    }
}

static bool check(const struct sk_parser *parser, enum sk_token_type type)
{
    return parser->current.type == type;
}

static bool match(struct sk_parser *parser, enum sk_token_type type)
{
    if (!check(parser, type)) {
        return false;
    }

    advance(parser);
    return true;
}

static void advance(struct sk_parser *parser)
{
    parser->previous = parser->current;

    for (;;) {
        parser->current = sk_lexer_next(&parser->lexer);
        if (parser->current.type != SK_TOKEN_ERR) {
            break;
        }

        error(parser, &parser->current, parser->current.start);
    }
}

static void consume(struct sk_parser *parser, enum sk_token_type type, const char *message)
{
    if (check(parser, type)) {
        advance(parser);
        return;
    }

    error(parser, &parser->current, message);
}

static struct sk_ast_node *parse_declaration(struct sk_parser *parser, bool is_statement_allowed)
{
    if (match(parser, SK_TOKEN_FN)) {
        return parse_fn_declaration(parser);
    }

    if (is_statement_allowed) {
        return parse_statement(parser);
    }

    error(parser, &parser->current, "Expected declaration.");
    return NULL;
}

static struct sk_ast_node *parse_fn_declaration(struct sk_parser *parser)
{
    consume(parser, SK_TOKEN_IDENTIFIER, "Expected identifier.");
    struct sk_token name = parser->previous;
    consume(parser, SK_TOKEN_LPAREN, "Expected '('.");
    consume(parser, SK_TOKEN_RPAREN, "Expected ')'.");
    struct sk_ast_node *body = parse_block(parser);
    return ast_fn_new(parser, name, body);
}

static struct sk_ast_node *parse_args(struct sk_parser *parser)
{
    consume(parser, SK_TOKEN_LPAREN, "Expected '('.");
    struct sk_ast_node *args = ast_args_new(parser);
    while (!check(parser, SK_TOKEN_RBRACE) && !check(parser, SK_TOKEN_EOF)) {
        struct sk_ast_node *arg = parse_expression(parser);
        if (args == NULL) {
            return NULL;
        }

        sk_ast_node_array_add(&args->as.args.args, arg);

        if (!match(parser, SK_TOKEN_COMMA)) {
            break;
        }
    }

    consume(parser, SK_TOKEN_RPAREN, "Expected ')'.");
    return args;
}

static struct sk_ast_node *parse_statement(struct sk_parser *parser)
{
    if (check(parser, SK_TOKEN_IF)) {
        return parse_if_statement(parser);
    }

    if (check(parser, SK_TOKEN_PRINT)) {
        return parse_print_statement(parser);
    }

    if (check(parser, SK_TOKEN_LBRACE)) {
        return parse_block(parser);
    }

    error(parser, &parser->current, "Expected statement.");
    return NULL;
}

static struct sk_ast_node *parse_block(struct sk_parser *parser)
{
    consume(parser, SK_TOKEN_LBRACE, "Expected '{'.");

    struct sk_ast_node *block = ast_block_new(parser);
    while (!check(parser, SK_TOKEN_RBRACE) && !check(parser, SK_TOKEN_EOF)) {
        const bool is_statement_allowed = true;
        struct sk_ast_node *statement = parse_declaration(parser, is_statement_allowed);
        if (statement == NULL) {
            return NULL;
        }

        sk_ast_node_array_add(&block->as.block.contents, statement);
    }

    consume(parser, SK_TOKEN_RBRACE, "Expected '}'.");
    return block;
}

static struct sk_ast_node *parse_if_statement(struct sk_parser *parser)
{
    consume(parser, SK_TOKEN_IF, "Expected 'if'.");
    consume(parser, SK_TOKEN_LPAREN, "Expected '('.");
    struct sk_ast_node *condition = parse_expression(parser);
    consume(parser, SK_TOKEN_RPAREN, "Expected ')'.");
    struct sk_ast_node *then_branch = parse_statement(parser);

    if (!match(parser, SK_TOKEN_ELSE)) {
        return ast_if_new(parser, condition, then_branch, NULL);
    }

    struct sk_ast_node *else_branch = parse_statement(parser);
    return ast_if_new(parser, condition, then_branch, else_branch);
}

static struct sk_ast_node *parse_print_statement(struct sk_parser *parser)
{
    consume(parser, SK_TOKEN_PRINT, "Expected 'print'.");
    struct sk_ast_node *args = parse_args(parser);
    return ast_print_new(parser, args);
}

static enum precedence get_precedence(enum sk_token_type token_type)
{
    switch (token_type) {
        case SK_TOKEN_OR:
            return PREC_OR;
        case SK_TOKEN_AND:
            return PREC_AND;
        case SK_TOKEN_EQUAL:
        case SK_TOKEN_NOT_EQUAL:
            return PREC_EQUALITY;
        case SK_TOKEN_LESS:
        case SK_TOKEN_LESS_EQ:
        case SK_TOKEN_GREATER:
        case SK_TOKEN_GREATER_EQ:
            return PREC_COMPARISON;
        case SK_TOKEN_PLUS:
        case SK_TOKEN_MINUS:
            return PREC_ADDITIVE;
        case SK_TOKEN_STAR:
        case SK_TOKEN_SLASH:
            return PREC_MULTIPLICATIVE;
        default:
            return PREC_NONE;
    }
}

static struct sk_ast_node *parse_expression(struct sk_parser *parser)
{
    return parse_pratt(parser, PREC_OR);
}

static struct sk_ast_node *parse_pratt(struct sk_parser *parser, enum precedence precedence)
{
    struct sk_ast_node *left = parse_prefix(parser);

    while (precedence <= get_precedence(parser->current.type)) {
        left = parse_infix(parser, left);
    }

    return left;
}

static struct sk_ast_node *parse_prefix(struct sk_parser *parser)
{
    advance(parser);
    switch (parser->previous.type) {
        case SK_TOKEN_LPAREN:
            return parse_grouping(parser);
        case SK_TOKEN_PLUS:
        case SK_TOKEN_MINUS:
        case SK_TOKEN_NOT:
            return parse_unary(parser);
        case SK_TOKEN_NUMBER:
        case SK_TOKEN_STRING:
        case SK_TOKEN_TRUE:
        case SK_TOKEN_FALSE:
            return parse_literal(parser);
        default:
            error(parser, &parser->previous, "Expected prefix expression.");
            return NULL;
    }
}

static struct sk_ast_node *parse_infix(struct sk_parser *parser, struct sk_ast_node *left)
{
    advance(parser);
    switch (parser->previous.type) {
        case SK_TOKEN_PLUS:
        case SK_TOKEN_MINUS:
        case SK_TOKEN_STAR:
        case SK_TOKEN_SLASH:
        case SK_TOKEN_LESS:
        case SK_TOKEN_LESS_EQ:
        case SK_TOKEN_GREATER:
        case SK_TOKEN_GREATER_EQ:
        case SK_TOKEN_OR:
        case SK_TOKEN_AND:
            return parse_binary(parser, left);
        default:
            error(parser, &parser->previous, "Expected binary expression.");
            return NULL;
    }
}

static struct sk_ast_node *parse_grouping(struct sk_parser *parser)
{
    struct sk_ast_node *expression = parse_expression(parser);
    consume(parser, SK_TOKEN_RPAREN, "Expected ')' after expression.");
    return expression;
}

static struct sk_ast_node *parse_binary(struct sk_parser *parser, struct sk_ast_node *left)
{
    struct sk_token operator = parser->previous;
    enum precedence precedence = get_precedence(operator.type);
    struct sk_ast_node *right = parse_pratt(parser, precedence + 1);
    return ast_binary_new(parser, operator, left, right);
}

static struct sk_ast_node *parse_unary(struct sk_parser *parser)
{
    struct sk_token operator = parser->previous;
    struct sk_ast_node *expression = parse_pratt(parser, PREC_UNARY);
    return ast_unary_new(parser, operator, expression);
}

static struct sk_ast_node *parse_literal(struct sk_parser *parser)
{
    return ast_literal_new(parser, parser->previous);
}
