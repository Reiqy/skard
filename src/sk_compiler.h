#ifndef SK_COMPILER_H
#define SK_COMPILER_H

#include <stdbool.h>

#include "sk_hashmap.h"
#include "sk_parser.h"
#include "sk_vm.h"

#define SK_COMPILER_MAX_LOCALS 256

struct sk_compiler_local {
    struct sk_token name;
    size_t slot;
};

struct sk_compiler {
    struct sk_chunk *current_chunk;
    bool has_error;

    struct sk_compiler_local locals[SK_COMPILER_MAX_LOCALS];
    struct sk_hashmap locals_map;
    size_t locals_count;
};

bool sk_compiler_compile(struct sk_compiler *compiler, struct sk_ast_node *node, struct sk_chunk *chunk);


#endif // SK_COMPILER_H
