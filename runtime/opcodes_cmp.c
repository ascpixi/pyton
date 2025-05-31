#include "opcodes.h"

#include "sys/core.h"
#include "rtl/stringop.h"

#define STACK_PUSH(x) stack[++(*stack_current)] = (x)

#define COMPARE_PROLOG                            \
    pyobj_t* left = stack[(*stack_current)--];    \
    pyobj_t* right = stack[(*stack_current)--];   \

#define BOTH_OF_TYPE($type) right->type == $type && left->type == $type

#define INT_COMPARISON($op)                                              \
    if (BOTH_OF_TYPE(&py_type_int)) {                                    \ 
        STACK_PUSH(AS_PY_BOOL(right->as_int $op left->as_int));          \
        return;                                                          \
    }                                                                    \

#define FLOAT_COMPARISON($op)                                            \
    if (BOTH_OF_TYPE(&py_type_float)) {                                  \
        STACK_PUSH(AS_PY_BOOL(right->as_float $op left->as_float));      \
        return;                                                          \
    }                                                                    \

static bool arbitrary_compare(
    void** stack,
    int* stack_current,
    const char* attr_name,
    pyobj_t* right,
    pyobj_t* left
) {
    pyobj_t* compare_fn = py_get_attribute(right, attr_name);
    if (compare_fn != NULL && compare_fn->type == &py_type_callable) {
        pyobj_t* args[] = { left };
        STACK_PUSH(compare_fn->as_callable(right, 1, args, 0, NULL));
        return true;
    }

    compare_fn = py_get_attribute(left, attr_name);
    if (compare_fn != NULL && compare_fn->type == &py_type_callable) {
        pyobj_t* args[] = { right };
        STACK_PUSH(compare_fn->as_callable(left, 1, args, 0, NULL));
        return true;
    }

    return false;
}

void py_opcode_compare_equ(void** stack, int* stack_current, bool coerce_to_bool) {
    COMPARE_PROLOG;
    INT_COMPARISON(==);
    // FLOAT_COMPARISON(==);

    if (BOTH_OF_TYPE(&py_type_str)) {
        STACK_PUSH(AS_PY_BOOL(rtl_strequ(right->as_str, left->as_str)));
        return;
    }

    // TODO: comparison between float and int

    if (arbitrary_compare(stack, stack_current, "__eq__", right, left))
        return;

    // No __eq__ method on any of the objects! Check for identity instead.
    STACK_PUSH(AS_PY_BOOL(left == right));
}

void py_opcode_compare_neq(void** stack, int* stack_current, bool coerce_to_bool) {
    COMPARE_PROLOG;
    INT_COMPARISON(!=);
    // FLOAT_COMPARISON(!=);

    if (BOTH_OF_TYPE(&py_type_str)) {
        STACK_PUSH(AS_PY_BOOL(!rtl_strequ(right->as_str, left->as_str)));
        return;
    }

    // TODO: comparison between float and int

    if (arbitrary_compare(stack, stack_current, "__ne__", right, left))
        return;

    // TODO: If no __ne__ method on any of the objects, invert __eq__ instead
    STACK_PUSH(AS_PY_BOOL(left != right));
}

void py_opcode_compare_lt(void** stack, int* stack_current, bool coerce_to_bool) {
    COMPARE_PROLOG;
    INT_COMPARISON(<);
    // FLOAT_COMPARISON(<);

    // TODO: comparison between float and int

    if (arbitrary_compare(stack, stack_current, "__lt__", right, left))
        return;

    sys_panic("'<' not supported between two instances of the given objects"); // TODO: exception
}

void py_opcode_compare_lte(void** stack, int* stack_current, bool coerce_to_bool) {
    COMPARE_PROLOG;
    INT_COMPARISON(<=);
    // FLOAT_COMPARISON(<=);

    // TODO: comparison between float and int

    if (arbitrary_compare(stack, stack_current, "__le__", right, left))
        return;

    sys_panic("'<=' not supported between two instances of the given objects"); // TODO: exception
}

void py_opcode_compare_gt(void** stack, int* stack_current, bool coerce_to_bool) {
    COMPARE_PROLOG;
    INT_COMPARISON(>);
    // FLOAT_COMPARISON(>);

    // TODO: comparison between float and int

    if (arbitrary_compare(stack, stack_current, "__gt__", right, left))
        return;

    sys_panic("'>' not supported between two instances of the given objects"); // TODO: exception
}

void py_opcode_compare_gte(void** stack, int* stack_current, bool coerce_to_bool) {
    COMPARE_PROLOG;
    INT_COMPARISON(>=);
    // FLOAT_COMPARISON(>=);

    // TODO: comparison between float and int

    if (arbitrary_compare(stack, stack_current, "__ge__", right, left))
        return;

    sys_panic("'>=' not supported between two instances of the given objects"); // TODO: exception
}
