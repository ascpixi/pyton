#pragma once

#include "symbols.h"
#include "functions.h"
#include "objects.h"

// Syntactic sugar. Signifies the start of a class declaration.
#define CLASS($name)

// Defines a Python type of name `$name`, leaving the attribute table undefined. The attribute
// table's name is expected to be `py_type_<name>_attrs`, and its type is required to be
// `vector_t(symbol_t)`. If `$intrinsic` is `true`, the type is marked as "intrinsic", meaning
// that instances of the class will not hold their own attribute table, but instead a direct
// value (e.g. for `int`, a `int64_t`).
//
// `$name` must be prefixed with `_` in order to avoid conflicts with C keywords and/or
// macros. This macro will also define a global with the name `$name` that points to the
// type object.
#define DEFINE_TYPE($name, $intrinsic, $inherits_from)                                \
    const pyobj_t py_type##$name = {                                                  \
        .type = &py_type_type,                                                        \
        .as_type = {                                                                  \
            .class_attributes = {                                                     \
                .elements = py_type##$name##_attrs_initial,                           \
                .length = sizeof(py_type##$name##_attrs_initial) / sizeof(symbol_t),  \
                .capacity = sizeof(py_type##$name##_attrs_initial) / sizeof(symbol_t) \
            },                                                                        \
            .is_intrinsic = $intrinsic,                                               \
            .base = $inherits_from                                                    \
        }                                                                             \
    };                                                                                \
    const pyobj_t* pyglobal_##$name = &py_type##$name;                                \

// Defines an intrinsic Python type, the instances of which would not hold their own
// attribute tables. See `DEFINE_TYPE` for more details.
#define DEFINE_INTRINSIC_TYPE($name) DEFINE_TYPE($name, true, NULL)

// Defines a regular Python type, provided by the runtime. See `DEFINE_TYPE` for more details.
#define DEFINE_BUILTIN_TYPE($name, $inherits_from) DEFINE_TYPE($name, false, $inherits_from)

// Begins the class attribute list for type `$name`.
#define CLASS_ATTRIBUTES($name)                   \
    symbol_t py_type##$name##_attrs_initial[] =

// Specifies that a type method should be included in a type attribute list defined
// by `CLASS_ATTRIBUTES`.
#define HAS_CLASS_METHOD($type, $name)                                   \
    { .name = #$name, .value = &py_type##$type##_method_##$name }

// Defines the implementation of the type method `$name`, belonging to type `$type`.
#define CLASS_METHOD($type, $name)                                          \
    PY_DEFINE(py_type##$type##_method_##$name##_fn);                        \
    static const pyobj_t py_type##$type##_method_##$name = {                \
        .type = &py_type_callable,                                          \
        .as_callable = &py_type##$type##_method_##$name##_fn                \
    };                                                                      \
    PY_DEFINE(py_type##$type##_method_##$name##_fn)                         

// Verifies that the type of the given `self` pointer is either `type`, or inherits
// from `type`.
void py_verify_self_arg(pyobj_t* self, pyobj_t* type);
