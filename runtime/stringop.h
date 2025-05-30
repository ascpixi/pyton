#pragma once

#include <stdbool.h>

// Compares two strings. If the strings are equal, the function returns 0.
int strcmp(const char* str1, const char* str2);

// Checks for equality between two strings. Similar to `strcmp`, but returns `true` if
// the strings are equal, and `false` otherwise. 
bool streq(const char* str1, const char* str2);