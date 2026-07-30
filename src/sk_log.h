#ifndef SKARD_SK_LOG_H
#define SKARD_SK_LOG_H

#include <stdlib.h>

enum sk_log_level {
    SK_LOG_INFO,
    SK_LOG_WARNING,
    SK_LOG_ERROR,
};

void sk_log(enum sk_log_level level, const char *filename, size_t line, size_t column, const char *message);

#define sk_info(filename, line, column, message) sk_log(SK_INFO, filename, line, column, message)
#define sk_warn(filename, line, column, message) sk_log(SK_WARNING, filename, line, column, message)
#define sk_error(filename, line, column, message) sk_log(SK_ERROR, filename, line, column, message)

#endif // SKARD_SK_LOG_H
