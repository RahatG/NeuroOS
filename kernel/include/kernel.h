/**
 * kernel.h - Main kernel header for NeuroOS
 * 
 * This file contains the main kernel definitions and declarations.
 */

#ifndef NEUROOS_KERNEL_H
#define NEUROOS_KERNEL_H

#include <stddef.h>
#include <stdint.h>

// Kernel version information
#define KERNEL_VERSION_MAJOR 0
#define KERNEL_VERSION_MINOR 1
#define KERNEL_VERSION_PATCH 0
#define KERNEL_VERSION_STRING "0.1.0"

// Kernel information structure
typedef struct {
    const char* version;
    const char* build_date;
    const char* build_time;
    const char* compiler;
    const char* architecture;
    uint64_t uptime;
    uint64_t memory_total;
    uint64_t memory_used;
    uint64_t memory_free;
    uint64_t memory_shared;
    uint64_t memory_buffers;
    uint64_t memory_cached;
    uint64_t swap_total;
    uint64_t swap_used;
    uint64_t swap_free;
    uint64_t processes;
    uint64_t threads;
    uint64_t load_1m;
    uint64_t load_5m;
    uint64_t load_15m;
} kernel_info_t;

// Kernel initialization and shutdown
void kernel_init(void);
void kernel_shutdown(void);

// Kernel main function
void kernel_main(void);

// Kernel information
void kernel_get_info(kernel_info_t* info);
const char* kernel_get_version(void);
uint64_t kernel_get_uptime(void);

// Kernel memory management
void* kernel_malloc(size_t size);
void* kernel_calloc(size_t nmemb, size_t size);
void* kernel_realloc(void* ptr, size_t size);
void kernel_free(void* ptr);

// Kernel process management
int kernel_create_process(const char* name, void (*entry)(void));
int kernel_terminate_process(int pid);
int kernel_get_process_info(int pid, void* info);
int kernel_get_current_process(void);
int kernel_get_parent_process(int pid);
int kernel_get_process_count(void);
int kernel_get_process_list(int* pids, int count);

// Kernel thread management
int kernel_create_thread(int pid, void (*entry)(void));
int kernel_terminate_thread(int tid);
int kernel_get_thread_info(int tid, void* info);
int kernel_get_current_thread(void);
int kernel_get_thread_count(int pid);
int kernel_get_thread_list(int pid, int* tids, int count);

// Kernel synchronization
int kernel_create_mutex(void);
int kernel_destroy_mutex(int mutex);
int kernel_lock_mutex(int mutex);
int kernel_unlock_mutex(int mutex);
int kernel_try_lock_mutex(int mutex);

// Kernel time management
uint64_t kernel_get_ticks(void);
uint64_t kernel_get_time(void);
void kernel_sleep(uint64_t ms);
void kernel_yield(void);

// Kernel I/O management
int kernel_open(const char* path, int flags);
int kernel_close(int fd);
int kernel_read(int fd, void* buf, size_t count);
int kernel_write(int fd, const void* buf, size_t count);
int kernel_seek(int fd, int64_t offset, int whence);
int kernel_tell(int fd);
int kernel_flush(int fd);
int kernel_ioctl(int fd, int request, void* arg);

// Kernel file system management
int kernel_mount(const char* source, const char* target, const char* fs_type, uint64_t flags, const void* data);
int kernel_unmount(const char* target);
int kernel_mkdir(const char* path, uint32_t mode);
int kernel_rmdir(const char* path);
int kernel_rename(const char* oldpath, const char* newpath);
int kernel_unlink(const char* path);
int kernel_stat(const char* path, void* buf);
int kernel_access(const char* path, int mode);
int kernel_chmod(const char* path, uint32_t mode);
int kernel_chown(const char* path, uint32_t owner, uint32_t group);

// Kernel network management
int kernel_socket(int domain, int type, int protocol);
int kernel_bind(int sockfd, const void* addr, uint32_t addrlen);
int kernel_listen(int sockfd, int backlog);
int kernel_accept(int sockfd, void* addr, uint32_t* addrlen);
int kernel_connect(int sockfd, const void* addr, uint32_t addrlen);
int kernel_send(int sockfd, const void* buf, size_t len, int flags);
int kernel_recv(int sockfd, void* buf, size_t len, int flags);
int kernel_sendto(int sockfd, const void* buf, size_t len, int flags, const void* dest_addr, uint32_t addrlen);
int kernel_recvfrom(int sockfd, void* buf, size_t len, int flags, void* src_addr, uint32_t* addrlen);
int kernel_setsockopt(int sockfd, int level, int optname, const void* optval, uint32_t optlen);
int kernel_getsockopt(int sockfd, int level, int optname, void* optval, uint32_t* optlen);
int kernel_getsockname(int sockfd, void* addr, uint32_t* addrlen);
int kernel_getpeername(int sockfd, void* addr, uint32_t* addrlen);
int kernel_shutdown(int sockfd, int how);

// Kernel device management
int kernel_device_register(const char* name, void* ops);
int kernel_device_unregister(const char* name);
int kernel_device_open(const char* name);
int kernel_device_close(int device);
int kernel_device_read(int device, void* buf, size_t count);
int kernel_device_write(int device, const void* buf, size_t count);
int kernel_device_ioctl(int device, int request, void* arg);

// Kernel interrupt management
int kernel_interrupt_register(int irq, void (*handler)(void));
int kernel_interrupt_unregister(int irq);
int kernel_interrupt_enable(int irq);
int kernel_interrupt_disable(int irq);
int kernel_interrupt_get_count(int irq);
int kernel_interrupt_get_status(int irq);

// Kernel module management
int kernel_module_load(const char* path);
int kernel_module_unload(const char* name);
int kernel_module_get_info(const char* name, void* info);
int kernel_module_get_count(void);
int kernel_module_get_list(char** names, int count);

// Kernel system management
int kernel_system_reboot(void);
int kernel_system_shutdown(void);
int kernel_system_suspend(void);
int kernel_system_hibernate(void);
int kernel_system_get_info(void* info);
int kernel_system_get_cpu_info(void* info);
int kernel_system_get_memory_info(void* info);
int kernel_system_get_disk_info(void* info);
int kernel_system_get_network_info(void* info);

// Kernel debug functions
void kernel_debug_print(const char* format, ...);
void kernel_debug_dump_memory(const void* ptr, size_t size);
void kernel_debug_dump_stack(void);
void kernel_debug_dump_registers(void);
void kernel_debug_dump_processes(void);
void kernel_debug_dump_threads(void);
void kernel_debug_dump_modules(void);
void kernel_debug_dump_devices(void);
void kernel_debug_dump_interrupts(void);
void kernel_debug_dump_file_systems(void);
void kernel_debug_dump_network(void);

// Kernel error codes
#define KERNEL_ERROR_NONE 0
#define KERNEL_ERROR_INVALID_ARGUMENT -1
#define KERNEL_ERROR_OUT_OF_MEMORY -2
#define KERNEL_ERROR_NOT_FOUND -3
#define KERNEL_ERROR_ALREADY_EXISTS -4
#define KERNEL_ERROR_PERMISSION_DENIED -5
#define KERNEL_ERROR_NOT_SUPPORTED -6
#define KERNEL_ERROR_TIMEOUT -7
#define KERNEL_ERROR_BUSY -8
#define KERNEL_ERROR_INTERRUPTED -9
#define KERNEL_ERROR_IO -10
#define KERNEL_ERROR_NETWORK -11
#define KERNEL_ERROR_PROTOCOL -12
#define KERNEL_ERROR_CONNECTION_REFUSED -13
#define KERNEL_ERROR_CONNECTION_RESET -14
#define KERNEL_ERROR_CONNECTION_ABORTED -15
#define KERNEL_ERROR_CONNECTION_CLOSED -16
#define KERNEL_ERROR_NOT_CONNECTED -17
#define KERNEL_ERROR_ALREADY_CONNECTED -18
#define KERNEL_ERROR_ADDRESS_IN_USE -19
#define KERNEL_ERROR_ADDRESS_NOT_AVAILABLE -20
#define KERNEL_ERROR_NETWORK_UNREACHABLE -21
#define KERNEL_ERROR_HOST_UNREACHABLE -22
#define KERNEL_ERROR_NETWORK_DOWN -23
#define KERNEL_ERROR_HOST_DOWN -24
#define KERNEL_ERROR_WOULD_BLOCK -25
#define KERNEL_ERROR_IN_PROGRESS -26
#define KERNEL_ERROR_CANCELED -27
#define KERNEL_ERROR_MEMORY_FAULT -28
#define KERNEL_ERROR_BAD_ADDRESS -29
#define KERNEL_ERROR_BAD_FILE_DESCRIPTOR -30
#define KERNEL_ERROR_FILE_EXISTS -31
#define KERNEL_ERROR_FILE_TOO_LARGE -32
#define KERNEL_ERROR_FILE_TABLE_OVERFLOW -33
#define KERNEL_ERROR_TOO_MANY_OPEN_FILES -34
#define KERNEL_ERROR_NO_SPACE_LEFT -35
#define KERNEL_ERROR_READ_ONLY_FILE_SYSTEM -36
#define KERNEL_ERROR_DIRECTORY_NOT_EMPTY -37
#define KERNEL_ERROR_TOO_MANY_LINKS -38
#define KERNEL_ERROR_BROKEN_PIPE -39
#define KERNEL_ERROR_ARGUMENT_LIST_TOO_LONG -40
#define KERNEL_ERROR_ARGUMENT_OUT_OF_DOMAIN -41
#define KERNEL_ERROR_INVALID_EXECUTABLE -42
#define KERNEL_ERROR_EXEC_FORMAT_ERROR -43
#define KERNEL_ERROR_INVALID_SYSTEM_CALL -44
#define KERNEL_ERROR_ILLEGAL_INSTRUCTION -45
#define KERNEL_ERROR_INVALID_HANDLE -46
#define KERNEL_ERROR_INVALID_OPERATION -47
#define KERNEL_ERROR_OPERATION_NOT_PERMITTED -48
#define KERNEL_ERROR_OPERATION_CANCELED -49
#define KERNEL_ERROR_OPERATION_IN_PROGRESS -50
#define KERNEL_ERROR_OPERATION_NOT_SUPPORTED -51
#define KERNEL_ERROR_OPERATION_WOULD_BLOCK -52
#define KERNEL_ERROR_OPERATION_TIMEOUT -53
#define KERNEL_ERROR_QUOTA_EXCEEDED -54
#define KERNEL_ERROR_RESOURCE_BUSY -55
#define KERNEL_ERROR_RESOURCE_DEADLOCK_AVOIDED -56
#define KERNEL_ERROR_RESOURCE_TEMPORARILY_UNAVAILABLE -57
#define KERNEL_ERROR_RESULT_TOO_LARGE -58
#define KERNEL_ERROR_STACK_OVERFLOW -59
#define KERNEL_ERROR_TOO_MANY_PROCESSES -60
#define KERNEL_ERROR_TOO_MANY_THREADS -61
#define KERNEL_ERROR_TOO_MANY_USERS -62
#define KERNEL_ERROR_UNKNOWN -63

#endif // NEUROOS_KERNEL_H
