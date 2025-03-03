/**
 * memory.c - Memory management implementation for NeuroOS
 * 
 * This file implements the memory management subsystem, which is responsible for
 * allocating and freeing memory, as well as managing memory protection.
 */

#include "include/memory.h"
#include "include/console.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// Memory map entry structure
typedef struct {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_attrs;
} memory_map_entry_t;

// Memory map
static memory_map_entry_t default_memory_map[1] = {
    { 0x100000, 64 * 1024 * 1024, 1, 0 } // 64MB of memory starting at 1MB
};
static memory_map_entry_t* memory_map_entries_array = default_memory_map;
static size_t memory_map_entries_count = 1;

// Memory regions - these will be implemented in future versions
// Currently unused but kept for API compatibility

// Page size
#define PAGE_SIZE 4096

// Page table entry flags
#define PTE_PRESENT  (1ULL << 0)
#define PTE_WRITABLE (1ULL << 1)
#define PTE_USER     (1ULL << 2)
#define PTE_ACCESSED (1ULL << 5)
#define PTE_DIRTY    (1ULL << 6)
#define PTE_HUGE     (1ULL << 7)
#define PTE_GLOBAL   (1ULL << 8)
#define PTE_NX       (1ULL << 63)

// Page table levels
#define PT_LEVELS 4

// Page table entry count per table
#define PT_ENTRIES 512

// Page table structures
typedef uint64_t pte_t;
typedef pte_t* pt_t;

// Page table root (CR3)
static pt_t page_table_root = NULL;

// Kernel heap
static void* kernel_heap_start = NULL;
static void* kernel_heap_end = NULL;
static void* kernel_heap_current = NULL;

// Memory allocation tracking
typedef struct memory_allocation {
    void* address;
    size_t size;
    memory_prot_t protection;
    uint32_t flags;
    struct memory_allocation* next;
} memory_allocation_t;

static memory_allocation_t* memory_allocations = NULL;

// Forward declarations
static int init_page_tables(void);
static int init_kernel_heap(void);
static void* allocate_physical_page(void);
static int map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);
static int unmap_page(uint64_t virt_addr);
static void* memory_resize(void* addr, size_t old_size, size_t new_size);

/**
 * Initialize the memory management subsystem
 */
void memory_init(void) {
    // Initialize the page tables
    if (init_page_tables() != 0) {
        console_printf("Error: Failed to initialize page tables\n");
        return;
    }
    
    // Initialize the kernel heap
    if (init_kernel_heap() != 0) {
        console_printf("Error: Failed to initialize kernel heap\n");
        return;
    }
    
    console_printf("Memory management initialized\n");
}

/**
 * Shutdown the memory management subsystem
 */
void memory_shutdown(void) {
    // Free all allocations
    memory_allocation_t* curr = memory_allocations;
    memory_allocation_t* next = NULL;
    
    while (curr) {
        next = curr->next;
        free(curr);
        curr = next;
    }
    
    memory_allocations = NULL;
    
    // Reset the kernel heap
    kernel_heap_start = NULL;
    kernel_heap_end = NULL;
    kernel_heap_current = NULL;
    
    // Reset the page table root
    page_table_root = NULL;
    
    console_printf("Memory management shutdown\n");
}

/**
 * Initialize the page tables
 * 
 * @return: 0 on success, -1 on failure
 */
static int init_page_tables(void) {
    // Allocate the page table root
    page_table_root = (pt_t)allocate_physical_page();
    if (!page_table_root) {
        return -1;
    }
    
    // Clear the page table root
    memset(page_table_root, 0, PAGE_SIZE);
    
    // Identity map the first 4GB of physical memory
    for (uint64_t addr = 0; addr < 4ULL * 1024 * 1024 * 1024; addr += PAGE_SIZE) {
        if (map_page(addr, addr, PTE_PRESENT | PTE_WRITABLE) != 0) {
            return -1;
        }
    }
    
    // Load the page table root into CR3
    __asm__ volatile("movl %0, %%cr3" : : "r"((uint32_t)(uintptr_t)page_table_root) : "memory");
    
    return 0;
}

/**
 * Initialize the kernel heap
 * 
 * @return: 0 on success, -1 on failure
 */
static int init_kernel_heap(void) {
    // Find a suitable memory region for the kernel heap
    uint64_t heap_start = 0;
    uint64_t heap_size = 16 * 1024 * 1024; // 16 MB
    
    for (size_t i = 0; i < memory_map_entries_count; i++) {
        if (memory_map_entries_array && memory_map_entries_array[i].type == 1 && memory_map_entries_array[i].length >= heap_size) {
            // Found a suitable memory region
            heap_start = memory_map_entries_array[i].base_addr;
            break;
        }
    }
    
    if (heap_start == 0) {
        return -1;
    }
    
    // Map the kernel heap
    for (uint64_t addr = heap_start; addr < heap_start + heap_size; addr += PAGE_SIZE) {
        if (map_page(addr, addr, PTE_PRESENT | PTE_WRITABLE) != 0) {
            return -1;
        }
    }
    
    // Initialize the kernel heap pointers
    kernel_heap_start = (void*)(uintptr_t)heap_start;
    kernel_heap_end = (void*)(uintptr_t)(heap_start + heap_size);
    kernel_heap_current = kernel_heap_start;
    
    return 0;
}

/**
 * Allocate a physical page
 * 
 * @return: Pointer to the allocated page, NULL on failure
 */
static void* allocate_physical_page(void) {
    // Find a free physical page
    for (size_t i = 0; i < memory_map_entries_count; i++) {
        if (memory_map_entries_array && memory_map_entries_array[i].type == 1 && memory_map_entries_array[i].length >= PAGE_SIZE) {
            // Found a free page
            void* page = (void*)(uintptr_t)memory_map_entries_array[i].base_addr;
            
            // Update the memory map
            memory_map_entries_array[i].base_addr += PAGE_SIZE;
            memory_map_entries_array[i].length -= PAGE_SIZE;
            
            // Clear the page
            memset(page, 0, PAGE_SIZE);
            
            return page;
        }
    }
    
    return NULL;
}

/**
 * Map a virtual address to a physical address
 * 
 * @param virt_addr: Virtual address
 * @param phys_addr: Physical address
 * @param flags: Page table entry flags
 * @return: 0 on success, -1 on failure
 */
static int map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags) {
    // Get the page table indices
    uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_idx = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_idx = (virt_addr >> 12) & 0x1FF;
    
    // Get the PML4 entry
    pte_t* pml4e = &page_table_root[pml4_idx];
    pt_t pdpt = NULL;
    
    if (*pml4e & PTE_PRESENT) {
        // PDPT already exists
        pdpt = (pt_t)(uintptr_t)(*pml4e & ~0xFFF);
    } else {
        // Allocate a new PDPT
        pdpt = (pt_t)allocate_physical_page();
        if (!pdpt) {
            return -1;
        }
        
        // Set the PML4 entry
        *pml4e = (pte_t)(uintptr_t)pdpt | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    
    // Get the PDPT entry
    pte_t* pdpte = &pdpt[pdpt_idx];
    pt_t pd = NULL;
    
    if (*pdpte & PTE_PRESENT) {
        // PD already exists
        pd = (pt_t)(uintptr_t)(*pdpte & ~0xFFF);
    } else {
        // Allocate a new PD
        pd = (pt_t)allocate_physical_page();
        if (!pd) {
            return -1;
        }
        
        // Set the PDPT entry
        *pdpte = (pte_t)(uintptr_t)pd | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    
    // Get the PD entry
    pte_t* pde = &pd[pd_idx];
    pt_t pt = NULL;
    
    if (*pde & PTE_PRESENT) {
        // PT already exists
        pt = (pt_t)(uintptr_t)(*pde & ~0xFFF);
    } else {
        // Allocate a new PT
        pt = (pt_t)allocate_physical_page();
        if (!pt) {
            return -1;
        }
        
        // Set the PD entry
        *pde = (pte_t)(uintptr_t)pt | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    
    // Set the PT entry
    pt[pt_idx] = phys_addr | flags;
    
    return 0;
}

/**
 * Unmap a virtual address
 * 
 * @param virt_addr: Virtual address
 * @return: 0 on success, -1 on failure
 */
static int unmap_page(uint64_t virt_addr) {
    // Get the page table indices
    uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_idx = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_idx = (virt_addr >> 12) & 0x1FF;
    
    // Get the PML4 entry
    pte_t* pml4e = &page_table_root[pml4_idx];
    
    if (!(*pml4e & PTE_PRESENT)) {
        // Page not mapped
        return 0;
    }
    
    // Get the PDPT
    pt_t pdpt = (pt_t)(uintptr_t)(*pml4e & ~0xFFF);
    pte_t* pdpte = &pdpt[pdpt_idx];
    
    if (!(*pdpte & PTE_PRESENT)) {
        // Page not mapped
        return 0;
    }
    
    // Get the PD
    pt_t pd = (pt_t)(uintptr_t)(*pdpte & ~0xFFF);
    pte_t* pde = &pd[pd_idx];
    
    if (!(*pde & PTE_PRESENT)) {
        // Page not mapped
        return 0;
    }
    
    // Get the PT
    pt_t pt = (pt_t)(uintptr_t)(*pde & ~0xFFF);
    pte_t* pte = &pt[pt_idx];
    
    if (!(*pte & PTE_PRESENT)) {
        // Page not mapped
        return 0;
    }
    
    // Unmap the page
    *pte = 0;
    
    // Invalidate the TLB entry
    __asm__ volatile("invlpg (%0)" : : "r"((uint32_t)virt_addr) : "memory");
    
    return 0;
}

/**
 * Allocate memory
 * 
 * @param size: Size of the memory to allocate
 * @param protection: Memory protection flags
 * @param flags: Memory allocation flags
 * @return: Pointer to the allocated memory, NULL on failure
 */
void* memory_alloc(size_t size, memory_prot_t protection, uint32_t flags) {
    // Check if the size is valid
    if (size == 0) {
        return NULL;
    }
    
    // Round up the size to a multiple of the page size
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    // Check if we have enough memory
    if ((uint8_t*)kernel_heap_current + size > (uint8_t*)kernel_heap_end) {
        return NULL;
    }
    
    // Allocate the memory
    void* addr = kernel_heap_current;
    kernel_heap_current = (uint8_t*)kernel_heap_current + size;
    
    // Set the memory protection
    if (memory_set_protection(addr, size, protection) != 0) {
        return NULL;
    }
    
    // Zero the memory if requested
    if (flags & MEMORY_ALLOC_ZEROED) {
        memset(addr, 0, size);
    }
    
    // Track the allocation
    memory_allocation_t* alloc = (memory_allocation_t*)malloc(sizeof(memory_allocation_t));
    if (!alloc) {
        return NULL;
    }
    
    alloc->address = addr;
    alloc->size = size;
    alloc->protection = protection;
    alloc->flags = flags;
    alloc->next = memory_allocations;
    memory_allocations = alloc;
    
    return addr;
}

/**
 * Allocate aligned memory
 * 
 * @param size: Size of the memory to allocate
 * @param alignment: Alignment of the memory to allocate
 * @return: Pointer to the allocated memory, NULL on failure
 */
void* memory_alloc_aligned(size_t size, size_t alignment) {
    // Check if the size and alignment are valid
    if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return NULL;
    }
    
    // Round up the size to a multiple of the page size
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    // Round up the alignment to a multiple of the page size
    alignment = (alignment + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    // Calculate the total size needed
    size_t total_size = size + alignment;
    
    // Check if we have enough memory
    if ((uint8_t*)kernel_heap_current + total_size > (uint8_t*)kernel_heap_end) {
        return NULL;
    }
    
    // Allocate the memory
    void* addr = kernel_heap_current;
    kernel_heap_current = (uint8_t*)kernel_heap_current + total_size;
    
    // Calculate the aligned address
    void* aligned_addr = (void*)(((uintptr_t)addr + alignment - 1) & ~(alignment - 1));
    
    // Set the memory protection
    if (memory_set_protection(addr, total_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE) != 0) {
        return NULL;
    }
    
    // Track the allocation
    memory_allocation_t* alloc = (memory_allocation_t*)malloc(sizeof(memory_allocation_t));
    if (!alloc) {
        return NULL;
    }
    
    alloc->address = addr;
    alloc->size = total_size;
    alloc->protection = MEMORY_PROT_READ | MEMORY_PROT_WRITE;
    alloc->flags = 0;
    alloc->next = memory_allocations;
    memory_allocations = alloc;
    
    return aligned_addr;
}

/**
 * Allocate and zero memory
 * 
 * @param nmemb: Number of elements
 * @param size: Size of each element
 * @return: Pointer to the allocated memory, NULL on failure
 */
void* memory_calloc(size_t nmemb, size_t size) {
    // Check if the size is valid
    if (nmemb == 0 || size == 0) {
        return NULL;
    }
    
    // Calculate the total size
    size_t total_size = nmemb * size;
    
    // Allocate the memory
    void* addr = memory_alloc(total_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    return addr;
}

/**
 * Reallocate memory
 * 
 * @param ptr: Pointer to the memory to reallocate
 * @param size: New size of the memory
 * @return: Pointer to the reallocated memory, NULL on failure
 */
void* memory_realloc(void* ptr, size_t size) {
    // Check if the pointer is valid
    if (!ptr) {
        return memory_alloc(size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, 0);
    }
    
    // Check if the size is valid
    if (size == 0) {
        memory_free(ptr, 0);
        return NULL;
    }
    
    // Find the allocation
    memory_allocation_t* alloc = memory_allocations;
    
    while (alloc) {
        if (alloc->address == ptr) {
            // Found the allocation
            return memory_resize(ptr, alloc->size, size);
        }
        
        alloc = alloc->next;
    }
    
    return NULL;
}

/**
 * Free memory
 * 
 * @param ptr: Address of the memory to free
 * @param size: Size of the memory to free
 */
void memory_free(void* ptr, size_t size) {
    // Check if the address is valid
    if (!ptr) {
        return;
    }
    
    // Round up the size to a multiple of the page size
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    // Find the allocation
    memory_allocation_t* prev = NULL;
    memory_allocation_t* curr = memory_allocations;
    
    while (curr) {
        if (curr->address == ptr && (size == 0 || curr->size == size)) {
            // Found the allocation
            if (prev) {
                prev->next = curr->next;
            } else {
                memory_allocations = curr->next;
            }
            
            // Free the allocation
            free(curr);
            
            // Unmap the pages
            for (uint64_t virt_addr = (uintptr_t)ptr; virt_addr < (uintptr_t)ptr + (size == 0 ? curr->size : size); virt_addr += PAGE_SIZE) {
                unmap_page(virt_addr);
            }
            
            return;
        }
        
        prev = curr;
        curr = curr->next;
    }
}

/**
 * Resize memory
 * 
 * @param addr: Address of the memory to resize
 * @param old_size: Old size of the memory
 * @param new_size: New size of the memory
 * @return: Pointer to the resized memory, NULL on failure
 */
void* memory_resize(void* addr, size_t old_size, size_t new_size) {
    // Check if the address is valid
    if (!addr) {
        return NULL;
    }
    
    // Round up the sizes to a multiple of the page size
    old_size = (old_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    new_size = (new_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    // Find the allocation
    memory_allocation_t* alloc = memory_allocations;
    
    while (alloc) {
        if (alloc->address == addr && alloc->size == old_size) {
            // Found the allocation
            if (new_size <= old_size) {
                // Shrinking the allocation
                alloc->size = new_size;
                
                // Unmap the extra pages
                for (uint64_t virt_addr = (uintptr_t)addr + new_size; virt_addr < (uintptr_t)addr + old_size; virt_addr += PAGE_SIZE) {
                    if (unmap_page(virt_addr) != 0) {
                        return NULL;
                    }
                }
                
                return addr;
            } else {
                // Growing the allocation
                // Check if we can grow in place
                if ((uint8_t*)addr + old_size == kernel_heap_current) {
                    // We can grow in place
                    if ((uint8_t*)addr + new_size > (uint8_t*)kernel_heap_end) {
                        // Not enough memory
                        return NULL;
                    }
                    
                    // Update the kernel heap pointer
                    kernel_heap_current = (uint8_t*)addr + new_size;
                    
                    // Update the allocation
                    alloc->size = new_size;
                    
                    // Set the memory protection for the new pages
                    if (memory_set_protection((uint8_t*)addr + old_size, new_size - old_size, alloc->protection) != 0) {
                        return NULL;
                    }
                    
                    return addr;
                } else {
                    // We need to allocate a new block
                    void* new_addr = memory_alloc(new_size, alloc->protection, alloc->flags);
                    if (!new_addr) {
                        return NULL;
                    }
                    
                    // Copy the data
                    memcpy(new_addr, addr, old_size);
                    
                    // Free the old block
                    memory_free(addr, old_size);
                    
                    return new_addr;
                }
            }
        }
        
        alloc = alloc->next;
    }
    
    return NULL;
}

/**
 * Get memory region information
 * 
 * @param address: Address to query
 * @param region: Pointer to store the region information
 * @return: 0 on success, -1 on failure
 */
int memory_get_region(uint64_t address, memory_region_t* region) {
    // Suppress unused parameter warnings
    (void)address;
    (void)region;
    
    // Requires memory region tracking system implementation
    return -1;
}

/**
 * Set memory protection
 * 
 * @param addr: Address of the memory to protect
 * @param size: Size of the memory to protect
 * @param protection: Memory protection flags
 * @return: 0 on success, -1 on failure
 */
int memory_set_protection(void* addr, size_t size, memory_prot_t protection) {
    // Check if the address is valid
    if (!addr) {
        return -1;
    }
    
    // Round up the size to a multiple of the page size
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    // Set the protection for each page
    for (uint64_t virt_addr = (uintptr_t)addr; virt_addr < (uintptr_t)addr + size; virt_addr += PAGE_SIZE) {
        // Get the page table indices
        uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
        uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
        uint64_t pd_idx = (virt_addr >> 21) & 0x1FF;
        uint64_t pt_idx = (virt_addr >> 12) & 0x1FF;
        
        // Get the PML4 entry
        pte_t* pml4e = &page_table_root[pml4_idx];
        
        if (!(*pml4e & PTE_PRESENT)) {
            // Page not mapped
            return -1;
        }
        
        // Get the PDPT
        pt_t pdpt = (pt_t)(uintptr_t)(*pml4e & ~0xFFF);
        pte_t* pdpte = &pdpt[pdpt_idx];
        
        if (!(*pdpte & PTE_PRESENT)) {
            // Page not mapped
            return -1;
        }
        
        // Get the PD
        pt_t pd = (pt_t)(uintptr_t)(*pdpte & ~0xFFF);
        pte_t* pde = &pd[pd_idx];
        
        if (!(*pde & PTE_PRESENT)) {
            // Page not mapped
            return -1;
        }
        
        // Get the PT
        pt_t pt = (pt_t)(uintptr_t)(*pde & ~0xFFF);
        pte_t* pte = &pt[pt_idx];
        
        if (!(*pte & PTE_PRESENT)) {
            // Page not mapped
            return -1;
        }
        
        // Set the protection
        uint64_t flags = *pte & ~(PTE_WRITABLE | PTE_USER | PTE_NX);
        
        if (protection & MEMORY_PROT_WRITE) {
            flags |= PTE_WRITABLE;
        }
        
        if (protection & MEMORY_PROT_USER) {
            flags |= PTE_USER;
        }
        
        if (!(protection & MEMORY_PROT_EXEC)) {
            flags |= PTE_NX;
        }
        
        // Update the page table entry
        *pte = flags;
        
        // Invalidate the TLB entry
        __asm__ volatile("invlpg (%0)" : : "r"((uint32_t)virt_addr) : "memory");
    }
    
    return 0;
}

/**
 * Get memory protection
 * 
 * @param addr: Address of the memory to query
 * @param protection: Pointer to store the protection flags
 * @return: 0 on success, -1 on failure
 */
int memory_get_protection(void* addr, memory_prot_t* protection) {
    // Check if the address is valid
    if (!addr || !protection) {
        return -1;
    }
    
    // Get the page table indices
    uint64_t virt_addr = (uintptr_t)addr;
    uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_idx = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_idx = (virt_addr >> 12) & 0x1FF;
    
    // Get the PML4 entry
    pte_t* pml4e = &page_table_root[pml4_idx];
    
    if (!(*pml4e & PTE_PRESENT)) {
        // Page not mapped
        return -1;
    }
    
    // Get the PDPT
    pt_t pdpt = (pt_t)(uintptr_t)(*pml4e & ~0xFFF);
    pte_t* pdpte = &pdpt[pdpt_idx];
    
    if (!(*pdpte & PTE_PRESENT)) {
        // Page not mapped
        return -1;
    }
    
    // Get the PD
    pt_t pd = (pt_t)(uintptr_t)(*pdpte & ~0xFFF);
    pte_t* pde = &pd[pd_idx];
    
    if (!(*pde & PTE_PRESENT)) {
        // Page not mapped
        return -1;
    }
    
    // Get the PT
    pt_t pt = (pt_t)(uintptr_t)(*pde & ~0xFFF);
    pte_t* pte = &pt[pt_idx];
    
    if (!(*pte & PTE_PRESENT)) {
        // Page not mapped
        return -1;
    }
    
    // Get the protection
    *protection = 0;
    
    if (*pte & PTE_WRITABLE) {
        *protection |= MEMORY_PROT_WRITE;
    }
    
    if (*pte & PTE_USER) {
        *protection |= MEMORY_PROT_USER;
    }
    
    if (!(*pte & PTE_NX)) {
        *protection |= MEMORY_PROT_EXEC;
    }
    
    return 0;
}

/**
 * Copy memory between address spaces
 * 
 * @param dest: Destination address
 * @param src: Source address
 * @param size: Size of the memory to copy
 */
void memory_copy(void* dest, const void* src, size_t size) {
    // Check if the addresses are valid
    if (!dest || !src) {
        return;
    }
    
    // Copy the memory
    memcpy(dest, src, size);
}

/**
 * Set memory to a value
 * 
 * @param dest: Destination address
 * @param value: Value to set
 * @param size: Size of the memory to set
 */
void memory_set(void* dest, int value, size_t size) {
    // Check if the address is valid
    if (!dest) {
        return;
    }
    
    // Set the memory
    memset(dest, value, size);
}

/**
 * Compare memory
 * 
 * @param s1: First memory address
 * @param s2: Second memory address
 * @param size: Size of the memory to compare
 * @return: 0 if equal, non-zero otherwise
 */
int memory_compare(const void* s1, const void* s2, size_t size) {
    // Check if the addresses are valid
    if (!s1 || !s2) {
        return -1;
    }
    
    // Compare the memory
    return memcmp(s1, s2, size);
}

/**
 * Find a byte in memory
 * 
 * @param s: Memory address
 * @param c: Byte to find
 * @param size: Size of the memory to search
 * @return: Pointer to the byte, NULL if not found
 */
void* memory_find(const void* s, int c, size_t size) {
    // Check if the address is valid
    if (!s) {
        return NULL;
    }
    
    // Find the byte
    return memchr(s, c, size);
}

/**
 * Convert a virtual address to a physical address
 * 
 * @param virtual_addr: Virtual address
 * @return: Physical address, 0 on failure
 */
uint64_t memory_virtual_to_physical(void* virtual_addr) {
    if (!virtual_addr) {
        return 0;
    }
    
    // Get the page table indices
    uintptr_t addr_val = (uintptr_t)virtual_addr;
    uint64_t virt_addr = (uint64_t)addr_val;
    uint64_t offset = virt_addr & 0xFFF;
    uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_idx = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_idx = (virt_addr >> 12) & 0x1FF;
    
    // Get the PML4 entry
    pte_t* pml4e = &page_table_root[pml4_idx];
    
    if (!(*pml4e & PTE_PRESENT)) {
        // Page not mapped
        return 0;
    }
    
    // Get the PDPT
    uintptr_t pdpt_addr = (uintptr_t)(*pml4e & ~0xFFF);
    pt_t pdpt = (pt_t)pdpt_addr;
    pte_t* pdpte = &pdpt[pdpt_idx];
    
    if (!(*pdpte & PTE_PRESENT)) {
        // Page not mapped
        return 0;
    }
    
    // Check if this is a 1GB page
    if (*pdpte & PTE_HUGE) {
        return (*pdpte & ~0x3FFFFFFF) | (virt_addr & 0x3FFFFFFF);
    }
    
    // Get the PD
    uintptr_t pd_addr = (uintptr_t)(*pdpte & ~0xFFF);
    pt_t pd = (pt_t)pd_addr;
    pte_t* pde = &pd[pd_idx];
    
    if (!(*pde & PTE_PRESENT)) {
        // Page not mapped
        return 0;
    }
    
    // Check if this is a 2MB page
    if (*pde & PTE_HUGE) {
        return (*pde & ~0x1FFFFF) | (virt_addr & 0x1FFFFF);
    }
    
    // Get the PT
    uintptr_t pt_addr = (uintptr_t)(*pde & ~0xFFF);
    pt_t pt = (pt_t)pt_addr;
    pte_t* pte = &pt[pt_idx];
    
    if (!(*pte & PTE_PRESENT)) {
        // Page not mapped
        return 0;
    }
    
    // Get the physical address
    return (*pte & ~0xFFF) | offset;
}
