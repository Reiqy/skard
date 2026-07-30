#include "sk_log.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static const char *get_log_level_string(enum sk_log_level level);

void sk_log(enum sk_log_level level, const char *filename, size_t line, size_t column, const char *message)
{
    const char *level_string = get_log_level_string(level);
    fprintf(stderr, "%s:%zu:%zu: %s: %s\n", filename, line, column, level_string, message);
}

static const char *get_log_level_string(const enum sk_log_level level)
{
    switch (level) {
        case SK_LOG_INFO:
            return "info";
        case SK_LOG_WARNING:
            return "warning";
        case SK_LOG_ERROR:
            return "error";
    }

    assert(false && "Invalid error level.");
    return "unknown";
}
