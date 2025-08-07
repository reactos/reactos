#!/bin/bash
# Runtime fix for ReactOS AMD64 msvcrt library
# This script combines the msvcrt import library with CRT startup code

set -e

if [ ! -f "CMakeCache.txt" ]; then
    echo "Error: This script must be run from the ReactOS build directory"
    exit 1
fi

echo "Fixing msvcrt library with CRT startup code..."

# Check if files exist
if [ ! -f "sdk/lib/crt/libmsvcrt_startup.a" ]; then
    echo "Error: libmsvcrt_startup.a not found. Build may not have progressed far enough."
    exit 1
fi

if [ ! -f "dll/win32/msvcrt/libmsvcrt.a" ]; then
    echo "Error: libmsvcrt.a not found. Build may not have progressed far enough."
    exit 1
fi

# Create temporary directory
TEMP_DIR="temp_msvcrt_fix_$$"
mkdir -p "$TEMP_DIR"

# Extract and combine libraries
echo "Extracting libraries..."
AR_TOOL="/home/ahmed/x-tools/x86_64-w64-mingw32/bin/x86_64-w64-mingw32-ar"

$AR_TOOL x sdk/lib/crt/libmsvcrt_startup.a --output="$TEMP_DIR"
$AR_TOOL x dll/win32/msvcrt/libmsvcrt.a --output="$TEMP_DIR"

echo "Creating combined library..."
$AR_TOOL rcs dll/win32/msvcrt/libmsvcrt_fixed.a "$TEMP_DIR"/*.o "$TEMP_DIR"/*.obj 2>/dev/null

# Backup and replace
if [ ! -f "dll/win32/msvcrt/libmsvcrt_original.a" ]; then
    mv dll/win32/msvcrt/libmsvcrt.a dll/win32/msvcrt/libmsvcrt_original.a
fi
mv dll/win32/msvcrt/libmsvcrt_fixed.a dll/win32/msvcrt/libmsvcrt.a

# Clean up
rm -rf "$TEMP_DIR"

echo "Successfully fixed msvcrt library!"
echo "You can now continue the build with: ninja livecd -j32"