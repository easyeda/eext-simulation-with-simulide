#!/bin/bash
#
# Cross-platform WASM build script for easyeda-simulation-engine
# Compatible with: Windows (MSYS2/Git Bash), Linux, macOS
#

set -e  # Exit on error

# Colors for output
if [ -t 1 ]; then
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    RED='\033[0;31m'
    BLUE='\033[0;34m'
    NC='\033[0m'
else
    GREEN=''
    YELLOW=''
    RED=''
    BLUE=''
    NC=''
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  WASM Build Script${NC}"
echo -e "${BLUE}========================================${NC}"

# Detect OS
OS_TYPE="unknown"
if [[ "$OSTYPE" == "msys"* ]] || [[ "$OSTYPE" == "cygwin"* ]] || [[ "$OSTYPE" == "win32"* ]]; then
    OS_TYPE="windows"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS_TYPE="macos"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS_TYPE="linux"
fi

echo -e "${YELLOW}Detected OS: $OS_TYPE${NC}"

# Get script directory (works cross-platform)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_DIR/build-wasm"

echo -e "${YELLOW}Project directory: $PROJECT_DIR${NC}"

# Configure EMSDK path based on OS
if [ -z "$EMSDK" ]; then
    case "$OS_TYPE" in
        windows)
            # Try common Windows locations
            if [ -d "/f/Program Files (x86)/emsdk/emsdk" ]; then
                EMSDK_PATH="/f/Program Files (x86)/emsdk/emsdk"
            elif [ -d "/c/emsdk" ]; then
                EMSDK_PATH="/c/emsdk"
            elif [ -d "$HOME/emsdk" ]; then
                EMSDK_PATH="$HOME/emsdk"
            else
                echo -e "${RED}Error: Could not find emsdk. Please set EMSDK environment variable.${NC}"
                echo -e "${YELLOW}Example: export EMSDK=\"/f/Program Files (x86)/emsdk/emsdk\"${NC}"
                exit 1
            fi
            ;;
        macos|linux)
            # Try common Unix locations
            if [ -d "$HOME/emsdk" ]; then
                EMSDK_PATH="$HOME/emsdk"
            elif [ -d "/opt/emsdk" ]; then
                EMSDK_PATH="/opt/emsdk"
            else
                echo -e "${RED}Error: Could not find emsdk. Please set EMSDK environment variable.${NC}"
                echo -e "${YELLOW}Example: export EMSDK=\"$HOME/emsdk\"${NC}"
                exit 1
            fi
            ;;
    esac
else
    EMSDK_PATH="$EMSDK"
fi

echo -e "${YELLOW}EMSDK path: $EMSDK_PATH${NC}"

# Find node directory (get the latest version)
NODE_DIR=$(ls -d "$EMSDK_PATH"/node/*_64bit 2>/dev/null | sort -V | tail -1)
if [ -z "$NODE_DIR" ]; then
    echo -e "${RED}Error: Could not find node in emsdk!${NC}"
    exit 1
fi

echo -e "${YELLOW}Node directory: $NODE_DIR${NC}"

# Set up Emscripten environment
echo -e "${GREEN}Setting up Emscripten environment...${NC}"
export EMSDK="$EMSDK_PATH"
export EM_CONFIG="$EMSDK/.emscripten"
export PATH="$EMSDK/upstream/emscripten:$NODE_DIR/bin:$PATH"

# Verify Emscripten is available
echo -e "${GREEN}Verifying Emscripten installation...${NC}"
if ! python "$EMSDK/upstream/emscripten/emcc.py" --version > /dev/null 2>&1; then
    echo -e "${RED}Failed to load Emscripten!${NC}"
    echo -e "${YELLOW}Please ensure emsdk is properly installed and activated.${NC}"
    exit 1
fi

EMCC_VERSION=$(python "$EMSDK/upstream/emscripten/emcc.py" --version | head -1)
echo -e "${GREEN}✓ Emscripten found: $EMCC_VERSION${NC}"

# Create build directory
echo -e "${GREEN}Creating build directory...${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Clean previous build if requested
if [ "$1" == "clean" ]; then
    echo -e "${YELLOW}Cleaning previous build...${NC}"
    rm -rf ./*
fi

# Configure with CMake
echo -e "${GREEN}Configuring CMake...${NC}"
python "$EMSDK/upstream/emscripten/emcmake.py" cmake .. \
    -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ CMake configuration failed!${NC}"
    exit 1
fi

echo -e "${GREEN}✓ CMake configuration successful${NC}"

# Determine number of CPU cores for parallel build
if [ "$OS_TYPE" == "macos" ] || [ "$OS_TYPE" == "linux" ]; then
    CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
else
    CORES=$(nproc 2>/dev/null || echo 4)
fi

echo -e "${GREEN}Building WASM with $CORES parallel jobs...${NC}"
make -j"$CORES"

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Build completed successfully!${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Output files:${NC}"
ls -lh lceda-pro-sim-server.* 2>/dev/null || echo -e "${RED}No output files found!${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}Build location: $BUILD_DIR${NC}"
