#pragma once

#include <limine/limine.h>
#include "../rtl/span.h"

// Currently the memory map types reflect the ones used by Limine. This technically is
// an implementation detail but we don't expect to change bootloaders any time soon.
// In the future we could define our own memmap types and translate between the bootloader-provided
// one and the abstracted ones.

// Represents a single framebuffer (a screen or some other display device).
typedef struct limine_framebuffer* bl_framebuffer_t;

// Represents a single memory map entry.
typedef struct limine_memmap_entry* bl_memmap_entry_t;
    DECLARE_SPAN_OF(bl_memmap_entry_t);

// Gets the start of the HHDM (Higher-Half Direct Map) virtual memory area.
// This area serves as a linear mapping between physical and virtual memory.
size_t bl_get_hhdm_start();

// Gets the physical memory map.
span_t(bl_memmap_entry_t) bl_get_memmap();

// Gets the framebuffer for the primary display device.
bl_framebuffer_t bl_get_framebuffer();