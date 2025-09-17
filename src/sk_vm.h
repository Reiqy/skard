#ifndef SKARD_SK_VM_H
#define SKARD_SK_VM_H

#include <stdint.h>

#include "sk_value.h"

enum sk_opcode {
    SK_OP_HALT,
    SK_OP_PRINT,

    SK_OP_POP,

    SK_OP_CONST,

    SK_OP_NNEG,
    SK_OP_NADD,
    SK_OP_NSUB,
    SK_OP_NMUL,
    SK_OP_NDIV,

    SK_OP_NLESS,
    SK_OP_NGREATER,
    SK_OP_NEQUAL,

    SK_OP_TRUE,
    SK_OP_FALSE,
    SK_OP_NOT,

    SK_OP_JMP,
    SK_OP_JMP_TRUE,
    SK_OP_JMP_FALSE,
};

struct sk_chunk {
    struct sk_value_array constants;
    uint8_t *code;
    size_t capacity;
    size_t count;
};

void sk_chunk_init(struct sk_chunk *chunk);
void sk_chunk_free(struct sk_chunk *chunk);
void sk_chunk_add(struct sk_chunk *chunk, uint8_t byte);
void sk_chunk_add_const(struct sk_chunk *chunk, struct sk_value constant);

#define SK_VM_STACK_MAX_SIZE 256

struct sk_vm_stack {
    struct sk_value stack[SK_VM_STACK_MAX_SIZE];
    struct sk_value *top;
};

void sk_vm_stack_init(struct sk_vm_stack *stack);
void sk_vm_stack_free(struct sk_vm_stack *stack);
void sk_vm_stack_push(struct sk_vm_stack *stack, struct sk_value value);
struct sk_value sk_vm_stack_pop(struct sk_vm_stack *stack);
struct sk_value sk_vm_stack_peek(const struct sk_vm_stack *stack, int depth);

struct sk_vm {
    struct sk_vm_stack stack;
    struct sk_chunk *chunk;
    uint8_t *ip;
};

void sk_vm_init(struct sk_vm *vm);
void sk_vm_free(struct sk_vm *vm);

enum sk_vm_result {
    SK_VM_OK,
    SK_VM_ERR,
};

enum sk_vm_result sk_vm_run(struct sk_vm *vm, struct sk_chunk *chunk);

#endif //SKARD_SK_VM_H
