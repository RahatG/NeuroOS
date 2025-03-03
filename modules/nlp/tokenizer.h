/**
 * tokenizer.h - Tokenizer interface for NeuroOS NLP module
 *
 * This file contains the tokenizer interface definitions and declarations for
 * the NeuroOS NLP module.
 */

#ifndef NEUROOS_NLP_TOKENIZER_H
#define NEUROOS_NLP_TOKENIZER_H

#include <stddef.h>
#include <stdint.h>

// Tokenizer types
#define TOKENIZER_TYPE_UNKNOWN     0
#define TOKENIZER_TYPE_WORDPIECE   1
#define TOKENIZER_TYPE_BPE         2
#define TOKENIZER_TYPE_UNIGRAM     3
#define TOKENIZER_TYPE_CHAR        4
#define TOKENIZER_TYPE_WORD        5
#define TOKENIZER_TYPE_WHITESPACE  6
#define TOKENIZER_TYPE_CUSTOM      7

// Tokenizer ID type
typedef uint32_t tokenizer_id_t;

// Token structure
typedef struct {
    uint32_t id;
    char text[64];
    float score;
} token_t;

// Tokenization result structure
typedef struct {
    token_t* tokens;
    size_t num_tokens;
    uint64_t tokenization_time;
} tokenization_result_t;

// Tokenizer configuration structure
typedef struct {
    char name[64];
    char path[256];
    uint32_t type;
    uint32_t vocab_size;
    uint32_t max_length;
    char bos_token[16];
    char eos_token[16];
    char pad_token[16];
    char sep_token[16];
    char cls_token[16];
    char mask_token[16];
    char unk_token[16];
    uint32_t bos_token_id;
    uint32_t eos_token_id;
    uint32_t pad_token_id;
    uint32_t sep_token_id;
    uint32_t cls_token_id;
    uint32_t mask_token_id;
    uint32_t unk_token_id;
} tokenizer_config_t;

// Tokenizer state structure
typedef struct {
    tokenizer_id_t id;
    char name[64];
    uint32_t type;
    uint64_t memory_usage;
    uint64_t load_time;
    uint64_t tokenization_time;
    uint32_t vocab_size;
    uint32_t max_length;
} tokenizer_state_t;

// Tokenizer initialization and shutdown
int tokenizer_init(void);
int tokenizer_shutdown(void);

// Tokenizer operations
tokenizer_id_t tokenizer_create(const tokenizer_config_t* config);
tokenizer_id_t tokenizer_load(const char* path);
int tokenizer_delete(tokenizer_id_t tokenizer_id);
int tokenizer_get_info(tokenizer_id_t tokenizer_id, tokenizer_state_t* state);
int tokenizer_tokenize(tokenizer_id_t tokenizer_id, const char* text, tokenization_result_t* result);
int tokenizer_detokenize(tokenizer_id_t tokenizer_id, const uint32_t* tokens, size_t num_tokens, char* text, size_t text_size);
int tokenizer_encode(tokenizer_id_t tokenizer_id, const char* text, uint32_t* tokens, size_t tokens_size, size_t* num_tokens);
int tokenizer_decode(tokenizer_id_t tokenizer_id, const uint32_t* tokens, size_t num_tokens, char* text, size_t text_size);
void tokenizer_free_tokenization_result(tokenization_result_t* result);
int tokenizer_load_config(const char* path, tokenizer_config_t* config);
int tokenizer_save_config(const char* path, const tokenizer_config_t* config);
int tokenizer_get_token_id(tokenizer_id_t tokenizer_id, const char* token);
int tokenizer_get_token_text(tokenizer_id_t tokenizer_id, uint32_t token_id, char* token, size_t token_size);

#endif // NEUROOS_NLP_TOKENIZER_H
