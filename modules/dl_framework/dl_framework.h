/**
 * dl_framework.h - Deep Learning Framework interface for NeuroOS
 *
 * This file contains the Deep Learning Framework interface definitions and declarations
 * for the NeuroOS deep learning module.
 */

#ifndef NEUROOS_DL_FRAMEWORK_H
#define NEUROOS_DL_FRAMEWORK_H

#include <stddef.h>
#include <stdint.h>
#include "../../kernel/include/neural_network.h"

// DL framework types
#define DL_FRAMEWORK_TYPE_UNKNOWN     0
#define DL_FRAMEWORK_TYPE_CUSTOM      1
#define DL_FRAMEWORK_TYPE_DEEPSEEK    2

// DL framework operation types
#define DL_OP_TYPE_UNKNOWN            0
#define DL_OP_TYPE_ADD                1
#define DL_OP_TYPE_SUB                2
#define DL_OP_TYPE_MUL                3
#define DL_OP_TYPE_DIV                4
#define DL_OP_TYPE_MATMUL             5
#define DL_OP_TYPE_CONV1D             6
#define DL_OP_TYPE_CONV2D             7
#define DL_OP_TYPE_MAXPOOL            8
#define DL_OP_TYPE_AVGPOOL            9
#define DL_OP_TYPE_RELU               10
#define DL_OP_TYPE_SIGMOID            11
#define DL_OP_TYPE_TANH               12
#define DL_OP_TYPE_SOFTMAX            13
#define DL_OP_TYPE_BATCHNORM          14
#define DL_OP_TYPE_DROPOUT            15
#define DL_OP_TYPE_EMBEDDING          16
#define DL_OP_TYPE_LSTM               17
#define DL_OP_TYPE_GRU                18
#define DL_OP_TYPE_ATTENTION          19
#define DL_OP_TYPE_TRANSFORMER        20
#define DL_OP_TYPE_CUSTOM             21

// DL framework ID type
typedef uint32_t dl_framework_id_t;

// DL framework operation ID type
typedef uint32_t dl_op_id_t;

// DL framework configuration structure
typedef struct {
    uint32_t type;
    char name[64];
    uint32_t num_threads;
    uint32_t use_gpu;
    uint32_t gpu_id;
    uint32_t memory_limit;
    uint32_t compute_precision;
    uint32_t optimization_level;
    uint32_t debug_mode;
    uint32_t profiling_mode;
    uint32_t cache_mode;
    char model_path[256];
} dl_framework_config_t;

// DL framework state structure
typedef struct {
    dl_framework_id_t id;
    uint32_t type;
    char name[64];
    uint32_t num_threads;
    uint32_t use_gpu;
    uint32_t gpu_id;
    uint32_t memory_usage;
    uint32_t compute_precision;
    uint32_t optimization_level;
    uint32_t debug_mode;
    uint32_t profiling_mode;
    uint32_t cache_mode;
    uint64_t load_time;
    uint64_t inference_time;
    uint64_t training_time;
    uint32_t num_operations;
    uint32_t num_tensors;
    uint32_t num_models;
    uint32_t num_layers;
    uint32_t num_parameters;
} dl_framework_state_t;

// DL framework operation structure
typedef struct {
    dl_op_id_t id;
    uint32_t type;
    char name[64];
    uint32_t num_inputs;
    uint32_t num_outputs;
    nn_tensor_t** inputs;
    nn_tensor_t** outputs;
    void* attributes;
    uint32_t attributes_size;
    uint64_t execution_time;
    uint32_t memory_usage;
    uint32_t flops;
} dl_op_t;

// DL framework initialization and shutdown
int dl_framework_init(void);
int dl_framework_shutdown(void);

// DL framework operations
dl_framework_id_t dl_framework_create(const dl_framework_config_t* config);
int dl_framework_destroy(dl_framework_id_t framework_id);
int dl_framework_get_state(dl_framework_id_t framework_id, dl_framework_state_t* state);
int dl_framework_load_model(dl_framework_id_t framework_id, const char* model_path, nn_model_t** model);
int dl_framework_save_model(dl_framework_id_t framework_id, const nn_model_t* model, const char* model_path);
int dl_framework_run_inference(dl_framework_id_t framework_id, const nn_model_t* model, const nn_tensor_t** inputs, uint32_t num_inputs, nn_tensor_t** outputs, uint32_t num_outputs);
int dl_framework_train_model(dl_framework_id_t framework_id, nn_model_t* model, const nn_tensor_t** inputs, const nn_tensor_t** targets, uint32_t num_samples, uint32_t batch_size, uint32_t epochs);
int dl_framework_optimize_model(dl_framework_id_t framework_id, nn_model_t* model, uint32_t optimization_level);
int dl_framework_quantize_model(dl_framework_id_t framework_id, nn_model_t* model, uint32_t quantization_level);
int dl_framework_profile_model(dl_framework_id_t framework_id, const nn_model_t* model, const nn_tensor_t** inputs, uint32_t num_inputs, char* profile_output, size_t output_size);
int dl_framework_debug_model(dl_framework_id_t framework_id, const nn_model_t* model, const nn_tensor_t** inputs, uint32_t num_inputs, char* debug_output, size_t output_size);
int dl_framework_export_model(dl_framework_id_t framework_id, const nn_model_t* model, const char* export_path, uint32_t export_format);
int dl_framework_import_model(dl_framework_id_t framework_id, const char* import_path, uint32_t import_format, nn_model_t** model);

// DL framework operation management
dl_op_id_t dl_framework_create_operation(dl_framework_id_t framework_id, uint32_t op_type, const char* name);
int dl_framework_destroy_operation(dl_framework_id_t framework_id, dl_op_id_t op_id);
int dl_framework_set_operation_inputs(dl_framework_id_t framework_id, dl_op_id_t op_id, nn_tensor_t** inputs, uint32_t num_inputs);
int dl_framework_set_operation_outputs(dl_framework_id_t framework_id, dl_op_id_t op_id, nn_tensor_t** outputs, uint32_t num_outputs);
int dl_framework_set_operation_attributes(dl_framework_id_t framework_id, dl_op_id_t op_id, const void* attributes, uint32_t attributes_size);
int dl_framework_get_operation_info(dl_framework_id_t framework_id, dl_op_id_t op_id, dl_op_t* op);
int dl_framework_execute_operation(dl_framework_id_t framework_id, dl_op_id_t op_id);

// DL framework tensor operations
int dl_framework_create_tensor(dl_framework_id_t framework_id, uint32_t* shape, uint32_t ndim, uint32_t dtype, nn_tensor_t** tensor);
int dl_framework_destroy_tensor(dl_framework_id_t framework_id, nn_tensor_t* tensor);
int dl_framework_copy_tensor(dl_framework_id_t framework_id, nn_tensor_t* src, nn_tensor_t* dst);
int dl_framework_reshape_tensor(dl_framework_id_t framework_id, nn_tensor_t* tensor, uint32_t* shape, uint32_t ndim);
int dl_framework_transpose_tensor(dl_framework_id_t framework_id, nn_tensor_t* tensor, uint32_t* axes, uint32_t ndim);
int dl_framework_slice_tensor(dl_framework_id_t framework_id, nn_tensor_t* tensor, uint32_t* start, uint32_t* end, uint32_t* step, nn_tensor_t* result);
int dl_framework_concat_tensors(dl_framework_id_t framework_id, nn_tensor_t** tensors, uint32_t num_tensors, uint32_t axis, nn_tensor_t* result);
int dl_framework_split_tensor(dl_framework_id_t framework_id, nn_tensor_t* tensor, uint32_t axis, uint32_t num_splits, nn_tensor_t** result);
int dl_framework_stack_tensors(dl_framework_id_t framework_id, nn_tensor_t** tensors, uint32_t num_tensors, uint32_t axis, nn_tensor_t* result);
int dl_framework_flatten_tensor(dl_framework_id_t framework_id, nn_tensor_t* tensor, nn_tensor_t* result);
int dl_framework_expand_dims(dl_framework_id_t framework_id, nn_tensor_t* tensor, uint32_t axis, nn_tensor_t* result);
int dl_framework_cast_tensor(dl_framework_id_t framework_id, nn_tensor_t* tensor, uint32_t dtype, nn_tensor_t* result);
int dl_framework_fill_tensor(dl_framework_id_t framework_id, nn_tensor_t* tensor, const void* value);
int dl_framework_fill_tensor_random(dl_framework_id_t framework_id, nn_tensor_t* tensor, float min, float max, uint64_t seed);
int dl_framework_get_tensor_value(dl_framework_id_t framework_id, nn_tensor_t* tensor, uint32_t* indices, uint32_t ndim, void* value);
int dl_framework_set_tensor_value(dl_framework_id_t framework_id, nn_tensor_t* tensor, uint32_t* indices, uint32_t ndim, const void* value);

// DL framework Deepseek R1 specific operations
int dl_framework_load_deepseek_model(dl_framework_id_t framework_id, const char* model_path, nn_model_t** model);
int dl_framework_deepseek_generate(dl_framework_id_t framework_id, nn_model_t* model, const char* prompt, char* output, size_t output_size, uint32_t max_tokens, float temperature, float top_p, float top_k, float repetition_penalty);
int dl_framework_deepseek_embed(dl_framework_id_t framework_id, nn_model_t* model, const char* text, float* embedding, uint32_t embedding_size);
int dl_framework_deepseek_classify(dl_framework_id_t framework_id, nn_model_t* model, const char* text, const char** labels, uint32_t num_labels, float* probabilities);
int dl_framework_deepseek_extract(dl_framework_id_t framework_id, nn_model_t* model, const char* text, const char* pattern, char* output, size_t output_size);
int dl_framework_deepseek_summarize(dl_framework_id_t framework_id, nn_model_t* model, const char* text, char* summary, size_t summary_size);
int dl_framework_deepseek_translate(dl_framework_id_t framework_id, nn_model_t* model, const char* text, const char* source_lang, const char* target_lang, char* translation, size_t translation_size);
int dl_framework_deepseek_answer(dl_framework_id_t framework_id, nn_model_t* model, const char* question, const char* context, char* answer, size_t answer_size);
int dl_framework_deepseek_chat(dl_framework_id_t framework_id, nn_model_t* model, const char** messages, uint32_t* roles, uint32_t num_messages, char* response, size_t response_size);

#endif // NEUROOS_DL_FRAMEWORK_H
