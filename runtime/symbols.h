#pragma once

#include "./rtl/vector.h"

// Represents a single entry in a symbol table.
typedef struct symbol {
    void* address;
    const char* name;
} symbol_t;

DEFINE_LINKED_LIST(symbol_t);

// Looks up a symbol within the locals, globals, and builtins, and returns its value.
// If the symbol wasn't found anywhere, the function returns NULL.
void* py_resolve_symbol(const char* name, vector_t(symbol_t) locals);

// A list of all global symbols.
extern vector_t(symbol_t) py_globals;
