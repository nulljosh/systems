#!/bin/bash
# NullOS Build Script
# Runs the full build pipeline with dependency checks.

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "NullOS Build"
echo "============"

# Check dependencies
MISSING=0

check_dep() {
    if ! command -v "$1" &> /dev/null; then
        echo -e "${RED}Missing: $1${NC} ($2)"
        MISSING=1
    else
        echo -e "${GREEN}Found:   $1${NC}"
    fi
}

echo ""
echo "Checking toolchain..."
check_dep "nasm" "brew install nasm"
check_dep "i686-elf-gcc" "brew tap nativeos/i686-elf-toolchain && brew install i686-elf-gcc"
check_dep "i686-elf-ld" "included with i686-elf-gcc"
check_dep "qemu-system-i386" "brew install qemu"

if [ $MISSING -eq 1 ]; then
    echo ""
    echo -e "${RED}Install missing dependencies and try again.${NC}"
    exit 1
fi

echo ""
echo "Building..."
make clean
make

echo ""
echo -e "${GREEN}Build complete.${NC}"
echo "Run with: make run"
