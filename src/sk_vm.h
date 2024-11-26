#ifndef SKARD_SK_VM_H
#define SKARD_SK_VM_H

#include <stdint.h>

#include "sk_value.h"

enum sk_opcode {
    SK_OP_HALT,
    SK_OP_DUMP,
    SK_OP_CONST,
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
