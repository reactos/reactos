# AMD64 Storage Driver Investigation - 2025-01-13

## Current Status
- PCI slot 0x21 is now accessible (unblocked previous crash-causing block)
- Device at slot 0x21 confirmed as Intel PIIX3 IDE controller (VendorID=0x8086, DeviceID=0x7010)
- System fails with IO1_INITIALIZATION_FAILED (0x69) - no storage devices found
- BusLogic driver fails to initialize (expected - not present in QEMU)
- IDE/PCI drivers not loading during boot phase

## Key Findings

### PCI Access Working
- Slot 0x21 (Device 1, Function 1) is being accessed successfully
- PCI configuration reads are working correctly
- No alignment issues detected in buffer access

### IDE Controller Detection Issue
- IDE controller is present and readable via PCI
- pciide.sys driver exists in the system
- Driver should load for PCI class 0x0101 (IDE controller)
- No evidence of pciide driver initialization in boot logs

## Next Steps
1. ~~Check why pciide driver isn't being loaded~~ - No PnP enumeration happening
2. ~~Verify PCI class/subclass is being read correctly~~ - Confirmed correct (0x01, 0x01, 0x80)
3. Find where boot-critical drivers are loaded during early boot
4. Add pci.sys, pciide.sys, and atapi.sys to boot driver list

## Files Modified
- `/hal/halx86/legacy/bus/pcibus.c` - Added PCI ID and class code logging for slot 0x21
- `/boot/bootdata/hivesys.inf` - Added registry entries for pci, pciide, pciidex, and atapi drivers

## Working Hypothesis
The IDE controller is detected at the HAL level but no storage drivers are loading because:
1. The PCI bus driver (pci.sys) is not being loaded as a boot-critical driver
2. Without PCI bus driver, no PnP device enumeration occurs
3. Without PnP enumeration, IDE drivers (pciide.sys, atapi.sys) cannot be matched to devices
4. The boot driver loading list needs to be updated for AMD64 to include these drivers

## Root Cause
The system is trying to initialize storage before the PnP subsystem is ready. The boot-critical drivers need to be loaded by the bootloader or early kernel initialization, not through PnP.