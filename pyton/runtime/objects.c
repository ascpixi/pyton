#include "objects.h"

#include "functions.h"
#include "classes.h"
#include "std/string.h"
#include "std/stringop.h"
#include "std/safety.h"
#include "std/tuple.h"
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
    // def __new__(cls):
    CLASS_METHOD(object, __new__) {
        pyobj_t* cls = NOT_NULL(self);

        // If this is called, that would mean that we're using the default implementation
        // of __new__. We create an empty object in this case.
        pyobj_t* obj = py_alloc_object(cls);
        return WITH_RESULT(obj);
    };

    // def __init__(...):
    CLASS_METHOD(object, __init__) {
        // The default implementation of __init__ is a no-op.
        return WITH_RESULT(&py_none);
    };

    // def __str__(self):
    CLASS_METHOD(object, __str__) {
        ENSURE_NOT_NULL(self);

        pyobj_t* name = py_get_attribute(self, STR("__name__"));
        if (name == NULL || name->type != &py_type_str)
            return WITH_RESULT(PY_STR("<unknown object>"));

        string_t first = std_strconcat(STR("<"), name->as_str);
        string_t full = std_strconcat(first, STR("object>"));
        mm_heap_free(first.str);
        
        pyobj_t* obj = mm_heap_alloc(sizeof(pyobj_t));
        obj->type = &py_type_str;
        obj->as_str = full;
        return WITH_RESULT(obj);
    }

    CLASS_ATTRIBUTES(object)
        HAS_CLASS_METHOD(object, __new__),
        HAS_CLASS_METHOD(object, __init__),
        HAS_CLASS_METHOD(object, __str__)
    END_CLASS_ATTRIBUTES;
DEFINE_ROOT_TYPE(object);

CLASS(bool)
    // def __str__(self):
    CLASS_METHOD_E(_bool, __str__) {
        ENSURE_NOT_NULL(self);
        py_verify_self_arg(self, &py_type_bool);
        return WITH_RESULT(self->as_bool ? PY_STR("True") : PY_STR("False"));
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
    // def __str__(self):
    CLASS_METHOD(str, __str__) {
        ENSURE_NOT_NULL(self);
        py_verify_self_arg(self, &py_type_str);
        return WITH_RESULT(self);
    };

    // def __new__(cls, value):
    CLASS_METHOD(str, __new__) {
        // pyobj_t* cls = NOT_NULL(self);
    
        if (argc != 1)
            sys_panic("Expected exactly one argument to str(...).");

        pyobj_t* value = NOT_NULL(argv)[0];
        pyobj_t* method_str;
        py_get_method_attribute(value, STR("__str__"), &method_str);

        if (method_str == NULL || method_str->type != &py_type_function)
            return WITH_RESULT(PY_STR("<object>"));

        return py_call(method_str, 0, NULL, 0, NULL, value);
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
    CLASS_METHOD(type, __call__) {
        ENSURE_NOT_NULL(self);

        // A call to a `type` object creates a new object of the given type to be
        // created. For example, if we did `class A: pass`, then `A()`, we'd be calling
        // the `type` object `A`.
        //
        // We first resolve the attribute __new__ on our type object - it might've been
        // overriden by the class. In most cases, we'll invoke the default __new__ implementation
        // on `type`. That'll give us an uninitialized empty object.
        pyobj_t* method_new;
        ASSERT(py_get_method_attribute(self, STR("__new__"), &method_new));

        // __new__ is a class method, not an instance method. First argument is the class.
        pyobj_t* obj = NOT_NULL(UNWRAP(py_call(method_new, argc, argv, kwargc, kwargv, self)));

        // If __new__() does not return an instance of cls, then the new instance's __init__() method will not be invoked.
        if (obj->type == self) {
            pyobj_t* method_init;
            ASSERT(py_get_method_attribute(obj, STR("__init__"), &method_init));

            // We forward the arguments we got to __init__. So, if we get invoked with `A(a, b, c)`,
            // we'd do A.__init__(obj, a, b, c).
            py_call(method_init, argc, argv, kwargc, kwargv, obj);
        }

        return WITH_RESULT(obj);
    };

    CLASS_ATTRIBUTES(type)
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
        ENSURE_NOT_NULL(self);
        
        if (self->type != &py_type_function)
            RAISE(TypeError, "expected a function as 'instance' in function.__get__");

        if (argc == 0)
            RAISE(TypeError, "expected an 'instance' argument for function.__get__");

        if (argc > 2)
            RAISE(TypeError, "too many arguments for function.__get__");

        pyobj_t* instance = NOT_NULL(NOT_NULL(argv)[0]);
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

pyobj_t py_none = { .type = &py_type_NoneType };
pyobj_t py_true = { .type = &py_type_bool, .as_bool = true };
pyobj_t py_false = { .type = &py_type_bool, .as_bool = false };

// Looks up `name` in the class attribute table of `type`, going only one level deep
// (i.e. not checking the base type). `target` should be assignable to `type`.
static pyobj_t* py_get_class_attribute(
    pyobj_t* target,
    pyobj_t* type,
    string_t name,
    bool unbound_methods,
    bool* out_is_unbound
) {
    ASSERT(type->type == &py_type_type);

    const vector_t(symbol_t)* attributes = &type->as_type->class_attributes;
    for (size_t i = 0; i < attributes->length; i++) {
        symbol_t attribute = attributes->elements[i];

        if (!std_strequ(attribute.name, name))
            continue;

        pyobj_t* attr = attribute.value;
        pyobj_t* val = attr; // this is the value we'll actually return

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
        //
        // We do NOT do this if `unbound_methods` is `true` and the object
        // we'd be calling __get__ on is a `function`. We are 100% sure what
        // __get__ on a `function` does (binds a method to an object), so we
        // can safely just not invoke it if we want an unbound method.
        bool skip_get_call = unbound_methods && attr->type == &py_type_function;

        if (!skip_get_call) {
            pyobj_t* get;
            if (
                py_get_method_attribute(attr, STR("__get__"), &get) &&
                get != NULL &&
                get->type == &py_type_method
            ) {
                pyobj_t* args[] = {
                    target,             // instance
                    type                // owner
                };

                pyreturn_t result = py_call(get, 2, args, 0, NULL, NULL);
                if (result.exception != NULL) {
                    // TODO: Raise exceptions thrown by __get__ when getting attributes
                    sys_panic("Exception was thrown while invoking __get__");
                }

                val = result.value;
            }
        }
        else {
            *out_is_unbound = true;
        }
        
        return val;
    }

    return NULL;
}

// Attribute resolution implementation.
// - `target`: the object that contains the desired attribute
// - `name`: the name of the attribute
// - `unbound_methods`: if `true`, methods will not be bound (`__get__` on `function` will never be called)
// - `out_is_unbound`: set to `true` if the returned value is a method that wasn't bound (only set w/ `unbound_methods == true`)
static pyobj_t* py_get_attribute_arbitrary(
    pyobj_t* target,
    string_t name,
    bool unbound_methods,
    bool* out_is_unbound
) {
    ENSURE_STR_VALID(name);
    ENSURE_NOT_NULL(target);
    ENSURE_NOT_NULL(out_is_unbound);

    *out_is_unbound = false;

    // First, try to find the attribute in the per-object attribute table if it's not
    // intrinsic. If it *is* intrinsic, we don't even hold an attribute table in the first
    // place. For example, `int`s are intrinsic, as they hold a single integer value, not
    // attributes - you cannot do `123.a = 2`.
    if (!target->type->as_type->is_intrinsic) {
        const vector_t(symbol_t)* attributes = &target->as_any;

        for (size_t i = 0; i < attributes->length; i++) {
            symbol_t attribute = attributes->elements[i];

            if (std_strequ(attribute.name, name)) {
                // We're *not* calling __get__ if we've retrieved the attribute from
                // what would be __dict__ in regular Python. For example:
                //      o.example = Always123()     # ...where 'o' is an object...
                //      print(o.example)            # will not invoke Always123.__get__
                // This is not the case with classes, which is a bit inconsistent.
                return attribute.value;
            }
        }
    }

    // If we didn't find anything, try searching in the class attribute tables. For
    // example, if we were to do something like:
    //      class A:
    //          abc = 123
    // ...then `A().abc` would be the same as `A.abc`. The type of `A()` would be set
    // to `A` - the object representing class `A`.

    // If the target is a type, we want to go search all class attributes present
    // in the inheritance chain of the actual type. So, if we had this inheritance chain:
    //      A <-- B <-- C                      (where <-- means "inherits from")
    // ...and only C had the class attribute "abc", A.abc would still resolve to C.abc.
    pyobj_t* current_base = target->type == &py_type_type ? target : target->type;

    while (current_base != NULL) {
        pyobj_t* attr = py_get_class_attribute(target, current_base, name, unbound_methods, out_is_unbound);
        if (attr != NULL)
            return attr;
        
        current_base = current_base->as_type->base;
    }

    return NULL;
}

bool py_get_method_attribute(pyobj_t* target, string_t name, pyobj_t** out_attr) {
    ENSURE_NOT_NULL(out_attr);

    bool is_unbound;
    *out_attr = py_get_attribute_arbitrary(target, name, true, &is_unbound);
    return is_unbound;
}

pyobj_t* py_get_attribute(pyobj_t* target, string_t name) {
    bool _is_unbound;
    return py_get_attribute_arbitrary(target, name, false, &_is_unbound);
}

void py_set_attribute(pyobj_t* target, string_t name, pyobj_t* value) {
    ENSURE_STR_VALID(name);
    ENSURE_NOT_NULL(target);
    ENSURE_NOT_NULL(value);

    if (target->type->as_type->is_intrinsic && target->type != &py_type_type)
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
        attributes = &target->as_type->class_attributes;
    }
    else {
        attributes = &target->as_any;
    }

    for (size_t i = 0; i < attributes->length; i++) {
        symbol_t* attribute = &attributes->elements[i];

        if (std_strequ(attribute->name, name)) {
            attribute->value = value;
            return;
        }
    }

    // No such attribute was defined before, so we add one.
    std_vector_append(attributes, ((symbol_t){ .name = name, .value = value }));
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
    obj->as_type = mm_heap_alloc(sizeof(type_data_t));
    obj->as_type->base = NOT_NULL(base);
    obj->as_type->is_intrinsic = false;
    obj->as_type->class_attributes = (vector_t(symbol_t)) {};
    return obj;
}

pyobj_t* py_alloc_object(pyobj_t* type) {
    ENSURE_NOT_NULL(type);
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
    symbol_t* kwargv,
    pyobj_t* self
) {
    ENSURE_NOT_NULL(target);
    if (argc != 0) {
        ENSURE_NOT_NULL(argv);
    }

    if (kwargc != 0) {
        ENSURE_NOT_NULL(kwargv);
    }

    if (target->type == &py_type_function) {
        // We pass in 'self' here in order to allow for unbound method calls without
        // the need to copy arguments. 
        return target->as_function(self, argc, argv, kwargc, kwargv);
    }

    if (self != NULL) {
        sys_panic("Attempted to provide a self parameter for a bound method.");
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
    //
    // Let's say we define a class like this:
    //      class A:
    //          def __call__(self):
    //              pass
    // When doing A(), we definitely don't want to do `A.__call__` - we want `type.__call__`!
    // Thus, we do do `target->type`, which is effectively `type(A)`.
    pyobj_t* call_attr;
    if (
        py_get_method_attribute(target->type, STR("__call__"), &call_attr) &&
        call_attr != NULL &&
        call_attr->type == &py_type_function
    ) {
        return py_call(call_attr, argc, argv, kwargc, kwargv, target);
    }

    RAISE(TypeError, "attempted to call a non-callable object");
}

string_t py_stringify(pyobj_t* target) {
    if (target == NULL)
        return STR("<NULL>");

    if (target == &py_none)
        return STR("None");

    pyobj_t* method_str;
    if (!py_get_method_attribute(target, STR("__str__"), &method_str))
        return STR("(unknown object)");

    pyreturn_t converted = py_call(method_str, 0, NULL, 0, NULL, target);
    if (converted.exception != NULL)
        return STR("<error while stringifying>");

    ENSURE_NOT_NULL(converted.value);

    if (converted.value->type != &py_type_str)
        return py_stringify(converted.value);

    return converted.value->as_str;
}

bool py_isinstance(const pyobj_t* target, const pyobj_t* type) {
    ENSURE_NOT_NULL(target);
    ENSURE_NOT_NULL(type);

    const pyobj_t* current = target->type;
    while (current != NULL) {
        ASSERT(current->type == &py_type_type);

        if (current == type)
            return true;

        current = current->as_type->base;
    }

    return false;
}