#pragma once

#include "objects.h"

// Initializes all runtime and system services, and prepares the system to run arbitrary
// C-transpiled Python code.
void sys_init(void);

// Ran after the C-transpiled main Python code fragment returns a value or raises
// an exception.
void sys_handle_main_return(pyreturn_t result);

// Defines the kernel entry-point, which, after system initialization, will invoke
// the given transpiled Python function.
#define DEFINE_ENTRYPOINT($main)                                        \
    void kmain(void) {                                                  \
        sys_init();                                                     \
        sys_handle_main_return($main(NULL, 0, NULL, 0, NULL));          \
    }
