#!/bin/bash

# NeuroOS Model Test Script
# This script tests the model loading functionality

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print banner
echo -e "${BLUE}"
echo "███╗   ██╗███████╗██╗   ██╗██████╗  ██████╗  ██████╗ ███████╗"
echo "████╗  ██║██╔════╝██║   ██║██╔══██╗██╔═══██╗██╔═══██╗██╔════╝"
echo "██╔██╗ ██║█████╗  ██║   ██║██████╔╝██║   ██║██║   ██║███████╗"
echo "██║╚██╗██║██╔══╝  ██║   ██║██╔══██╗██║   ██║██║   ██║╚════██║"
echo "██║ ╚████║███████╗╚██████╔╝██║  ██║╚██████╔╝╚██████╔╝███████║"
echo "╚═╝  ╚═══╝╚══════╝ ╚═════╝ ╚═╝  ╚═╝ ╚═════╝  ╚═════╝ ╚══════╝"
echo -e "${NC}"
echo -e "${GREEN}NeuroOS Model Test${NC}"
echo

# Check if the deepseek directory exists
if [ ! -d "deepseek" ]; then
    echo -e "${RED}Error: deepseek directory not found.${NC}"
    exit 1
fi

# Check if the model files exist
if [ ! -f "deepseek/model.safetensors" ]; then
    echo -e "${RED}Error: model.safetensors not found.${NC}"
    exit 1
fi

if [ ! -f "deepseek/tokenizer.json" ]; then
    echo -e "${RED}Error: tokenizer.json not found.${NC}"
    exit 1
fi

if [ ! -f "deepseek/config.json" ]; then
    echo -e "${RED}Error: config.json not found.${NC}"
    exit 1
fi

if [ ! -f "deepseek/generation_config.json" ]; then
    echo -e "${RED}Error: generation_config.json not found.${NC}"
    exit 1
fi

echo -e "${GREEN}All model files found.${NC}"
echo

# Create a simple test program
cat > test_model.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernel/include/neural_network.h"
#include "kernel/include/nlp.h"
#include "kernel/include/ai_interface.h"

int main() {
    printf("Testing Deepseek model loading and inference...\n");
    
    // Initialize the neural network subsystem
    if (nn_init() != 0) {
        printf("Failed to initialize neural network subsystem\n");
        return 1;
    }
    
    // Load the model
    nn_model_t* model = NULL;
    if (nn_load_deepseek_model("deepseek", &model) != 0) {
        printf("Failed to load Deepseek model\n");
        nn_shutdown();
        return 1;
    }
    
    printf("Model loaded successfully\n");
    
    // Test text generation
    const char* prompt = "Hello, I am NeuroOS, an AI-powered operating system.";
    char output[1024];
    
    if (nn_deepseek_generate(model, prompt, output, sizeof(output), 100, 0.7f, 0.9f, 40.0f, 1.1f) != 0) {
        printf("Failed to generate text\n");
        nn_model_destroy(model);
        nn_shutdown();
        return 1;
    }
    
    printf("Generated text: %s\n", output);
    
    // Test embedding
    float embedding[768]; // Assuming 768-dimensional embeddings
    if (nn_deepseek_embed(model, prompt, embedding, 768) != 0) {
        printf("Failed to generate embeddings\n");
        nn_model_destroy(model);
        nn_shutdown();
        return 1;
    }
    
    printf("Embedding generated successfully\n");
    
    // Test classification
    const char* labels[] = {"positive", "negative", "neutral"};
    float probabilities[3];
    
    if (nn_deepseek_classify(model, prompt, labels, 3, probabilities) != 0) {
        printf("Failed to classify text\n");
        nn_model_destroy(model);
        nn_shutdown();
        return 1;
    }
    
    printf("Classification results:\n");
    for (int i = 0; i < 3; i++) {
        printf("  %s: %.2f%%\n", labels[i], probabilities[i] * 100.0f);
    }
    
    // Clean up
    nn_model_destroy(model);
    nn_shutdown();
    
    printf("Test completed successfully\n");
    return 0;
}
EOF

# Test model loading
echo -e "${BLUE}Testing model loading and inference...${NC}"
echo -e "${YELLOW}This may take a while...${NC}"

# Compile the test program
gcc -o test_model test_model.c -I. -Wall -Wextra

# Run the test program
./test_model

# Clean up
rm -f test_model test_model.c

echo -e "${GREEN}All tests passed!${NC}"
