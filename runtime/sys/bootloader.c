#include "bootloader.h"

#include <limine/limine.h>
#include "../std/safety.h"
#include "core.h"

// We currently use Limine as our bootloader. We may change this in the future.
// The use of Limine is an implementation detail.

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

size_t bl_get_hhdm_start(void) {
    return (size_t)NOT_NULL(hhdm_request.response)->offset;
}

span_t(bl_memmap_entry_t) bl_get_memmap(void) {
    ENSURE_NOT_NULL(memmap_request.response);

    return (span_t(bl_memmap_entry_t)) {
        .entries = memmap_request.response->entries,
        .length = memmap_request.response->entry_count
    };
}

bl_memmap_entry_t* bl_get_memmap_entries(void) {
    return NOT_NULL(memmap_request.response)->entries;
}

size_t bl_get_memmap_length(void) {
    return NOT_NULL(memmap_request.response)->entry_count;
}

bl_framebuffer_t bl_get_framebuffer(void) {
    ENSURE_NOT_NULL(framebuffer_request.response);

    if (framebuffer_request.response->framebuffer_count == 0) {
        sys_panic("There isn't an available framebuffer to use as a display surface.");
    }

    return framebuffer_request.response->framebuffers[0];
} 