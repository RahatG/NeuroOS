/**
 * process.c - Process management implementation for NeuroOS
 * 
 * This file implements the process management subsystem, which is responsible
 * for creating, scheduling, and terminating processes.
 */

#include "include/process.h"
#include "include/memory.h"
#include "include/console.h"
#include "include/interrupts.h"

// Maximum number of processes
#define MAX_PROCESSES 1024

// Process table
static process_t* process_table[MAX_PROCESSES];

// Current process
static process_t* current_process = NULL;

// Next available process ID
static pid_t next_pid = 1;

// Process scheduler state
static struct {
    int initialized;
    int enabled;
    uint64_t ticks;
    uint64_t quantum;
} scheduler;

// Forward declarations
static void process_scheduler_tick(void);
static void process_switch(process_t* next);
static void process_cleanup(process_t* process);

/**
 * Initialize the process management subsystem
 */
void process_init(void) {
    // Initialize the process table
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i] = NULL;
    }
    
    // Create the initial kernel process
    process_t* kernel_process = (process_t*)memory_alloc(sizeof(process_t), MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_KERNEL | MEMORY_ALLOC_ZEROED);
    
    if (!kernel_process) {
        console_printf("Error: Failed to allocate kernel process\n");
        return;
    }
    
    // Initialize the kernel process
    kernel_process->pid = 0;
    kernel_process->state = PROCESS_STATE_RUNNING;
    kernel_process->priority = PROCESS_PRIORITY_NORMAL;
    kernel_process->flags = PROCESS_FLAG_KERNEL;
    kernel_process->parent = NULL;
    kernel_process->next = NULL;
    kernel_process->prev = NULL;
    
    // Set the process name
    const char* name = "kernel";
    for (int i = 0; i < PROCESS_NAME_MAX - 1 && name[i]; i++) {
        kernel_process->name[i] = name[i];
    }
    
    // Add the kernel process to the process table
    process_table[0] = kernel_process;
    
    // Set the current process
    current_process = kernel_process;
    
    // Initialize the scheduler
    scheduler.initialized = 1;
    scheduler.enabled = 0;
    scheduler.ticks = 0;
    scheduler.quantum = 10; // 10 ms time quantum
    
    // Register the timer interrupt handler
    interrupts_register_irq_handler(IRQ_TIMER, process_scheduler_tick);
    
    console_printf("Process management initialized\n");
}

/**
 * Create a new process
 * 
 * @param name: Process name
 * @param entry_point: Process entry point function
 * @param stack_size: Process stack size
 * @param priority: Process priority
 * @param flags: Process flags
 * @return: Process ID on success, 0 on failure
 */
pid_t process_create(const char* name, void (*entry_point)(void), size_t stack_size,
                    process_priority_t priority, process_flags_t flags) {
    // Check if the process management subsystem is initialized
    if (!scheduler.initialized) {
        console_printf("Error: Process management not initialized\n");
        return 0;
    }
    
    // Check if we have reached the maximum number of processes
    if (next_pid >= MAX_PROCESSES) {
        console_printf("Error: Maximum number of processes reached\n");
        return 0;
    }
    
    // Allocate memory for the process structure
    process_t* process = (process_t*)memory_alloc(sizeof(process_t), MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!process) {
        console_printf("Error: Failed to allocate process structure\n");
        return 0;
    }
    
    // Allocate memory for the process stack
    void* stack = memory_alloc(stack_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!stack) {
        console_printf("Error: Failed to allocate process stack\n");
        memory_free(process, sizeof(process_t));
        return 0;
    }
    
    // Allocate memory for the kernel stack
    void* kernel_stack = memory_alloc(4096, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_KERNEL | MEMORY_ALLOC_ZEROED);
    
    if (!kernel_stack) {
        console_printf("Error: Failed to allocate kernel stack\n");
        memory_free(stack, stack_size);
        memory_free(process, sizeof(process_t));
        return 0;
    }
    
    // Initialize the process
    process->pid = next_pid++;
    process->state = PROCESS_STATE_CREATED;
    process->priority = priority;
    process->flags = flags;
    process->stack = stack;
    process->stack_size = stack_size;
    process->kernel_stack = kernel_stack;
    process->kernel_stack_size = 4096;
    process->parent = current_process;
    process->next = NULL;
    process->prev = NULL;
    process->cpu_time = 0;
    process->creation_time = scheduler.ticks;
    process->exit_code = 0;
    
    // Set the process name
    for (int i = 0; i < PROCESS_NAME_MAX - 1 && name[i]; i++) {
        process->name[i] = name[i];
    }
    
    // Set up the process context
    uint32_t* stack_ptr = (uint32_t*)((uint8_t*)stack + stack_size);
    
    // Push the entry point
    *(--stack_ptr) = (uint32_t)entry_point;
    
    // Push a process termination return address
    *(--stack_ptr) = 0;
    
    // Push initial register values
    *(--stack_ptr) = 0; // EBP
    *(--stack_ptr) = 0; // EBX
    *(--stack_ptr) = 0; // ESI
    *(--stack_ptr) = 0; // EDI
    
    // Set the process context
    process->context.eip = (uint32_t)entry_point;
    process->context.esp = (uint32_t)stack_ptr;
    process->context.ebp = (uint32_t)stack_ptr + 4;
    process->context.ebx = 0;
    process->context.esi = 0;
    process->context.edi = 0;
    
    // Add the process to the process table
    process_table[process->pid] = process;
    
    // Set the process state to ready
    process->state = PROCESS_STATE_READY;
    
    return process->pid;
}

/**
 * Terminate a process
 * 
 * @param pid: Process ID
 * @param exit_code: Process exit code
 * @return: 0 on success, -1 on failure
 */
int process_terminate(pid_t pid, int exit_code) {
    // Check if the process management subsystem is initialized
    if (!scheduler.initialized) {
        console_printf("Error: Process management not initialized\n");
        return -1;
    }
    
    // Check if the PID is valid
    if (pid >= MAX_PROCESSES || !process_table[pid]) {
        console_printf("Error: Invalid process ID\n");
        return -1;
    }
    
    // Get the process
    process_t* process = process_table[pid];
    
    // Check if the process is already terminated
    if (process->state == PROCESS_STATE_TERMINATED) {
        return 0;
    }
    
    // Set the process state to terminated
    process->state = PROCESS_STATE_TERMINATED;
    process->exit_code = exit_code;
    
    // If the process is the current process, schedule another process
    if (process == current_process) {
        process_yield();
    }
    
    // Clean up the process
    process_cleanup(process);
    
    return 0;
}

/**
 * Get the current process
 * 
 * @return: Pointer to the current process
 */
process_t* process_get_current(void) {
    return current_process;
}

/**
 * Get a process by ID
 * 
 * @param pid: Process ID
 * @return: Pointer to the process, or NULL if not found
 */
process_t* process_get_by_id(pid_t pid) {
    // Check if the PID is valid
    if (pid >= MAX_PROCESSES) {
        return NULL;
    }
    
    return process_table[pid];
}

/**
 * Set process priority
 * 
 * @param pid: Process ID
 * @param priority: New process priority
 * @return: 0 on success, -1 on failure
 */
int process_set_priority(pid_t pid, process_priority_t priority) {
    // Check if the process management subsystem is initialized
    if (!scheduler.initialized) {
        console_printf("Error: Process management not initialized\n");
        return -1;
    }
    
    // Check if the PID is valid
    if (pid >= MAX_PROCESSES || !process_table[pid]) {
        console_printf("Error: Invalid process ID\n");
        return -1;
    }
    
    // Get the process
    process_t* process = process_table[pid];
    
    // Set the process priority
    process->priority = priority;
    
    return 0;
}

/**
 * Block a process
 * 
 * @param pid: Process ID
 * @return: 0 on success, -1 on failure
 */
int process_block(pid_t pid) {
    // Check if the process management subsystem is initialized
    if (!scheduler.initialized) {
        console_printf("Error: Process management not initialized\n");
        return -1;
    }
    
    // Check if the PID is valid
    if (pid >= MAX_PROCESSES || !process_table[pid]) {
        console_printf("Error: Invalid process ID\n");
        return -1;
    }
    
    // Get the process
    process_t* process = process_table[pid];
    
    // Check if the process is already blocked or terminated
    if (process->state == PROCESS_STATE_BLOCKED || process->state == PROCESS_STATE_TERMINATED) {
        return 0;
    }
    
    // Set the process state to blocked
    process->state = PROCESS_STATE_BLOCKED;
    
    // If the process is the current process, schedule another process
    if (process == current_process) {
        process_yield();
    }
    
    return 0;
}

/**
 * Unblock a process
 * 
 * @param pid: Process ID
 * @return: 0 on success, -1 on failure
 */
int process_unblock(pid_t pid) {
    // Check if the process management subsystem is initialized
    if (!scheduler.initialized) {
        console_printf("Error: Process management not initialized\n");
        return -1;
    }
    
    // Check if the PID is valid
    if (pid >= MAX_PROCESSES || !process_table[pid]) {
        console_printf("Error: Invalid process ID\n");
        return -1;
    }
    
    // Get the process
    process_t* process = process_table[pid];
    
    // Check if the process is blocked
    if (process->state != PROCESS_STATE_BLOCKED) {
        return 0;
    }
    
    // Set the process state to ready
    process->state = PROCESS_STATE_READY;
    
    return 0;
}

/**
 * Yield the CPU
 */
void process_yield(void) {
    // Check if the process management subsystem is initialized
    if (!scheduler.initialized) {
        return;
    }
    
    // Check if the scheduler is enabled
    if (!scheduler.enabled) {
        return;
    }
    
    // Find the next process to run
    process_t* next = NULL;
    
    // Start from the current process
    process_t* process = current_process->next;
    
    // If we reached the end of the list, start from the beginning
    if (!process) {
        process = process_table[0];
    }
    
    // Find the next ready process
    while (process != current_process) {
        if (process && process->state == PROCESS_STATE_READY) {
            next = process;
            break;
        }
        
        // Move to the next process
        process = process->next;
        
        // If we reached the end of the list, start from the beginning
        if (!process) {
            process = process_table[0];
        }
    }
    
    // If we found a process, switch to it
    if (next) {
        process_switch(next);
    }
}

/**
 * Sleep for the specified number of milliseconds
 * 
 * @param ms: Number of milliseconds to sleep
 */
void process_sleep(uint32_t ms) {
    // Check if the process management subsystem is initialized
    if (!scheduler.initialized) {
        return;
    }
    
    // Check if the scheduler is enabled
    if (!scheduler.enabled) {
        return;
    }
    
    // Calculate the number of ticks to sleep
    uint64_t ticks = ms / 10; // Assuming 10 ms per tick
    
    // Block the current process
    current_process->state = PROCESS_STATE_BLOCKED;
    
    // Schedule another process
    process_yield();
    
    // Sleep for the specified number of ticks
    uint64_t start_ticks = scheduler.ticks;
    while (scheduler.ticks - start_ticks < ticks) {
        // Wait for the next tick
        __asm__ volatile("hlt");
    }
    
    // Unblock the current process
    current_process->state = PROCESS_STATE_READY;
}

/**
 * Get process statistics
 * 
 * @param pid: Process ID
 * @param cpu_time: Pointer to store the CPU time used by the process
 * @param state: Pointer to store the process state
 * @param priority: Pointer to store the process priority
 * @return: 0 on success, -1 on failure
 */
int process_get_stats(pid_t pid, uint64_t* cpu_time, process_state_t* state,
                     process_priority_t* priority) {
    // Check if the process management subsystem is initialized
    if (!scheduler.initialized) {
        console_printf("Error: Process management not initialized\n");
        return -1;
    }
    
    // Check if the PID is valid
    if (pid >= MAX_PROCESSES || !process_table[pid]) {
        console_printf("Error: Invalid process ID\n");
        return -1;
    }
    
    // Get the process
    process_t* process = process_table[pid];
    
    // Store the CPU time
    if (cpu_time) {
        *cpu_time = process->cpu_time;
    }
    
    // Store the process state
    if (state) {
        *state = process->state;
    }
    
    // Store the process priority
    if (priority) {
        *priority = process->priority;
    }
    
    return 0;
}

/**
 * Get the number of processes
 * 
 * @return: Number of processes
 */
size_t process_get_count(void) {
    // Check if the process management subsystem is initialized
    if (!scheduler.initialized) {
        return 0;
    }
    
    // Count the number of processes
    size_t count = 0;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i]) {
            count++;
        }
    }
    
    return count;
}

/**
 * Get the list of processes
 * 
 * @param processes: Array to store the processes
 * @param max_count: Maximum number of processes to store
 * @return: Number of processes stored
 */
size_t process_get_list(process_t* processes, size_t max_count) {
    // Check if the process management subsystem is initialized
    if (!scheduler.initialized) {
        return 0;
    }
    
    // Check if the array is valid
    if (!processes || max_count == 0) {
        return 0;
    }
    
    // Get the list of processes
    size_t count = 0;
    
    for (int i = 0; i < MAX_PROCESSES && count < max_count; i++) {
        if (process_table[i]) {
            processes[count++] = *process_table[i];
        }
    }
    
    return count;
}

/**
 * Process scheduler tick handler
 * 
 * This function is called by the timer interrupt handler.
 */
static void process_scheduler_tick(void) {
    // Increment the tick counter
    scheduler.ticks++;
    
    // Check if the scheduler is enabled
    if (!scheduler.enabled) {
        return;
    }
    
    // Check if the current process is running
    if (current_process && current_process->state == PROCESS_STATE_RUNNING) {
        // Increment the CPU time
        current_process->cpu_time++;
        
        // Check if the time quantum has expired
        if (current_process->cpu_time % scheduler.quantum == 0) {
            // Set the process state to ready
            current_process->state = PROCESS_STATE_READY;
            
            // Schedule another process
            process_yield();
        }
    }
}

/**
 * Switch to a new process
 * 
 * @param next: Process to switch to
 */
static void process_switch(process_t* next) {
    // Check if the process is valid
    if (!next) {
        return;
    }
    
    // Check if the process is ready
    if (next->state != PROCESS_STATE_READY) {
        return;
    }
    
    // Save the current process context
    process_t* prev = current_process;
    
    // Set the new process as the current process
    current_process = next;
    
    // Set the process state to running
    current_process->state = PROCESS_STATE_RUNNING;
    
    // Switch to the new process context
    // Save current context and load the new one
    if (prev != next) {
        // Save the current process context
        __asm__ volatile(
            "pushfl\n"
            "push %%eax\n"
            "push %%ebx\n"
            "push %%ecx\n"
            "push %%edx\n"
            "push %%esi\n"
            "push %%edi\n"
            "push %%ebp\n"
            "movl %%esp, %0\n"
            "movl %1, %%esp\n"
            "pop %%ebp\n"
            "pop %%edi\n"
            "pop %%esi\n"
            "pop %%edx\n"
            "pop %%ecx\n"
            "pop %%ebx\n"
            "pop %%eax\n"
            "popfl\n"
            : "=m" (prev->context.esp)
            : "m" (next->context.esp)
            : "memory"
        );
        
        // Update page directory if needed
        if (prev->page_directory != next->page_directory) {
            memory_switch_page_directory(next->page_directory);
        }
    }
}

/**
 * Clean up a terminated process
 * 
 * @param process: Process to clean up
 */
static void process_cleanup(process_t* process) {
    // Check if the process is valid
    if (!process) {
        return;
    }
    
    // Free the process stack
    if (process->stack) {
        memory_free(process->stack, process->stack_size);
    }
    
    // Free the kernel stack
    if (process->kernel_stack) {
        memory_free(process->kernel_stack, process->kernel_stack_size);
    }
    
    // Remove the process from the process table
    process_table[process->pid] = NULL;
    
    // Free the process structure
    memory_free(process, sizeof(process_t));
}
