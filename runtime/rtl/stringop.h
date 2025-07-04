#pragma once

#include <stdbool.h>
#include <stddef.h>

// Compares two strings. If the strings are equal, the function returns 0.
int strcmp(const char* str1, const char* str2);

// Calculates the length of a zero-terminated string.
size_t strlen(const char* str);

// Checks for equality between two strings. Similar to `strcmp`, but returns `true` if
// the strings are equal, and `false` otherwise. 
bool rtl_strequ(const char* str1, const char* str2);

// Combines two strings into one.
const char* rtl_strconcat(const char* str1, const char* str2);