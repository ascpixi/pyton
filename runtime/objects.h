#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "rtl/vector.h"

// Represents a symbol - an object associated with a name. Symbols may represent attributes,
// or be entries in symbol tables (e.g. `builtins` or `globals`).
typedef struct symbol symbol_t;

// Represents any Python object.
typedef struct pyobj pyobj_t;

// Represents a pointer to a function that handles a built-in Python callable (function or method).
typedef pyobj_t* (*py_fnptr_callable_t)(
    pyobj_t* self,
    int argc,
    pyobj_t** argv,
    int kwargc,
    symbol_t* kwargv
);

struct pyobj {
    // Represents the type of the object.
    const pyobj_t* type;

    union {
        // Valid when `type` points to a non-intrinsic `pyobj_t`. Represents a pointer
        // to the attribute table of the object. The table should always end with
        // an attribute with a NULL name and value.
        vector_t(symbol_t) as_any;

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
};

// Represents a single entry in a symbol table.
typedef struct symbol {
    pyobj_t* value;
    const char* name;
} symbol_t;

USES_VECTOR_FOR(symbol_t);

// The type that represents the `bool` Python class.
extern const pyobj_t py_type_bool;

// The type that represents the `bytearray` Python class.
extern const pyobj_t py_type_bytearray;

// The type that represents the `tuple` Python class.
extern const pyobj_t py_type_tuple;

// The type that represents the `type` Python class.
extern const pyobj_t py_type_type;

// The type that represents the `str` Python class.
extern const pyobj_t py_type_str;

// The type that represents the `int` Python class.
extern const pyobj_t py_type_int;

// The type that represents the `float` Python class.
extern const pyobj_t py_type_float;

// The type that represents the `NoneType` Python class.
extern const pyobj_t py_type_nonetype;

// The type that represents any callable. This serves to represent both `function`, `method`,
// and `builtin_function_or_method`.
extern const pyobj_t py_type_callable;

// The type that represents the `code` Python class. Objects of this type contain
// a pointer to a compiled representation of a function or method. This is similar to
// py_type_builtin_callable, but instead of the runtime providing the function implementation,
// it's user-provided code.
extern const pyobj_t py_type_code;

// Represents `None`. This object always has the type of `py_type_nonetype` (`NoneType`).
extern const pyobj_t py_none;

// Represents `True`. This object always has the type of `py_type_bool` (`bool`).
extern const pyobj_t py_true;

// Represents `False`. This object always has the type of `py_type_bool` (`bool`).
extern const pyobj_t py_false;

// Returns `py_true` if `x` evaluates to `true`, and `py_false` otherwise.
#define AS_PY_BOOL(x) ((x) ? &py_true : &py_false)

#define AS_PY_INT(x) 

// Attempts to call the given object, assuming it is callable. This function succeeds when
// target is either of type `py_type_builtin_callable`, or when it contains a `__call__` attribute.
pyobj_t* py_call(
    pyobj_t* target,
    pyobj_t* self,
    int argc,
    pyobj_t** argv,
    int kwargc,
    symbol_t* kwargv
);

// Tries to find the value associated with the attribute with the given name on the
// given object. If no such attribute exists, returns NULL instead.
pyobj_t* py_get_attribute(pyobj_t* target, const char* name);