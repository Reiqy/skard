#ifndef SK_VALUE_H
#define SK_VALUE_H

#include <stddef.h>
#include <stdbool.h>

typedef double sk_number;
typedef bool sk_bool;

sk_number sk_number_from_string(const char *str, size_t length);

struct sk_value {
    union {
        sk_number number;
        sk_bool boolean;
    } as;
};

#define sk_as_number(value) value.as.number
#define sk_as_boolean(value) value.as.boolean

#define sk_number_value(value) ((struct sk_value) { .as.number = (value) })
#define sk_boolean_value(value) ((struct sk_value) { .as.boolean = (value) })

#define sk_boolean_true sk_boolean_value(true)
#define sk_boolean_false sk_boolean_value(false)

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
