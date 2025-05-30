#pragma once

#include <stddef.h>

// Computes `ceil(a / b)`.
#define CEIL_INTDIV($a, $b) (($a + $b - 1) / $b)
