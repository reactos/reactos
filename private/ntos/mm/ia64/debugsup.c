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

    Koichi Yamada (kyamada) 09-Jan-1996 : IA64 version based on i386 version

--*/

#include "mi.h"


PVOID
MmDbgReadCheck (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    IA64 implementation specific:

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

    if (!(MI_IS_ADDRESS_VALID_FOR_KD(VirtualAddress))) {

        return NULL;

    }

    if (!MmIsAddressValid (VirtualAddress) && !MI_IS_PCR_PAGE(VirtualAddress)) {

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

    IA64 implementation specific:

    This routine checks the specified virtual address and if it is
    valid and writable, it returns that virtual address, otherwise
    it returns NULL.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

    Opaque - Supplies a pointer to fill with an opaque value.

Return Value:

    Returns NULL if the address is not valid or writable, otherwise
    returns the virtual address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    PMMPTE PointerPte;
    MMPTE PteContents;
    PMMPTE InputPte;

    InputPte = (PMMPTE)Opaque;

    InputPte->u.Long = 0;

    if (!(MI_IS_ADDRESS_VALID_FOR_KD(VirtualAddress))) {

        return NULL;

    }

    if (MI_IS_PHYSICAL_ADDRESS(VirtualAddress)) {
        return VirtualAddress;
    }

    PointerPte = MiGetPteAddress (VirtualAddress);

    if (PointerPte->u.Hard.Write == 0) {

        //
        // PTE is not writable, make it so.
        //

        PteContents = *PointerPte;
    
        *InputPte = PteContents;
    
        //
        // Modify the PTE to ensure write permissions.
        //
    
        PteContents.u.Hard.Write = 1;

        *PointerPte = PteContents;
    
        //
        // BUGBUG John Vert (jvert) 3/4/1999
        //   KeFillEntryTb is liable to IPI the other processors. This is
        //   definitely NOT what we want as the other processors are frozen
        //   in the debugger and we will deadlock if we try and IPI them.
        //   Just flush the the current processor instead.
        //KeFillEntryTb ((PHARDWARE_PTE)PointerPte, VirtualAddress, TRUE);
        KiFlushSingleTb(TRUE, VirtualAddress);

    }

#if 0
    if (!MI_IS_PCR_PAGE(VirtualAddress)) {

        //
        // PTE is not writable, return NULL.
        //

        return NULL;

    }
#endif

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

    Opaque - Supplies an opaque value.

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

    if (InputPte->u.Long != 0) {

        PointerPte = MiGetPteAddress (VirtualAddress);
    
        TempPte = *InputPte;
        
        // LWFIX: Need to make the write go out to memory but can't
        // make it dirty here ! TempPte.u.Hard.Dirty = MM_PTE_DIRTY;
    
        *PointerPte = TempPte;
    
        //
        // BUGBUG John Vert (jvert) 3/4/1999
        //   KeFillEntryTb is liable to IPI the other processors. This is
        //   definitely NOT what we want as the other processors are frozen
        //   in the debugger and we will deadlock if we try and IPI them.
        //   Just flush the current processor instead.
        //KeFillEntryTb ((PHARDWARE_PTE)PointerPte, VirtualAddress, TRUE);
        KiFlushSingleTb(TRUE, VirtualAddress);
    }

    return;
}

PVOID64
MmDbgTranslatePhysicalAddress64 (
    IN PHYSICAL_ADDRESS PhysicalAddress
    )

/*++

Routine Description:

    IA64 implementation specific:

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
    PVOID BaseAddress;
    PMMPFN Pfn1;
    ULONG Page;

    Page = (ULONG)(PhysicalAddress.QuadPart >> PAGE_SHIFT);

    if ((Page > (LONGLONG)MmHighestPhysicalPage) ||
        (Page < (LONGLONG)MmLowestPhysicalPage)) {
        return NULL;
    }

    Pfn1 = MI_PFN_ELEMENT (Page);

    if (!MmIsAddressValid (Pfn1)) {
        return NULL;
    }

    BaseAddress = MiGetVirtualAddressMappedByPte (MmDebugPte);

    KiFlushSingleTb (TRUE, BaseAddress);

    *MmDebugPte = ValidKernelPte;
    MmDebugPte->u.Hard.PageFrameNumber = Page;

    return (PVOID64)((PCHAR)BaseAddress + BYTE_OFFSET(PhysicalAddress.LowPart));
}
