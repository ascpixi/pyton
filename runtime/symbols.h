#pragma once

#include "objects.h"
#include "rtl/vector.h"

// // Looks up a symbol within the global and builtin symbol tables, and returns its value.
// // If the symbol wasn't found anywhere, the function returns NULL.
// pyobj_t* py_resolve_symbol(const char* name);

// Creates or overrides a symbol with the given name in the global symbol table.
void py_assign_global(const char* name, pyobj_t* value);

// A list of all global symbols.
extern vector_t(symbol_t) py_globals;

// Manually defines a known global. When defining such a global, be sure to also define
// a corresponding `PY_GLOBAL_<name>_WELLKNOWN` preprocessor symbol, so that the transpiler
// does not create a duplicate.
#define KNOWN_GLOBAL($name) pyglobal__##$name
