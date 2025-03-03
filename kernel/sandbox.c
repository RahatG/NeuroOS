/**
 * sandbox.c - Sandboxing implementation for NeuroOS
 * 
 * This file implements the sandboxing subsystem, which is responsible for
 * creating isolated environments where the AI can safely test code modifications
 * without affecting the running system.
 */

#include "include/sandbox.h"
#include "include/memory.h"
#include "include/process.h"
#include "include/console.h"
#include "include/backup.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>

// Forward declarations for functions not in headers
int process_get_file_path(int pid, int fd, char* path, size_t size);
int process_is_fd_open(int pid, int fd);
int process_open_file(int pid, const char* path, int fd);
int process_close_socket(int pid, int socket);
int process_close_device(int pid, int device);
int process_get_syscall_history(int pid, void* syscalls, int* count);
int process_get_sockets(int pid, void* sockets, int* count);
int process_get_file_access_history(int pid, void* accesses, int* count);
int process_get_memory_violations(int pid, void* violations, int* count);
int process_get_state(int pid, void* state);

// Define additional flags not in sandbox.h
#define SANDBOX_FLAG_RESTRICTED_ENV 0x00100000

// Define file descriptors if not already defined
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

// File access modes
#define FILE_ACCESS_READ    1
#define FILE_ACCESS_WRITE   2
#define FILE_ACCESS_CREATE  3
#define FILE_ACCESS_DELETE  4
#define FILE_ACCESS_EXECUTE 5

// Memory violation types
#define MEMORY_VIOLATION_READ  1
#define MEMORY_VIOLATION_WRITE 2
#define MEMORY_VIOLATION_EXEC  3

// Network connection structure
typedef struct {
    int socket;
    uint32_t local_addr;
    uint16_t local_port;
    uint32_t remote_addr;
    uint16_t remote_port;
    uint8_t protocol;
    uint8_t state;
} network_connection_t;

// File access structure
typedef struct {
    char path[256];
    int mode;
    uint64_t timestamp;
    uint32_t process_id;
} file_access_t;

// Memory violation structure
typedef struct {
    uint64_t address;
    int type;
    uint64_t timestamp;
    uint32_t process_id;
} memory_violation_t;

// Environment variable structure
typedef struct {
    char name[64];
    char value[256];
} env_var_t;

// Process state structure for sandbox
typedef struct {
    uint32_t pid;
    uint32_t ppid;
    uint32_t priority;
    uint32_t state;
    uint64_t start_time;
    uint64_t cpu_time;
    uint64_t memory_usage;
    char name[64];
    char cwd[256];
    int num_env_vars;
    env_var_t env_vars[32];
} sandbox_process_state_t;

// Maximum number of sandboxes
#define MAX_SANDBOXES 16

// Internal sandbox structure
typedef struct {
    sandbox_id_t id;
    uint32_t state;
    uint32_t flags;
    char name[64];
    sandbox_config_t config;
    sandbox_state_t stats;
    int initial_process;
    void* memory_snapshot;
    void* filesystem_snapshot;
    void* network_snapshot;
    void* device_snapshot;
} sandbox_t;

// Sandbox table
static sandbox_t* sandbox_table[MAX_SANDBOXES];

// Next available sandbox ID
static sandbox_id_t next_sandbox_id = 1;

// Forward declarations
static int sandbox_check_limits(sandbox_id_t id);
static int sandbox_create_snapshots(sandbox_id_t id);
static int sandbox_restore_snapshots(sandbox_id_t id);
static int sandbox_check_syscalls(sandbox_id_t id);
static int sandbox_check_network_access(sandbox_id_t id);
static int sandbox_check_file_access(sandbox_id_t id);
static int sandbox_check_memory_access(sandbox_id_t id);
static int sandbox_get_modified_files(sandbox_id_t id, char*** modified_files, size_t* num_modified_files);
static int sandbox_apply_process_changes(sandbox_id_t id);
static int sandbox_apply_network_changes(sandbox_id_t id);

/**
 * Initialize the sandboxing subsystem
 */
int sandbox_init(void) {
    // Initialize the sandbox table
    for (int i = 0; i < MAX_SANDBOXES; i++) {
        sandbox_table[i] = NULL;
    }
    
    console_printf("Sandboxing initialized\n");
    return 0;
}

/**
 * Create a new sandbox
 * 
 * @param config: Sandbox configuration
 * @return: Sandbox ID on success, 0 on failure
 */
uint32_t sandbox_create(const sandbox_config_t* config) {
    // Check if we have reached the maximum number of sandboxes
    if (next_sandbox_id >= MAX_SANDBOXES) {
        console_printf("Error: Maximum number of sandboxes reached\n");
        return 0;
    }
    
    // Check if the config is valid
    if (!config) {
        console_printf("Error: Invalid sandbox configuration\n");
        return 0;
    }
    
    // Allocate memory for the sandbox structure
    sandbox_t* sandbox = (sandbox_t*)memory_alloc(sizeof(sandbox_t), MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!sandbox) {
        console_printf("Error: Failed to allocate sandbox structure\n");
        return 0;
    }
    
    // Initialize the sandbox
    sandbox->id = next_sandbox_id++;
    sandbox->state = SANDBOX_STATE_CREATED;
    sandbox->flags = config->flags;
    
    // Set the sandbox name
    strncpy(sandbox->name, config->name, sizeof(sandbox->name) - 1);
    sandbox->name[sizeof(sandbox->name) - 1] = '\0'; // Ensure null termination
    
    // Copy the configuration
    memcpy(&sandbox->config, config, sizeof(sandbox_config_t));
    
    // Initialize the resource usage statistics
    memset(&sandbox->stats, 0, sizeof(sandbox_state_t));
    sandbox->stats.id = sandbox->id;
    sandbox->stats.type = config->type;
    sandbox->stats.flags = config->flags;
    sandbox->stats.state = SANDBOX_STATE_CREATED;
    
    // Add the sandbox to the sandbox table
    sandbox_table[sandbox->id] = sandbox;
    
    return sandbox->id;
}

/**
 * Destroy a sandbox
 * 
 * @param id: Sandbox ID
 * @return: 0 on success, -1 on failure
 */
int sandbox_destroy(uint32_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Check if the sandbox is running
    if (sandbox->state == SANDBOX_STATE_RUNNING) {
        // Terminate the sandbox
        sandbox_terminate(id);
    }
    
    // Free the memory snapshots
    if (sandbox->memory_snapshot) {
        memory_free(sandbox->memory_snapshot, 0); // Size is unknown
    }
    
    // Free the filesystem snapshots
    if (sandbox->filesystem_snapshot) {
        memory_free(sandbox->filesystem_snapshot, 0); // Size is unknown
    }
    
    // Free the network snapshots
    if (sandbox->network_snapshot) {
        memory_free(sandbox->network_snapshot, 0); // Size is unknown
    }
    
    // Free the device snapshots
    if (sandbox->device_snapshot) {
        memory_free(sandbox->device_snapshot, 0); // Size is unknown
    }
    
    // Remove the sandbox from the sandbox table
    sandbox_table[id] = NULL;
    
    // Free the sandbox structure
    memory_free(sandbox, sizeof(sandbox_t));
    
    return 0;
}

/**
 * Start a sandbox
 * 
 * @param id: Sandbox ID
 * @return: 0 on success, -1 on failure
 */
int sandbox_start(uint32_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Check if the sandbox is already running
    if (sandbox->state == SANDBOX_STATE_RUNNING) {
        console_printf("Error: Sandbox is already running\n");
        return -1;
    }
    
    // Create snapshots for rollback
    if (sandbox_create_snapshots(id) != 0) {
        console_printf("Error: Failed to create snapshots\n");
        return -1;
    }
    
    // Create a process for the sandbox
    uint32_t process_flags = PROCESS_FLAG_KERNEL;
    
    // Create the process
    int pid = process_create(sandbox->name, NULL, 4096, PROCESS_PRIORITY_NORMAL, process_flags);
    
    if (pid == 0) {
        console_printf("Error: Failed to create sandbox process\n");
        return -1;
    }
    
    // Store the process ID
    sandbox->initial_process = pid;
    
    // Set the sandbox state to running
    sandbox->state = SANDBOX_STATE_RUNNING;
    
    return 0;
}

/**
 * Pause a sandbox
 * 
 * @param id: Sandbox ID
 * @return: 0 on success, -1 on failure
 */
int sandbox_pause(uint32_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Check if the sandbox is running
    if (sandbox->state != SANDBOX_STATE_RUNNING) {
        console_printf("Error: Sandbox is not running\n");
        return -1;
    }
    
    // Suspend the sandbox process
    if (process_suspend(sandbox->initial_process) != 0) {
        console_printf("Error: Failed to suspend sandbox process\n");
        return -1;
    }
    
    // Set the sandbox state to paused
    sandbox->state = SANDBOX_STATE_PAUSED;
    
    return 0;
}

/**
 * Resume a sandbox
 * 
 * @param id: Sandbox ID
 * @return: 0 on success, -1 on failure
 */
int sandbox_resume(uint32_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Check if the sandbox is paused
    if (sandbox->state != SANDBOX_STATE_PAUSED) {
        console_printf("Error: Sandbox is not paused\n");
        return -1;
    }
    
    // Resume the sandbox process
    if (process_resume(sandbox->initial_process) != 0) {
        console_printf("Error: Failed to resume sandbox process\n");
        return -1;
    }
    
    // Set the sandbox state to running
    sandbox->state = SANDBOX_STATE_RUNNING;
    
    return 0;
}

/**
 * Terminate a sandbox
 * 
 * @param id: Sandbox ID
 * @return: 0 on success, -1 on failure
 */
int sandbox_terminate(uint32_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Check if the sandbox is running or paused
    if (sandbox->state != SANDBOX_STATE_RUNNING && sandbox->state != SANDBOX_STATE_PAUSED) {
        console_printf("Error: Sandbox is not running or paused\n");
        return -1;
    }
    
    // Terminate the sandbox process
    if (process_terminate(sandbox->initial_process, 0) != 0) {
        console_printf("Error: Failed to terminate sandbox process\n");
        return -1;
    }
    
    // Set the sandbox state to terminated
    sandbox->state = SANDBOX_STATE_TERMINATED;
    
    return 0;
}

/**
 * Get sandbox state
 * 
 * @param id: Sandbox ID
 * @param state: Pointer to store the sandbox state
 * @return: 0 on success, -1 on failure
 */
int sandbox_get_state(uint32_t id, sandbox_state_t* state) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Check if the state pointer is valid
    if (!state) {
        console_printf("Error: Invalid state pointer\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Copy the state information
    memcpy(state, &sandbox->stats, sizeof(sandbox_state_t));
    
    return 0;
}

/**
 * Get sandbox configuration
 * 
 * @param id: Sandbox ID
 * @param config: Pointer to store the configuration
 * @return: 0 on success, -1 on failure
 */
int sandbox_get_config(uint32_t id, sandbox_config_t* config) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Check if the config pointer is valid
    if (!config) {
        console_printf("Error: Invalid config pointer\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Copy the configuration
    memcpy(config, &sandbox->config, sizeof(sandbox_config_t));
    
    return 0;
}

/**
 * Set sandbox configuration
 * 
 * @param id: Sandbox ID
 * @param config: New configuration
 * @return: 0 on success, -1 on failure
 */
int sandbox_set_config(uint32_t id, const sandbox_config_t* config) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Check if the config pointer is valid
    if (!config) {
        console_printf("Error: Invalid config pointer\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Set the configuration
    memcpy(&sandbox->config, config, sizeof(sandbox_config_t));
    
    return 0;
}

/**
 * Create a memory snapshot for rollback
 * 
 * @param id: Sandbox ID
 * @return: 0 on success, -1 on failure
 */
static int sandbox_create_memory_snapshot(sandbox_id_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Free the previous snapshot if it exists
    if (sandbox->memory_snapshot) {
        memory_free(sandbox->memory_snapshot, 0); // Size is unknown
        sandbox->memory_snapshot = NULL;
    }
    
    // Get the process memory regions
    memory_region_t* regions = NULL;
    int num_regions = 0;
    
    // Get memory regions from the process
    if (process_get_memory_regions(sandbox->initial_process, regions, num_regions) != 0) {
        console_printf("Error: Failed to get memory regions for process %d\n", sandbox->initial_process);
        return -1;
    }
    
    // Calculate the total size needed for the snapshot
    size_t total_size = sizeof(int); // Store the number of regions
    
    for (int i = 0; i < num_regions; i++) {
        total_size += sizeof(memory_region_t); // Region metadata
        total_size += regions[i].size; // Region data
    }
    
    // Allocate memory for the snapshot
    sandbox->memory_snapshot = memory_alloc(total_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!sandbox->memory_snapshot) {
        console_printf("Error: Failed to allocate memory snapshot\n");
        // Free the regions
        if (regions) {
            memory_free(regions, num_regions * sizeof(memory_region_t));
        }
        return -1;
    }
    
    // Store the number of regions at the beginning of the snapshot
    *((int*)sandbox->memory_snapshot) = num_regions;
    
    // Copy the region metadata and data
    char* snapshot_ptr = (char*)sandbox->memory_snapshot + sizeof(int);
    
    for (int i = 0; i < num_regions; i++) {
        // Copy the region metadata
        memory_region_t* region_metadata = (memory_region_t*)snapshot_ptr;
        *region_metadata = regions[i];
        snapshot_ptr += sizeof(memory_region_t);
        
        // Copy the region data
        if (regions[i].flags & MEMORY_PROT_READ) {
            memcpy(snapshot_ptr, (void*)(uintptr_t)regions[i].start, regions[i].size);
        } else {
            // If the region is not readable, just zero it out
            memset(snapshot_ptr, 0, regions[i].size);
        }
        snapshot_ptr += regions[i].size;
    }
    
    // Free the regions
    memory_free(regions, num_regions * sizeof(memory_region_t));
    
    console_printf("Created memory snapshot for sandbox %u with %d regions (%zu bytes)\n", 
                  id, num_regions, total_size);
    
    return 0;
}

/**
 * Rollback to a memory snapshot
 * 
 * @param id: Sandbox ID
 * @return: 0 on success, -1 on failure
 */
static int sandbox_rollback_memory(sandbox_id_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Check if a snapshot exists
    if (!sandbox->memory_snapshot) {
        console_printf("Error: No memory snapshot available\n");
        return -1;
    }
    
    // Get the number of regions from the snapshot
    int num_regions = *((int*)sandbox->memory_snapshot);
    
    // Restore each memory region
    char* snapshot_ptr = (char*)sandbox->memory_snapshot + sizeof(int);
    
    for (int i = 0; i < num_regions; i++) {
        // Get the region metadata
        memory_region_t* region_metadata = (memory_region_t*)snapshot_ptr;
        snapshot_ptr += sizeof(memory_region_t);
        
        // Check if the region is writable
        if (region_metadata->flags & MEMORY_PROT_WRITE) {
            // Restore the region data
            memcpy((void*)(uintptr_t)region_metadata->start, snapshot_ptr, region_metadata->size);
        }
        
        // Move to the next region
        snapshot_ptr += region_metadata->size;
    }
    
    console_printf("Restored memory snapshot for sandbox %u with %d regions\n", id, num_regions);
    
    return 0;
}

/**
 * Create a filesystem snapshot for rollback
 * 
 * @param id: Sandbox ID
 * @return: 0 on success, -1 on failure
 */
static int sandbox_create_filesystem_snapshot(sandbox_id_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Free the previous snapshot if it exists
    if (sandbox->filesystem_snapshot) {
        memory_free(sandbox->filesystem_snapshot, 0); // Size is unknown
        sandbox->filesystem_snapshot = NULL;
    }
    
    // Get the process file descriptors
    int* fds = NULL;
    int num_fds = 0;
    
    // Get file descriptors count first
    if (process_get_file_descriptors(sandbox->initial_process, NULL, 0) != 0) {
        console_printf("Error: Failed to get file descriptor count for process %d\n", sandbox->initial_process);
        return -1;
    }
    
    // We need to determine num_fds another way since the function doesn't return it
    num_fds = process_get_file_descriptor_count(sandbox->initial_process);
    if (num_fds < 0) {
        console_printf("Error: Failed to get file descriptor count for process %d\n", sandbox->initial_process);
        return -1;
    }
    
    // Allocate memory for file descriptors
    if (num_fds > 0) {
        fds = (int*)memory_alloc(num_fds * sizeof(int), MEMORY_PROT_READ | MEMORY_PROT_WRITE, 0);
        if (!fds) {
            console_printf("Error: Failed to allocate memory for file descriptors\n");
            return -1;
        }
        
        // Get the actual file descriptors
        if (process_get_file_descriptors(sandbox->initial_process, fds, num_fds) != 0) {
            console_printf("Error: Failed to get file descriptors for process %d\n", sandbox->initial_process);
            memory_free(fds, num_fds * sizeof(int));
            return -1;
        }
    }
    
    // Calculate the total size needed for the snapshot
    size_t total_size = sizeof(int); // Store the number of file descriptors
    
    // Add space for file descriptor metadata and content
    for (int i = 0; i < num_fds; i++) {
        // Get file information
        char path[256];
        size_t file_size = 0;
        
        if (process_get_file_path(sandbox->initial_process, fds[i], path, sizeof(path)) == 0) {
            // Get file size
            FILE* file = fopen(path, "rb");
            if (file) {
                fseek(file, 0, SEEK_END);
                file_size = ftell(file);
                fclose(file);
            }
        }
        
        total_size += sizeof(int); // File descriptor
        total_size += sizeof(size_t); // File size
        total_size += 256; // Path
        total_size += file_size; // File content
    }
    
    // Allocate memory for the snapshot
    sandbox->filesystem_snapshot = memory_alloc(total_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!sandbox->filesystem_snapshot) {
        console_printf("Error: Failed to allocate filesystem snapshot\n");
        // Free the file descriptors
        if (fds) {
            memory_free(fds, num_fds * sizeof(int));
        }
        return -1;
    }
    
    // Store the number of file descriptors at the beginning of the snapshot
    *((int*)sandbox->filesystem_snapshot) = num_fds;
    
    // Copy the file descriptor metadata and content
    char* snapshot_ptr = (char*)sandbox->filesystem_snapshot + sizeof(int);
    
    for (int i = 0; i < num_fds; i++) {
        // Store the file descriptor
        *((int*)snapshot_ptr) = fds[i];
        snapshot_ptr += sizeof(int);
        
        // Get file information
        char path[256];
        size_t file_size = 0;
        
        if (process_get_file_path(sandbox->initial_process, fds[i], path, sizeof(path)) == 0) {
            // Get file size
            FILE* file = fopen(path, "rb");
            if (file) {
                fseek(file, 0, SEEK_END);
                file_size = ftell(file);
                fseek(file, 0, SEEK_SET);
                
                // Store the file size
                *((size_t*)snapshot_ptr) = file_size;
                snapshot_ptr += sizeof(size_t);
                
                // Store the file path
                strncpy(snapshot_ptr, path, 256);
                snapshot_ptr += 256;
                
                // Read and store the file content
                if (file_size > 0) {
                    size_t bytes_read = fread(snapshot_ptr, 1, file_size, file);
                    if (bytes_read < file_size) {
                        console_printf("Warning: Only read %zu of %zu bytes from file\n", bytes_read, file_size);
                    }
                    snapshot_ptr += file_size;
                }
                
                fclose(file);
            } else {
                // No file content
                *((size_t*)snapshot_ptr) = 0;
                snapshot_ptr += sizeof(size_t);
                
                // Store the file path
                strncpy(snapshot_ptr, path, 256);
                snapshot_ptr += 256;
            }
        } else {
            // No file path
            *((size_t*)snapshot_ptr) = 0;
            snapshot_ptr += sizeof(size_t);
            
            // Empty path
            memset(snapshot_ptr, 0, 256);
            snapshot_ptr += 256;
        }
    }
    
    // Free the file descriptors
    memory_free(fds, num_fds * sizeof(int));
    
    console_printf("Created filesystem snapshot for sandbox %u with %d file descriptors (%zu bytes)\n", 
                  id, num_fds, total_size);
    
    return 0;
}

/**
 * Rollback to a filesystem snapshot
 * 
 * @param id: Sandbox ID
 * @return: 0 on success, -1 on failure
 */
static int sandbox_rollback_filesystem(sandbox_id_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Check if a snapshot exists
    if (!sandbox->filesystem_snapshot) {
        console_printf("Error: No filesystem snapshot available\n");
        return -1;
    }
    
    // Get the number of file descriptors from the snapshot
    int num_fds = *((int*)sandbox->filesystem_snapshot);
    
    // Restore each file
    char* snapshot_ptr = (char*)sandbox->filesystem_snapshot + sizeof(int);
    
    for (int i = 0; i < num_fds; i++) {
        // Get the file descriptor
        int fd = *((int*)snapshot_ptr);
        snapshot_ptr += sizeof(int);
        
        // Get the file size
        size_t file_size = *((size_t*)snapshot_ptr);
        snapshot_ptr += sizeof(size_t);
        
        // Get the file path
        char path[256];
        strncpy(path, snapshot_ptr, 255);
        path[255] = '\0'; // Ensure null termination
        snapshot_ptr += 256;
        
        // If the file has content, restore it
        if (file_size > 0) {
            // Create or truncate the file
            FILE* file = fopen(path, "wb");
            if (file) {
                // Write the file content
                fwrite(snapshot_ptr, 1, file_size, file);
                fclose(file);
            }
            
            // Move to the next file
            snapshot_ptr += file_size;
        }
        
        // Restore the file descriptor if needed
        if (process_is_fd_open(sandbox->initial_process, fd) == 0) {
            // File descriptor is not open, reopen it
            if (strlen(path) > 0) {
                process_open_file(sandbox->initial_process, path, fd);
            }
        }
    }
    
    console_printf("Restored filesystem snapshot for sandbox %u with %d file descriptors\n", id, num_fds);
    
    return 0;
}

/**
 * Execute code in a sandbox
 * 
 * @param id: Sandbox ID
 * @param code: Code to execute
 * @param code_size: Size of the code
 * @param result: Pointer to store the result
 * @param result_size: Size of the result buffer
 * @return: 0 on success, -1 on failure
 */
int sandbox_exec(uint32_t id, const char* command, const char* args __attribute__((unused))) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Check if the command is valid
    if (!command) {
        console_printf("Error: Invalid command\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Check if the sandbox is running
    if (sandbox->state != SANDBOX_STATE_RUNNING) {
        console_printf("Error: Sandbox is not running\n");
        return -1;
    }
    
    // Execute the command in the sandbox process
    int pipe_fd[2];
    if (pipe(pipe_fd) != 0) {
        console_printf("Error: Failed to create pipe\n");
        return -1;
    }
    
    // Fork a new process within the sandbox
    pid_t child_pid = fork();
    
    if (child_pid < 0) {
        // Fork failed
        console_printf("Error: Failed to fork process\n");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return -1;
    } else if (child_pid == 0) {
        // Child process
        
        // Redirect stdout and stderr to the pipe
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        dup2(pipe_fd[1], STDERR_FILENO);
        close(pipe_fd[1]);
        
        // Set up environment for the sandbox
        if (sandbox->config.flags & SANDBOX_FLAG_RESTRICTED_ENV) {
            // Clear environment and set only essential variables
            clearenv();
            setenv("PATH", "/bin:/usr/bin", 1);
            setenv("HOME", "/tmp", 1);
            setenv("USER", "sandbox", 1);
            setenv("SHELL", "/bin/sh", 1);
        }
        
        // Apply resource limits
        if (sandbox->config.cpu_limit > 0) {
            // Set CPU limit
            struct rlimit rlim;
            rlim.rlim_cur = sandbox->config.cpu_limit;
            rlim.rlim_max = sandbox->config.cpu_limit;
            setrlimit(RLIMIT_CPU, &rlim);
        }
        
        if (sandbox->config.memory_limit > 0) {
            // Set memory limit
            struct rlimit rlim;
            rlim.rlim_cur = sandbox->config.memory_limit;
            rlim.rlim_max = sandbox->config.memory_limit;
            setrlimit(RLIMIT_AS, &rlim);
        }
        
        // Execute the command
        execl("/bin/sh", "sh", "-c", command, NULL);
        
        // If execl returns, it failed
        fprintf(stderr, "Error: Failed to execute command: %s\n", command);
        exit(1);
    } else {
        // Parent process
        
        // Close the write end of the pipe
        close(pipe_fd[1]);
        
        // Wait for the child process to exit
        int status;
        waitpid(child_pid, &status, 0);
        
        // Read any output from the pipe
        char buffer[4096];
        ssize_t bytes_read;
        
        while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            console_printf("Sandbox output: %s", buffer);
        }
        
        // Close the read end of the pipe
        close(pipe_fd[0]);
        
        // Check if the command succeeded
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return -1;
        }
    }
}

/**
 * Check if a sandbox is safe to commit
 * 
 * @param id: Sandbox ID
 * @param is_safe: Pointer to store the result (1 if safe, 0 if not)
 * @return: 0 on success, -1 on failure
 */
int sandbox_check_safety(sandbox_id_t id, int* is_safe) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Check if the is_safe pointer is valid
    if (!is_safe) {
        console_printf("Error: Invalid is_safe pointer\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Check if the sandbox is running or terminated
    if (sandbox->state != SANDBOX_STATE_RUNNING && sandbox->state != SANDBOX_STATE_TERMINATED) {
        console_printf("Error: Sandbox is not running or terminated\n");
        return -1;
    }
    
    // Check if the sandbox has exceeded its resource limits
    if (sandbox_check_limits(id) != 0) {
        *is_safe = 0;
        return 0;
    }
    
    // Check for suspicious system calls
    if (sandbox_check_syscalls(id) != 0) {
        *is_safe = 0;
        return 0;
    }
    
    // Check for network access violations
    if (sandbox_check_network_access(id) != 0) {
        *is_safe = 0;
        return 0;
    }
    
    // Check for file access violations
    if (sandbox_check_file_access(id) != 0) {
        *is_safe = 0;
        return 0;
    }
    
    // Check for memory access violations
    if (sandbox_check_memory_access(id) != 0) {
        *is_safe = 0;
        return 0;
    }
    
    *is_safe = 1;
    
    return 0;
}

/**
 * Commit sandbox changes
 * 
 * @param id: Sandbox ID
 * @return: 0 on success, -1 on failure
 */
int sandbox_commit(sandbox_id_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Check if the sandbox is running or terminated
    if (sandbox->state != SANDBOX_STATE_RUNNING && sandbox->state != SANDBOX_STATE_TERMINATED) {
        console_printf("Error: Sandbox is not running or terminated\n");
        return -1;
    }
    
    // Check if the sandbox is safe to commit
    int is_safe;
    if (sandbox_check_safety(id, &is_safe) != 0 || !is_safe) {
        console_printf("Error: Sandbox is not safe to commit\n");
        return -1;
    }
    
    // Get the list of modified files
    char** modified_files = NULL;
    size_t num_modified_files = 0;
    
    if (sandbox_get_modified_files(id, &modified_files, &num_modified_files) != 0) {
        console_printf("Error: Failed to get modified files\n");
        return -1;
    }
    
    // Copy the modified files to the main system
    for (size_t i = 0; i < num_modified_files; i++) {
        char sandbox_path[512];
        char system_path[512];
        
        snprintf(sandbox_path, sizeof(sandbox_path), "/sandbox/%u/%s", id, modified_files[i]);
        snprintf(system_path, sizeof(system_path), "/%s", modified_files[i]);
        
        // Copy the file from sandbox to system
        FILE *src = fopen(sandbox_path, "rb");
        FILE *dst = fopen(system_path, "wb");
        
        if (src && dst) {
            char buffer[8192];
            size_t bytes_read;
            
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                fwrite(buffer, 1, bytes_read, dst);
            }
            
            fclose(src);
            fclose(dst);
            
            // Preserve file permissions
            struct stat st;
            if (stat(sandbox_path, &st) == 0) {
                chmod(system_path, st.st_mode & 0777);
            }
            
            console_printf("Copied file from %s to %s\n", sandbox_path, system_path);
        } else {
            if (src) fclose(src);
            if (dst) fclose(dst);
            console_printf("Error: Failed to copy file from %s to %s\n", sandbox_path, system_path);
        }
        
        // Free the file path
        free(modified_files[i]);
    }
    
    // Free the modified files list
    free(modified_files);
    
    // Apply any process state changes
    if (sandbox_apply_process_changes(id) != 0) {
        console_printf("Error: Failed to apply process changes\n");
        return -1;
    }
    
    // Apply any network state changes
    if (sandbox_apply_network_changes(id) != 0) {
        console_printf("Error: Failed to apply network changes\n");
        return -1;
    }
    
    return 0;
}

/**
 * Rollback sandbox changes
 * 
 * @param id: Sandbox ID
 * @return: 0 on success, -1 on failure
 */
int sandbox_rollback(sandbox_id_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Check if the sandbox is running or terminated
    if (sandbox->state != SANDBOX_STATE_RUNNING && sandbox->state != SANDBOX_STATE_TERMINATED) {
        console_printf("Error: Sandbox is not running or terminated\n");
        return -1;
    }
    
    // Rollback the changes
    if (sandbox_restore_snapshots(id) != 0) {
        console_printf("Error: Failed to restore snapshots\n");
        return -1;
    }
    
    return 0;
}

/**
 * Check if a sandbox has exceeded its resource limits
 * 
 * @param id: Sandbox ID
 * @return: 0 if within limits, -1 if exceeded
 */
static int sandbox_check_limits(sandbox_id_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Check memory usage
    if (sandbox->stats.memory_usage > sandbox->config.memory_limit) {
        return -1;
    }
    
    // Check process count
    if (sandbox->stats.process_count > sandbox->config.process_limit) {
        return -1;
    }
    
    // Check file count
    if (sandbox->stats.file_count > sandbox->config.file_limit) {
        return -1;
    }
    
    // Check network connection count
    if (sandbox->stats.socket_count > sandbox->config.socket_limit) {
        return -1;
    }
    
    return 0;
}

/**
 * Create snapshots for rollback
 * 
 * @param id: Sandbox ID
 * @return: 0 on success, -1 on failure
 */
static int sandbox_create_snapshots(sandbox_id_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    // Create memory snapshot
    if (sandbox_create_memory_snapshot(id) != 0) {
        return -1;
    }
    
    // Create filesystem snapshot
    if (sandbox_create_filesystem_snapshot(id) != 0) {
        return -1;
    }
    
    // Create network snapshot
    network_connection_t* connections = NULL;
    int num_connections = 0;
    
    if (process_get_network_connections(sandbox->initial_process, &connections, &num_connections) == 0) {
        // Calculate size needed for network snapshot
        size_t snapshot_size = sizeof(int) + (num_connections * sizeof(network_connection_t));
        
        // Allocate memory for the snapshot
        sandbox->network_snapshot = memory_alloc(snapshot_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
        
        if (sandbox->network_snapshot) {
            // Store the number of connections
            *((int*)sandbox->network_snapshot) = num_connections;
            
            // Copy the connections
            if (num_connections > 0) {
                memcpy(
                    (char*)sandbox->network_snapshot + sizeof(int),
                    connections,
                    num_connections * sizeof(network_connection_t)
                );
            }
            
            console_printf("Created network snapshot with %d connections\n", num_connections);
        }
        
        // Free the connections array
        memory_free(connections, num_connections * sizeof(network_connection_t));
    }
    
    // Create device snapshot
    // Get list of open devices
    int* devices = NULL;
    int num_devices = 0;
    
    if (process_get_open_devices(sandbox->initial_process, &devices, &num_devices) == 0) {
        // Calculate size needed for device snapshot
        size_t snapshot_size = sizeof(int) + (num_devices * sizeof(int));
        
        // Allocate memory for the snapshot
        sandbox->device_snapshot = memory_alloc(snapshot_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
        
        if (sandbox->device_snapshot) {
            // Store the number of devices
            *((int*)sandbox->device_snapshot) = num_devices;
            
            // Copy the device handles
            if (num_devices > 0) {
                memcpy(
                    (char*)sandbox->device_snapshot + sizeof(int),
                    devices,
                    num_devices * sizeof(int)
                );
            }
            
            console_printf("Created device snapshot with %d devices\n", num_devices);
        }
        
        // Free the devices array
        memory_free(devices, num_devices * sizeof(int));
    }
    
    return 0;
}

/**
 * Restore snapshots for rollback
 * 
 * @param id: Sandbox ID
 * @return: 0 on success, -1 on failure
 */
static int sandbox_restore_snapshots(sandbox_id_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    // Restore memory snapshot
    if (sandbox_rollback_memory(id) != 0) {
        return -1;
    }
    
    // Restore filesystem snapshot
    if (sandbox_rollback_filesystem(id) != 0) {
        return -1;
    }
    
    // Restore network snapshot
    if (sandbox->network_snapshot) {
        // Get the number of connections from the snapshot
        int num_connections = *((int*)sandbox->network_snapshot);
        
        if (num_connections > 0) {
            // Get the connections from the snapshot
            network_connection_t* connections = (network_connection_t*)((char*)sandbox->network_snapshot + sizeof(int));
            
            // Close all current connections that weren't in the snapshot
            network_connection_t* current_connections = NULL;
            int num_current_connections = 0;
            
            if (process_get_network_connections(sandbox->initial_process, &current_connections, &num_current_connections) == 0) {
                for (int i = 0; i < num_current_connections; i++) {
                    // Check if this connection was in the snapshot
                    int found = 0;
                    
                    for (int j = 0; j < num_connections; j++) {
                        if (current_connections[i].socket == connections[j].socket) {
                            found = 1;
                            break;
                        }
                    }
                    
                    // If not found in snapshot, close it
                    if (!found) {
                        process_close_socket(sandbox->initial_process, current_connections[i].socket);
                    }
                }
                
                // Free the current connections array
                memory_free(current_connections, num_current_connections * sizeof(network_connection_t));
            }
            
            console_printf("Restored network snapshot with %d connections\n", num_connections);
        }
    }
    
    // Restore device snapshot
    if (sandbox->device_snapshot) {
        // Get the number of devices from the snapshot
        int num_devices = *((int*)sandbox->device_snapshot);
        
        if (num_devices > 0) {
            // Get the devices from the snapshot
            int* devices = (int*)((char*)sandbox->device_snapshot + sizeof(int));
            
            // Close all current devices that weren't in the snapshot
            int* current_devices = NULL;
            int num_current_devices = 0;
            
            if (process_get_open_devices(sandbox->initial_process, &current_devices, &num_current_devices) == 0) {
                for (int i = 0; i < num_current_devices; i++) {
                    // Check if this device was in the snapshot
                    int found = 0;
                    
                    for (int j = 0; j < num_devices; j++) {
                        if (current_devices[i] == devices[j]) {
                            found = 1;
                            break;
                        }
                    }
                    
                    // If not found in snapshot, close it
                    if (!found) {
                        process_close_device(sandbox->initial_process, current_devices[i]);
                    }
                }
                
                // Free the current devices array
                memory_free(current_devices, num_current_devices * sizeof(int));
            }
            
            console_printf("Restored device snapshot with %d devices\n", num_devices);
        }
    }
    
    return 0;
}

/**
 * Check for suspicious system calls
 * 
 * @param id: Sandbox ID
 * @return: 0 if no suspicious calls, -1 if suspicious calls detected
 */
static int sandbox_check_syscalls(sandbox_id_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Get the process syscall history
    int* syscalls = NULL;
    int num_syscalls = 0;
    
    if (process_get_syscall_history(sandbox->initial_process, &syscalls, &num_syscalls) != 0) {
        console_printf("Error: Failed to get syscall history for process %d\n", sandbox->initial_process);
        return -1;
    }
    
    // Define suspicious syscalls
    const int suspicious_syscalls[] = {
        1,   // sys_exit
        2,   // sys_fork
        11,  // sys_execve
        37,  // sys_kill
        38,  // sys_rename
        39,  // sys_mkdir
        40,  // sys_rmdir
        41,  // sys_dup
        57,  // sys_setpriority
        60,  // sys_umask
        83,  // sys_symlink
        87,  // sys_swapon
        88,  // sys_reboot
        90,  // sys_mmap
        91,  // sys_munmap
        105, // sys_setuid
        106, // sys_setgid
        122, // sys_uname
        146, // sys_writev
        153, // sys_chown
        169, // sys_reboot
        283, // sys_kexec_load
    };
    
    const int num_suspicious_syscalls = sizeof(suspicious_syscalls) / sizeof(suspicious_syscalls[0]);
    
    // Check for suspicious syscalls
    for (int i = 0; i < num_syscalls; i++) {
        for (int j = 0; j < num_suspicious_syscalls; j++) {
            if (syscalls[i] == suspicious_syscalls[j]) {
                // Found a suspicious syscall
                console_printf("Warning: Sandbox %u made suspicious syscall %d\n", id, syscalls[i]);
                
                // Free the syscalls array
                memory_free(syscalls, num_syscalls * sizeof(int));
                
                return -1;
            }
        }
    }
    
    // Free the syscalls array
    memory_free(syscalls, num_syscalls * sizeof(int));
    
    return 0;
}

/**
 * Check for network access violations
 * 
 * @param id: Sandbox ID
 * @return: 0 if no violations, -1 if violations detected
 */
static int sandbox_check_network_access(sandbox_id_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Check if network access is allowed
    if (!(sandbox->flags & SANDBOX_FLAG_NETWORK)) {
        // Network access is not allowed, check if any network activity occurred
        
        // Get the process network activity
        int* sockets = NULL;
        int num_sockets = 0;
        
        if (process_get_sockets(sandbox->initial_process, &sockets, &num_sockets) != 0) {
            console_printf("Error: Failed to get sockets for process %d\n", sandbox->initial_process);
            return -1;
        }
        
        // If any sockets are open, this is a violation
        if (num_sockets > 0) {
            console_printf("Warning: Sandbox %u has %d open sockets but network access is not allowed\n", 
                          id, num_sockets);
            
            // Free the sockets array
            memory_free(sockets, num_sockets * sizeof(int));
            
            return -1;
        }
        
        // Free the sockets array
        memory_free(sockets, num_sockets * sizeof(int));
    } else {
        // Network access is allowed, check for suspicious activity
        
        // Get the process network connections
        network_connection_t* connections = NULL;
        int num_connections = 0;
        
        if (process_get_network_connections(sandbox->initial_process, &connections, &num_connections) != 0) {
            console_printf("Error: Failed to get network connections for process %d\n", sandbox->initial_process);
            return -1;
        }
        
        // Define suspicious ports
        const int suspicious_ports[] = {
            21,   // FTP
            22,   // SSH
            23,   // Telnet
            25,   // SMTP
            53,   // DNS
            80,   // HTTP
            443,  // HTTPS
            3306, // MySQL
            5432, // PostgreSQL
            6379, // Redis
            27017 // MongoDB
        };
        
        const int num_suspicious_ports = sizeof(suspicious_ports) / sizeof(suspicious_ports[0]);
        
        // Check for connections to suspicious ports
        for (int i = 0; i < num_connections; i++) {
            for (int j = 0; j < num_suspicious_ports; j++) {
                if (connections[i].remote_port == suspicious_ports[j]) {
                    // Found a suspicious connection
                    console_printf("Warning: Sandbox %u connected to suspicious port %d\n", 
                                  id, connections[i].remote_port);
                    
                    // Free the connections array
                    memory_free(connections, num_connections * sizeof(network_connection_t));
                    
                    return -1;
                }
            }
        }
        
        // Free the connections array
        memory_free(connections, num_connections * sizeof(network_connection_t));
    }
    
    return 0;
}

/**
 * Check for file access violations
 * 
 * @param id: Sandbox ID
 * @return: 0 if no violations, -1 if violations detected
 */
static int sandbox_check_file_access(sandbox_id_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Get the process file access history
    file_access_t* accesses = NULL;
    int num_accesses = 0;
    
    if (process_get_file_access_history(sandbox->initial_process, &accesses, &num_accesses) != 0) {
        console_printf("Error: Failed to get file access history for process %d\n", sandbox->initial_process);
        return -1;
    }
    
    // Define sensitive directories
    const char* sensitive_dirs[] = {
        "/etc",
        "/var",
        "/usr",
        "/bin",
        "/sbin",
        "/lib",
        "/boot",
        "/dev",
        "/proc",
        "/sys",
        "/root"
    };
    
    const int num_sensitive_dirs = sizeof(sensitive_dirs) / sizeof(sensitive_dirs[0]);
    
    // Check for access to sensitive directories
    for (int i = 0; i < num_accesses; i++) {
        // Skip read-only access if filesystem access is allowed
        if ((sandbox->flags & SANDBOX_FLAG_FILESYSTEM) && 
            (accesses[i].mode == FILE_ACCESS_READ)) {
            continue;
        }
        
        // Check if the file is in a sensitive directory
        for (int j = 0; j < num_sensitive_dirs; j++) {
            if (strncmp(accesses[i].path, sensitive_dirs[j], strlen(sensitive_dirs[j])) == 0) {
                // Found access to a sensitive directory
                console_printf("Warning: Sandbox %u accessed sensitive file %s with mode %d\n", 
                              id, accesses[i].path, accesses[i].mode);
                
                // Free the accesses array
                memory_free(accesses, num_accesses * sizeof(file_access_t));
                
                return -1;
            }
        }
    }
    
    // Free the accesses array
    memory_free(accesses, num_accesses * sizeof(file_access_t));
    
    return 0;
}

/**
 * Check for memory access violations
 * 
 * @param id: Sandbox ID
 * @return: 0 if no violations, -1 if violations detected
 */
static int sandbox_check_memory_access(sandbox_id_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Get the process memory access violations
    memory_violation_t* violations = NULL;
    int num_violations = 0;
    
    if (process_get_memory_violations(sandbox->initial_process, &violations, &num_violations) != 0) {
        console_printf("Error: Failed to get memory violations for process %d\n", sandbox->initial_process);
        return -1;
    }
    
    // Check if there are any violations
    if (num_violations > 0) {
        console_printf("Warning: Sandbox %u has %d memory access violations\n", id, num_violations);
        
        // Log the violations
        for (int i = 0; i < num_violations; i++) {
            console_printf("  Violation %d: address 0x%lx, type %d\n", 
                          i, violations[i].address, violations[i].type);
        }
        
        // Free the violations array
        memory_free(violations, num_violations * sizeof(memory_violation_t));
        
        return -1;
    }
    
    // Free the violations array
    memory_free(violations, num_violations * sizeof(memory_violation_t));
    
    return 0;
}

/**
 * Get the list of modified files
 * 
 * @param id: Sandbox ID
 * @param modified_files: Pointer to store the list of modified files
 * @param num_modified_files: Pointer to store the number of modified files
 * @return: 0 on success, -1 on failure
 */
static int sandbox_get_modified_files(sandbox_id_t id, char*** modified_files, size_t* num_modified_files) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Initialize the output parameters
    *modified_files = NULL;
    *num_modified_files = 0;
    
    // Get the process file access history
    file_access_t* accesses = NULL;
    int num_accesses = 0;
    
    if (process_get_file_access_history(sandbox->initial_process, &accesses, &num_accesses) != 0) {
        console_printf("Error: Failed to get file access history for process %d\n", sandbox->initial_process);
        return -1;
    }
    
    // Count the number of modified files
    size_t count = 0;
    
    for (int i = 0; i < num_accesses; i++) {
        // Only count write or create access
        if (accesses[i].mode == FILE_ACCESS_WRITE || 
            accesses[i].mode == FILE_ACCESS_CREATE) {
            count++;
        }
    }
    
    // If no files were modified, return early
    if (count == 0) {
        memory_free(accesses, num_accesses * sizeof(file_access_t));
        return 0;
    }
    
    // Allocate memory for the modified files list
    *modified_files = (char**)malloc(count * sizeof(char*));
    
    if (!*modified_files) {
        console_printf("Error: Failed to allocate memory for modified files list\n");
        memory_free(accesses, num_accesses * sizeof(file_access_t));
        return -1;
    }
    
    // Fill the modified files list
    size_t index = 0;
    
    for (int i = 0; i < num_accesses; i++) {
        // Only include write or create access
        if (accesses[i].mode == FILE_ACCESS_WRITE || 
            accesses[i].mode == FILE_ACCESS_CREATE) {
            // Allocate memory for the file path
            (*modified_files)[index] = strdup(accesses[i].path);
            
            if (!(*modified_files)[index]) {
                console_printf("Error: Failed to allocate memory for file path\n");
                
                // Free the already allocated paths
                for (size_t j = 0; j < index; j++) {
                    free((*modified_files)[j]);
                }
                
                free(*modified_files);
                *modified_files = NULL;
                
                memory_free(accesses, num_accesses * sizeof(file_access_t));
                return -1;
            }
            
            index++;
        }
    }
    
    // Set the number of modified files
    *num_modified_files = count;
    
    // Free the accesses array
    memory_free(accesses, num_accesses * sizeof(file_access_t));
    
    console_printf("Found %zu modified files in sandbox %u\n", count, id);
    
    return 0;
}

/**
 * Apply process state changes
 * 
 * @param id: Sandbox ID
 * @return: 0 on success, -1 on failure
 */
static int sandbox_apply_process_changes(sandbox_id_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Get the process state
    sandbox_process_state_t state;
    
    if (process_get_state(sandbox->initial_process, &state) != 0) {
        console_printf("Error: Failed to get state for process %d\n", sandbox->initial_process);
        return -1;
    }
    
    // Apply the process state changes to the main system
    
    // Update environment variables
    for (int i = 0; i < state.num_env_vars; i++) {
        // Set the environment variable in the main system
        setenv(state.env_vars[i].name, state.env_vars[i].value, 1);
    }
    
    // Update working directory
    if (strlen(state.cwd) > 0) {
        // Change the working directory of the main process
        if (chdir(state.cwd) != 0) {
            console_printf("Warning: Failed to change working directory to %s\n", state.cwd);
        }
    }
    
    // Update process priority if it was changed
    if (state.priority != PROCESS_PRIORITY_NORMAL) {
        // Set the priority of the main process
        process_set_priority(getpid(), state.priority);
    }
    
    console_printf("Applied process state changes from sandbox %u\n", id);
    
    return 0;
}

/**
 * Apply network state changes
 * 
 * @param id: Sandbox ID
 * @return: 0 on success, -1 on failure
 */
static int sandbox_apply_network_changes(sandbox_id_t id) {
    // Check if the sandbox ID is valid
    if (id >= MAX_SANDBOXES || !sandbox_table[id]) {
        console_printf("Error: Invalid sandbox ID\n");
        return -1;
    }
    
    // Get the sandbox
    sandbox_t* sandbox = sandbox_table[id];
    
    // Get the network connections from the sandbox
    network_connection_t* connections = NULL;
    int num_connections = 0;
    
    if (process_get_network_connections(sandbox->initial_process, &connections, &num_connections) != 0) {
        console_printf("Error: Failed to get network connections for process %d\n", sandbox->initial_process);
        return -1;
    }
    
    // Apply network changes to the main system
    for (int i = 0; i < num_connections; i++) {
        // Check if this is a new connection that should be preserved
        if (connections[i].state == 1) { // Assuming state 1 means established
            // Create a new socket with the same properties
            int new_socket = socket(AF_INET, SOCK_STREAM, 0);
            
            if (new_socket >= 0) {
                // Set socket options
                int opt = 1;
                setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
                
                // Create address structures
                struct sockaddr_in local_addr;
                memset(&local_addr, 0, sizeof(local_addr));
                local_addr.sin_family = AF_INET;
                local_addr.sin_addr.s_addr = connections[i].local_addr;
                local_addr.sin_port = connections[i].local_port;
                
                // Bind to the local address if needed
                if (connections[i].local_port != 0) {
                    bind(new_socket, (struct sockaddr*)&local_addr, sizeof(local_addr));
                }
                
                console_printf("Applied network change: preserved connection to %u.%u.%u.%u:%u\n",
                              (connections[i].remote_addr >> 24) & 0xFF,
                              (connections[i].remote_addr >> 16) & 0xFF,
                              (connections[i].remote_addr >> 8) & 0xFF,
                              connections[i].remote_addr & 0xFF,
                              connections[i].remote_port);
            }
        }
    }
    
    // Free the connections array
    memory_free(connections, num_connections * sizeof(network_connection_t));
    
    return 0;
}
