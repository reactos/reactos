#!/bin/bash

# ReactOS AMD64 UEFI Boot Test Script
# Tests the full UEFI GOP and desktop boot implementation

echo "=== ReactOS AMD64 UEFI Boot Test ==="
echo

# Build ReactOS with UEFI support
echo "Building ReactOS with UEFI support..."
export UEFI_BUILD=1
ninja -C output-MinGW-amd64 ntoskrnl freeldr bootvid || exit 1

echo "Building boot CD..."
ninja -C output-MinGW-amd64 bootcd || exit 1

# Check if OVMF firmware exists
if [ ! -f "/usr/share/ovmf/OVMF.fd" ] && [ ! -f "/usr/share/OVMF/OVMF.fd" ]; then
    echo "ERROR: OVMF UEFI firmware not found!"
    echo "Please install OVMF package (apt install ovmf / dnf install edk2-ovmf)"
    exit 1
fi

OVMF_PATH="/usr/share/ovmf/OVMF.fd"
[ -f "/usr/share/OVMF/OVMF.fd" ] && OVMF_PATH="/usr/share/OVMF/OVMF.fd"

# Run QEMU with UEFI
echo "Starting QEMU with UEFI firmware..."
echo "Press Ctrl+C to stop"
echo

qemu-system-x86_64 \
    -bios "$OVMF_PATH" \
    -m 2048 \
    -cpu qemu64 \
    -smp 1 \
    -vga std \
    -cdrom output-MinGW-amd64/bootcd.iso \
    -serial file:debug.log \
    -monitor stdio \
    -d int,cpu_reset \
    -D qemu_trace.log \
    -boot d \
    -no-reboot \
    -no-shutdown

echo
echo "=== Boot Log Analysis ==="
if [ -f debug.log ]; then
    echo "Checking for key boot messages..."
    grep -E "(UEFI|GOP|Framebuffer|DESKTOP READY|smss\.exe|csrss\.exe|explorer\.exe)" debug.log | tail -20
    
    echo
    echo "Checking for errors..."
    grep -E "(ERROR|FAILED|Exception|BUGCHECK)" debug.log | tail -10
fi

echo
echo "=== Test Complete ==="
echo "Check debug.log for full output"