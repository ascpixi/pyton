#pragma once

#include "functions.h"
#include "symbols.h"
#include "objects.h"

// A list of all built-in symbols. This is constant, and does not change.
extern const symbol_t py_builtins[];

PY_DEFINE(py_builtin_print);
