#include "memory.h"

#include <stdint.h>

void* memcpy(void* restrict dest, const void* restrict src, size_t n) {
    uint8_t* restrict pdest = (uint8_t* restrict)dest;
    const uint8_t* restrict psrc = (const uint8_t *restrict)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void rtl_memmove_back(void* target, size_t n, size_t offset) {
    uint8_t* dest = (uint8_t*)target - offset;
    uint8_t* src = (uint8_t*)target;
    
    // Move bytes from start to end since we're moving backwards
    for (size_t i = 0; i < n; i++) {
        dest[i] = src[i];
    }
}