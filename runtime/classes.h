#pragma once

#include "symbols.h"
#include "functions.h"
#include "objects.h"

// Syntactic sugar. Signifies the start of a class declaration.
#define CLASS($name)

// Equivalent to `DEFINE_TYPE`, but specialized for types that have names that may conflict
// with C macro definitions. `$name` must be prefixed with `_`.
#define DEFINE_TYPE_E($name, $intrinsic, $inherits_from)                              \
    const pyobj_t py_type##$name = {                                                  \
        .type = &py_type_type,                                                        \
        .as_type = {                                                                  \
            .class_attributes = {                                                     \
                .elements = py_type##$name##_attrs_initial,                           \
                .length = sizeof(py_type##$name##_attrs_initial) / sizeof(symbol_t),  \
                .capacity = sizeof(py_type##$name##_attrs_initial) / sizeof(symbol_t) \
            },                                                                        \
            .is_intrinsic = ($intrinsic),                                             \
            .base = ($inherits_from)                                                  \
        }                                                                             \
    };                                                                                \
    const pyobj_t* pyglobal_##$name = &py_type##$name;                                \

// Defines a Python type of name `$name`, leaving the attribute table undefined. The attribute
// table's name is expected to be `py_type_<name>_attrs`, and its type is required to be
// `vector_t(symbol_t)`. If `$intrinsic` is `true`, the type is marked as "intrinsic", meaning
// that instances of the class will not hold their own attribute table, but instead a direct
// value (e.g. for `int`, a `int64_t`).
//
// This macro will also define a global with the name `$name` that points to the type object.
#define DEFINE_TYPE($name, $intrinsic, $inherits_from) \
    DEFINE_TYPE_E(_##$name, $intrinsic, $inherits_from)

// Equivalent to `DEFINE_INTRINSIC_TYPE`, but specialized for types that have names that may
// conflict with C macro definitions. `$name` must be prefixed with `_`.
#define DEFINE_INTRINSIC_TYPE_E($name) DEFINE_TYPE_E($name, true, NULL)

// Defines an intrinsic Python type, the instances of which would not hold their own
// attribute tables. See `DEFINE_TYPE` for more details.
#define DEFINE_INTRINSIC_TYPE($name) DEFINE_INTRINSIC_TYPE_E(_##$name)

// Equivalent to `DEFINE_BUILTIN_TYPE`, but specialized for types that have names that may
// conflict with C macro definitions. `$name` must be prefixed with `_`.
#define DEFINE_BUILTIN_TYPE_E($name, $inherits_from) DEFINE_TYPE_E($name, false, $inherits_from)

// Defines a regular Python type, provided by the runtime. See `DEFINE_TYPE` for more details.
#define DEFINE_BUILTIN_TYPE($name, $inherits_from) DEFINE_BUILTIN_TYPE_E(_##$name, $inherits_from)

// Begins the class attribute list for type `$name`, where `$name` is a underscore-prefixed
// typename. The name will be displayed as `$name_display`. This macro should be used when
// the typename may conflict with a C keyword or macro definition.
#define CLASS_ATTRIBUTES_E($name, $name_display)                                  \
    static pyobj_t py_type##$name##_name = PY_STR_LITERAL($name_display);         \
    symbol_t py_type##$name##_attrs_initial[] = {                                 \
        { .name = "__name__", .value = &py_type##$name##_name },                  \

// Begins the class attribute list for type `$name`.
#define CLASS_ATTRIBUTES($name) CLASS_ATTRIBUTES_E(_##$name, #$name)

// Ends a class attribute list defined previously by `CLASS_ATTRIBUTES($name)`.
#define END_CLASS_ATTRIBUTES }

// Equivalent to `HAS_CLASS_METHOD`, but specialized for types that have names that may
// conflict with C macro definitions. `$name` must be prefixed with `_`.
#define HAS_CLASS_METHOD_E($type, $name)                     \
    { .name = #$name, .value = &py_type##$type##_method_##$name }

// Specifies that a type method should be included in a type attribute list defined
// by `CLASS_ATTRIBUTES`.
#define HAS_CLASS_METHOD($type, $name) HAS_CLASS_METHOD_E(_##$type, $name)

// Equivalent to `CLASS_METHOD`, but specialized for types that have names that may
// conflict with C macro definitions. `$name` must be prefixed with `_`.
#define CLASS_METHOD_E($type, $name)                                        \
    PY_DEFINE(py_type##$type##_method_##$name##_fn);                        \
    static pyobj_t py_type##$type##_method_##$name = {                      \
        .type = &py_type_function,                                          \
        .as_function = &py_type##$type##_method_##$name##_fn                \
    };                                                                      \
    PY_DEFINE(py_type##$type##_method_##$name##_fn)                         

// Defines the implementation of the type method `$name`, belonging to type `$type`.
#define CLASS_METHOD($type, $name) CLASS_METHOD_E(_##$type, $name)

// Verifies that the type of the given `self` pointer is either `type`, or inherits
// from `type`.
void py_verify_self_arg(pyobj_t* self, const pyobj_t* type);
