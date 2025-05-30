#include "objects.h"
#include "stringop.h"
#include "sys/core.h"

#define INTRINSIC_TYPE(name) { .type = &py_type_type, .as_typename = name }

// The following represent intrinsic types - ones that we know about and that hold
// C data that represent them internally. User-defined types use these types to
// define their own structures. We treat them specially.

const pyobj_t py_type_bool = INTRINSIC_TYPE("bool");
const pyobj_t py_type_bytearray = INTRINSIC_TYPE("bytearray");
const pyobj_t py_type_float = INTRINSIC_TYPE("float");
const pyobj_t py_type_int = INTRINSIC_TYPE("int");
const pyobj_t py_type_str = INTRINSIC_TYPE("str");
const pyobj_t py_type_tuple = INTRINSIC_TYPE("tuple");
const pyobj_t py_type_type = INTRINSIC_TYPE("type");
const pyobj_t py_type_callable = INTRINSIC_TYPE("callable");
const pyobj_t py_type_nonetype = INTRINSIC_TYPE("NoneType");

const pyobj_t py_none = { .type = &py_type_nonetype };
const pyobj_t py_true = { .type = &py_type_bool, .as_bool = true };
const pyobj_t py_false = { .type = &py_type_bool, .as_bool = false };

pyobj_t* py_get_attribute(pyobj_t* target, const char* name) {
    if (name == NULL)
        return NULL; // TODO: probably log a warning here

    if (target->type == &py_type_bool) {
        // Attribute set for `bool`
        // TODO: Attributes like `__abs__` and `__eq__` for `bool`
        return NULL;
    }

    if (target->type == &py_type_str) {
        // Attribute set for `str`
        // TODO: Attributes like `__eq__` for `str`
        return NULL;
    }

    if (target->type == &py_type_callable) {
        // Attribute set for `py_type_callable`
        // TODO: Attributes like `__call__` for `py_type_callable`
        return NULL;
    }

    if (target->type == &py_type_bytearray) {
        // Attribute set for `py_type_bytearray`
        // TODO: Attributes like `__eq__` for `py_type_bytearray`
        return NULL;
    }

    if (target->type == &py_type_float) {
        // Attribute set for `py_type_float`
        // TODO: Attributes like `__eq__` for `py_type_float`
        return NULL;
    }

    if (target->type == &py_type_int) {
        // Attribute set for `py_type_int`
        // TODO: Attributes like `__eq__` for `py_type_int`
        return NULL;
    }

    if (target->type == &py_type_nonetype) {
        // Attribute set for `py_type_nonetype`
        // TODO: Attributes like `__eq__` for `py_type_nonetype`
        return NULL;
    }

    if (target->type == &py_type_tuple) {
        // Attribute set for `py_type_tuple`
        // TODO: Attributes like `__eq__` for `py_type_tuple`
        return NULL;
    }

    if (target->type == &py_type_type) {
        // Attribute set for `py_type_type`
        // TODO: Attributes like `__eq__` for `py_type_type`
        return NULL;
    }

    // This isn't any intrinsic type we recognize, so we assume that this is an object
    // with a user-defined type.
    pyattribute_t* attributes = target->as_any;
    while (attributes->name != NULL) {
        if (streq(attributes->name, name)) {
            return attributes->value;
        }

        attributes++; // We iterate until name is NULL, which indicates that we reached the end
    }
}

pyobj_t* py_call(
    pyobj_t* target,
    pyobj_t* self,
    int argc,
    pyobj_t** argv,
    int kwargc,
    pyattribute_t* kwargv
) {
    if (target->type == &py_type_callable) {
        // This would be either 'function', 'method', or 'builtin_function_or_method' in CPython.
        return target->as_callable(self, argc, argv, kwargc, kwargv);
    }

    // If the object WASN'T callable, then it might have a `__call__` attribute, which should
    // be a method, which in turn would be a callable.
    pyobj_t* call_attr = py_get_attribute(target, "__call__");
    if (call_attr != NULL && call_attr->type == &py_type_callable) {
        return call_attr->as_callable(self, argc, argv, kwargc, kwargv);
    }

    // TODO: Raise a `TypeError: '...' object is not callable` exception instead of panicking here.
    sys_panic("attempted to call a non-callable object");
}
