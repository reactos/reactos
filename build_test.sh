#!/bin/bash
set -e

echo "Building ReactOS with debug fixes..."
cd /home/ahmed/WorkDir/TTE/reactos

# Clean and recreate build directory
rm -rf build_test
mkdir build_test
cd build_test

# Configure with CMake for AMD64
echo "Configuring..."
cmake .. -G Ninja -DARCH=amd64 -DCMAKE_BUILD_TYPE=Debug

# Build the kernel
echo "Building ntoskrnl..."
ninja ntoskrnl || true

# Find the ISO if any
echo "Looking for ISO files..."
find . -name "*.iso" 2>/dev/null || echo "No ISO found in build directory"

echo "Build attempt complete."