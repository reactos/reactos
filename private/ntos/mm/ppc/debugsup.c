/*++

Copyright (c) 1989  Microsoft Corporation
Copyright (c) 1993  IBM Corporation

Module Name:

   debugsup.c

Abstract:

    This module contains routines which provide support for the
    kernel debugger.

Author:

    Lou Perazzoli (loup) 02-Aug-90

    Modified for PowerPC by Mark Mergen (mergen@watson.ibm.com) 6-Oct-93

Revision History:

--*/

#include "mi.h"

PVOID
MmDbgReadCheck (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    PowerPC implementation specific:

    This routine returns the virtual address which is valid (mapped)
    for read access.

    The address may be within the PowerPC kernel BAT or may be
    otherwise valid and readable.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    Returns NULL if the address is not valid or readable, otherwise
    returns the virtual address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    if ((VirtualAddress >= (PVOID)KSEG0_BASE) &&
        (VirtualAddress < (PVOID)KSEG2_BASE)) {
        return VirtualAddress;
    }

    if ((VirtualAddress >= (PVOID)KIPCR) &&
            (VirtualAddress < (PVOID)(KIPCR2 + PAGE_SIZE))) {
        return VirtualAddress;
    }

    if (!MmIsAddressValid (VirtualAddress)) {
        return NULL;
    }

    return VirtualAddress;
}

PVOID
MmDbgWriteCheck (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    PowerPC implementation specific:

    This routine returns the virtual address which is valid (mapped)
    for write access.

    The address may be within the PowerPC kernel BAT or may be
    otherwise valid and writable.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    Returns NULL if the address is not valid or writable, otherwise
    returns the virtual address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    PMMPTE PointerPte;

    if ((VirtualAddress >= (PVOID)KSEG0_BASE) &&
        (VirtualAddress < (PVOID)KSEG2_BASE)) {
        return VirtualAddress;
    }

    if ((VirtualAddress >= (PVOID)KIPCR) &&
        (VirtualAddress < (PVOID)(KIPCR2 + PAGE_SIZE))) {
        return VirtualAddress;
    }

    if (!MmIsAddressValid (VirtualAddress)) {
        return NULL;
    }

    //
    // This is being added back in permanently since the PowerPC
    // hardware debug registers break in before the instruction
    // is executed. This will generally allow the kernel debugger
    // to step over the instruction that triggered the hardware
    // debug register breakpoint.
    //

    if (VirtualAddress <= MM_HIGHEST_USER_ADDRESS) {

        // This code is similar in spirit to that in the MIPS version.
        // It returns a writable alias for breakpoints in user pages.
        // However, it uses the virtual address reserved for the debugger,
        // rather than the wired-in KSEG0 translation available in MIPS.
        //
        // N.B. Microsoft says kernel debugger can't do user code at all.

        return MmDbgTranslatePhysicalAddress (
                   MmGetPhysicalAddress (VirtualAddress) );
    }

    PointerPte = MiGetPteAddress (VirtualAddress);
    if (PointerPte->u.Hard.Write == 0) {
        return NULL;
    }

    return VirtualAddress;
}

PVOID64
MmDbgReadCheck64 (
    IN PVOID64 VirtualAddress
    )

/*++

Routine Description:

    PowerPC implementation specific:

    This routine returns the virtual address which is valid (mapped)
    for read access.

    The address may be within the PowerPC kernel BAT or may be
    otherwise valid and readable.

    NO 64-bit suport, return NULL.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    Returns NULL if the address is not valid or readable, otherwise
    returns the virtual address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    return NULL;
}

PVOID64
MmDbgWriteCheck64 (
    IN PVOID64 VirtualAddress
    )

/*++

Routine Description:

    PowerPC implementation specific:

    This routine returns the virtual address which is valid (mapped)
    for write access.

    The address may be within the PowerPC kernel BAT or may be
    otherwise valid and writable.

    NO 64-bit suport, return NULL.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    Returns NULL if the address is not valid or writable, otherwise
    returns the virtual address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{

    return NULL;
}

PVOID
MmDbgTranslatePhysicalAddress (
    IN PHYSICAL_ADDRESS PhysicalAddress
    )

/*++

Routine Description:

    PowerPC implementation specific:

    This routine maps the specified physical address and returns
    the virtual address which maps the physical address.

    The next call to MmDbgTranslatePhyiscalAddress removes the
    previous phyiscal address translation, hence on a single
    physical address can be examined at a time (can't cross page
    boundaries).

Arguments:

    PhysicalAddress - Supplies the phyiscal address to map and translate.

Return Value:

    The virtual address which corresponds to the phyiscal address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    PVOID BaseAddress;

    BaseAddress = MiGetVirtualAddressMappedByPte (MmDebugPte);

    KiFlushSingleTb (TRUE, BaseAddress);

    *MmDebugPte = ValidKernelPte;
    MmDebugPte->u.Hard.PageFrameNumber = PhysicalAddress.LowPart >> PAGE_SHIFT;

    return (PVOID)((ULONG)BaseAddress + BYTE_OFFSET(PhysicalAddress.LowPart));
}
