#include "sk_utils.h"

#include <stdio.h>

void *sk_reallocate(void *ptr, size_t new_size)
{
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }

    void *result = realloc(ptr, new_size);
    if (result == NULL) {
        fprintf(stderr, "Not enough memory.\n");
        free(ptr);
        exit(EXIT_FAILURE);
        return NULL;
    }

    return result;
}
