#pragma once

#include <stddef.h>
#include <stdint.h>
#include "memory.h"

// Represents a single structure or primitive of an unknown type, usually being a part
// of a larger sequence.
typedef struct unit {
    void* data;
    size_t size;
} unit_t;

// Assuming `sequence` is a sequence of units of the same size as `unit`, sets the `index`-th
// unit in the sequence to `unit`.
void rtl_unit_set(void* sequence, int index, unit_t unit) {
    memcpy(
        (uint8_t*)sequence + (index * unit.size),
        unit.data,
        unit.size
    );
}

void rtl_unit_read(void* sequence, int index, void* out, size_t unit_size) {
    memcpy(
        out,
        (uint8_t*)sequence + (index * unit_size),
        unit_size
    );
}