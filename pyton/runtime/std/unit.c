#include "unit.h"
#include "safety.h"

void std_unit_set(void* sequence, int index, unit_t unit) {
    ENSURE_NOT_NULL(sequence);
    ENSURE_NOT_NULL(unit.data);
    ASSERT(index >= 0);
    ASSERT(unit.size > 0);

    memcpy(
        (uint8_t*)sequence + (index * unit.size),
        unit.data,
        unit.size
    );
}

void std_unit_read(void* sequence, int index, void* out, size_t unit_size) {
    ENSURE_NOT_NULL(sequence);
    ENSURE_NOT_NULL(out);
    ASSERT(index >= 0);

    memcpy(
        out,
        (uint8_t*)sequence + (index * unit_size),
        unit_size
    );
}
