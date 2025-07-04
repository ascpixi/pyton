#include "stringop.h"

#include "memory.h"
#include "sys/mm.h"

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

size_t strlen(const char* str) {
    const char* p;

    if (str == NULL)
        return 0;

    p = str;
    while (*p != '\0')  {
        p++; 
    }

    return p - str;
}

bool rtl_strequ(const char* s1, const char* s2) {
    return strcmp(s1, s2) == 0;
}

const char* rtl_strconcat(const char* s1, const char* s2) {
    size_t s1_len = strlen(s1);
    size_t s2_len = strlen(s2);
    size_t result_len = s1_len + s2_len + 1;
    char* result = mm_heap_alloc(result_len);

    memcpy(result, s1, s1_len);
    memcpy(result + s1_len, s2, s2_len);
    result[result_len - 1] = '\0';
    return result;
}