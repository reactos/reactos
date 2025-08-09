#!/bin/bash

echo "=== ReactOS AMD64 Desktop Boot Test ==="
echo "Building with full debugging..."

# Build with debugging
ninja ntoskrnl || exit 1
ninja bootcd || exit 1

echo "Starting QEMU test..."
timeout 30 qemu-system-x86_64 \
    -m 2048 \
    -cpu qemu64 \
    -cdrom bootcd.iso \
    -serial stdio \
    -display none \
    2>&1 | tee boot_test.log

echo
echo "=== Analyzing Boot Log ==="
echo

echo "1. Checking for SMSS loading:"
grep -E "Loading SMSS|smss\.exe|RtlCreateUserProcess|ZwOpenFile" boot_test.log | head -10

echo
echo "2. Checking for errors:"
grep -E "ERROR|FAILED|Cannot open|0xC0000" boot_test.log | head -10

echo
echo "3. Checking for process creation:"
grep -E "Process created|PID|Thread created" boot_test.log | head -10

echo
echo "4. Checking for file system:"
grep -E "CDFS|ISO|File system|IoInitSystem" boot_test.log | head -10

echo
echo "=== Test Complete ==="