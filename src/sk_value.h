#ifndef SK_VALUE_H
#define SK_VALUE_H

#include <stddef.h>

typedef double sk_number;

struct sk_value {
    union {
        sk_number number;
    } as;
};

#define sk_as_number(value) value.as.number
#define sk_number(value) (struct sk_value) { .as.number = (value) }

void sk_value_print(struct sk_value value);

struct sk_value_array {
    struct sk_value *array;
    size_t capacity;
    size_t count;
};

void sk_value_array_init(struct sk_value_array *array);
void sk_value_array_free(struct sk_value_array *array);
void sk_value_array_add(struct sk_value_array *array, struct sk_value value);
struct sk_value sk_value_array_get(const struct sk_value_array *array, size_t index);

#endif //SK_VALUE_H
