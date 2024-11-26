#include <stdio.h>

#include "skard.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    struct sk_chunk c;

    sk_chunk_init(&c);

    sk_chunk_add_const(&c, sk_number_value(6.9));
    sk_chunk_add_const(&c, sk_number_value(4.2));
    sk_chunk_add(&c, SK_OP_NADD);
    sk_chunk_add_const(&c, sk_number_value(3.14));
    sk_chunk_add(&c, SK_OP_NMUL);
    sk_chunk_add(&c, SK_OP_NNEG);
    sk_chunk_add(&c, SK_OP_DUMP);
    sk_chunk_add(&c, SK_OP_HALT);

    sk_debug_chunk(&c, "Test");

    struct sk_vm vm;

    sk_vm_init(&vm);

    sk_vm_run(&vm, &c);

    sk_vm_free(&vm);

    sk_chunk_free(&c);

    return 0;
}
