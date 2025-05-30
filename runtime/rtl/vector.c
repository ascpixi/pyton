#include "vector.h"

#include "../sys/core.h"
#include "../sys/mm.h"
#include "safety.h"

static void verify_index(size_t length, int index) {
    if (length <= index) {
        sys_panic("Attempted to access an out-of-bounds index of a vector.");
    }

    if (index < 0) {
        sys_panic("Attempted to access a negative index of a vector.");
    }
}

void rtl_vector_any_set(vector_any_t* vec, int index, unit_t value) {
    ENSURE_NOT_NULL(vec, "rtl_vector_any_set");
    ENSURE_NOT_NULL(value.data, "rtl_vector_any_set");
    verify_index(vec->length, index);

    rtl_unit_set(vec->elements, index, value);
}

void rtl_vector_any_append(vector_any_t* vec, unit_t value) {
    ENSURE_NOT_NULL(vec, "rtl_vector_any_append");
    ENSURE_NOT_NULL(value.data, "rtl_vector_any_append");

    if (vec->elements == NULL || vec->length == 0 || vec->capacity == 0) {
        // This vector hasn't been initialized yet.
        // Start with a capacity of 4.
        vec->elements = mm_heap_alloc(4 * value.size);
        vec->length = 1;
        vec->capacity = 4;
        rtl_vector_any_set(vec, 0, value);
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

    rtl_vector_any_set(vec, vec->length, value);
    vec->length++;
}

void rtl_vector_any_remove(vector_any_t* vec, int index, size_t unit_size) {
    ENSURE_NOT_NULL(vec, "rtl_vector_any_remove");
    verify_index(vec->length, index);

    // If the index points to the last element, we don't have to move anything.
    if (index == vec->length - 1) {
        vec->length--;
        return;
    }

    // Move everything to the right of `index` one cell to the left. We'll be overwriting
    // the element at `index` itself.

    // Move all elements after index one position to the left
    uint8_t* dest = (uint8_t*)vec->elements + ((index + 1) * unit_size);
    size_t area_size = (vec->length - index - 1) * unit_size;

    rtl_memmove_back(dest, area_size, unit_size);
    vec->length--;
}