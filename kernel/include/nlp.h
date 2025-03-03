/**
 * nlp.h - Natural Language Processing interface for NeuroOS
 * 
 * This file contains the NLP interface definitions and declarations.
 */

#ifndef NEUROOS_NLP_H
#define NEUROOS_NLP_H

#include <stddef.h>
#include <stdint.h>
#include "neural_network.h"

// NLP model ID type
typedef uint32_t nlp_model_id_t;

// NLP model types
#define NLP_MODEL_TYPE_UNKNOWN       0
#define NLP_MODEL_TYPE_TRANSFORMER   1
#define NLP_MODEL_TYPE_BERT          2
#define NLP_MODEL_TYPE_GPT           3
#define NLP_MODEL_TYPE_T5            4
#define NLP_MODEL_TYPE_ROBERTA       5
#define NLP_MODEL_TYPE_ALBERT        6
#define NLP_MODEL_TYPE_DISTILBERT    7
#define NLP_MODEL_TYPE_XLM           8
#define NLP_MODEL_TYPE_XLNET         9
#define NLP_MODEL_TYPE_ELECTRA       10
#define NLP_MODEL_TYPE_LONGFORMER    11
#define NLP_MODEL_TYPE_REFORMER      12
#define NLP_MODEL_TYPE_BART          13
#define NLP_MODEL_TYPE_PEGASUS       14
#define NLP_MODEL_TYPE_MARIAN        15
#define NLP_MODEL_TYPE_DEEPSEEK      16
#define NLP_MODEL_TYPE_CUSTOM        17

// Simplified model type aliases
#define NLP_MODEL_BERT               NLP_MODEL_TYPE_BERT

// NLP task types
#define NLP_TASK_TYPE_UNKNOWN                0
#define NLP_TASK_TYPE_TEXT_GENERATION        1
#define NLP_TASK_TYPE_TEXT_CLASSIFICATION    2
#define NLP_TASK_TYPE_TOKEN_CLASSIFICATION   3
#define NLP_TASK_TYPE_QUESTION_ANSWERING     4
#define NLP_TASK_TYPE_SUMMARIZATION          5
#define NLP_TASK_TYPE_TRANSLATION            6
#define NLP_TASK_TYPE_FEATURE_EXTRACTION     7
#define NLP_TASK_TYPE_FILL_MASK              8
#define NLP_TASK_TYPE_SENTENCE_SIMILARITY    9
#define NLP_TASK_TYPE_ZERO_SHOT_CLASSIFICATION 10
#define NLP_TASK_TYPE_FEW_SHOT_CLASSIFICATION 11
#define NLP_TASK_TYPE_NAMED_ENTITY_RECOGNITION 12
#define NLP_TASK_TYPE_SENTIMENT_ANALYSIS     13
#define NLP_TASK_TYPE_TEXT_GENERATION_WITH_PREFIX 14
#define NLP_TASK_TYPE_CONVERSATIONAL         15
#define NLP_TASK_TYPE_CUSTOM                 16

// Simplified task type aliases
#define NLP_TASK_TEXT_CLASSIFICATION         NLP_TASK_TYPE_TEXT_CLASSIFICATION
#define NLP_TASK_NAMED_ENTITY_RECOGNITION    NLP_TASK_TYPE_NAMED_ENTITY_RECOGNITION

// NLP tokenizer types
#define NLP_TOKENIZER_TYPE_UNKNOWN           0
#define NLP_TOKENIZER_TYPE_WORDPIECE         1
#define NLP_TOKENIZER_TYPE_SENTENCEPIECE     2
#define NLP_TOKENIZER_TYPE_BPE               3
#define NLP_TOKENIZER_TYPE_UNIGRAM           4
#define NLP_TOKENIZER_TYPE_CHAR              5
#define NLP_TOKENIZER_TYPE_WORD              6
#define NLP_TOKENIZER_TYPE_WHITESPACE        7
#define NLP_TOKENIZER_TYPE_CUSTOM            8

// Simplified tokenizer type aliases
#define NLP_TOKENIZER_WORDPIECE              NLP_TOKENIZER_TYPE_WORDPIECE

// NLP model configuration structure
typedef struct {
    char name[64];
    char model_path[256];
    char tokenizer_path[256];
    char config_path[256];
    uint32_t type;
    uint32_t task;
    uint32_t tokenizer;
    uint32_t vocab_size;
    uint32_t max_seq_length;
    uint32_t embedding_dim;
    uint32_t num_attention_heads;
    uint32_t num_hidden_layers;
    uint32_t intermediate_size;
    float dropout_rate;
    float attention_dropout_rate;
    nn_model_id_t nn_model_id;
} nlp_model_config_t;

// NLP model state structure
typedef struct {
    nlp_model_id_t id;
    char name[64];
    uint32_t type;
    uint32_t task;
    uint32_t tokenizer;
    uint32_t vocab_size;
    uint32_t max_seq_length;
    uint32_t embedding_dim;
    uint32_t num_attention_heads;
    uint32_t num_hidden_layers;
    uint32_t intermediate_size;
    nn_model_id_t nn_model_id;
    uint64_t memory_usage;
    uint64_t inference_time;
    uint64_t tokenization_time;
} nlp_model_state_t;

// NLP token structure
typedef struct {
    uint32_t id;
    char text[64];
    uint32_t offset;
    uint32_t length;
    float score;
    uint32_t type;
    uint32_t flags;
} nlp_token_t;

// NLP tokenization result structure
typedef struct {
    nlp_token_t* tokens;
    uint32_t num_tokens;
    uint64_t tokenization_time;
} nlp_tokenization_result_t;

// NLP classification result structure
typedef struct {
    char label[64];
    float confidence;
    uint32_t num_classes;
    char** class_names;
    float* class_scores;
} nlp_classification_result_t;

// NLP entity structure
typedef struct {
    char text[64];
    char entity_type[32];
    uint32_t start_pos;
    uint32_t end_pos;
    float confidence;
} nlp_entity_t;

// NLP named entity recognition result structure
typedef struct {
    nlp_entity_t* entities;
    uint32_t num_entities;
} nlp_ner_result_t;

// NLP sentiment analysis result structure
typedef struct {
    uint32_t sentiment;
    float confidence;
    float positive_score;
    float negative_score;
    float neutral_score;
} nlp_sentiment_result_t;

// NLP sentiment types
#define NLP_SENTIMENT_UNKNOWN        0
#define NLP_SENTIMENT_POSITIVE       1
#define NLP_SENTIMENT_NEGATIVE       2
#define NLP_SENTIMENT_NEUTRAL        3

// NLP tokenizer structure
typedef struct {
    uint32_t type;
    uint32_t vocab_size;
    uint32_t max_length;
    char* unk_token;
    char* sep_token;
    char* pad_token;
    char* cls_token;
    char* mask_token;
    char* bos_token;
    char* eos_token;
    uint32_t unk_token_id;
    uint32_t sep_token_id;
    uint32_t pad_token_id;
    uint32_t cls_token_id;
    uint32_t mask_token_id;
    uint32_t bos_token_id;
    uint32_t eos_token_id;
    int do_lower_case;
    int do_basic_tokenize;
    int strip_accents;
    int split_on_punctuation;
    int add_prefix_space;
    int add_special_tokens;
    int truncation;
    int padding;
    void* vocab;
    void* merges;
    void* config;
    void* state;
} nlp_tokenizer_t;

// NLP model structure
typedef struct {
    uint32_t type;
    uint32_t task;
    nlp_tokenizer_t* tokenizer;
    nn_model_t* model;
    uint32_t hidden_size;
    uint32_t num_hidden_layers;
    uint32_t num_attention_heads;
    uint32_t intermediate_size;
    uint32_t max_position_embeddings;
    uint32_t type_vocab_size;
    uint32_t vocab_size;
    uint32_t pad_token_id;
    uint32_t bos_token_id;
    uint32_t eos_token_id;
    uint32_t sep_token_id;
    uint32_t cls_token_id;
    uint32_t mask_token_id;
    uint32_t unk_token_id;
    float attention_dropout;
    float hidden_dropout;
    float classifier_dropout;
    float initializer_range;
    float layer_norm_eps;
    void* config;
    void* state;
} nlp_model_t;

// NLP generation parameters structure
typedef struct {
    uint32_t max_length;
    uint32_t min_length;
    uint32_t do_sample;
    float temperature;
    float top_k;
    float top_p;
    float repetition_penalty;
    float length_penalty;
    float no_repeat_ngram_size;
    float bad_words_ids;
    float num_beam_groups;
    float diversity_penalty;
    uint32_t num_return_sequences;
    uint32_t num_beams;
    uint32_t early_stopping;
    uint32_t use_cache;
    uint32_t output_scores;
    uint32_t return_dict_in_generate;
    uint32_t output_attentions;
    uint32_t output_hidden_states;
} nlp_generation_params_t;

// NLP initialization and shutdown
int nlp_init(void);
int nlp_shutdown(void);

// NLP tokenizer operations
nlp_tokenizer_t* nlp_tokenizer_create(uint32_t type);
int nlp_tokenizer_destroy(nlp_tokenizer_t* tokenizer);
int nlp_tokenizer_from_file(const char* path, nlp_tokenizer_t** tokenizer);
int nlp_tokenizer_save(nlp_tokenizer_t* tokenizer, const char* path);
int nlp_tokenizer_encode(nlp_tokenizer_t* tokenizer, const char* text, uint32_t** ids, uint32_t* num_ids);
int nlp_tokenizer_decode(nlp_tokenizer_t* tokenizer, const uint32_t* ids, uint32_t num_ids, char* text, size_t size);
int nlp_tokenizer_tokenize(nlp_tokenizer_t* tokenizer, const char* text, nlp_token_t** tokens, uint32_t* num_tokens);
int nlp_tokenizer_detokenize(nlp_tokenizer_t* tokenizer, const nlp_token_t* tokens, uint32_t num_tokens, char* text, size_t size);
int nlp_tokenizer_get_vocab(nlp_tokenizer_t* tokenizer, char*** vocab, uint32_t* vocab_size);
int nlp_tokenizer_get_token(nlp_tokenizer_t* tokenizer, uint32_t id, char* token, size_t size);
int nlp_tokenizer_get_token_id(nlp_tokenizer_t* tokenizer, const char* token, uint32_t* id);
int nlp_tokenizer_add_token(nlp_tokenizer_t* tokenizer, const char* token, uint32_t* id);
int nlp_tokenizer_add_special_token(nlp_tokenizer_t* tokenizer, const char* token, uint32_t* id);
int nlp_tokenizer_set_truncation(nlp_tokenizer_t* tokenizer, int truncation, uint32_t max_length);
int nlp_tokenizer_set_padding(nlp_tokenizer_t* tokenizer, int padding, uint32_t max_length);
int nlp_tokenizer_set_add_special_tokens(nlp_tokenizer_t* tokenizer, int add_special_tokens);
int nlp_tokenizer_set_add_prefix_space(nlp_tokenizer_t* tokenizer, int add_prefix_space);
int nlp_tokenizer_set_do_lower_case(nlp_tokenizer_t* tokenizer, int do_lower_case);
int nlp_tokenizer_set_do_basic_tokenize(nlp_tokenizer_t* tokenizer, int do_basic_tokenize);
int nlp_tokenizer_set_strip_accents(nlp_tokenizer_t* tokenizer, int strip_accents);
int nlp_tokenizer_set_split_on_punctuation(nlp_tokenizer_t* tokenizer, int split_on_punctuation);

// NLP model operations
nlp_model_t* nlp_model_create(uint32_t type, uint32_t task);
int nlp_model_destroy(nlp_model_t* model);
int nlp_model_from_file(const char* path, nlp_model_t** model);
int nlp_model_save(nlp_model_t* model, const char* path);
int nlp_model_set_tokenizer(nlp_model_t* model, nlp_tokenizer_t* tokenizer);
int nlp_model_get_tokenizer(nlp_model_t* model, nlp_tokenizer_t** tokenizer);
int nlp_model_set_nn_model(nlp_model_t* model, nn_model_t* nn_model);
int nlp_model_get_nn_model(nlp_model_t* model, nn_model_t** nn_model);
int nlp_model_generate(nlp_model_t* model, const char* prompt, char* output, size_t size, nlp_generation_params_t* params);
int nlp_model_generate_tokens(nlp_model_t* model, const uint32_t* input_ids, uint32_t num_input_ids, uint32_t** output_ids, uint32_t* num_output_ids, nlp_generation_params_t* params);
int nlp_model_classify(nlp_model_t* model, const char* text, const char** labels, uint32_t num_labels, float* probabilities);
int nlp_model_classify_tokens(nlp_model_t* model, const uint32_t* input_ids, uint32_t num_input_ids, const char** labels, uint32_t num_labels, float* probabilities);
int nlp_model_token_classify(nlp_model_t* model, const char* text, const char** labels, uint32_t num_labels, float** probabilities, uint32_t* num_tokens);
int nlp_model_token_classify_tokens(nlp_model_t* model, const uint32_t* input_ids, uint32_t num_input_ids, const char** labels, uint32_t num_labels, float** probabilities, uint32_t* num_tokens);
int nlp_model_question_answer(nlp_model_t* model, const char* question, const char* context, char* answer, size_t size, uint32_t* start, uint32_t* end, float* score);
int nlp_model_question_answer_tokens(nlp_model_t* model, const uint32_t* question_ids, uint32_t num_question_ids, const uint32_t* context_ids, uint32_t num_context_ids, uint32_t** answer_ids, uint32_t* num_answer_ids, uint32_t* start, uint32_t* end, float* score);
int nlp_model_summarize(nlp_model_t* model, const char* text, char* summary, size_t size, nlp_generation_params_t* params);
int nlp_model_summarize_tokens(nlp_model_t* model, const uint32_t* input_ids, uint32_t num_input_ids, uint32_t** output_ids, uint32_t* num_output_ids, nlp_generation_params_t* params);
int nlp_model_translate(nlp_model_t* model, const char* text, const char* source_lang, const char* target_lang, char* translation, size_t size, nlp_generation_params_t* params);
int nlp_model_translate_tokens(nlp_model_t* model, const uint32_t* input_ids, uint32_t num_input_ids, const char* source_lang, const char* target_lang, uint32_t** output_ids, uint32_t* num_output_ids, nlp_generation_params_t* params);
int nlp_model_embed(nlp_model_t* model, const char* text, float* embedding, uint32_t embedding_size);
int nlp_model_embed_tokens(nlp_model_t* model, const uint32_t* input_ids, uint32_t num_input_ids, float* embedding, uint32_t embedding_size);
int nlp_model_similarity(nlp_model_t* model, const char* text1, const char* text2, float* similarity);
int nlp_model_similarity_tokens(nlp_model_t* model, const uint32_t* input_ids1, uint32_t num_input_ids1, const uint32_t* input_ids2, uint32_t num_input_ids2, float* similarity);
int nlp_model_fill_mask(nlp_model_t* model, const char* text, char* filled_text, size_t size, float* score);
int nlp_model_fill_mask_tokens(nlp_model_t* model, const uint32_t* input_ids, uint32_t num_input_ids, uint32_t** output_ids, uint32_t* num_output_ids, float* score);
int nlp_model_zero_shot_classify(nlp_model_t* model, const char* text, const char** labels, uint32_t num_labels, float* probabilities);
int nlp_model_zero_shot_classify_tokens(nlp_model_t* model, const uint32_t* input_ids, uint32_t num_input_ids, const char** labels, uint32_t num_labels, float* probabilities);
int nlp_model_few_shot_classify(nlp_model_t* model, const char* text, const char** examples, const char** labels, uint32_t num_examples, uint32_t num_labels, float* probabilities);
int nlp_model_few_shot_classify_tokens(nlp_model_t* model, const uint32_t* input_ids, uint32_t num_input_ids, const uint32_t** example_ids, const char** labels, uint32_t* num_example_ids, uint32_t num_examples, uint32_t num_labels, float* probabilities);
int nlp_model_ner(nlp_model_t* model, const char* text, const char** entity_types, uint32_t num_entity_types, char*** entities, uint32_t** entity_types_ids, uint32_t* num_entities);
int nlp_model_ner_tokens(nlp_model_t* model, const uint32_t* input_ids, uint32_t num_input_ids, const char** entity_types, uint32_t num_entity_types, uint32_t** entity_start_ids, uint32_t** entity_end_ids, uint32_t** entity_types_ids, uint32_t* num_entities);
int nlp_model_sentiment(nlp_model_t* model, const char* text, float* sentiment);
int nlp_model_sentiment_tokens(nlp_model_t* model, const uint32_t* input_ids, uint32_t num_input_ids, float* sentiment);
int nlp_model_chat(nlp_model_t* model, const char** messages, uint32_t* roles, uint32_t num_messages, char* response, size_t size, nlp_generation_params_t* params);
int nlp_model_chat_tokens(nlp_model_t* model, const uint32_t** message_ids, uint32_t* num_message_ids, uint32_t* roles, uint32_t num_messages, uint32_t** response_ids, uint32_t* num_response_ids, nlp_generation_params_t* params);

// NLP utility functions
int nlp_create_generation_params(nlp_generation_params_t* params);
int nlp_set_generation_params(nlp_generation_params_t* params, uint32_t max_length, uint32_t min_length, uint32_t do_sample, float temperature, float top_k, float top_p, float repetition_penalty, float length_penalty, float no_repeat_ngram_size, float bad_words_ids, float num_beam_groups, float diversity_penalty, uint32_t num_return_sequences, uint32_t num_beams, uint32_t early_stopping, uint32_t use_cache, uint32_t output_scores, uint32_t return_dict_in_generate, uint32_t output_attentions, uint32_t output_hidden_states);
int nlp_get_generation_params(nlp_generation_params_t* params, uint32_t* max_length, uint32_t* min_length, uint32_t* do_sample, float* temperature, float* top_k, float* top_p, float* repetition_penalty, float* length_penalty, float* no_repeat_ngram_size, float* bad_words_ids, float* num_beam_groups, float* diversity_penalty, uint32_t* num_return_sequences, uint32_t* num_beams, uint32_t* early_stopping, uint32_t* use_cache, uint32_t* output_scores, uint32_t* return_dict_in_generate, uint32_t* output_attentions, uint32_t* output_hidden_states);
int nlp_reset_generation_params(nlp_generation_params_t* params);
int nlp_copy_generation_params(nlp_generation_params_t* src, nlp_generation_params_t* dst);
int nlp_free_tokens(nlp_token_t* tokens, uint32_t num_tokens);
int nlp_free_ids(uint32_t* ids);
int nlp_free_text(char* text);
int nlp_free_embeddings(float* embeddings);
int nlp_free_probabilities(float* probabilities);
int nlp_free_entities(char** entities, uint32_t num_entities);
int nlp_free_entity_types_ids(uint32_t* entity_types_ids);
int nlp_free_entity_start_ids(uint32_t* entity_start_ids);
int nlp_free_entity_end_ids(uint32_t* entity_end_ids);
int nlp_free_vocab(char** vocab, uint32_t vocab_size);
int nlp_free_labels(char** labels, uint32_t num_labels);
int nlp_free_examples(char** examples, uint32_t num_examples);
int nlp_free_messages(char** messages, uint32_t num_messages);
int nlp_free_roles(uint32_t* roles);
int nlp_free_message_ids(uint32_t** message_ids, uint32_t num_messages);
int nlp_free_num_message_ids(uint32_t* num_message_ids);
int nlp_free_response_ids(uint32_t* response_ids);
int nlp_free_num_response_ids(uint32_t* num_response_ids);
int nlp_free_output_ids(uint32_t* output_ids);
int nlp_free_num_output_ids(uint32_t* num_output_ids);
int nlp_free_example_ids(uint32_t** example_ids, uint32_t num_examples);
int nlp_free_num_example_ids(uint32_t* num_example_ids);

// NLP Deepseek R1 model operations
int nlp_load_deepseek_model(const char* path, nlp_model_t** model);
int nlp_deepseek_tokenize(const char* text, uint32_t** tokens, uint32_t* num_tokens);
int nlp_deepseek_detokenize(uint32_t* tokens, uint32_t num_tokens, char* text, size_t size);
int nlp_deepseek_generate(nlp_model_t* model, const char* prompt, char* output, size_t size, uint32_t max_tokens, float temperature, float top_p, float top_k, float repetition_penalty);
int nlp_deepseek_embed(nlp_model_t* model, const char* text, float* embedding, uint32_t embedding_size);
int nlp_deepseek_classify(nlp_model_t* model, const char* text, const char** labels, uint32_t num_labels, float* probabilities);
int nlp_deepseek_extract(nlp_model_t* model, const char* text, const char* pattern, char* output, size_t size);
int nlp_deepseek_summarize(nlp_model_t* model, const char* text, char* summary, size_t size);
int nlp_deepseek_translate(nlp_model_t* model, const char* text, const char* source_lang, const char* target_lang, char* translation, size_t size);
int nlp_deepseek_answer(nlp_model_t* model, const char* question, const char* context, char* answer, size_t size);
int nlp_deepseek_chat(nlp_model_t* model, const char** messages, uint32_t* roles, uint32_t num_messages, char* response, size_t size);

#endif // NEUROOS_NLP_H
