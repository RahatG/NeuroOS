/**
 * backup.h - Backup system for NeuroOS
 * 
 * This file contains the backup system definitions and declarations.
 */

#ifndef NEUROOS_BACKUP_H
#define NEUROOS_BACKUP_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// Backup ID type
typedef uint32_t backup_id_t;

// Backup type and flags types
typedef uint32_t backup_type_t;
typedef uint32_t backup_flags_t;

// Backup magic and version
#define BACKUP_MAGIC 0x4E424B50 // "NBKP"
#define BACKUP_VERSION 1

// Maximum description length
#define BACKUP_DESCRIPTION_MAX 256

// Backup types
#define BACKUP_TYPE_FULL      0
#define BACKUP_TYPE_KERNEL    1
#define BACKUP_TYPE_FILESYSTEM 2
#define BACKUP_TYPE_CONFIGURATION 3
#define BACKUP_TYPE_INCREMENTAL 4
#define BACKUP_TYPE_DIFFERENTIAL 5
#define BACKUP_TYPE_SNAPSHOT  6
#define BACKUP_TYPE_ARCHIVE   7
#define BACKUP_TYPE_MIRROR    8
#define BACKUP_TYPE_CLONE     9
#define BACKUP_TYPE_IMAGE     10
#define BACKUP_TYPE_CUSTOM    11

// Backup flags
#define BACKUP_FLAG_NONE      0x00000000
#define BACKUP_FLAG_COMPRESS  0x00000001
#define BACKUP_FLAG_ENCRYPT   0x00000002
#define BACKUP_FLAG_VERIFY    0x00000004
#define BACKUP_FLAG_CHECKSUM  0x00000008
#define BACKUP_FLAG_TIMESTAMP 0x00000010
#define BACKUP_FLAG_VERSIONED 0x00000020
#define BACKUP_FLAG_INCREMENTAL 0x00000040
#define BACKUP_FLAG_DIFFERENTIAL 0x00000080
#define BACKUP_FLAG_SNAPSHOT  0x00000100
#define BACKUP_FLAG_ARCHIVE   0x00000200
#define BACKUP_FLAG_MIRROR    0x00000400
#define BACKUP_FLAG_CLONE     0x00000800
#define BACKUP_FLAG_IMAGE     0x00001000
#define BACKUP_FLAG_CUSTOM    0x00002000
#define BACKUP_FLAG_RECURSIVE 0x00004000
#define BACKUP_FLAG_FOLLOW_LINKS 0x00008000
#define BACKUP_FLAG_PRESERVE_PERMISSIONS 0x00010000
#define BACKUP_FLAG_PRESERVE_OWNERSHIP 0x00020000
#define BACKUP_FLAG_PRESERVE_TIMESTAMPS 0x00040000
#define BACKUP_FLAG_PRESERVE_EXTENDED_ATTRIBUTES 0x00080000
#define BACKUP_FLAG_PRESERVE_ACL 0x00100000
#define BACKUP_FLAG_PRESERVE_XATTR 0x00200000
#define BACKUP_FLAG_PRESERVE_CAPABILITIES 0x00400000
#define BACKUP_FLAG_PRESERVE_SELINUX 0x00800000
#define BACKUP_FLAG_PRESERVE_SMACK 0x01000000
#define BACKUP_FLAG_PRESERVE_APPARMOR 0x02000000
#define BACKUP_FLAG_PRESERVE_SECURITY 0x04000000
#define BACKUP_FLAG_PRESERVE_ALL 0x07FF0000
#define BACKUP_FLAG_EXCLUDE_CACHE 0x08000000
#define BACKUP_FLAG_EXCLUDE_TEMP 0x10000000
#define BACKUP_FLAG_EXCLUDE_LOGS 0x20000000
#define BACKUP_FLAG_EXCLUDE_BACKUP 0x40000000
#define BACKUP_FLAG_EXCLUDE_CUSTOM 0x80000000
#define BACKUP_FLAG_CRITICAL 0x00000020 // Critical backup that cannot be deleted

// Backup states
#define BACKUP_STATE_NONE     0
#define BACKUP_STATE_CREATED  1
#define BACKUP_STATE_CREATING 2
#define BACKUP_STATE_RUNNING  3
#define BACKUP_STATE_PAUSED   4
#define BACKUP_STATE_STOPPED  5
#define BACKUP_STATE_COMPLETED 6
#define BACKUP_STATE_FAILED   7
#define BACKUP_STATE_ABORTED  8
#define BACKUP_STATE_CORRUPTED 9
#define BACKUP_STATE_VERIFIED 10
#define BACKUP_STATE_RESTORED 11
#define BACKUP_STATE_DELETED  12
#define BACKUP_STATE_CUSTOM   13
#define BACKUP_STATE_READY    14
#define BACKUP_STATE_ERROR    15
#define BACKUP_STATE_RESTORING 16

// Backup error codes
#define BACKUP_ERROR_NONE     0
#define BACKUP_ERROR_INVALID_ARGUMENT -1
#define BACKUP_ERROR_OUT_OF_MEMORY -2
#define BACKUP_ERROR_PERMISSION_DENIED -3
#define BACKUP_ERROR_NOT_FOUND -4
#define BACKUP_ERROR_ALREADY_EXISTS -5
#define BACKUP_ERROR_NOT_SUPPORTED -6
#define BACKUP_ERROR_TIMEOUT -7
#define BACKUP_ERROR_BUSY -8
#define BACKUP_ERROR_INTERRUPTED -9
#define BACKUP_ERROR_IO -10
#define BACKUP_ERROR_INVALID_STATE -11
#define BACKUP_ERROR_INVALID_TYPE -12
#define BACKUP_ERROR_INVALID_FLAGS -13
#define BACKUP_ERROR_INVALID_CONFIG -14
#define BACKUP_ERROR_INVALID_FORMAT -15
#define BACKUP_ERROR_INVALID_CHECKSUM -16
#define BACKUP_ERROR_INVALID_SIGNATURE -17
#define BACKUP_ERROR_INVALID_ENCRYPTION -18
#define BACKUP_ERROR_INVALID_COMPRESSION -19
#define BACKUP_ERROR_INVALID_ARCHIVE -20
#define BACKUP_ERROR_UNKNOWN -21

// Backup header structure
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t type;
    uint32_t flags;
    uint64_t creation_time;
    uint64_t size;
    uint32_t parent_id;
    char description[BACKUP_DESCRIPTION_MAX];
} backup_header_t;

// Backup information structure
typedef struct {
    backup_id_t id;
    backup_type_t type;
    uint32_t state;
    backup_flags_t flags;
    uint64_t creation_time;
    uint64_t size;
    backup_id_t parent_id;
    char description[BACKUP_DESCRIPTION_MAX];
    char filename[256];
} backup_info_t;

// Backup timer structure
typedef struct {
    backup_type_t type;
    uint64_t interval_ms;
    uint64_t next_backup_time;
    uint32_t timer_id;
    char description_prefix[BACKUP_DESCRIPTION_MAX];
} backup_timer_t;

// Backup configuration structure
typedef struct {
    uint32_t type;
    uint32_t flags;
    char source_path[1024];
    char destination_path[1024];
    char exclude_patterns[1024];
    char include_patterns[1024];
    char password[256];
    char encryption_key[256];
    char encryption_algorithm[64];
    char compression_algorithm[64];
    char checksum_algorithm[64];
    char signature_algorithm[64];
    char timestamp_format[64];
    char version_format[64];
    uint32_t compression_level;
    uint32_t encryption_level;
    uint32_t checksum_level;
    uint32_t signature_level;
    uint32_t timestamp_level;
    uint32_t version_level;
    uint32_t max_versions;
    uint32_t max_backups;
    uint32_t max_size;
    uint32_t max_files;
    uint32_t max_directories;
    uint32_t max_links;
    uint32_t max_devices;
    uint32_t max_depth;
    uint32_t buffer_size;
    uint32_t block_size;
    uint32_t io_size;
    uint32_t thread_count;
    uint32_t process_count;
    uint32_t retry_count;
    uint32_t retry_delay;
    uint32_t timeout;
    uint32_t interval;
    uint32_t priority;
    uint32_t reserved[32];
} backup_config_t;

// Backup state structure
typedef struct {
    uint32_t id;
    uint32_t type;
    uint32_t flags;
    uint32_t state;
    uint32_t error;
    uint64_t start_time;
    uint64_t end_time;
    uint64_t elapsed_time;
    uint64_t estimated_time;
    uint64_t remaining_time;
    uint64_t total_size;
    uint64_t processed_size;
    uint64_t remaining_size;
    uint64_t total_files;
    uint64_t processed_files;
    uint64_t remaining_files;
    uint64_t total_directories;
    uint64_t processed_directories;
    uint64_t remaining_directories;
    uint64_t total_links;
    uint64_t processed_links;
    uint64_t remaining_links;
    uint64_t total_devices;
    uint64_t processed_devices;
    uint64_t remaining_devices;
    uint64_t total_errors;
    uint64_t total_warnings;
    uint64_t total_skipped;
    uint64_t total_retried;
    uint64_t total_ignored;
    uint64_t total_excluded;
    uint64_t total_included;
    uint64_t total_filtered;
    uint64_t total_matched;
    uint64_t total_unmatched;
    uint64_t total_processed;
    uint64_t total_unprocessed;
    uint64_t total_successful;
    uint64_t total_unsuccessful;
    uint64_t total_completed;
    uint64_t total_incomplete;
    uint64_t total_aborted;
    uint64_t total_interrupted;
    uint64_t total_timeout;
    uint64_t total_retry;
    uint64_t total_error;
    uint64_t total_warning;
    uint64_t total_info;
    uint64_t total_debug;
    uint64_t total_trace;
} backup_state_t;

// Function declarations for backup.c
void backup_init(void);
backup_id_t backup_create(backup_type_t type, backup_flags_t flags, const char* description, backup_id_t parent_id);
int backup_delete(backup_id_t id);
int backup_restore(backup_id_t id);
int backup_get_info(backup_id_t id, backup_info_t* info);
size_t backup_get_list(int type, backup_id_t* backups, size_t max_count);
size_t backup_get_count(int type);
backup_id_t backup_get_latest(backup_type_t type);
backup_id_t backup_create_kernel(const char* description);
backup_id_t backup_create_filesystem(const char* description);
backup_id_t backup_create_configuration(const char* description);
backup_id_t backup_create_full(const char* description);
int backup_schedule_automatic(backup_type_t type, uint64_t interval_ms, const char* description_prefix);
int backup_cancel_automatic(backup_type_t type);
int backup_set_max_count(backup_type_t type, size_t max_count);
int backup_set_max_size(backup_type_t type, uint64_t max_size);
int backup_export(backup_id_t id, const char* filename);
backup_id_t backup_import(const char* filename, const char* description);

// Helper function declarations
uint64_t get_current_timestamp(void);
const char* get_backup_type_name(backup_type_t type);
uint64_t calculate_backup_size(backup_id_t id);
int create_full_system_snapshot(void** data, size_t* data_size);
int create_kernel_snapshot(void** data, size_t* data_size);
int create_filesystem_snapshot(void** data, size_t* data_size);
int create_configuration_snapshot(void** data, size_t* data_size);
int create_custom_snapshot(void** data, size_t* data_size);
int compress_data(void* data, size_t data_size, void** compressed_data, size_t* compressed_size);
int decompress_data(void* compressed_data, size_t compressed_size, void** data, size_t* data_size);
int write_file(const char* filename, void* data, size_t data_size);
int read_file(const char* filename, void** data, size_t* data_size);
int delete_file(const char* filename);
int file_exists(const char* filename);
int update_backup_index(void);
uint32_t register_timer(uint64_t interval_ms, void (*callback)(void*), void* data);
int cancel_timer(uint32_t timer_id);
backup_timer_t* find_backup_timer(backup_type_t type);
int add_backup_timer(backup_timer_t* timer);
int remove_backup_timer(backup_type_t type);
void backup_timer_callback(void* data);
int restore_full_system_snapshot(void* data, size_t data_size);
int restore_kernel_snapshot(void* data, size_t data_size);
int restore_filesystem_snapshot(void* data, size_t data_size);
int restore_configuration_snapshot(void* data, size_t data_size);
int restore_custom_snapshot(void* data, size_t data_size);

#endif // NEUROOS_BACKUP_H
