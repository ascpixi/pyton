#include "opcodes.h"

#include "sys/core.h"
#include "rtl/stringop.h"

#define STACK_PUSH(x) stack[++(*stack_current)] = (x)

#define OPERATION_PROLOG                          \
    pyobj_t* left = stack[(*stack_current)--];    \
    pyobj_t* right = stack[(*stack_current)--];   \

#define BOTH_OF_TYPE($type) right->type == $type && left->type == $type

#define INT_OPERATION($op)                                               \
    if (BOTH_OF_TYPE(&py_type_int)) {                                    \ 
        STACK_PUSH(ALLOC_PY_INT(right->as_int $op left->as_int));     \
        return;                                                          \
    }                                                                    \

static bool arbitrary_op(
    void** stack,
    int* stack_current,
    const char* attr_name,
    pyobj_t* right,
    pyobj_t* left
) {
    pyobj_t* op_fn = py_get_attribute(right, attr_name);
    if (op_fn != NULL && op_fn->type == &py_type_callable) {
        pyobj_t* args[] = { left };
        STACK_PUSH(op_fn->as_callable(right, 1, args, 0, NULL));
        return true;
    }

    return false;
}

// TODO: string concat
// TODO: float operations

void py_opcode_op_add(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(+);
    arbitrary_op(stack, stack_current, "__add__", right, left);
}

void py_opcode_op_and(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(&);
    arbitrary_op(stack, stack_current, "__and__", right, left);
}

void py_opcode_op_floordiv(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(/);
    arbitrary_op(stack, stack_current, "__floordiv__", right, left);
}

void py_opcode_op_lsh(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(<<);
    arbitrary_op(stack, stack_current, "__lshift__", right, left);
}

void py_opcode_op_matmul(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    arbitrary_op(stack, stack_current, "__matmul__", right, left);
}

void py_opcode_op_mul(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(*);
    arbitrary_op(stack, stack_current, "__mul__", right, left);
}

void py_opcode_op_rem(void** stack, int* stack_current) {
    // TODO: Verify the correctness of using a regular modulo as an int remainder
    OPERATION_PROLOG;
    INT_OPERATION(%);
    arbitrary_op(stack, stack_current, "__mod__", right, left);
}

void py_opcode_op_or(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(|);
    arbitrary_op(stack, stack_current, "__or__", right, left);
}

void py_opcode_op_pow(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    // TODO: int power
    arbitrary_op(stack, stack_current, "__pow__", right, left);
}

void py_opcode_op_rsh(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(>>);
    arbitrary_op(stack, stack_current, "__rshift__", right, left);
}

void py_opcode_op_sub(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(-);
    arbitrary_op(stack, stack_current, "__sub__", right, left);
}

void py_opcode_op_xor(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(^);
    arbitrary_op(stack, stack_current, "__xor__", right, left);
}

void py_opcode_op_iadd(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(+);
    arbitrary_op(stack, stack_current, "__iadd__", right, left);
}

void py_opcode_op_iand(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(&);
    arbitrary_op(stack, stack_current, "__iand__", right, left);
}

void py_opcode_op_ifloordiv(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(/);
    arbitrary_op(stack, stack_current, "__ifloordiv__", right, left);
}

void py_opcode_op_ilsh(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(<<);
    arbitrary_op(stack, stack_current, "__ilshift__", right, left);
}

void py_opcode_op_imatmul(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    arbitrary_op(stack, stack_current, "__imatmul__", right, left);
}

void py_opcode_op_imul(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(*);
    arbitrary_op(stack, stack_current, "__imul__", right, left);
}

void py_opcode_op_irem(void** stack, int* stack_current) {
    // TODO: Verify the correctness of using a regular modulo as an int remainder
    OPERATION_PROLOG;
    INT_OPERATION(%);
    arbitrary_op(stack, stack_current, "__imod__", right, left);
}

void py_opcode_op_ior(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(|);
    arbitrary_op(stack, stack_current, "__ior__", right, left);
}

void py_opcode_op_ipow(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    // TODO: int power
    arbitrary_op(stack, stack_current, "__ipow__", right, left);
}

void py_opcode_op_irsh(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(>>);
    arbitrary_op(stack, stack_current, "__irshift__", right, left);
}

void py_opcode_op_isub(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(-);
    arbitrary_op(stack, stack_current, "__isub__", right, left);
}

void py_opcode_op_ixor(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    INT_OPERATION(^);
    arbitrary_op(stack, stack_current, "__ixor__", right, left);
}

void py_opcode_op_subscr(void** stack, int* stack_current) {
    OPERATION_PROLOG;
    // this one's kinda a guess
    arbitrary_op(stack, stack_current, "__getitem__", right, left);
}