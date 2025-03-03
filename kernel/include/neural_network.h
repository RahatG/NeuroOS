/**
 * neural_network.h - Neural network interface for NeuroOS
 * 
 * This file contains the neural network interface definitions and declarations.
 */

#ifndef NEUROOS_NEURAL_NETWORK_H
#define NEUROOS_NEURAL_NETWORK_H

#include <stddef.h>
#include <stdint.h>

// Neural network model ID type
typedef uint32_t nn_model_id_t;

// Neural network model types
typedef enum {
    NN_MODEL_TYPE_UNKNOWN = 0,
    NN_MODEL_TYPE_DEEPSEEK = 1,
    NN_MODEL_TYPE_LLAMA = 2,
    NN_MODEL_TYPE_BERT = 3,
    NN_MODEL_TYPE_GPT2 = 4,
    NN_MODEL_TYPE_CUSTOM = 5
} nn_model_type_t;

// Neural network model information structure
typedef struct {
    nn_model_id_t id;
    nn_model_type_t type;
    char name[64];
} nn_model_info_t;

// Neural network types
#define NN_TYPE_UNKNOWN       0
#define NN_TYPE_FEEDFORWARD   1
#define NN_TYPE_RECURRENT     2
#define NN_TYPE_CONVOLUTIONAL 3
#define NN_TYPE_TRANSFORMER   4
#define NN_TYPE_LSTM          5
#define NN_TYPE_GRU           6
#define NN_TYPE_AUTOENCODER   7
#define NN_TYPE_GAN           8
#define NN_TYPE_VAE           9
#define NN_TYPE_CUSTOM        10

// Neural network activation functions
#define NN_ACTIVATION_NONE      0
#define NN_ACTIVATION_SIGMOID   1
#define NN_ACTIVATION_TANH      2
#define NN_ACTIVATION_RELU      3
#define NN_ACTIVATION_LEAKY_RELU 4
#define NN_ACTIVATION_ELU       5
#define NN_ACTIVATION_SELU      6
#define NN_ACTIVATION_SOFTMAX   7
#define NN_ACTIVATION_SOFTPLUS  8
#define NN_ACTIVATION_SOFTSIGN  9
#define NN_ACTIVATION_SWISH     10
#define NN_ACTIVATION_MISH      11
#define NN_ACTIVATION_GELU      12
#define NN_ACTIVATION_CUSTOM    13

// Neural network loss functions
#define NN_LOSS_NONE                 0
#define NN_LOSS_MSE                  1
#define NN_LOSS_MAE                  2
#define NN_LOSS_HUBER                3
#define NN_LOSS_BINARY_CROSSENTROPY  4
#define NN_LOSS_CATEGORICAL_CROSSENTROPY 5
#define NN_LOSS_SPARSE_CATEGORICAL_CROSSENTROPY 6
#define NN_LOSS_HINGE                7
#define NN_LOSS_SQUARED_HINGE        8
#define NN_LOSS_CATEGORICAL_HINGE    9
#define NN_LOSS_LOGCOSH              10
#define NN_LOSS_POISSON              11
#define NN_LOSS_KL_DIVERGENCE        12
#define NN_LOSS_CUSTOM               13

// Neural network optimizer types
#define NN_OPTIMIZER_NONE            0
#define NN_OPTIMIZER_SGD             1
#define NN_OPTIMIZER_MOMENTUM        2
#define NN_OPTIMIZER_ADAGRAD         3
#define NN_OPTIMIZER_ADADELTA        4
#define NN_OPTIMIZER_RMSPROP         5
#define NN_OPTIMIZER_ADAM            6
#define NN_OPTIMIZER_ADAMAX          7
#define NN_OPTIMIZER_NADAM           8
#define NN_OPTIMIZER_FTRL            9
#define NN_OPTIMIZER_CUSTOM          10

// Neural network layer types
#define NN_LAYER_NONE                0
#define NN_LAYER_INPUT               1
#define NN_LAYER_DENSE               2
#define NN_LAYER_CONV1D              3
#define NN_LAYER_CONV2D              4
#define NN_LAYER_CONV3D              5
#define NN_LAYER_POOLING1D           6
#define NN_LAYER_POOLING2D           7
#define NN_LAYER_POOLING3D           8
#define NN_LAYER_RECURRENT           9
#define NN_LAYER_LSTM                10
#define NN_LAYER_GRU                 11
#define NN_LAYER_EMBEDDING           12
#define NN_LAYER_BATCH_NORMALIZATION 13
#define NN_LAYER_DROPOUT             14
#define NN_LAYER_FLATTEN             15
#define NN_LAYER_RESHAPE             16
#define NN_LAYER_PERMUTE             17
#define NN_LAYER_ATTENTION           18
#define NN_LAYER_TRANSFORMER_ENCODER 19
#define NN_LAYER_TRANSFORMER_DECODER 20
#define NN_LAYER_CUSTOM              21

// Neural network tensor data types
#define NN_DTYPE_UNKNOWN             0
#define NN_DTYPE_FLOAT16             1
#define NN_DTYPE_FLOAT32             2
#define NN_DTYPE_FLOAT64             3
#define NN_DTYPE_INT8                4
#define NN_DTYPE_INT16               5
#define NN_DTYPE_INT32               6
#define NN_DTYPE_INT64               7
#define NN_DTYPE_UINT8               8
#define NN_DTYPE_UINT16              9
#define NN_DTYPE_UINT32              10
#define NN_DTYPE_UINT64              11
#define NN_DTYPE_BOOL                12

// Neural network layer configuration structure
typedef struct {
    uint32_t type;
    uint32_t units;
    uint32_t activation;
    size_t weights_size;
    void* weights;
    size_t bias_size;
    void* bias;
    float quantization_scale;
    uint32_t quantization_bits;
} nn_layer_config_t;

// Neural network model configuration structure
typedef struct {
    char name[64];
    uint32_t type;
    uint32_t input_shape[4];
    uint32_t num_layers;
    nn_layer_config_t* layers;
    uint32_t loss;
    uint32_t optimizer;
    float learning_rate;
    uint32_t batch_size;
    uint32_t epochs;
    uint32_t steps_per_epoch;
    uint32_t validation_steps;
    float validation_split;
    uint32_t verbose;
} nn_model_config_t;

// Neural network inference result structure
typedef struct {
    void* output;
    size_t output_size;
    float confidence;
    size_t predicted_class;
    uint64_t inference_time;
} nn_inference_result_t;

// Neural network model state structure
typedef struct {
    nn_model_id_t id;
    char name[64];
    uint32_t input_shape[4];
    uint32_t output_shape[4];
    uint32_t num_layers;
    nn_layer_config_t* layers;
    uint32_t loss;
    uint32_t optimizer;
    float learning_rate;
    uint32_t batch_size;
    uint32_t epochs;
    uint32_t current_epoch;
    float train_loss;
    float train_accuracy;
    float val_loss;
    float val_accuracy;
    uint64_t training_time;
    uint64_t inference_time;
    uint64_t memory_usage;
} nn_model_state_t;

// Neural network tensor structure
typedef struct {
    void* data;
    uint32_t* shape;
    uint32_t ndim;
    uint32_t dtype;
    uint32_t size;
    uint32_t flags;
} nn_tensor_t;

// Neural network layer structure
typedef struct {
    uint32_t type;
    uint32_t activation;
    uint32_t input_shape[8];
    uint32_t output_shape[8];
    uint32_t input_ndim;
    uint32_t output_ndim;
    uint32_t num_weights;
    uint32_t num_biases;
    nn_tensor_t* weights;
    nn_tensor_t* biases;
    void* config;
    void* state;
} nn_layer_t;

// Forward declaration of the internal model structure
typedef struct nn_model nn_model_t;

// Neural network initialization and shutdown
int nn_init(void);
int nn_shutdown(void);

// Neural network model operations
int nn_load_model(nn_model_type_t type, const char* name, const char* path, nn_model_id_t* id);
int nn_unload_model(nn_model_id_t id);
int nn_get_model_info(nn_model_id_t id, nn_model_info_t* info);
int nn_generate(nn_model_id_t id, const char* prompt, char* output, size_t size, uint32_t max_tokens, float temperature, float top_p, float top_k, float repetition_penalty);
int nn_run_inference(nn_model_id_t id, nn_tensor_t* input, nn_tensor_t* output);

// Neural network model loading and inference with different model types
int nn_load_deepseek_model(nn_model_t* model, const char* path);
int nn_unload_deepseek_model(nn_model_t* model);
int nn_deepseek_tokenize(nn_model_t* model, const char* text, uint32_t** tokens, size_t* num_tokens);
int nn_deepseek_detokenize(uint32_t* tokens, uint32_t num_tokens, char* text, size_t size);
int nn_deepseek_generate(nn_model_t* model, const char* prompt, char* output, size_t size, uint32_t max_tokens, float temperature, float top_p, float top_k, float repetition_penalty);

int nn_load_llama_model(nn_model_t* model, const char* path);
int nn_unload_llama_model(nn_model_t* model);
int nn_llama_generate(nn_model_t* model, const char* prompt, char* output, size_t size, uint32_t max_tokens, float temperature, float top_p, float top_k, float repetition_penalty);

int nn_load_bert_model(nn_model_t* model, const char* path);
int nn_unload_bert_model(nn_model_t* model);
int nn_bert_generate(nn_model_t* model, const char* prompt, char* output, size_t size, uint32_t max_tokens, float temperature, float top_p, float top_k, float repetition_penalty);

int nn_load_gpt2_model(nn_model_t* model, const char* path);
int nn_unload_gpt2_model(nn_model_t* model);
int nn_gpt2_generate(nn_model_t* model, const char* prompt, char* output, size_t size, uint32_t max_tokens, float temperature, float top_p, float top_k, float repetition_penalty);

int nn_load_custom_model(nn_model_t* model, const char* path);
int nn_unload_custom_model(nn_model_t* model);
int nn_custom_generate(nn_model_t* model, const char* prompt, char* output, size_t size, uint32_t max_tokens, float temperature, float top_p, float top_k, float repetition_penalty);

#endif // NEUROOS_NEURAL_NETWORK_H
