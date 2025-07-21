#pragma once

// Represents a tuple of two elements.
// Usage:
// ```
//      tuple_t(bool, int) tup = { true, 1 };
//      tup.i1 = false;
//      tup.i2 = 2;
// ```
#define tuple_t(T1, T2) struct tuple_##T1##_##T2

// Declares that the given translation unit will use a `tuple_t(T1, T2)` structure.
#define USES_TUPLE_FOR(T1, T2) struct tuple_##T1##_##T2 { T1 i1; T2 i2; }
