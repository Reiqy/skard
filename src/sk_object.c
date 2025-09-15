#include "sk_object.h"

#include <string.h>

#include "sk_utils.h"

struct sk_object *sk_object_new(size_t size)
{
    struct sk_object *object = sk_allocs(size);
    object->unused = false;
    return object;
}

#define allocate_object(type, additional_size) ((type *)sk_object_new(sizeof(type) + (additional_size)))

struct sk_object_string *sk_object_string_new(size_t length)
{
    struct sk_object_string *string = allocate_object(struct sk_object_string, (length + 1) * sizeof(char));
    string->length = length;
    return string;
}

struct sk_object_string *sk_object_string_from_chars(const char *chars, size_t length)
{
    struct sk_object_string *string = sk_object_string_new(length);
    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';
    return string;
}
