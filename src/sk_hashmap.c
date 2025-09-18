#include "sk_hashmap.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "sk_utils.h"

#define HASHMAP_MAX_LOAD_FACTOR 0.75

static void adjust_capacity(struct sk_hashmap *hashmap, size_t new_capacity);
static struct sk_hashmap_entry *find(struct sk_hashmap_entry *entries, size_t capacity, const char *key, size_t key_len);
static uint32_t hash(const char *key, size_t key_len);

void sk_hashmap_init(struct sk_hashmap *hashmap)
{
    hashmap->entries = NULL;
    hashmap->capacity = 0;
    hashmap->count = 0;
}

void sk_hashmap_free(struct sk_hashmap *hashmap)
{
    sk_free(hashmap->entries);
    sk_hashmap_init(hashmap);
}

bool sk_hashmap_set(struct sk_hashmap *hashmap, const char *key, size_t key_len, void *value)
{
    if ((double)hashmap->count + 1 > (double)hashmap->capacity * HASHMAP_MAX_LOAD_FACTOR) {
        size_t capacity = sk_grow(hashmap->capacity);
        adjust_capacity(hashmap, capacity);
    }

    struct sk_hashmap_entry *entry = find(hashmap->entries, hashmap->capacity, key, key_len);
    bool is_new = entry->key == NULL;
    if (is_new) {
        hashmap->count++;
    }

    entry->key = key;
    entry->key_len = key_len;
    entry->value = value;
    return is_new;
}

bool sk_hashmap_get(const struct sk_hashmap *hashmap, const char *key, size_t key_len, void **value)
{
    if (hashmap->count == 0) {
        return false;
    }

    struct sk_hashmap_entry *entry = find(hashmap->entries, hashmap->capacity, key, key_len);
    if (entry->key == NULL) {
        return false;
    }

    *value = entry->value;
    return true;
}

static void adjust_capacity(struct sk_hashmap *hashmap, size_t new_capacity)
{
    assert(new_capacity > 0);

    struct sk_hashmap_entry *entries = NULL;
    entries = sk_realloc(entries, new_capacity);
    // Clear new buffer.
    for (size_t i = 0; i < new_capacity; i++) {
        entries[i].key = NULL;
        entries[i].key_len = 0;
        entries[i].value = NULL;
    }

    // Rehash old buffer.
    for (size_t i = 0; i < hashmap->capacity; i++) {
        struct sk_hashmap_entry *entry = &hashmap->entries[i];
        if (entry->key == NULL) {
            continue;
        }

        struct sk_hashmap_entry *dest = find(entries, new_capacity, entry->key, entry->key_len);
        dest->key = entry->key;
        dest->key_len = entry->key_len;
        dest->value = entry->value;
    }

    sk_free(hashmap->entries);
    hashmap->entries = entries;
    hashmap->capacity = new_capacity;
}

static struct sk_hashmap_entry *find(struct sk_hashmap_entry *entries, size_t capacity, const char *key, size_t key_len)
{
    uint32_t index = hash(key, key_len) % capacity;
    for (;;) {
        struct sk_hashmap_entry *entry = &entries[index];
        if (entry->key == NULL || memcmp(entry->key, key, key_len) == 0) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

static uint32_t hash(const char *key, size_t key_len)
{
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < key_len; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }

    return hash;
}
