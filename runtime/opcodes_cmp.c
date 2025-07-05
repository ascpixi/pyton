#include "opcodes.h"

#include "sys/core.h"
#include "rtl/stringop.h"

#define STACK_PUSH(x) stack[++(*stack_current)] = (x)

#define COMPARE_PROLOG                            \
    pyobj_t* left = stack[(*stack_current)--];    \
    pyobj_t* right = stack[(*stack_current)--];   \

#define BOTH_OF_TYPE($type) (right->type == ($type) && left->type == ($type))

#define INT_COMPARISON($op)                                              \
    if (BOTH_OF_TYPE(&py_type_int)) {                                    \
        STACK_PUSH(AS_PY_BOOL(right->as_int $op left->as_int));          \
        return NULL;                                                     \
    }                                                                    \

#define FLOAT_COMPARISON($op)                                            \
    if (BOTH_OF_TYPE(&py_type_float)) {                                  \
        STACK_PUSH(AS_PY_BOOL(right->as_float $op left->as_float));      \
        return NULL;                                                     \
    }                                                                    \

static bool arbitrary_compare_side(
    pyobj_t* side1,
    pyobj_t* side2,
    const char* attr_name,
    void** stack,
    pyobj_t** out_exception,
    int* stack_current
) {
    pyobj_t* compare_fn = py_get_attribute(side1, attr_name);
    if (compare_fn == NULL || compare_fn->type != &py_type_method)
        return false;

    pyobj_t* args[] = { side2 };

    pyreturn_t result = py_call(compare_fn, 1, args, 0, NULL);
    if (result.exception != NULL) {
        *out_exception = result.exception;
    }
    else {
        STACK_PUSH(result.value);
    }

    return true;
}

static bool arbitrary_compare(
    void** stack,
    int* stack_current,
    const char* attr_name,
    pyobj_t* right,
    pyobj_t* left,
    pyobj_t** out_exception
) {
    if (arbitrary_compare_side(right, left, attr_name, stack, out_exception, stack_current))
        return true;

    if (arbitrary_compare_side(left, right, attr_name, stack, out_exception, stack_current))
        return true;

    return false;
}

pyobj_t* py_opcode_compare_equ(void** stack, int* stack_current, bool coerce_to_bool) {
    COMPARE_PROLOG;
    INT_COMPARISON(==);
    // FLOAT_COMPARISON(==);

    if (BOTH_OF_TYPE(&py_type_str)) {
        STACK_PUSH(AS_PY_BOOL(rtl_strequ(right->as_str, left->as_str)));
        return NULL;
    }

    // TODO: comparison between float and int

    pyobj_t* exception = NULL;
    if (arbitrary_compare(stack, stack_current, "__eq__", right, left, &exception))
        return exception;

    // No __eq__ method on any of the objects! Check for identity instead.
    STACK_PUSH(AS_PY_BOOL(left == right));
    return NULL;
}

pyobj_t* py_opcode_compare_neq(void** stack, int* stack_current, bool coerce_to_bool) {
    COMPARE_PROLOG;
    INT_COMPARISON(!=);
    // FLOAT_COMPARISON(!=);

    if (BOTH_OF_TYPE(&py_type_str)) {
        STACK_PUSH(AS_PY_BOOL(!rtl_strequ(right->as_str, left->as_str)));
        return NULL;
    }

    // TODO: comparison between float and int

    pyobj_t* exception = NULL;
    if (arbitrary_compare(stack, stack_current, "__ne__", right, left, &exception))
        return exception;

    // TODO: If no __ne__ method on any of the objects, invert __eq__ instead
    STACK_PUSH(AS_PY_BOOL(left != right));
    return NULL;
}

pyobj_t* py_opcode_compare_lt(void** stack, int* stack_current, bool coerce_to_bool) {
    COMPARE_PROLOG;
    INT_COMPARISON(<);
    // FLOAT_COMPARISON(<);

    // TODO: comparison between float and int

    pyobj_t* exception = NULL;
    if (arbitrary_compare(stack, stack_current, "__lt__", right, left, &exception))
        return exception;

    return NEW_EXCEPTION_INLINE(TypeError, "'<' not supported between two instances of the given objects");
}

pyobj_t* py_opcode_compare_lte(void** stack, int* stack_current, bool coerce_to_bool) {
    COMPARE_PROLOG;
    INT_COMPARISON(<=);
    // FLOAT_COMPARISON(<=);

    // TODO: comparison between float and int

    pyobj_t* exception = NULL;
    if (arbitrary_compare(stack, stack_current, "__le__", right, left, &exception))
        return exception;

    return NEW_EXCEPTION_INLINE(TypeError, "'<=' not supported between two instances of the given objects");
}

pyobj_t* py_opcode_compare_gt(void** stack, int* stack_current, bool coerce_to_bool) {
    COMPARE_PROLOG;
    INT_COMPARISON(>);
    // FLOAT_COMPARISON(>);

    // TODO: comparison between float and int

    pyobj_t* exception = NULL;
    if (arbitrary_compare(stack, stack_current, "__gt__", right, left, &exception))
        return exception;

    return NEW_EXCEPTION_INLINE(TypeError, "'>' not supported between two instances of the given objects");
}

pyobj_t* py_opcode_compare_gte(void** stack, int* stack_current, bool coerce_to_bool) {
    COMPARE_PROLOG;
    INT_COMPARISON(>=);
    // FLOAT_COMPARISON(>=);

    // TODO: comparison between float and int

    pyobj_t* exception = NULL;
    if (arbitrary_compare(stack, stack_current, "__ge__", right, left, &exception))
        return exception;

    return NEW_EXCEPTION_INLINE(TypeError, "'>=' not supported between two instances of the given objects");
}
