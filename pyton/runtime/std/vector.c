#include "vector.h"

#include "../sys/core.h"
#include "../sys/mm.h"
#include "safety.h"

static void verify_index(size_t length, int index) {
    if (index < 0) {
        sys_panic("Attempted to access a negative index of a vector.");
    }

    if (length <= (size_t)index) {
        sys_panic("Attempted to access an out-of-bounds index of a vector.");
    }
}

void std_vector_any_set(vector_any_t* vec, int index, unit_t value) {
    ENSURE_NOT_NULL(vec);
    ENSURE_NOT_NULL(value.data);
    ASSERT(value.size > 0);
    verify_index(vec->length, index);

    std_unit_set(vec->elements, index, value);
}

void std_vector_any_append(vector_any_t* vec, unit_t value) {
    ENSURE_NOT_NULL(vec);
    ENSURE_NOT_NULL(value.data);
    ASSERT(value.size > 0);

    if (vec->elements == NULL || vec->length == 0 || vec->capacity == 0) {
        // This vector hasn't been initialized yet.
        // Start with a capacity of 4.
        vec->elements = mm_heap_alloc(4 * value.size);
        vec->length = 1;
        vec->capacity = 4;
        std_vector_any_set(vec, 0, value);
        return;
    }

    // Check if we're over capacity.
    if (vec->length == vec->capacity) {
        // If so, we need to expand the vector.
        void* old_elements = vec->elements;
        size_t old_length = vec->length;

        vec->capacity *= 2; // exponential expansion policy
        vec->elements = mm_heap_alloc(vec->capacity * value.size);

        // Copy what was previously in our old element array
        memcpy(vec->elements, old_elements, old_length * value.size);

        // The newly allocated array is now our backing store. We can safely free the
        // previous one.
        mm_heap_free(old_elements);
    }

    vec->length++;
    std_vector_any_set(vec, vec->length - 1, value);
}

void std_vector_any_remove(vector_any_t* vec, int index, size_t unit_size) {
    ENSURE_NOT_NULL(vec);
    ASSERT(unit_size > 0);
    verify_index(vec->length, index);

    size_t idx = (size_t)index;

    // If the index points to the last element, we don't have to move anything.
    if (idx == vec->length - 1) {
        vec->length--;
        return;
    }

    // Move everything to the right of `index` one cell to the left. We'll be overwriting
    // the element at `index` itself.

    // Move all elements after index one position to the left
    uint8_t* dest = (uint8_t*)vec->elements + ((idx + 1) * unit_size);
    size_t area_size = (vec->length - idx - 1) * unit_size;

    std_memmove_back(dest, area_size, unit_size);
    vec->length--;
}