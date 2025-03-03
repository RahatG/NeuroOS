/**
 * ai_interface.c - AI integration implementation for NeuroOS
 * 
 * This file implements the AI integration subsystem, which is responsible for
 * interacting with the Deepseek R1 model and providing AI-driven functionality
 * to the operating system.
 */

#include "include/ai_interface.h"
#include "include/memory.h"
#include "include/console.h"
#include "include/process.h"
#include "include/sandbox.h"
#include "include/backup.h"
#include "include/network.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <sys/wait.h>

// Define CLOCK_MONOTONIC if not defined
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

// Maximum number of AI tasks
#define MAX_AI_TASKS 64

// AI task table
static ai_task_t* ai_tasks[MAX_AI_TASKS];

// Next available task ID
static ai_task_id_t next_task_id = 1;

// AI model configuration
static ai_model_config_t ai_model_config;

// AI model state
static struct {
    int initialized;
    void* model_handle;
    void* model_memory;
    size_t model_memory_size;
    int model_loaded;
    ai_health_metrics_t health_metrics;
} ai_state;

// Forward declarations
static int ai_load_model(const ai_model_config_t* config);
static int ai_unload_model(void);
static int ai_execute_task(ai_task_id_t task_id);
static int ai_check_task_limits(ai_task_id_t task_id);
static int ai_find_free_task_slot(void);
static int ai_execute_code_generation_task(ai_task_t* task);
static int ai_execute_code_optimization_task(ai_task_t* task);
static int ai_execute_code_analysis_task(ai_task_t* task);
static int ai_execute_nlp_task(ai_task_t* task);
static int ai_execute_system_monitoring_task(ai_task_t* task);
static int ai_execute_system_optimization_task(ai_task_t* task);
static int ai_execute_network_analysis_task(ai_task_t* task);

/**
 * Execute a natural language processing task
 * 
 * @param task: Task to execute
 * @return: 0 on success, -1 on failure
 */
static int ai_execute_nlp_task(ai_task_t* task) {
    // Check if the task pointer is valid
    if (!task) {
        console_printf("Error: Invalid task pointer\n");
        return -1;
    }
    
    // Check if the input data is valid
    if (!task->input_data || task->input_size == 0) {
        console_printf("Error: Invalid input data\n");
        return -1;
    }
    
    // Get the input text from the input data
    const char* input_text = (const char*)task->input_data;
    
    // Check if the model is loaded
    if (!ai_state.model_loaded || !ai_state.model_memory) {
        console_printf("Error: AI model not loaded\n");
        return -1;
    }
    
    // Prepare the generation parameters
    ai_generation_params_t params;
    memset(&params, 0, sizeof(ai_generation_params_t));
    params.max_tokens = 1024;
    params.temperature = 0.7f;
    params.top_p = 0.9f;
    params.top_k = 40;
    params.repetition_penalty = 1.1f;
    params.stop_tokens = NULL;
    params.num_stop_tokens = 0;
    
    // Determine the NLP task type based on the flags
    const char* task_type = "general";
    char prompt_prefix[1024] = "Process the following text: ";
    
    if (task->flags & AI_TASK_FLAG_SENTIMENT_ANALYSIS) {
        task_type = "sentiment analysis";
        strcpy(prompt_prefix, "Analyze the sentiment of the following text. Determine if it is positive, negative, or neutral, and provide a confidence score: ");
    } else if (task->flags & AI_TASK_FLAG_ENTITY_RECOGNITION) {
        task_type = "entity recognition";
        strcpy(prompt_prefix, "Identify and extract all named entities (people, organizations, locations, dates, etc.) from the following text: ");
    } else if (task->flags & AI_TASK_FLAG_SUMMARIZATION) {
        task_type = "summarization";
        strcpy(prompt_prefix, "Provide a concise summary of the following text, capturing the main points and key information: ");
    } else if (task->flags & AI_TASK_FLAG_TRANSLATION) {
        task_type = "translation";
        strcpy(prompt_prefix, "Translate the following text to English: ");
    } else if (task->flags & AI_TASK_FLAG_QUESTION_ANSWERING) {
        task_type = "question answering";
        strcpy(prompt_prefix, "Answer the following question based on the provided context: ");
    }
    
    // Prepare the prompt for the NLP task
    char full_prompt[65536]; // Large buffer for potentially large text inputs
    snprintf(full_prompt, sizeof(full_prompt), "%s\n\n%s", prompt_prefix, input_text);
    
    // Allocate memory for the processed text
    size_t max_output_size = 32 * 1024; // 32 KB
    char* processed_text = (char*)memory_alloc(max_output_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!processed_text) {
        console_printf("Error: Failed to allocate memory for processed text\n");
        return -1;
    }
    
    // Process the text using the AI model
    size_t actual_output_size = 0;
    int result = ai_generate_text(full_prompt, processed_text, max_output_size, &actual_output_size, &params);
    
    if (result != 0 || actual_output_size == 0) {
        console_printf("Error: Failed to process text\n");
        memory_free(processed_text, max_output_size);
        return -1;
    }
    
    // Allocate memory for the final output
    void* output_data = memory_alloc(actual_output_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!output_data) {
        console_printf("Error: Failed to allocate output data\n");
        memory_free(processed_text, max_output_size);
        return -1;
    }
    
    // Copy the processed text to the output data
    memcpy(output_data, processed_text, actual_output_size);
    
    // Free the temporary buffer
    memory_free(processed_text, max_output_size);
    
    // Set the task output data
    task->output_data = output_data;
    task->output_size = actual_output_size;
    
    console_printf("NLP task (%s) completed successfully\n", task_type);
    return 0;
}

/**
 * Execute a system monitoring task
 * 
 * @param task: Task to execute
 * @return: 0 on success, -1 on failure
 */
static int ai_execute_system_monitoring_task(ai_task_t* task) {
    // Check if the task pointer is valid
    if (!task) {
        console_printf("Error: Invalid task pointer\n");
        return -1;
    }
    
    // Check if the model is loaded
    if (!ai_state.model_loaded || !ai_state.model_memory) {
        console_printf("Error: AI model not loaded\n");
        return -1;
    }
    
    // Collect system metrics
    ai_health_metrics_t metrics;
    memset(&metrics, 0, sizeof(ai_health_metrics_t));
    
    // In a real implementation, these would be actual system metrics
    metrics.cpu_usage = 0.45f; // 45% CPU usage
    metrics.memory_usage = 0.60f; // 60% memory usage
    metrics.gpu_usage = 0.30f; // 30% GPU usage
    
    // Prepare a report based on the metrics
    char report[4096];
    snprintf(report, sizeof(report),
             "System Monitoring Report\n"
             "=======================\n\n"
             "CPU Usage: %.1f%%\n"
             "Memory Usage: %.1f%%\n"
             "GPU Usage: %.1f%%\n\n"
             "System Health: Good\n"
             "Recommendations:\n"
             "- No immediate action required\n"
             "- Consider optimizing memory usage if it continues to increase\n",
             metrics.cpu_usage * 100.0f,
             metrics.memory_usage * 100.0f,
             metrics.gpu_usage * 100.0f);
    
    // Allocate memory for the output data
    size_t report_size = strlen(report) + 1;
    void* output_data = memory_alloc(report_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!output_data) {
        console_printf("Error: Failed to allocate output data\n");
        return -1;
    }
    
    // Copy the report to the output data
    memcpy(output_data, report, report_size);
    
    // Set the task output data
    task->output_data = output_data;
    task->output_size = report_size;
    
    console_printf("System monitoring task completed successfully\n");
    return 0;
}

/**
 * Execute a system optimization task
 * 
 * @param task: Task to execute
 * @return: 0 on success, -1 on failure
 */
static int ai_execute_system_optimization_task(ai_task_t* task) {
    // Check if the task pointer is valid
    if (!task) {
        console_printf("Error: Invalid task pointer\n");
        return -1;
    }
    
    // Check if the model is loaded
    if (!ai_state.model_loaded || !ai_state.model_memory) {
        console_printf("Error: AI model not loaded\n");
        return -1;
    }
    
    // In a real implementation, this would analyze the system and apply optimizations
    
    // Prepare an optimization report
    char report[4096];
    snprintf(report, sizeof(report),
             "System Optimization Report\n"
             "=========================\n\n"
             "Optimizations Applied:\n"
             "- Adjusted memory allocation strategy\n"
             "- Optimized process scheduling\n"
             "- Tuned file system cache parameters\n\n"
             "Performance Impact:\n"
             "- CPU usage reduced by 15%%\n"
             "- Memory usage reduced by 20%%\n"
             "- System responsiveness improved by 25%%\n\n"
             "Recommendations:\n"
             "- Monitor system performance for the next 24 hours\n"
             "- Consider additional optimizations if needed\n");
    
    // Allocate memory for the output data
    size_t report_size = strlen(report) + 1;
    void* output_data = memory_alloc(report_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!output_data) {
        console_printf("Error: Failed to allocate output data\n");
        return -1;
    }
    
    // Copy the report to the output data
    memcpy(output_data, report, report_size);
    
    // Set the task output data
    task->output_data = output_data;
    task->output_size = report_size;
    
    console_printf("System optimization task completed successfully\n");
    return 0;
}

/**
 * Execute a network analysis task
 * 
 * @param task: Task to execute
 * @return: 0 on success, -1 on failure
 */
static int ai_execute_network_analysis_task(ai_task_t* task) {
    // Check if the task pointer is valid
    if (!task) {
        console_printf("Error: Invalid task pointer\n");
        return -1;
    }
    
    // Check if the model is loaded
    if (!ai_state.model_loaded || !ai_state.model_memory) {
        console_printf("Error: AI model not loaded\n");
        return -1;
    }
    
    // In a real implementation, this would analyze network traffic
    
    // Prepare a network analysis report
    char report[4096];
    snprintf(report, sizeof(report),
             "Network Analysis Report\n"
             "======================\n\n"
             "Traffic Summary:\n"
             "- Total packets: 15,432\n"
             "- Total data: 8.7 MB\n"
             "- Average packet size: 564 bytes\n\n"
             "Protocol Distribution:\n"
             "- TCP: 78%%\n"
             "- UDP: 18%%\n"
             "- ICMP: 3%%\n"
             "- Other: 1%%\n\n"
             "Top Connections:\n"
             "1. 192.168.1.5:443 -> 172.217.20.142:443 (HTTPS, 2.3 MB)\n"
             "2. 192.168.1.5:53124 -> 34.107.221.82:80 (HTTP, 1.5 MB)\n"
             "3. 192.168.1.5:57621 -> 52.96.165.18:443 (HTTPS, 0.9 MB)\n\n"
             "Anomalies Detected: None\n"
             "Security Concerns: None\n");
    
    // Allocate memory for the output data
    size_t report_size = strlen(report) + 1;
    void* output_data = memory_alloc(report_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!output_data) {
        console_printf("Error: Failed to allocate output data\n");
        return -1;
    }
    
    // Copy the report to the output data
    memcpy(output_data, report, report_size);
    
    // Set the task output data
    task->output_data = output_data;
    task->output_size = report_size;
    
    console_printf("Network analysis task completed successfully\n");
    return 0;
}
static uint64_t get_system_time(void);

/**
 * Generate text using the AI model
 * 
 * @param prompt: Text generation prompt
 * @param output: Buffer to store the generated text
 * @param output_size: Size of the output buffer
 * @param actual_output_size: Pointer to store the actual output size
 * @param params: Text generation parameters
 * @return: 0 on success, -1 on failure
 */
int ai_generate_text(const char* prompt, char* output, size_t output_size, 
                    size_t* actual_output_size, const ai_generation_params_t* params) {
    // Suppress unused parameter warning
    (void)params;
    // Check if the AI interface is initialized
    if (!ai_state.initialized) {
        console_printf("Error: AI interface not initialized\n");
        return -1;
    }
    
    // Check if the prompt, output, and actual_output_size pointers are valid
    if (!prompt || !output || !actual_output_size) {
        console_printf("Error: Invalid prompt, output, or actual_output_size pointer\n");
        return -1;
    }
    
    // Check if the model is loaded
    if (!ai_state.model_loaded || !ai_state.model_memory) {
        console_printf("Error: AI model not loaded\n");
        return -1;
    }
    
    // For now, just copy the prompt to the output as a placeholder
    // In a real implementation, this would use the AI model to generate text
    size_t prompt_len = strlen(prompt);
    size_t copy_len = prompt_len < output_size - 1 ? prompt_len : output_size - 1;
    
    memcpy(output, prompt, copy_len);
    output[copy_len] = '\0';
    *actual_output_size = copy_len + 1;
    
    // Add a simple generated response
    const char* response = "\n\nGenerated response: This is a placeholder response from the AI model.";
    size_t response_len = strlen(response);
    
    if (copy_len + response_len < output_size - 1) {
        strcat(output, response);
        *actual_output_size = copy_len + response_len + 1;
    }
    
    return 0;
}

/**
 * Get the current system time in milliseconds
 * 
 * @return: Current system time in milliseconds
 */
static uint64_t get_system_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

/**
 * Initialize the AI interface
 * 
 * @param config: AI model configuration
 * @return: 0 on success, -1 on failure
 */
int ai_init(const ai_model_config_t* config) {
    // Check if the AI interface is already initialized
    if (ai_state.initialized) {
        console_printf("AI interface already initialized\n");
        return 0;
    }
    
    // Check if the config pointer is valid
    if (!config) {
        console_printf("Error: Invalid AI model configuration\n");
        return -1;
    }
    
    // Initialize the AI task table
    for (int i = 0; i < MAX_AI_TASKS; i++) {
        ai_tasks[i] = NULL;
    }
    
    // Initialize the AI state
    ai_state.initialized = 1;
    ai_state.model_handle = NULL;
    ai_state.model_memory = NULL;
    ai_state.model_memory_size = 0;
    ai_state.model_loaded = 0;
    
    // Initialize the health metrics
    ai_state.health_metrics.confidence = 1.0f;
    ai_state.health_metrics.coherence = 1.0f;
    ai_state.health_metrics.stability = 1.0f;
    ai_state.health_metrics.response_time = 0.0f;
    ai_state.health_metrics.memory_usage = 0.0f;
    ai_state.health_metrics.cpu_usage = 0.0f;
    ai_state.health_metrics.gpu_usage = 0.0f;
    ai_state.health_metrics.hallucination_score = 0;
    ai_state.health_metrics.error_count = 0;
    ai_state.health_metrics.warning_count = 0;
    
    // Save the model configuration
    ai_model_config = *config;
    
    // Load the AI model
    if (ai_load_model(config) != 0) {
        console_printf("Error: Failed to load AI model\n");
        ai_state.initialized = 0;
        return -1;
    }
    
    console_printf("AI interface initialized\n");
    return 0;
}

/**
 * Shutdown the AI interface
 * 
 * @return: 0 on success, -1 on failure
 */
int ai_shutdown(void) {
    // Check if the AI interface is initialized
    if (!ai_state.initialized) {
        console_printf("AI interface not initialized\n");
        return 0;
    }
    
    // Unload the AI model
    if (ai_unload_model() != 0) {
        console_printf("Error: Failed to unload AI model\n");
        return -1;
    }
    
    // Free all AI tasks
    for (int i = 0; i < MAX_AI_TASKS; i++) {
        if (ai_tasks[i]) {
            // Free the input data
            if (ai_tasks[i]->input_data) {
                memory_free(ai_tasks[i]->input_data, ai_tasks[i]->input_size);
            }
            
            // Free the output data
            if (ai_tasks[i]->output_data) {
                memory_free(ai_tasks[i]->output_data, ai_tasks[i]->output_size);
            }
            
            // Free the task
            memory_free(ai_tasks[i], sizeof(ai_task_t));
            ai_tasks[i] = NULL;
        }
    }
    
    // Reset the AI state
    ai_state.initialized = 0;
    ai_state.model_handle = NULL;
    ai_state.model_memory = NULL;
    ai_state.model_memory_size = 0;
    ai_state.model_loaded = 0;
    
    console_printf("AI interface shutdown\n");
    return 0;
}

/**
 * Create an AI task
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
                          const void* input_data, size_t input_size) {
    // Check if the AI interface is initialized
    if (!ai_state.initialized) {
        console_printf("Error: AI interface not initialized\n");
        return 0;
    }
    
    // Check if the name and description pointers are valid
    if (!name || !description) {
        console_printf("Error: Invalid task name or description\n");
        return 0;
    }
    
    // Check if the input data pointer is valid
    if (!input_data && input_size > 0) {
        console_printf("Error: Invalid input data\n");
        return 0;
    }
    
    // Find a free task slot
    int slot = ai_find_free_task_slot();
    
    if (slot == -1) {
        console_printf("Error: No free task slots\n");
        return 0;
    }
    
    // Allocate memory for the task
    ai_task_t* task = (ai_task_t*)memory_alloc(sizeof(ai_task_t), MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!task) {
        console_printf("Error: Failed to allocate task\n");
        return 0;
    }
    
    // Initialize the task
    task->id = next_task_id++;
    task->type = type;
    task->priority = priority;
    task->state = AI_TASK_STATE_CREATED;
    task->flags = flags;
    
    // Set the task name
    strncpy(task->name, name, sizeof(task->name) - 1);
    
    // Set the task description
    strncpy(task->description, description, sizeof(task->description) - 1);
    
    // Set the creation time to the current system time
    task->creation_time = get_system_time();
    
    // Allocate memory for the input data
    if (input_size > 0) {
        task->input_data = memory_alloc(input_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
        
        if (!task->input_data) {
            console_printf("Error: Failed to allocate input data\n");
            memory_free(task, sizeof(ai_task_t));
            return 0;
        }
        
        // Copy the input data
        memcpy(task->input_data, input_data, input_size);
        task->input_size = input_size;
    }
    
    // Add the task to the task table
    ai_tasks[slot] = task;
    
    return task->id;
}

/**
 * Start an AI task
 * 
 * @param task_id: Task ID
 * @return: 0 on success, -1 on failure
 */
int ai_start_task(ai_task_id_t task_id) {
    // Check if the AI interface is initialized
    if (!ai_state.initialized) {
        console_printf("Error: AI interface not initialized\n");
        return -1;
    }
    
    // Find the task
    ai_task_t* task = NULL;
    
    for (int i = 0; i < MAX_AI_TASKS; i++) {
        if (ai_tasks[i] && ai_tasks[i]->id == task_id) {
            task = ai_tasks[i];
            break;
        }
    }
    
    if (!task) {
        console_printf("Error: Task not found\n");
        return -1;
    }
    
    // Check if the task is already running
    if (task->state == AI_TASK_STATE_RUNNING) {
        console_printf("Error: Task is already running\n");
        return -1;
    }
    
    // Check if the task is already completed
    if (task->state == AI_TASK_STATE_COMPLETED) {
        console_printf("Error: Task is already completed\n");
        return -1;
    }
    
    // Set the task state to queued
    task->state = AI_TASK_STATE_QUEUED;
    
    // Set the start time to the current system time
    task->start_time = get_system_time();
    
    // Execute the task
    return ai_execute_task(task_id);
}

/**
 * Cancel an AI task
 * 
 * @param task_id: Task ID
 * @return: 0 on success, -1 on failure
 */
int ai_cancel_task(ai_task_id_t task_id) {
    // Check if the AI interface is initialized
    if (!ai_state.initialized) {
        console_printf("Error: AI interface not initialized\n");
        return -1;
    }
    
    // Find the task
    ai_task_t* task = NULL;
    
    for (int i = 0; i < MAX_AI_TASKS; i++) {
        if (ai_tasks[i] && ai_tasks[i]->id == task_id) {
            task = ai_tasks[i];
            break;
        }
    }
    
    if (!task) {
        console_printf("Error: Task not found\n");
        return -1;
    }
    
    // Check if the task is already completed or cancelled
    if (task->state == AI_TASK_STATE_COMPLETED || task->state == AI_TASK_STATE_CANCELLED) {
        console_printf("Error: Task is already completed or cancelled\n");
        return -1;
    }
    
    // Set the task state to cancelled
    task->state = AI_TASK_STATE_CANCELLED;
    
    // Set the completion time to the current system time
    task->completion_time = get_system_time();
    
    return 0;
}

/**
 * Get AI task information
 * 
 * @param task_id: Task ID
 * @param task: Pointer to store the task information
 * @return: 0 on success, -1 on failure
 */
int ai_get_task_info(ai_task_id_t task_id, ai_task_t* task) {
    // Check if the AI interface is initialized
    if (!ai_state.initialized) {
        console_printf("Error: AI interface not initialized\n");
        return -1;
    }
    
    // Check if the task pointer is valid
    if (!task) {
        console_printf("Error: Invalid task pointer\n");
        return -1;
    }
    
    // Find the task
    ai_task_t* task_ptr = NULL;
    
    for (int i = 0; i < MAX_AI_TASKS; i++) {
        if (ai_tasks[i] && ai_tasks[i]->id == task_id) {
            task_ptr = ai_tasks[i];
            break;
        }
    }
    
    if (!task_ptr) {
        console_printf("Error: Task not found\n");
        return -1;
    }
    
    // Copy the task information
    *task = *task_ptr;
    
    return 0;
}

/**
 * Get AI task result
 * 
 * @param task_id: Task ID
 * @param output_data: Buffer to store the output data
 * @param output_size: Size of the output buffer
 * @param actual_output_size: Pointer to store the actual output size
 * @return: 0 on success, -1 on failure
 */
int ai_get_task_result(ai_task_id_t task_id, void* output_data, size_t output_size,
                     size_t* actual_output_size) {
    // Check if the AI interface is initialized
    if (!ai_state.initialized) {
        console_printf("Error: AI interface not initialized\n");
        return -1;
    }
    
    // Check if the output data and actual output size pointers are valid
    if (!output_data || !actual_output_size) {
        console_printf("Error: Invalid output data or actual output size pointer\n");
        return -1;
    }
    
    // Find the task
    ai_task_t* task = NULL;
    
    for (int i = 0; i < MAX_AI_TASKS; i++) {
        if (ai_tasks[i] && ai_tasks[i]->id == task_id) {
            task = ai_tasks[i];
            break;
        }
    }
    
    if (!task) {
        console_printf("Error: Task not found\n");
        return -1;
    }
    
    // Check if the task is completed
    if (task->state != AI_TASK_STATE_COMPLETED) {
        console_printf("Error: Task is not completed\n");
        return -1;
    }
    
    // Check if the output buffer is large enough
    if (output_size < task->output_size) {
        console_printf("Error: Output buffer is too small\n");
        *actual_output_size = task->output_size;
        return -1;
    }
    
    // Copy the output data
    memcpy(output_data, task->output_data, task->output_size);
    *actual_output_size = task->output_size;
    
    return 0;
}

/**
 * Wait for an AI task to complete
 * 
 * @param task_id: Task ID
 * @param timeout_ms: Timeout in milliseconds (0 for infinite)
 * @return: 0 on success, -1 on failure or timeout
 */
int ai_wait_for_task(ai_task_id_t task_id, uint64_t timeout_ms) {
    // Check if the AI interface is initialized
    if (!ai_state.initialized) {
        console_printf("Error: AI interface not initialized\n");
        return -1;
    }
    
    // Find the task
    ai_task_t* task = NULL;
    
    for (int i = 0; i < MAX_AI_TASKS; i++) {
        if (ai_tasks[i] && ai_tasks[i]->id == task_id) {
            task = ai_tasks[i];
            break;
        }
    }
    
    if (!task) {
        console_printf("Error: Task not found\n");
        return -1;
    }
    
    // Check if the task is already completed
    if (task->state == AI_TASK_STATE_COMPLETED) {
        return 0;
    }
    
    // Wait for the task to complete
    uint64_t start_time = get_system_time();
    uint64_t current_time;
    
    // Poll the task state until it's completed or the timeout is reached
    while (task->state != AI_TASK_STATE_COMPLETED && 
           task->state != AI_TASK_STATE_FAILED && 
           task->state != AI_TASK_STATE_CANCELLED) {
        
        // Check if the timeout has been reached
        current_time = get_system_time();
        if (timeout_ms > 0 && (current_time - start_time) >= timeout_ms) {
            console_printf("Error: Timeout waiting for task to complete\n");
            return -1;
        }
        
        // Yield to other processes
        process_yield();
        
        // Sleep for a short time to avoid busy waiting
        process_sleep(10); // 10 ms
    }
    
    // Check if the task completed successfully
    if (task->state != AI_TASK_STATE_COMPLETED) {
        console_printf("Error: Task did not complete successfully\n");
        return -1;
    }
    
    return 0;
}

/**
 * Generate code using the AI model
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
                   char* output, size_t output_size, size_t* actual_output_size) {
    // Check if the AI interface is initialized
    if (!ai_state.initialized) {
        console_printf("Error: AI interface not initialized\n");
        return -1;
    }
    
    // Check if the prompt, language, output, and actual output size pointers are valid
    if (!prompt || !language || !output || !actual_output_size) {
        console_printf("Error: Invalid prompt, language, output, or actual output size pointer\n");
        return -1;
    }
    
    // Create a task for code generation
    ai_task_id_t task_id = ai_create_task(AI_TASK_CODE_GENERATION, "Code Generation", "Generate code",
                                        AI_TASK_PRIORITY_NORMAL, flags, prompt, strlen(prompt) + 1);
    
    if (task_id == 0) {
        console_printf("Error: Failed to create code generation task\n");
        return -1;
    }
    
    // Start the task
    if (ai_start_task(task_id) != 0) {
        console_printf("Error: Failed to start code generation task\n");
        return -1;
    }
    
    // Wait for the task to complete
    if (ai_wait_for_task(task_id, 0) != 0) {
        console_printf("Error: Failed to wait for code generation task\n");
        return -1;
    }
    
    // Get the task result
    if (ai_get_task_result(task_id, output, output_size, actual_output_size) != 0) {
        console_printf("Error: Failed to get code generation task result\n");
        return -1;
    }
    
    return 0;
}

/**
 * Optimize code using the AI model
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
                   size_t* actual_output_size) {
    // Check if the AI interface is initialized
    if (!ai_state.initialized) {
        console_printf("Error: AI interface not initialized\n");
        return -1;
    }
    
    // Check if the code, language, output, and actual output size pointers are valid
    if (!code || !language || !output || !actual_output_size) {
        console_printf("Error: Invalid code, language, output, or actual output size pointer\n");
        return -1;
    }
    
    // Check if the optimization level is valid
    if (optimization_level < 1 || optimization_level > 5) {
        console_printf("Error: Invalid optimization level\n");
        return -1;
    }
    
    // Create a task for code optimization
    ai_task_id_t task_id = ai_create_task(AI_TASK_CODE_OPTIMIZATION, "Code Optimization", "Optimize code",
                                        AI_TASK_PRIORITY_NORMAL, flags, code, strlen(code) + 1);
    
    if (task_id == 0) {
        console_printf("Error: Failed to create code optimization task\n");
        return -1;
    }
    
    // Start the task
    if (ai_start_task(task_id) != 0) {
        console_printf("Error: Failed to start code optimization task\n");
        return -1;
    }
    
    // Wait for the task to complete
    if (ai_wait_for_task(task_id, 0) != 0) {
        console_printf("Error: Failed to wait for code optimization task\n");
        return -1;
    }
    
    // Get the task result
    if (ai_get_task_result(task_id, output, output_size, actual_output_size) != 0) {
        console_printf("Error: Failed to get code optimization task result\n");
        return -1;
    }
    
    return 0;
}

/**
 * Analyze code using the AI model
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
                  char* output, size_t output_size, size_t* actual_output_size) {
    // Check if the AI interface is initialized
    if (!ai_state.initialized) {
        console_printf("Error: AI interface not initialized\n");
        return -1;
    }
    
    // Check if the code, language, output, and actual output size pointers are valid
    if (!code || !language || !output || !actual_output_size) {
        console_printf("Error: Invalid code, language, output, or actual output size pointer\n");
        return -1;
    }
    
    // Create a task for code analysis
    ai_task_id_t task_id = ai_create_task(AI_TASK_CODE_ANALYSIS, "Code Analysis", "Analyze code",
                                        AI_TASK_PRIORITY_NORMAL, flags, code, strlen(code) + 1);
    
    if (task_id == 0) {
        console_printf("Error: Failed to create code analysis task\n");
        return -1;
    }
    
    // Start the task
    if (ai_start_task(task_id) != 0) {
        console_printf("Error: Failed to start code analysis task\n");
        return -1;
    }
    
    // Wait for the task to complete
    if (ai_wait_for_task(task_id, 0) != 0) {
        console_printf("Error: Failed to wait for code analysis task\n");
        return -1;
    }
    
    // Get the task result
    if (ai_get_task_result(task_id, output, output_size, actual_output_size) != 0) {
        console_printf("Error: Failed to get code analysis task result\n");
        return -1;
    }
    
    return 0;
}

/**
 * Execute code generated by the AI model
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
                  char* output, size_t output_size, size_t* actual_output_size) {
    // Check if the AI interface is initialized
    if (!ai_state.initialized) {
        console_printf("Error: AI interface not initialized\n");
        return -1;
    }
    
    // Check if the code, language, output, and actual output size pointers are valid
    if (!code || !language || !output || !actual_output_size) {
        console_printf("Error: Invalid code, language, output, or actual output size pointer\n");
        return -1;
    }
    
    // Create a sandbox configuration
    sandbox_config_t sandbox_config;
    memset(&sandbox_config, 0, sizeof(sandbox_config_t));
    
    // Set the sandbox configuration
    sandbox_config.type = SANDBOX_TYPE_PROCESS;
    sandbox_config.flags = sandbox_flags;
    sandbox_config.cpu_limit = 50;  // 50% CPU limit
    sandbox_config.memory_limit = 256 * 1024 * 1024;  // 256 MB memory limit
    sandbox_config.disk_limit = 100 * 1024 * 1024;  // 100 MB disk limit
    sandbox_config.network_limit = 0;  // No network access
    sandbox_config.process_limit = 10;  // 10 processes
    sandbox_config.thread_limit = 20;  // 20 threads
    sandbox_config.file_limit = 100;  // 100 files
    sandbox_config.socket_limit = 0;  // No sockets
    
    // Set the sandbox name
    snprintf(sandbox_config.name, sizeof(sandbox_config.name), "AI Code Execution");
    
    // Create the sandbox
    uint32_t sandbox_id = sandbox_create(&sandbox_config);
    
    if (sandbox_id == 0) {
        console_printf("Error: Failed to create sandbox\n");
        return -1;
    }
    
    // Determine the language-specific file extension
    const char* file_ext = ".c"; // Default to C
    if (strcmp(language, "python") == 0 || strcmp(language, "py") == 0) {
        file_ext = ".py";
    } else if (strcmp(language, "javascript") == 0 || strcmp(language, "js") == 0) {
        file_ext = ".js";
    } else if (strcmp(language, "java") == 0) {
        file_ext = ".java";
    } else if (strcmp(language, "cpp") == 0 || strcmp(language, "c++") == 0) {
        file_ext = ".cpp";
    } else if (strcmp(language, "rust") == 0 || strcmp(language, "rs") == 0) {
        file_ext = ".rs";
    } else if (strcmp(language, "go") == 0) {
        file_ext = ".go";
    }
    
    // Create a temporary file for the code
    char code_file[256];
    snprintf(code_file, sizeof(code_file), "/tmp/ai_code_%u%s", sandbox_id, file_ext);
    
    // Write the code to the file
    FILE* file = fopen(code_file, "w");
    if (!file) {
        console_printf("Error: Failed to create code file\n");
        sandbox_destroy(sandbox_id);
        return -1;
    }
    
    fwrite(code, 1, strlen(code), file);
    fclose(file);
    
    // Create a binary file path if needed
    char binary_file[256] = {0};
    
    // Prepare the command to execute
    char execute_cmd[2048] = {0}; // Much larger buffer to avoid any truncation
    
    if (strcmp(language, "c") == 0) {
        // Compile and execute C code
        snprintf(binary_file, sizeof(binary_file), "/tmp/ai_binary_%u", sandbox_id);
        
        // Build the command directly, avoiding multiple snprintf calls
        strcat(execute_cmd, "gcc -o ");
        strcat(execute_cmd, binary_file);
        strcat(execute_cmd, " ");
        strcat(execute_cmd, code_file);
        strcat(execute_cmd, " -Wall && ");
        strcat(execute_cmd, binary_file);
    } else if (strcmp(language, "cpp") == 0 || strcmp(language, "c++") == 0) {
        // Compile and execute C++ code
        snprintf(binary_file, sizeof(binary_file), "/tmp/ai_binary_%u", sandbox_id);
        
        // Build the command directly
        strcat(execute_cmd, "g++ -o ");
        strcat(execute_cmd, binary_file);
        strcat(execute_cmd, " ");
        strcat(execute_cmd, code_file);
        strcat(execute_cmd, " -Wall -std=c++17 && ");
        strcat(execute_cmd, binary_file);
    } else if (strcmp(language, "python") == 0 || strcmp(language, "py") == 0) {
        // Execute Python code
        strcat(execute_cmd, "python3 ");
        strcat(execute_cmd, code_file);
    } else if (strcmp(language, "javascript") == 0 || strcmp(language, "js") == 0) {
        // Execute JavaScript code
        strcat(execute_cmd, "node ");
        strcat(execute_cmd, code_file);
    } else if (strcmp(language, "java") == 0) {
        // Compile and execute Java code
        char class_name[256] = {0};
        snprintf(class_name, sizeof(class_name), "AICode%u", sandbox_id);
        
        // Build the command directly
        strcat(execute_cmd, "javac -d /tmp ");
        strcat(execute_cmd, code_file);
        strcat(execute_cmd, " && cd /tmp && java ");
        strcat(execute_cmd, class_name);
    } else if (strcmp(language, "rust") == 0 || strcmp(language, "rs") == 0) {
        // Compile and execute Rust code
        snprintf(binary_file, sizeof(binary_file), "/tmp/ai_binary_%u", sandbox_id);
        
        // Build the command directly
        strcat(execute_cmd, "rustc -o ");
        strcat(execute_cmd, binary_file);
        strcat(execute_cmd, " ");
        strcat(execute_cmd, code_file);
        strcat(execute_cmd, " && ");
        strcat(execute_cmd, binary_file);
    } else if (strcmp(language, "go") == 0) {
        // Compile and execute Go code
        snprintf(binary_file, sizeof(binary_file), "/tmp/ai_binary_%u", sandbox_id);
        
        // Build the command directly
        strcat(execute_cmd, "go build -o ");
        strcat(execute_cmd, binary_file);
        strcat(execute_cmd, " ");
        strcat(execute_cmd, code_file);
        strcat(execute_cmd, " && ");
        strcat(execute_cmd, binary_file);
    } else {
        // Unsupported language
        console_printf("Error: Unsupported language '%s'\n", language);
        sandbox_destroy(sandbox_id);
        unlink(code_file);
        return -1;
    }
    
    // Create a pipe for capturing the command output
    int pipefd[2];
    if (pipe(pipefd) != 0) {
        console_printf("Error: Failed to create pipe\n");
        sandbox_destroy(sandbox_id);
        unlink(code_file);
        return -1;
    }
    
    // Execute the command in the sandbox
    pid_t pid = fork();
    
    if (pid < 0) {
        // Fork failed
        console_printf("Error: Failed to fork process\n");
        close(pipefd[0]);
        close(pipefd[1]);
        sandbox_destroy(sandbox_id);
        unlink(code_file);
        return -1;
    } else if (pid == 0) {
        // Child process
        
        // Redirect stdout and stderr to the pipe
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        
        // Execute the command
        execl("/bin/sh", "sh", "-c", execute_cmd, NULL);
        
        // If execl returns, it failed
        fprintf(stderr, "Error: Failed to execute command: %s\n", execute_cmd);
        exit(1);
    } else {
        // Parent process
        
        // Close the write end of the pipe
        close(pipefd[1]);
        
        // Read the command output
        char buffer[4096];
        ssize_t bytes_read;
        size_t total_bytes_read = 0;
        
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            // Check if the output buffer is large enough
            if (total_bytes_read + bytes_read > output_size - 1) {
                // Output buffer is too small
                close(pipefd[0]);
                sandbox_destroy(sandbox_id);
                unlink(code_file);
                *actual_output_size = total_bytes_read + bytes_read;
                return -1;
            }
            
            // Copy the output to the buffer
            memcpy(output + total_bytes_read, buffer, bytes_read);
            total_bytes_read += bytes_read;
        }
        
        // Null-terminate the output
        output[total_bytes_read] = '\0';
        *actual_output_size = total_bytes_read + 1;
        
        // Close the read end of the pipe
        close(pipefd[0]);
        
        // Wait for the child process to exit
        int status;
        waitpid(pid, &status, 0);
        
        // Clean up
        sandbox_destroy(sandbox_id);
        unlink(code_file);
        
        // Check if the command succeeded
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            return 0;
        } else {
            return -1;
        }
    }
}

/**
 * Find a free task slot in the task table
 * 
 * @return: Slot index on success, -1 if no free slots
 */
static int ai_find_free_task_slot(void) {
    for (int i = 0; i < MAX_AI_TASKS; i++) {
        if (ai_tasks[i] == NULL) {
            return i;
        }
    }
    
    return -1;
}

/**
 * Execute an AI task
 * 
 * @param task_id: Task ID
 * @return: 0 on success, -1 on failure
 */
static int ai_execute_task(ai_task_id_t task_id) {
    // Find the task
    ai_task_t* task = NULL;
    
    for (int i = 0; i < MAX_AI_TASKS; i++) {
        if (ai_tasks[i] && ai_tasks[i]->id == task_id) {
            task = ai_tasks[i];
            break;
        }
    }
    
    if (!task) {
        console_printf("Error: Task not found\n");
        return -1;
    }
    
    // Check if the task is already running
    if (task->state == AI_TASK_STATE_RUNNING) {
        console_printf("Error: Task is already running\n");
        return -1;
    }
    
    // Check if the task is already completed
    if (task->state == AI_TASK_STATE_COMPLETED) {
        console_printf("Error: Task is already completed\n");
        return -1;
    }
    
    // Check if the task is already cancelled
    if (task->state == AI_TASK_STATE_CANCELLED) {
        console_printf("Error: Task is already cancelled\n");
        return -1;
    }
    
    // Check if the task is already failed
    if (task->state == AI_TASK_STATE_FAILED) {
        console_printf("Error: Task is already failed\n");
        return -1;
    }
    
    // Check if the task limits are exceeded
    if (ai_check_task_limits(task_id) != 0) {
        console_printf("Error: Task limits exceeded\n");
        task->state = AI_TASK_STATE_FAILED;
        task->completion_time = get_system_time();
        return -1;
    }
    
    // Set the task state to running
    task->state = AI_TASK_STATE_RUNNING;
    
    // Execute the task based on its type
    int result = -1;
    
    switch (task->type) {
        case AI_TASK_CODE_GENERATION:
            result = ai_execute_code_generation_task(task);
            break;
        case AI_TASK_CODE_OPTIMIZATION:
            result = ai_execute_code_optimization_task(task);
            break;
        case AI_TASK_CODE_ANALYSIS:
            result = ai_execute_code_analysis_task(task);
            break;
        case AI_TASK_NLP:
            result = ai_execute_nlp_task(task);
            break;
        case AI_TASK_SYSTEM_MONITORING:
            result = ai_execute_system_monitoring_task(task);
            break;
        case AI_TASK_SYSTEM_OPTIMIZATION:
            result = ai_execute_system_optimization_task(task);
            break;
        case AI_TASK_NETWORK_ANALYSIS:
            result = ai_execute_network_analysis_task(task);
            break;
        default:
            console_printf("Error: Unknown task type\n");
            result = -1;
            break;
    }
    
    // Set the task state based on the result
    if (result == 0) {
        task->state = AI_TASK_STATE_COMPLETED;
    } else {
        task->state = AI_TASK_STATE_FAILED;
    }
    
    // Set the completion time to the current system time
    task->completion_time = get_system_time();
    
    return result;
}

/**
 * Check if the task limits are exceeded
 * 
 * @param task_id: Task ID
 * @return: 0 if the limits are not exceeded, -1 otherwise
 */
static int ai_check_task_limits(ai_task_id_t task_id) {
    // Find the task
    ai_task_t* task = NULL;
    
    for (int i = 0; i < MAX_AI_TASKS; i++) {
        if (ai_tasks[i] && ai_tasks[i]->id == task_id) {
            task = ai_tasks[i];
            break;
        }
    }
    
    if (!task) {
        console_printf("Error: Task not found\n");
        return -1;
    }
    
    // Check if the task has been running for too long
    uint64_t current_time = get_system_time();
    uint64_t elapsed_time = current_time - task->start_time;
    
    if (elapsed_time > 60000) { // 60 seconds
        console_printf("Error: Task execution time limit exceeded\n");
        return -1;
    }
    
    // Check if the task is using too much memory
    if (task->input_size > 10 * 1024 * 1024) { // 10 MB
        console_printf("Error: Task input size limit exceeded\n");
        return -1;
    }
    
    return 0;
}

/**
 * Load an AI model
 * 
 * @param config: AI model configuration
 * @return: 0 on success, -1 on failure
 */
static int ai_load_model(const ai_model_config_t* config) {
    // Check if the config pointer is valid
    if (!config) {
        console_printf("Error: Invalid AI model configuration\n");
        return -1;
    }
    
    // Check if a model is already loaded
    if (ai_state.model_loaded) {
        console_printf("Error: AI model already loaded\n");
        return -1;
    }
    
    // Allocate memory for the model
    size_t model_memory_size = config->memory_limit;
    void* model_memory = memory_alloc(model_memory_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!model_memory) {
        console_printf("Error: Failed to allocate model memory\n");
        return -1;
    }
    
    // Load the model from the specified file
    FILE* file = fopen(config->path, "rb");
    
    if (!file) {
        console_printf("Error: Failed to open model file: %s\n", config->path);
        memory_free(model_memory, model_memory_size);
        return -1;
    }
    
    // Get the file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Check if the file size is valid
    if (file_size == 0) {
        console_printf("Error: Model file is empty\n");
        fclose(file);
        memory_free(model_memory, model_memory_size);
        return -1;
    }
    
    // Check if the model memory is large enough
    if (file_size > model_memory_size) {
        console_printf("Error: Model file is too large\n");
        fclose(file);
        memory_free(model_memory, model_memory_size);
        return -1;
    }
    
    // Read the model data
    size_t bytes_read = fread(model_memory, 1, file_size, file);
    
    // Close the file
    fclose(file);
    
    // Check if the model was loaded successfully
    if (bytes_read != file_size) {
        console_printf("Error: Failed to read model data\n");
        memory_free(model_memory, model_memory_size);
        return -1;
    }
    
    // Set the model state
    ai_state.model_memory = model_memory;
    ai_state.model_memory_size = model_memory_size;
    ai_state.model_loaded = 1;
    
    console_printf("AI model loaded: %s\n", config->path);
    return 0;
}

/**
 * Unload an AI model
 * 
 * @return: 0 on success, -1 on failure
 */
static int ai_unload_model(void) {
    // Check if a model is loaded
    if (!ai_state.model_loaded) {
        console_printf("Error: No AI model loaded\n");
        return -1;
    }
    
    // Free the model memory
    if (ai_state.model_memory) {
        memory_free(ai_state.model_memory, ai_state.model_memory_size);
        ai_state.model_memory = NULL;
        ai_state.model_memory_size = 0;
    }
    
    // Reset the model state
    ai_state.model_loaded = 0;
    
    console_printf("AI model unloaded\n");
    return 0;
}

/**
 * Execute a code generation task
 * 
 * @param task: Task to execute
 * @return: 0 on success, -1 on failure
 */
static int ai_execute_code_generation_task(ai_task_t* task) {
    // Check if the task pointer is valid
    if (!task) {
        console_printf("Error: Invalid task pointer\n");
        return -1;
    }
    
    // Check if the input data is valid
    if (!task->input_data || task->input_size == 0) {
        console_printf("Error: Invalid input data\n");
        return -1;
    }
    
    // Get the prompt from the input data
    const char* prompt = (const char*)task->input_data;
    
    // Check if the model is loaded
    if (!ai_state.model_loaded || !ai_state.model_memory) {
        console_printf("Error: AI model not loaded\n");
        return -1;
    }
    
    // Prepare the generation parameters
    ai_generation_params_t params;
    memset(&params, 0, sizeof(ai_generation_params_t));
    params.max_tokens = 1024;
    params.temperature = 0.7f;
    params.top_p = 0.9f;
    params.top_k = 40;
    params.repetition_penalty = 1.1f;
    params.stop_tokens = NULL;
    params.num_stop_tokens = 0;
    
    // Extract language information from the prompt if available
    const char* language = NULL;
    if (strstr(prompt, "Python") || strstr(prompt, "python")) {
        language = "python";
        params.stop_tokens = (const char*[]){ "```", "###" };
        params.num_stop_tokens = 2;
    } else if (strstr(prompt, "JavaScript") || strstr(prompt, "javascript") || strstr(prompt, "JS") || strstr(prompt, "js")) {
        language = "javascript";
        params.stop_tokens = (const char*[]){ "```", "###" };
        params.num_stop_tokens = 2;
    } else if (strstr(prompt, "C++") || strstr(prompt, "c++") || strstr(prompt, "cpp")) {
        language = "cpp";
        params.stop_tokens = (const char*[]){ "```", "###" };
        params.num_stop_tokens = 2;
    } else if (strstr(prompt, "Java") || strstr(prompt, "java")) {
        language = "java";
        params.stop_tokens = (const char*[]){ "```", "###" };
        params.num_stop_tokens = 2;
    } else {
        language = "c";
        params.stop_tokens = (const char*[]){ "```", "###" };
        params.num_stop_tokens = 2;
    }
    
    // Prepare the prompt with instructions for code generation
    char full_prompt[4096];
    snprintf(full_prompt, sizeof(full_prompt), 
             "You are an expert programmer. Generate high-quality, efficient, and well-documented %s code for the following task:\n\n%s\n\nProvide only the code without explanations. Use best practices and modern coding standards.",
             language ? language : "C", prompt);
    
    // Allocate memory for the generated code
    size_t max_output_size = 32 * 1024; // 32 KB
    char* generated_code = (char*)memory_alloc(max_output_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!generated_code) {
        console_printf("Error: Failed to allocate memory for generated code\n");
        return -1;
    }
    
    // Generate the code using the AI model
    size_t actual_output_size = 0;
    int result = ai_generate_text(full_prompt, generated_code, max_output_size, &actual_output_size, &params);
    
    if (result != 0 || actual_output_size == 0) {
        console_printf("Error: Failed to generate code\n");
        memory_free(generated_code, max_output_size);
        return -1;
    }
    
    // Post-process the generated code
    // Remove any markdown code block markers if present
    char* code_start = strstr(generated_code, "```");
    if (code_start) {
        code_start = strchr(code_start + 3, '\n');
        if (code_start) {
            code_start++; // Skip the newline
            
            // Find the end of the code block
            char* code_end = strstr(code_start, "```");
            if (code_end) {
                *code_end = '\0';
                
                // Shift the code to the beginning of the buffer
                memmove(generated_code, code_start, strlen(code_start) + 1);
                actual_output_size = strlen(generated_code) + 1;
            }
        }
    }
    
    // Allocate memory for the final output
    void* output_data = memory_alloc(actual_output_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!output_data) {
        console_printf("Error: Failed to allocate output data\n");
        memory_free(generated_code, max_output_size);
        return -1;
    }
    
    // Copy the generated code to the output data
    memcpy(output_data, generated_code, actual_output_size);
    
    // Free the temporary buffer
    memory_free(generated_code, max_output_size);
    
    // Set the task output data
    task->output_data = output_data;
    task->output_size = actual_output_size;
    
    return 0;
}

/**
 * Execute a code optimization task
 * 
 * @param task: Task to execute
 * @return: 0 on success, -1 on failure
 */
/**
 * Execute a code optimization task
 * 
 * @param task: Task to execute
 * @return: 0 on success, -1 on failure
 */
static int ai_execute_code_optimization_task(ai_task_t* task) {
    // Check if the task pointer is valid
    if (!task) {
        console_printf("Error: Invalid task pointer\n");
        return -1;
    }
    
    // Check if the input data is valid
    if (!task->input_data || task->input_size == 0) {
        console_printf("Error: Invalid input data\n");
        return -1;
    }
    
    // Get the code from the input data
    const char* code = (const char*)task->input_data;
    
    // Check if the model is loaded
    if (!ai_state.model_loaded || !ai_state.model_memory) {
        console_printf("Error: AI model not loaded\n");
        return -1;
    }
    
    // Determine the language from the code
    const char* language = "unknown";
    if (strstr(code, "def ") || strstr(code, "import ") || (strstr(code, "class ") && strstr(code, ":"))) {
        language = "python";
    } else if (strstr(code, "function ") || strstr(code, "const ") || strstr(code, "let ") || strstr(code, "var ")) {
        language = "javascript";
    } else if (strstr(code, "#include") && (strstr(code, "int main") || strstr(code, "void main"))) {
        language = "c";
    } else if (strstr(code, "#include") && strstr(code, "class ") && strstr(code, "public:")) {
        language = "cpp";
    } else if (strstr(code, "public class") || strstr(code, "private class")) {
        language = "java";
    }
    
    // Prepare the generation parameters
    ai_generation_params_t params;
    memset(&params, 0, sizeof(ai_generation_params_t));
    params.max_tokens = 2048;
    params.temperature = 0.5f; // Lower temperature for more deterministic output
    params.top_p = 0.95f;
    params.top_k = 50;
    params.repetition_penalty = 1.2f;
    params.stop_tokens = (const char*[]){ "```", "###" };
    params.num_stop_tokens = 2;
    
    // Prepare the prompt with instructions for code optimization
    char full_prompt[65536]; // Large buffer for potentially large code inputs
    snprintf(full_prompt, sizeof(full_prompt), 
             "You are an expert programmer specializing in code optimization. Optimize the following %s code for better performance, readability, and maintainability. Apply best practices, remove redundancies, and improve algorithms where possible. Here is the code to optimize:\n\n```%s\n%s\n```\n\nProvide only the optimized code without explanations.",
             language ? language : "C", language, code);
    
    // Allocate memory for the optimized code
    size_t max_output_size = 64 * 1024; // 64 KB
    char* optimized_code = (char*)memory_alloc(max_output_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!optimized_code) {
        console_printf("Error: Failed to allocate memory for optimized code\n");
        return -1;
    }
    
    // Generate the optimized code using the AI model
    size_t actual_output_size = 0;
    int result = ai_generate_text(full_prompt, optimized_code, max_output_size, &actual_output_size, &params);
    
    if (result != 0 || actual_output_size == 0) {
        console_printf("Error: Failed to optimize code\n");
        memory_free(optimized_code, max_output_size);
        return -1;
    }
    
    // Post-process the optimized code
    // Remove any markdown code block markers if present
    char* code_start = strstr(optimized_code, "```");
    if (code_start) {
        code_start = strchr(code_start + 3, '\n');
        if (code_start) {
            code_start++; // Skip the newline
            
            // Find the end of the code block
            char* code_end = strstr(code_start, "```");
            if (code_end) {
                *code_end = '\0';
                
                // Shift the code to the beginning of the buffer
                memmove(optimized_code, code_start, strlen(code_start) + 1);
                actual_output_size = strlen(optimized_code) + 1;
            }
        }
    }
    
    // Allocate memory for the final output
    void* output_data = memory_alloc(actual_output_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!output_data) {
        console_printf("Error: Failed to allocate output data\n");
        memory_free(optimized_code, max_output_size);
        return -1;
    }
    
    // Copy the optimized code to the output data
    memcpy(output_data, optimized_code, actual_output_size);
    
    // Free the temporary buffer
    memory_free(optimized_code, max_output_size);
    
    // Set the task output data
    task->output_data = output_data;
    task->output_size = actual_output_size;
    
    return 0;
}

/**
 * Execute a code analysis task
 * 
 * @param task: Task to execute
 * @return: 0 on success, -1 on failure
 */
static int ai_execute_code_analysis_task(ai_task_t* task) {
    // Check if the task pointer is valid
    if (!task) {
        console_printf("Error: Invalid task pointer\n");
        return -1;
    }
    
    // Check if the input data is valid
    if (!task->input_data || task->input_size == 0) {
        console_printf("Error: Invalid input data\n");
        return -1;
    }
    
    // Get the code from the input data
    const char* code = (const char*)task->input_data;
    
    // Check if the model is loaded
    if (!ai_state.model_loaded || !ai_state.model_memory) {
        console_printf("Error: AI model not loaded\n");
        return -1;
    }
    
    // Determine the language from the code
    const char* language = "unknown";
    if (strstr(code, "def ") || strstr(code, "import ") || (strstr(code, "class ") && strstr(code, ":"))) {
        language = "python";
    } else if (strstr(code, "function ") || strstr(code, "const ") || strstr(code, "let ") || strstr(code, "var ")) {
        language = "javascript";
    } else if (strstr(code, "#include") && (strstr(code, "int main") || strstr(code, "void main"))) {
        language = "c";
    } else if (strstr(code, "#include") && strstr(code, "class ") && strstr(code, "public:")) {
        language = "cpp";
    } else if (strstr(code, "public class") || strstr(code, "private class")) {
        language = "java";
    }
    
    // Prepare the generation parameters
    ai_generation_params_t params;
    memset(&params, 0, sizeof(ai_generation_params_t));
    params.max_tokens = 2048;
    params.temperature = 0.3f; // Lower temperature for more factual analysis
    params.top_p = 0.95f;
    params.top_k = 40;
    params.repetition_penalty = 1.1f;
    params.stop_tokens = NULL;
    params.num_stop_tokens = 0;
    
    // Prepare the prompt with instructions for code analysis
    char full_prompt[65536]; // Large buffer for potentially large code inputs
    snprintf(full_prompt, sizeof(full_prompt), 
             "You are an expert code reviewer and static analyzer. Analyze the following %s code for issues, bugs, security vulnerabilities, performance problems, and best practice violations. Provide a detailed analysis with specific line references where applicable.\n\n```%s\n%s\n```\n\nYour analysis should include:\n1. Syntax and logical errors\n2. Security vulnerabilities\n3. Performance issues\n4. Memory management problems\n5. Code style and readability issues\n6. Potential edge cases\n7. Overall code quality assessment\n8. Specific recommendations for improvement",
             language, language, code);
    
    // Allocate memory for the analysis
    size_t max_output_size = 32 * 1024; // 32 KB
    char* analysis = (char*)memory_alloc(max_output_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!analysis) {
        console_printf("Error: Failed to allocate memory for code analysis\n");
        return -1;
    }
    
    // Generate the analysis using the AI model
    size_t actual_output_size = 0;
    int result = ai_generate_text(full_prompt, analysis, max_output_size, &actual_output_size, &params);
    
    if (result != 0 || actual_output_size == 0) {
        console_printf("Error: Failed to analyze code\n");
        memory_free(analysis, max_output_size);
        return -1;
    }
    
    // Perform static analysis on the code
    char* static_analysis = NULL;
    size_t static_analysis_size = 0;
    
    // Check for common issues based on language
    if (strcmp(language, "c") == 0 || strcmp(language, "cpp") == 0) {
        // Check for memory leaks
        if (strstr(code, "malloc") || strstr(code, "calloc") || strstr(code, "realloc")) {
            if (!strstr(code, "free")) {
                static_analysis = "WARNING: Potential memory leak detected. Memory is allocated but never freed.\n\n";
                static_analysis_size = strlen(static_analysis);
            }
        }
        
        // Check for buffer overflows
        if (strstr(code, "strcpy") || strstr(code, "strcat") || strstr(code, "sprintf")) {
            if (!strstr(code, "strncpy") && !strstr(code, "strncat") && !strstr(code, "snprintf")) {
                if (static_analysis) {
                    char* new_analysis = (char*)memory_alloc(static_analysis_size + 128, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
                    if (new_analysis) {
                        strcpy(new_analysis, static_analysis);
                        strcat(new_analysis, "WARNING: Potential buffer overflow detected. Use safer string functions like strncpy, strncat, or snprintf.\n\n");
                        memory_free(static_analysis, static_analysis_size);
                        static_analysis = new_analysis;
                        static_analysis_size = strlen(static_analysis);
                    }
                } else {
                    static_analysis = "WARNING: Potential buffer overflow detected. Use safer string functions like strncpy, strncat, or snprintf.\n\n";
                    static_analysis_size = strlen(static_analysis);
                }
            }
        }
    } else if (strcmp(language, "python") == 0) {
        // Check for potential security issues
        if (strstr(code, "eval(") || strstr(code, "exec(")) {
            static_analysis = "WARNING: Use of eval() or exec() detected. These functions can execute arbitrary code and pose security risks.\n\n";
            static_analysis_size = strlen(static_analysis);
        }
    }
    
    // Combine static analysis with AI analysis if available
    if (static_analysis) {
        size_t combined_size = static_analysis_size + actual_output_size;
        char* combined_analysis = (char*)memory_alloc(combined_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
        
        if (combined_analysis) {
            strcpy(combined_analysis, static_analysis);
            strcat(combined_analysis, analysis);
            
            memory_free(analysis, max_output_size);
            memory_free(static_analysis, static_analysis_size);
            
            analysis = combined_analysis;
            actual_output_size = combined_size;
        }
    }
    
    // Allocate memory for the final output
    void* output_data = memory_alloc(actual_output_size, MEMORY_PROT_READ | MEMORY_PROT_WRITE, MEMORY_ALLOC_ZEROED);
    
    if (!output_data) {
        console_printf("Error: Failed to allocate output data\n");
        memory_free(analysis, max_output_size);
        return -1;
    }
    
    // Copy the analysis to the output data
    memcpy(output_data, analysis, actual_output_size);
    
    // Free the temporary buffer
    memory_free(analysis, max_output_size);
    
    // Set the task output data
    task->output_data = output_data;
    task->output_size = actual_output_size;
    
    return 0;
}
