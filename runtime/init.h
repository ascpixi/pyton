#pragma once

// Initializes all runtime and system services, and prepares the system to run arbitrary
// C-transpiled Python code.
void sys_init(void);

// Defines the kernel entry-point, which, after system initialization, will invoke
// the given transpiled Python function.
#define DEFINE_ENTRYPOINT($main)        \
    void kmain(void) {                  \
        sys_init();                     \
        $main(NULL, 0, NULL, 0, NULL);  \
    }
