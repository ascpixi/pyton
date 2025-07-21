#pragma once

#include "objects.h"
#include "symbols.h"
#include "fragments.h"
#include "exceptions.h"
#include "std/safety.h"

// Performs a `CALL` on the given stack, reading the parameters and callable from the stack,
// and pushing the return value to the stack.
//
// On the stack are (in ascending order):
//     - The callable
//     - `self` or `NULL`
//     - The remaining positional arguments
//
#define PY_OPCODE_CALL($argc, $exc_depth, $lasti)                                       \
    {                                                                                   \
        pyobj_t* call_argv[$argc];                                                      \
        for (int i = ($argc) - 1; i >= 0; i--) {                                        \
            call_argv[i] = STACK_POP();                                                 \
        }                                                                               \
        pyobj_t* self = STACK_POP();                                                    \
        pyobj_t* callable = STACK_POP();                                                \
        pyreturn_t result = py_call(callable, $argc, call_argv, 0, NULL, self);         \
        if (result.exception != NULL) {                                                 \
            RAISE_CATCHABLE(result.exception, $exc_depth, $lasti);                      \
        }                                                                               \
        STACK_PUSH() = result.value;                                                    \
    }

// Pops a value from the stack, and jumps to `$label` if the popped object has a boolean
// value of `false`. Assumes that the object on the stack is an exact `bool` operand.
// If the object is not of type `py_type_bool`, then the behavior is undefined.
#define PY_OPCODE_POP_JUMP_IF_FALSE($label)                         \
    if ( ((pyobj_t*)STACK_POP())->as_bool == false )                \
        goto $label;         

// Pops a value from the stack, and jumps to `$label` if the popped object has a boolean
// value of `true`. Assumes that the object on the stack is an exact `bool` operand.
// If the object is not of type `py_type_bool`, then the behavior is undefined.
#define PY_OPCODE_POP_JUMP_IF_TRUE($label)                          \
    if ( ((pyobj_t*)STACK_POP())->as_bool == true )                 \
        goto $label;

// Performs the following operations, in order:
// - pops a value from the stack,
// - pushes the current exception to the top of the stack,
// - pushes the value originally popped back to the stack. 
#define PY_OPCODE_PUSH_EXC_INFO()                   \
    {                                               \
        pyobj_t* tmp = STACK_POP();                 \
        STACK_PUSH() = caught_exception;            \
        STACK_PUSH() = tmp;                         \
    }

// Push the i-th item to the top of the stack without removing it from its original location.
#define PY_OPCODE_COPY($i)                                \
    {                                                     \
        pyobj_t* tmp = (pyobj_t*)(STACK_ITEM($i));        \
        STACK_PUSH() = tmp;                               \
    }

// Performs exception matching for except. Tests whether the STACK[-2] is an exception
// matching STACK[-1]. Pops STACK[-1] and pushes the boolean result of the test.
#define PY_OPCODE_CHECK_EXC_MATCH()                                     \
    {                                                                   \
        pyobj_t* sm2 = stack[stack_current - 1];                        \
        pyobj_t* sm1 = stack[stack_current];                            \
        stack_current--;                                                \
        STACK_PUSH() = AS_PY_BOOL(py_isinstance(sm2, sm1));             \
    }

// Performs the given comparison on the stack.
#define PY_OPCODE_COMPARISON($op, $coerce_to_bool, $exc_depth, $lasti)                      \
    {                                                                                       \
        pyobj_t* exc = py_opcode_compare_##$op(stack, &stack_current, $coerce_to_bool);     \
        if (exc != NULL) {                                                                  \
            RAISE_CATCHABLE(exc, $exc_depth, $lasti);                                       \
        }                                                                                   \
    }                                                                                       \

// Performs the given operation on the stack.
#define PY_OPCODE_OPERATION($op, $exc_depth, $lasti)                  \
    {                                                                 \
        pyobj_t* exc = py_opcode_op_##$op(stack, &stack_current);     \
        if (exc != NULL) {                                            \
            RAISE_CATCHABLE(exc, $exc_depth, $lasti);                 \
        }                                                             \
    }

// Performs the following:
// ```
//      obj = STACK.pop()
//      value = STACK.pop()
//      obj.<$name> = value
// ```
#define PY_OPCODE_STORE_ATTR($name)                                 \
    {                                                               \
        pyobj_t* obj = (pyobj_t*)(STACK_POP());                     \
        pyobj_t* value = (pyobj_t*)(STACK_POP());                   \
        py_set_attribute(obj, STR($name), value);                   \
    }

// Replaces `STACK[-1]` with `getattr(STACK[-1], $name)`.
// This is equivalent to the `LOAD_ATTR` op-code when the low bit of `namei` is not set.
#define PY_OPCODE_LOAD_ATTR($name)                                   \
    {                                                                \
        pyobj_t* attr = (pyobj_t*)(STACK_PEEK());                    \
        STACK_PEEK() = NOT_NULL(py_get_attribute(attr, STR($name))); \
    }

// Attempts to load a method named `$name` from the `STACK[-1]` object.
// `STACK[-1]` is popped. This bytecode distinguishes two cases:
// - if `STACK[-1]` has a method with the correct name, the bytecode pushes the
//   unbound method and `STACK[-1]`. `STACK[-1]` will be used as the first argument
//   (`self`) by `CALL` or `CALL_KW` when calling the unbound method.
// - Otherwise, `NULL` and the object returned by the attribute lookup are pushed.
//
// This op-code is generated when the retrieved attribute will be called.
#define PY_OPCODE_LOAD_ATTR_CALLABLE($name)                                         \
    {                                                                               \
        pyobj_t* owner = (pyobj_t*)(STACK_POP());                                   \
        pyobj_t* attr;                                                              \
        bool is_unbound = py_get_method_attribute(owner, STR($name), &attr);        \
        STACK_PUSH() = is_unbound ? owner : NULL;                                   \
        STACK_PUSH() = attr;                                                        \
    }                     

// Swap the top of the stack with the i-th element:
// ```
//      STACK[-i], STACK[-1] = STACK[-1], STACK[-i]
// ```
#define PY_OPCODE_SWAP($i)                             \
    {                                                  \
        pyobj_t* tmp = (pyobj_t*)(STACK_ITEM($i));     \
        STACK_ITEM($i) = STACK_PEEK();                 \
        STACK_PEEK() = tmp;                            \
    }

// Sets the annotations (`STACK[-2]`) for a given function (`STACK[-1]`).
// This currently only removes the annotations from the stack.
#define PY_OPCODE_SET_FUNC_ATTR_ANNOTATIONS()           \
    {                                                   \
        pyobj_t* fn = (pyobj_t*)STACK_POP();            \
        pyobj_t* annotations = (pyobj_t*)STACK_POP();   \
        STACK_PUSH() = fn;                              \
    }

// Implements `STACK[-1] = iter(STACK[-1])`.
#define PY_OPCODE_GET_ITER($exc_depth, $lasti)                                      \
    {                                                                               \
        pyreturn_t status = py_opcode_get_iter((void**)&stack, &stack_current);     \
        if (status.exception != NULL) {                                             \
            RAISE_CATCHABLE(status.exception, $exc_depth, $lasti);                  \
        }                                                                           \
    }

// `STACK[-1]` is an iterator. Call its `__next__()` method. If this yields a new value,
// push it on the stack (leaving the iterator below it). If the iterator indicates it
// is exhausted then the byte code counter is incremented by delta.
#define PY_OPCODE_FOR_ITER($label, $exc_depth, $lasti)                                      \
    {                                                                                       \
        bool exhausted;                                                                     \
        pyreturn_t status = py_opcode_for_iter((void**)&stack, &stack_current, &exhausted); \
        if (status.exception != NULL) {                                                     \
            RAISE_CATCHABLE(status.exception, $exc_depth, $lasti);                          \
        }                                                                                   \
        if (exhausted)                                                                      \
            goto $label;                                                                    \
    }

// Special case for the `LOAD_NAME` op-code, where the op-code is present within
// a class initialization function (passed into `builtins.__build_class__`).
// Locals are equivalent to `self` attributes in class bodies.
#define PY_OPCODE_LOAD_NAME_CLASS($name)                     \
    STACK_PUSH() = COALESCE_2(                               \
        py_get_attribute(self, STR(#$name)),                 \
        KNOWN_GLOBAL($name)                                  \
    );

// The following functions are implemented in 'opcodes.c'.

// Compliments `PY_OPCODE_FOR_ITER`. `out_exhausted` is set to `true` if the iterator
// was exhausted and the byte code counter should be incremented by delta. If the
// return value has an associated exception, it should be raised.
pyreturn_t py_opcode_for_iter(void** stack, int* stack_current, bool* out_exhausted);

// Compliments `PY_OPCODE_GET_ITER`.
pyreturn_t py_opcode_get_iter(void** stack, int* stack_current);

// The following functions are implemented in 'opcodes_cmp.c'.

// Equivalent to `right < left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_compare_lt(void** stack, int* stack_current, bool coerce_to_bool);

// Equivalent to `right <= left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_compare_lte(void** stack, int* stack_current, bool coerce_to_bool);

// Equivalent to `right == left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_compare_equ(void** stack, int* stack_current, bool coerce_to_bool);

// Equivalent to `right != left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_compare_neq(void** stack, int* stack_current, bool coerce_to_bool);

// Equivalent to `right > left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_compare_gt(void** stack, int* stack_current, bool coerce_to_bool);

// Equivalent to `right >= left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_compare_gte(void** stack, int* stack_current, bool coerce_to_bool);

// The following functions are implemented in 'opcodes_op.c'.

// Equivalent to `right + left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_add(void** stack, int* stack_current);

// Equivalent to `right & left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_and(void** stack, int* stack_current);

// Equivalent to `right // left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_floordiv(void** stack, int* stack_current);

// Equivalent to `right << left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_lsh(void** stack, int* stack_current);

// Equivalent to `right @ left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_matmul(void** stack, int* stack_current);

// Equivalent to `right * left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_mul(void** stack, int* stack_current);

// Equivalent to `right % left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_rem(void** stack, int* stack_current);

// Equivalent to `right | left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_or(void** stack, int* stack_current);

// Equivalent to `right ** left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_pow(void** stack, int* stack_current);

// Equivalent to `right >> left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_rsh(void** stack, int* stack_current);

// Equivalent to `right - left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_sub(void** stack, int* stack_current);

// Equivalent to `right ^ left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_xor(void** stack, int* stack_current);

// Equivalent to `right += left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_iadd(void** stack, int* stack_current);

// Equivalent to `right &= left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_iand(void** stack, int* stack_current);

// Equivalent to `right //= left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_ifloordiv(void** stack, int* stack_current);

// Equivalent to `right <<= left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_ilsh(void** stack, int* stack_current);

// Equivalent to `right @= left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_imatmul(void** stack, int* stack_current);

// Equivalent to `right *= left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_imul(void** stack, int* stack_current);

// Equivalent to `right %= left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_irem(void** stack, int* stack_current);

// Equivalent to `right |= left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_ior(void** stack, int* stack_current);

// Equivalent to `right **= left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_ipow(void** stack, int* stack_current);

// Equivalent to `right >>= left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_irsh(void** stack, int* stack_current);

// Equivalent to `right -= left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_isub(void** stack, int* stack_current);

// Equivalent to `right ^= left`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_ixor(void** stack, int* stack_current);

// Equivalent to `right[left]`, where `right` and `left` are placed on the stack. Returns an exception or NULL.
pyobj_t* py_opcode_op_subscr(void** stack, int* stack_current);
