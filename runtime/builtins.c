#include "builtins.h"

const symbol_t py_builtins[] = {
    { .name = "print", .address = &py_builtin_print }
};

PY_DEFINE(py_builtin_print) {
    // Do note, we only expect one string argument - but the actual print() function
    // should be able to accept any kind of argument
    // TODO: Fully featured print()

    if (argc == 0) {
        // New-line
        
    }
}
