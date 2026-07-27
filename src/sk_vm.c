#include "sk_vm.h"

#include <stdio.h>

#include "sk_memory.h"

void sk_chunk_init(struct sk_chunk *chunk)
{
    sk_value_array_init(&chunk->constants);

    chunk->locals_count = 0;

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

void sk_program_init(struct sk_program *program)
{
    program->functions.functions = NULL;
    program->functions.capacity = 0;
    program->functions.count = 0;
    program->entry = 0;
}

void sk_program_free(struct sk_program *program)
{
    for (size_t i = 0; i < program->functions.count; i++) {
        sk_chunk_free(&program->functions.functions[i].chunk);
    }

    sk_free(program->functions.functions);
    sk_program_init(program);
}

struct sk_compiled_function *sk_program_add_function(struct sk_program *program, sk_fnptr fnptr)
{
    if (fnptr >= program->functions.capacity) {
        size_t old_capacity = program->functions.capacity;
        size_t new_capacity = old_capacity == 0 ? 8 : old_capacity;
        while (fnptr >= new_capacity) {
            new_capacity = sk_grow(new_capacity);
        }

        program->functions.functions = sk_realloc(program->functions.functions, new_capacity);
        for (size_t i = old_capacity; i < new_capacity; i++) {
            sk_chunk_init(&program->functions.functions[i].chunk);
            program->functions.functions[i].parameter_count = 0;
        }
        program->functions.capacity = new_capacity;
    }

    if (fnptr >= program->functions.count) {
        program->functions.count = fnptr + 1;
    }

    return &program->functions.functions[fnptr];
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
    vm->program = NULL;
    vm->frame_count = 0;
}

void sk_vm_free(struct sk_vm *vm)
{
    sk_vm_stack_free(&vm->stack);
}

static enum sk_vm_result vm_loop(struct sk_vm *vm);
static void reserve_stack_slots(struct sk_vm *vm, size_t count);
static void vm_print(struct sk_vm_stack *stack);

enum sk_vm_result sk_vm_run(struct sk_vm *vm, struct sk_program *program)
{
    vm->program = program;
    const struct sk_compiled_function *entry = &program->functions.functions[program->entry];
    vm->frames[0].function = entry;
    vm->frames[0].ip = entry->chunk.code;
    vm->frames[0].base = 0;
    vm->frame_count = 1;

    reserve_stack_slots(vm, entry->chunk.locals_count);

    return vm_loop(vm);
}

static enum sk_vm_result vm_loop(struct sk_vm *vm)
{
#define frame() (&vm->frames[vm->frame_count - 1])
#define read_byte() *frame()->ip++
#define read_const() sk_value_array_get(&frame()->function->chunk.constants, read_byte())
#define read_short() (frame()->ip += 2, (uint16_t)(frame()->ip[-2] << 8 | frame()->ip[-1]))

#define push(value) sk_vm_stack_push(&vm->stack, value)
#define pop() sk_vm_stack_pop(&vm->stack)
#define peek(depth) sk_vm_stack_peek(&vm->stack, (depth))

    for (;;) {
        switch (read_byte()) {
            case SK_OP_HALT:
                return SK_VM_OK;
            case SK_OP_RETURN: {
                struct sk_value result = pop();
                if (vm->frame_count == 1) {
                    return SK_VM_OK;
                }

                size_t call_base = vm->frames[vm->frame_count - 1].base;
                vm->frame_count--;
                vm->stack.top = vm->stack.stack + call_base;
                push(result);
                break;
            }
            case SK_OP_PRINT: {
                vm_print(&vm->stack);
                break;
            }

            case SK_OP_POP:
                pop();
                break;

            case SK_OP_NOTHING:
                push(sk_nothing_value());
                break;

            case SK_OP_CONST:
                push(read_const());
                break;

            case SK_OP_LOAD_LOCAL: {
                uint8_t slot = read_byte();
                push(vm->stack.stack[frame()->base + slot]);
                break;
            }

            case SK_OP_STORE_LOCAL: {
                uint8_t slot = read_byte();
                vm->stack.stack[frame()->base + slot] = pop();
                break;
            }

            case SK_OP_CALL: {
                uint8_t argument_count = read_byte();
                size_t call_base = (size_t)(vm->stack.top - vm->stack.stack) - argument_count - 1;
                sk_fnptr fnptr = sk_as_fnptr(vm->stack.stack[call_base]);
                const struct sk_compiled_function *function = &vm->program->functions.functions[fnptr];

                for (size_t i = 0; i < argument_count; i++) {
                    vm->stack.stack[call_base + i] = vm->stack.stack[call_base + i + 1];
                }

                vm->frames[vm->frame_count].function = function;
                vm->frames[vm->frame_count].ip = function->chunk.code;
                vm->frames[vm->frame_count].base = call_base;
                vm->frame_count++;
                vm->stack.top = vm->stack.stack + call_base + function->chunk.locals_count;
                for (size_t i = argument_count; i < function->chunk.locals_count; i++) {
                    vm->stack.stack[call_base + i] = sk_nothing_value();
                }
                break;
            }

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
                frame()->ip += offset;
                break;
            }
            case SK_OP_JMP_BACK: {
                uint16_t offset = read_short();
                frame()->ip -= offset;
                break;
            }
            case SK_OP_JMP_TRUE: {
                uint16_t offset = read_short();
                sk_bool a = sk_as_boolean(peek(0));
                if (a) {
                    frame()->ip += offset;
                }

                break;
            }
            case SK_OP_JMP_FALSE: {
                uint16_t offset = read_short();
                sk_bool a = sk_as_boolean(peek(0));
                if (!a) {
                    frame()->ip += offset;
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
#undef frame
#undef read_short
#undef read_const
#undef read_byte
}

static void vm_print(struct sk_vm_stack *stack)
{
    const struct sk_object_string *template = sk_as_string(sk_vm_stack_pop(stack));
    for (size_t i = 0; i < template->length; i++) {
        char c = template->chars[i];
        if (c == '%') {
            // The following line is safe because the char on length + 1 is '\0'.
            char next_c = template->chars[++i];
            switch (next_c) {
                case 'n':
                    sk_number_print(sk_vm_stack_pop(stack));
                    break;
                case 'b':
                    sk_boolean_print(sk_vm_stack_pop(stack));
                    break;
                case 's':
                    sk_string_print(sk_vm_stack_pop(stack));
                    break;
                case 'f':
                    sk_fnptr_print(sk_vm_stack_pop(stack));
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
}

static void reserve_stack_slots(struct sk_vm *vm, size_t count)
{
    struct sk_vm_stack *stack = &vm->stack;
    for (size_t i = 0; i < count; i++) {
        stack->stack[i] = sk_nothing_value();
    }

    stack->top = stack->stack + count;
}
