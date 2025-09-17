#include "sk_vm.h"

#include <stdio.h>

#include "sk_utils.h"

void sk_chunk_init(struct sk_chunk *chunk)
{
    sk_value_array_init(&chunk->constants);

    chunk->code = NULL;
    chunk->capacity = 0;
    chunk->count = 0;
}

void sk_chunk_free(struct sk_chunk *chunk)
{
    sk_value_array_free(&chunk->constants);

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

void sk_chunk_add_const(struct sk_chunk *chunk, struct sk_value constant)
{
    sk_value_array_add(&chunk->constants, constant);
    size_t index = chunk->constants.count - 1;

    sk_chunk_add(chunk, SK_OP_CONST);
    // FIXME: This can currently fail with index greater than a UINT8_MAX.
    sk_chunk_add(chunk, index);
}

void sk_vm_stack_init(struct sk_vm_stack *stack)
{
    stack->top = stack->stack;
}

void sk_vm_stack_free(struct sk_vm_stack *stack)
{
    (void)stack;
}

void sk_vm_stack_push(struct sk_vm_stack *stack, struct sk_value value)
{
    *stack->top = value;
    stack->top++;
}

struct sk_value sk_vm_stack_pop(struct sk_vm_stack *stack)
{
    stack->top--;
    return *stack->top;
}

struct sk_value sk_vm_stack_peek(const struct sk_vm_stack *stack, int depth)
{
    return stack->top[-depth - 1];
}

void sk_vm_init(struct sk_vm *vm)
{
    sk_vm_stack_init(&vm->stack);
}

void sk_vm_free(struct sk_vm *vm)
{
    sk_vm_stack_free(&vm->stack);
}

static enum sk_vm_result vm_loop(struct sk_vm *vm);

enum sk_vm_result sk_vm_run(struct sk_vm *vm, struct sk_chunk *chunk)
{
    vm->chunk = chunk;
    vm->ip = chunk->code;

    return vm_loop(vm);
}

static enum sk_vm_result vm_loop(struct sk_vm *vm)
{
#define read_byte() *vm->ip++
#define read_const() sk_value_array_get(&vm->chunk->constants, read_byte())
#define read_short() (vm->ip += 2, (uint16_t)(vm->ip[-2] << 8 | vm->ip[-1]))

#define push(value) sk_vm_stack_push(&vm->stack, value)
#define pop() sk_vm_stack_pop(&vm->stack)
#define peek(depth) sk_vm_stack_peek(&vm->stack, (depth))

    for (;;) {
        switch (read_byte()) {
            case SK_OP_HALT:
                return SK_VM_OK;
            case SK_OP_PRINT: {
                // TODO: Eventually remove this temporary code.
                const struct sk_object_string *template = sk_as_string(pop());
                for (size_t i = 0; i < template->length; i++) {
                    char c = template->chars[i];
                    if (c == '%') {
                        // The following line is safe because the char on length + 1 is '\0'.
                        char next_c = template->chars[++i];
                        switch (next_c) {
                            case 'n':
                                sk_number_print(pop());
                                break;
                            case 'b':
                                sk_boolean_print(pop());
                                break;
                            case 's':
                                sk_string_print(pop());
                                break;
                            default:
                                printf("INVALID");
                                break;
                        }

                        continue;
                    }

                    printf("%c", c);
                }

                printf("\n");
                break;
            }

            case SK_OP_POP:
                pop();
                break;

            case SK_OP_CONST:
                push(read_const());
                break;

            case SK_OP_NNEG: {
                sk_number a = sk_as_number(pop());
                push(sk_number_value(-a));
                break;
            }
            case SK_OP_NADD: {
                sk_number b = sk_as_number(pop());
                sk_number a = sk_as_number(pop());
                push(sk_number_value(a + b));
                break;
            }
            case SK_OP_NSUB: {
                sk_number b = sk_as_number(pop());
                sk_number a = sk_as_number(pop());
                push(sk_number_value(a - b));
                break;
            }
            case SK_OP_NMUL: {
                sk_number b = sk_as_number(pop());
                sk_number a = sk_as_number(pop());
                push(sk_number_value(a * b));
                break;
            }
            case SK_OP_NDIV: {
                sk_number b = sk_as_number(pop());
                sk_number a = sk_as_number(pop());
                push(sk_number_value(a / b));
                break;
            }

            case SK_OP_NLESS: {
                sk_number b = sk_as_number(pop());
                sk_number a = sk_as_number(pop());
                push(sk_boolean_value(a < b));
                break;
            }
            case SK_OP_NGREATER: {
                sk_number b = sk_as_number(pop());
                sk_number a = sk_as_number(pop());
                push(sk_boolean_value(a > b));
                break;
            }
            case SK_OP_NEQUAL: {
                sk_number b = sk_as_number(pop());
                sk_number a = sk_as_number(pop());
                push(sk_boolean_value(a == b));
                break;
            }

            case SK_OP_TRUE: {
                push(sk_boolean_true);
                break;
            }
            case SK_OP_FALSE: {
                push(sk_boolean_false);
                break;
            }
            case SK_OP_NOT: {
                sk_bool a = sk_as_boolean(pop());
                push(sk_boolean_value(!a));
                break;
            }

            case SK_OP_JMP: {
                uint16_t offset = read_short();
                vm->ip += offset;
                break;
            }
            case SK_OP_JMP_TRUE: {
                uint16_t offset = read_short();
                sk_bool a = sk_as_boolean(peek(0));
                if (a) {
                    vm->ip += offset;
                }

                break;
            }
            case SK_OP_JMP_FALSE: {
                uint16_t offset = read_short();
                sk_bool a = sk_as_boolean(peek(0));
                if (!a) {
                    vm->ip += offset;
                }

                break;
            }
            default:
                fprintf(stderr, "Invalid instruction.\n");
                return SK_VM_ERR;
        }
    }

#undef pop
#undef push
#undef read_short
#undef read_const
#undef read_byte
}
