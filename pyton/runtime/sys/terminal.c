#include "terminal.h"

#include "bootloader.h"
#include "std/stringop.h"
#include "std/safety.h"
#include <flanterm/src/flanterm.h>
#include <flanterm/src/flanterm_backends/fb.h>

typedef struct flanterm_context flanterm_context_t;

static flanterm_context_t* global_terminal;
static bool terminal_initialized = false;

void terminal_init(void) {
    bl_framebuffer_t fb = bl_get_framebuffer();

    global_terminal = flanterm_fb_init(
        NULL, // TODO: Provide custom heap allocation functions here
        NULL,
        fb->address,
        fb->width,
        fb->height,
        fb->pitch,
        fb->red_mask_size,
        fb->red_mask_shift,
        fb->green_mask_size,
        fb->green_mask_shift,
        fb->blue_mask_size,
        fb->blue_mask_shift,
        NULL, // canvas
        NULL, NULL, // ansi colors
        NULL, NULL, // bg/fg
        NULL, NULL, // bg/fg bright
        NULL, 0, 0, 1, // font/font width/font height/font spacing
        0, 0, // font scale
        0 // margin
    );

    terminal_initialized = true;
}

void terminal_println(const char* str) {
    ENSURE_NOT_NULL(str);
    flanterm_write(global_terminal, str, strlen(str) + 1); // we add 1 to account for the \0
    terminal_newline();
}

void terminal_newline(void) {
    const char newline[] = "\n";
    flanterm_write(global_terminal, newline, sizeof(newline));
}

bool terminal_is_initialized(void) {
    return terminal_initialized;
}