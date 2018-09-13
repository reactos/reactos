/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   debugsup.c

Abstract:

    This module contains routines which provide support for the
    kernel debugger.

Author:

    Lou Perazzoli (loup) 02-Aug-90

Revision History:

--*/

#include "mi.h"

PVOID
MmDbgReadCheck (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    i386/486 implementation specific:

    This routine checks the specified virtual address and if it is
    valid and readable, it returns that virtual address, otherwise
    it returns NULL.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    Returns NULL if the address is not valid or readable, otherwise
    returns the virtual address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{

    if (!MmIsAddressValid (VirtualAddress)) {
        return NULL;
    }

    return VirtualAddress;

}

PVOID
MmDbgWriteCheck (
    IN PVOID VirtualAddress,
    IN PHARDWARE_PTE Opaque
    )

/*++

Routine Description:

    i386/486 implementation specific:

    This routine checks the specified virtual address and if it is
    valid and writable, it returns that virtual address, otherwise
    it returns NULL.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

    Opaque - Supplies an opaque pointer.

Return Value:

    Returns NULL if the address is not valid or writable, otherwise
    returns the virtual address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    MMPTE TempPte;
    MMPTE PteContents;
    PMMPTE InputPte;
    PMMPTE PointerPte;
    LOGICAL LargePage;

    InputPte = (PMMPTE)Opaque;

    InputPte->u.Long = 0;

    if (!MmIsAddressValid (VirtualAddress)) {
        return NULL;
    }

    PointerPte = MiGetPdeAddress (VirtualAddress);
    LargePage = TRUE;
    if (PointerPte->u.Hard.LargePage == 0) {
        PointerPte = MiGetPteAddress (VirtualAddress);
        LargePage = FALSE;
    }

#if defined(NT_UP)
    if (PointerPte->u.Hard.Write == 0)
#else
    if (PointerPte->u.Hard.Writable == 0)
#endif
    {

        //
        // PTE is not writable, make it so.
        //

        PteContents = *PointerPte;
    
        *InputPte = PteContents;
    
        //
        // Modify the PTE to ensure write permissions.
        //
    
        if (LargePage == TRUE) {
            TempPte = ValidKernelPde;
            TempPte.u.Hard.PageFrameNumber = PteContents.u.Hard.PageFrameNumber;
            TempPte.u.Hard.LargePage = 1;
        }
        else {
            MI_MAKE_VALID_PTE (TempPte,
                               PteContents.u.Hard.PageFrameNumber,
                               MM_READWRITE,
                               PointerPte);
#if !defined(NT_UP)
            TempPte.u.Hard.Writable = 1;
#endif
        }
    
        *PointerPte = TempPte;
    
        KeFillEntryTb ((PHARDWARE_PTE)PointerPte, VirtualAddress, TRUE);
    }

    return VirtualAddress;
}

VOID
MmDbgReleaseAddress (
    IN PVOID VirtualAddress,
    IN PHARDWARE_PTE Opaque
    )

/*++

Routine Description:

    i386/486 implementation specific:

    This routine resets the specified virtual address access permissions
    to its original state.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

    Opaque - Supplies an opaque pointer.

Return Value:

    None.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PMMPTE InputPte;

    InputPte = (PMMPTE)Opaque;

    ASSERT (MmIsAddressValid (VirtualAddress));

    // LWFIX: checksum regen and clear checksum flag

    if (InputPte->u.Long != 0) {

        PointerPte = MiGetPdeAddress (VirtualAddress);
        if (PointerPte->u.Hard.LargePage == 0) {
            PointerPte = MiGetPteAddress (VirtualAddress);
        }

        TempPte = *InputPte;
        TempPte.u.Hard.Dirty = 1;
    
        *PointerPte = TempPte;
    
        KeFillEntryTb ((PHARDWARE_PTE)PointerPte, VirtualAddress, TRUE);
    }

    return;
}

PVOID64
MmDbgReadCheck64 (
    IN PVOID64 VirtualAddress
    )

/*++

Routine Description:

    i386/486 implementation specific:

    This routine checks the specified virtual address and if it is
    valid and readable, it returns that virtual address, otherwise
    it returns NULL.

    NO 64-bit support, returns NULL.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    Returns NULL if the address is not valid or readable, otherwise
    returns the virtual address of the corresponding virtual address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    UNREFERENCED_PARAMETER (VirtualAddress);

    return NULL;
}

PVOID64
MmDbgWriteCheck64 (
    IN PVOID64 VirtualAddress
    )

/*++

Routine Description:

    i386/486 implementation specific:

    This routine checks the specified virtual address and if it is
    valid and writable, it returns that virtual address, otherwise
    it returns NULL.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    Returns NULL if the address is not valid or writable, otherwise
    returns the virtual address of the corresponding virtual address.

    NO 64-bit support, returns NULL.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    UNREFERENCED_PARAMETER (VirtualAddress);

    return NULL;
}

PVOID64
MmDbgTranslatePhysicalAddress64 (
    IN PHYSICAL_ADDRESS PhysicalAddress
    )

/*++

Routine Description:

    i386/486 implementation specific:

    This routine maps the specified physical address and returns
    the virtual address which maps the physical address.

    The next call to MmDbgTranslatePhysicalAddress removes the
    previous physical address translation, hence on a single
    physical address can be examined at a time (can't cross page
    boundaries).

Arguments:

    PhysicalAddress - Supplies the physical address to map and translate.

Return Value:

    The virtual address which corresponds to the physical address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    MMPTE TempPte;
    PVOID BaseAddress;

    BaseAddress = MiGetVirtualAddressMappedByPte (MmDebugPte);

    KiFlushSingleTb (TRUE, BaseAddress);

    TempPte = ValidKernelPte;

    TempPte.u.Hard.PageFrameNumber = (ULONG)(PhysicalAddress.QuadPart >> PAGE_SHIFT);

    *MmDebugPte = TempPte;

    return (PVOID64)((ULONG)BaseAddress + BYTE_OFFSET(PhysicalAddress.LowPart));
}
