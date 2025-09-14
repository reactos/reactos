# AMD64 Debug Notes - ARC Names Issue
## Date: 2025-09-13

## Problem Summary
ReactOS AMD64 crashes with Fatal System Error 0x00000069 (IO1_INITIALIZATION_FAILED) during boot because IopCreateArcNames fails with status 0xc0000034 (STATUS_OBJECT_NAME_NOT_FOUND).

## Root Cause
The ARC names creation fails because:
1. No CD-ROM devices are detected (CdRomCount = 0)
2. The system is booting from CD-ROM: `multi(0)disk(0)cdrom(96)`
3. IopCreateArcNamesCd returns STATUS_OBJECT_NAME_NOT_FOUND when no CD-ROM devices are found

## Investigation Findings

### 1. Boot Environment
- System is NOT detecting UEFI boot (FirmwareTypeEfi = 0)
- Using OVMF UEFI firmware but ReactOS thinks it's BIOS boot
- Boot device: `multi(0)disk(0)cdrom(96)`

### 2. Strange Code Execution Issue
**CRITICAL FINDING**: Code added to IopCreateArcNamesCd after line 218 (CdRomCount print) is NOT being executed!
- Debug strings are compiled into the binary (verified with `strings`)
- Strings are present in the ISO's ntoskrnl.exe
- DBG=1 is defined during compilation
- DPRINT1 macros should be working
- BUT: Code immediately after CdRomCount print never executes
- Execution jumps directly from the CdRomCount print to the Cleanup label

### 3. Attempted Fixes
1. **Added early return for CD boot**: If CdRomCount==0 and booting from CD, return SUCCESS
   - Result: Code not executed
2. **Added unconditional debug print**: Simple DPRINT1 after CdRomCount
   - Result: Never appears in output
3. **Verified binary updates**: Confirmed new strings are in the kernel
   - Result: Strings present but code not executing

## Technical Details
```
(/ntoskrnl/io/iomgr/arcname.c:218) AGENT-DEBUG: CdRomCount = 0
(/ntoskrnl/io/iomgr/arcname.c:458) AGENT-DEBUG: IopCreateArcNamesCd exiting with Status=0xc0000034
```
Note: Line numbers in debug output don't match actual file due to __LINE__ being compiled in.

## Hypothesis
There appears to be a fundamental issue with either:
1. **Code optimization**: The compiler might be optimizing out code in a way that causes unexpected jumps
2. **Stack corruption**: Something might be corrupting the stack causing the function to jump to cleanup
3. **Binary loading**: The wrong kernel binary might be loaded at runtime
4. **Memory corruption**: Early memory corruption causing code execution to fail

## Next Steps
1. Investigate why code after line 218 is not executing
2. Check for stack/memory corruption
3. Try alternative approaches to bypass ARC names requirement
4. Consider making IopCreateArcNames non-fatal for all boot scenarios

## Files Modified
- `/ntoskrnl/io/iomgr/arcname.c`: Added CD-ROM detection fixes (not executing)
- `/ntoskrnl/io/iomgr/iomgr.c`: Previously made ARC names non-fatal for UEFI (but UEFI not detected)