#ifndef SKARD_SK_UTILS_H
#define SKARD_SK_UTILS_H

#include <stdlib.h>

void *sk_reallocate(void *ptr, size_t new_size);

#define sk_free(ptr) sk_reallocate((ptr), 0)
#define sk_realloc(ptr, new_capacity) sk_reallocate((ptr), (new_capacity) * sizeof *(ptr))
#define sk_alloc(type) sk_reallocate(NULL, sizeof(type))
#define sk_allocs(size) sk_reallocate(NULL, (size))

#define sk_grow(capacity) ((capacity) < 8 ? 8 : 2 * (capacity))

#endif //SKARD_SK_UTILS_H
