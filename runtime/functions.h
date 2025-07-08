#pragma once

#include "objects.h"
#include "symbols.h"
#include "exceptions.h"
#include "std/util.h"

// Starts the definition of a C representation of a Python function.
//
// Example usage:
// ```
//      PY_DEFINE(example_fn) {
//          pyobj_t* first_arg = argv[0];
//      }
// ```
#define PY_DEFINE($name)                  \
    pyreturn_t $name (                    \
        pyobj_t* self,                    \
        int argc,                         \
        pyobj_t** argv,                   \
        int kwargc,                       \
        symbol_t* kwargv                  \
    )

// Returns the name of the callable object that wraps over the function `$fn`.
#define FUNCTION_WRAPPER($fn) $fn##_object

// Defines a fixed callable object that wraps over the function `$fn`. To reference
// this object, use `FUNCTION_WRAPPER($fn)`.
#define DEFINE_FUNCTION_WRAPPER($fn, $global_name)                       \
    static pyobj_t FUNCTION_WRAPPER($fn) = {                             \
        .type = &py_type_function,                                       \
        .as_function = &($fn)                                            \
    };                                                                   \
    pyobj_t* KNOWN_GLOBAL($global_name) = &FUNCTION_WRAPPER($fn);        \

// Copies positional arguments (specified by variables `argc` and `argv`) to
// local variables (specified by array variable `pos_args`), copying at most
// `$max_args` variables.
#define PY_POS_ARGS_TO_VARS($max_args)                              \
    {                                                               \
        int i = 0;                                                  \
        if (self != NULL) {                                         \
            *pos_args[i++] = self;                                  \
        }                                                           \
        for (int j = 0; j < MIN(argc, $max_args); j++) {            \
            *pos_args[i++] = argv[j];                               \
        }                                                           \
    }                                                               \

// Raises an exception if the amount of positional arguments provided (specified
// by variable `argc`) exceeds `$max_args`.
#define PY_POS_ARG_MAX($max_args)                                   \
    if (argc_all > ($max_args)) {                                   \
        RAISE(TypeError, "too many positional arguments");          \
    }

// Raises an exception if the amount of positional arguments provided (specified
// by variable `argc`) is lower than `$min_args`.
#define PY_POS_ARG_MIN($min_args)                                   \
    if (argc_all < ($min_args)) {                                    \
        RAISE(TypeError, "not enough positional arguments");        \
    }
