#include "bootloader.h"

#include "../lib/limine/limine.h"

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

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

size_t bl_get_hhdm_start() {
    return (size_t)hhdm_request.response->offset;
}

span_t(bl_memmap_entry_t) bl_get_memmap() {
    return (span_t(bl_memmap_entry_t)) {
        .entries = memmap_request.response->entries,
        .length = memmap_request.response->entry_count
    };
}