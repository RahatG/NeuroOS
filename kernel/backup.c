/**
 * backup.c - Backup system implementation for NeuroOS
 * 
 * This file implements the backup system, which is responsible for creating
 * and managing backups of the system state to enable rollback in case of failures.
 */

#include "include/backup.h"
#include "include/memory.h"
#include "include/console.h"
#include <string.h>
#include <stdlib.h>

// Maximum number of backups
#define MAX_BACKUPS 64

// Backup table
static backup_info_t* backup_table[MAX_BACKUPS];

// Next available backup ID
static backup_id_t next_backup_id = 1;

// Maximum number of backups to keep for each type
static size_t max_backups_per_type[BACKUP_TYPE_CUSTOM + 1] = {
    10,  // BACKUP_TYPE_FULL
    20,  // BACKUP_TYPE_KERNEL
    20,  // BACKUP_TYPE_FILESYSTEM
    10,  // BACKUP_TYPE_CONFIGURATION
    10   // BACKUP_TYPE_CUSTOM
};

// Maximum total size of backups for each type (in bytes)
static uint64_t max_backup_size_per_type[BACKUP_TYPE_CUSTOM + 1] = {
    1024ULL * 1024 * 1024,  // 1 GB for BACKUP_TYPE_FULL
    512ULL * 1024 * 1024,   // 512 MB for BACKUP_TYPE_KERNEL
    2048ULL * 1024 * 1024,  // 2 GB for BACKUP_TYPE_FILESYSTEM
    64ULL * 1024 * 1024,    // 64 MB for BACKUP_TYPE_CONFIGURATION
    256ULL * 1024 * 1024    // 256 MB for BACKUP_TYPE_CUSTOM
};

// Forward declarations
static void backup_cleanup_old(backup_type_t type);
static int backup_check_limits(backup_type_t type);

/**
 * Initialize the backup system
 */
void backup_init(void) {
    // Initialize the backup table
    for (int i = 0; i < MAX_BACKUPS; i++) {
        backup_table[i] = NULL;
    }
    
    console_printf("Backup system initialized\n");
}

/**
 * Create a new backup
 * 
 * @param type: Backup type
 * @param flags: Backup flags
 * @param description: Backup description
 * @param parent_id: Parent backup ID (for incremental backups), 0 for full backups
 * @return: Backup ID on success, 0 on failure
 */
backup_id_t backup_create(backup_type_t type, backup_flags_t flags,
                         const char* description, backup_id_t parent_id) {
    // Check if the backup type is valid
    if (type > BACKUP_TYPE_CUSTOM) {
        console_printf("Error: Invalid backup type\n");
        return 0;
    }
    
    // Check if we have reached the maximum number of backups
    if (next_backup_id >= MAX_BACKUPS) {
        console_printf("Error: Maximum number of backups reached\n");
        return 0;
    }
    
    // Check if we need to clean up old backups
    backup_cleanup_old(type);
    
    // Allocate memory for the backup information
    backup_info_t* backup = (backup_info_t*)memory_alloc(sizeof(backup_info_t), MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!backup) {
        console_printf("Error: Failed to allocate backup information\n");
        return 0;
    }
    
    // Initialize the backup information
    backup->id = next_backup_id++;
    backup->type = type;
    backup->state = BACKUP_STATE_CREATING;
    backup->flags = flags;
    backup->creation_time = 0; // Will be set when the backup is complete
    backup->size = 0; // Will be set when the backup is complete
    backup->parent_id = parent_id;
    
    // Set the backup description
    if (description) {
        strncpy(backup->description, description, BACKUP_DESCRIPTION_MAX - 1);
        backup->description[BACKUP_DESCRIPTION_MAX - 1] = '\0'; // Ensure null termination
    } else {
        strncpy(backup->description, "Unnamed backup", BACKUP_DESCRIPTION_MAX - 1);
        backup->description[BACKUP_DESCRIPTION_MAX - 1] = '\0'; // Ensure null termination
    }
    
    // Add the backup to the backup table
    backup_table[backup->id] = backup;
    
    // Create the backup
    // Allocate memory for the backup data
    void* backup_data = NULL;
    size_t backup_data_size = 0;
    
    // Determine what to back up based on the type
    switch (type) {
        case BACKUP_TYPE_FULL:
            // Full system backup includes kernel, filesystem, and configuration
            if (create_full_system_snapshot(&backup_data, &backup_data_size) != 0) {
                console_printf("Error: Failed to create full system snapshot\n");
                backup->state = BACKUP_STATE_ERROR;
                return backup->id;
            }
            break;
            
        case BACKUP_TYPE_KERNEL:
            // Kernel backup includes kernel code and data
            if (create_kernel_snapshot(&backup_data, &backup_data_size) != 0) {
                console_printf("Error: Failed to create kernel snapshot\n");
                backup->state = BACKUP_STATE_ERROR;
                return backup->id;
            }
            break;
            
        case BACKUP_TYPE_FILESYSTEM:
            // Filesystem backup includes file data and metadata
            if (create_filesystem_snapshot(&backup_data, &backup_data_size) != 0) {
                console_printf("Error: Failed to create filesystem snapshot\n");
                backup->state = BACKUP_STATE_ERROR;
                return backup->id;
            }
            break;
            
        case BACKUP_TYPE_CONFIGURATION:
            // Configuration backup includes system settings
            if (create_configuration_snapshot(&backup_data, &backup_data_size) != 0) {
                console_printf("Error: Failed to create configuration snapshot\n");
                backup->state = BACKUP_STATE_ERROR;
                return backup->id;
            }
            break;
            
        case BACKUP_TYPE_CUSTOM:
            // Custom backup type
            if (create_custom_snapshot(&backup_data, &backup_data_size) != 0) {
                console_printf("Error: Failed to create custom snapshot\n");
                backup->state = BACKUP_STATE_ERROR;
                return backup->id;
            }
            break;
    }
    
    // Compress the backup data
    void* compressed_data = NULL;
    size_t compressed_size = 0;
    
    if (compress_data(backup_data, backup_data_size, &compressed_data, &compressed_size) != 0) {
        console_printf("Error: Failed to compress backup data\n");
        memory_free(backup_data, backup_data_size);
        backup->state = BACKUP_STATE_ERROR;
        return backup->id;
    }
    
    // Free the uncompressed data
    memory_free(backup_data, backup_data_size);
    
    // Generate a unique filename for the backup
    char filename[256];
    snprintf(filename, sizeof(filename), "/backups/%s_%u_%lu.bak",
             get_backup_type_name(type), backup->id, (unsigned long)get_current_timestamp());
    
    // Write the compressed data to the backup file
    if (write_file(filename, compressed_data, compressed_size) != 0) {
        console_printf("Error: Failed to write backup file\n");
        memory_free(compressed_data, compressed_size);
        backup->state = BACKUP_STATE_ERROR;
        return backup->id;
    }
    
    // Free the compressed data
    memory_free(compressed_data, compressed_size);
    
    // Store the backup filename
    strncpy(backup->filename, filename, sizeof(backup->filename) - 1);
    backup->filename[sizeof(backup->filename) - 1] = '\0'; // Ensure null termination
    
    // Set the backup state to ready
    backup->state = BACKUP_STATE_READY;
    
    // Set the creation time to the current system time
    backup->creation_time = get_current_timestamp();
    
    // Calculate and set the actual backup size
    backup->size = calculate_backup_size(backup->id);
    
    return backup->id;
}

/**
 * Delete a backup
 * 
 * @param id: Backup ID
 * @return: 0 on success, -1 on failure
 */
int backup_delete(backup_id_t id) {
    // Check if the backup ID is valid
    if (id >= MAX_BACKUPS || !backup_table[id]) {
        console_printf("Error: Invalid backup ID\n");
        return -1;
    }
    
    // Get the backup information
    backup_info_t* backup = backup_table[id];
    
    // Check if the backup is critical
    if (backup->flags & BACKUP_FLAG_CRITICAL) {
        console_printf("Error: Cannot delete critical backup\n");
        return -1;
    }
    
    // Delete the backup
    // Check if the backup file exists
    if (file_exists(backup->filename)) {
        // Delete the backup file
        if (delete_file(backup->filename) != 0) {
            console_printf("Error: Failed to delete backup file: %s\n", backup->filename);
            return -1;
        }
    }
    
    // If this is an incremental backup, we need to update the chain
    if (backup->parent_id != 0) {
        // Find all backups that have this backup as their parent
        for (int i = 0; i < MAX_BACKUPS; i++) {
            if (backup_table[i] && backup_table[i]->parent_id == id) {
                // Update the parent ID to this backup's parent
                backup_table[i]->parent_id = backup->parent_id;
            }
        }
    }
    
    // Update the backup index file
    if (update_backup_index() != 0) {
        console_printf("Warning: Failed to update backup index\n");
    }
    
    // Set the backup state to deleted
    backup->state = BACKUP_STATE_DELETED;
    
    // Remove the backup from the backup table
    backup_table[id] = NULL;
    
    // Free the backup information
    memory_free(backup, sizeof(backup_info_t));
    
    return 0;
}

/**
 * Restore a backup
 * 
 * @param id: Backup ID
 * @return: 0 on success, -1 on failure
 */
int backup_restore(backup_id_t id) {
    // Check if the backup ID is valid
    if (id >= MAX_BACKUPS || !backup_table[id]) {
        console_printf("Error: Invalid backup ID\n");
        return -1;
    }
    
    // Get the backup information
    backup_info_t* backup = backup_table[id];
    
    // Check if the backup is ready
    if (backup->state != BACKUP_STATE_READY) {
        console_printf("Error: Backup is not ready\n");
        return -1;
    }
    
    // Set the backup state to restoring
    backup->state = BACKUP_STATE_RESTORING;
    
    // Restore the backup
    // Check if the backup file exists
    if (!file_exists(backup->filename)) {
        console_printf("Error: Backup file not found: %s\n", backup->filename);
        backup->state = BACKUP_STATE_ERROR;
        return -1;
    }
    
    // Read the backup file
    void* compressed_data = NULL;
    size_t compressed_size = 0;
    
    if (read_file(backup->filename, &compressed_data, &compressed_size) != 0) {
        console_printf("Error: Failed to read backup file\n");
        backup->state = BACKUP_STATE_ERROR;
        return -1;
    }
    
    // Decompress the backup data
    void* backup_data = NULL;
    size_t backup_data_size = 0;
    
    if (decompress_data(compressed_data, compressed_size, &backup_data, &backup_data_size) != 0) {
        console_printf("Error: Failed to decompress backup data\n");
        memory_free(compressed_data, compressed_size);
        backup->state = BACKUP_STATE_ERROR;
        return -1;
    }
    
    // Free the compressed data
    memory_free(compressed_data, compressed_size);
    
    // Apply the backup based on its type
    int result = -1;
    
    switch (backup->type) {
        case BACKUP_TYPE_FULL:
            // Restore full system state
            result = restore_full_system_snapshot(backup_data, backup_data_size);
            break;
            
        case BACKUP_TYPE_KERNEL:
            // Restore kernel code and data
            result = restore_kernel_snapshot(backup_data, backup_data_size);
            break;
            
        case BACKUP_TYPE_FILESYSTEM:
            // Restore filesystem data
            result = restore_filesystem_snapshot(backup_data, backup_data_size);
            break;
            
        case BACKUP_TYPE_CONFIGURATION:
            // Restore system configuration
            result = restore_configuration_snapshot(backup_data, backup_data_size);
            break;
            
        case BACKUP_TYPE_CUSTOM:
            // Restore custom data
            result = restore_custom_snapshot(backup_data, backup_data_size);
            break;
    }
    
    // Free the backup data
    memory_free(backup_data, backup_data_size);
    
    if (result != 0) {
        console_printf("Error: Failed to restore backup\n");
        backup->state = BACKUP_STATE_ERROR;
        return -1;
    }
    
    // Set the backup state back to ready
    backup->state = BACKUP_STATE_READY;
    
    return 0;
}

/**
 * Get backup information
 * 
 * @param id: Backup ID
 * @param info: Pointer to store the backup information
 * @return: 0 on success, -1 on failure
 */
int backup_get_info(backup_id_t id, backup_info_t* info) {
    // Check if the backup ID is valid
    if (id >= MAX_BACKUPS || !backup_table[id]) {
        console_printf("Error: Invalid backup ID\n");
        return -1;
    }
    
    // Check if the info pointer is valid
    if (!info) {
        console_printf("Error: Invalid info pointer\n");
        return -1;
    }
    
    // Get the backup information
    backup_info_t* backup = backup_table[id];
    
    // Copy the backup information
    *info = *backup;
    
    return 0;
}

/**
 * Get the list of backups
 * 
 * @param type: Backup type (or -1 for all types)
 * @param backups: Array to store the backup IDs
 * @param max_count: Maximum number of backup IDs to store
 * @return: Number of backup IDs stored
 */
size_t backup_get_list(int type, backup_id_t* backups, size_t max_count) {
    // Check if the backups pointer is valid
    if (!backups || max_count == 0) {
        return 0;
    }
    
    // Get the list of backups
    size_t count = 0;
    
    for (int i = 0; i < MAX_BACKUPS && count < max_count; i++) {
        if (backup_table[i] && (type == -1 || (backup_type_t)backup_table[i]->type == (backup_type_t)type)) {
            backups[count++] = backup_table[i]->id;
        }
    }
    
    return count;
}

/**
 * Get the number of backups
 * 
 * @param type: Backup type (or -1 for all types)
 * @return: Number of backups
 */
size_t backup_get_count(int type) {
    // Count the number of backups
    size_t count = 0;
    
    for (int i = 0; i < MAX_BACKUPS; i++) {
        if (backup_table[i] && (type == -1 || (backup_type_t)backup_table[i]->type == (backup_type_t)type)) {
            count++;
        }
    }
    
    return count;
}

/**
 * Get the latest backup
 * 
 * @param type: Backup type
 * @return: Backup ID on success, 0 if no backups are available
 */
backup_id_t backup_get_latest(backup_type_t type) {
    // Check if the backup type is valid
    if (type > BACKUP_TYPE_CUSTOM) {
        console_printf("Error: Invalid backup type\n");
        return 0;
    }
    
    // Find the latest backup
    backup_id_t latest_id = 0;
    uint64_t latest_time = 0;
    
    for (int i = 0; i < MAX_BACKUPS; i++) {
        if (backup_table[i] && backup_table[i]->type == type &&
            backup_table[i]->state == BACKUP_STATE_READY &&
            backup_table[i]->creation_time > latest_time) {
            latest_id = backup_table[i]->id;
            latest_time = backup_table[i]->creation_time;
        }
    }
    
    return latest_id;
}

/**
 * Create a backup of the kernel code
 * 
 * @param description: Backup description
 * @return: Backup ID on success, 0 on failure
 */
backup_id_t backup_create_kernel(const char* description) {
    return backup_create(BACKUP_TYPE_KERNEL, BACKUP_FLAG_NONE, description, 0);
}

/**
 * Create a backup of the filesystem
 * 
 * @param description: Backup description
 * @return: Backup ID on success, 0 on failure
 */
backup_id_t backup_create_filesystem(const char* description) {
    return backup_create(BACKUP_TYPE_FILESYSTEM, BACKUP_FLAG_NONE, description, 0);
}

/**
 * Create a backup of the system configuration
 * 
 * @param description: Backup description
 * @return: Backup ID on success, 0 on failure
 */
backup_id_t backup_create_configuration(const char* description) {
    return backup_create(BACKUP_TYPE_CONFIGURATION, BACKUP_FLAG_NONE, description, 0);
}

/**
 * Create a full system backup
 * 
 * @param description: Backup description
 * @return: Backup ID on success, 0 on failure
 */
backup_id_t backup_create_full(const char* description) {
    return backup_create(BACKUP_TYPE_FULL, BACKUP_FLAG_NONE, description, 0);
}

/**
 * Schedule automatic backups
 * 
 * @param type: Backup type
 * @param interval_ms: Backup interval in milliseconds
 * @param description_prefix: Prefix for the backup description
 * @return: 0 on success, -1 on failure
 */
int backup_schedule_automatic(backup_type_t type, uint64_t interval_ms,
                             const char* description_prefix) {
    // Check if the backup type is valid
    if (type > BACKUP_TYPE_CUSTOM) {
        console_printf("Error: Invalid backup type\n");
        return -1;
    }
    
    // Set up a timer to trigger backups at the specified interval
    // Create a backup timer structure
    backup_timer_t* timer = (backup_timer_t*)memory_alloc(sizeof(backup_timer_t), 
                                                         MEMORY_PROT_READ | MEMORY_PROT_WRITE, 
                                                         MEMORY_ALLOC_ZEROED);
    
    if (!timer) {
        console_printf("Error: Failed to allocate backup timer\n");
        return -1;
    }
    
    // Initialize the timer
    timer->type = type;
    timer->interval_ms = interval_ms;
    timer->next_backup_time = get_current_timestamp() + interval_ms;
    
    // Set the description prefix
    if (description_prefix) {
        strncpy(timer->description_prefix, description_prefix, sizeof(timer->description_prefix) - 1);
    } else {
        strncpy(timer->description_prefix, "Auto backup", sizeof(timer->description_prefix) - 1);
    }
    
    // Register the timer with the system
    timer->timer_id = register_timer(interval_ms, backup_timer_callback, timer);
    
    if (timer->timer_id == 0) {
        console_printf("Error: Failed to register backup timer\n");
        memory_free(timer, sizeof(backup_timer_t));
        return -1;
    }
    
    // Store the timer in the backup timer table
    if (add_backup_timer(timer) != 0) {
        console_printf("Error: Failed to add backup timer\n");
        cancel_timer(timer->timer_id);
        memory_free(timer, sizeof(backup_timer_t));
        return -1;
    }
    
    return 0;
}

/**
 * Cancel automatic backups
 * 
 * @param type: Backup type
 * @return: 0 on success, -1 on failure
 */
int backup_cancel_automatic(backup_type_t type) {
    // Check if the backup type is valid
    if (type > BACKUP_TYPE_CUSTOM) {
        console_printf("Error: Invalid backup type\n");
        return -1;
    }
    
    // Find and cancel the timer associated with this backup type
    backup_timer_t* timer = find_backup_timer(type);
    
    if (!timer) {
        console_printf("Error: No automatic backup scheduled for this type\n");
        return -1;
    }
    
    // Cancel the timer
    if (cancel_timer(timer->timer_id) != 0) {
        console_printf("Error: Failed to cancel backup timer\n");
        return -1;
    }
    
    // Remove the timer from the backup timer table
    if (remove_backup_timer(type) != 0) {
        console_printf("Error: Failed to remove backup timer\n");
        return -1;
    }
    
    // Free the timer structure
    memory_free(timer, sizeof(backup_timer_t));
    
    return 0;
}

/**
 * Set the maximum number of backups to keep
 * 
 * @param type: Backup type
 * @param max_count: Maximum number of backups to keep (0 for unlimited)
 * @return: 0 on success, -1 on failure
 */
int backup_set_max_count(backup_type_t type, size_t max_count) {
    // Check if the backup type is valid
    if (type > BACKUP_TYPE_CUSTOM) {
        console_printf("Error: Invalid backup type\n");
        return -1;
    }
    
    // Set the maximum number of backups
    max_backups_per_type[type] = max_count;
    
    // Check if we need to clean up old backups
    backup_cleanup_old(type);
    
    return 0;
}

/**
 * Set the maximum total size of backups to keep
 * 
 * @param type: Backup type
 * @param max_size: Maximum total size in bytes (0 for unlimited)
 * @return: 0 on success, -1 on failure
 */
int backup_set_max_size(backup_type_t type, uint64_t max_size) {
    // Check if the backup type is valid
    if (type > BACKUP_TYPE_CUSTOM) {
        console_printf("Error: Invalid backup type\n");
        return -1;
    }
    
    // Set the maximum total size
    max_backup_size_per_type[type] = max_size;
    
    // Check if we need to clean up old backups
    backup_cleanup_old(type);
    
    return 0;
}

/**
 * Export a backup to a file
 * 
 * @param id: Backup ID
 * @param filename: Output filename
 * @return: 0 on success, -1 on failure
 */
int backup_export(backup_id_t id, const char* filename) {
    // Check if the backup ID is valid
    if (id >= MAX_BACKUPS || !backup_table[id]) {
        console_printf("Error: Invalid backup ID\n");
        return -1;
    }
    
    // Check if the filename is valid
    if (!filename) {
        console_printf("Error: Invalid filename\n");
        return -1;
    }
    
    // Get the backup information
    backup_info_t* backup = backup_table[id];
    
    // Check if the backup is ready
    if (backup->state != BACKUP_STATE_READY) {
        console_printf("Error: Backup is not ready\n");
        return -1;
    }
    
    // Export the backup
    // Check if the backup file exists
    if (!file_exists(backup->filename)) {
        console_printf("Error: Backup file not found: %s\n", backup->filename);
        return -1;
    }
    
    // Create the export file
    FILE* export_file = fopen(filename, "wb");
    if (!export_file) {
        console_printf("Error: Failed to create export file: %s\n", filename);
        return -1;
    }
    
    // Write the backup header
    backup_header_t header;
    memset(&header, 0, sizeof(header));
    
    header.magic = BACKUP_MAGIC;
    header.version = BACKUP_VERSION;
    header.type = backup->type;
    header.flags = backup->flags;
    header.creation_time = backup->creation_time;
    header.size = backup->size;
    header.parent_id = backup->parent_id;
    
    // Copy description with explicit length limit to avoid truncation warning
    memcpy(header.description, backup->description, sizeof(header.description) - 1);
    header.description[sizeof(header.description) - 1] = '\0'; // Ensure null termination
    
    if (fwrite(&header, sizeof(header), 1, export_file) != 1) {
        console_printf("Error: Failed to write backup header\n");
        fclose(export_file);
        return -1;
    }
    
    // Open the backup file
    FILE* backup_file = fopen(backup->filename, "rb");
    if (!backup_file) {
        console_printf("Error: Failed to open backup file: %s\n", backup->filename);
        fclose(export_file);
        return -1;
    }
    
    // Copy the backup data to the export file
    char buffer[4096];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), backup_file)) > 0) {
        if (fwrite(buffer, 1, bytes_read, export_file) != bytes_read) {
            console_printf("Error: Failed to write backup data\n");
            fclose(backup_file);
            fclose(export_file);
            return -1;
        }
    }
    
    // Close the files
    fclose(backup_file);
    fclose(export_file);
    
    return 0;
}

/**
 * Import a backup from a file
 * 
 * @param filename: Input filename
 * @param description: Backup description
 * @return: Backup ID on success, 0 on failure
 */
backup_id_t backup_import(const char* filename, const char* description) {
    // Check if the filename is valid
    if (!filename) {
        console_printf("Error: Invalid filename\n");
        return 0;
    }
    
    // Check if we have reached the maximum number of backups
    if (next_backup_id >= MAX_BACKUPS) {
        console_printf("Error: Maximum number of backups reached\n");
        return 0;
    }
    
    // Import the backup
    // Open the import file
    FILE* import_file = fopen(filename, "rb");
    if (!import_file) {
        console_printf("Error: Failed to open import file: %s\n", filename);
        return 0;
    }
    
    // Read the backup header
    backup_header_t header;
    if (fread(&header, sizeof(header), 1, import_file) != 1) {
        console_printf("Error: Failed to read backup header\n");
        fclose(import_file);
        return 0;
    }
    
    // Verify the backup header
    if (header.magic != BACKUP_MAGIC) {
        console_printf("Error: Invalid backup file format\n");
        fclose(import_file);
        return 0;
    }
    
    // Check if the backup version is compatible
    if (header.version > BACKUP_VERSION) {
        console_printf("Error: Backup version not supported\n");
        fclose(import_file);
        return 0;
    }
    
    // Create a new backup
    backup_id_t id = backup_create(header.type, header.flags, description ? description : header.description, 0);
    if (id == 0) {
        console_printf("Error: Failed to create backup\n");
        fclose(import_file);
        return 0;
    }
    
    // Get the backup information
    backup_info_t* backup = backup_table[id];
    
    // Set the backup creation time to match the imported backup
    backup->creation_time = header.creation_time;
    
    // Generate a unique filename for the backup
    char backup_filename[256];
    snprintf(backup_filename, sizeof(backup_filename), "/backups/%s_%u_%lu.bak", 
             get_backup_type_name(header.type), id, (unsigned long)header.creation_time);
    
    // Create the backup file
    FILE* backup_file = fopen(backup_filename, "wb");
    if (!backup_file) {
        console_printf("Error: Failed to create backup file: %s\n", backup_filename);
        fclose(import_file);
        backup_delete(id);
        return 0;
    }
    
    // Copy the backup data to the backup file
    char buffer[4096];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), import_file)) > 0) {
        if (fwrite(buffer, 1, bytes_read, backup_file) != bytes_read) {
            console_printf("Error: Failed to write backup data\n");
            fclose(import_file);
            fclose(backup_file);
            backup_delete(id);
            return 0;
        }
    }
    
    // Close the files
    fclose(import_file);
    fclose(backup_file);
    
    // Store the backup filename
    strncpy(backup->filename, backup_filename, sizeof(backup->filename) - 1);
    backup->filename[sizeof(backup->filename) - 1] = '\0'; // Ensure null termination
    
    // Set the backup state to ready
    backup->state = BACKUP_STATE_READY;
    
    // Set the backup size
    backup->size = header.size;
    
    return id;
}

/**
 * Clean up old backups of a specific type
 * 
 * @param type: Backup type
 */
static void backup_cleanup_old(backup_type_t type) {
    // Check if the backup type is valid
    if (type > BACKUP_TYPE_CUSTOM) {
        console_printf("Error: Invalid backup type\n");
        return;
    }
    
    // Check if we need to clean up old backups
    if (backup_check_limits(type) == 0) {
        // No cleanup needed
        return;
    }
    
    // Get the list of backups of this type
    backup_id_t backups[MAX_BACKUPS];
    size_t count = backup_get_list(type, backups, MAX_BACKUPS);
    
    // Sort the backups by creation time (oldest first)
    for (size_t i = 0; i < count; i++) {
        for (size_t j = i + 1; j < count; j++) {
            backup_info_t info_i, info_j;
            
            if (backup_get_info(backups[i], &info_i) == 0 &&
                backup_get_info(backups[j], &info_j) == 0) {
                if (info_i.creation_time > info_j.creation_time) {
                    // Swap the backup IDs
                    backup_id_t temp = backups[i];
                    backups[i] = backups[j];
                    backups[j] = temp;
                }
            }
        }
    }
    
    // Delete old backups until we're under the limits
    for (size_t i = 0; i < count && backup_check_limits(type) != 0; i++) {
        backup_info_t info;
        
        if (backup_get_info(backups[i], &info) == 0) {
            // Skip critical backups
            if (info.flags & BACKUP_FLAG_CRITICAL) {
                continue;
            }
            
            // Delete the backup
            backup_delete(backups[i]);
        }
    }
}

/**
 * Check if we've reached the limits for a backup type
 * 
 * @param type: Backup type
 * @return: 1 if limits are exceeded, 0 otherwise
 */
static int backup_check_limits(backup_type_t type) {
    // Check if the backup type is valid
    if (type > BACKUP_TYPE_CUSTOM) {
        console_printf("Error: Invalid backup type\n");
        return 0;
    }
    
    // Check if we've reached the maximum number of backups
    size_t count = backup_get_count(type);
    if (max_backups_per_type[type] > 0 && count >= max_backups_per_type[type]) {
        return 1;
    }
    
    // Check if we've reached the maximum total size
    if (max_backup_size_per_type[type] > 0) {
        uint64_t total_size = 0;
        
        // Get the list of backups of this type
        backup_id_t backups[MAX_BACKUPS];
        size_t backup_count = backup_get_list(type, backups, MAX_BACKUPS);
        
        // Calculate the total size
        for (size_t i = 0; i < backup_count; i++) {
            backup_info_t info;
            
            if (backup_get_info(backups[i], &info) == 0) {
                total_size += info.size;
            }
        }
        
        if (total_size >= max_backup_size_per_type[type]) {
            return 1;
        }
    }
    
    return 0;
}
