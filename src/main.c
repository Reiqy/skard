#include <stdio.h>

#include "skard.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    struct sk_chunk c;

    sk_chunk_init(&c);

    for (int i = 0; i < 100; i++) {
        sk_chunk_add(&c, SK_OP_RETURN);
    }

    sk_debug_chunk(&c, "Test");

    sk_chunk_free(&c);

    return 0;
}
