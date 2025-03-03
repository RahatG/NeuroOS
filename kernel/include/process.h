/**
 * process.h - Process management for NeuroOS
 * 
 * This file contains the process management definitions and declarations.
 */

#ifndef NEUROOS_PROCESS_H
#define NEUROOS_PROCESS_H

#include <stddef.h>
#include <stdint.h>

// Process name maximum length
#define PROCESS_NAME_MAX 256

// Process types
typedef int pid_t;
typedef uint8_t process_state_t;
typedef uint8_t process_priority_t;
typedef uint32_t process_flags_t;

// Process states
#define PROCESS_STATE_CREATED    0
#define PROCESS_STATE_READY      1
#define PROCESS_STATE_RUNNING    2
#define PROCESS_STATE_BLOCKED    3
#define PROCESS_STATE_SUSPENDED  4
#define PROCESS_STATE_TERMINATED 5
#define PROCESS_STATE_ZOMBIE     6

// Process flags
#define PROCESS_FLAG_KERNEL      (1 << 0)
#define PROCESS_FLAG_USER        (1 << 1)
#define PROCESS_FLAG_DAEMON      (1 << 2)
#define PROCESS_FLAG_CRITICAL    (1 << 3)
#define PROCESS_FLAG_SUSPENDED   (1 << 4)
#define PROCESS_FLAG_TRACED      (1 << 5)
#define PROCESS_FLAG_STOPPED     (1 << 6)
#define PROCESS_FLAG_BACKGROUND  (1 << 7)
#define PROCESS_FLAG_FOREGROUND  (1 << 8)
#define PROCESS_FLAG_INTERACTIVE (1 << 9)
#define PROCESS_FLAG_BATCH       (1 << 10)
#define PROCESS_FLAG_REALTIME    (1 << 11)
#define PROCESS_FLAG_IDLE        (1 << 12)
#define PROCESS_FLAG_SYSTEM      (1 << 13)
#define PROCESS_FLAG_SCHEDULER   (1 << 14)
#define PROCESS_FLAG_INIT        (1 << 15)
#define PROCESS_FLAG_SESSION     (1 << 16)
#define PROCESS_FLAG_GROUP       (1 << 17)
#define PROCESS_FLAG_DETACHED    (1 << 18)
#define PROCESS_FLAG_ORPHANED    (1 << 19)
#define PROCESS_FLAG_EXITING     (1 << 20)
#define PROCESS_FLAG_KILLED      (1 << 21)
#define PROCESS_FLAG_CORE        (1 << 22)
#define PROCESS_FLAG_SIGNALED    (1 << 23)
#define PROCESS_FLAG_DUMPED      (1 << 24)
#define PROCESS_FLAG_CONTINUED   (1 << 25)
#define PROCESS_FLAG_STOPPED_SIG (1 << 26)
#define PROCESS_FLAG_STOPPED_TRACE (1 << 27)
#define PROCESS_FLAG_STOPPED_DEBUG (1 << 28)
#define PROCESS_FLAG_NOLOAD      (1 << 29)
#define PROCESS_FLAG_NOUNLOAD    (1 << 30)
#define PROCESS_FLAG_RESERVED    (1 << 31)

// Process priorities
#define PROCESS_PRIORITY_IDLE      0
#define PROCESS_PRIORITY_LOWEST    1
#define PROCESS_PRIORITY_VERY_LOW  2
#define PROCESS_PRIORITY_LOW       3
#define PROCESS_PRIORITY_NORMAL    4
#define PROCESS_PRIORITY_HIGH      5
#define PROCESS_PRIORITY_VERY_HIGH 6
#define PROCESS_PRIORITY_HIGHEST   7
#define PROCESS_PRIORITY_REALTIME  8

// Process context structure
typedef struct {
    uint32_t eip;
    uint32_t esp;
    uint32_t ebp;
    uint32_t ebx;
    uint32_t esi;
    uint32_t edi;
} process_context_t;

// Process structure
typedef struct process {
    pid_t pid;
    char name[PROCESS_NAME_MAX];
    process_state_t state;
    process_priority_t priority;
    process_flags_t flags;
    
    void* stack;
    size_t stack_size;
    void* kernel_stack;
    size_t kernel_stack_size;
    void* page_directory;
    
    process_context_t context;
    
    uint64_t cpu_time;
    uint64_t creation_time;
    int exit_code;
    
    struct process* parent;
    struct process* next;
    struct process* prev;
} process_t;

// Process information structure
typedef struct {
    int pid;
    int ppid;
    int pgid;
    int sid;
    int uid;
    int gid;
    int euid;
    int egid;
    int state;
    int priority;
    uint32_t flags;
    uint64_t start_time;
    uint64_t user_time;
    uint64_t system_time;
    uint64_t total_time;
    uint64_t virtual_memory;
    uint64_t resident_memory;
    uint64_t shared_memory;
    uint64_t text_memory;
    uint64_t data_memory;
    uint64_t stack_memory;
    uint64_t heap_memory;
    uint64_t swap_memory;
    uint64_t minor_faults;
    uint64_t major_faults;
    uint64_t voluntary_context_switches;
    uint64_t involuntary_context_switches;
    uint64_t signals_received;
    uint64_t signals_delivered;
    uint64_t signals_pending;
    uint64_t signals_blocked;
    uint64_t signals_ignored;
    uint64_t signals_caught;
    uint64_t read_bytes;
    uint64_t write_bytes;
    uint64_t read_chars;
    uint64_t write_chars;
    uint64_t read_syscalls;
    uint64_t write_syscalls;
    uint64_t total_syscalls;
    uint64_t threads;
    uint64_t files;
    uint64_t sockets;
    uint64_t pipes;
    uint64_t locks;
    uint64_t semaphores;
    uint64_t message_queues;
    uint64_t shared_memory_segments;
    uint64_t cpu_usage;
    uint64_t memory_usage;
    uint64_t io_usage;
    uint64_t network_usage;
    uint64_t disk_usage;
    char name[256];
    char cmdline[1024];
    char cwd[1024];
    char root[1024];
    char exe[1024];
} process_info_t;

// Process initialization and shutdown
void process_init(void);
void process_shutdown(void);

// Process creation and termination
pid_t process_create(const char* name, void (*entry_point)(void), size_t stack_size,
                    process_priority_t priority, process_flags_t flags);
int process_terminate(pid_t pid, int exit_code);
int process_kill(int pid, int signal);
int process_wait(int pid, int* status);
int process_detach(int pid);
int process_suspend(int pid);
int process_resume(int pid);
int process_set_priority(pid_t pid, process_priority_t priority);
int process_set_flags(int pid, uint32_t flags);
int process_clear_flags(int pid, uint32_t flags);

// Process information
int process_get_info(int pid, process_info_t* info);
size_t process_get_count(void);
size_t process_get_list(process_t* processes, size_t max_count);
process_t* process_get_current(void);
process_t* process_get_by_id(pid_t pid);
int process_get_parent(int pid);
int process_get_children(int pid, int* children, int count);
int process_get_siblings(int pid, int* siblings, int count);
int process_get_group(int pgid, int* pids, int count);
int process_get_session(int sid, int* pids, int count);
int process_get_foreground(int sid);
int process_set_foreground(int pid, int sid);

// Process resources
int process_get_memory_usage(int pid, uint64_t* usage);
int process_get_cpu_usage(int pid, uint64_t* usage);
int process_get_io_usage(int pid, uint64_t* usage);
int process_get_network_usage(int pid, uint64_t* usage);
int process_get_disk_usage(int pid, uint64_t* usage);
int process_get_resource_limit(int pid, int resource, uint64_t* soft, uint64_t* hard);
int process_set_resource_limit(int pid, int resource, uint64_t soft, uint64_t hard);

// Process signals
int process_send_signal(int pid, int signal);
int process_set_signal_handler(int signal, void (*handler)(int));
int process_get_signal_handler(int signal, void (**handler)(int));
int process_block_signals(int* signals, int count);
int process_unblock_signals(int* signals, int count);
int process_get_blocked_signals(int* signals, int* count);
int process_get_pending_signals(int* signals, int* count);
int process_get_signal_mask(uint64_t* mask);
int process_set_signal_mask(uint64_t mask);

// Process scheduling
void process_yield(void);
void process_sleep(uint32_t ms);
int process_wake(int pid);
int process_set_scheduler(int pid, int scheduler, int priority);
int process_get_scheduler(int pid, int* scheduler, int* priority);
int process_set_affinity(int pid, uint64_t mask);
int process_get_affinity(int pid, uint64_t* mask);

// Process environment
int process_get_environment(int pid, char** env, int* count);
int process_set_environment(int pid, char** env, int count);
int process_get_environment_variable(int pid, const char* name, char* value, size_t size);
int process_set_environment_variable(int pid, const char* name, const char* value);
int process_unset_environment_variable(int pid, const char* name);
int process_clear_environment(int pid);

// Process file descriptors
int process_get_file_descriptors(int pid, int* fds, int count);
int process_get_file_path(int pid, int fd, char* path, size_t size);
int process_is_fd_open(int pid, int fd);
int process_open_file(int pid, const char* path, int fd);
int process_get_file_descriptor_count(int pid);
int process_get_file_descriptor_info(int pid, int fd, void* info);
int process_close_file_descriptor(int pid, int fd);
int process_duplicate_file_descriptor(int pid, int fd);
int process_set_file_descriptor_flags(int pid, int fd, int flags);
int process_get_file_descriptor_flags(int pid, int fd, int* flags);

// Process memory
int process_get_memory_regions(int pid, void* regions, int count);
int process_get_memory_region_count(int pid);
int process_get_memory_region_info(int pid, void* region, void* info);
int process_read_memory(int pid, void* address, void* buffer, size_t size);
int process_write_memory(int pid, void* address, const void* buffer, size_t size);
int process_allocate_memory(int pid, size_t size, uint32_t flags, void** address);
int process_free_memory(int pid, void* address, size_t size);
int process_protect_memory(int pid, void* address, size_t size, uint32_t flags);

// Process debug functions
int process_attach_debugger(int pid);
int process_detach_debugger(int pid);
int process_set_breakpoint(int pid, void* address);
int process_clear_breakpoint(int pid, void* address);
int process_step(int pid);
int process_continue(int pid);
int process_get_registers(int pid, void* registers);
int process_set_registers(int pid, const void* registers);
int process_get_fpu_registers(int pid, void* registers);
int process_set_fpu_registers(int pid, const void* registers);
int process_get_debug_registers(int pid, void* registers);
int process_set_debug_registers(int pid, const void* registers);
int process_get_signal_info(int pid, int signal, void* info);
int process_get_event_message(int pid, void* message);
int process_get_syscall_info(int pid, void* info);

// Process network functions
int process_get_network_connections(int pid, void* connections, int* count);
int process_get_sockets(int pid, void* sockets, int* count);
int process_close_socket(int pid, int socket);

// Process device functions
int process_get_open_devices(int pid, void* devices, int* count);
int process_close_device(int pid, int device);

// Process syscall functions
int process_get_syscall_history(int pid, void* syscalls, int* count);

// Process file access functions
int process_get_file_access_history(int pid, void* accesses, int* count);

// Process memory violation functions
int process_get_memory_violations(int pid, void* violations, int* count);

// Process state functions
int process_get_state(int pid, void* state);

// Process statistics
int process_get_stats(pid_t pid, uint64_t* cpu_time, process_state_t* state,
                     process_priority_t* priority);

#endif // NEUROOS_PROCESS_H
