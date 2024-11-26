#include "sk_debug.h"

#include <stdio.h>

static size_t debug_simple_instruction(const char *name);

static size_t debug_instruction(uint8_t instruction);

void sk_debug_chunk(const struct sk_chunk *chunk, const char *name)
{
    printf("Chunk: %s\n", name);

    size_t instruction_index = 0;
    while (instruction_index < chunk->count) {
        uint8_t instruction = chunk->code[instruction_index];
        printf("%.4zu | 0x%.2hhx | ", instruction_index, instruction);
        instruction_index += debug_instruction(instruction);
        printf("\n");
    }
}

static size_t debug_simple_instruction(const char *name)
{
    printf("%s", name);
    return 1;
}

static size_t debug_const_instruction()
{
    printf("CONST");
    return 2;
}

static size_t debug_instruction(uint8_t instruction)
{
    switch (instruction) {
        case SK_OP_HALT:
            return debug_simple_instruction("HALT");
        case SK_OP_DUMP:
            return debug_simple_instruction("DUMP");
        case SK_OP_CONST:
            return debug_const_instruction();
        case SK_OP_NNEG:
            return debug_simple_instruction("NNEG");
        case SK_OP_NADD:
            return debug_simple_instruction("NADD");
        case SK_OP_NSUB:
            return debug_simple_instruction("NSUB");
        case SK_OP_NMUL:
            return debug_simple_instruction("NMUL");
        case SK_OP_NDIV:
            return debug_simple_instruction("NDIV");
        default:
            return debug_simple_instruction("INVALID");
    }
}
