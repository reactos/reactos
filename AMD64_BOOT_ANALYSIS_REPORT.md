# ReactOS AMD64 Boot Analysis Report

## Executive Summary

The fundamental issue preventing ReactOS from booting in AMD64 mode is **NOT** that FreeLdr doesn't support long mode. The analysis reveals that FreeLdr actually has comprehensive AMD64 support, but there appears to be a mismatch between the bootloader's execution mode and kernel expectations.

## Key Findings

### 1. FreeLdr DOES Support AMD64 Long Mode

Contrary to the initial assessment, FreeLdr has full AMD64 support:

- **`arch/realmode/amd64.S`**: Contains complete real mode to long mode transition code
- **`arch/amd64/entry.S`**: Implements 64-bit entry point and execution environment  
- **`ntldr/arch/amd64/winldr.c`**: Has AMD64-specific Windows loader implementation

### 2. Long Mode Transition Process

The bootloader performs the following steps to enter long mode:

1. **CPU Verification** (`CheckFor64BitSupport`):
   - Checks for CPUID support
   - Verifies PAE and PGE capabilities
   - Confirms Long Mode availability (CPUID 0x80000001)

2. **Page Table Setup** (`BuildPageTables`):
   - Creates PML4 at 0x1000
   - Sets up PDP and PD structures
   - Maps first 1GB of memory using 2MB pages

3. **Mode Transition** (`ExitToLongMode`):
   - Enables PAE and PGE in CR4
   - Sets up page tables in CR3
   - Enables Long Mode via EFER MSR (bit 8)
   - Activates paging and protection mode simultaneously

4. **Serial Debug Output**:
   - Outputs progress markers: '!', '@', '#', '1'-'5', 'F', 'S', 'B'
   - These markers trace the boot progression through different stages

### 3. The Real Problem: Mode Mismatch

The issue is that the kernel appears to be loaded and executed while still in 32-bit protected mode:

```
CS=0010 is a 32-bit code segment descriptor
```

This suggests one of several possible issues:

1. **Incorrect GDT Setup**: The GDT may not have proper 64-bit code segments
2. **Failed Mode Transition**: The long mode transition might be failing silently
3. **Build Configuration**: The kernel might be built for 32-bit despite PE32+ format
4. **Entry Point Confusion**: The loader might be using a 32-bit entry point

### 4. Kernel Initialization Code

The kernel (`ntoskrnl/ke/amd64/kiinit.c`) expects:
- To be entered in long mode
- Proper segment selectors (LMODE_DS, LMODE_CS)
- Valid PCR (Processor Control Region) setup
- Extensive debug output via COM1 serial port

### 5. PE Loader Capabilities

The PE loader (`lib/peloader.c`) supports:
- Both 32-bit (PE32) and 64-bit (PE32+) executables
- Proper relocation handling
- Import resolution
- Security cookie initialization

## Root Cause Analysis

The boot failure occurs because:

1. **The kernel is correctly compiled as 64-bit** (PE32+ x86-64 format)
2. **FreeLdr has AMD64 support** but may not be entering long mode correctly
3. **The execution environment at kernel entry** shows 32-bit protected mode (CS=0010)

This indicates the problem is in the **transition logic** between FreeLdr's mode switching and kernel handoff.

## Potential Issues to Investigate

### 1. Build System Configuration
- Check if FreeLdr is being built with AMD64 support enabled
- Verify CMake flags for AMD64 compilation
- Ensure proper preprocessor definitions (_M_AMD64, _WIN64)

### 2. Boot Path Selection
- The code has both BIOS and UEFI boot paths
- Different paths may handle AMD64 differently
- UEFI path appears more AMD64-aware

### 3. GDT Segment Descriptors
The GDT in `amd64.S` defines:
```assembly
.word HEX(0000), HEX(0000), HEX(9800), HEX(0020) /* 10: long mode CS */
.word HEX(FFFF), HEX(0000), HEX(F300), HEX(00CF) /* 18: long mode DS */
```
But CS=0010 suggests these aren't being used correctly.

### 4. Entry Point Resolution
The code reads the entry point from the PE header but may not handle the AMD64 calling convention properly.

## Recommendations

1. **Enable Serial Debug Output**: Monitor COM1 (0x3F8) for the debug markers to see where boot fails
2. **Verify Build Configuration**: Ensure CMAKE_SYSTEM_PROCESSOR is set to AMD64
3. **Check Boot Mode**: Determine if booting via BIOS or UEFI path
4. **Trace Mode Transitions**: Add more debug output around the long mode switch
5. **Validate GDT Loading**: Ensure the correct GDT with 64-bit segments is loaded
6. **Review Entry Point**: Verify the kernel entry point is being called with proper mode

## Conclusion

The issue is **not** that FreeLdr lacks AMD64 support - it clearly has extensive 64-bit code. The problem is that despite having this support, the kernel is being entered in 32-bit mode (CS=0010) instead of 64-bit long mode. This points to either a build configuration issue, a failed mode transition, or incorrect boot path selection.

The next debugging steps should focus on:
1. Monitoring serial output to identify where in the boot process it fails
2. Verifying the build system is actually compiling the AMD64 code paths
3. Ensuring the correct boot path (BIOS vs UEFI) is being taken for AMD64