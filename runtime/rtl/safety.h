#pragma once

#include "../sys/core.h"
#include "util.h"

#define ENSURE_NOT_NULL($value, $function)                          \
    if (($value) == NULL) {                                         \
        sys_panic($function ": '" #$value "' was null.");           \
    }                                                               \

#define ASSERT($expr, $func)                                      \
    if (!($expr)) {                                               \
        sys_panic("assertion '" #$expr "' failed @ " $func);      \
    }                                                             \
