#include "objects.h"
#include "functions.h"
#include "classes.h"
#include "rtl/stringop.h"
#include "rtl/safety.h"
#include "sys/core.h"

// The following represent intrinsic types - ones that we know about and that hold
// C data that represent them internally. User-defined types use these types to
// define their own structures. We treat them specially.
//
// Do note that these *are* built-ins - for all other builtin types, see `builtins.h` and
// `builtins.c`. A type should be here instead of those two files if we have a special
// `as_<...>` field in the `pyobj_t` union.

CLASS(bool)
    static const pyobj_t py_strliteral_true = PY_STR_LITERAL("True");
    static const pyobj_t py_strliteral_false = PY_STR_LITERAL("False");

    CLASS_METHOD(_bool, __str__) {
        py_verify_self_arg(self, &py_type_bool);
        return self->as_bool ? &py_strliteral_true : &py_strliteral_false;
    };

    CLASS_ATTRIBUTES(_bool) {
        // TODO: methods for bool
        HAS_CLASS_METHOD(_bool, __str__)
    };
DEFINE_INTRINSIC_TYPE(_bool);

CLASS(float)
    CLASS_ATTRIBUTES(_float) {
        // TODO: methods for float
    };
DEFINE_INTRINSIC_TYPE(_float);

CLASS(int)
    CLASS_ATTRIBUTES(_int) {
        // TODO: methods for int
    };
DEFINE_INTRINSIC_TYPE(_int);

CLASS(str)
    static const pyobj_t py_strliteral_unknown = PY_STR_LITERAL("<object>");

    CLASS_METHOD(_str, __str__) {
        py_verify_self_arg(self, &py_type_str);
        return self;
    };

    CLASS_METHOD(_str, __new__) {
        pyobj_t* cls = self;
        
        if (argc != 1)
            sys_panic("Expected exactly one argument to str(...).");

        pyobj_t* value = argv[0];
        pyobj_t* method_str = py_get_attribute(value, "__str__");
        if (method_str == NULL)
            return &py_strliteral_unknown;

        return py_call(method_str, value, 0, NULL, 0, NULL);
    };

    CLASS_ATTRIBUTES(_str) {
        HAS_CLASS_METHOD(_str, __str__),
        HAS_CLASS_METHOD(_str, __new__)
    };
DEFINE_INTRINSIC_TYPE(_str);

CLASS(tuple)
    CLASS_ATTRIBUTES(_tuple) {
        // TODO: methods for tuple
    };
DEFINE_INTRINSIC_TYPE(_tuple);

CLASS(list)
    CLASS_ATTRIBUTES(_list) {
        // TODO: methods for list
    };
DEFINE_INTRINSIC_TYPE(_list);

CLASS(type)
    CLASS_METHOD(_type, __new__) {
        pyobj_t* cls = self;

        // If this is called, that would mean that we're using the default implementation
        // of __new__. We create an empty object in this case.
        pyobj_t* obj = py_alloc_object(cls);
        return obj;
    };

    CLASS_METHOD(_type, __init__) {
        // The default implementation of __init__ is a no-op.
        return &py_none;
    };

    CLASS_METHOD(_type, __call__) {
        ENSURE_NOT_NULL(self, "type.__call__");

        // A call to a `type` object creates a new object of the given type to be
        // created. For example, if we did `class A: pass`, then `A()`, we'd be calling
        // the `type` object `A`.
        //
        // We first resolve the attribute __new__ on our type object - it might've been
        // overriden by the class. In most cases, we'll invoke the default __new__ implementation
        // on `type`. That'll gives an uninitialized empty object.
        pyobj_t* method_new = py_get_attribute(self, "__new__");
        ENSURE_NOT_NULL(method_new, "type.__call__");

        // __new__ is a class method, not an instance method. First argument is the class.
        pyobj_t* obj = py_call(method_new, self, argc, argv, kwargc, kwargv);
        ENSURE_NOT_NULL(obj, "type.__call__"); // e.g. &py_none would be fine, but NULL means something's wrong
        
        // If __new__() does not return an instance of cls, then the new instance's __init__() method will not be invoked.
        if (obj->type == self) {
            pyobj_t* method_init = py_get_attribute(obj, "__init__");
            ENSURE_NOT_NULL(method_init, "type.__call__");

            // We forward the arguments we got to __init__. So, if we get invoked with `A(a, b, c)`,
            // we'd do A.__init__(obj, a, b, c).
            py_call(method_init, obj, argc, argv, kwargc, kwargv);
        }

        return obj;
    };

    CLASS_ATTRIBUTES(_type) {
        HAS_CLASS_METHOD(_type, __new__),
        HAS_CLASS_METHOD(_type, __init__),
        HAS_CLASS_METHOD(_type, __call__)
    };
DEFINE_INTRINSIC_TYPE(_type);

CLASS(callable)
    CLASS_ATTRIBUTES(_callable) {
        // TODO: methods for callable
    };
DEFINE_INTRINSIC_TYPE(_callable);

CLASS(NoneType)
    CLASS_ATTRIBUTES(_nonetype) {
        // TODO: methods for nonetype
    };
DEFINE_BUILTIN_TYPE(_nonetype);

const pyobj_t py_none = { .type = &py_type_nonetype };
const pyobj_t py_true = { .type = &py_type_bool, .as_bool = true };
const pyobj_t py_false = { .type = &py_type_bool, .as_bool = false };

pyobj_t* py_get_attribute(pyobj_t* target, const char* name) {
    ENSURE_NOT_NULL(name, "py_get_attribute");
    ENSURE_NOT_NULL(target, "py_get_attribute");

    // First, try to find the attribute in the per-object attribute table if it's not
    // intrinsic. If it *is* intrinsic, we don't even hold an attribute table in the first
    // place. For example, `int`s are intrinsic, as they hold a single integer value, not
    // attributes - you cannot do `123.a = 2`.
    if (!target->type->as_type.is_intrinsic) {
        vector_t(symbol_t)* attributes = &target->as_any;
        for (size_t i = 0; i < attributes->length; i++) {
            symbol_t attribute = attributes->elements[i];

            if (rtl_strequ(attribute.name, name)) {
                return attribute.value;
            }
        }
    }

    // If we didn't find anything, AND this is not an object representing a type, try
    // searching in the class attribute table. For example, if we were to do something like:
    //      class A:
    //          abc = 123
    // ...then `A().abc` would be the same as `A.abc`. The type of `A()` would be set
    // to `A` - the object representing class `A`.
    if (target->type != &py_type_type) {
        return py_get_attribute(target->type, name);
    }
    else {
        // If this *is* an object representing a type, we still might have a base class
        // that has more attributes.
        //
        // For example, if we were to do:
        //      class A:
        //          abc = 123
        //
        //      class B(A):
        //          pass
        // ...then doing `B.abc` would be equivalennt to `A.abc` in this case.
        // It follows that `B().abc` would also be equivalent to `A.abc`.
        if (target->type->as_type.base != NULL) {
            return py_get_attribute(target->type->as_type.base, name);
        }
    }

    return NULL;
}

void py_set_attribute(pyobj_t* target, const char* name, pyobj_t* value) {
    ENSURE_NOT_NULL(name, "py_set_attribute");
    ENSURE_NOT_NULL(target, "py_set_attribute");
    ENSURE_NOT_NULL(value, "py_set_attribute");

    if (target->type->as_type.is_intrinsic)
        sys_panic("The given object is of an immutable type, and cannot be assigned to.");

    vector_t(symbol_t)* attributes = &target->as_any;
    for (size_t i = 0; i < attributes->length; i++) {
        symbol_t* attribute = &attributes->elements[i];

        if (rtl_strequ(attribute->name, name)) {
            attribute->value = value;
            return;
        }
    }

    // No such attribute was defined before, so we add one.
    rtl_vector_append(attributes, ((symbol_t){ .name = name, .value = value }));
} 

pyobj_t* py_alloc_int(int64_t x) {
    pyobj_t* obj = mm_heap_alloc(sizeof(pyobj_t));
    obj->type = &py_type_int;
    obj->as_int = x;
    return obj;
}

pyobj_t* py_alloc_object(pyobj_t* type) {
    ENSURE_NOT_NULL(type, "py_alloc_object");
    if (type->type != &py_type_type) {
        // This might be a bit confusing - basically, we're checking that the type object
        // actually represents a type. This panic would be triggered if someone e.g.
        // would invoke `__new__` with the `cls` parameter being an `int`.
        sys_panic("Attempted to allocate an object with a type object that is not a 'type'.");
    }

    pyobj_t* obj = mm_heap_alloc(sizeof(pyobj_t));
    obj->type = type;
    obj->as_any = (vector_t(symbol_t)) {}; // we start out with no attributes
    return obj;
}

pyobj_t* py_call(
    pyobj_t* target,
    pyobj_t* self,
    int argc,
    pyobj_t** argv,
    int kwargc,
    symbol_t* kwargv
) {
    ENSURE_NOT_NULL(target, "py_call");

    if (target->type == &py_type_callable) {
        // This would be either 'function', 'method', or 'builtin_function_or_method' in CPython.
        return target->as_callable(self, argc, argv, kwargc, kwargv);
    }

    // If the object WASN'T callable, then it might have a `__call__` attribute, which should
    // be a method, which in turn would be a callable.
    pyobj_t* call_attr = py_get_attribute(target, "__call__");
    if (call_attr != NULL && call_attr->type == &py_type_callable) {
        return call_attr->as_callable(self, argc, argv, kwargc, kwargv);
    }

    // TODO: Raise a `TypeError: '...' object is not callable` exception instead of panicking here.
    sys_panic("attempted to call a non-callable object");
    return NULL;
}
