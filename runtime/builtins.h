#pragma once

#include "functions.h"
#include "symbols.h"
#include "objects.h"

// Define builtins here. Builtins are a special case of globals, where they're provided
// by the runtime.

// Manually defines a known global. When defining such a global, be sure to also define
// a corresponding `PY_GLOBAL_<name>_WELLKNOWN` preprocessor symbol, so that the transpiler
// does not create a duplicate.
#define KNOWN_GLOBAL($name) pyglobal__##$name

// def print(...)
#define PY_GLOBAL_print_WELLKNOWN
extern const pyobj_t* KNOWN_GLOBAL(print);
PY_DEFINE(py_builtin_print);
