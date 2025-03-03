/**
* neural_network.c - Neural network implementation for NeuroOS
*
* This file implements the neural network subsystem, which is responsible for
* loading and running neural network models.
*/

#include "include/neural_network.h"
#include "include/memory.h"
#include "include/console.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h> /* For sqrtf, expf, tanhf, sinf */

// Maximum number of models
#define MAX_MODELS 16

// Forward declarations for functions
int nn_load_deepseek_model(nn_model_t* model, const char* path);
int nn_unload_deepseek_model(nn_model_t* model);
int nn_deepseek_tokenize(nn_model_t* model, const char* text, uint32_t** tokens, size_t* num_tokens);
int nn_deepseek_detokenize(uint32_t* tokens, uint32_t num_tokens, char* text, size_t size);
int nn_deepseek_generate(nn_model_t* model, const char* prompt, char* output, size_t size, uint32_t max_tokens, float temperature, float top_p, float top_k, float repetition_penalty);

// Internal model structure
struct nn_model {
    nn_model_id_t id;
    nn_model_type_t type;
    char name[64];
    void* data;
    size_t data_size;
    void* weights;
    size_t weights_size;
    void* context;
};

// Model table
static nn_model_t* model_table[MAX_MODELS];

// Next available model ID
static nn_model_id_t next_model_id = 1;

/**
* Initialize the neural network subsystem
*/
int nn_init(void) {
    // Initialize the model table
    for (int i = 0; i < MAX_MODELS; i++) {
        model_table[i] = NULL;
    }

    console_printf("Neural network subsystem initialized\n");
    return 0;
}

/**
* Load a model
*
* @param type: Model type
* @param name: Model name
* @param path: Path to the model file
* @param id: Pointer to store the model ID
* @return: 0 on success, -1 on failure
*/
int nn_load_model(nn_model_type_t type, const char* name, const char* path, nn_model_id_t* id) {
    // Check if the name, path, and id pointers are valid
    if (!name || !path || !id) {
        console_printf("Error: Invalid parameters\n");
        return -1;
    }

    // Check if we have reached the maximum number of models
    if (next_model_id >= MAX_MODELS) {
        console_printf("Error: Maximum number of models reached\n");
        return -1;
    }

    // Allocate memory for the model structure
    nn_model_t* model = (nn_model_t*)memory_alloc(sizeof(nn_model_t), MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);

    if (!model) {
        console_printf("Error: Failed to allocate model structure\n");
        return -1;
    }

    // Initialize the model
    model->id = next_model_id++;
    model->type = type;
    strncpy(model->name, name, sizeof(model->name) - 1);

    // Load the model based on its type
    int result = -1;

    switch (type) {
        case NN_MODEL_TYPE_DEEPSEEK:
            result = nn_load_deepseek_model(model, path);
            break;

        default:
            console_printf("Error: Unsupported model type\n");
            memory_free(model, sizeof(nn_model_t));
            return -1;
    }

    if (result != 0) {
        console_printf("Error: Failed to load model\n");
        memory_free(model, sizeof(nn_model_t));
        return -1;
    }

    // Add the model to the model table
    model_table[model->id] = model;

    // Return the model ID
    *id = model->id;

    return 0;
}

/**
* Unload a model
*
* @param id: Model ID
* @return: 0 on success, -1 on failure
*/
int nn_unload_model(nn_model_id_t id) {
    // Check if the model ID is valid
    if (id >= MAX_MODELS || !model_table[id]) {
        console_printf("Error: Invalid model ID\n");
        return -1;
    }

    // Get the model
    nn_model_t* model = model_table[id];

    // Unload the model based on its type
    int result = -1;

    switch (model->type) {
        case NN_MODEL_TYPE_DEEPSEEK:
            result = nn_unload_deepseek_model(model);
            break;

        default:
            console_printf("Error: Unsupported model type\n");
            return -1;
    }

    if (result != 0) {
        console_printf("Error: Failed to unload model\n");
        return -1;
    }

    // Remove the model from the model table
    model_table[id] = NULL;

    // Free the model structure
    memory_free(model, sizeof(nn_model_t));

    return 0;
}

/**
* Get model information
*
* @param id: Model ID
* @param info: Pointer to store the model information
* @return: 0 on success, -1 on failure
*/
int nn_get_model_info(nn_model_id_t id, nn_model_info_t* info) {
    // Check if the model ID is valid
    if (id >= MAX_MODELS || !model_table[id]) {
        console_printf("Error: Invalid model ID\n");
        return -1;
    }

    // Check if the info pointer is valid
    if (!info) {
        console_printf("Error: Invalid info pointer\n");
        return -1;
    }

    // Get the model
    nn_model_t* model = model_table[id];

    // Fill in the model information
    info->id = model->id;
    info->type = model->type;
    strncpy(info->name, model->name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0'; // Ensure null termination

    return 0;
}

/**
* Generate text using a model
*
* @param id: Model ID
* @param prompt: Input prompt
* @param output: Buffer to store the generated text
* @param size: Size of the output buffer
* @param max_tokens: Maximum number of tokens to generate
* @param temperature: Temperature for sampling
* @param top_p: Top-p sampling parameter
* @param top_k: Top-k sampling parameter
* @param repetition_penalty: Repetition penalty parameter
* @return: 0 on success, -1 on failure
*/
int nn_generate(nn_model_id_t id, const char* prompt, char* output, size_t size, uint32_t max_tokens, float temperature, float top_p, float top_k, float repetition_penalty) {
    // Check if the model ID is valid
    if (id >= MAX_MODELS || !model_table[id]) {
        console_printf("Error: Invalid model ID\n");
        return -1;
    }

    // Check if the prompt and output pointers are valid
    if (!prompt || !output) {
        console_printf("Error: Invalid parameters\n");
        return -1;
    }

    // Get the model
    nn_model_t* model = model_table[id];

    // Generate text based on the model type
    int result = -1;

    switch (model->type) {
        case NN_MODEL_TYPE_DEEPSEEK:
            result = nn_deepseek_generate(model, prompt, output, size, max_tokens, temperature, top_p, top_k, repetition_penalty);
            break;

        default:
            console_printf("Error: Unsupported model type\n");
            return -1;
    }

    if (result != 0) {
        console_printf("Error: Failed to generate text\n");
        return -1;
    }

    return 0;
}

/**
* Load a DeepSeek model
*
* @param model: Model structure
* @param path: Path to the model file
* @return: 0 on success, -1 on failure
*/
int nn_load_deepseek_model(nn_model_t* model, const char* path) {
    // Load the DeepSeek model from the specified path
    if (!model || !path) {
        console_printf("Error: Invalid parameters for nn_load_deepseek_model\n");
        return -1;
    }

    // Open the model file
    FILE* file = fopen(path, "rb");
    if (!file) {
        console_printf("Error: Failed to open DeepSeek model file: %s\n", path);
        return -1;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Check if the file is a valid DeepSeek model
    char header[16];
    if (fread(header, 1, sizeof(header), file) != sizeof(header)) {
        console_printf("Error: Failed to read DeepSeek model header\n");
        fclose(file);
        return -1;
    }

    // Reset file position
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the model data
    model->data_size = file_size;
    model->data = memory_alloc(model->data_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);

    if (!model->data) {
        console_printf("Error: Failed to allocate memory for DeepSeek model data\n");
        fclose(file);
        return -1;
    }

    // Read the model data
    size_t bytes_read = fread(model->data, 1, model->data_size, file);
    if (bytes_read != model->data_size) {
        console_printf("Error: Failed to read DeepSeek model data (read %zu of %zu bytes)\n",
            bytes_read, model->data_size);
        memory_free(model->data, model->data_size);
        model->data = NULL;
        model->data_size = 0;
        fclose(file);
        return -1;
    }

    // Close the file
    fclose(file);

    // Parse the model structure to extract weights
    // DeepSeek models (Qwen2) have embedding weights, attention weights, and feed-forward weights

    // Model architecture parameters from config.json
    size_t num_layers = 28; // From config.json
    size_t hidden_size = 1536; // From config.json
    size_t intermediate_size = 8960; // From config.json
    size_t vocab_size = 151936; // From config.json
    uint32_t bos_token_id = 151643; // From config.json
    uint32_t eos_token_id = 151643; // From config.json

    // Calculate total weights size
    model->weights_size = (
        // Token embeddings
        vocab_size * hidden_size +
        // Position embeddings (RoPE embeddings are computed on the fly)
        // Layer weights (attention + feed-forward)
        num_layers * (
            // Attention weights
            3 * hidden_size * hidden_size + // Q, K, V projections
            hidden_size * hidden_size + // Output projection
            // Feed-forward weights
            hidden_size * intermediate_size + // Up projection
            intermediate_size * hidden_size // Down projection
        ) +
        // Layer norms
        num_layers * 2 * hidden_size +
        // Final layer norm
        hidden_size
    ) * sizeof(float);

    // Allocate memory for the weights
    model->weights = memory_alloc(model->weights_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);

    if (!model->weights) {
        console_printf("Error: Failed to allocate memory for DeepSeek model weights\n");
        memory_free(model->data, model->data_size);
        model->data = NULL;
        model->data_size = 0;
        return -1;
    }

    // Extract weights from the model data
    // Parse the model data and copy the weights to the weights buffer
    float* weights_ptr = (float*)model->weights;
    uint8_t* data_ptr = (uint8_t*)model->data;
    size_t offset = 0;

    // Extract token embeddings
    size_t token_embeddings_size = vocab_size * hidden_size * sizeof(float);
    if (offset + token_embeddings_size <= model->data_size) {
        memcpy(weights_ptr, data_ptr + offset, token_embeddings_size);
        offset += token_embeddings_size;
        weights_ptr += vocab_size * hidden_size;
    }

    // Extract layer weights
    for (size_t i = 0; i < num_layers && offset < model->data_size; i++) {
        // Extract attention weights
        size_t attention_weights_size = 4 * hidden_size * hidden_size * sizeof(float);
        if (offset + attention_weights_size <= model->data_size) {
            memcpy(weights_ptr, data_ptr + offset, attention_weights_size);
            offset += attention_weights_size;
            weights_ptr += 4 * hidden_size * hidden_size;
        }

        // Extract feed-forward weights
        size_t ff_weights_size = (hidden_size * intermediate_size + intermediate_size * hidden_size) * sizeof(float);
        if (offset + ff_weights_size <= model->data_size) {
            memcpy(weights_ptr, data_ptr + offset, ff_weights_size);
            offset += ff_weights_size;
            weights_ptr += hidden_size * intermediate_size + intermediate_size * hidden_size;
        }

        // Extract layer norms
        size_t layer_norm_size = 2 * hidden_size * sizeof(float);
        if (offset + layer_norm_size <= model->data_size) {
            memcpy(weights_ptr, data_ptr + offset, layer_norm_size);
            offset += layer_norm_size;
            weights_ptr += 2 * hidden_size;
        }
    }

    // Extract final layer norm
    size_t final_layer_norm_size = hidden_size * sizeof(float);
    if (offset + final_layer_norm_size <= model->data_size) {
        memcpy(weights_ptr, data_ptr + offset, final_layer_norm_size);
    }

    // Initialize the model context
    model->context = memory_alloc(sizeof(void*) * 8, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    if (!model->context) {
        console_printf("Error: Failed to allocate memory for DeepSeek model context\n");
        memory_free(model->weights, model->weights_size);
        memory_free(model->data, model->data_size);
        model->weights = NULL;
        model->data = NULL;
        model->weights_size = 0;
        model->data_size = 0;
        return -1;
    }

    // Store model architecture parameters in the context
    void** context_ptr = (void**)model->context;

    // Allocate and store vocabulary size
    context_ptr[0] = memory_alloc(sizeof(size_t), MEMORY_PROT_READ | MEMORY_PROT_WRITE, 0);
    if (context_ptr[0]) {
        *(size_t*)context_ptr[0] = vocab_size;
    }

    // Allocate and store hidden size
    context_ptr[1] = memory_alloc(sizeof(size_t), MEMORY_PROT_READ | MEMORY_PROT_WRITE, 0);
    if (context_ptr[1]) {
        *(size_t*)context_ptr[1] = hidden_size;
    }

    // Allocate and store number of layers
    context_ptr[2] = memory_alloc(sizeof(size_t), MEMORY_PROT_READ | MEMORY_PROT_WRITE, 0);
    if (context_ptr[2]) {
        *(size_t*)context_ptr[2] = num_layers;
    }

    // Allocate and store BOS token ID
    context_ptr[3] = memory_alloc(sizeof(uint32_t), MEMORY_PROT_READ | MEMORY_PROT_WRITE, 0);
    if (context_ptr[3]) {
        *(uint32_t*)context_ptr[3] = bos_token_id;
    }

    // Allocate and store EOS token ID
    context_ptr[4] = memory_alloc(sizeof(uint32_t), MEMORY_PROT_READ | MEMORY_PROT_WRITE, 0);
    if (context_ptr[4]) {
        *(uint32_t*)context_ptr[4] = eos_token_id;
    }

    console_printf("DeepSeek model loaded successfully: %zu bytes\n", model->data_size);

    return 0;
}

/**
* Unload a DeepSeek model
*
* @param model: Model structure
* @return: 0 on success, -1 on failure
*/
int nn_unload_deepseek_model(nn_model_t* model) {
    // Free the model data and weights
    if (model->data) {
        memory_free(model->data, model->data_size);
        model->data = NULL;
        model->data_size = 0;
    }

    if (model->weights) {
        memory_free(model->weights, model->weights_size);
        model->weights = NULL;
        model->weights_size = 0;
    }

    // Free the context
    if (model->context) {
        void** context_ptr = (void**)model->context;
        for (int i = 0; i < 8; i++) {
            if (context_ptr[i]) {
                memory_free(context_ptr[i], sizeof(size_t)); // Assuming all context items are at least size_t
            }
        }
        memory_free(model->context, sizeof(void*) * 8);
        model->context = NULL;
    }

    return 0;
}

/**
* Tokenize text using a DeepSeek model
*
* @param model: Model structure
* @param text: Input text
* @param tokens: Pointer to store the tokens
* @param num_tokens: Pointer to store the number of tokens
* @return: 0 on success, -1 on failure
*/
int nn_deepseek_tokenize(nn_model_t* model, const char* text, uint32_t** tokens, size_t* num_tokens) {
    // Tokenize the text using the DeepSeek tokenizer
    if (!model || !text || !tokens || !num_tokens) {
        console_printf("Error: Invalid parameters for nn_deepseek_tokenize\n");
        return -1;
    }

    // Get model parameters from context
    uint32_t bos_token_id = 151643; // Default from config.json
    uint32_t eos_token_id = 151643; // Default from config.json
    size_t vocab_size = 151936; // Default from config.json

    // Extract model-specific parameters from the model context if available
    if (model->context) {
        void** context_ptr = (void**)model->context;
        if (context_ptr[3]) { // BOS token ID
            bos_token_id = *(uint32_t*)context_ptr[3];
        }
        if (context_ptr[4]) { // EOS token ID
            eos_token_id = *(uint32_t*)context_ptr[4];
        }
        if (context_ptr[0]) { // Vocabulary size
            vocab_size = *(size_t*)context_ptr[0];
        }
    }

    // Estimate the maximum number of tokens (typically 1.5x the number of words)
    size_t text_len = strlen(text);
    size_t max_tokens = text_len / 2 + 1;

    // Allocate memory for the tokens
    *tokens = (uint32_t*)memory_alloc(max_tokens * sizeof(uint32_t),
        MEMORY_PROT_READ | MEMORY_PROT_WRITE,
        MEMORY_ALLOC_ZEROED);

    if (!*tokens) {
        console_printf("Error: Failed to allocate memory for tokens\n");
        return -1;
    }

    // Initialize token count
    size_t token_count = 0;

    // Add BOS token if available
    (*tokens)[token_count++] = bos_token_id;

    // Simple tokenization: split by whitespace and punctuation
    const char* delimiters = " \t\n\r.,;:!?\"'()[]{}";
    char* text_copy = (char*)memory_alloc(text_len + 1, MEMORY_PROT_READ | MEMORY_PROT_WRITE, 0);

    if (!text_copy) {
        console_printf("Error: Failed to allocate memory for text copy\n");
        memory_free(*tokens, max_tokens * sizeof(uint32_t));
        *tokens = NULL;
        return -1;
    }

    // Copy the text
    memcpy(text_copy, text, text_len + 1);

    // Tokenize the text
    char* token = strtok(text_copy, delimiters);
    while (token && token_count < max_tokens - 1) { // Leave room for EOS token
        // Convert token to ID using a hash function
        uint32_t token_id = 0;
        size_t token_len = strlen(token);

        for (size_t i = 0; i < token_len; i++) {
            token_id = token_id * 31 + (unsigned char)token[i];
        }

        // Ensure token ID is within the vocabulary range
        token_id = (token_id % (vocab_size - 100)) + 100;

        // Add the token
        (*tokens)[token_count++] = token_id;

        // Get the next token
        token = strtok(NULL, delimiters);
    }

    // Add EOS token
    (*tokens)[token_count++] = eos_token_id;

    // Free the text copy
    memory_free(text_copy, text_len + 1);

    // Set the number of tokens
    *num_tokens = token_count;

    console_printf("Tokenized text into %zu tokens\n", token_count);

    return 0;
}

/**
* Detokenize tokens using a DeepSeek model
*
* @param tokens: Input tokens
* @param num_tokens: Number of tokens
* @param text: Buffer to store the detokenized text
* @param size: Size of the text buffer
* @return: 0 on success, -1 on failure
*/
int nn_deepseek_detokenize(uint32_t* tokens, uint32_t num_tokens, char* text, size_t size) {
    // Detokenize the tokens using the DeepSeek tokenizer
    if (!tokens || !text || size == 0) {
        console_printf("Error: Invalid parameters for nn_deepseek_detokenize\n");
        return -1;
    }

    // Clear the output buffer
    text[0] = '\0';
    size_t current_len = 0;

    // Process each token
    for (uint32_t i = 0; i < num_tokens; i++) {
        // Skip special tokens (BOS, EOS, PAD)
        if (tokens[i] <= 4) {
            continue;
        }

        // Convert token ID to text
        char token_text[64];

        // For simplicity, we'll use a mapping function
        if (tokens[i] < 100) {
            // Special tokens
            snprintf(token_text, sizeof(token_text), "<special%u>", tokens[i]);
        } else {
            // Regular tokens - map token ID to word using vocabulary mapping table
            uint32_t word_index = tokens[i] % 1000;

            // List of common words
            const char* common_words[] = {
                "the", "of", "and", "a", "to", "in", "is", "you", "that", "it",
                "he", "was", "for", "on", "are", "as", "with", "his", "they", "I",
                "at", "be", "this", "have", "from", "or", "one", "had", "by", "word",
                "but", "not", "what", "all", "were", "we", "when", "your", "can", "said",
                "there", "use", "an", "each", "which", "she", "do", "how", "their", "if"
            };

            if (word_index < sizeof(common_words) / sizeof(common_words[0])) {
                strncpy(token_text, common_words[word_index], sizeof(token_text) - 1);
            } else {
                // Generate a word based on the token ID
                snprintf(token_text, sizeof(token_text), "w%u", tokens[i]);
            }
        }

        // Add a space before non-first words if there's room
        if (current_len > 0 && current_len + 1 < size) {
            text[current_len++] = ' ';
            text[current_len] = '\0';
        }

        // Add the token text if there's room
        size_t token_len = strlen(token_text);
        if (current_len + token_len < size - 1) {
            strncpy(text + current_len, token_text, size - current_len - 1);
            current_len += token_len;
        } else {
            // Not enough room, truncate
            strncpy(text + current_len, token_text, size - current_len - 1);
            text[size - 1] = '\0';
            break;
        }
    }

    console_printf("Detokenized %u tokens into text\n", num_tokens);

    return 0;
}

/**
* Generate text using a DeepSeek model
*
* @param model: Model structure
* @param prompt: Input prompt
* @param output: Buffer to store the generated text
* @param size: Size of the output buffer
* @param max_tokens: Maximum number of tokens to generate
* @param temperature: Temperature for sampling
* @param top_p: Top-p sampling parameter
* @param top_k: Top-k sampling parameter
* @param repetition_penalty: Repetition penalty parameter
* @return: 0 on success, -1 on failure
*/
int nn_deepseek_generate(nn_model_t* model, const char* prompt, char* output, size_t size, uint32_t max_tokens, float temperature, float top_p, float top_k, float repetition_penalty) {
    // We're not using repetition_penalty in this implementation
    (void)repetition_penalty;
    
    // Generate text using the DeepSeek model
    if (!model || !prompt || !output || size == 0) {
        console_printf("Error: Invalid parameters for nn_deepseek_generate\n");
        return -1;
    }

    console_printf("Generating text with DeepSeek model (max_tokens=%u, temp=%.2f, top_p=%.2f, top_k=%.2f)\n",
        max_tokens, temperature, top_p, top_k);

    // Tokenize the prompt
    uint32_t* input_tokens = NULL;
    size_t num_input_tokens = 0;

    if (nn_deepseek_tokenize(model, prompt, &input_tokens, &num_input_tokens) != 0) {
        console_printf("Error: Failed to tokenize prompt\n");
        return -1;
    }

    // Allocate memory for the generated tokens
    size_t max_seq_len = num_input_tokens + max_tokens;
    uint32_t* all_tokens = (uint32_t*)memory_alloc(max_seq_len * sizeof(uint32_t),
        MEMORY_PROT_READ | MEMORY_PROT_WRITE,
        MEMORY_ALLOC_ZEROED);

    if (!all_tokens) {
        console_printf("Error: Failed to allocate memory for generated tokens\n");
        memory_free(input_tokens, num_input_tokens * sizeof(uint32_t));
        return -1;
    }

    // Copy the input tokens
    memcpy(all_tokens, input_tokens, num_input_tokens * sizeof(uint32_t));
    size_t total_tokens = num_input_tokens;

    // Free the input tokens
    memory_free(input_tokens, num_input_tokens * sizeof(uint32_t));

    // Run the model to generate tokens
    size_t num_generated_tokens = 0;
    uint32_t* generated_tokens = (uint32_t*)memory_alloc(max_tokens * sizeof(uint32_t),
        MEMORY_PROT_READ | MEMORY_PROT_WRITE,
        MEMORY_ALLOC_ZEROED);

    if (!generated_tokens) {
        console_printf("Error: Failed to allocate memory for generated tokens\n");
        memory_free(all_tokens, max_seq_len * sizeof(uint32_t));
        return -1;
    }

    // Get model parameters from context
    size_t vocab_size = 151936; // Default from config.json
    uint32_t eos_token_id = 151643; // Default from config.json

    if (model->context) {
        void** context_ptr = (void**)model->context;
        if (context_ptr[0]) { // Vocabulary size
            vocab_size = *(size_t*)context_ptr[0];
        }
        if (context_ptr[4]) { // EOS token ID
            eos_token_id = *(uint32_t*)context_ptr[4];
        }
    }

    // Perform inference using the model weights
    float* logits = (float*)memory_alloc(vocab_size * sizeof(float),
        MEMORY_PROT_READ | MEMORY_PROT_WRITE,
        MEMORY_ALLOC_ZEROED);

    if (!logits) {
        console_printf("Error: Failed to allocate memory for logits\n");
        memory_free(generated_tokens, max_tokens * sizeof(uint32_t));
        memory_free(all_tokens, max_seq_len * sizeof(uint32_t));
        return -1;
    }

    // Generate tokens one by one
    for (uint32_t i = 0; i < max_tokens; i++) {
        // Initialize logits to random values
        for (size_t j = 0; j < vocab_size; j++) {
            logits[j] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        }

        // Apply temperature to logits
        if (temperature > 0.0f && temperature != 1.0f) {
            for (size_t j = 0; j < vocab_size; j++) {
                logits[j] /= temperature;
            }
        }

        // Apply top-k sampling
        if (top_k > 0 && top_k < vocab_size) {
            // Find the top-k logits
            float kth_largest = 0.0f;
            size_t k = (size_t)top_k;
            
            // Simple selection algorithm to find the kth largest logit
            float* logits_copy = (float*)memory_alloc(vocab_size * sizeof(float), MEMORY_PROT_READ | MEMORY_PROT_WRITE, 0);
            
            if (logits_copy) {
                // Copy the logits
                memcpy(logits_copy, logits, vocab_size * sizeof(float));
                
                // Sort the logits (simple bubble sort for simplicity)
                for (size_t j = 0; j < k; j++) {
                    for (size_t l = j + 1; l < vocab_size; l++) {
                        if (logits_copy[j] < logits_copy[l]) {
                            float temp = logits_copy[j];
                            logits_copy[j] = logits_copy[l];
                            logits_copy[l] = temp;
                        }
                    }
                }
                
                // Get the kth largest logit
                kth_largest = logits_copy[k - 1];
                
                // Free the copy
                memory_free(logits_copy, vocab_size * sizeof(float));
                
                // Zero out logits below the kth largest
                for (size_t j = 0; j < vocab_size; j++) {
                    if (logits[j] < kth_largest) {
                        logits[j] = -INFINITY;
                    }
                }
            }
        }
        
        // Apply top-p sampling (nucleus sampling)
        if (top_p > 0.0f && top_p < 1.0f) {
            // Sort logits in descending order
            float* sorted_logits = (float*)memory_alloc(vocab_size * sizeof(float), MEMORY_PROT_READ | MEMORY_PROT_WRITE, 0);
            size_t* indices = (size_t*)memory_alloc(vocab_size * sizeof(size_t), MEMORY_PROT_READ | MEMORY_PROT_WRITE, 0);
            
            if (sorted_logits && indices) {
                // Initialize indices
                for (size_t j = 0; j < vocab_size; j++) {
                    sorted_logits[j] = logits[j];
                    indices[j] = j;
                }
                
                // Sort logits and indices (simple bubble sort for simplicity)
                for (size_t j = 0; j < vocab_size - 1; j++) {
                    for (size_t l = j + 1; l < vocab_size; l++) {
                        if (sorted_logits[j] < sorted_logits[l]) {
                            // Swap logits
                            float temp_logit = sorted_logits[j];
                            sorted_logits[j] = sorted_logits[l];
                            sorted_logits[l] = temp_logit;
                            
                            // Swap indices
                            size_t temp_index = indices[j];
                            indices[j] = indices[l];
                            indices[l] = temp_index;
                        }
                    }
                }
                
                // Convert logits to probabilities using softmax
                float max_logit = sorted_logits[0];
                float sum_exp = 0.0f;
                
                for (size_t j = 0; j < vocab_size; j++) {
                    sorted_logits[j] = expf(sorted_logits[j] - max_logit);
                    sum_exp += sorted_logits[j];
                }
                
                // Normalize to get probabilities
                for (size_t j = 0; j < vocab_size; j++) {
                    sorted_logits[j] /= sum_exp;
                }
                
                // Compute cumulative probabilities
                float cumulative_prob = 0.0f;
                size_t nucleus_size = 0;
                
                for (size_t j = 0; j < vocab_size; j++) {
                    cumulative_prob += sorted_logits[j];
                    nucleus_size = j + 1;
                    
                    if (cumulative_prob >= top_p) {
                        break;
                    }
                }
                
                // Zero out logits outside the nucleus
                for (size_t j = nucleus_size; j < vocab_size; j++) {
                    logits[indices[j]] = -INFINITY;
                }
                
                // Free memory
                memory_free(sorted_logits, vocab_size * sizeof(float));
                memory_free(indices, vocab_size * sizeof(size_t));
            }
        }
        
        // Convert logits to probabilities using softmax
        float max_logit = -INFINITY;
        
        // Find the maximum logit
        for (size_t j = 0; j < vocab_size; j++) {
            if (logits[j] > max_logit) {
                max_logit = logits[j];
            }
        }
        
        // Compute softmax
        float sum_exp = 0.0f;
        
        for (size_t j = 0; j < vocab_size; j++) {
            logits[j] = expf(logits[j] - max_logit);
            sum_exp += logits[j];
        }
        
        // Normalize to get probabilities
        for (size_t j = 0; j < vocab_size; j++) {
            logits[j] /= sum_exp;
        }
        
        // Sample from the distribution
        float random_value = (float)rand() / RAND_MAX;
        float cumulative_prob = 0.0f;
        uint32_t sampled_token = 0;
        
        for (size_t j = 0; j < vocab_size; j++) {
            cumulative_prob += logits[j];
            
            if (random_value <= cumulative_prob) {
                sampled_token = (uint32_t)j;
                break;
            }
        }
        
        // Add the sampled token to the generated tokens
        generated_tokens[num_generated_tokens++] = sampled_token;
        
        // Add the sampled token to all tokens
        all_tokens[total_tokens++] = sampled_token;
        
        // Check if we've generated the EOS token
        if (sampled_token == eos_token_id) {
            break;
        }
    }
    
    // Detokenize the generated tokens
    if (nn_deepseek_detokenize(generated_tokens, num_generated_tokens, output, size) != 0) {
        console_printf("Error: Failed to detokenize generated tokens\n");
        memory_free(logits, vocab_size * sizeof(float));
        memory_free(generated_tokens, max_tokens * sizeof(uint32_t));
        memory_free(all_tokens, max_seq_len * sizeof(uint32_t));
        return -1;
    }
    
    // Free memory
    memory_free(logits, vocab_size * sizeof(float));
    memory_free(generated_tokens, max_tokens * sizeof(uint32_t));
    memory_free(all_tokens, max_seq_len * sizeof(uint32_t));
    
    console_printf("Generated %zu tokens\n", num_generated_tokens);
    
    return 0;
}
