/**
 * sandbox.h - Sandbox interface for NeuroOS
 * 
 * This file contains the sandbox interface definitions and declarations.
 */

#ifndef NEUROOS_SANDBOX_H
#define NEUROOS_SANDBOX_H

#include <stddef.h>
#include <stdint.h>

// Sandbox ID and flags types
typedef uint32_t sandbox_id_t;
typedef uint32_t sandbox_flags_t;

// Sandbox types
#define SANDBOX_TYPE_PROCESS    0
#define SANDBOX_TYPE_THREAD     1
#define SANDBOX_TYPE_CONTAINER  2
#define SANDBOX_TYPE_VM         3
#define SANDBOX_TYPE_NAMESPACE  4
#define SANDBOX_TYPE_SECCOMP    5
#define SANDBOX_TYPE_CHROOT     6
#define SANDBOX_TYPE_JAIL       7
#define SANDBOX_TYPE_CUSTOM     8

// Sandbox flags
#define SANDBOX_FLAG_NONE           0x00000000
#define SANDBOX_FLAG_READONLY       0x00000001
#define SANDBOX_FLAG_NETWORK        0x00000002
#define SANDBOX_FLAG_FILESYSTEM     0x00000004
#define SANDBOX_FLAG_DEVICES        0x00000008
#define SANDBOX_FLAG_IPC            0x00000010
#define SANDBOX_FLAG_SIGNALS        0x00000020
#define SANDBOX_FLAG_PROCESS        0x00000040
#define SANDBOX_FLAG_MEMORY         0x00000080
#define SANDBOX_FLAG_CPU            0x00000100
#define SANDBOX_FLAG_SYSCALLS       0x00000200
#define SANDBOX_FLAG_CAPABILITIES   0x00000400
#define SANDBOX_FLAG_PRIVILEGES     0x00000800
#define SANDBOX_FLAG_RESOURCES      0x00001000
#define SANDBOX_FLAG_NAMESPACES     0x00002000
#define SANDBOX_FLAG_CGROUPS        0x00004000
#define SANDBOX_FLAG_SECCOMP        0x00008000
#define SANDBOX_FLAG_APPARMOR       0x00010000
#define SANDBOX_FLAG_SELINUX        0x00020000
#define SANDBOX_FLAG_ISOLATION      0x40000000
#define SANDBOX_FLAG_SECURITY       0x80000000

// Sandbox states
#define SANDBOX_STATE_CREATED    0
#define SANDBOX_STATE_RUNNING    1
#define SANDBOX_STATE_PAUSED     2
#define SANDBOX_STATE_STOPPED    3
#define SANDBOX_STATE_TERMINATED 4
#define SANDBOX_STATE_ERROR      5

// Sandbox error codes
#define SANDBOX_ERROR_NONE                 0
#define SANDBOX_ERROR_INVALID_ARGUMENT    -1
#define SANDBOX_ERROR_OUT_OF_MEMORY       -2
#define SANDBOX_ERROR_PERMISSION_DENIED   -3
#define SANDBOX_ERROR_NOT_FOUND           -4
#define SANDBOX_ERROR_ALREADY_EXISTS      -5
#define SANDBOX_ERROR_NOT_SUPPORTED       -6
#define SANDBOX_ERROR_TIMEOUT             -7
#define SANDBOX_ERROR_BUSY                -8
#define SANDBOX_ERROR_INTERRUPTED         -9
#define SANDBOX_ERROR_IO                  -10
#define SANDBOX_ERROR_INVALID_STATE       -11
#define SANDBOX_ERROR_INVALID_TYPE        -12
#define SANDBOX_ERROR_INVALID_FLAGS       -13
#define SANDBOX_ERROR_INVALID_CONFIG      -14
#define SANDBOX_ERROR_RESOURCE_LIMIT      -15
#define SANDBOX_ERROR_UNKNOWN             -16

// Sandbox configuration structure
typedef struct {
    uint32_t type;
    uint32_t flags;
    uint32_t cpu_limit;
    uint32_t memory_limit;
    uint32_t disk_limit;
    uint32_t network_limit;
    uint32_t process_limit;
    uint32_t thread_limit;
    uint32_t file_limit;
    uint32_t socket_limit;
    char name[64];
    char path[256];
    char command[256];
    char args[1024];
    char env[1024];
    char cwd[256];
    char root[256];
    char stdin_path[256];
    char stdout_path[256];
    char stderr_path[256];
    char log_path[256];
} sandbox_config_t;

// Sandbox state structure
typedef struct {
    uint32_t id;
    uint32_t type;
    uint32_t flags;
    uint32_t state;
    uint32_t error;
    uint32_t cpu_usage;
    uint32_t memory_usage;
    uint32_t disk_usage;
    uint32_t network_usage;
    uint32_t process_count;
    uint32_t thread_count;
    uint32_t file_count;
    uint32_t socket_count;
    uint64_t start_time;
    uint64_t end_time;
    uint64_t elapsed_time;
    uint64_t cpu_time;
    uint64_t system_time;
    uint64_t user_time;
    uint64_t idle_time;
    uint64_t io_time;
    uint64_t wait_time;
    uint64_t sleep_time;
    uint64_t block_time;
    uint64_t context_switches;
    uint64_t page_faults;
    uint64_t major_page_faults;
    uint64_t minor_page_faults;
    uint64_t read_bytes;
    uint64_t write_bytes;
    char name[64];
    char path[256];
    char command[256];
    char args[1024];
    char env[1024];
    char cwd[256];
    char root[256];
    char stdin_path[256];
    char stdout_path[256];
    char stderr_path[256];
    char log_path[256];
} sandbox_state_t;

// Sandbox initialization and shutdown
int sandbox_init(void);
int sandbox_shutdown(void);

// Sandbox management
uint32_t sandbox_create(const sandbox_config_t* config);
int sandbox_destroy(uint32_t id);
int sandbox_start(uint32_t id);
int sandbox_stop(uint32_t id);
int sandbox_pause(uint32_t id);
int sandbox_resume(uint32_t id);
int sandbox_restart(uint32_t id);
int sandbox_reset(uint32_t id);
int sandbox_clone(uint32_t id, uint32_t* new_id);
int sandbox_fork(uint32_t id, uint32_t* new_id);
int sandbox_exec(uint32_t id, const char* command, const char* args);
int sandbox_wait(uint32_t id, int* status, int options);
int sandbox_signal(uint32_t id, int signal);
int sandbox_kill(uint32_t id);
int sandbox_terminate(uint32_t id);
int sandbox_get_state(uint32_t id, sandbox_state_t* state);
int sandbox_set_config(uint32_t id, const sandbox_config_t* config);
int sandbox_get_config(uint32_t id, sandbox_config_t* config);
int sandbox_get_error(uint32_t id, int* error);
int sandbox_clear_error(uint32_t id);
int sandbox_get_exit_code(uint32_t id, int* exit_code);
int sandbox_get_exit_status(uint32_t id, int* exit_status);
int sandbox_get_exit_signal(uint32_t id, int* exit_signal);
int sandbox_get_exit_core_dump(uint32_t id, int* exit_core_dump);
int sandbox_get_exit_reason(uint32_t id, char* reason, size_t size);
int sandbox_get_exit_message(uint32_t id, char* message, size_t size);
int sandbox_get_exit_time(uint32_t id, uint64_t* exit_time);

// Sandbox resource management
int sandbox_set_cpu_limit(uint32_t id, uint32_t limit);
int sandbox_get_cpu_limit(uint32_t id, uint32_t* limit);
int sandbox_set_memory_limit(uint32_t id, uint32_t limit);
int sandbox_get_memory_limit(uint32_t id, uint32_t* limit);
int sandbox_set_disk_limit(uint32_t id, uint32_t limit);
int sandbox_get_disk_limit(uint32_t id, uint32_t* limit);
int sandbox_set_network_limit(uint32_t id, uint32_t limit);
int sandbox_get_network_limit(uint32_t id, uint32_t* limit);
int sandbox_set_process_limit(uint32_t id, uint32_t limit);
int sandbox_get_process_limit(uint32_t id, uint32_t* limit);
int sandbox_set_thread_limit(uint32_t id, uint32_t limit);
int sandbox_get_thread_limit(uint32_t id, uint32_t* limit);
int sandbox_set_file_limit(uint32_t id, uint32_t limit);
int sandbox_get_file_limit(uint32_t id, uint32_t* limit);
int sandbox_set_socket_limit(uint32_t id, uint32_t limit);
int sandbox_get_socket_limit(uint32_t id, uint32_t* limit);

// Sandbox policy management
int sandbox_set_policy(uint32_t id, const char* policy);
int sandbox_get_policy(uint32_t id, char* policy, size_t size);
int sandbox_set_seccomp(uint32_t id, const char* seccomp);
int sandbox_get_seccomp(uint32_t id, char* seccomp, size_t size);
int sandbox_set_apparmor(uint32_t id, const char* apparmor);
int sandbox_get_apparmor(uint32_t id, char* apparmor, size_t size);
int sandbox_set_selinux(uint32_t id, const char* selinux);
int sandbox_get_selinux(uint32_t id, char* selinux, size_t size);

// Sandbox namespace management
int sandbox_set_namespace(uint32_t id, int type, int flags);
int sandbox_get_namespace(uint32_t id, int type, int* flags);
int sandbox_join_namespace(uint32_t id, int type);
int sandbox_leave_namespace(uint32_t id, int type);
int sandbox_create_namespace(uint32_t id, int type, int flags);
int sandbox_destroy_namespace(uint32_t id, int type);

// Sandbox cgroup management
int sandbox_set_cgroup(uint32_t id, const char* cgroup);
int sandbox_get_cgroup(uint32_t id, char* cgroup, size_t size);
int sandbox_join_cgroup(uint32_t id, const char* cgroup);
int sandbox_leave_cgroup(uint32_t id, const char* cgroup);
int sandbox_create_cgroup(uint32_t id, const char* cgroup);
int sandbox_destroy_cgroup(uint32_t id, const char* cgroup);

// Sandbox capability management
int sandbox_set_capability(uint32_t id, int capability, int value);
int sandbox_get_capability(uint32_t id, int capability, int* value);
int sandbox_add_capability(uint32_t id, int capability);
int sandbox_remove_capability(uint32_t id, int capability);
int sandbox_clear_capabilities(uint32_t id);
int sandbox_reset_capabilities(uint32_t id);

// Sandbox syscall management
int sandbox_set_syscall(uint32_t id, int syscall, int action);
int sandbox_get_syscall(uint32_t id, int syscall, int* action);
int sandbox_allow_syscall(uint32_t id, int syscall);
int sandbox_deny_syscall(uint32_t id, int syscall);
int sandbox_trace_syscall(uint32_t id, int syscall);
int sandbox_log_syscall(uint32_t id, int syscall);
int sandbox_clear_syscalls(uint32_t id);
int sandbox_reset_syscalls(uint32_t id);

// Sandbox file management
int sandbox_set_file_access(uint32_t id, const char* path, int access);
int sandbox_get_file_access(uint32_t id, const char* path, int* access);
int sandbox_allow_file(uint32_t id, const char* path, int access);
int sandbox_deny_file(uint32_t id, const char* path);
int sandbox_clear_file_access(uint32_t id);
int sandbox_reset_file_access(uint32_t id);

// Sandbox network management
int sandbox_set_network_access(uint32_t id, int family, int type, int protocol, int access);
int sandbox_get_network_access(uint32_t id, int family, int type, int protocol, int* access);
int sandbox_allow_network(uint32_t id, int family, int type, int protocol);
int sandbox_deny_network(uint32_t id, int family, int type, int protocol);
int sandbox_clear_network_access(uint32_t id);
int sandbox_reset_network_access(uint32_t id);

// Sandbox device management
int sandbox_set_device_access(uint32_t id, const char* device, int access);
int sandbox_get_device_access(uint32_t id, const char* device, int* access);
int sandbox_allow_device(uint32_t id, const char* device, int access);
int sandbox_deny_device(uint32_t id, const char* device);
int sandbox_clear_device_access(uint32_t id);
int sandbox_reset_device_access(uint32_t id);

// Sandbox IPC management
int sandbox_set_ipc_access(uint32_t id, int type, int access);
int sandbox_get_ipc_access(uint32_t id, int type, int* access);
int sandbox_allow_ipc(uint32_t id, int type);
int sandbox_deny_ipc(uint32_t id, int type);
int sandbox_clear_ipc_access(uint32_t id);
int sandbox_reset_ipc_access(uint32_t id);

// Sandbox signal management
int sandbox_set_signal_access(uint32_t id, int signal, int access);
int sandbox_get_signal_access(uint32_t id, int signal, int* access);
int sandbox_allow_signal(uint32_t id, int signal);
int sandbox_deny_signal(uint32_t id, int signal);
int sandbox_clear_signal_access(uint32_t id);
int sandbox_reset_signal_access(uint32_t id);

// Sandbox debug functions
void sandbox_dump_state(uint32_t id);
void sandbox_dump_config(uint32_t id);
void sandbox_dump_policy(uint32_t id);
void sandbox_dump_seccomp(uint32_t id);
void sandbox_dump_apparmor(uint32_t id);
void sandbox_dump_selinux(uint32_t id);
void sandbox_dump_namespaces(uint32_t id);
void sandbox_dump_cgroups(uint32_t id);
void sandbox_dump_capabilities(uint32_t id);
void sandbox_dump_syscalls(uint32_t id);
void sandbox_dump_files(uint32_t id);
void sandbox_dump_network(uint32_t id);
void sandbox_dump_devices(uint32_t id);
void sandbox_dump_ipc(uint32_t id);
void sandbox_dump_signals(uint32_t id);

#endif // NEUROOS_SANDBOX_H
