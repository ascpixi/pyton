#pragma once

#include "../sys/core.h"

#define ENSURE_NOT_NULL($value, $function)                          \
    if ($value == NULL) {                                           \
        sys_panic($function ": '" #$value "' was null.");  \
    }                                                               \


