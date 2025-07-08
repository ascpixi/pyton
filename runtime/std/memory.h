#pragma once

#include <stddef.h>

// Copies `n` bytes from `src` to `dest`. The addresses cannot overlap.
void* memcpy(void* restrict dest, const void* restrict src, size_t n);

// Sets all bytes in the memory area starting at `dest` and ending at `dest + len` to `val`.
void* memset(void* dest, int val, size_t len);

// Moves the specified memory area `offset` bytes backwards (to lower addresses).
// This can be visualized as moving the entire area to the left.
void std_memmove_back(void* target, size_t n, size_t offset);