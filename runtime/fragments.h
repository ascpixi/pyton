#pragma once

// This header file specifies fragments used from transpiled code.
// This does not include opcode related fragments - see opcodes.h for those.

// Provides the stack item at position `-$i`. For example, `STACK_ITEM(1)` returns
// the top of the stack (`STACK[-1]`).
#define STACK_ITEM($i) stack[stack_current - ($i) + 1]

#define STACK_POP()  stack[stack_current--]
#define STACK_PUSH() stack[++stack_current]
#define STACK_PEEK() stack[stack_current]

// Pushes to the stack, assuming `stack` is a `void**` and `stack_current` is a `int*`.
#define STACK_PUSH_INDIRECT($x) stack[++(*stack_current)] = ($x)

// Pops from the stack, assuming `stack` is a `void**` and `stack_current` is a `int*`.
#define STACK_POP_INDIRECT() stack[(*stack_current)--]

// The symbol name for the return values of '<module>' functions.
#define MODULE_INIT_STATE($name) py_module_initstate_##$name

// Present in the top of '<module>' functions.
#define MODULE_PROLOGUE($name)                                          \
    if (MODULE_INIT_STATE($name))                                       \
        return WITH_RESULT(&py_none);                                   \
    MODULE_INIT_STATE($name) = true;
