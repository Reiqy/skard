#include "sk_parser.h"

#include <stdio.h>
#include <stdlib.h>

sk_number sk_ast_evaluate(struct sk_ast_node *node)
{
    switch (node->type) {
        case SK_AST_LITERAL:
            return sk_number_from_string(node->as.literal.token.start, node->as.literal.token.length);
        case SK_AST_UNARY:
            return -sk_ast_evaluate(node->as.unary.expression);
        case SK_AST_BINARY:
            switch (node->as.binary.operator.type) {
                case SK_TOKEN_PLUS:
                    return sk_ast_evaluate(node->as.binary.left) + sk_ast_evaluate(node->as.binary.right);
                case SK_TOKEN_MINUS:
                    return sk_ast_evaluate(node->as.binary.left) - sk_ast_evaluate(node->as.binary.right);
                case SK_TOKEN_STAR:
                    return sk_ast_evaluate(node->as.binary.left) * sk_ast_evaluate(node->as.binary.right);
                case SK_TOKEN_SLASH:
                    return sk_ast_evaluate(node->as.binary.left) / sk_ast_evaluate(node->as.binary.right);
                default:
                    fprintf(stderr, "Unexpected operator type.\n");
                    exit(EXIT_FAILURE);
            }
        default:
            fprintf(stderr, "Unexpected node type.\n");
            exit(EXIT_FAILURE);
    }
}
