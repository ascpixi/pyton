#include "unit.h"

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
