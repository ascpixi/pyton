#pragma once

#include <stdbool.h>
#include <stdint.h>

// Represents any Python object.
typedef struct pyobj {
    // Represents the type of the object.
    pyobj_t* type;

    union {
        // Valid when `type` points to a non-intrinsic `pyobj_t`. Represents a pointer
        // to the attribute table of the object. The table should always end with
        // an attribute with a NULL name and value.
        pyattribute_t* as_any;

        // Valid when `type` points to `py_type_bool`.
        bool as_bool;
        
        // Valid when `type` points to `py_type_str`.
        const char* as_str;

        // Valid when `type` points to `py_type_int`.
        int64_t as_int;

        // Valid when `type` points to `py_type_float`.
        double as_float;

        // Valid when `type` points to `py_type_type`.
        const char* as_typename;

        // Valid when `type` points to `py_type_callable`.
        py_fnptr_callable_t as_callable;
    };
} pyobj_t;

// Represents an attribute - an object associated with a name. Attributes are present on
// all non-intrinsic Python objects.
typedef struct pyattribute {
    const char* name;
    pyobj_t* value;
} pyattribute_t;

// Represents a pointer to a function that handles a built-in Python callable (function or method).
typedef pyobj_t* (*py_fnptr_callable_t)(
    pyobj_t* self,
    int argc,
    pyobj_t** argv,
    int kwargc,
    pyattribute_t* kwargv
);

// The type that represents the `bool` Python class.
const pyobj_t py_type_bool;

// The type that represents the `bytearray` Python class.
const pyobj_t py_type_bytearray;

// The type that represents the `tuple` Python class.
const pyobj_t py_type_tuple;

// The type that represents the `type` Python class.
const pyobj_t py_type_type;

// The type that represents the `str` Python class.
const pyobj_t py_type_str;

// The type that represents the `int` Python class.
const pyobj_t py_type_int;

// The type that represents the `float` Python class.
const pyobj_t py_type_float;

// The type that represents the `NoneType` Python class.
const pyobj_t py_type_nonetype;

// The type that represents any callable. This serves to represent both `function`, `method`,
// and `builtin_function_or_method`.
const pyobj_t py_type_callable;

// The type that represents the `code` Python class. Objects of this type contain
// a pointer to a compiled representation of a function or method. This is similar to
// py_type_builtin_callable, but instead of the runtime providing the function implementation,
// it's user-provided code.
const pyobj_t py_type_code;

// Represents `None`. This object always has the type of `py_type_nonetype` (`NoneType`).
const pyobj_t py_none;

// Represents `True`. This object always has the type of `py_type_bool` (`bool`).
const pyobj_t py_true;

// Represents `False`. This object always has the type of `py_type_bool` (`bool`).
const pyobj_t py_false;

// Attempts to call the given object, assuming it is callable. This function succeeds when
// target is either of type `py_type_builtin_callable`, or when it contains a `__call__` attribute.
pyobj_t* py_call(
    pyobj_t* target,
    pyobj_t* self,
    int argc,
    pyobj_t** argv,
    int kwargc,
    pyattribute_t* kwargv
);

// Tries to find the value associated with the attribute with the given name on the
// given object. If no such attribute exists, returns NULL instead.
pyobj_t* py_get_attribute(pyobj_t* target, const char* name);