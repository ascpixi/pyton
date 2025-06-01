#include "builtins.h"

#include "sys/core.h"
#include "sys/terminal.h"
#include "rtl/safety.h"
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
    CLASS_ATTRIBUTES(_bytearray) {
        // TODO: methods for bytearray
    };
DEFINE_BUILTIN_TYPE(_bytearray);

CLASS(range_iterator)
    CLASS_METHOD(_range_iterator, __iter__) {
        return self;
    };

    CLASS_METHOD(_range_iterator, __init__) {
        if (argc != 1)
            sys_panic("Expected only one argument to range_iterator.");

        pyobj_t* arg = argv[0];
        if (arg->type != &py_type_range)
            sys_panic("The first argument of 'range_iterator' has to be an instance of 'range'.");
    
        py_set_attribute(self, "start", py_get_attribute(arg, "start"));
        py_set_attribute(self, "stop", py_get_attribute(arg, "stop"));
        py_set_attribute(self, "step", py_get_attribute(arg, "step"));

        return &py_none;
    };

    CLASS_METHOD(_range_iterator, __next__) {

    };

    CLASS_ATTRIBUTES(_range_iterator) {
        HAS_CLASS_METHOD(_range_iterator, __init__),
        HAS_CLASS_METHOD(_range_iterator, __iter__)
    };
DEFINE_BUILTIN_TYPE(_range_iterator);

CLASS(range)
    const pyobj_t py_range_default_start = PY_INT_CONSTANT(0);
    const pyobj_t py_range_default_step = PY_INT_CONSTANT(1);

    CLASS_METHOD(_range, __init__) {
        ENSURE_NOT_NULL(self, "range.__init__");

        if (argc == 0)
            sys_panic("range expected at least 1 argument, got 0");

        if (argc > 3)
            sys_panic("range expected at most 3 arguments");

        if (argc == 1) {
            // range(stop)
            py_set_attribute(self, "start", &py_range_default_start);
            py_set_attribute(self, "stop", argv[0]);
            py_set_attribute(self, "step", &py_range_default_step);
        }
        else if (argc == 2) {
            // range(start, stop)
            py_set_attribute(self, "start", argv[0]);
            py_set_attribute(self, "stop", argv[1]);
            py_set_attribute(self, "step", &py_range_default_step);
        }
        else {
            // range(start, stop, step)
            py_set_attribute(self, "start", argv[0]);
            py_set_attribute(self, "stop", argv[1]);
            py_set_attribute(self, "step", argv[2]);
        }

        return &py_none;
    };

    CLASS_METHOD(_range, __iter__) {
        ENSURE_NOT_NULL(self, "range.__iter__");

        // def __iter__(self):
        //     return range_iterator(self)

        pyobj_t* args[] = { self };
        return py_call(
            &py_type_range_iterator,
            &py_type_range_iterator,
            1, args,
            0, NULL
        );
    };

    CLASS_ATTRIBUTES(_range) {
        HAS_CLASS_METHOD(_range, __init__),
        HAS_CLASS_METHOD(_range, __iter__)
    };
DEFINE_BUILTIN_TYPE(_range);

