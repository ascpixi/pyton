#include "init.h"

#include "sys/mm.h"
#include "sys/terminal.h"

void sys_init(void) {
    mm_init();
    terminal_init();

    terminal_println("Pyton 0.0.1 on bare metal");
    terminal_println("All systems nominal");
}