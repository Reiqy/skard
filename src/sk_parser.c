#include "sk_parser.h"

#include <stdio.h>
#include <stdbool.h>

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

static void print_indent(int depth)
{
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
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

static void print_expression(const struct sk_ast_node *node, int depth)
{
    print_indent(depth);
    print_parenthesized_expression(node);
    printf("\n");
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

void sk_ast_node_print(const struct sk_ast_node *node)
{
    ast_node_print_impl(node, 0);
}

static struct sk_ast_node *ast_node_new(void);

static struct sk_ast_node *ast_literal_new(struct sk_token token);
static struct sk_ast_node *ast_unary_new(struct sk_token operator, struct sk_ast_node *expression);
static struct sk_ast_node *ast_binary_new(struct sk_token operator, struct sk_ast_node *left, struct sk_ast_node *right);

static struct sk_ast_node *ast_block_new(void);
static struct sk_ast_node *ast_print_new(struct sk_ast_node *expression);

static struct sk_ast_node *ast_fn_new(struct sk_token name, struct sk_ast_node *body);

static struct sk_ast_node *ast_program_new(void);

static struct sk_ast_node *ast_node_new(void)
{
    struct sk_ast_node *node = sk_alloc(struct sk_ast_node);
    return node;
}

static struct sk_ast_node *ast_literal_new(struct sk_token token)
{
    struct sk_ast_node *literal = ast_node_new();
    *literal = (struct sk_ast_node) {
        .type = SK_AST_LITERAL,
        .as.literal = (struct sk_ast_literal) {
            .token = token,
        },
    };

    return literal;
}

static struct sk_ast_node *ast_unary_new(struct sk_token operator, struct sk_ast_node *expression)
{
    struct sk_ast_node *unary = ast_node_new();
    *unary = (struct sk_ast_node) {
        .type = SK_AST_UNARY,
        .as.unary = (struct sk_ast_unary) {
            .operator = operator,
            .expression = expression,
        },
    };

    return unary;
}

static struct sk_ast_node *ast_binary_new(struct sk_token operator, struct sk_ast_node *left, struct sk_ast_node *right)
{
    struct sk_ast_node *binary = ast_node_new();
    *binary = (struct sk_ast_node) {
        .type = SK_AST_BINARY,
        .as.binary = (struct sk_ast_binary) {
            .operator = operator,
            .left = left,
            .right = right,
        },
    };

    return binary;
}

static struct sk_ast_node *ast_block_new(void)
{
    struct sk_ast_block inner = {0};
    struct sk_ast_node *block = ast_node_new();
    *block = (struct sk_ast_node) {
        .type = SK_AST_BLOCK,
        .as.block = inner,
    };

    sk_ast_node_array_init(&block->as.block.contents);

    return block;
}

static struct sk_ast_node *ast_print_new(struct sk_ast_node *expression)
{
    struct sk_ast_node *print = ast_node_new();
    *print = (struct sk_ast_node) {
        .type = SK_AST_PRINT,
        .as.print = (struct sk_ast_print) {
            .expression = expression,
        },
    };

    return print;
}

static struct sk_ast_node *ast_fn_new(struct sk_token name, struct sk_ast_node *body)
{
    struct sk_ast_node *fn = ast_node_new();
    *fn = (struct sk_ast_node) {
        .type = SK_AST_FN,
        .as.fn = (struct sk_ast_fn) {
            .name = name,
            .body = body,
        },
    };

    return fn;
}

static struct sk_ast_node *ast_program_new(void)
{
    struct sk_ast_program inner = {0};
    struct sk_ast_node *program = ast_node_new();
    *program = (struct sk_ast_node) {
        .type = SK_AST_PROGRAM,
        .as.program = inner,
    };

    sk_ast_node_array_init(&program->as.program.declarations);

    return program;
}

static bool check(const struct sk_parser *parser, enum sk_token_type type);
static bool match(struct sk_parser *parser, enum sk_token_type type);
static void advance(struct sk_parser *parser);
static void consume(struct sk_parser *parser, enum sk_token_type type, const char *message);

static struct sk_ast_node *parse_declaration(struct sk_parser *parser);
static struct sk_ast_node *parse_fn_declaration(struct sk_parser *parser);

static struct sk_ast_node *parse_statement(struct sk_parser *parser);
static struct sk_ast_node *parse_block(struct sk_parser *parser);
static struct sk_ast_node *parse_print_statement(struct sk_parser *parser);

enum precedence {
    PREC_NONE = 0,
    PREC_ADDITIVE = 1,
    PREC_MULTIPLICATIVE = 2,
    PREC_UNARY = 3,
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
    sk_lexer_init(&parser->lexer, source);
}

struct sk_ast_node *sk_parser_parse(struct sk_parser *parser)
{
    advance(parser);

    struct sk_ast_node *program = ast_program_new();
    while (!match(parser, SK_TOKEN_EOF)) {
        struct sk_ast_node *declaration = parse_declaration(parser);
        sk_ast_node_array_add(&program->as.program.declarations, declaration);
    }

    return program;
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

    // TODO: We should handle error tokens.
    parser->current = sk_lexer_next(&parser->lexer);
}

static void consume(struct sk_parser *parser, enum sk_token_type type, const char *message)
{
    if (check(parser, type)) {
        advance(parser);
        return;
    }

    // TODO: Delegate to error handling.
    fprintf(stderr, message);
}

static struct sk_ast_node *parse_declaration(struct sk_parser *parser)
{
    if (match(parser, SK_TOKEN_FN)) {
        return parse_fn_declaration(parser);
    }

    // TODO: Handle in a better way.
    fprintf(stderr, "Expected declaration.\n");
    exit(EXIT_FAILURE);
}

static struct sk_ast_node *parse_fn_declaration(struct sk_parser *parser)
{
    consume(parser, SK_TOKEN_IDENTIFIER, "Expected identifier.\n");
    struct sk_token name = parser->previous;
    consume(parser, SK_TOKEN_LPAREN, "Expected '('.\n");
    consume(parser, SK_TOKEN_RPAREN, "Expected ')'.\n");
    struct sk_ast_node *body = parse_block(parser);
    return ast_fn_new(name, body);
}

static struct sk_ast_node *parse_statement(struct sk_parser *parser)
{
    if (match(parser, SK_TOKEN_PRINT)) {
        return parse_print_statement(parser);
    }

    // TODO: Handle in a better way.
    fprintf(stderr, "Expected statement.\n");
    exit(EXIT_FAILURE);
}

static struct sk_ast_node *parse_block(struct sk_parser *parser)
{
    consume(parser, SK_TOKEN_LBRACE, "Expected '{'.\n");

    struct sk_ast_node *block = ast_block_new();
    while (!match(parser, SK_TOKEN_RBRACE)) {
        // TODO: This is not finished. We want to allow declarations inside blocks as well.
        struct sk_ast_node *statement = parse_statement(parser);
        sk_ast_node_array_add(&block->as.block.contents, statement);
    }

    return block;
}

static struct sk_ast_node *parse_print_statement(struct sk_parser *parser)
{
    struct sk_ast_node *expression = parse_expression(parser);
    return ast_print_new(expression);
}

static enum precedence get_precedence(enum sk_token_type token_type)
{
    switch (token_type) {
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
    return parse_pratt(parser, PREC_ADDITIVE);
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
            return parse_unary(parser);
        case SK_TOKEN_NUMBER:
            return parse_literal(parser);
        default:
            exit(EXIT_FAILURE);
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
            return parse_binary(parser, left);
        default:
            exit(EXIT_FAILURE);
    }
}

static struct sk_ast_node *parse_grouping(struct sk_parser *parser)
{
    struct sk_ast_node *expression = parse_expression(parser);
    consume(parser, SK_TOKEN_RPAREN, "Expected ')' after expression.\n");
    return expression;
}

static struct sk_ast_node *parse_binary(struct sk_parser *parser, struct sk_ast_node *left)
{
    struct sk_token operator = parser->previous;
    enum precedence precedence = get_precedence(operator.type);
    struct sk_ast_node *right = parse_pratt(parser, precedence + 1);
    return ast_binary_new(operator, left, right);
}

static struct sk_ast_node *parse_unary(struct sk_parser *parser)
{
    struct sk_token operator = parser->previous;
    struct sk_ast_node *expression = parse_pratt(parser, PREC_UNARY);
    return ast_unary_new(operator, expression);
}

static struct sk_ast_node *parse_literal(struct sk_parser *parser)
{
    return ast_literal_new(parser->previous);
}
