/**
 * ai_interface.h - AI integration interface for NeuroOS
 * 
 * This file defines the interface for the AI integration subsystem, which is
 * responsible for interacting with the Deepseek R1 model and providing
 * AI-driven functionality to the operating system.
 */

#ifndef NEUROOS_AI_INTERFACE_H
#define NEUROOS_AI_INTERFACE_H

#include <stdint.h>
#include <stddef.h>
#include "sandbox.h"

// AI model types
typedef enum {
    AI_MODEL_TYPE_DEEPSEEK_R1,
    AI_MODEL_TYPE_CUSTOM
} ai_model_type_t;

// AI task types
typedef enum {
    AI_TASK_CODE_GENERATION,
    AI_TASK_CODE_OPTIMIZATION,
    AI_TASK_CODE_ANALYSIS,
    AI_TASK_SYSTEM_MONITORING,
    AI_TASK_SYSTEM_OPTIMIZATION,
    AI_TASK_NETWORK_ANALYSIS,
    AI_TASK_NATURAL_LANGUAGE_PROCESSING,
    AI_TASK_NLP = AI_TASK_NATURAL_LANGUAGE_PROCESSING, // Alias for convenience
    AI_TASK_CUSTOM
} ai_task_type_t;

// AI task priorities
typedef enum {
    AI_TASK_PRIORITY_LOW,
    AI_TASK_PRIORITY_NORMAL,
    AI_TASK_PRIORITY_HIGH,
    AI_TASK_PRIORITY_CRITICAL
} ai_task_priority_t;

// AI task states
typedef enum {
    AI_TASK_STATE_CREATED,
    AI_TASK_STATE_QUEUED,
    AI_TASK_STATE_RUNNING,
    AI_TASK_STATE_COMPLETED,
    AI_TASK_STATE_FAILED,
    AI_TASK_STATE_CANCELLED
} ai_task_state_t;

// AI task flags
typedef enum {
    AI_TASK_FLAG_NONE = 0,
    AI_TASK_FLAG_SANDBOX = (1 << 0),
    AI_TASK_FLAG_BACKGROUND = (1 << 1),
    AI_TASK_FLAG_PERSISTENT = (1 << 2),
    AI_TASK_FLAG_SYSTEM = (1 << 3),
    AI_TASK_FLAG_USER = (1 << 4),
    AI_TASK_FLAG_NETWORK = (1 << 5),
    // NLP specific task flags
    AI_TASK_FLAG_SENTIMENT_ANALYSIS = (1 << 8),
    AI_TASK_FLAG_ENTITY_RECOGNITION = (1 << 9),
    AI_TASK_FLAG_SUMMARIZATION = (1 << 10),
    AI_TASK_FLAG_TRANSLATION = (1 << 11),
    AI_TASK_FLAG_QUESTION_ANSWERING = (1 << 12)
} ai_task_flags_t;

// AI model configuration
typedef struct {
    ai_model_type_t type;
    char path[256];
    size_t memory_limit;
    int use_gpu;
    int num_threads;
    float temperature;
    int max_tokens;
    int top_k;
    float top_p;
    float repetition_penalty;
} ai_model_config_t;

// AI task ID type
typedef uint32_t ai_task_id_t;

// AI task structure
typedef struct {
    ai_task_id_t id;
    ai_task_type_t type;
    ai_task_priority_t priority;
    ai_task_state_t state;
    ai_task_flags_t flags;
    char name[64];
    char description[256];
    void* input_data;
    size_t input_size;
    void* output_data;
    size_t output_size;
    uint64_t creation_time;
    uint64_t start_time;
    uint64_t completion_time;
    sandbox_id_t sandbox_id;
    int exit_code;
    char error_message[256];
} ai_task_t;

// AI health metrics
typedef struct {
    float confidence;
    float coherence;
    float stability;
    float response_time;
    float memory_usage;
    float cpu_usage;
    float gpu_usage;
    int hallucination_score;
    int error_count;
    int warning_count;
} ai_health_metrics_t;

// AI text generation parameters
typedef struct {
    int max_tokens;
    float temperature;
    float top_p;
    int top_k;
    float repetition_penalty;
    const char** stop_tokens;
    int num_stop_tokens;
} ai_generation_params_t;

/**
 * Initialize the AI interface
 * 
 * This function initializes the AI interface and loads the specified model.
 * 
 * @param config: AI model configuration
 * @return: 0 on success, -1 on failure
 */
int ai_init(const ai_model_config_t* config);

/**
 * Shutdown the AI interface
 * 
 * This function shuts down the AI interface and unloads the model.
 * 
 * @return: 0 on success, -1 on failure
 */
int ai_shutdown(void);

/**
 * Create an AI task
 * 
 * This function creates a new AI task with the specified parameters.
 * 
 * @param type: Task type
 * @param name: Task name
 * @param description: Task description
 * @param priority: Task priority
 * @param flags: Task flags
 * @param input_data: Input data for the task
 * @param input_size: Size of the input data
 * @return: Task ID on success, 0 on failure
 */
ai_task_id_t ai_create_task(ai_task_type_t type, const char* name, const char* description,
                          ai_task_priority_t priority, ai_task_flags_t flags,
                          const void* input_data, size_t input_size);

/**
 * Start an AI task
 * 
 * This function starts the specified AI task.
 * 
 * @param task_id: Task ID
 * @return: 0 on success, -1 on failure
 */
int ai_start_task(ai_task_id_t task_id);

/**
 * Cancel an AI task
 * 
 * This function cancels the specified AI task.
 * 
 * @param task_id: Task ID
 * @return: 0 on success, -1 on failure
 */
int ai_cancel_task(ai_task_id_t task_id);

/**
 * Get AI task information
 * 
 * This function gets information about the specified AI task.
 * 
 * @param task_id: Task ID
 * @param task: Pointer to store the task information
 * @return: 0 on success, -1 on failure
 */
int ai_get_task_info(ai_task_id_t task_id, ai_task_t* task);

/**
 * Get AI task result
 * 
 * This function gets the result of the specified AI task.
 * 
 * @param task_id: Task ID
 * @param output_data: Buffer to store the output data
 * @param output_size: Size of the output buffer
 * @param actual_output_size: Pointer to store the actual output size
 * @return: 0 on success, -1 on failure
 */
int ai_get_task_result(ai_task_id_t task_id, void* output_data, size_t output_size,
                     size_t* actual_output_size);

/**
 * Wait for an AI task to complete
 * 
 * This function waits for the specified AI task to complete.
 * 
 * @param task_id: Task ID
 * @param timeout_ms: Timeout in milliseconds (0 for infinite)
 * @return: 0 on success, -1 on failure or timeout
 */
int ai_wait_for_task(ai_task_id_t task_id, uint64_t timeout_ms);

/**
 * Generate code using the AI model
 * 
 * This function generates code using the AI model based on the specified prompt.
 * 
 * @param prompt: Code generation prompt
 * @param language: Programming language
 * @param flags: Task flags
 * @param output: Buffer to store the generated code
 * @param output_size: Size of the output buffer
 * @param actual_output_size: Pointer to store the actual output size
 * @return: 0 on success, -1 on failure
 */
int ai_generate_code(const char* prompt, const char* language, ai_task_flags_t flags,
                   char* output, size_t output_size, size_t* actual_output_size);

/**
 * Optimize code using the AI model
 * 
 * This function optimizes the specified code using the AI model.
 * 
 * @param code: Code to optimize
 * @param language: Programming language
 * @param optimization_level: Optimization level (1-5)
 * @param flags: Task flags
 * @param output: Buffer to store the optimized code
 * @param output_size: Size of the output buffer
 * @param actual_output_size: Pointer to store the actual output size
 * @return: 0 on success, -1 on failure
 */
int ai_optimize_code(const char* code, const char* language, int optimization_level,
                   ai_task_flags_t flags, char* output, size_t output_size,
                   size_t* actual_output_size);

/**
 * Analyze code using the AI model
 * 
 * This function analyzes the specified code using the AI model.
 * 
 * @param code: Code to analyze
 * @param language: Programming language
 * @param flags: Task flags
 * @param output: Buffer to store the analysis results
 * @param output_size: Size of the output buffer
 * @param actual_output_size: Pointer to store the actual output size
 * @return: 0 on success, -1 on failure
 */
int ai_analyze_code(const char* code, const char* language, ai_task_flags_t flags,
                  char* output, size_t output_size, size_t* actual_output_size);

/**
 * Execute code generated by the AI model
 * 
 * This function executes code generated by the AI model in a sandbox.
 * 
 * @param code: Code to execute
 * @param language: Programming language
 * @param sandbox_flags: Sandbox flags
 * @param output: Buffer to store the execution results
 * @param output_size: Size of the output buffer
 * @param actual_output_size: Pointer to store the actual output size
 * @return: 0 on success, -1 on failure
 */
int ai_execute_code(const char* code, const char* language, sandbox_flags_t sandbox_flags,
                  char* output, size_t output_size, size_t* actual_output_size);

/**
 * Process natural language using the AI model
 * 
 * This function processes natural language using the AI model.
 * 
 * @param input: Input text
 * @param flags: Task flags
 * @param output: Buffer to store the processed text
 * @param output_size: Size of the output buffer
 * @param actual_output_size: Pointer to store the actual output size
 * @return: 0 on success, -1 on failure
 */
int ai_process_natural_language(const char* input, ai_task_flags_t flags,
                              char* output, size_t output_size, size_t* actual_output_size);

/**
 * Monitor system health using the AI model
 * 
 * This function monitors system health using the AI model.
 * 
 * @param metrics: Buffer to store the health metrics
 * @return: 0 on success, -1 on failure
 */
int ai_monitor_system_health(ai_health_metrics_t* metrics);

/**
 * Optimize system performance using the AI model
 * 
 * This function optimizes system performance using the AI model.
 * 
 * @param target_subsystem: Target subsystem to optimize (NULL for all)
 * @param optimization_level: Optimization level (1-5)
 * @param flags: Task flags
 * @param output: Buffer to store the optimization results
 * @param output_size: Size of the output buffer
 * @param actual_output_size: Pointer to store the actual output size
 * @return: 0 on success, -1 on failure
 */
int ai_optimize_system(const char* target_subsystem, int optimization_level,
                     ai_task_flags_t flags, char* output, size_t output_size,
                     size_t* actual_output_size);

/**
 * Analyze network traffic using the AI model
 * 
 * This function analyzes network traffic using the AI model.
 * 
 * @param interface_name: Network interface name (NULL for all)
 * @param duration_ms: Analysis duration in milliseconds
 * @param flags: Task flags
 * @param output: Buffer to store the analysis results
 * @param output_size: Size of the output buffer
 * @param actual_output_size: Pointer to store the actual output size
 * @return: 0 on success, -1 on failure
 */
int ai_analyze_network(const char* interface_name, uint64_t duration_ms,
                     ai_task_flags_t flags, char* output, size_t output_size,
                     size_t* actual_output_size);

/**
 * Get AI model information
 * 
 * This function gets information about the loaded AI model.
 * 
 * @param config: Pointer to store the model configuration
 * @return: 0 on success, -1 on failure
 */
int ai_get_model_info(ai_model_config_t* config);

/**
 * Set AI model parameters
 * 
 * This function sets parameters for the loaded AI model.
 * 
 * @param temperature: Temperature parameter (0.0-1.0)
 * @param max_tokens: Maximum number of tokens to generate
 * @param top_k: Top-k sampling parameter
 * @param top_p: Top-p sampling parameter
 * @param repetition_penalty: Repetition penalty parameter
 * @return: 0 on success, -1 on failure
 */
int ai_set_model_parameters(float temperature, int max_tokens, int top_k,
                          float top_p, float repetition_penalty);

/**
 * Check AI health
 * 
 * This function checks the health of the AI system.
 * 
 * @param metrics: Pointer to store the health metrics
 * @return: 0 on success, -1 on failure
 */
int ai_check_health(ai_health_metrics_t* metrics);

/**
 * Reset AI system
 * 
 * This function resets the AI system in case of issues.
 * 
 * @return: 0 on success, -1 on failure
 */
int ai_reset(void);

/**
 * Update AI model
 * 
 * This function updates the AI model from the specified source.
 * 
 * @param source_url: URL to download the model from (NULL to use default)
 * @return: 0 on success, -1 on failure
 */
int ai_update_model(const char* source_url);

/**
 * Save AI model state
 * 
 * This function saves the current state of the AI model.
 * 
 * @param filename: Output filename
 * @return: 0 on success, -1 on failure
 */
int ai_save_state(const char* filename);

/**
 * Load AI model state
 * 
 * This function loads a previously saved AI model state.
 * 
 * @param filename: Input filename
 * @return: 0 on success, -1 on failure
 */
int ai_load_state(const char* filename);

/**
 * Generate text using the AI model
 * 
 * This function generates text using the AI model based on the specified prompt.
 * 
 * @param prompt: Text generation prompt
 * @param output: Buffer to store the generated text
 * @param output_size: Size of the output buffer
 * @param actual_output_size: Pointer to store the actual output size
 * @param params: Text generation parameters
 * @return: 0 on success, -1 on failure
 */
int ai_generate_text(const char* prompt, char* output, size_t output_size, 
                    size_t* actual_output_size, const ai_generation_params_t* params);

#endif /* NEUROOS_AI_INTERFACE_H */
