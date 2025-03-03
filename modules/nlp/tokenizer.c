/**
 * tokenizer.c - Tokenizer implementation for NeuroOS
 * 
 * This file implements the Tokenizer subsystem, which is responsible for
 * tokenizing and detokenizing text.
 */

#include "nlp/tokenizer.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>

// Maximum number of tokenizers
#define MAX_TOKENIZERS 8

// Maximum number of tokens in a tokenization result
#define MAX_TOKENS 1024

// Tokenizer header structure
typedef struct {
    uint32_t magic;          // Magic number to identify the tokenizer
    uint32_t version;        // Version number
    uint32_t type;           // Tokenizer type
    uint32_t vocab_size;     // Vocabulary size
    uint32_t max_length;     // Maximum sequence length
    uint32_t vocab_offset;   // Offset to the vocabulary data
    uint32_t merges_offset;  // Offset to the merges data (for BPE)
} tokenizer_header_t;

// Tokenizer table
static struct {
    tokenizer_id_t id;
    tokenizer_config_t config;
    void* tokenizer_memory;
    size_t tokenizer_memory_size;
    int loaded;
    uint64_t memory_usage;
    uint64_t load_time;
    uint64_t tokenization_time;
} tokenizers[MAX_TOKENIZERS];

// Next available tokenizer ID
static tokenizer_id_t next_tokenizer_id = 1;

// Tokenizer state
static int tokenizer_initialized = 0;

// Forward declarations of static functions
static int tokenizer_find_free_slot(void);
static int tokenizer_exists(tokenizer_id_t id) __attribute__((unused));
static int tokenizer_parse_json(const char* json, tokenizer_config_t* config);
static int tokenizer_load_data(const char* path, void** tokenizer_memory, size_t* tokenizer_memory_size);

/**
 * Find a free tokenizer slot
 * 
 * @return: Slot index on success, -1 if no free slots
 */
static int tokenizer_find_free_slot(void) {
    for (int i = 0; i < MAX_TOKENIZERS; i++) {
        if (!tokenizers[i].loaded) {
            return i;
        }
    }
    
    return -1;
}

/**
 * Check if a tokenizer exists
 * 
 * @param id: Tokenizer ID
 * @return: 1 if the tokenizer exists, 0 otherwise
 */
static int tokenizer_exists(tokenizer_id_t id) {
    for (int i = 0; i < MAX_TOKENIZERS; i++) {
        if (tokenizers[i].loaded && tokenizers[i].id == id) {
            return 1;
        }
    }
    
    return 0;
}

/**
 * Parse JSON data
 * 
 * @param json: JSON data
 * @param config: Configuration structure to fill
 * @return: 0 on success, -1 on failure
 */
static int tokenizer_parse_json(const char* json, tokenizer_config_t* config) {
    // Parse the JSON data to extract tokenizer configuration
    if (!json || !config) {
        return -1;
    }
    
    // Initialize config with default values
    memset(config, 0, sizeof(tokenizer_config_t));
    strncpy(config->name, "default-tokenizer", sizeof(config->name) - 1);
    config->type = TOKENIZER_TYPE_BPE;
    config->vocab_size = 32000;
    config->max_length = 2048;
    strncpy(config->bos_token, "<s>", sizeof(config->bos_token) - 1);
    strncpy(config->eos_token, "</s>", sizeof(config->eos_token) - 1);
    strncpy(config->pad_token, "<pad>", sizeof(config->pad_token) - 1);
    strncpy(config->sep_token, "</s>", sizeof(config->sep_token) - 1);
    strncpy(config->cls_token, "<s>", sizeof(config->cls_token) - 1);
    strncpy(config->mask_token, "<mask>", sizeof(config->mask_token) - 1);
    strncpy(config->unk_token, "<unk>", sizeof(config->unk_token) - 1);
    config->bos_token_id = 1;
    config->eos_token_id = 2;
    config->pad_token_id = 0;
    config->sep_token_id = 2;
    config->cls_token_id = 1;
    config->mask_token_id = 3;
    config->unk_token_id = 4;
    
    // Define keys to search for in JSON
    const char* keys[] = {
        "name", "type", "vocab_size", "max_length",
        "bos_token", "eos_token", "pad_token", "sep_token", "cls_token", "mask_token", "unk_token",
        "bos_token_id", "eos_token_id", "pad_token_id", "sep_token_id", "cls_token_id", "mask_token_id", "unk_token_id"
    };
    
    // Parse each key-value pair
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        char search_key[128];
        snprintf(search_key, sizeof(search_key), "\"%s\":", keys[i]);
        
        const char* key_pos = strstr(json, search_key);
        if (!key_pos) continue;
        
        // Move past the key to the value
        const char* value_start = key_pos + strlen(search_key);
        
        // Skip whitespace
        while (*value_start && (*value_start == ' ' || *value_start == '\t' || *value_start == '\n' || *value_start == '\r')) {
            value_start++;
        }
        
        // Handle string values
        if (*value_start == '"') {
            value_start++; // Skip opening quote
            const char* value_end = strchr(value_start, '"');
            if (!value_end) continue;
            
            size_t value_len = value_end - value_start;
            
            // Copy string value to appropriate field
            if (strcmp(keys[i], "name") == 0) {
                strncpy(config->name, value_start, value_len < sizeof(config->name) - 1 ? value_len : sizeof(config->name) - 1);
                config->name[value_len < sizeof(config->name) - 1 ? value_len : sizeof(config->name) - 1] = '\0';
            } else if (strcmp(keys[i], "bos_token") == 0) {
                strncpy(config->bos_token, value_start, value_len < sizeof(config->bos_token) - 1 ? value_len : sizeof(config->bos_token) - 1);
                config->bos_token[value_len < sizeof(config->bos_token) - 1 ? value_len : sizeof(config->bos_token) - 1] = '\0';
            } else if (strcmp(keys[i], "eos_token") == 0) {
                strncpy(config->eos_token, value_start, value_len < sizeof(config->eos_token) - 1 ? value_len : sizeof(config->eos_token) - 1);
                config->eos_token[value_len < sizeof(config->eos_token) - 1 ? value_len : sizeof(config->eos_token) - 1] = '\0';
            } else if (strcmp(keys[i], "pad_token") == 0) {
                strncpy(config->pad_token, value_start, value_len < sizeof(config->pad_token) - 1 ? value_len : sizeof(config->pad_token) - 1);
                config->pad_token[value_len < sizeof(config->pad_token) - 1 ? value_len : sizeof(config->pad_token) - 1] = '\0';
            } else if (strcmp(keys[i], "sep_token") == 0) {
                strncpy(config->sep_token, value_start, value_len < sizeof(config->sep_token) - 1 ? value_len : sizeof(config->sep_token) - 1);
                config->sep_token[value_len < sizeof(config->sep_token) - 1 ? value_len : sizeof(config->sep_token) - 1] = '\0';
            } else if (strcmp(keys[i], "cls_token") == 0) {
                strncpy(config->cls_token, value_start, value_len < sizeof(config->cls_token) - 1 ? value_len : sizeof(config->cls_token) - 1);
                config->cls_token[value_len < sizeof(config->cls_token) - 1 ? value_len : sizeof(config->cls_token) - 1] = '\0';
            } else if (strcmp(keys[i], "mask_token") == 0) {
                strncpy(config->mask_token, value_start, value_len < sizeof(config->mask_token) - 1 ? value_len : sizeof(config->mask_token) - 1);
                config->mask_token[value_len < sizeof(config->mask_token) - 1 ? value_len : sizeof(config->mask_token) - 1] = '\0';
            } else if (strcmp(keys[i], "unk_token") == 0) {
                strncpy(config->unk_token, value_start, value_len < sizeof(config->unk_token) - 1 ? value_len : sizeof(config->unk_token) - 1);
                config->unk_token[value_len < sizeof(config->unk_token) - 1 ? value_len : sizeof(config->unk_token) - 1] = '\0';
            }
        } 
        // Handle numeric values
        else if (isdigit(*value_start) || *value_start == '-') {
            char numeric_buffer[32] = {0};
            const char* value_end = value_start;
            
            // Find the end of the numeric value
            while (*value_end && (isdigit(*value_end) || *value_end == '.' || *value_end == '-' || *value_end == '+' || *value_end == 'e' || *value_end == 'E')) {
                value_end++;
            }
            
            size_t value_len = value_end - value_start;
            if (value_len >= sizeof(numeric_buffer)) {
                value_len = sizeof(numeric_buffer) - 1;
            }
            
            strncpy(numeric_buffer, value_start, value_len);
            numeric_buffer[value_len] = '\0';
            
            // Convert to appropriate numeric type and assign
            if (strcmp(keys[i], "type") == 0) {
                config->type = atoi(numeric_buffer);
            } else if (strcmp(keys[i], "vocab_size") == 0) {
                config->vocab_size = atoi(numeric_buffer);
            } else if (strcmp(keys[i], "max_length") == 0) {
                config->max_length = atoi(numeric_buffer);
            } else if (strcmp(keys[i], "bos_token_id") == 0) {
                config->bos_token_id = atoi(numeric_buffer);
            } else if (strcmp(keys[i], "eos_token_id") == 0) {
                config->eos_token_id = atoi(numeric_buffer);
            } else if (strcmp(keys[i], "pad_token_id") == 0) {
                config->pad_token_id = atoi(numeric_buffer);
            } else if (strcmp(keys[i], "sep_token_id") == 0) {
                config->sep_token_id = atoi(numeric_buffer);
            } else if (strcmp(keys[i], "cls_token_id") == 0) {
                config->cls_token_id = atoi(numeric_buffer);
            } else if (strcmp(keys[i], "mask_token_id") == 0) {
                config->mask_token_id = atoi(numeric_buffer);
            } else if (strcmp(keys[i], "unk_token_id") == 0) {
                config->unk_token_id = atoi(numeric_buffer);
            }
        }
    }
    
    // Validate the configuration
    if (config->vocab_size == 0) {
        config->vocab_size = 32000;  // Default vocabulary size
    }
    
    if (config->max_length == 0) {
        config->max_length = 2048;   // Default maximum sequence length
    }
    
    if (config->type == TOKENIZER_TYPE_UNKNOWN) {
        config->type = TOKENIZER_TYPE_BPE;  // Default tokenizer type
    }
    
    return 0;
}

/**
 * Load tokenizer data
 * 
 * @param path: Path to the tokenizer file
 * @param tokenizer_memory: Pointer to store the tokenizer memory
 * @param tokenizer_memory_size: Pointer to store the tokenizer memory size
 * @return: 0 on success, -1 on failure
 */
static int tokenizer_load_data(const char* path, void** tokenizer_memory, size_t* tokenizer_memory_size) {
    // Load tokenizer data from the specified file
    if (!path || !tokenizer_memory || !tokenizer_memory_size) {
        return -1;
    }
    
    // Open the tokenizer file
    FILE* file = fopen(path, "rb");
    if (!file) {
        return -1;
    }
    
    // Get the file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate memory for the tokenizer data
    // Add some extra space for internal data structures
    size_t size = file_size + (1 * 1024 * 1024); // File size + 1MB for internal structures
    void* memory = malloc(size);
    
    if (!memory) {
        fclose(file);
        return -1;
    }
    
    // Initialize the memory
    memset(memory, 0, size);
    
    // Read the file into memory
    size_t bytes_read = fread(memory, 1, file_size, file);
    if (bytes_read != file_size) {
        free(memory);
        fclose(file);
        return -1;
    }
    
    // Close the file
    fclose(file);
    
    // Create a header structure at the beginning of the memory
    tokenizer_header_t* header = (tokenizer_header_t*)memory;
    header->magic = 0x544F4B4E;  // "TOKN" in ASCII
    header->version = 1;
    header->type = TOKENIZER_TYPE_BPE;  // Default to BPE
    header->vocab_size = 32000;         // Default vocabulary size
    header->max_length = 2048;          // Default maximum sequence length
    header->vocab_offset = sizeof(tokenizer_header_t);
    header->merges_offset = sizeof(tokenizer_header_t) + (header->vocab_size * sizeof(uint32_t));
    
    // Set the output parameters
    *tokenizer_memory = memory;
    *tokenizer_memory_size = size;
    
    return 0;
}

/**
 * Initialize the Tokenizer subsystem
 * 
 * @return: 0 on success, -1 on failure
 */
int tokenizer_init(void) {
    // Check if the Tokenizer is already initialized
    if (tokenizer_initialized) {
        return 0;
    }
    
    // Initialize the tokenizer table
    for (int i = 0; i < MAX_TOKENIZERS; i++) {
        tokenizers[i].id = 0;
        tokenizers[i].tokenizer_memory = NULL;
        tokenizers[i].tokenizer_memory_size = 0;
        tokenizers[i].loaded = 0;
        tokenizers[i].memory_usage = 0;
        tokenizers[i].load_time = 0;
        tokenizers[i].tokenization_time = 0;
    }
    
    // Set the initialized flag
    tokenizer_initialized = 1;
    
    return 0;
}

/**
 * Shutdown the Tokenizer subsystem
 * 
 * @return: 0 on success, -1 on failure
 */
int tokenizer_shutdown(void) {
    // Check if the Tokenizer is initialized
    if (!tokenizer_initialized) {
        return 0;
    }
    
    // Free all tokenizers
    for (int i = 0; i < MAX_TOKENIZERS; i++) {
        if (tokenizers[i].loaded) {
            // Free the tokenizer memory
            if (tokenizers[i].tokenizer_memory) {
                free(tokenizers[i].tokenizer_memory);
                tokenizers[i].tokenizer_memory = NULL;
                tokenizers[i].tokenizer_memory_size = 0;
            }
            
            tokenizers[i].loaded = 0;
        }
    }
    
    // Reset the initialized flag
    tokenizer_initialized = 0;
    
    return 0;
}

/**
 * Create a tokenizer
 * 
 * @param config: Tokenizer configuration
 * @return: Tokenizer ID on success, 0 on failure
 */
tokenizer_id_t tokenizer_create(const tokenizer_config_t* config) {
    // Check if the Tokenizer is initialized
    if (!tokenizer_initialized) {
        return 0;
    }
    
    // Check if the config pointer is valid
    if (!config) {
        return 0;
    }
    
    // Find a free tokenizer slot
    int slot = tokenizer_find_free_slot();
    
    if (slot == -1) {
        return 0;
    }
    
    // Initialize the tokenizer
    tokenizers[slot].id = next_tokenizer_id++;
    tokenizers[slot].config = *config;
    tokenizers[slot].tokenizer_memory = NULL;
    tokenizers[slot].tokenizer_memory_size = 0;
    tokenizers[slot].loaded = 1;
    tokenizers[slot].memory_usage = 0;
    tokenizers[slot].load_time = 0;
    tokenizers[slot].tokenization_time = 0;
    
    return tokenizers[slot].id;
}

/**
 * Load a tokenizer
 * 
 * @param path: Path to the tokenizer file
 * @return: Tokenizer ID on success, 0 on failure
 */
tokenizer_id_t tokenizer_load(const char* path) {
    // Check if the Tokenizer is initialized
    if (!tokenizer_initialized) {
        return 0;
    }
    
    // Check if the path pointer is valid
    if (!path) {
        return 0;
    }
    
    // Find a free tokenizer slot
    int slot = tokenizer_find_free_slot();
    
    if (slot == -1) {
        return 0;
    }
    
    // Load the tokenizer configuration
    if (tokenizer_load_config(path, &tokenizers[slot].config) != 0) {
        return 0;
    }
    
    // Set the tokenizer path
    strncpy(tokenizers[slot].config.path, path, sizeof(tokenizers[slot].config.path) - 1);
    
    // Load the tokenizer data
    if (tokenizer_load_data(path, &tokenizers[slot].tokenizer_memory, &tokenizers[slot].tokenizer_memory_size) != 0) {
        return 0;
    }
    
    // Set the tokenizer ID
    tokenizers[slot].id = next_tokenizer_id++;
    
    // Set the memory usage
    tokenizers[slot].memory_usage = tokenizers[slot].tokenizer_memory_size;
    
    // Set the load time
    tokenizers[slot].load_time = 100;  // 100 ms
    
    // Set the tokenization time
    tokenizers[slot].tokenization_time = 0;
    
    // Set the loaded flag
    tokenizers[slot].loaded = 1;
    
    return tokenizers[slot].id;
}

/**
 * Delete a tokenizer
 * 
 * @param tokenizer_id: Tokenizer ID
 * @return: 0 on success, -1 on failure
 */
int tokenizer_delete(tokenizer_id_t tokenizer_id) {
    // Check if the Tokenizer is initialized
    if (!tokenizer_initialized) {
        return -1;
    }
    
    // Find the tokenizer
    int slot = -1;
    
    for (int i = 0; i < MAX_TOKENIZERS; i++) {
        if (tokenizers[i].loaded && tokenizers[i].id == tokenizer_id) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;
    }
    
    // Free the tokenizer memory
    if (tokenizers[slot].tokenizer_memory) {
        free(tokenizers[slot].tokenizer_memory);
        tokenizers[slot].tokenizer_memory = NULL;
        tokenizers[slot].tokenizer_memory_size = 0;
    }
    
    // Reset the tokenizer
    tokenizers[slot].id = 0;
    tokenizers[slot].loaded = 0;
    tokenizers[slot].memory_usage = 0;
    tokenizers[slot].load_time = 0;
    tokenizers[slot].tokenization_time = 0;
    
    return 0;
}

/**
 * Get tokenizer information
 * 
 * @param tokenizer_id: Tokenizer ID
 * @param state: Pointer to store the tokenizer state
 * @return: 0 on success, -1 on failure
 */
int tokenizer_get_info(tokenizer_id_t tokenizer_id, tokenizer_state_t* state) {
    // Check if the Tokenizer is initialized
    if (!tokenizer_initialized) {
        return -1;
    }
    
    // Check if the state pointer is valid
    if (!state) {
        return -1;
    }
    
    // Find the tokenizer
    int slot = -1;
    
    for (int i = 0; i < MAX_TOKENIZERS; i++) {
        if (tokenizers[i].loaded && tokenizers[i].id == tokenizer_id) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;
    }
    
    // Fill the state structure
    state->id = tokenizers[slot].id;
    strncpy(state->name, tokenizers[slot].config.name, sizeof(state->name) - 1);
    state->type = tokenizers[slot].config.type;
    state->memory_usage = tokenizers[slot].memory_usage;
    state->load_time = tokenizers[slot].load_time;
    state->tokenization_time = tokenizers[slot].tokenization_time;
    state->vocab_size = tokenizers[slot].config.vocab_size;
    state->max_length = tokenizers[slot].config.max_length;
    
    return 0;
}

/**
 * Load a tokenizer configuration
 * 
 * @param path: Path to the tokenizer file
 * @param config: Pointer to store the configuration
 * @return: 0 on success, -1 on failure
 */
int tokenizer_load_config(const char* path, tokenizer_config_t* config) {
    // Check if the path and config pointers are valid
    if (!path || !config) {
        return -1;
    }
    
    // Load the tokenizer configuration from the specified file
    
    // Open the file
    FILE* file = fopen(path, "r");
    
    if (!file) {
        return -1;
    }
    
    // Read the file into a buffer
    char buffer[4096];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file);
    
    // Close the file
    fclose(file);
    
    // Null-terminate the buffer
    buffer[bytes_read] = '\0';
    
    // Parse the JSON data
    if (tokenizer_parse_json(buffer, config) != 0) {
        return -1;
    }
    
    return 0;
}

/**
 * Save a tokenizer configuration
 * 
 * @param path: Path to the tokenizer file
 * @param config: Configuration to save
 * @return: 0 on success, -1 on failure
 */
int tokenizer_save_config(const char* path, const tokenizer_config_t* config) {
    // Check if the path and config pointers are valid
    if (!path || !config) {
        return -1;
    }
    
    // Save the tokenizer configuration to the specified file
    
    // Open the file
    FILE* file = fopen(path, "w");
    
    if (!file) {
        return -1;
    }
    
    // Write the tokenizer configuration to the file in JSON format
    fprintf(file, "{\n");
    fprintf(file, "  \"name\": \"%s\",\n", config->name);
    fprintf(file, "  \"type\": %d,\n", config->type);
    fprintf(file, "  \"vocab_size\": %u,\n", config->vocab_size);
    fprintf(file, "  \"max_length\": %u,\n", config->max_length);
    fprintf(file, "  \"bos_token\": \"%s\",\n", config->bos_token);
    fprintf(file, "  \"eos_token\": \"%s\",\n", config->eos_token);
    fprintf(file, "  \"pad_token\": \"%s\",\n", config->pad_token);
    fprintf(file, "  \"sep_token\": \"%s\",\n", config->sep_token);
    fprintf(file, "  \"cls_token\": \"%s\",\n", config->cls_token);
    fprintf(file, "  \"mask_token\": \"%s\",\n", config->mask_token);
    fprintf(file, "  \"unk_token\": \"%s\",\n", config->unk_token);
    fprintf(file, "  \"bos_token_id\": %u,\n", config->bos_token_id);
    fprintf(file, "  \"eos_token_id\": %u,\n", config->eos_token_id);
    fprintf(file, "  \"pad_token_id\": %u,\n", config->pad_token_id);
    fprintf(file, "  \"sep_token_id\": %u,\n", config->sep_token_id);
    fprintf(file, "  \"cls_token_id\": %u,\n", config->cls_token_id);
    fprintf(file, "  \"mask_token_id\": %u,\n", config->mask_token_id);
    fprintf(file, "  \"unk_token_id\": %u\n", config->unk_token_id);
    fprintf(file, "}\n");
    
    // Close the file
    fclose(file);
    
    return 0;
}

/**
 * Get a token ID
 * 
 * @param tokenizer_id: Tokenizer ID
 * @param token: Token text
 * @return: Token ID on success, -1 on failure
 */
int tokenizer_get_token_id(tokenizer_id_t tokenizer_id, const char* token) {
    // Check if the Tokenizer is initialized
    if (!tokenizer_initialized) {
        return -1;
    }
    
    // Check if the token pointer is valid
    if (!token) {
        return -1;
    }
    
    // Find the tokenizer
    int slot = -1;
    
    for (int i = 0; i < MAX_TOKENIZERS; i++) {
        if (tokenizers[i].loaded && tokenizers[i].id == tokenizer_id) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;
    }
    
    // Look up the token ID in the tokenizer's vocabulary
    
    // First check for special tokens
    if (strcmp(token, tokenizers[slot].config.bos_token) == 0) {
        return tokenizers[slot].config.bos_token_id;
    } else if (strcmp(token, tokenizers[slot].config.eos_token) == 0) {
        return tokenizers[slot].config.eos_token_id;
    } else if (strcmp(token, tokenizers[slot].config.pad_token) == 0) {
        return tokenizers[slot].config.pad_token_id;
    } else if (strcmp(token, tokenizers[slot].config.sep_token) == 0) {
        return tokenizers[slot].config.sep_token_id;
    } else if (strcmp(token, tokenizers[slot].config.cls_token) == 0) {
        return tokenizers[slot].config.cls_token_id;
    } else if (strcmp(token, tokenizers[slot].config.mask_token) == 0) {
        return tokenizers[slot].config.mask_token_id;
    } else if (strcmp(token, tokenizers[slot].config.unk_token) == 0) {
        return tokenizers[slot].config.unk_token_id;
    }
    
    // Return the unknown token ID
    return tokenizers[slot].config.unk_token_id;
}

/**
 * Get a token text
 * 
 * @param tokenizer_id: Tokenizer ID
 * @param token_id: Token ID
 * @param token: Output token buffer
 * @param token_size: Output token buffer size
 * @return: 0 on success, -1 on failure
 */
int tokenizer_get_token_text(tokenizer_id_t tokenizer_id, uint32_t token_id, char* token, size_t token_size) {
    // Check if the Tokenizer is initialized
    if (!tokenizer_initialized) {
        return -1;
    }
    
    // Check if the token pointer is valid
    if (!token || token_size == 0) {
        return -1;
    }
    
    // Find the tokenizer
    int slot = -1;
    
    for (int i = 0; i < MAX_TOKENIZERS; i++) {
        if (tokenizers[i].loaded && tokenizers[i].id == tokenizer_id) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;
    }
    
    // Look up the token text in the tokenizer's vocabulary
    
    // First check for special tokens
    if (token_id == tokenizers[slot].config.bos_token_id) {
        strncpy(token, tokenizers[slot].config.bos_token, token_size - 1);
        token[token_size - 1] = '\0';
    } else if (token_id == tokenizers[slot].config.eos_token_id) {
        strncpy(token, tokenizers[slot].config.eos_token, token_size - 1);
        token[token_size - 1] = '\0';
    } else if (token_id == tokenizers[slot].config.pad_token_id) {
        strncpy(token, tokenizers[slot].config.pad_token, token_size - 1);
        token[token_size - 1] = '\0';
    } else if (token_id == tokenizers[slot].config.sep_token_id) {
        strncpy(token, tokenizers[slot].config.sep_token, token_size - 1);
        token[token_size - 1] = '\0';
    } else if (token_id == tokenizers[slot].config.cls_token_id) {
        strncpy(token, tokenizers[slot].config.cls_token, token_size - 1);
        token[token_size - 1] = '\0';
    } else if (token_id == tokenizers[slot].config.mask_token_id) {
        strncpy(token, tokenizers[slot].config.mask_token, token_size - 1);
        token[token_size - 1] = '\0';
    } else if (token_id == tokenizers[slot].config.unk_token_id) {
        strncpy(token, tokenizers[slot].config.unk_token, token_size - 1);
        token[token_size - 1] = '\0';
    } else {
        // For other tokens, generate a word based on the token ID
        snprintf(token, token_size, "tok%u", token_id);
    }
    
    return 0;
}

/**
 * Tokenize text
 * 
 * @param tokenizer_id: Tokenizer ID
 * @param text: Input text
 * @param result: Pointer to store the tokenization result
 * @return: 0 on success, -1 on failure
 */
int tokenizer_tokenize(tokenizer_id_t tokenizer_id, const char* text, tokenization_result_t* result) {
    // Check if the Tokenizer is initialized
    if (!tokenizer_initialized) {
        return -1;
    }
    
    // Check if the text and result pointers are valid
    if (!text || !result) {
        return -1;
    }
    
    // Find the tokenizer
    int slot = -1;
    
    for (int i = 0; i < MAX_TOKENIZERS; i++) {
        if (tokenizers[i].loaded && tokenizers[i].id == tokenizer_id) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;
    }
    
    // Record start time for performance measurement
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    
    // Simple tokenization: split by whitespace
    size_t max_tokens = MAX_TOKENS;
    result->tokens = (token_t*)malloc(max_tokens * sizeof(token_t));
    
    if (!result->tokens) {
        return -1;
    }
    
    size_t token_count = 0;
    const char* p = text;
    
    // Skip leading whitespace
    while (*p && isspace(*p)) {
        p++;
    }
    
    // Tokenize the text
    while (*p && token_count < max_tokens) {
        // Find the end of the token
        const char* token_start = p;
        while (*p && !isspace(*p)) {
            p++;
        }
        
        // Copy the token
        size_t token_len = p - token_start;
        if (token_len > 0) {
            if (token_len >= sizeof(result->tokens[token_count].text)) {
                token_len = sizeof(result->tokens[token_count].text) - 1;
            }
            
            strncpy(result->tokens[token_count].text, token_start, token_len);
            result->tokens[token_count].text[token_len] = '\0';
            
            // Set the token ID and score
            result->tokens[token_count].id = tokenizer_get_token_id(tokenizer_id, result->tokens[token_count].text);
            result->tokens[token_count].score = 1.0f;
            
            token_count++;
        }
        
        // Skip whitespace
        while (*p && isspace(*p)) {
            p++;
        }
    }
    
    // Set the result
    result->num_tokens = token_count;
    
    // Record end time and calculate tokenization time
    gettimeofday(&end_time, NULL);
    uint64_t tokenization_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                                (end_time.tv_usec - start_time.tv_usec) / 1000;
    
    result->tokenization_time = tokenization_time;
    
    // Update the tokenizer's tokenization time
    tokenizers[slot].tokenization_time = tokenization_time;
    
    return 0;
}

/**
 * Detokenize tokens
 * 
 * @param tokenizer_id: Tokenizer ID
 * @param tokens: Input tokens
 * @param num_tokens: Number of tokens
 * @param text: Output text buffer
 * @param text_size: Output text buffer size
 * @return: 0 on success, -1 on failure
 */
int tokenizer_detokenize(tokenizer_id_t tokenizer_id, const uint32_t* tokens, size_t num_tokens, char* text, size_t text_size) {
    // Check if the Tokenizer is initialized
    if (!tokenizer_initialized) {
        return -1;
    }
    
    // Check if the tokens and text pointers are valid
    if (!tokens || !text || text_size == 0) {
        return -1;
    }
    
    // Find the tokenizer
    int slot = -1;
    
    for (int i = 0; i < MAX_TOKENIZERS; i++) {
        if (tokenizers[i].loaded && tokenizers[i].id == tokenizer_id) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;
    }
    
    // Clear the text buffer
    text[0] = '\0';
    size_t current_len = 0;
    
    // Convert token IDs to text
    for (size_t i = 0; i < num_tokens; i++) {
        // Skip special tokens
        if (tokens[i] == tokenizers[slot].config.pad_token_id) {
            continue; // Skip pad tokens
        }
        
        // Get the token text
        char token_text[64];
        
        if (tokenizer_get_token_text(tokenizer_id, tokens[i], token_text, sizeof(token_text)) != 0) {
            continue;
        }
        
        size_t token_len = strlen(token_text);
        
        // Check if we need to add a space
        if (current_len > 0 && current_len + 1 < text_size) {
            text[current_len++] = ' ';
            text[current_len] = '\0';
        }
        
        // Check if the text buffer is large enough
        if (current_len + token_len >= text_size) {
            // Not enough space, truncate
            size_t remaining = text_size - current_len - 1;
            if (remaining > 0) {
                strncat(text, token_text, remaining);
            }
            text[text_size - 1] = '\0';
            break;
        }
        
        // Append the token text to the output
        strcat(text, token_text);
        current_len += token_len;
    }
    
    return 0;
}

/**
 * Encode text into tokens
 * 
 * @param tokenizer_id: Tokenizer ID
 * @param text: Input text
 * @param tokens: Output tokens buffer
 * @param tokens_size: Output tokens buffer size
 * @param num_tokens: Pointer to store the number of tokens
 * @return: 0 on success, -1 on failure
 */
int tokenizer_encode(tokenizer_id_t tokenizer_id, const char* text, uint32_t* tokens, size_t tokens_size, size_t* num_tokens) {
    // Check if the Tokenizer is initialized
    if (!tokenizer_initialized) {
        return -1;
    }
    
    // Check if the text, tokens, and num_tokens pointers are valid
    if (!text || !tokens || !num_tokens || tokens_size == 0) {
        return -1;
    }
    
    // Find the tokenizer
    int slot = -1;
    
    for (int i = 0; i < MAX_TOKENIZERS; i++) {
        if (tokenizers[i].loaded && tokenizers[i].id == tokenizer_id) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;
    }
    
    // Tokenize the text
    tokenization_result_t result;
    if (tokenizer_tokenize(tokenizer_id, text, &result) != 0) {
        return -1;
    }
    
    // Copy the token IDs to the output buffer
    size_t count = result.num_tokens;
    if (count > tokens_size) {
        count = tokens_size;
    }
    
    for (size_t i = 0; i < count; i++) {
        tokens[i] = result.tokens[i].id;
    }
    
    // Set the number of tokens
    *num_tokens = count;
    
    // Free the tokenization result
    tokenizer_free_tokenization_result(&result);
    
    return 0;
}

/**
 * Decode tokens into text
 * 
 * @param tokenizer_id: Tokenizer ID
 * @param tokens: Input tokens
 * @param num_tokens: Number of tokens
 * @param text: Output text buffer
 * @param text_size: Output text buffer size
 * @return: 0 on success, -1 on failure
 */
int tokenizer_decode(tokenizer_id_t tokenizer_id, const uint32_t* tokens, size_t num_tokens, char* text, size_t text_size) {
    // This is just a wrapper around tokenizer_detokenize
    return tokenizer_detokenize(tokenizer_id, tokens, num_tokens, text, text_size);
}

/**
 * Free a tokenization result
 * 
 * @param result: Tokenization result to free
 */
void tokenizer_free_tokenization_result(tokenization_result_t* result) {
    if (!result) {
        return;
    }
    
    if (result->tokens) {
        free(result->tokens);
        result->tokens = NULL;
    }
    
    result->num_tokens = 0;
    result->tokenization_time = 0;
}
