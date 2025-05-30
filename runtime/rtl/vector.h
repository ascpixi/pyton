#pragma once

#pragma once

#include <stddef.h>
#include "unit.h"

// Defines the fields used by all vector structures (i.e. both the generic and type-erased versions).
#define _VECTOR_FIELDS(T)   \
    T* elements;            \
    size_t length;          \
    size_t capacity;        \

// Represents a vector that holds elements of an unknown type. This is the type-erased
// variant of `vector_t(T)` and is equivalent in structure to `vector_t(void)`.
typedef struct vector_any { _VECTOR_FIELDS(void) } vector_any_t;

// Declares that the given translation unit will use a `vector_t(T)` structure.
#define USES_VECTOR_FOR(T) struct vector_##T { _VECTOR_FIELDS(T) }

// Represents a vector that holds elements of type `T`.
// Usage:
// ```
//      vector_t(int) vec = {};
//      rtl_vector_append(&vec, 1);
//      rtl_vector_append(&vec, 2);
//      printf(vec.elements[0]); // prints 1
// ```
#define vector_t(T) struct vector_##T

// Appends a new element, expanding the vector if necessary.
#define rtl_vector_append($vector, $element)                                         \
    {                                                                                \
        __typeof__(($element)) element = ($element);                                 \
        rtl_vector_any_append(                                                       \
            (vector_any_t*)($vector),                                                \
            (unit_t) { .data = (void*)&element, .size = sizeof(element) }            \
        );                                                                           \
    }

// Returns the size of a single element of the given vector.
#define rtl_vector_unit_size($vector) sizeof(*($vector)->elements)

// Removes the element at the given index.
#define rtl_vector_remove($vector, $index)  \
    rtl_vector_any_remove((vector_any_t*)($vector), $index, rtl_vector_unit_size($vector))

// Sets the element at `index` to the given value. This is a non-generic (type-erased)
// function, and should be avoided if possible.
void rtl_vector_any_set(vector_any_t* vec, int index, unit_t value);

// Appends a new element, expanding the vector if necessary. This is a non-generic (type-erased)
// function, and should be avoided if possible. See `rtl_vector_append` instead.
void rtl_vector_any_append(vector_any_t* vec, unit_t value);

// Removes the element at the given index. This is a non-generic (type-erased)
// function, and should be avoided if possible. See `rtl_vector_remove` instead.
void rtl_vector_any_remove(vector_any_t* vec, int index, size_t unit_size);
