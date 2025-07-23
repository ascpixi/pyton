#include "string.h"

#include "safety.h"
#include "memory.h"
#include "../sys/mm.h"

bool std_strequ(string_t s1, string_t s2) {
    ENSURE_STR_VALID(s1);
    ENSURE_STR_VALID(s2);

    if (s1.length != s2.length)
        return false;

    for (int i = 0; i < s1.length; i++) {
        if (s1.str[i] != s2.str[i]) {
            return false;
        }
    }

    return true;
}

string_t std_strconcat_array(const string_t* strings, int n) {
    if (n == 0)
        return STR("");

    ENSURE_NOT_NULL(strings);
    ASSERT(n > 0);

    int result_len = 1; // null terminator
    for (int i = 0; i < n; i++) {
        result_len += strings[i].length;
    }

    char* result = mm_heap_alloc(result_len);

    int pos = 0;
    for (int i = 0; i < n; i++) {
        string_t current = strings[i];
        memcpy(result + pos, current.str, current.length);

        pos += current.length;
    }

    ASSERT(pos == result_len - 1); // we've added up all of the string lengths
    result[result_len - 1] = '\0';
    
    return (string_t) { .str = result, .length = result_len - 1 };
}
