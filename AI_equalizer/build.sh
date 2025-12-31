#!/bin/bash

# AI Equalizer Build Script

echo "Building AI Equalizer..."

# Check if Qt6 is available
if ! command -v qmake6 &> /dev/null && ! command -v qmake &> /dev/null; then
    echo "Error: Qt6 not found. Please install Qt6 development packages."
    echo "Ubuntu/Debian: sudo apt install qt6-base-dev qt6-multimedia-dev"
    echo "Fedora: sudo dnf install qt6-qtbase-devel qt6-qtmultimedia-devel"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

# Run CMake
echo "Running CMake..."
cmake .. || { echo "CMake configuration failed"; exit 1; }

# Build
echo "Compiling..."
make -j$(nproc) || { echo "Build failed"; exit 1; }

echo ""
echo "Build successful!"
echo ""
echo "To run the application:"
echo "  1. Set your Google Gemini API key:"
echo "     export GEMINI_API_KEY='your_api_key_here'"
echo ""
echo "  2. Run the application:"
echo "     ./AI_equalizer"
echo ""
