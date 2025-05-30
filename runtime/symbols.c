#include "symbols.h"

#include "builtins.h"
#include "rtl/stringop.h"
#include "rtl/util.h"

vector_t(symbol_t) py_globals;

static pyobj_t* py_find_symbol(const char* name, const symbol_t* symbols, int symbol_count) {
    for (size_t i = 0; i < (size_t)symbol_count; i++) {
        if (rtl_strequ(symbols[i].name, name)) {
            return symbols[i].value;
        }
    }

    return NULL;
}

pyobj_t* py_resolve_symbol(const char* name) {
    return COALESCE_2(
        py_find_symbol(name, py_globals.elements, py_globals.length),
        py_find_symbol(name, py_builtins, LENGTH_OF(py_builtins))
    );
}

void py_assign_global(const char* name, pyobj_t* value) {
    for (size_t i = 0; i < (size_t)py_globals.length; i++) {
        if (rtl_strequ(py_globals.elements[i].name, name)) {
            // This symbol was already defined before, so we're overwriting it
            py_globals.elements[i].value = value;
            return;
        }
    }

    // This symbol *wasn't* defined before.
    rtl_vector_append(&py_globals, ( (symbol_t) { .name = name, .value = value } ));
}