#include "exceptions.h"

#include "objects.h"
#include "classes.h"
#include "rtl/safety.h"

#define DEFINE_EXCEPTION($name, $derives_from)                    \
    CLASS($name)                                                  \
        CLASS_ATTRIBUTES($name)                                   \
        END_CLASS_ATTRIBUTES;                                     \
    DEFINE_BUILTIN_TYPE($name, &py_type_##$derives_from)          \

CLASS(BaseException)
    CLASS_METHOD(BaseException, __init__) {
        if (argc != 1) {
            RAISE(Exception, "exceptions accept only one argument");
        }

        py_set_attribute(self, "msg", argv[0]);
        return WITH_RESULT(&py_none);
    };

    CLASS_METHOD(BaseException, __str__) {
        ENSURE_NOT_NULL(self, "BaseException.__str__");

        

        return WITH_RESULT(py_get_attribute(self, "msg"));
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

