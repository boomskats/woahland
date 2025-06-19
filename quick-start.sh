#!/bin/bash
# woahland quick-start script
# Downloads SDK and builds the head tracking mouse control
# Run this from the root of the cloned woahland repository

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
SDK_URL="https://static.viture.dev/external-file/sdk/viture_linux_sdk_v1.0.7.tar.xz"
SDK_TARBALL="viture_linux_sdk_v1.0.7.tar.xz"
SDK_DIR="viture_one_linux_sdk_1.0.7"

echo -e "${GREEN}🚀 woahland Quick Start${NC}"
echo "Setting up head tracking mouse control for Viture glasses..."
echo

# Check if we're in the right directory
if [ ! -f "head_mouse_wayland.c" ] || [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}Error: This script must be run from the woahland repository root${NC}"
    echo "Usage:"
    echo "  git clone https://github.com/yourusername/woahland.git"
    echo "  cd woahland"
    echo "  ./quick-start.sh"
    exit 1
fi

# Check for required dependencies
echo "Checking dependencies..."
MISSING_DEPS=()

# Check build tools
command -v cmake >/dev/null 2>&1 || MISSING_DEPS+=("cmake")
command -v gcc >/dev/null 2>&1 || MISSING_DEPS+=("gcc")
command -v g++ >/dev/null 2>&1 || MISSING_DEPS+=("g++")
command -v make >/dev/null 2>&1 || MISSING_DEPS+=("make")
command -v curl >/dev/null 2>&1 && command -v wget >/dev/null 2>&1 || MISSING_DEPS+=("curl or wget")

# Check X11 development libraries
if ! pkg-config --exists x11 2>/dev/null; then
    MISSING_DEPS+=("libx11-dev")
fi

if ! pkg-config --exists xtst 2>/dev/null; then
    MISSING_DEPS+=("libxtst-dev")
fi

if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
    echo -e "${RED}Missing dependencies:${NC}"
    for dep in "${MISSING_DEPS[@]}"; do
        echo "  - $dep"
    done
    echo
    echo "Please install missing dependencies:"
    echo "  Ubuntu/Debian: sudo apt install cmake build-essential libx11-dev libxtst-dev curl"
    echo "  Fedora: sudo dnf install cmake gcc gcc-c++ make libX11-devel libXtst-devel curl"
    echo "  Arch: sudo pacman -S cmake base-devel libx11 libxtst curl"
    exit 1
fi

echo -e "${GREEN}✓ All dependencies found${NC}"
echo

# Clean existing build directory if it exists
if [ -d "build" ]; then
    echo "Cleaning existing build directory..."
    rm -rf build
fi

# Create temporary directory for SDK
WORK_DIR=".viture-sdk-temp"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

# Download SDK if not present
if [ ! -f "$SDK_TARBALL" ]; then
    echo "Downloading Viture SDK..."
    if command -v curl >/dev/null 2>&1; then
        curl -L -o "$SDK_TARBALL" "$SDK_URL"
    else
        wget -O "$SDK_TARBALL" "$SDK_URL"
    fi
    echo -e "${GREEN}✓ SDK downloaded${NC}"
else
    echo -e "${YELLOW}SDK already downloaded, skipping...${NC}"
fi

# Extract SDK
if [ ! -d "$SDK_DIR" ]; then
    echo "Extracting SDK..."
    tar xf "$SDK_TARBALL"
    echo -e "${GREEN}✓ SDK extracted${NC}"
else
    echo -e "${YELLOW}SDK already extracted, skipping...${NC}"
fi

cd "$SDK_DIR"

# Copy SDK files back to repo root for building
echo "Setting up build environment..."
REPO_ROOT="../.."

# Copy SDK includes and libs to repo
mkdir -p "$REPO_ROOT/include"
mkdir -p "$REPO_ROOT/libs"
cp -r include/* "$REPO_ROOT/include/"
cp -r libs/* "$REPO_ROOT/libs/"

# Go back to repo root for building
cd "$REPO_ROOT"

echo -e "${GREEN}✓ Build environment ready${NC}"

# Build the project
echo "Building woahland..."
mkdir -p build
cd build
cmake ..
make -j$(nproc)

echo -e "${GREEN}✓ Build complete!${NC}"
echo

# Check permissions
echo "Checking permissions..."
if groups | grep -q input && [ -r /dev/uinput ] && [ -w /dev/uinput ]; then
    echo -e "${GREEN}✓ Permissions already configured${NC}"
else
    echo -e "${YELLOW}⚠ Permissions not configured${NC}"
    echo "Would you like to set up permissions now? This will:"
    echo "  - Add your user to the 'input' group"
    echo "  - Install a udev rule for /dev/uinput access"
    echo "  - Require logout/login to take effect"
    echo
    read -p "Set up permissions? [Y/n] " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]] || [[ -z $REPLY ]]; then
        chmod +x setup-permissions.sh
        ./setup-permissions.sh
    fi
fi

echo
echo -e "${GREEN}🎉 Installation complete!${NC}"
echo
echo "Binaries have been built in: build/"
echo
echo "To run woahland:"
echo "  ./build/run_head_mouse_wayland.sh    # For Wayland"
echo "  ./build/run_head_mouse_x11.sh        # For X11"
echo
echo "Control commands:"
echo "  ./build/viture-mouse-ctl toggle      # Toggle on/off"
echo "  ./build/viture-mouse-ctl recenter    # Recenter cursor"
echo "  ./build/viture-mouse-ctl status      # Check status"
echo
echo "For more information, see README.md"
echo
echo -e "${YELLOW}woah, dude... your head is now a mouse 🤯${NC}"