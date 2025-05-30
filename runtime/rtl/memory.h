#pragma once

// Copies `n` bytes from `src` to `dest`. The addresses cannot overlap. This is a C
// standard library function.
void* memcpy(void* restrict dest, const void* restrict src, size_t n);

// Moves the specified memory area `offset` bytes backwards (to lower addresses).
// This can be visualized as moving the entire area to the left.
void rtl_memmove_back(void* target, size_t n, size_t offset);