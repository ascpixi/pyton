#pragma once

#include "objects.h"

// Performs a `CALL` on the given stack, reading the parameters and callable from the stack,
// and pushing the return value to the stack.
//
// On the stack are (in ascending order):
//     - The callable
//     - `self` or `NULL`
//     - The remaining positional arguments
//
#define PY_OPCODE_CALL($argc)                                                           \
    {                                                                                   \
        pyobj_t* call_argv[$argc];                                                      \
        for (int i = 0; i < $argc; i++) {                                               \
            call_argv[i] = stack[stack_current--];                                      \
        }                                                                               \
                                                                                        \
        pyobj_t* self = stack[stack_current--];                                         \
        pyobj_t* callable = stack[stack_current--];                                     \
        stack[stack_current++] = py_call(callable, self, $argc, call_argv, 0, NULL);    \
    }
