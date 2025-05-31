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
