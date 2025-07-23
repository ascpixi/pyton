#pragma once

#include "stdbool.h"
#include "safety.h"

// Represents an immutable C string with an associated length.
typedef struct string {
    const char* str;
    int length;
} string_t;

// Converts a regular string literal into a `string_t` value.
#define STR($x) ((string_t) { .str = ($x), .length = (sizeof($x) - 1) } )

#define ENSURE_STR_VALID($x) ASSERT(($x).length == 0 || ($x).str != NULL)

// Returns `true` if `s1` and `s2` are equal.
bool std_strequ(string_t s1, string_t s2);

// Combines multiple strings into one. The recommended way to use this function is
// via the `std_strconcat` macro.
string_t std_strconcat_array(const string_t* strings, int n);

// Combines multiple strings into one.
#define std_strconcat(...) std_strconcat_array(                 \
    (const string_t[]){ __VA_ARGS__ },                          \
    sizeof(string_t []){ __VA_ARGS__ } / sizeof(string_t)       \
)
