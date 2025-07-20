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

// Combines two strings into one.
string_t std_strconcat(string_t s1, string_t s2);
