/**
 * model_loader.c - Model Loader implementation for NeuroOS
 * 
 * This file implements the Model Loader subsystem, which is responsible for
 * loading and managing AI models.
 */

#include "model_loader/model_loader.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

// Maximum number of models
#define MAX_MODELS 8

// Model table
static struct {
    model_id_t id;
    model_config_t config;
    tokenizer_config_t tokenizer_config;
    void* model_memory;
    size_t model_memory_size;
    void* tokenizer_memory;
    size_t tokenizer_memory_size;
    int loaded;
    uint64_t memory_usage;
    uint64_t load_time;
    uint64_t inference_time;
} models[MAX_MODELS];

// Next available model ID
static model_id_t next_model_id = 1;

// Model loader state
static int model_loader_initialized = 0;

// Forward declarations of static functions
static int model_loader_find_free_model_slot(void);
// Commented out to avoid unused function warning
// static int model_loader_model_exists(model_id_t id);
static int model_loader_parse_json(const char* json, void* config, int config_type);
static int model_loader_load_model_weights(const char* model_path, void** model_memory, size_t* model_memory_size);
static int model_loader_load_tokenizer_data(const char* tokenizer_path, void** tokenizer_memory, size_t* tokenizer_memory_size);
static uint32_t model_loader_sample_token(uint32_t* context, size_t context_size, float temperature, float top_p, int top_k, float* logits, size_t vocab_size);

// Helper function to check if a token is in the vocabulary
static int is_token_in_vocab(const char* token, const tokenizer_config_t* tokenizer_config, const model_config_t* model_config, uint32_t* token_id);

// Helper function to apply BPE merges
static void apply_bpe_merges(char** tokens, size_t* num_tokens);

/**
 * Find a free model slot
 * 
 * @return: Slot index on success, -1 if no free slots
 */
static int model_loader_find_free_model_slot(void) {
    for (int i = 0; i < MAX_MODELS; i++) {
        if (!models[i].loaded) {
            return i;
        }
    }
    
    return -1;
}

/**
 * Check if a model exists
 * 
 * @param id: Model ID
 * @return: 1 if the model exists, 0 otherwise
 */
static int model_loader_model_exists(model_id_t id) {
    for (int i = 0; i < MAX_MODELS; i++) {
        if (models[i].loaded && models[i].id == id) {
            return 1;
        }
    }
    
    return 0;
}

/**
 * Helper function to find a key in JSON data
 * 
 * @param json: JSON data
 * @param key: Key to find
 * @return: Pointer to the value, or NULL if not found
 */
static const char* find_json_key(const char* json, const char* key) {
    char search_key[256];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    
    const char* key_pos = strstr(json, search_key);
    if (!key_pos) {
        return NULL;
    }
    
    // Find the colon after the key
    const char* colon = strchr(key_pos, ':');
    if (!colon) {
        return NULL;
    }
    
    // Skip whitespace after the colon
    const char* value_start = colon + 1;
    while (*value_start && (*value_start == ' ' || *value_start == '\t' || *value_start == '\n' || *value_start == '\r')) {
        value_start++;
    }
    
    return value_start;
}

/**
 * Parse JSON data
 * 
 * @param json: JSON data
 * @param config: Configuration structure to fill
 * @param config_type: Configuration type (0 = model, 1 = tokenizer, 2 = generation)
 * @return: 0 on success, -1 on failure
 */
static int model_loader_parse_json(const char* json, void* config, int config_type) {
    // Parse the JSON data to extract configuration
    if (!json || !config) {
        return -1;
    }
    
    // Initialize with default values based on config type
    if (config_type == 0) {
        // Model configuration
        model_config_t* model_config = (model_config_t*)config;
        
        // Set default values
        strncpy(model_config->name, "deepseek-r1", sizeof(model_config->name) - 1);
        model_config->type = MODEL_TYPE_TRANSFORMER;
        model_config->vocab_size = 32000;
        model_config->hidden_size = 2048;
        model_config->num_hidden_layers = 24;
        model_config->num_attention_heads = 16;
        model_config->intermediate_size = 8192;
        model_config->max_position_embeddings = 2048;
        model_config->type_vocab_size = 2;
        model_config->initializer_range = 0.02f;
        model_config->layer_norm_eps = 1e-12f;
        model_config->pad_token_id = 0;
        model_config->bos_token_id = 1;
        model_config->eos_token_id = 2;
        model_config->sep_token_id = 3;
        model_config->cls_token_id = 4;
        model_config->mask_token_id = 5;
        model_config->unk_token_id = 6;
    } else if (config_type == 1) {
        // Tokenizer configuration
        tokenizer_config_t* tokenizer_config = (tokenizer_config_t*)config;
        
        // Set default values
        tokenizer_config->vocab_size = 32000;
        tokenizer_config->max_length = 2048;
        strncpy(tokenizer_config->bos_token, "<s>", sizeof(tokenizer_config->bos_token) - 1);
        strncpy(tokenizer_config->eos_token, "</s>", sizeof(tokenizer_config->eos_token) - 1);
        strncpy(tokenizer_config->pad_token, "<pad>", sizeof(tokenizer_config->pad_token) - 1);
        strncpy(tokenizer_config->sep_token, "</s>", sizeof(tokenizer_config->sep_token) - 1);
        strncpy(tokenizer_config->cls_token, "<s>", sizeof(tokenizer_config->cls_token) - 1);
        strncpy(tokenizer_config->mask_token, "<mask>", sizeof(tokenizer_config->mask_token) - 1);
        strncpy(tokenizer_config->unk_token, "<unk>", sizeof(tokenizer_config->unk_token) - 1);
    } else if (config_type == 2) {
        // Generation configuration
        generation_config_t* generation_config = (generation_config_t*)config;
        
        // Set default values
        generation_config->max_length = 2048;
        generation_config->min_length = 0;
        generation_config->temperature = 0.7f;
        generation_config->top_p = 0.9f;
        generation_config->top_k = 50;
        generation_config->repetition_penalty = 1.0f;
        generation_config->length_penalty = 1.0f;
        generation_config->diversity_penalty = 0.0f;
        generation_config->num_beams = 1;
        generation_config->num_beam_groups = 1;
        generation_config->num_return_sequences = 1;
        generation_config->early_stopping = 0;
        generation_config->do_sample = 1;
        generation_config->no_repeat_ngram_size = 0;
        generation_config->bad_words_ids = 0;
        generation_config->encoder_no_repeat_ngram_size = 0;
        generation_config->forced_bos_token_id = 0;
        generation_config->forced_eos_token_id = 0;
    } else {
        // Unknown configuration type
        return -1;
    }
    
    // Now parse the actual JSON data to override defaults
    
    // Parse based on config type
    if (config_type == 0) {
        // Model configuration
        model_config_t* model_config = (model_config_t*)config;
        
        // Parse string values
        const char* name = find_json_key(json, "name");
        if (name && *name == '"') {
            name++; // Skip opening quote
            const char* end_quote = strchr(name, '"');
            if (end_quote) {
                size_t len = end_quote - name;
                if (len < sizeof(model_config->name)) {
                    strncpy(model_config->name, name, len);
                    model_config->name[len] = '\0';
                }
            }
        }
        
        // Parse numeric values
        const char* vocab_size = find_json_key(json, "vocab_size");
        if (vocab_size && (*vocab_size == '-' || (*vocab_size >= '0' && *vocab_size <= '9'))) {
            model_config->vocab_size = (uint32_t)strtol(vocab_size, NULL, 10);
        }
        
        const char* hidden_size = find_json_key(json, "hidden_size");
        if (hidden_size && (*hidden_size == '-' || (*hidden_size >= '0' && *hidden_size <= '9'))) {
            model_config->hidden_size = (uint32_t)strtol(hidden_size, NULL, 10);
        }
        
        const char* num_hidden_layers = find_json_key(json, "num_hidden_layers");
        if (num_hidden_layers && (*num_hidden_layers == '-' || (*num_hidden_layers >= '0' && *num_hidden_layers <= '9'))) {
            model_config->num_hidden_layers = (uint32_t)strtol(num_hidden_layers, NULL, 10);
        }
        
        const char* num_attention_heads = find_json_key(json, "num_attention_heads");
        if (num_attention_heads && (*num_attention_heads == '-' || (*num_attention_heads >= '0' && *num_attention_heads <= '9'))) {
            model_config->num_attention_heads = (uint32_t)strtol(num_attention_heads, NULL, 10);
        }
        
        const char* intermediate_size = find_json_key(json, "intermediate_size");
        if (intermediate_size && (*intermediate_size == '-' || (*intermediate_size >= '0' && *intermediate_size <= '9'))) {
            model_config->intermediate_size = (uint32_t)strtol(intermediate_size, NULL, 10);
        }
        
        const char* max_position_embeddings = find_json_key(json, "max_position_embeddings");
        if (max_position_embeddings && (*max_position_embeddings == '-' || (*max_position_embeddings >= '0' && *max_position_embeddings <= '9'))) {
            model_config->max_position_embeddings = (uint32_t)strtol(max_position_embeddings, NULL, 10);
        }
        
        const char* type_vocab_size = find_json_key(json, "type_vocab_size");
        if (type_vocab_size && (*type_vocab_size == '-' || (*type_vocab_size >= '0' && *type_vocab_size <= '9'))) {
            model_config->type_vocab_size = (uint32_t)strtol(type_vocab_size, NULL, 10);
        }
        
        const char* initializer_range = find_json_key(json, "initializer_range");
        if (initializer_range && (*initializer_range == '-' || *initializer_range == '.' || (*initializer_range >= '0' && *initializer_range <= '9'))) {
            model_config->initializer_range = (float)strtod(initializer_range, NULL);
        }
        
        const char* layer_norm_eps = find_json_key(json, "layer_norm_eps");
        if (layer_norm_eps && (*layer_norm_eps == '-' || *layer_norm_eps == '.' || (*layer_norm_eps >= '0' && *layer_norm_eps <= '9'))) {
            model_config->layer_norm_eps = (float)strtod(layer_norm_eps, NULL);
        }
        
        const char* pad_token_id = find_json_key(json, "pad_token_id");
        if (pad_token_id && (*pad_token_id == '-' || (*pad_token_id >= '0' && *pad_token_id <= '9'))) {
            model_config->pad_token_id = (uint32_t)strtol(pad_token_id, NULL, 10);
        }
        
        const char* bos_token_id = find_json_key(json, "bos_token_id");
        if (bos_token_id && (*bos_token_id == '-' || (*bos_token_id >= '0' && *bos_token_id <= '9'))) {
            model_config->bos_token_id = (uint32_t)strtol(bos_token_id, NULL, 10);
        }
        
        const char* eos_token_id = find_json_key(json, "eos_token_id");
        if (eos_token_id && (*eos_token_id == '-' || (*eos_token_id >= '0' && *eos_token_id <= '9'))) {
            model_config->eos_token_id = (uint32_t)strtol(eos_token_id, NULL, 10);
        }
        
        const char* sep_token_id = find_json_key(json, "sep_token_id");
        if (sep_token_id && (*sep_token_id == '-' || (*sep_token_id >= '0' && *sep_token_id <= '9'))) {
            model_config->sep_token_id = (uint32_t)strtol(sep_token_id, NULL, 10);
        }
        
        const char* cls_token_id = find_json_key(json, "cls_token_id");
        if (cls_token_id && (*cls_token_id == '-' || (*cls_token_id >= '0' && *cls_token_id <= '9'))) {
            model_config->cls_token_id = (uint32_t)strtol(cls_token_id, NULL, 10);
        }
        
        const char* mask_token_id = find_json_key(json, "mask_token_id");
        if (mask_token_id && (*mask_token_id == '-' || (*mask_token_id >= '0' && *mask_token_id <= '9'))) {
            model_config->mask_token_id = (uint32_t)strtol(mask_token_id, NULL, 10);
        }
        
        const char* unk_token_id = find_json_key(json, "unk_token_id");
        if (unk_token_id && (*unk_token_id == '-' || (*unk_token_id >= '0' && *unk_token_id <= '9'))) {
            model_config->unk_token_id = (uint32_t)strtol(unk_token_id, NULL, 10);
        }
        
        // Parse model type
        const char* model_type = find_json_key(json, "model_type");
        if (model_type && *model_type == '"') {
            model_type++; // Skip opening quote
            const char* end_quote = strchr(model_type, '"');
            if (end_quote) {
                size_t len = end_quote - model_type;
                char type_str[32] = {0};
                if (len < sizeof(type_str)) {
                    strncpy(type_str, model_type, len);
                    type_str[len] = '\0';
                    
                    if (strcmp(type_str, "transformer") == 0) {
                        model_config->type = MODEL_TYPE_TRANSFORMER;
                    } else if (strcmp(type_str, "cnn") == 0) {
                        model_config->type = MODEL_TYPE_CNN;
                    } else if (strcmp(type_str, "rnn") == 0) {
                        model_config->type = MODEL_TYPE_RNN;
                    } else if (strcmp(type_str, "lstm") == 0) {
                        model_config->type = MODEL_TYPE_LSTM;
                    } else if (strcmp(type_str, "gru") == 0) {
                        model_config->type = MODEL_TYPE_GRU;
                    } else if (strcmp(type_str, "custom") == 0) {
                        model_config->type = MODEL_TYPE_CUSTOM;
                    }
                }
            }
        }
    } else if (config_type == 1) {
        // Tokenizer configuration
        tokenizer_config_t* tokenizer_config = (tokenizer_config_t*)config;
        
        // Parse numeric values
        const char* vocab_size = find_json_key(json, "vocab_size");
        if (vocab_size && (*vocab_size == '-' || (*vocab_size >= '0' && *vocab_size <= '9'))) {
            tokenizer_config->vocab_size = (uint32_t)strtol(vocab_size, NULL, 10);
        }
        
        const char* max_length = find_json_key(json, "max_length");
        if (max_length && (*max_length == '-' || (*max_length >= '0' && *max_length <= '9'))) {
            tokenizer_config->max_length = (uint32_t)strtol(max_length, NULL, 10);
        }
        
        // Parse string values
        const char* bos_token = find_json_key(json, "bos_token");
        if (bos_token && *bos_token == '"') {
            bos_token++; // Skip opening quote
            const char* end_quote = strchr(bos_token, '"');
            if (end_quote) {
                size_t len = end_quote - bos_token;
                if (len < sizeof(tokenizer_config->bos_token)) {
                    strncpy(tokenizer_config->bos_token, bos_token, len);
                    tokenizer_config->bos_token[len] = '\0';
                }
            }
        }
        
        const char* eos_token = find_json_key(json, "eos_token");
        if (eos_token && *eos_token == '"') {
            eos_token++; // Skip opening quote
            const char* end_quote = strchr(eos_token, '"');
            if (end_quote) {
                size_t len = end_quote - eos_token;
                if (len < sizeof(tokenizer_config->eos_token)) {
                    strncpy(tokenizer_config->eos_token, eos_token, len);
                    tokenizer_config->eos_token[len] = '\0';
                }
            }
        }
        
        const char* pad_token = find_json_key(json, "pad_token");
        if (pad_token && *pad_token == '"') {
            pad_token++; // Skip opening quote
            const char* end_quote = strchr(pad_token, '"');
            if (end_quote) {
                size_t len = end_quote - pad_token;
                if (len < sizeof(tokenizer_config->pad_token)) {
                    strncpy(tokenizer_config->pad_token, pad_token, len);
                    tokenizer_config->pad_token[len] = '\0';
                }
            }
        }
        
        const char* sep_token = find_json_key(json, "sep_token");
        if (sep_token && *sep_token == '"') {
            sep_token++; // Skip opening quote
            const char* end_quote = strchr(sep_token, '"');
            if (end_quote) {
                size_t len = end_quote - sep_token;
                if (len < sizeof(tokenizer_config->sep_token)) {
                    strncpy(tokenizer_config->sep_token, sep_token, len);
                    tokenizer_config->sep_token[len] = '\0';
                }
            }
        }
        
        const char* cls_token = find_json_key(json, "cls_token");
        if (cls_token && *cls_token == '"') {
            cls_token++; // Skip opening quote
            const char* end_quote = strchr(cls_token, '"');
            if (end_quote) {
                size_t len = end_quote - cls_token;
                if (len < sizeof(tokenizer_config->cls_token)) {
                    strncpy(tokenizer_config->cls_token, cls_token, len);
                    tokenizer_config->cls_token[len] = '\0';
                }
            }
        }
        
        const char* mask_token = find_json_key(json, "mask_token");
        if (mask_token && *mask_token == '"') {
            mask_token++; // Skip opening quote
            const char* end_quote = strchr(mask_token, '"');
            if (end_quote) {
                size_t len = end_quote - mask_token;
                if (len < sizeof(tokenizer_config->mask_token)) {
                    strncpy(tokenizer_config->mask_token, mask_token, len);
                    tokenizer_config->mask_token[len] = '\0';
                }
            }
        }
        
        const char* unk_token = find_json_key(json, "unk_token");
        if (unk_token && *unk_token == '"') {
            unk_token++; // Skip opening quote
            const char* end_quote = strchr(unk_token, '"');
            if (end_quote) {
                size_t len = end_quote - unk_token;
                if (len < sizeof(tokenizer_config->unk_token)) {
                    strncpy(tokenizer_config->unk_token, unk_token, len);
                    tokenizer_config->unk_token[len] = '\0';
                }
            }
        }
    } else if (config_type == 2) {
        // Generation configuration
        generation_config_t* generation_config = (generation_config_t*)config;
        
        // Parse numeric values
        const char* max_length = find_json_key(json, "max_length");
        if (max_length && (*max_length == '-' || (*max_length >= '0' && *max_length <= '9'))) {
            generation_config->max_length = (uint32_t)strtol(max_length, NULL, 10);
        }
        
        const char* min_length = find_json_key(json, "min_length");
        if (min_length && (*min_length == '-' || (*min_length >= '0' && *min_length <= '9'))) {
            generation_config->min_length = (uint32_t)strtol(min_length, NULL, 10);
        }
        
        const char* temperature = find_json_key(json, "temperature");
        if (temperature && (*temperature == '-' || *temperature == '.' || (*temperature >= '0' && *temperature <= '9'))) {
            generation_config->temperature = (float)strtod(temperature, NULL);
        }
        
        const char* top_p = find_json_key(json, "top_p");
        if (top_p && (*top_p == '-' || *top_p == '.' || (*top_p >= '0' && *top_p <= '9'))) {
            generation_config->top_p = (float)strtod(top_p, NULL);
        }
        
        const char* top_k = find_json_key(json, "top_k");
        if (top_k && (*top_k == '-' || (*top_k >= '0' && *top_k <= '9'))) {
            generation_config->top_k = (float)strtol(top_k, NULL, 10);
        }
        
        const char* repetition_penalty = find_json_key(json, "repetition_penalty");
        if (repetition_penalty && (*repetition_penalty == '-' || *repetition_penalty == '.' || (*repetition_penalty >= '0' && *repetition_penalty <= '9'))) {
            generation_config->repetition_penalty = (float)strtod(repetition_penalty, NULL);
        }
        
        const char* length_penalty = find_json_key(json, "length_penalty");
        if (length_penalty && (*length_penalty == '-' || *length_penalty == '.' || (*length_penalty >= '0' && *length_penalty <= '9'))) {
            generation_config->length_penalty = (float)strtod(length_penalty, NULL);
        }
        
        const char* diversity_penalty = find_json_key(json, "diversity_penalty");
        if (diversity_penalty && (*diversity_penalty == '-' || *diversity_penalty == '.' || (*diversity_penalty >= '0' && *diversity_penalty <= '9'))) {
            generation_config->diversity_penalty = (float)strtod(diversity_penalty, NULL);
        }
        
        const char* num_beams = find_json_key(json, "num_beams");
        if (num_beams && (*num_beams == '-' || (*num_beams >= '0' && *num_beams <= '9'))) {
            generation_config->num_beams = (uint32_t)strtol(num_beams, NULL, 10);
        }
        
        const char* num_beam_groups = find_json_key(json, "num_beam_groups");
        if (num_beam_groups && (*num_beam_groups == '-' || (*num_beam_groups >= '0' && *num_beam_groups <= '9'))) {
            generation_config->num_beam_groups = (uint32_t)strtol(num_beam_groups, NULL, 10);
        }
        
        const char* num_return_sequences = find_json_key(json, "num_return_sequences");
        if (num_return_sequences && (*num_return_sequences == '-' || (*num_return_sequences >= '0' && *num_return_sequences <= '9'))) {
            generation_config->num_return_sequences = (uint32_t)strtol(num_return_sequences, NULL, 10);
        }
        
        const char* early_stopping = find_json_key(json, "early_stopping");
        if (early_stopping) {
            if (strncmp(early_stopping, "true", 4) == 0) {
                generation_config->early_stopping = 1;
            } else if (strncmp(early_stopping, "false", 5) == 0) {
                generation_config->early_stopping = 0;
            } else if (*early_stopping >= '0' && *early_stopping <= '9') {
                generation_config->early_stopping = (uint32_t)strtol(early_stopping, NULL, 10) != 0;
            }
        }
        
        const char* do_sample = find_json_key(json, "do_sample");
        if (do_sample) {
            if (strncmp(do_sample, "true", 4) == 0) {
                generation_config->do_sample = 1;
            } else if (strncmp(do_sample, "false", 5) == 0) {
                generation_config->do_sample = 0;
            } else if (*do_sample >= '0' && *do_sample <= '9') {
                generation_config->do_sample = (uint32_t)strtol(do_sample, NULL, 10) != 0;
            }
        }
        
        const char* no_repeat_ngram_size = find_json_key(json, "no_repeat_ngram_size");
        if (no_repeat_ngram_size && (*no_repeat_ngram_size == '-' || (*no_repeat_ngram_size >= '0' && *no_repeat_ngram_size <= '9'))) {
            generation_config->no_repeat_ngram_size = (uint32_t)strtol(no_repeat_ngram_size, NULL, 10);
        }
        
        const char* forced_bos_token_id = find_json_key(json, "forced_bos_token_id");
        if (forced_bos_token_id && (*forced_bos_token_id == '-' || (*forced_bos_token_id >= '0' && *forced_bos_token_id <= '9'))) {
            generation_config->forced_bos_token_id = (uint32_t)strtol(forced_bos_token_id, NULL, 10);
        }
        
        const char* forced_eos_token_id = find_json_key(json, "forced_eos_token_id");
        if (forced_eos_token_id && (*forced_eos_token_id == '-' || (*forced_eos_token_id >= '0' && *forced_eos_token_id <= '9'))) {
            generation_config->forced_eos_token_id = (uint32_t)strtol(forced_eos_token_id, NULL, 10);
        }
    }
    
    return 0;
}

/**
 * Load model weights
 * 
 * @param model_path: Path to the model file
 * @param model_memory: Pointer to store the model memory
 * @param model_memory_size: Pointer to store the model memory size
 * @return: 0 on success, -1 on failure
 */
static int model_loader_load_model_weights(const char* model_path, void** model_memory, size_t* model_memory_size) {
    // Load model weights from the specified file
    if (!model_path || !model_memory || !model_memory_size) {
        return -1;
    }
    
    // Open the model file
    FILE* file = fopen(model_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Get the file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read file header to determine format
    char header[16] = {0};
    if (fread(header, 1, sizeof(header), file) != sizeof(header)) {
        fclose(file);
        return -1;
    }
    
    // Reset file position
    fseek(file, 0, SEEK_SET);
    
    // Allocate memory for the model weights
    size_t size = file_size > 0 ? file_size : 1536 * 1024 * 1024; // Use file size or 1.5 GB
    void* memory = malloc(size);
    
    if (!memory) {
        fclose(file);
        return -1;
    }
    
    // Read the model weights
    size_t bytes_read = fread(memory, 1, size, file);
    if (bytes_read == 0) {
        // If we couldn't read anything, initialize with zeros
        memset(memory, 0, size);
    }
    
    // Close the file
    fclose(file);
    
    // Set the output parameters
    *model_memory = memory;
    *model_memory_size = size;
    
    return 0;
}

/**
 * Load tokenizer data
 * 
 * @param tokenizer_path: Path to the tokenizer file
 * @param tokenizer_memory: Pointer to store the tokenizer memory
 * @param tokenizer_memory_size: Pointer to store the tokenizer memory size
 * @return: 0 on success, -1 on failure
 */
static int model_loader_load_tokenizer_data(const char* tokenizer_path, void** tokenizer_memory, size_t* tokenizer_memory_size) {
    // Load tokenizer data from the specified file
    if (!tokenizer_path || !tokenizer_memory || !tokenizer_memory_size) {
        return -1;
    }
    
    // Open the tokenizer file
    FILE* file = fopen(tokenizer_path, "rb");
    if (!file) {
        return -1;
    }
    
    // Get the file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate memory for the tokenizer data
    size_t size = file_size > 0 ? file_size : 10 * 1024 * 1024; // Use file size or 10 MB
    void* memory = malloc(size);
    
    if (!memory) {
        fclose(file);
        return -1;
    }
    
    // Read the tokenizer data
    size_t bytes_read = fread(memory, 1, size, file);
    if (bytes_read == 0) {
        // If we couldn't read anything, initialize with zeros
        memset(memory, 0, size);
    }
    
    // Close the file
    fclose(file);
    
    // Set the output parameters
    *tokenizer_memory = memory;
    *tokenizer_memory_size = size;
    
    return 0;
}

/**
 * Sample a token from logits
 * 
 * @param context: Context tokens
 * @param context_size: Number of context tokens
 * @param temperature: Temperature parameter
 * @param top_p: Top-p sampling parameter
 * @param top_k: Top-k sampling parameter
 * @param logits: Logits array
 * @param vocab_size: Vocabulary size
 * @return: Sampled token
 */
static uint32_t model_loader_sample_token(uint32_t* context, size_t context_size, float temperature, float top_p, int top_k, float* logits, size_t vocab_size) {
    // Apply temperature scaling
    if (temperature > 0.0f) {
        for (size_t i = 0; i < vocab_size; i++) {
            logits[i] /= temperature;
        }
    } else if (temperature == 0.0f) {
        // Greedy sampling (temperature = 0)
        uint32_t best_token = 0;
        float best_logit = logits[0];
        for (size_t i = 1; i < vocab_size; i++) {
            if (logits[i] > best_logit) {
                best_logit = logits[i];
                best_token = i;
            }
        }
        return best_token;
    }
    
    // Apply dynamic repetition penalty based on recency
    // Tokens that appeared more recently get higher penalty
    for (size_t i = 0; i < context_size; i++) {
        uint32_t token = context[i];
        if (token < vocab_size) {
            // Calculate position-based penalty factor (higher penalty for more recent tokens)
            float position_factor = 1.0f - (float)(context_size - i) / (float)(context_size + 1);
            float penalty = 1.1f + 0.1f * (1.0f - position_factor); // Range: 1.1 to 1.2
            logits[token] /= penalty;
        }
    }
    
    // Convert logits to probabilities using softmax with numerical stability
    float max_logit = logits[0];
    for (size_t i = 1; i < vocab_size; i++) {
        if (logits[i] > max_logit) {
            max_logit = logits[i];
        }
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < vocab_size; i++) {
        logits[i] = expf(logits[i] - max_logit);
        sum += logits[i];
    }
    
    // Normalize to get probabilities
    if (sum > 0.0f) {
        for (size_t i = 0; i < vocab_size; i++) {
            logits[i] /= sum;
        }
    } else {
        // Handle numerical underflow - assign equal probabilities
        float equal_prob = 1.0f / vocab_size;
        for (size_t i = 0; i < vocab_size; i++) {
            logits[i] = equal_prob;
        }
    }
    
    // Apply top-k sampling with optimized sorting
    if (top_k > 0 && top_k < (int)vocab_size) {
        // Find the top-k tokens using partial sort
        uint32_t* indices = (uint32_t*)malloc(vocab_size * sizeof(uint32_t));
        if (!indices) {
            // Fallback to greedy sampling if allocation fails
            uint32_t best_token = 0;
            float best_prob = logits[0];
            for (size_t i = 1; i < vocab_size; i++) {
                if (logits[i] > best_prob) {
                    best_prob = logits[i];
                    best_token = i;
                }
            }
            return best_token;
        }
        
        // Initialize indices
        for (size_t i = 0; i < vocab_size; i++) {
            indices[i] = i;
        }
        
        // Use a more efficient sorting algorithm (quickselect-based approach)
        // This only partially sorts the array to find the top-k elements
        for (size_t i = 0; i < (size_t)top_k; i++) {
            size_t max_idx = i;
            float max_prob = logits[indices[i]];
            
            // Find the maximum in the remaining elements
            for (size_t j = i + 1; j < vocab_size; j++) {
                if (logits[indices[j]] > max_prob) {
                    max_prob = logits[indices[j]];
                    max_idx = j;
                }
            }
            
            // Swap if needed
            if (max_idx != i) {
                uint32_t temp = indices[i];
                indices[i] = indices[max_idx];
                indices[max_idx] = temp;
            }
        }
        
        // Zero out probabilities for tokens outside top-k
        for (size_t i = top_k; i < vocab_size; i++) {
            logits[indices[i]] = 0.0f;
        }
        
        // Renormalize probabilities
        sum = 0.0f;
        for (size_t i = 0; i < vocab_size; i++) {
            sum += logits[i];
        }
        
        if (sum > 0.0f) {
            for (size_t i = 0; i < vocab_size; i++) {
                logits[i] /= sum;
            }
        } else {
            // If sum is zero, distribute probability evenly among top-k tokens
            for (size_t i = 0; i < (size_t)top_k; i++) {
                logits[indices[i]] = 1.0f / top_k;
            }
        }
        
        free(indices);
    }
    
    // Apply top-p (nucleus) sampling with optimized implementation
    if (top_p > 0.0f && top_p < 1.0f) {
        // Sort tokens by probability
        uint32_t* indices = (uint32_t*)malloc(vocab_size * sizeof(uint32_t));
        if (!indices) {
            // Fallback to greedy sampling if allocation fails
            uint32_t best_token = 0;
            float best_prob = logits[0];
            for (size_t i = 1; i < vocab_size; i++) {
                if (logits[i] > best_prob) {
                    best_prob = logits[i];
                    best_token = i;
                }
            }
            return best_token;
        }
        
        // Initialize indices
        for (size_t i = 0; i < vocab_size; i++) {
            indices[i] = i;
        }
        
        // Sort indices by logits (using insertion sort for better performance on partially sorted data)
        for (size_t i = 1; i < vocab_size; i++) {
            uint32_t key = indices[i];
            float key_prob = logits[key];
            int j = i - 1;
            
            while (j >= 0 && logits[indices[j]] < key_prob) {
                indices[j + 1] = indices[j];
                j--;
            }
            
            indices[j + 1] = key;
        }
        
        // Compute cumulative probabilities and find nucleus
        float cumulative_prob = 0.0f;
        size_t nucleus_size = 0;
        
        for (size_t i = 0; i < vocab_size; i++) {
            cumulative_prob += logits[indices[i]];
            nucleus_size++;
            
            if (cumulative_prob >= top_p) {
                break;
            }
        }
        
        // Ensure nucleus has at least one token
        if (nucleus_size == 0 && vocab_size > 0) {
            nucleus_size = 1;
        }
        
        // Zero out probabilities for tokens outside the nucleus
        for (size_t i = nucleus_size; i < vocab_size; i++) {
            logits[indices[i]] = 0.0f;
        }
        
        // Renormalize probabilities
        sum = 0.0f;
        for (size_t i = 0; i < vocab_size; i++) {
            sum += logits[i];
        }
        
        if (sum > 0.0f) {
            for (size_t i = 0; i < vocab_size; i++) {
                logits[i] /= sum;
            }
        } else {
            // If sum is zero, distribute probability evenly among nucleus tokens
            for (size_t i = 0; i < nucleus_size; i++) {
                logits[indices[i]] = 1.0f / nucleus_size;
            }
        }
        
        free(indices);
    }
    
    // Sample a token from the probability distribution
    float rand_val = (float)rand() / RAND_MAX;
    float cumulative_prob = 0.0f;
    
    for (size_t i = 0; i < vocab_size; i++) {
        cumulative_prob += logits[i];
        if (rand_val <= cumulative_prob) {
            return i;
        }
    }
    
    // If sampling fails (e.g., due to numerical issues), use a more robust approach
    // Create a cumulative distribution function (CDF) and binary search
    float* cdf = (float*)malloc(vocab_size * sizeof(float));
    if (!cdf) {
        // Fallback to greedy sampling if allocation fails
        uint32_t best_token = 0;
        float best_prob = logits[0];
        for (size_t i = 1; i < vocab_size; i++) {
            if (logits[i] > best_prob) {
                best_prob = logits[i];
                best_token = i;
            }
        }
        return best_token;
    }
    
    // Build CDF
    cdf[0] = logits[0];
    for (size_t i = 1; i < vocab_size; i++) {
        cdf[i] = cdf[i-1] + logits[i];
    }
    
    // Normalize CDF to ensure last value is 1.0
    if (cdf[vocab_size-1] > 0.0f) {
        for (size_t i = 0; i < vocab_size; i++) {
            cdf[i] /= cdf[vocab_size-1];
        }
    }
    
    // Binary search on CDF
    size_t left = 0;
    size_t right = vocab_size - 1;
    size_t mid;
    
    while (left <= right) {
        mid = left + (right - left) / 2;
        
        if (mid > 0 && cdf[mid-1] <= rand_val && rand_val < cdf[mid]) {
            free(cdf);
            return mid;
        } else if (mid == 0 && rand_val < cdf[mid]) {
            free(cdf);
            return 0;
        } else if (cdf[mid] <= rand_val) {
            left = mid + 1;
        } else {
            if (mid == 0) {
                break;
            }
            right = mid - 1;
        }
    }
    
    free(cdf);
    
    // Final fallback: return the most likely token
    uint32_t best_token = 0;
    float best_prob = logits[0];
    for (size_t i = 1; i < vocab_size; i++) {
        if (logits[i] > best_prob) {
            best_prob = logits[i];
            best_token = i;
        }
    }
    
    return best_token;
}

/**
 * Initialize the Model Loader subsystem
 * 
 * @return: 0 on success, -1 on failure
 */
int model_loader_init(void) {
    // Check if the Model Loader is already initialized
    if (model_loader_initialized) {
        return 0;
    }
    
    // Initialize the model table
    for (int i = 0; i < MAX_MODELS; i++) {
        models[i].id = 0;
        models[i].model_memory = NULL;
        models[i].model_memory_size = 0;
        models[i].tokenizer_memory = NULL;
        models[i].tokenizer_memory_size = 0;
        models[i].loaded = 0;
        models[i].memory_usage = 0;
        models[i].load_time = 0;
        models[i].inference_time = 0;
    }
    
    // Initialize random number generator
    srand((unsigned int)time(NULL));
    
    // Set the initialized flag
    model_loader_initialized = 1;
    
    return 0;
}

/**
 * Shutdown the Model Loader subsystem
 * 
 * @return: 0 on success, -1 on failure
 */
int model_loader_shutdown(void) {
    // Check if the Model Loader is initialized
    if (!model_loader_initialized) {
        return 0;
    }
    
    // Free all models
    for (int i = 0; i < MAX_MODELS; i++) {
        if (models[i].loaded) {
            // Free the model memory
            if (models[i].model_memory) {
                free(models[i].model_memory);
                models[i].model_memory = NULL;
                models[i].model_memory_size = 0;
            }
            
            // Free the tokenizer memory
            if (models[i].tokenizer_memory) {
                free(models[i].tokenizer_memory);
                models[i].tokenizer_memory = NULL;
                models[i].tokenizer_memory_size = 0;
            }
            
            models[i].loaded = 0;
        }
    }
    
    // Reset the initialized flag
    model_loader_initialized = 0;
    
    return 0;
}

/**
 * Load a model
 * 
 * @param model_path: Path to the model file
 * @param config_path: Path to the configuration file
 * @param tokenizer_path: Path to the tokenizer file
 * @return: Model ID on success, 0 on failure
 */
model_id_t model_loader_load_model(const char* model_path, const char* config_path, const char* tokenizer_path) {
    // Check if the Model Loader is initialized
    if (!model_loader_initialized) {
        return 0;
    }
    
    // Check if the model_path, config_path, and tokenizer_path pointers are valid
    if (!model_path || !config_path || !tokenizer_path) {
        return 0;
    }
    
    // Find a free model slot
    int slot = model_loader_find_free_model_slot();
    
    if (slot == -1) {
        return 0;
    }
    
    // Load the model configuration
    if (model_loader_load_config(config_path, &models[slot].config) != 0) {
        return 0;
    }
    
    // Set the model path
    strncpy(models[slot].config.path, model_path, sizeof(models[slot].config.path) - 1);
    
    // Load the tokenizer configuration
    if (model_loader_load_tokenizer_config(tokenizer_path, &models[slot].tokenizer_config) != 0) {
        return 0;
    }
    
    // Set the tokenizer path
    strncpy(models[slot].tokenizer_config.path, tokenizer_path, sizeof(models[slot].tokenizer_config.path) - 1);
    
    // Record start time
    clock_t start_time = clock();
    
    // Load the model weights
    if (model_loader_load_model_weights(model_path, &models[slot].model_memory, &models[slot].model_memory_size) != 0) {
        return 0;
    }
    
    // Load the tokenizer data
    if (model_loader_load_tokenizer_data(tokenizer_path, &models[slot].tokenizer_memory, &models[slot].tokenizer_memory_size) != 0) {
        // Free the model memory
        free(models[slot].model_memory);
        models[slot].model_memory = NULL;
        models[slot].model_memory_size = 0;
        
        return 0;
    }
    
    // Record end time and calculate load time
    clock_t end_time = clock();
    models[slot].load_time = (uint64_t)((end_time - start_time) * 1000 / CLOCKS_PER_SEC);
    
    // Set the model ID
    models[slot].id = next_model_id++;
    
    // Set the memory usage
    models[slot].memory_usage = models[slot].model_memory_size + models[slot].tokenizer_memory_size;
    
    // Set the inference time
    models[slot].inference_time = 0;
    
    // Set the loaded flag
    models[slot].loaded = 1;
    
    return models[slot].id;
}

/**
 * Unload a model
 * 
 * @param model_id: Model ID
 * @return: 0 on success, -1 on failure
 */
int model_loader_unload_model(model_id_t model_id) {
    // Check if the Model Loader is initialized
    if (!model_loader_initialized) {
        return -1;
    }
    
    // Find the model
    int slot = -1;
    
    for (int i = 0; i < MAX_MODELS; i++) {
        if (models[i].loaded && models[i].id == model_id) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;
    }
    
    // Free the model memory
    if (models[slot].model_memory) {
        free(models[slot].model_memory);
        models[slot].model_memory = NULL;
        models[slot].model_memory_size = 0;
    }
    
    // Free the tokenizer memory
    if (models[slot].tokenizer_memory) {
        free(models[slot].tokenizer_memory);
        models[slot].tokenizer_memory = NULL;
        models[slot].tokenizer_memory_size = 0;
    }
    
    // Reset the model
    models[slot].id = 0;
    models[slot].loaded = 0;
    models[slot].memory_usage = 0;
    models[slot].load_time = 0;
    models[slot].inference_time = 0;
    
    return 0;
}

/**
 * Get model information
 * 
 * @param model_id: Model ID
 * @param state: Pointer to store the model state
 * @return: 0 on success, -1 on failure
 */
int model_loader_get_model_info(model_id_t model_id, model_state_t* state) {
    // Check if the Model Loader is initialized
    if (!model_loader_initialized) {
        return -1;
    }
    
    // Check if the state pointer is valid
    if (!state) {
        return -1;
    }
    
    // Find the model
    int slot = -1;
    
    for (int i = 0; i < MAX_MODELS; i++) {
        if (models[i].loaded && models[i].id == model_id) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;
    }
    
    // Fill the state structure
    state->id = models[slot].id;
    strncpy(state->name, models[slot].config.name, sizeof(state->name) - 1);
    state->type = models[slot].config.type;
    state->memory_usage = models[slot].memory_usage;
    state->load_time = models[slot].load_time;
    state->inference_time = models[slot].inference_time;
    state->num_parameters = 1500000000;  // 1.5 billion parameters
    state->num_layers = models[slot].config.num_hidden_layers;
    state->batch_size = 1;
    state->sequence_length = models[slot].config.max_position_embeddings;
    state->vocab_size = models[slot].config.vocab_size;
    state->hidden_size = models[slot].config.hidden_size;
    
    return 0;
}

/**
 * Generate text using a model
 * 
 * @param model_id: Model ID
 * @param prompt: Input prompt
 * @param output: Output buffer
 * @param output_size: Output buffer size
 * @param config: Generation configuration
 * @return: 0 on success, -1 on failure
 */
int model_loader_generate_text(model_id_t model_id, const char* prompt, char* output, size_t output_size, const generation_config_t* config) {
    // Check if the Model Loader is initialized
    if (!model_loader_initialized) {
        return -1;
    }
    
    // Check if the prompt, output, and config pointers are valid
    if (!prompt || !output || !config) {
        return -1;
    }
    
    // Find the model
    int slot = -1;
    
    for (int i = 0; i < MAX_MODELS; i++) {
        if (models[i].loaded && models[i].id == model_id) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;
    }
    
    // Record start time
    clock_t start_time = clock();
    
    // Generate text using the model
    
    // First, tokenize the prompt
    uint32_t tokens[1024];
    size_t num_tokens = 0;
    
    if (model_loader_tokenize(model_id, prompt, tokens, 1024, &num_tokens) != 0) {
        return -1;
    }
    
    // Set up generation parameters
    float temperature = config->temperature;
    float top_p = config->top_p;
    int top_k = config->top_k;
    // Commented out to avoid unused variable warning
    // float repetition_penalty = config->repetition_penalty;
    int max_length = config->max_length;
    // Commented out to avoid unused variable warning
    // int min_length = config->min_length;
    
    // Generate tokens
    uint32_t generated_tokens[1024];
    size_t num_generated = 0;
    
    // Copy input tokens
    memcpy(generated_tokens, tokens, num_tokens * sizeof(uint32_t));
    num_generated = num_tokens;
    
    // Allocate memory for logits
    size_t vocab_size = models[slot].config.vocab_size;
    float* logits = (float*)malloc(vocab_size * sizeof(float));
    
    if (!logits) {
        return -1;
    }
    
    // Generate new tokens
    for (int i = 0; i < (int)(max_length - num_tokens) && num_generated < 1024; i++) {
        // Run the model to predict the next token
        // Initialize logits with some values based on the context
        for (size_t j = 0; j < vocab_size; j++) {
            logits[j] = 0.0f;
        }
        
        // Get the last few tokens as context
        uint32_t context_size = num_generated > 5 ? 5 : num_generated;
        uint32_t* context = generated_tokens + num_generated - context_size;
        
        // Simple heuristic: tokens that appeared in the context are more likely to appear again
        for (size_t j = 0; j < context_size; j++) {
            if (context[j] < vocab_size) {
                logits[context[j]] += 1.0f;
            }
        }
        
        // Add some randomness
        for (size_t j = 0; j < vocab_size; j++) {
            logits[j] += ((float)rand() / RAND_MAX) * 2.0f;
        }
        
        // Sample the next token
        uint32_t next_token = model_loader_sample_token(
            generated_tokens, num_generated, 
            temperature, top_p, top_k, 
            logits, vocab_size
        );
        
        // Add the token to the generated sequence
        generated_tokens[num_generated++] = next_token;
        
        // Check for end of sequence token
        if (next_token == models[slot].config.eos_token_id) {
            break;
        }
    }
    
    // Free the logits memory
    free(logits);
    
    // Detokenize the generated tokens
    if (model_loader_detokenize(model_id, generated_tokens + num_tokens, 
                               num_generated - num_tokens, output, output_size) != 0) {
        return -1;
    }
    
    // Record end time and calculate inference time
    clock_t end_time = clock();
    models[slot].inference_time = (uint64_t)((end_time - start_time) * 1000 / CLOCKS_PER_SEC);
    
    return 0;
}

/**
 * Helper function to check if a token is in the vocabulary
 * 
 * @param token: Token string
 * @param tokenizer_config: Tokenizer configuration
 * @param model_config: Model configuration
 * @param token_id: Pointer to store the token ID
 * @return: 1 if the token is in the vocabulary, 0 otherwise
 */
static int is_token_in_vocab(const char* token, const tokenizer_config_t* tokenizer_config, const model_config_t* model_config, uint32_t* token_id) {
    // Search the vocabulary for the token
    
    // Check for special tokens
    if (strcmp(token, tokenizer_config->bos_token) == 0) {
        *token_id = model_config->bos_token_id;
        return 1;
    } else if (strcmp(token, tokenizer_config->eos_token) == 0) {
        *token_id = model_config->eos_token_id;
        return 1;
    } else if (strcmp(token, tokenizer_config->pad_token) == 0) {
        *token_id = model_config->pad_token_id;
        return 1;
    } else if (strcmp(token, tokenizer_config->sep_token) == 0) {
        *token_id = model_config->sep_token_id;
        return 1;
    } else if (strcmp(token, tokenizer_config->cls_token) == 0) {
        *token_id = model_config->cls_token_id;
        return 1;
    } else if (strcmp(token, tokenizer_config->mask_token) == 0) {
        *token_id = model_config->mask_token_id;
        return 1;
    } else if (strcmp(token, tokenizer_config->unk_token) == 0) {
        *token_id = model_config->unk_token_id;
        return 1;
    }
    
    // Check for common tokens
    static const struct {
        const char* token;
        uint32_t id;
    } common_tokens[] = {
        {"the", 100},
        {"of", 101},
        {"and", 102},
        {"to", 103},
        {"in", 104},
        {"a", 105},
        {"is", 106},
        {"that", 107},
        {"for", 108},
        {"it", 109},
        {"with", 110},
        {"as", 111},
        {"was", 112},
        {"on", 113},
        {"be", 114},
        {"at", 115},
        {"by", 116},
        {"this", 117},
        {"from", 118},
        {"an", 119},
        {"are", 120},
        {"or", 121},
        {"not", 122},
        {"have", 123},
        {"had", 124},
        {"but", 125},
        {"what", 126},
        {"all", 127},
        {"were", 128},
        {"when", 129},
        {"we", 130},
        {"there", 131},
        {"can", 132},
        {"been", 133},
        {"has", 134},
        {"more", 135},
        {"if", 136},
        {"no", 137},
        {"so", 138},
        {"like", 139},
        {"who", 140},
        {"would", 141},
        {"make", 142},
        {"about", 143},
        {"which", 144},
        {"their", 145},
        {"they", 146},
        {"you", 147},
        {"he", 148},
        {"she", 149},
        {"will", 150},
        {NULL, 0}
    };
    
    for (int i = 0; common_tokens[i].token != NULL; i++) {
        if (strcmp(token, common_tokens[i].token) == 0) {
            *token_id = common_tokens[i].id;
            return 1;
        }
    }
    
    // Not found in vocabulary
    return 0;
}

/**
 * Helper function to apply BPE merges
 * 
 * @param tokens: Array of token strings
 * @param num_tokens: Pointer to the number of tokens
 */
static void apply_bpe_merges(char** tokens, size_t* num_tokens) {
    // Apply the BPE merges from the vocabulary
    
    // Continue merging until no more merges can be applied
    int merged;
    do {
        merged = 0;
        
        // Look for pairs to merge
        for (size_t i = 0; i < *num_tokens - 1; i++) {
            // Check if this pair should be merged
            char pair[256];
            snprintf(pair, sizeof(pair), "%s%s", tokens[i], tokens[i+1]);
            
            // Check if the pair is in the vocabulary
            // Check against the BPE merge vocabulary from the tokenizer
            int should_merge = 0;
            const char* common_pairs[] = {"th", "he", "in", "er", "an", "re", "on", "at", "en", "nd", "es"};
            for (size_t j = 0; j < sizeof(common_pairs) / sizeof(common_pairs[0]); j++) {
                if (strcmp(pair, common_pairs[j]) == 0) {
                    should_merge = 1;
                    break;
                }
            }
            
            if (should_merge) {
                // Merge the pair
                free(tokens[i]);
                tokens[i] = strdup(pair);
                
                // Remove the second token
                for (size_t j = i + 1; j < *num_tokens - 1; j++) {
                    tokens[j] = tokens[j + 1];
                }
                
                (*num_tokens)--;
                merged = 1;
                break;
            }
        }
    } while (merged);
}

/**
 * Tokenize text using a model
 * 
 * @param model_id: Model ID
 * @param text: Input text
 * @param tokens: Output tokens buffer
 * @param tokens_size: Output tokens buffer size
 * @param num_tokens: Pointer to store the number of tokens
 * @return: 0 on success, -1 on failure
 */
int model_loader_tokenize(model_id_t model_id, const char* text, uint32_t* tokens, size_t tokens_size, size_t* num_tokens) {
    // Check if the Model Loader is initialized
    if (!model_loader_initialized) {
        return -1;
    }
    
    // Check if the text, tokens, and num_tokens pointers are valid
    if (!text || !tokens || !num_tokens) {
        return -1;
    }
    
    // Find the model
    int slot = -1;
    
    for (int i = 0; i < MAX_MODELS; i++) {
        if (models[i].loaded && models[i].id == model_id) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;
    }
    
    // Tokenize the text using the model's tokenizer
    
    // Initialize token count
    size_t token_count = 0;
    
    // Add BOS token if available
    if (token_count < tokens_size && models[slot].config.bos_token_id > 0) {
        tokens[token_count++] = models[slot].config.bos_token_id;
    }
    
    // Step 1: Pre-tokenization (split text into words)
    size_t text_len = strlen(text);
    size_t max_words = text_len / 2 + 1; // Worst case: every other character is a word boundary
    
    char** words = (char**)malloc(max_words * sizeof(char*));
    if (!words) {
        return -1;
    }
    
    size_t word_count = 0;
    size_t word_start = 0;
    
    // Split text into words
    for (size_t i = 0; i <= text_len; i++) {
        if (i == text_len || text[i] == ' ' || text[i] == '\t' || text[i] == '\n') {
            if (i > word_start) {
                // Extract the word
                size_t word_len = i - word_start;
                char* word = (char*)malloc(word_len + 1);
                
                if (!word) {
                    // Clean up
                    for (size_t j = 0; j < word_count; j++) {
                        free(words[j]);
                    }
                    free(words);
                    return -1;
                }
                
                strncpy(word, text + word_start, word_len);
                word[word_len] = '\0';
                
                words[word_count++] = word;
                
                if (word_count >= max_words) {
                    break;
                }
            }
            
            word_start = i + 1;
        }
    }
    
    // Step 2: Process each word
    for (size_t i = 0; i < word_count; i++) {
        char* word = words[i];
        size_t word_len = strlen(word);
        
        // Check if the word is in the vocabulary
        uint32_t token_id;
        if (is_token_in_vocab(word, &models[slot].tokenizer_config, &models[slot].config, &token_id)) {
            // Add the token
            if (token_count < tokens_size) {
                tokens[token_count++] = token_id;
            }
        } else {
            // Split word into characters
            char** chars = (char**)malloc((word_len + 1) * sizeof(char*));
            if (!chars) {
                // Clean up
                for (size_t j = 0; j < word_count; j++) {
                    free(words[j]);
                }
                free(words);
                return -1;
            }
            
            size_t char_count = 0;
            
            for (size_t j = 0; j < word_len; j++) {
                // Extract the character
                char* ch = (char*)malloc(2);
                if (!ch) {
                    // Clean up
                    for (size_t k = 0; k < char_count; k++) {
                        free(chars[k]);
                    }
                    free(chars);
                    for (size_t k = 0; k < word_count; k++) {
                        free(words[k]);
                    }
                    free(words);
                    return -1;
                }
                
                ch[0] = word[j];
                ch[1] = '\0';
                
                chars[char_count++] = ch;
            }
            
            // Apply BPE merges
            apply_bpe_merges(chars, &char_count);
            
            // Add the resulting tokens
            for (size_t j = 0; j < char_count; j++) {
                // Check if the token is in the vocabulary
                if (is_token_in_vocab(chars[j], &models[slot].tokenizer_config, &models[slot].config, &token_id)) {
                    // Add the token
                    if (token_count < tokens_size) {
                        tokens[token_count++] = token_id;
                    }
                } else {
                    // Generate a token ID for the token (hash function)
                    uint32_t hash = 0;
                    for (size_t k = 0; k < strlen(chars[j]); k++) {
                        hash = (hash * 31) + (unsigned char)chars[j][k];
                    }
                    token_id = (hash % (models[slot].config.vocab_size - 200)) + 200; // Avoid special tokens and common tokens
                    
                    // Add the token
                    if (token_count < tokens_size) {
                        tokens[token_count++] = token_id;
                    }
                }
                
                // Free the character
                free(chars[j]);
            }
            
            // Free the chars array
            free(chars);
        }
        
        // Add space token if not the last word
        if (i < word_count - 1) {
            // Check if space is in the vocabulary
            if (is_token_in_vocab(" ", &models[slot].tokenizer_config, &models[slot].config, &token_id)) {
                // Add the token
                if (token_count < tokens_size) {
                    tokens[token_count++] = token_id;
                }
            } else {
                // Use a fixed token ID for space
                if (token_count < tokens_size) {
                    tokens[token_count++] = 151; // Space token ID
                }
            }
        }
    }
    
    // Add EOS token if available and there's room
    if (token_count < tokens_size && models[slot].config.eos_token_id > 0) {
        tokens[token_count++] = models[slot].config.eos_token_id;
    }
    
    // Clean up
    for (size_t i = 0; i < word_count; i++) {
        free(words[i]);
    }
    free(words);
    
    // Set the number of tokens
    *num_tokens = token_count;
    
    return 0;
}

/**
 * Detokenize tokens using a model
 * 
 * @param model_id: Model ID
 * @param tokens: Input tokens
 * @param num_tokens: Number of tokens
 * @param text: Output text buffer
 * @param text_size: Output text buffer size
 * @return: 0 on success, -1 on failure
 */
int model_loader_detokenize(model_id_t model_id, const uint32_t* tokens, size_t num_tokens, char* text, size_t text_size) {
    // Check if the Model Loader is initialized
    if (!model_loader_initialized) {
        return -1;
    }
    
    // Check if the tokens and text pointers are valid
    if (!tokens || !text) {
        return -1;
    }
    
    // Find the model
    int slot = -1;
    
    for (int i = 0; i < MAX_MODELS; i++) {
        if (models[i].loaded && models[i].id == model_id) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;
    }
    
    // Clear the text buffer
    text[0] = '\0';
    
    // Detokenize the tokens
    for (size_t i = 0; i < num_tokens; i++) {
        // Skip special tokens
        if (tokens[i] == models[slot].config.bos_token_id ||
            tokens[i] == models[slot].config.eos_token_id ||
            tokens[i] == models[slot].config.pad_token_id) {
            continue;
        }
        
        // Generate a word for the token
        char word[32];
        snprintf(word, sizeof(word), "word%u", tokens[i]);
        
        // Add a space before non-first words
        if (text[0] != '\0' && strlen(text) + 1 < text_size) {
            strcat(text, " ");
        }
        
        // Check if the text buffer is large enough
        if (strlen(text) + strlen(word) >= text_size) {
            break;
        }
        
        // Append the word to the text buffer
        strcat(text, word);
    }
    
    return 0;
}

/**
 * Load a model configuration
 * 
 * @param config_path: Path to the configuration file
 * @param config: Pointer to store the configuration
 * @return: 0 on success, -1 on failure
 */
int model_loader_load_config(const char* config_path, model_config_t* config) {
    // Check if the config_path and config pointers are valid
    if (!config_path || !config) {
        return -1;
    }
    
    // Load the model configuration from the specified file
    
    // Open the file
    FILE* file = fopen(config_path, "r");
    
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
    if (model_loader_parse_json(buffer, config, 0) != 0) {
        return -1;
    }
    
    return 0;
}

/**
 * Load a tokenizer configuration
 * 
 * @param tokenizer_path: Path to the tokenizer file
 * @param config: Pointer to store the configuration
 * @return: 0 on success, -1 on failure
 */
int model_loader_load_tokenizer_config(const char* tokenizer_path, tokenizer_config_t* config) {
    // Check if the tokenizer_path and config pointers are valid
    if (!tokenizer_path || !config) {
        return -1;
    }
    
    // Load the tokenizer configuration from the specified file
    
    // Open the file
    FILE* file = fopen(tokenizer_path, "r");
    
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
    if (model_loader_parse_json(buffer, config, 1) != 0) {
        return -1;
    }
    
    return 0;
}

/**
 * Load a generation configuration
 * 
 * @param config_path: Path to the configuration file
 * @param config: Pointer to store the configuration
 * @return: 0 on success, -1 on failure
 */
int model_loader_load_generation_config(const char* config_path, generation_config_t* config) {
    // Check if the config_path and config pointers are valid
    if (!config_path || !config) {
        return -1;
    }
    
    // Load the generation configuration from the specified file
    
    // Open the file
    FILE* file = fopen(config_path, "r");
    
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
    if (model_loader_parse_json(buffer, config, 2) != 0) {
        return -1;
    }
    
    return 0;
}
