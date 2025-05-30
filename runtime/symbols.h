#pragma once

#include "objects.h"
#include "rtl/vector.h"

// Represents a single entry in a symbol table.
typedef struct symbol {
    pyobj_t* value;
    const char* name;
} symbol_t;

USES_VECTOR_FOR(symbol_t);

// Looks up a symbol within the locals, globals, and builtins, and returns its value.
// If the symbol wasn't found anywhere, the function returns NULL.
pyobj_t* py_resolve_symbol(
    const char* name,
    vector_t(symbol_t)* locals
);

// A list of all global symbols.
extern vector_t(symbol_t) py_globals;
