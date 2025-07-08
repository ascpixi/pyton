#pragma once

#include <stddef.h>
#include "../sys/core.h"
#include "util.h"

#define FILE_LOCATION __FILE_NAME__ "@" MACRO_AS_STRING(__LINE__)

#define ENSURE_NOT_NULL($value)                                    \
    if ((void*)($value) == NULL) {                                 \
        sys_panic(FILE_LOCATION ": '" #$value "' was null.");      \
    }

#define NOT_NULL($value)                                              \
    ({                                                                \
        __typeof__(($value)) _tmp = ($value);                         \
        if ((void*)(_tmp) == NULL) {                                  \
            sys_panic(FILE_LOCATION ": '" #$value "' was null.");     \
        }                                                             \
        _tmp;                                                         \
    })

#define ASSERT($expr)                                                     \
    if (!($expr)) {                                                       \
        sys_panic("assertion '" #$expr "' failed at " FILE_LOCATION);     \
    }
