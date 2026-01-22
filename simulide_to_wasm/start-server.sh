#!/bin/bash
#
# Server launcher for macOS and Linux
# Compatible with: Linux, macOS, Windows (Git Bash/MSYS2)
#

set -e

# Colors for output
if [ -t 1 ]; then
    GREEN='\033[0;32m'
    RED='\033[0;31m'
    BLUE='\033[0;34m'
    NC='\033[0m'
else
    GREEN=''
    RED=''
    BLUE=''
    NC=''
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Starting WASM Test Server${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Check if Python is available
if ! command -v python3 &> /dev/null && ! command -v python &> /dev/null; then
    echo -e "${RED}Error: Python is not installed or not in PATH${NC}"
    echo -e "Please install Python 3 from https://www.python.org/"
    exit 1
fi

# Use python3 if available, otherwise python
PYTHON_CMD="python3"
if ! command -v python3 &> /dev/null; then
    PYTHON_CMD="python"
fi

# Get script directory and change to it
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Start the server
echo -e "${GREEN}Starting server with $PYTHON_CMD...${NC}"
$PYTHON_CMD serve.py
