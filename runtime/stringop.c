#include "stringop.h"

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

bool streq(const char* s1, const char* s2) {
    return strcmp(s1, s2) == 0;
}