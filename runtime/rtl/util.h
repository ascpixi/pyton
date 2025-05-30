#pragma once

// Performs null-coalescing across two values:
//     - if the evaluation of `$a` is not `NULL`, it is returned, otherwise:
//     - `$b` is returned.
#define COALESCE_2($a, $b) ({    \
    __typeof__($a) _tmp;         \
    (_tmp = ($a)) ? _tmp :       \
    ($b);                        \
})

// Performs null-coalescing across three values:
//     - if the evaluation of `$a` is not `NULL`, it is returned, otherwise:
//     - if the evaluation of `$b` is not `NULL`, it is returned, otherwise:
//     - `$c` is returned.
#define COALESCE_3($a, $b, $c) ({  \
    __typeof__($a) _tmp;           \
    (_tmp = ($a)) ? _tmp :         \
    (_tmp = ($b)) ? _tmp :         \
    ($c);                          \
})

// Returns the length (number of elements) of an array.
#define LENGTH_OF($arr) (sizeof($arr) / sizeof(*$arr))