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

pyobj_t* py_resolve_symbol(const char* name, vector_t(symbol_t)* locals) {
    return COALESCE(
        py_find_symbol(name, locals->elements, locals->length),
        py_find_symbol(name, py_globals.elements, py_globals.length),
        py_find_symbol(name, py_builtins, LENGTH_OF(py_builtins))
    );
}