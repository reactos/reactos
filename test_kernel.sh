#!/bin/bash
echo "Starting kernel test..."
timeout 10 qemu-system-x86_64 \
    -cdrom output-MinGW-amd64/livecd.iso \
    -m 256 \
    -serial stdio \
    -bios /usr/share/ovmf/OVMF.fd \
    -no-reboot \
    -display none 2>&1 | \
    grep -a "KERNEL:" | \
    grep -v "freeldr" | \
    tail -100
echo "Test complete."