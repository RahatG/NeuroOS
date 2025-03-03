#!/bin/bash

# NeuroOS ISO Generator Script
# This script generates an ISO image of NeuroOS

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
echo -e "${GREEN}NeuroOS ISO Generator${NC}"
echo

# Check for required tools
echo -e "${BLUE}Checking for required tools...${NC}"
for tool in grub-mkrescue xorriso; do
    if ! command -v $tool &> /dev/null; then
        echo -e "${RED}Error: $tool is not installed. Please install it and try again.${NC}"
        exit 1
    fi
done
echo -e "${GREEN}All required tools are installed.${NC}"
echo

# Build directories
BUILD_DIR="build"
ISO_DIR="$BUILD_DIR/iso"
BOOT_DIR="$ISO_DIR/boot"
GRUB_DIR="$BOOT_DIR/grub"

# Create build directories
echo -e "${BLUE}Creating build directories...${NC}"
mkdir -p $BOOT_DIR
mkdir -p $GRUB_DIR
echo -e "${GREEN}Build directories created.${NC}"
echo

# Check if kernel binary exists
echo -e "${BLUE}Checking for kernel binary...${NC}"
if [ ! -f "$BOOT_DIR/neuroos.bin" ]; then
    echo -e "${RED}Error: Kernel binary not found. Please build the kernel first.${NC}"
    exit 1
fi
echo -e "${GREEN}Kernel binary found.${NC}"
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

echo -e "${GREEN}ISO generation completed successfully!${NC}"
echo -e "${BLUE}To run NeuroOS in QEMU use: qemu-system-x86_64 -cdrom neuroos.iso${NC}"
