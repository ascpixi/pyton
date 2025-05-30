#pragma once

#include "stddef.h"

// Declares a `span_t(T)` structure for the given type.
#define DECLARE_SPAN_OF(T)          \
    typedef struct T ## _span {     \
        T* entries;                 \
        size_t length               \
    } T ## _span_t;                 \

// Represents a contigous virtual memory segment.
#define span_t(T) T ## _span_t