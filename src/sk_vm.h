#ifndef SKARD_SK_VM_H
#define SKARD_SK_VM_H

#include <stdint.h>

enum sk_opcode {
    SK_OP_HALT,
    SK_OP_RETURN,
    SK_OP_CONST,
};

struct sk_chunk {

    uint8_t *code;
    size_t capacity;
    size_t count;
};

void sk_chunk_init(struct sk_chunk *chunk);
void sk_chunk_free(struct sk_chunk *chunk);
void sk_chunk_add(struct sk_chunk *chunk, uint8_t byte);

struct sk_vm {
    struct sk_chunk *chunk;
    uint8_t *ip;
};

void sk_vm_init(struct sk_vm *vm);
void sk_vm_free(struct sk_vm *vm);

#endif //SKARD_SK_VM_H
