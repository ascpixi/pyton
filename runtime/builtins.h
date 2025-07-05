#pragma once

#include "functions.h"
#include "symbols.h"
#include "objects.h"

// Define builtins here. Builtins are a special case of globals, where they're provided
// by the runtime.

// def __build_class__(...)
#define PY_GLOBAL___build_class___WELLKNOWN
extern const pyobj_t* KNOWN_GLOBAL(__build_class__);
PY_DEFINE(py_builtin_build_class);

// def print(...)
#define PY_GLOBAL_print_WELLKNOWN
extern const pyobj_t* KNOWN_GLOBAL(print);
PY_DEFINE(py_builtin_print);

// class range(...)
#define PY_GLOBAL_range_WELLKNOWN
extern const pyobj_t py_type_range;
extern const pyobj_t* KNOWN_GLOBAL(range);

// class bytearray(...)
#define PY_GLOBAL_bytearray_WELLKNOWN
extern const pyobj_t py_type_bytearray;
extern const pyobj_t* KNOWN_GLOBAL(bytearray);
