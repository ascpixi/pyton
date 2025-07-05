#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "symbols.h"
#include "rtl/vector.h"
#include "sys/mm.h"

// Represents a symbol - an object associated with a name. Symbols may represent attributes,
// or be entries in symbol tables (e.g. `builtins` or `globals`).
typedef struct symbol symbol_t;
USES_VECTOR_FOR(symbol_t);

// Represents any Python object.
typedef struct pyobj pyobj_t;

// Represents the return value of a callable object (a Python function or method).
typedef struct pyreturn pyreturn_t;

// Represents a pointer to a function that handles a built-in Python callable (function or method).
typedef pyreturn_t (*py_fnptr_callable_t)(
    pyobj_t* self,
    int argc,
    pyobj_t** argv,
    int kwargc,
    symbol_t* kwargv
);

struct pyobj {
    // Represents the type of the object.
    pyobj_t* type;

    union {
        // Valid when `type` points to a non-intrinsic `pyobj_t` - as in, when
        // `type->as_type.is_intrinsic` is `false`.
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
        struct type_data {
            // Represents the attribute table of the class.
            vector_t(symbol_t) class_attributes;

            // The class this one inherits from. If there is no such class, this
            // field is equal to `NULL`.
            pyobj_t* base;

            // If `true`, objects of this type are qualified as "intrinsic", meaning
            // that they do not hold an attribute table that would usually be accessed
            // via `as_any`.
            bool is_intrinsic;
        } as_type;

        // Valid when `type` points to `py_type_function`.
        py_fnptr_callable_t as_function;

        // Valid when `type` points to `py_type_method`.
        struct method_data {
            // The object the method is bound to. This will be the `self` parameter.
            pyobj_t* bound;

            // The body of the method.
            py_fnptr_callable_t body;
        } as_method;

        // Valid when `type` points to `py_type_list` *or* `py_type_tuple`.
        vector_t(pyobj_t*) as_list;
    };
};

struct symbol {
    pyobj_t* value;
    const char* name;
};

struct pyreturn {
    pyobj_t* value;
    pyobj_t* exception;
};

// The type that represents the `object` Python class.
extern const pyobj_t py_type_object;
extern const pyobj_t* KNOWN_GLOBAL(object);
#define PY_GLOBAL_object_WELLKNOWN

// The type that represents the `bool` Python class.
extern const pyobj_t py_type_bool;
extern const pyobj_t* KNOWN_GLOBAL(bool);
#define PY_GLOBAL_bool_WELLKNOWN

// The type that represents the `tuple` Python class.
extern const pyobj_t py_type_tuple;
extern const pyobj_t* KNOWN_GLOBAL(tuple);
#define PY_GLOBAL_tuple_WELLKNOWN

// The type that represents the `list` Python class.
extern const pyobj_t py_type_list;
extern const pyobj_t* KNOWN_GLOBAL(list);
#define PY_GLOBAL_list_WELLKNOWN

// The type that represents the `type` Python class.
extern const pyobj_t py_type_type;
extern const pyobj_t* KNOWN_GLOBAL(type);
#define PY_GLOBAL_type_WELLKNOWN

// The type that represents the `str` Python class.
extern const pyobj_t py_type_str;
extern const pyobj_t* KNOWN_GLOBAL(str);
#define PY_GLOBAL_str_WELLKNOWN

// The type that represents the `int` Python class.
extern const pyobj_t py_type_int;
extern const pyobj_t* KNOWN_GLOBAL(int);
#define PY_GLOBAL_int_WELLKNOWN

// The type that represents the `float` Python class.
extern const pyobj_t py_type_float;
extern const pyobj_t* KNOWN_GLOBAL(float);
#define PY_GLOBAL_float_WELLKNOWN

// The type that represents the `NoneType` Python class.
extern const pyobj_t py_type_NoneType;

// Represents a Python function, which is not bound to an object instance. When invoking
// it, `self` will always be `NULL` and will not be part of the positional arguments of
// the function.
extern const pyobj_t py_type_function;

// Represents a Python method, which is bound to an object instance. When invoking it,
// `self` will be equal to the instance the method is bound to, making it the first
// positional argument of the function.
extern const pyobj_t py_type_method;

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

// Returns `py_true` if `$x` evaluates to `true`, and `py_false` otherwise.
#define AS_PY_BOOL($x) (($x) ? &py_true : &py_false)

// Converts a C integer into a Python one, allocating it on the heap.
pyobj_t* py_alloc_int(int64_t x);

// Creates a function wrapper over the given callable.
pyobj_t* py_alloc_function(py_fnptr_callable_t callable);

// Creates a method bound to a given object instance.
pyobj_t* py_alloc_method(
    py_fnptr_callable_t callable,
    pyobj_t* bound
);

// Allocates an empty `type` instance.
pyobj_t* py_alloc_type(pyobj_t* base);

// Allocates an arbitrary non-intrinsic Python object with the given type.
pyobj_t* py_alloc_object(pyobj_t* type);

// Defines the structure of a `pyobj_t` that defines a string literal.
#define PY_STR_LITERAL($content) { .type = &py_type_str, .as_str = ($content) }

// Defines the structure of a `pyobj_t` that defines an integer constant.
#define PY_INT_CONSTANT($num) { .type = &py_type_int, .as_int = ($num) }

// Attempts to call the given object, assuming it is callable. This function succeeds when
// target is either of type `py_type_builtin_callable`, or when it contains a `__call__` attribute.
pyreturn_t py_call(
    pyobj_t* target,
    int argc,
    pyobj_t** argv,
    int kwargc,
    symbol_t* kwargv
);

// Calls `__str__` on the given object, with no parameters.
const char* py_stringify(pyobj_t* target);

// Tries to find the value associated with the attribute with the given name on the
// given object. If no such attribute exists, returns `NULL` instead.
pyobj_t* py_get_attribute(pyobj_t* target, const char* name);

// Sets the attribute with the name `name` on the given object to `value`.
void py_set_attribute(pyobj_t* target, const char* name, pyobj_t* value);

// Checks if `target` is an instance of `type`.
bool py_isinstance(pyobj_t* target, pyobj_t* type);
