#include "builtins.h"

#include "sys/core.h"
#include "sys/terminal.h"
#include "classes.h"

DEFINE_FUNCTION_WRAPPER(py_builtin_print, print);

PY_DEFINE(py_builtin_print) {
    // Do note, we only expect one string argument - but the actual print() function
    // should be able to accept any kind of argument
    // TODO: Fully featured print()

    if (argc == 0) {
        terminal_newline();
        return &py_none;
    }

    if (argc != 1) {
        // TODO: print() can accept any number of arguments.
        sys_panic("More than one argument is not yet supported for print().");
    }

    pyobj_t* value = argv[0];
    if (value->type != &py_type_str) {
        // TODO: Any kind of object should be accepted here.
        sys_panic("Expected a 'str' argument for print().");
    }

    terminal_println(value->as_str);
    return &py_none;
}

CLASS(bytearray)
    CLASS_ATTRIBUTES(type_bytearray) {
        // TODO: methods for bytearray
    };
DEFINE_TYPE(type_bytearray, false);


