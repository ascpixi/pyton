#include "mm.h"

#include <stdbool.h>
#include "bootloader.h"
#include "core.h"
#include "../rtl/math.h"
#include "../rtl/safety.h"

static bool mm_was_initialized;

// Represents a single entry in the page free-list.
typedef struct mm_freelist_entry {
    mm_freelist_entry_t* next;
} mm_freelist_entry_t;

// A pointer to the next available page in the free-list.
static mm_freelist_entry_t* mm_freelist_first;
static mm_freelist_entry_t* mm_freelist_last;

void mm_init() {
    if (mm_was_initialized) {
        sys_panic("Attempted to initialize the memory manager twice.");
    }

    mm_freelist_first = NULL;
    mm_freelist_last = NULL;

    span_t(bl_memmap_entry_t) memmap = bl_get_memmap();
    for (size_t i = 0; i < memmap.length; i++) {
        bl_memmap_entry_t entry = memmap.entries[i];

        // We're only interested in usable memory regions.
        if (entry->type != LIMINE_MEMMAP_USABLE)
            continue;

        for (size_t offset = 0; offset < entry->length; offset += PAGE_SIZE) {
            physaddr_t paddr = entry->base + offset;
            void* vaddr = mm_phys_to_virt(paddr);

            if (mm_freelist_first == NULL) {
                // This is our very first usable page!
                mm_freelist_first = mm_freelist_last = vaddr;
                mm_freelist_first->next = NULL; // no next page yet... but we're probably gonna have more.
                continue;
            }
            
            mm_freelist_last->next = vaddr;
            mm_freelist_last = vaddr;
            mm_freelist_last->next = NULL; // no next page yet!
        }
    }

    mm_was_initialized = true;
}

// Panics if the memory manager hasn't been initialized.
#define ENSURE_INITIALIZED($caller)                                               \
    if (!mm_was_initialized) {                                                    \
        sys_panic("Cannot invoke '" $caller "' before 'mm_init' is called.");     \
    }                                                                             \

void* mm_page_alloc() {
    ENSURE_INITIALIZED("mm_page_alloc");

    if (mm_freelist_first == NULL) {
        // If `mm_freelist_first` is NULL, that means that a previous call to `mm_page_alloc`
        // assigned the next pointer of the very last page to this variable. We've ran out
        // of available pages!
        sys_panic("Out of physical memory.");
    }

    void* page = mm_freelist_first;
    mm_freelist_first = mm_freelist_first->next;
    return page;
}

void mm_page_free(void* ptr) {
    ENSURE_INITIALIZED("mm_page_free");
    ENSURE_NOT_NULL(ptr, "mm_page_free");

    if (mm_freelist_first == NULL) {
        // Oof... we're very low on memory.
        mm_freelist_first = ptr;
        mm_freelist_last = ptr;
        mm_freelist_first->next = NULL;
        return;
    }

    mm_freelist_last->next = ptr;  // What was previously the last page points to the freed one
    mm_freelist_last = ptr;        // Now the freed one is the last one
    mm_freelist_last->next = NULL; // The last one doesn't point to anything
}

void* mm_phys_to_virt(physaddr_t address) {
    return (void*)(bl_get_hhdm_start() + address);
}

void* mm_heap_alloc(size_t count) {
    ENSURE_INITIALIZED("mm_alloc");

    // TODO: Actual heap allocator
    // Right now we fail for allocs > 4KiB since we just forward everything to the page allocator.
    // This will most probably change in the future
    if (count <= 4096) {
        return mm_page_alloc();
    }

    sys_panic("Allocations larger than 4KiB aren't supported.");
}

void mm_heap_free(void* ptr) {
    ENSURE_NOT_NULL(ptr, "mm_heap_free");

    mm_page_free(ptr);
}