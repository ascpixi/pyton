#pragma once

// Manually defines a known global. When defining such a global, be sure to also define
// a corresponding `PY_GLOBAL_<name>_WELLKNOWN` preprocessor symbol, so that the transpiler
// does not create a duplicate.
#define KNOWN_GLOBAL($name) pyglobal__##$name
