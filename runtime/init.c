#include "init.h"

#include "sys/mm.h"
#include "sys/terminal.h"

static pyobj_t py_str_main_name = PY_STR_LITERAL("__name__");
pyobj_t* KNOWN_GLOBAL(__name__) = &py_str_main_name;

void sys_init(void) {
    mm_init();
    terminal_init();

    terminal_println("Pyton 0.0.1 on bare metal");
    terminal_println("All systems nominal");
}

void sys_handle_main_return(pyreturn_t result) {
    if (result.exception != NULL) {
        // Oops, the script finished running with an exception...
        terminal_println("An uncaught exception was encountered.");
        terminal_println(py_stringify(result.exception));
    }
    else {
        terminal_println("(script finished running, hanging)");
    }

    while (true) {}
}
