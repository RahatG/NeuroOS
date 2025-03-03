/**
 * memory.h - Memory management for NeuroOS
 * 
 * This file contains the memory management definitions and declarations.
 */

#ifndef NEUROOS_MEMORY_H
#define NEUROOS_MEMORY_H

#include <stddef.h>
#include <stdint.h>

// Memory page size
#define MEMORY_PAGE_SIZE 4096

// Memory protection flags
typedef uint32_t memory_prot_t;
#define MEMORY_PROT_NONE    0
#define MEMORY_PROT_READ    (1 << 0)
#define MEMORY_PROT_WRITE   (1 << 1)
#define MEMORY_PROT_EXEC    (1 << 2)
#define MEMORY_PROT_USER    (1 << 3)

// Memory allocation flags
typedef uint32_t memory_alloc_flags_t;
#define MEMORY_ALLOC_NONE        0
#define MEMORY_ALLOC_ZEROED      (1 << 0)
#define MEMORY_ALLOC_CONTIGUOUS  (1 << 1)
#define MEMORY_ALLOC_KERNEL      (1 << 2)

// Memory flags
#define MEMORY_FLAG_READ      (1 << 0)
#define MEMORY_FLAG_WRITE     (1 << 1)
#define MEMORY_FLAG_EXECUTE   (1 << 2)
#define MEMORY_FLAG_USER      (1 << 3)
#define MEMORY_FLAG_KERNEL    (1 << 4)
#define MEMORY_FLAG_CACHED    (1 << 5)
#define MEMORY_FLAG_UNCACHED  (1 << 6)
#define MEMORY_FLAG_DEVICE    (1 << 7)
#define MEMORY_FLAG_DMA       (1 << 8)
#define MEMORY_FLAG_SHARED    (1 << 9)
#define MEMORY_FLAG_PRIVATE   (1 << 10)
#define MEMORY_FLAG_FIXED     (1 << 11)
#define MEMORY_FLAG_GROWSDOWN (1 << 12)
#define MEMORY_FLAG_GROWSUP   (1 << 13)
#define MEMORY_FLAG_STACK     (1 << 14)
#define MEMORY_FLAG_HEAP      (1 << 15)
#define MEMORY_FLAG_MMIO      (1 << 16)
#define MEMORY_FLAG_ACPI      (1 << 17)
#define MEMORY_FLAG_NVS       (1 << 18)
#define MEMORY_FLAG_RESERVED  (1 << 19)
#define MEMORY_FLAG_BADRAM    (1 << 20)

// Memory region structure
typedef struct {
    uint64_t start;
    uint64_t size;
    uint32_t flags;
    const char* name;
} memory_region_t;

// Memory statistics structure
typedef struct {
    uint64_t total;
    uint64_t used;
    uint64_t free;
    uint64_t shared;
    uint64_t buffers;
    uint64_t cached;
    uint64_t swap_total;
    uint64_t swap_used;
    uint64_t swap_free;
    uint64_t available;
} memory_stats_t;

// Memory initialization and shutdown
void memory_init(void);
void memory_shutdown(void);

// Memory allocation and deallocation
void* memory_alloc(size_t size, memory_prot_t protection, memory_alloc_flags_t flags);
void* memory_alloc_aligned(size_t size, size_t alignment);
void* memory_calloc(size_t nmemb, size_t size);
void* memory_realloc(void* ptr, size_t size);
void memory_free(void* ptr, size_t size);
uintptr_t memory_alloc_physical(size_t num_pages, memory_alloc_flags_t flags);
void memory_free_physical(uintptr_t physical_addr, size_t num_pages);

// Memory mapping and unmapping
int memory_map(uintptr_t physical_addr, void* virtual_addr, size_t size, memory_prot_t protection);
void memory_unmap(void* virtual_addr, size_t size);
int memory_set_protection(void* addr, size_t size, memory_prot_t protection);
int memory_lock(void* virtual, size_t size);
int memory_unlock(void* virtual, size_t size);
int memory_sync(void* virtual, size_t size, int flags);

// Memory address translation
uint64_t memory_virtual_to_physical(void* virtual);
void* memory_physical_to_virtual(uint64_t physical);

// Page directory management
void memory_switch_page_directory(void* page_directory);

// Memory region management
int memory_add_region(uint64_t start, uint64_t size, uint32_t flags, const char* name);
int memory_remove_region(uint64_t start);
int memory_get_region(uint64_t address, memory_region_t* region);
int memory_get_region_count(void);
int memory_get_region_list(memory_region_t* regions, int count);

// Memory statistics
void memory_get_stats(size_t* total, size_t* used, size_t* free);
int memory_get_stats_struct(memory_stats_t* stats);
uint64_t memory_get_total(void);
uint64_t memory_get_used(void);
uint64_t memory_get_free(void);
uint64_t memory_get_available(void);

// Memory utilities
void memory_copy(void* dest, const void* src, size_t size);
void memory_set(void* dest, int value, size_t size);
int memory_compare(const void* s1, const void* s2, size_t size);
void* memory_find(const void* s, int c, size_t size);

// Memory debug functions
void memory_dump(const void* ptr, size_t size);
void memory_print_stats(void);
void memory_print_regions(void);
void memory_print_page_tables(void);
void memory_print_heap(void);
void memory_check_leaks(void);

#endif // NEUROOS_MEMORY_H
