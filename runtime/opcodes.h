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

// Pops a value from the stack, and jumps to `$label` if the popped object has a boolean
// value of `false`. Assumes that the object on the stack is an exact `bool` operand.
// If the object is not of type `py_type_bool`, then the behavior is undefined.
#define PY_OPCODE_POP_JUMP_IF_FALSE($label)                         \
    if ( ((pyobj_t*)stack[stack_current--])->as_bool == false )     \
        goto $label;                                                \

// Equivalent to `right < left`, where `right` and `left` are placed on the stack.
void py_opcode_compare_lt(void** stack, int* stack_current, bool coerce_to_bool);

// Equivalent to `right <= left`, where `right` and `left` are placed on the stack.
void py_opcode_compare_lte(void** stack, int* stack_current, bool coerce_to_bool);

// Equivalent to `right == left`, where `right` and `left` are placed on the stack.
void py_opcode_compare_equ(void** stack, int* stack_current, bool coerce_to_bool);

// Equivalent to `right != left`, where `right` and `left` are placed on the stack.
void py_opcode_compare_neq(void** stack, int* stack_current, bool coerce_to_bool);

// Equivalent to `right > left`, where `right` and `left` are placed on the stack.
void py_opcode_compare_gt(void** stack, int* stack_current, bool coerce_to_bool);

// Equivalent to `right >= left`, where `right` and `left` are placed on the stack.
void py_opcode_compare_gte(void** stack, int* stack_current, bool coerce_to_bool);
