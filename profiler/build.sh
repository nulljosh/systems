#!/bin/bash
# Build script for CPU Profiler

set -e

echo "🔨 Building CPU Profiler..."
echo ""

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Check if Makefile exists
if [ ! -f "Makefile" ]; then
    echo -e "${RED}✗ Makefile not found${NC}"
    exit 1
fi

# Check dependencies
echo -e "${BLUE}Checking dependencies...${NC}"

if ! command -v clang &> /dev/null; then
    echo -e "${RED}✗ clang not found. Install Xcode tools with: xcode-select --install${NC}"
    exit 1
fi
echo -e "${GREEN}✓ clang${NC}"

if ! command -v python3 &> /dev/null; then
    echo -e "${RED}✗ python3 not found${NC}"
    exit 1
fi
echo -e "${GREEN}✓ python3${NC}"

# Build the C extension
echo ""
echo -e "${BLUE}Building C extension...${NC}"
make clean > /dev/null 2>&1 || true
make build 2>&1 || make 2>&1

# Verify library was created
if [ ! -f "libprofiler.dylib" ] && [ ! -f "libprofiler.so" ]; then
    echo -e "${RED}✗ Failed to build library${NC}"
    exit 1
fi

LIBNAME="libprofiler.dylib"
if [ -f "libprofiler.so" ]; then
    LIBNAME="libprofiler.so"
fi

echo -e "${GREEN}✓ Built ${LIBNAME} ($(stat -f%z $LIBNAME 2>/dev/null || stat -c%s $LIBNAME) bytes)${NC}"

# Make Python files executable
chmod +x profiler_cli.py profiler_binding.py example_usage.py 2>/dev/null || true

echo ""
echo -e "${BLUE}Verifying installation...${NC}"

# Quick Python import test
python3 -c "from profiler_binding import CPUProfiler, load_profiler_lib; print('✓ Python imports OK')" 2>/dev/null || {
    echo -e "${RED}✗ Python import test failed${NC}"
    exit 1
}

echo ""
echo -e "${GREEN}✅ Build successful!${NC}"
echo ""
echo "Next steps:"
echo "  1. Try an example:      python3 example_usage.py 3"
echo "  2. Run the CLI:         python3 profiler_cli.py run <script.py>"
echo "  3. Read the docs:       cat README.md"
echo ""
