#include "exceptions.h"

#include "objects.h"
#include "classes.h"
#include "std/safety.h"

#define DEFINE_EXCEPTION($name, $derives_from)                    \
    CLASS($name)                                                  \
        CLASS_ATTRIBUTES($name)                                   \
        END_CLASS_ATTRIBUTES;                                     \
    DEFINE_BUILTIN_TYPE($name, &py_type_##$derives_from)          \

CLASS(BaseException)
    CLASS_METHOD(BaseException, __init__) {
        if (argc > 1)
            RAISE(Exception, "exceptions accept at most one argument");

        if (argc == 1) {
            py_set_attribute(NOT_NULL(self), "msg", NOT_NULL(argv)[0]);
        }

        return WITH_RESULT(&py_none);
    };

    CLASS_METHOD(BaseException, __str__) {
        ENSURE_NOT_NULL(self);

        pyobj_t* msg = py_get_attribute(self, "msg");

        if (msg == NULL) {
            // We might not have a message, e.g. `raise StopIteration()`
            msg = NOT_NULL(py_get_attribute(self, "__name__"));
        }

        return WITH_RESULT(msg);
    };

    CLASS_ATTRIBUTES(BaseException)
        HAS_CLASS_METHOD(BaseException, __init__),
        HAS_CLASS_METHOD(BaseException, __str__)
    END_CLASS_ATTRIBUTES;
DEFINE_BUILTIN_TYPE(BaseException, &py_type_object);

DEFINE_EXCEPTION(Exception, BaseException);
// DEFINE_EXCEPTION(ArithmeticError, Exception);
// DEFINE_EXCEPTION(AssertionError, Exception);
// DEFINE_EXCEPTION(AttributeError, Exception);
// DEFINE_EXCEPTION(BufferError, Exception);
// DEFINE_EXCEPTION(EOFError, Exception);
// DEFINE_EXCEPTION(ImportError, Exception);
// DEFINE_EXCEPTION(LookupError, Exception);
// DEFINE_EXCEPTION(MemoryError, Exception);
// DEFINE_EXCEPTION(NameError, Exception);
// DEFINE_EXCEPTION(OSError, Exception);
// DEFINE_EXCEPTION(ReferenceError, Exception);
// DEFINE_EXCEPTION(RuntimeError, Exception);
// DEFINE_EXCEPTION(StopAsyncIteration, Exception);
DEFINE_EXCEPTION(StopIteration, Exception);
// DEFINE_EXCEPTION(SyntaxError, Exception);
// DEFINE_EXCEPTION(SystemError, Exception);
DEFINE_EXCEPTION(TypeError, Exception);
// DEFINE_EXCEPTION(ValueError, Exception);

pyobj_t* py_coerce_exception(pyobj_t* from) {
    if (from->type == &py_type_type) {
        const pyobj_t* current = from;
        while (current != NULL) {
            ASSERT(current->type == &py_type_type);

            if (current == &py_type_BaseException) {
                // We've received a `type` that represents a class that derives from
                // `BaseException`. This can happen when doing the following:
                //      raise StopIteration
                // ...we simply call the type with no arguments.
                pyreturn_t result = py_call(from, 0, NULL, 0, NULL, NULL);
                if (result.exception != NULL)
                    return result.exception;

                return NOT_NULL(result.value);
            }

            current = current->as_type.base;
        }

        // This is a `type`, but it doesn't inherit from `BaseException`!
        goto invalid;
    }

    if (py_isinstance(from, &py_type_BaseException)) {
        // Regular ol' exception. No need to do anything fancy.
        return from;
    }

invalid:
    return NEW_EXCEPTION_INLINE(TypeError, "exceptions must derive from BaseException");
}