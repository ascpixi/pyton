#include "opcodes.h"

#include "sys/core.h"
#include "std/safety.h"
#include "std/stringop.h"

#define OPERATION_PROLOG                               \
    pyobj_t* left = NOT_NULL(STACK_POP_INDIRECT());    \
    pyobj_t* right = NOT_NULL(STACK_POP_INDIRECT());   \

#define OPERATION_EPILOG($method, $op)                                                  \
    pyobj_t* exception = NULL;                                                          \
    arbitrary_op(stack, stack_current, $method, right, left, &exception);               \
    if (exception != NULL)                                                              \
        return exception;                                                               \
    return NEW_EXCEPTION_INLINE(TypeError, "unsupported operand type(s) for " $op);     \

#define BOTH_OF_TYPE($type) (right->type == ($type) && left->type == ($type))

#define INT_OPERATION($op)                                                       \
    if (BOTH_OF_TYPE(&py_type_int)) {                                            \
        STACK_PUSH_INDIRECT(py_alloc_int(right->as_int $op left->as_int));       \
        return NULL;                                                             \
    }                                                                            \

static bool arbitrary_op(
    void** stack,
    int* stack_current,
    const char* attr_name,
    pyobj_t* right,
    pyobj_t* left,
    pyobj_t** exception
) {
    pyobj_t* op_fn;

    if (
        py_get_method_attribute(right, attr_name, &op_fn) &&
        op_fn != NULL &&
        op_fn->type == &py_type_method
    ) {
        pyobj_t* args[] = { left };
        pyreturn_t result = py_call(op_fn, 1, args, 0, NULL, NULL);

        if (result.exception != NULL) {
            *exception = result.exception;
        }
        else {
            STACK_PUSH_INDIRECT(result.value);
        }

        return true;
    }

    return false;
}

// TODO: string concat
// TODO: float operations

pyobj_t* py_opcode_op_add(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(+);
    OPERATION_EPILOG("__add__", "+");
}

pyobj_t* py_opcode_op_and(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(&);
    OPERATION_EPILOG("__and__", "&");
}

pyobj_t* py_opcode_op_floordiv(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(/);
    OPERATION_EPILOG("__floordiv__", "//");
}

pyobj_t* py_opcode_op_lsh(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(<<);
    OPERATION_EPILOG("__lshift__", "<<");
}

pyobj_t* py_opcode_op_matmul(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    OPERATION_EPILOG("__matmul__", "@");
}

pyobj_t* py_opcode_op_mul(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(*);
    OPERATION_EPILOG("__mul__", "*");
}

pyobj_t* py_opcode_op_rem(void** stack, int* stack_current) {
    // TODO: Verify the correctness of using a regular modulo as an int remainder
    OPERATION_PROLOG;
    INT_OPERATION(%);
    OPERATION_EPILOG("__mod__", "%");
}

pyobj_t* py_opcode_op_or(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(|);
    OPERATION_EPILOG("__or__", "|");
}

pyobj_t* py_opcode_op_pow(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    // TODO: int power
    OPERATION_EPILOG("__pow__", "**");
}

pyobj_t* py_opcode_op_rsh(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(>>);
    OPERATION_EPILOG("__rshift__", ">>");
}

pyobj_t* py_opcode_op_sub(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(-);
    OPERATION_EPILOG("__sub__", "-");
}

pyobj_t* py_opcode_op_xor(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(^);
    OPERATION_EPILOG("__xor__", "^");
}

pyobj_t* py_opcode_op_iadd(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(+);
    OPERATION_EPILOG("__iadd__", "+=");
}

pyobj_t* py_opcode_op_iand(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(&);
    OPERATION_EPILOG("__iand__", "&=");
}

pyobj_t* py_opcode_op_ifloordiv(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(/);
    OPERATION_EPILOG("__ifloordiv__", "//=");
}

pyobj_t* py_opcode_op_ilsh(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(<<);
    OPERATION_EPILOG("__ilshift__", "<<=");
}

pyobj_t* py_opcode_op_imatmul(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    OPERATION_EPILOG("__imatmul__", "@=");
}

pyobj_t* py_opcode_op_imul(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(*);
    OPERATION_EPILOG("__imul__", "*=");
}

pyobj_t* py_opcode_op_irem(void** stack, int* stack_current) {
    // TODO: Verify the correctness of using a regular modulo as an int remainder
    OPERATION_PROLOG;
    INT_OPERATION(%);
    OPERATION_EPILOG("__imod__", "%=");
}

pyobj_t* py_opcode_op_ior(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(|);
    OPERATION_EPILOG("__ior__", "|=");
}

pyobj_t* py_opcode_op_ipow(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    // TODO: int power
    OPERATION_EPILOG("__ipow__", "**=");
}

pyobj_t* py_opcode_op_irsh(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(>>);
    OPERATION_EPILOG("__irshift__", ">>=");
}

pyobj_t* py_opcode_op_isub(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(-);
    OPERATION_EPILOG("__isub__", "-=");
}

pyobj_t* py_opcode_op_ixor(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(^);
    OPERATION_EPILOG("__ixor__", "^=");
}

pyobj_t* py_opcode_op_subscr(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    // this one's kinda a guess
    OPERATION_EPILOG("__getitem__", "[]");
}