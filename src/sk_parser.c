#include "sk_parser.h"

#include <stdio.h>

#include "sk_utils.h"

static void ast_node_print_impl(const struct sk_ast_node *node)
{
    switch (node->type) {
        case SK_AST_LITERAL:
            printf("%.*s", (int)node->as.literal.token.length, node->as.literal.token.start);
        break;
        case SK_AST_UNARY:
            printf("(");
            printf("%.*s", (int)node->as.unary.operator.length, node->as.unary.operator.start);
            ast_node_print_impl(node->as.unary.expression);
            printf(")");
            break;
        case SK_AST_BINARY:
            printf("(");
            ast_node_print_impl(node->as.binary.left);
            printf(" %.*s ", (int)node->as.binary.operator.length, node->as.binary.operator.start);
            ast_node_print_impl(node->as.binary.right);
            printf(")");
            break;
        case SK_AST_GROUPING:
            ast_node_print_impl(node->as.grouping.expression);
            break;
        default:
            break;
    }
}

void sk_ast_node_print(const struct sk_ast_node *node)
{
    ast_node_print_impl(node);
    printf("\n");
}

static struct sk_ast_node *ast_node_new(void);
static struct sk_ast_node *ast_literal_new(struct sk_token token);
static struct sk_ast_node *ast_grouping_new(struct sk_ast_node *expression);
static struct sk_ast_node *ast_unary_new(struct sk_token operator, struct sk_ast_node *expression);
static struct sk_ast_node *ast_binary_new(struct sk_token operator, struct sk_ast_node *left, struct sk_ast_node *right);

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
        }
    };

    return literal;
}

static struct sk_ast_node *ast_grouping_new(struct sk_ast_node *expression)
{
    struct sk_ast_node *grouping = ast_node_new();
    *grouping = (struct sk_ast_node) {
        .type = SK_AST_GROUPING,
        .as.grouping = (struct sk_ast_grouping) {
            .expression = expression,
        }
    };

    return grouping;
}

static struct sk_ast_node *ast_unary_new(struct sk_token operator, struct sk_ast_node *expression)
{
    struct sk_ast_node *unary = ast_node_new();
    *unary = (struct sk_ast_node) {
        .type = SK_AST_UNARY,
        .as.unary = (struct sk_ast_unary) {
            .operator = operator,
            .expression = expression,
        }
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
        }
    };

    return binary;
}

static void advance(struct sk_parser *parser);
static void consume(struct sk_parser *parser, enum sk_token_type type, const char *message);

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
    struct sk_ast_node *expression = parse_expression(parser);
    consume(parser, SK_TOKEN_EOF, "Expected end of file.\n");
    return expression;
}

static void advance(struct sk_parser *parser)
{
    parser->previous = parser->current;

    // TODO: We should handle error tokens.
    parser->current = sk_lexer_next(&parser->lexer);
}

static void consume(struct sk_parser *parser, enum sk_token_type type, const char *message)
{
    if (parser->current.type == type) {
        advance(parser);
        return;
    }

    // TODO: Delegate to error handling.
    fprintf(stderr, message);
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
    return ast_grouping_new(expression);
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
