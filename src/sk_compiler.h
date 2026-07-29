#ifndef SK_COMPILER_H
#define SK_COMPILER_H

#include <stdbool.h>

#include "sk_parser.h"
#include "sk_vm.h"

struct sk_compiler {
    struct sk_chunk *current_chunk;
    struct sk_program *program;
    bool has_error;
};

bool sk_compiler_compile(struct sk_compiler *compiler, struct sk_ast_node *node, struct sk_program *program);


#endif // SK_COMPILER_H
