#pragma once

#include "objects.h"
#include "symbols.h"

// Starts the definition of a C representation of a Python function.
//
// Example usage:
// ```
//      PY_DEFINE(example_fn) {
//          pyobj_t* first_arg = argv[0];
//      }
// ```
#define PY_DEFINE($name)        \
    pyobj_t* $name (            \
        pyobj_t* self,          \
        int argc,               \
        pyobj_t** argv,         \
        int kwargc,             \
        symbol_t* kwargv        \
    )

// Returns the name of the callable object that wraps over the function `$fn`.
#define FUNCTION_WRAPPER($fn) $fn##_object

// Defines a fixed callable object that wraps over the function `$fn`. To reference
// this object, use `FUNCTION_WRAPPER($fn)`.
#define DEFINE_FUNCTION_WRAPPER($fn, $global_name)                       \
    static const pyobj_t FUNCTION_WRAPPER($fn) = {                       \
        .type = &py_type_callable,                                       \
        .as_callable = &$fn                                              \
    };                                                                   \
    const pyobj_t* KNOWN_GLOBAL($global_name) = &FUNCTION_WRAPPER($fn);  \

