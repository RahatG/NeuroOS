#!/bin/bash

# NeuroOS Build Script
# This script compiles the NeuroOS kernel, sets up the bootloader, and creates an ISO image

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Build directories
BUILD_DIR="build"
ISO_DIR="$BUILD_DIR/iso"
BOOT_DIR="$ISO_DIR/boot"
GRUB_DIR="$BOOT_DIR/grub"

# Print banner
echo -e "${BLUE}"
echo "███╗   ██╗███████╗██╗   ██╗██████╗  ██████╗  ██████╗ ███████╗"
echo "████╗  ██║██╔════╝██║   ██║██╔══██╗██╔═══██╗██╔═══██╗██╔════╝"
echo "██╔██╗ ██║█████╗  ██║   ██║██████╔╝██║   ██║██║   ██║███████╗"
echo "██║╚██╗██║██╔══╝  ██║   ██║██╔══██╗██║   ██║██║   ██║╚════██║"
echo "██║ ╚████║███████╗╚██████╔╝██║  ██║╚██████╔╝╚██████╔╝███████║"
echo "╚═╝  ╚═══╝╚══════╝ ╚═════╝ ╚═╝  ╚═╝ ╚═════╝  ╚═════╝ ╚══════╝"
echo -e "${NC}"
echo -e "${GREEN}Building NeuroOS - AI-Powered Operating System${NC}"
echo

# Check for required tools
echo -e "${BLUE}Checking for required tools...${NC}"
for tool in nasm gcc ld grub-mkrescue xorriso; do
    if ! command -v $tool &> /dev/null; then
        echo -e "${RED}Error: $tool is not installed. Please install it and try again.${NC}"
        exit 1
    fi
done
echo -e "${GREEN}All required tools are installed.${NC}"
echo

# Create build directories
echo -e "${BLUE}Creating build directories...${NC}"
mkdir -p $BOOT_DIR
mkdir -p $GRUB_DIR
echo -e "${GREEN}Build directories created.${NC}"
echo

# Compile boot.asm
echo -e "${BLUE}Compiling bootloader...${NC}"
nasm -f elf64 boot/boot.asm -o $BUILD_DIR/boot.o
echo -e "${GREEN}Bootloader compiled.${NC}"
echo

# Compile kernel C files
echo -e "${BLUE}Compiling kernel...${NC}"
C_FILES=$(find kernel -name "*.c")
ASM_FILES=$(find kernel -name "*.asm" | grep -v "boot.asm")
OBJ_FILES=""

# Compile C files
for file in $C_FILES; do
    obj_file="$BUILD_DIR/$(basename ${file%.c}.o)"
    echo "Compiling $file..."
    gcc -c $file -o $obj_file -ffreestanding -O2 -Wall -Wextra -I kernel/include
    OBJ_FILES="$OBJ_FILES $obj_file"
done

# Compile ASM files
for file in $ASM_FILES; do
    obj_file="$BUILD_DIR/$(basename ${file%.asm}.o)"
    echo "Assembling $file..."
    nasm -f elf64 $file -o $obj_file
    OBJ_FILES="$OBJ_FILES $obj_file"
done

echo -e "${GREEN}Kernel compiled.${NC}"
echo

# Create modules directory if it doesn't exist
if [ ! -d "modules" ]; then
    echo -e "${BLUE}Creating modules directory structure...${NC}"
    mkdir -p modules/gui
    mkdir -p modules/nlp
    mkdir -p modules/dl_framework
    mkdir -p modules/model_loader
    mkdir -p modules/shell
    echo -e "${GREEN}Modules directory structure created.${NC}"
    echo
fi

# Compile modules
echo -e "${BLUE}Compiling modules...${NC}"
MODULE_DIRS="modules/gui modules/nlp modules/dl_framework modules/model_loader modules/shell"
for dir in $MODULE_DIRS; do
    if [ -d "$dir" ]; then
        MODULE_C_FILES=$(find $dir -name "*.c" 2>/dev/null)
        for file in $MODULE_C_FILES; do
            obj_file="$BUILD_DIR/$(basename ${file%.c}.o)"
            echo "Compiling $file..."
            gcc -c $file -o $obj_file -ffreestanding -O2 -Wall -Wextra -I kernel/include -I modules
            OBJ_FILES="$OBJ_FILES $obj_file"
        done
    fi
done
echo -e "${GREEN}Modules compiled.${NC}"
echo

# Link everything together
echo -e "${BLUE}Linking kernel...${NC}"
ld -T kernel/linker.ld -o $BOOT_DIR/neuroos.bin $BUILD_DIR/boot.o $OBJ_FILES
echo -e "${GREEN}Kernel linked.${NC}"
echo

# Create GRUB configuration
echo -e "${BLUE}Creating GRUB configuration...${NC}"
cat > $GRUB_DIR/grub.cfg << 'GRUBCFG'
set timeout=0
set default=0

menuentry "NeuroOS" {
    multiboot /boot/neuroos.bin
    boot
}
GRUBCFG
echo -e "${GREEN}GRUB configuration created.${NC}"
echo

# Create ISO
echo -e "${BLUE}Creating ISO image...${NC}"
grub-mkrescue -o neuroos.iso $ISO_DIR
echo -e "${GREEN}ISO image created: neuroos.iso${NC}"
echo

# Clean up userland directory as requested
if [ -d "userland" ]; then
    echo -e "${BLUE}Removing userland directory as requested...${NC}"
    rm -rf userland
    echo -e "${GREEN}Userland directory removed.${NC}"
    echo
fi

# Verify model files
echo -e "${BLUE}Verifying model files...${NC}"
if [ -d "deepseek" ] && [ -f "deepseek/model.safetensors" ] && [ -f "deepseek/tokenizer.json" ]; then
    echo -e "${GREEN}Model files verified.${NC}"
else
    echo -e "${YELLOW}Warning: Some model files may be missing. Please ensure all required files are in the deepseek directory.${NC}"
fi
echo

echo -e "${GREEN}Build completed successfully!${NC}"
echo -e "${BLUE}To run NeuroOS in QEMU, use: qemu-system-x86_64 -cdrom neuroos.iso${NC}"
