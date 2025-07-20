#include "stringop.h"

#include "memory.h"
#include "safety.h"
#include "sys/mm.h"

int strcmp(const char* s1, const char* s2) {
    if (s1 == NULL || s2 == NULL)
        return s1 == s2;

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

