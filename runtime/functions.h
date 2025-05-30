#pragma once

#include "objects.h"

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
        pyattribute_t* kwargv   \
    )
