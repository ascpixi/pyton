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

string_t std_strconcat(string_t s1, string_t s2) {
    ENSURE_STR_VALID(s1);
    ENSURE_STR_VALID(s2);

    int result_len = s1.length + s2.length + 1;
    char* result = mm_heap_alloc(result_len);

    memcpy(result, s1.str, s1.length);
    memcpy(result + s1.length, s2.str, s2.length);
    result[result_len - 1] = '\0';
    
    return (string_t) { .str = result, .length = result_len - 1 };
}
