#ifndef SK_COMPILER_H
#define SK_COMPILER_H

#include <stdbool.h>

#include "sk_vm.h"
#include "sk_parser.h"

struct sk_compiler {
    struct sk_chunk *current_chunk;
};

bool sk_compiler_compile(struct sk_compiler *compiler, struct sk_ast_node *node, struct sk_chunk *chunk);


#endif //SK_COMPILER_H
