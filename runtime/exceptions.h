#pragma once

#include "objects.h"
#include "symbols.h"

// Creates a `pyreturn_t` value that represents a raised exception.
#define WITH_EXCEPTION($exc) ((pyreturn_t) { .exception = ($exc) } )

// Creates a `pyreturn_t` value that represents a returned value.
#define WITH_RESULT($val) ((pyreturn_t) { .value = ($val), .exception = NULL } )

// Unwraps the given `pyreturn_t` to its object representation. If the `pyreturn_t`
// value represents an exception, the calling function is terminated early, returning
// the exception instead.
#define UNWRAP($result)                                    \
    ({                                                     \
        __typeof__(($result)) _tmp = ($result);            \
        if (_tmp.exception != NULL)                        \
            return WITH_EXCEPTION(_tmp.exception);         \
        _tmp.value;                                        \
    })

// Creates a new exception of type `$type`, providing the argument `$msg`. Within
// Python, this would be equivalent to `$type($msg)`. `$msg` usually is a Python
// string literal.
#define NEW_EXCEPTION($type, $msg)                          \
    ({                                                      \
        pyobj_t* args[] = { ($msg) };                       \
        py_call(($type), 1, args, 0, NULL, NULL).value;     \
    })

// Similar to `NEW_EXCEPTION`, but also defines a string literal object as
// a `static const` symbol.
#define NEW_EXCEPTION_INLINE($type, $msg)                                               \
    ({                                                                                  \
        static pyobj_t MACRO_CONCAT(exc_literal_l, __LINE__) = PY_STR_LITERAL($msg);    \
        NEW_EXCEPTION(&py_type_##$type, &MACRO_CONCAT(exc_literal_l, __LINE__));        \
    })

// NOLINTBEGIN(clang-diagnostic-incompatible-pointer-types-discards-qualifiers)

// Unconditionally raise an exception and terminate the function's execution. This macro
// should not be used by transpiled code.
#define RAISE($type, $msg) \
    {                                                                                                   \
        static pyobj_t MACRO_CONCAT(exc_literal_l, __LINE__) = PY_STR_LITERAL($msg);                    \
        return WITH_EXCEPTION(NEW_EXCEPTION(&py_type_##$type, &MACRO_CONCAT(exc_literal_l, __LINE__))); \
    }  

// NOLINTEND(clang-diagnostic-incompatible-pointer-types-discards-qualifiers)

// Raise an exception that may be caught. This macro should only be used by transpiled code.
#define RAISE_CATCHABLE($obj, $depth, $lasti)                                               \
    caught_exception = py_coerce_exception($obj);                                           \
    stack_current = ($depth) - 1;                                                           \
    if (($lasti) != -1) {                                                                   \
        static pyobj_t _lasti_py = { .type = &py_type_int, .as_int = ($lasti) };            \
        stack[++stack_current] = &_lasti_py;                                                \
    }                                                                                       \
    stack[++stack_current] = caught_exception;                                              \
    goto PY__EXCEPTION_HANDLER_LABEL;                                                       \

#define PY_GLOBAL_BaseException_WELLKNOWN
extern pyobj_t py_type_BaseException;
extern pyobj_t* KNOWN_GLOBAL(BaseException);

#define PY_GLOBAL_Exception_WELLKNOWN
extern pyobj_t py_type_Exception;
extern pyobj_t* KNOWN_GLOBAL(Exception);

#define PY_GLOBAL_StopIteration_WELLKNOWN
extern pyobj_t py_type_StopIteration;
extern pyobj_t* KNOWN_GLOBAL(StopIteration);

#define PY_GLOBAL_TypeError_WELLKNOWN
extern pyobj_t py_type_TypeError;
extern pyobj_t* KNOWN_GLOBAL(TypeError);

// Coerces an object to an exception when raising. It accepts objects of the
// following types:
//      - any subtype of `BaseException` or `BaseException` itself. In this case,
//        `from` is returned.
//      - a `type`. The type represented by the object must be assignable to
//        `BaseException` (i.e. one of its `as_type->base`'s must be `&py_type_BaseException`
//        or it needs to be a `&py_type_BaseException` itself).
//
// If the object does not meet any of the above criteria, a `TypeError` is returned
// instead informing the user of an invalid exception type.
pyobj_t* py_coerce_exception(pyobj_t* from);
