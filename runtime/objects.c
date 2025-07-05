#include "objects.h"
#include "functions.h"
#include "classes.h"
#include "rtl/stringop.h"
#include "rtl/safety.h"
#include "sys/core.h"
#include "sys/terminal.h"

// The following represent intrinsic types - ones that we know about and that hold
// C data that represent them internally. User-defined types use these types to
// define their own structures. We treat them specially.
//
// Do note that these *are* built-ins - for all other builtin types, see `builtins.h` and
// `builtins.c`. A type should be here instead of those two files if we have a special
// `as_<...>` field in the `pyobj_t` union.

CLASS(object)
    static pyobj_t py_strliteral_object_unknown = PY_STR_LITERAL("<unknown object>");

    // def __str__(self):
    CLASS_METHOD(object, __str__) {
        ENSURE_NOT_NULL(self, "object.__str__");

        pyobj_t* name = py_get_attribute(self, "__name__");
        if (name->type != &py_type_str)
            return WITH_RESULT(&py_strliteral_object_unknown);

        char* first = rtl_strconcat("<", name->as_str);
        char* full = rtl_strconcat(first, "object>");
        mm_heap_free(first);
        
        pyobj_t* obj = mm_heap_alloc(sizeof(pyobj_t));
        obj->type = &py_type_str;
        obj->as_str = full;
        return WITH_RESULT(obj);
    }

    CLASS_ATTRIBUTES(object)
        HAS_CLASS_METHOD(object, __str__)
    END_CLASS_ATTRIBUTES;
DEFINE_INTRINSIC_TYPE(object);

CLASS(bool)
    static pyobj_t py_strliteral_true = PY_STR_LITERAL("True");
    static pyobj_t py_strliteral_false = PY_STR_LITERAL("False");

    // def __str__(self):
    CLASS_METHOD_E(_bool, __str__) {
        py_verify_self_arg(self, &py_type_bool);
        return WITH_RESULT(self->as_bool ? &py_strliteral_true : &py_strliteral_false);
    };

    CLASS_ATTRIBUTES_E(_bool, "bool")
        // TODO: methods for bool
        HAS_CLASS_METHOD_E(_bool, __str__)
    END_CLASS_ATTRIBUTES;
DEFINE_INTRINSIC_TYPE_E(_bool);

CLASS(float)
    CLASS_ATTRIBUTES_E(_float, "float")
        // TODO: methods for float
    END_CLASS_ATTRIBUTES;
DEFINE_INTRINSIC_TYPE_E(_float);

CLASS(int)
    CLASS_ATTRIBUTES_E(_int, "int")
        // TODO: methods for int
    END_CLASS_ATTRIBUTES;
DEFINE_INTRINSIC_TYPE_E(_int);

CLASS(str)
    static pyobj_t py_strliteral_str_unknown = PY_STR_LITERAL("<object>");

    // def __str__(self):
    CLASS_METHOD(str, __str__) {
        py_verify_self_arg(self, &py_type_str);
        return WITH_RESULT(self);
    };

    // def __new__(cls, value):
    CLASS_METHOD(str, __new__) {
        pyobj_t* cls = self;
        
        if (argc != 1)
            sys_panic("Expected exactly one argument to str(...).");

        pyobj_t* value = argv[0];
        pyobj_t* method_str = py_get_attribute(value, "__str__");
        if (method_str == NULL || method_str->type != &py_type_method)
            return WITH_RESULT(&py_strliteral_str_unknown);

        return py_call(method_str, 0, NULL, 0, NULL);
    };

    CLASS_ATTRIBUTES(str)
        HAS_CLASS_METHOD(str, __str__),
        HAS_CLASS_METHOD(str, __new__)
    END_CLASS_ATTRIBUTES;
DEFINE_INTRINSIC_TYPE(str);

CLASS(tuple)
    CLASS_ATTRIBUTES(tuple)
        // TODO: methods for tuple
    END_CLASS_ATTRIBUTES;
DEFINE_INTRINSIC_TYPE(tuple);

CLASS(list)
    CLASS_ATTRIBUTES(list)
        // TODO: methods for list
    END_CLASS_ATTRIBUTES;
DEFINE_INTRINSIC_TYPE(list);

CLASS(type)
    // def __new__(cls):
    CLASS_METHOD(type, __new__) {
        pyobj_t* cls = self;

        // If this is called, that would mean that we're using the default implementation
        // of __new__. We create an empty object in this case.
        pyobj_t* obj = py_alloc_object(cls);
        return WITH_RESULT(obj);
    };

    // def __init__(...):
    CLASS_METHOD(type, __init__) {
        // The default implementation of __init__ is a no-op.
        return WITH_RESULT(&py_none);
    };

    CLASS_METHOD(type, __call__) {
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
        const pyobj_t* obj = UNWRAP(py_call(method_new, argc, argv, kwargc, kwargv));
        ENSURE_NOT_NULL(obj, "type.__call__"); // e.g. &py_none would be fine, but NULL means something's wrong
        
        // If __new__() does not return an instance of cls, then the new instance's __init__() method will not be invoked.
        if (obj->type == self) {
            pyobj_t* method_init = py_get_attribute(obj, "__init__");
            ENSURE_NOT_NULL(method_init, "type.__call__");

            if (method_init->type == &py_type_method) {
                // We forward the arguments we got to __init__. So, if we get invoked with `A(a, b, c)`,
                // we'd do A.__init__(obj, a, b, c).
                py_call(method_init, argc, argv, kwargc, kwargv);
            }
        }

        return WITH_RESULT(obj);
    };

    CLASS_ATTRIBUTES(type)
        HAS_CLASS_METHOD(type, __new__),
        HAS_CLASS_METHOD(type, __init__),
        HAS_CLASS_METHOD(type, __call__)
    END_CLASS_ATTRIBUTES;
DEFINE_INTRINSIC_TYPE(type);

CLASS(method)
    CLASS_ATTRIBUTES(method)

    END_CLASS_ATTRIBUTES;
DEFINE_INTRINSIC_TYPE(method);

CLASS(function)
    // def __get__(self, instance, owner)
    CLASS_METHOD(function, __get__) {
        // __get__ on 'function' objects will bind them to 'instance'. 'owner' is ignored.
        ENSURE_NOT_NULL(self, "function.__get__");
        
        if (self->type != &py_type_function) {
            RAISE(TypeError, "expected a function as 'instance' in function.__get__");
        }

        if (argc == 0) {
            RAISE(TypeError, "expected an 'instance' argument for function.__get__");
        }

        if (argc > 2) {
            RAISE(TypeError, "too many arguments for function.__get__");
        }

        pyobj_t* instance = argv[0];
        ENSURE_NOT_NULL(instance, "function.__get__");

        return WITH_RESULT(py_alloc_method(self->as_function, instance));
    };

    CLASS_ATTRIBUTES(function)
        HAS_CLASS_METHOD(function, __get__)
    END_CLASS_ATTRIBUTES;
DEFINE_INTRINSIC_TYPE(function);

CLASS(NoneType)
    CLASS_ATTRIBUTES(NoneType)
        // TODO: methods for nonetype
    END_CLASS_ATTRIBUTES;
DEFINE_BUILTIN_TYPE(NoneType, NULL);

const pyobj_t py_none = { .type = &py_type_NoneType };
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
        const vector_t(symbol_t)* attributes = &target->as_any;

        for (size_t i = 0; i < attributes->length; i++) {
            symbol_t attribute = attributes->elements[i];

            if (rtl_strequ(attribute.name, name)) {
                // We're *not* calling __get__ if we've retrieved the attribute from
                // what would be __dict__ in regular Python. For example:
                //      o.example = Always123()     # ...where 'o' is an object...
                //      print(o.example)            # will not invoke Always123.__get__
                // This is not the case with classes, which is a bit inconsistent.
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
    //
    // We search all class attribute tables, going down the inheritance chain.
    // Do note that if the object itself is a 'type', we begin from *the object itself*,
    // not its type.
    //
    // For example, if 'A' is a class, if we did:
    //      A.example = Always123()
    //      print(A.example)
    // Always123.__get__ WOULD get called, as opposed to if A would be a regular object.
    pyobj_t* current_base =
        (target->type == &py_type_type)
            ? target // target is a type itself
            : target->type; // target is any other object

    while (current_base != NULL) {
        const vector_t(symbol_t)* attributes = &current_base->as_type.class_attributes;
        for (size_t i = 0; i < attributes->length; i++) {
            symbol_t attribute = attributes->elements[i];

            if (rtl_strequ(attribute.name, name)) {
                pyobj_t* attr = attribute.value;

                // If 'attr' has a '__get__' method, we invoke it. This implements
                // descriptors. For an actual descriptor, this would result in the
                // following call graph:
                //
                //                          owner.attr
                //                               |
                //                     attr.__get__(owner, O)
                //                               |
                //                 attr.__get__.__get__(attr, D)
                //
                // (O = owner class, D = descriptor class)
                //
                // ...after which point we'd be invoking the '__get__' method for
                // a 'function', which binds the function to the given instance, 
                // returning a 'method'.
                
                pyobj_t* val = attr;

                pyobj_t* get = py_get_attribute(attr, "__get__");
                if (get != NULL && get->type == &py_type_method) {
                    pyobj_t* args[] = {
                        target,             // instance
                        current_base        // owner
                    };

                    pyreturn_t result = py_call(get, 2, args, 0, NULL);
                    if (result.exception != NULL) {
                        // TODO: Raise exceptions thrown by __get__ when getting attributes
                        sys_panic("Exception was thrown while invoking __get__");
                    }

                    val = result.value;
                }

                return val;
            }
        }

        current_base = current_base->as_type.base;
    }

    return NULL;
}

void py_set_attribute(pyobj_t* target, const char* name, pyobj_t* value) {
    ENSURE_NOT_NULL(name, "py_set_attribute");
    ENSURE_NOT_NULL(target, "py_set_attribute");
    ENSURE_NOT_NULL(value, "py_set_attribute");

    if (target->type->as_type.is_intrinsic && target->type != &py_type_type)
        sys_panic("The given object is of an immutable type, and cannot be assigned to.");

    vector_t(symbol_t)* attributes;
    if (target->type == &py_type_type) {
        // Special case for 'type' - we can do something like this:
        //      class C:
        //          pass
        //      C.attr = 123
        //      print(C.attr)   # prints "123"
        // ...thus assigning to a 'type' object is equivalent to assigning to
        // its class attribute table.
        attributes = &target->as_type.class_attributes;
    }
    else {
        attributes = &target->as_any;
    }

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

pyobj_t* py_alloc_function(py_fnptr_callable_t callable) {
    pyobj_t* obj = mm_heap_alloc(sizeof(pyobj_t));
    obj->type = &py_type_function;
    obj->as_function = callable;
    return obj;
}

pyobj_t* py_alloc_method(py_fnptr_callable_t callable, pyobj_t* bound) {
    pyobj_t* obj = mm_heap_alloc(sizeof(pyobj_t));
    obj->type = &py_type_method;
    obj->as_method.body = callable;
    obj->as_method.bound = bound;
    return obj;
}

pyobj_t* py_alloc_type(pyobj_t* base) {
    pyobj_t* obj = mm_heap_alloc(sizeof(pyobj_t));
    obj->type = &py_type_type;
    obj->as_type.base = base;
    obj->as_type.is_intrinsic = false;
    obj->as_type.class_attributes = (vector_t(symbol_t)) {};
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

pyreturn_t py_call(
    pyobj_t* target,
    int argc,
    pyobj_t** argv,
    int kwargc,
    symbol_t* kwargv
) {
    ENSURE_NOT_NULL(target, "py_call");

    if (target->type == &py_type_function) {
        return target->as_function(NULL, argc, argv, kwargc, kwargv);
    }

    if (target->type == &py_type_method) {
        return target->as_method.body(
            target->as_method.bound,
            argc, argv,
            kwargc, kwargv
        );
    }

    // If the object WASN'T a method or a function, then it might have a `__call__`
    // attribute, which should be a method.
    pyobj_t* call_attr = py_get_attribute(target, "__call__");
    if (call_attr != NULL && call_attr->type == &py_type_method)
        return py_call(call_attr, argc, argv, kwargc, kwargv);

    RAISE(TypeError, "attempted to call a non-callable object");
}

const char* py_stringify(pyobj_t* target) {
    if (target == NULL)
        return "<NULL>";

    if (target == &py_none)
        return "None";

    pyobj_t* method_str = py_get_attribute(target, "__str__");
    if (method_str == NULL)
        return "(unknown object)";

    pyreturn_t converted = py_call(method_str, 0, NULL, 0, NULL);
    if (converted.exception != NULL)
        return "<error while stringifying>";

    ENSURE_NOT_NULL(converted.value, "py_stringify");

    if (converted.value->type != &py_type_str)
        return py_stringify(converted.value);

    return converted.value->as_str;
}

bool py_isinstance(pyobj_t* target, pyobj_t* type) {
    ENSURE_NOT_NULL(target, "py_isinstance");
    ENSURE_NOT_NULL(type, "py_isinstance");

    pyobj_t* current = target->type;
    while (current != NULL) {
        ASSERT(current->type == &py_type_type, "py_isinstance");

        if (current == type)
            return true;

        current = current->as_type.base;
    }

    return false;
}