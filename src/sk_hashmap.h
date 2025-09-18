#ifndef SKARD_SK_HASHMAP_H
#define SKARD_SK_HASHMAP_H

#include <stddef.h>
#include <stdbool.h>

struct sk_hashmap_entry {
    const char *key;
    size_t key_len;
    void *value;
};

struct sk_hashmap {
    struct sk_hashmap_entry *entries;
    size_t capacity;
    size_t count;
};

void sk_hashmap_init(struct sk_hashmap *hashmap);
void sk_hashmap_free(struct sk_hashmap *hashmap);

bool sk_hashmap_set(struct sk_hashmap *hashmap, const char *key, size_t key_len, void *value);
bool sk_hashmap_get(const struct sk_hashmap *hashmap, const char *key, size_t key_len, void **value);

#endif //SKARD_SK_HASHMAP_H
