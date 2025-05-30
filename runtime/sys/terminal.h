#pragma once

#include <stdbool.h>

// Initializes the on-screen terminal.
void terminal_init(void);

// Prints the given line to the on-screen terminal.
void terminal_println(const char* str);

// Returns `true` if the terminal has been initialized.
bool terminal_is_initialized(void);

// Prints a single new-line character.
void terminal_newline(void);