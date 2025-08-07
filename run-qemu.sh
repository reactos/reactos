#!/bin/bash

# Clear or create the log file first
> ReactOS.log

# Start QEMU with monitor on stdio or unix socket
qemu-system-x86_64 \
    -enable-kvm \
    -cpu host \
    -m 2G \
    -drive if=ide,index=0,media=disk,file=ReactOS.qcow2 \
    -drive if=ide,index=2,media=cdrom,file=livecd.iso \
    -boot order=d \
    -rtc base=localtime \
    -serial stdio \
    -monitor unix:/tmp/qemu-monitor.sock,server,nowait 2>&1 | tee ReactOS.log &

# Store QEMU PID
QEMU_PID=$!

# Wait 3 seconds
sleep 3

# Send Enter key via monitor
echo "sendkey ret" | socat - UNIX-CONNECT:/tmp/qemu-monitor.sock

# Wait for the specific string to appear in the log file
echo "Waiting for ReactOS to boot..."
tail -f ReactOS.log | while read line; do
    if echo "$line" | grep -q "(/ntoskrnl/kd64/kdinit.c:95) ReactOS 0.4.16-amd64-dev"; then
        echo "ReactOS boot string detected!"
        break
    fi
done

# Wait additional 10 seconds
echo "Waiting 10 more seconds..."
sleep 10

# Gracefully shutdown QEMU via monitor
echo "Shutting down QEMU..."
echo "quit" | socat - UNIX-CONNECT:/tmp/qemu-monitor.sock

# Give it a moment to shut down gracefully
sleep 1

# If still running, force kill
if kill -0 $QEMU_PID 2>/dev/null; then
    echo "Force killing QEMU..."
    kill -9 $QEMU_PID
fi

echo "Done!"