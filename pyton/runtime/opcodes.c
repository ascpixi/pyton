#include "opcodes.h"

pyreturn_t py_opcode_get_iter(void** stack, int* stack_current) {
    pyobj_t* obj = (pyobj_t*)STACK_POP_INDIRECT();
    pyobj_t* iter_method;

    if (!py_get_method_attribute(obj, STR("__iter__"), &iter_method) || iter_method == NULL)
        RAISE(TypeError, "type is not iterable");

    pyobj_t* iter = UNWRAP(py_call(iter_method, 0, NULL, 0, NULL, obj));
    STACK_PUSH_INDIRECT(iter);
    return WITH_RESULT(NULL);
}

pyreturn_t py_opcode_for_iter(void** stack, int* stack_current, bool* out_exhausted) {
    pyobj_t* iter = (pyobj_t*)stack[*stack_current];
    pyobj_t* next;

    if (!py_get_method_attribute(iter, STR("__next__"), &next) || next == NULL)
        RAISE(TypeError, "iterator is missing __next__");

    pyreturn_t status = py_call(next, 0, NULL, 0, NULL, iter);
    if (status.exception != NULL) {
        if (status.exception->type == &py_type_StopIteration) {
            *out_exhausted = true;
            return WITH_RESULT(NULL);
        }

        return status;
    }

    *out_exhausted = false;
    STACK_PUSH_INDIRECT(status.value);
    return WITH_RESULT(NULL);
}