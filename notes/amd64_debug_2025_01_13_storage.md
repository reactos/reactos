# ReactOS AMD64 Storage Detection Issues - 2025-01-13

## Problem Summary
ReactOS AMD64 fails to boot with IO1_INITIALIZATION_FAILED (0x69) because no storage devices are detected. The system expects to boot from `multi(0)disk(0)cdrom(96)` but finds:
- DiskCount = 0
- CdRomCount = 0

## Root Causes Identified

### 1. PCI Slot 0x21 Access Violation
- **Location**: Device 1, Function 1 (slot 0x21)
- **Impact**: Accessing this slot causes access violation at 0xFFFFF8800549B065
- **Workaround**: Block access to slot 0x21 to prevent crashes
- **Side Effect**: This slot typically contains the PIIX3 IDE controller in QEMU

### 2. Incomplete PCI Device Enumeration
- **Issue**: Original PCI scan only checked function 0 of each device
- **Fixed**: Modified scan to check all 8 functions per device
- **Problem**: Still no devices detected during HalpInitializePciStubs

### 3. Storage Driver Loading
- **Expected**: IDE/ATAPI drivers should load for QEMU's default storage
- **Actual**: BusLogic driver attempts to load and crashes
- **Configuration**: QEMU uses `-cdrom livecd.iso` which attaches to IDE controller

## Technical Analysis

### PCI Slot Encoding
```c
typedef struct _PCI_SLOT_NUMBER {
    union {
        struct {
            ULONG DeviceNumber:5;    // Bits 0-4
            ULONG FunctionNumber:3;   // Bits 5-7
            ULONG Reserved:24;        // Bits 8-31
        } bits;
        ULONG AsULONG;
    } u;
} PCI_SLOT_NUMBER;
```
- Slot 0x21 = Device 1, Function 1

### QEMU Default Configuration
- Uses PIIX3 chipset emulation
- IDE controller at Device 1, Function 1
- CD-ROM attached to IDE controller via `-cdrom` parameter

## Attempted Fixes

1. **Block slot 0x21 entirely**: Prevents crash but also blocks IDE detection
2. **Allow limited access to slot 0x21**: Tried to allow vendor ID read only
3. **Fix PCI enumeration**: Added proper multi-function device scanning
4. **Prevent BusLogic loading**: Blocked slot 0x21 prevents BusLogic crash

## Current Status
- System boots past PCI initialization
- No storage devices detected
- Fails at IopCreateArcNames with STATUS_OBJECT_NAME_NOT_FOUND

## Next Steps
1. Find alternative way to detect IDE controller without accessing slot 0x21
2. Force-load IDE drivers even without PCI detection
3. Consider using a different storage backend (virtio, AHCI)
4. Debug why slot 0x21 causes access violations in AMD64 but not i386

## Comparison with i386
- i386 build works correctly with same QEMU configuration
- i386 can access slot 0x21 without crashes
- Suggests AMD64-specific issue with:
  - Memory alignment
  - Pointer sizes
  - Calling conventions
  - Stack handling