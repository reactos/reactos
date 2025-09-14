# AMD64 Boot Driver Loading Issue - 2025-09-13

## Problem Identified
ReactOS AMD64 fails to boot with IO1_INITIALIZATION_FAILED (0x69) because critical storage drivers are not being loaded.

## Root Cause
AMD64 drivers are being built with MajorOperatingSystemVersion = 4.0 instead of 5.1, causing the kernel to skip them as "NT 4 drivers" during boot.

### Evidence
1. Kernel message: "Skipping NT 4 driver @ FFFFF880055B1000" (and many others)
2. objdump shows AMD64 pciide.sys has MajorOSystemVersion = 4
3. objdump shows i386 pciide.sys has MajorOSystemVersion = 5

### Root Cause Location
File: `/home/ahmed/WorkDir/reactos_backup_orig/sdk/cmake/gcc.cmake`
Lines: 397-400
```cmake
# Disable for amd64 builds due to binutils segfault bug
if(NOT ARCH STREQUAL "amd64")
    target_link_options(${MODULE} PRIVATE
        -Wl,--major-image-version,5 -Wl,--minor-image-version,01 -Wl,--major-os-version,5 -Wl,--minor-os-version,01)
endif()
```

## Fix Strategy
1. Re-enable the version settings for AMD64 builds
2. Test if the binutils segfault bug still exists
3. If bug persists, find an alternative way to set the versions

## Affected Drivers
All boot-critical drivers including:
- pciide.sys
- pciidex.sys
- atapi.sys
- pci.sys
- And many others

## Test Results
- Before fix: Boot fails with IO1_INITIALIZATION_FAILED
- After fix: Still fails - drivers are being skipped

## Additional Findings
The issue is more complex than initially thought:
1. We fixed the cmake build system to generate drivers with correct OS version (5.1)
2. The newly built drivers in the ISO have correct version numbers
3. BUT the kernel still skips drivers at boot time

## Why Drivers Are Still Being Skipped
The drivers being skipped are the ones already loaded into memory by the bootloader BEFORE the kernel starts. The bootloader loads these drivers from somewhere else, not from the ISO filesystem. We need to investigate:
1. Where does the bootloader get the boot drivers from?
2. How does the bootloader load drivers into memory?
3. Why are old driver binaries still being used?

## Next Steps
Need to examine the bootloader's driver loading mechanism to ensure it loads the newly built drivers with correct versions.