#include "memory.h"

#include <stdint.h>
#include "safety.h"

void* memcpy(void* restrict dest, const void* restrict src, size_t n) {
    ENSURE_NOT_NULL(dest);
    ENSURE_NOT_NULL(src);

    uint8_t* restrict pdest = (uint8_t* restrict)dest;
    const uint8_t* restrict psrc = (const uint8_t *restrict)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void* memset(void* dest, int val, size_t len) {
    ENSURE_NOT_NULL(dest);

    uint8_t* ptr = (uint8_t*)dest;
    while (len-- > 0) {
        *ptr++ = val;
    }

    return dest;
}

void std_memmove_back(void* target, size_t n, size_t offset) {
    ENSURE_NOT_NULL(target);

    uint8_t* dest = (uint8_t*)target - offset;
    uint8_t* src = (uint8_t*)target;
    
    // Move bytes from start to end since we're moving backwards
    for (size_t i = 0; i < n; i++) {
        dest[i] = src[i];
    }
}
