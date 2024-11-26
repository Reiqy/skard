#include "sk_value.h"

#include <stdio.h>

#include "sk_utils.h"

void sk_value_print(struct sk_value value)
{
    // Currently we only support numbers
    printf("%lf", sk_as_number(value));
}

void sk_value_array_init(struct sk_value_array *array)
{
    array->array = NULL;
    array->capacity = 0;
    array->count = 0;
}

void sk_value_array_free(struct sk_value_array *array)
{
    sk_free(array->array);
    sk_value_array_init(array);
}

void sk_value_array_add(struct sk_value_array *array, struct sk_value value)
{
    if (array->count >= array->capacity) {
        array->capacity = sk_grow(array->capacity);
        array->array = sk_realloc(array->array, array->capacity);
    }

    array->array[array->count] = value;
    array->count++;
}

struct sk_value sk_value_array_get(const struct sk_value_array *array, size_t index)
{
    return array->array[index];
}
