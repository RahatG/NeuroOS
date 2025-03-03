/**
 * model_loader.h - Model Loader interface for NeuroOS
 * 
 * This file defines the interface for the Model Loader subsystem, which is
 * responsible for loading and managing AI models.
 */

#ifndef NEUROOS_MODEL_LOADER_H
#define NEUROOS_MODEL_LOADER_H

#include <stddef.h>
#include <stdint.h>

// Model ID type
typedef uint32_t model_id_t;

// Model types
typedef enum {
    MODEL_TYPE_TRANSFORMER = 0,
    MODEL_TYPE_CNN = 1,
    MODEL_TYPE_RNN = 2,
    MODEL_TYPE_LSTM = 3,
    MODEL_TYPE_GRU = 4,
    MODEL_TYPE_CUSTOM = 5
} model_type_t;

// Model configuration
typedef struct {
    char name[64];
    char path[256];
    model_type_t type;
    uint32_t vocab_size;
    uint32_t hidden_size;
    uint32_t num_hidden_layers;
    uint32_t num_attention_heads;
    uint32_t intermediate_size;
    uint32_t max_position_embeddings;
    uint32_t type_vocab_size;
    float initializer_range;
    float layer_norm_eps;
    uint32_t pad_token_id;
    uint32_t bos_token_id;
    uint32_t eos_token_id;
    uint32_t sep_token_id;
    uint32_t cls_token_id;
    uint32_t mask_token_id;
    uint32_t unk_token_id;
} model_config_t;

// Model state
typedef struct {
    model_id_t id;
    char name[64];
    model_type_t type;
    uint64_t memory_usage;
    uint64_t load_time;
    uint64_t inference_time;
    uint32_t num_parameters;
    uint32_t num_layers;
    uint32_t batch_size;
    uint32_t sequence_length;
    uint32_t vocab_size;
    uint32_t hidden_size;
} model_state_t;

// Tokenizer configuration
typedef struct {
    char path[256];
    uint32_t vocab_size;
    uint32_t max_length;
    char bos_token[16];
    char eos_token[16];
    char pad_token[16];
    char sep_token[16];
    char cls_token[16];
    char mask_token[16];
    char unk_token[16];
} tokenizer_config_t;

// Generation configuration
typedef struct {
    uint32_t max_length;
    uint32_t min_length;
    float temperature;
    float top_p;
    float top_k;
    float repetition_penalty;
    float length_penalty;
    float diversity_penalty;
    uint32_t num_beams;
    uint32_t num_beam_groups;
    uint32_t num_return_sequences;
    uint32_t early_stopping;
    uint32_t do_sample;
    uint32_t no_repeat_ngram_size;
    uint32_t bad_words_ids;
    uint32_t encoder_no_repeat_ngram_size;
    uint32_t forced_bos_token_id;
    uint32_t forced_eos_token_id;
} generation_config_t;

// Model loader initialization and shutdown
int model_loader_init(void);
int model_loader_shutdown(void);

// Model management
model_id_t model_loader_load_model(const char* model_path, const char* config_path, const char* tokenizer_path);
int model_loader_unload_model(model_id_t model_id);
int model_loader_get_model_info(model_id_t model_id, model_state_t* state);

// Model operations
int model_loader_generate_text(model_id_t model_id, const char* prompt, char* output, size_t output_size, const generation_config_t* config);
int model_loader_tokenize(model_id_t model_id, const char* text, uint32_t* tokens, size_t tokens_size, size_t* num_tokens);
int model_loader_detokenize(model_id_t model_id, const uint32_t* tokens, size_t num_tokens, char* text, size_t text_size);

// Utility functions
int model_loader_load_config(const char* config_path, model_config_t* config);
int model_loader_load_tokenizer_config(const char* tokenizer_path, tokenizer_config_t* config);
int model_loader_load_generation_config(const char* config_path, generation_config_t* config);

#endif // NEUROOS_MODEL_LOADER_H
