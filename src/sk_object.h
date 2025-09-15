#ifndef SKARD_SK_OBJECT_H
#define SKARD_SK_OBJECT_H

#include <stddef.h>
#include <stdbool.h>

struct sk_object {
    // TODO: The following value is here only because this struct cannot be empty.
    // Eventually it will be replaced by something else.
    bool unused;
};

struct sk_object *sk_object_new(size_t size);

struct sk_object_string {
    struct sk_object obj;
    size_t length;
    char chars[];
};

struct sk_object_string *sk_object_string_new(size_t length);
struct sk_object_string *sk_object_string_from_chars(const char *chars, size_t length);


#endif //SKARD_SK_OBJECT_H
