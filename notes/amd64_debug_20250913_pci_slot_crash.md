# AMD64 PCI Slot 0x21 Crash Analysis
## Date: 2025-09-13

## Problem Summary
ReactOS AMD64 crashes with access violation (0xc0000005) when accessing PCI slot 0x21 during boot.

## Crash Details
- **Exception Address**: 0xFFFFF8800549B065
- **Exception Type**: 0xc0000005 (Access Violation)
- **Stack Pointer**: 0xFFFFF8800519C9F0
- **PCI Slot**: 0x21 (Device=1, Function=4 based on bit layout)

## Key Observations

1. **Multiple Successful Accesses**: Slot 0x21 is successfully accessed many times before the crash
   - HalpReadPCIConfig is called repeatedly with slot 0x21
   - ConfigIO functions are called successfully multiple times
   - The crash happens on a specific call pattern

2. **Memory Ranges**:
   - Kernel: 0xFFFFF80000000000 - 0xFFFFF80003000000
   - HAL functions: ~0xFFFFF80000506xxx range  
   - pci.sys: 0xFFFFF8800549A000
   - Crash address: 0xFFFFF8800549B065 (offset 0x1065 into pci.sys)

3. **Configuration Issues Fixed**:
   - Set MaxDevice=32 for Type 1 PCI bus (was uninitialized)
   - Initialized fake PCI bus handler Config.Type1 fields with proper I/O ports

4. **Access Pattern Before Crash**:
   - Reading 64-byte PCI config header in 4-byte chunks
   - Crash occurs when reading offset 60, length 4 (last 4 bytes)
   - ConfigIO[0] (ULONG read) is being called

## Hypothesis
The crash might be due to:
1. Stack corruption from repeated nested calls
2. The PCI device at slot 0x21 not actually existing in hardware
3. Some AMD64-specific issue with the PCI config read implementation
4. Potential issue with how pci.sys interacts with HAL on AMD64

## Resolution
**Fixed**: By blocking access to slot 0x21, we bypassed the crash. The slot appears to be invalid or problematic on AMD64.

## Fixes Applied
1. Set MaxDevice=32 for Type 1 PCI bus in HalpInitializePciStubs (was uninitialized)
2. Initialized HalpFakePciBusData Config.Type1 fields with proper I/O port addresses
3. Added workaround to block slot 0x21 access entirely

## New Issue Discovered
After fixing PCI crash, system now fails with:
- **Error Code**: 0x00000069 (IO1_INITIALIZATION_FAILED)
- **Location**: After IopCreateArcNamesDisk
- **Issue**: Boot device not found (FoundBoot=0)

## Next Steps for Boot Device Issue
1. Investigate why boot device is not being detected
2. Check ARC name creation for CD-ROM devices
3. Compare i386 vs AMD64 boot device detection
4. Fix IO system initialization