#include "builtins.h"

#include "sys/core.h"
#include "sys/terminal.h"
#include "classes.h"

// Returns the name of the callable object that wraps over the function `$fn`.
#define FUNCTION_WRAPPER($fn) $fn##_object

// Defines a fixed callable object that wraps over the function `$fn`. To reference
// this object, use `FUNCTION_WRAPPER($fn)`.
#define DEFINE_FUNCTION_WRAPPER($fn, $global_name)                       \
    static const pyobj_t FUNCTION_WRAPPER($fn) = {                       \
        .type = &py_type_callable,                                       \
        .as_callable = &$fn                                              \
    };                                                                   \
    const pyobj_t* KNOWN_GLOBAL($global_name) = &FUNCTION_WRAPPER($fn);  \

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

DEFINE_TYPE(bytearray, false);
CLASS_ATTRIBUTES_BEGIN(bytearray)
    // TODO: methods for bytearray
CLASS_ATTRIBUTES_END(bytearray)


