#include "builtins.h"

#include "sys/core.h"
#include "sys/terminal.h"
#include "std/safety.h"
#include "classes.h"
#include "exceptions.h"

DEFINE_FUNCTION_WRAPPER(py_builtin_build_class, __build_class__);
PY_DEFINE(py_builtin_build_class) {
    //   class C(A, B, metaclass=M, other=42, *more_bases, *more_kwds):
    //      ...
    // translates into:
    //   C = __build_class__(<func>, 'C', A, B, metaclass=M, other=42, *more_bases, *more_kwds)

    if (argc < 2)
        RAISE(TypeError, "__build_class__ accepts at least two arguments");

    if (argc > 3)
        RAISE(TypeError, "multiple inheritence is not yet supported");

    ENSURE_NOT_NULL(argv);

    pyobj_t* body = argv[0]; // func
    pyobj_t* name = argv[1]; // name
    
    pyobj_t* base_class = argc == 3  ? argv[2] : &py_type_object;
    
    ENSURE_NOT_NULL(body);
    ENSURE_NOT_NULL(name);

    if (body->type != &py_type_function)
        RAISE(TypeError, "__build_class__: func must be a function");

    if (name->type != &py_type_str)
        RAISE(TypeError, "__build_class__: name must be a string");

    // Class bodies are special-cased by the transpiler. All locals are actually
    // assigned to the class attribute table, e.g. for a class C:
    //      LOAD_CONST               2 (<code object __init__ at 0x7f8c7028b2f0, file "<dis>", line 2>)
    //      MAKE_FUNCTION
    //      STORE_NAME               4 (__init__)
    // ...would function as 'C.__init__ = <fn for code object>`. The transpiler knows
    // about this, and all functions passed into `__build_class__` will expect
    // a hidden parameter that will serve as the object to assign locals as attributes to.

    pyobj_t* type = py_alloc_type(base_class);
    pyreturn_t result = py_call(body, 0, NULL, 0, NULL, type); // 'type' is 'self' here, the hidden parameter
    
    // We don't really care about the return value of the class body. It's usually None.
    if (result.exception != NULL)
        return result;

    return WITH_RESULT(type);
}

DEFINE_FUNCTION_WRAPPER(py_builtin_print, print);
PY_DEFINE(py_builtin_print) {
    // Do note, we only expect one string argument - but the actual print() function
    // should be able to accept any kind of argument
    // TODO: Fully featured print()

    if (argc == 0) {
        terminal_newline();
        return WITH_RESULT(&py_none);
    }

    if (argc != 1) {
        // TODO: print() can accept any number of arguments.
        sys_panic("More than one argument is not yet supported for print().");
    }

    pyobj_t* value = NOT_NULL(argv)[0];
    if (value->type != &py_type_str) {
        // TODO: Any kind of object should be accepted here.
        sys_panic("Expected a 'str' argument for print().");
    }

    terminal_println(value->as_str.str);
    return WITH_RESULT(&py_none);
}
