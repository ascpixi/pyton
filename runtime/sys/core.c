#include "core.h"

#include "terminal.h"

noreturn void sys_panic(const char* message) {
    if (terminal_is_initialized()) {
        terminal_println("Pyton has encountered a fatal error and cannot continue.");
        terminal_println(message);
    }

    while (1) {}
}