#pragma once

#include "stddef.h"

// Represents a physical memory address.
typedef size_t physaddr_t;

// The page size of the architecture the system will be compiled for.
#define PAGE_SIZE 4096

// Initializes the memory manager.
void mm_init(void);

// Allocates a single page of physical memory.
void* mm_page_alloc(void);

// Frees a pointer previously allocated with `pmm_alloc`.
void mm_page_free(void* ptr);

// Converts a physical memory address to a virtual one.
void* mm_phys_to_virt(physaddr_t address);

// Allocates an arbitrary number of bytes from the heap.
void* mm_heap_alloc(size_t count);

// Frees an area of memory previously allocated with `mm_heap_alloc`.
void mm_heap_free(void* ptr);
