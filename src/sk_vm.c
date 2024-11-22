#include "sk_vm.h"

#include "sk_utils.h"

void sk_chunk_init(struct sk_chunk *chunk)
{
    chunk->code = NULL;
    chunk->capacity = 0;
    chunk->count = 0;
}

void sk_chunk_free(struct sk_chunk *chunk)
{
    sk_free(chunk->code);
    sk_chunk_init(chunk);
}

void sk_chunk_add(struct sk_chunk *chunk, uint8_t byte)
{
    if (chunk->count >= chunk->capacity) {
        chunk->capacity = sk_grow(chunk->capacity);
        chunk->code = sk_realloc(chunk->code, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;
}

void sk_vm_init(struct sk_vm *vm)
{
    (void)vm;
}

void sk_vm_free(struct sk_vm *vm)
{
    (void)vm;
}
