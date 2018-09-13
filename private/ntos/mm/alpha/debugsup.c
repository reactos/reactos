/*++

Copyright (c) 1989  Microsoft Corporation
Copyright (c) 1992  Digital Equipment Corporation

Module Name:

   debugsup.c

Abstract:

    This module contains routines which provide support for the
    kernel debugger.

Author:

    Lou Perazzoli (loup) 02-Aug-90
    Joe Notarangelo  23-Apr-1992

Revision History:

--*/

#include "mi.h"

PVOID
MmDbgReadCheck (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:


    ALPHA implementation specific:

    This routine returns the virtual address which is valid (mapped)
    for read access.

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

    ALPHA implementation specific:

    This routine returns the physical address for a virtual address
    which is valid (mapped) for write access.

    If the address is valid and writable and not within KSEG0
    the physical address within KSEG0 is returned.  If the address
    is within KSEG0 then the called address is returned.

    NOTE: The physical address must only be used while the interrupt
    level on ALL processors is above DISPATCH_LEVEL, otherwise the
    binding between the virtual address and the physical address can
    change due to paging.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

    Opaque - Supplies a pointer to fill with an opaque value.

Return Value:

    Returns NULL if the address is not valid or readable, otherwise
    returns the physical address of the corresponding virtual address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    MMPTE PteContents;
    PMMPTE PointerPte;
    PMMPTE InputPte;

    InputPte = (PMMPTE)Opaque;

    InputPte->u.Long = 0;

    if ((VirtualAddress >= (PVOID)KSEG0_BASE) &&
        (VirtualAddress < (PVOID)KSEG2_BASE)) {
        return VirtualAddress;
    }

    if (!MmIsAddressValid (VirtualAddress)) {
        return NULL;
    }

    PointerPte = MiGetPteAddress (VirtualAddress);
    if ((VirtualAddress <= MM_HIGHEST_USER_ADDRESS) &&
         (PointerPte->u.Hard.PageFrameNumber < MM_PAGES_IN_KSEG0)) {

        //
        // User mode - return the physical address.  This prevents
        // copy on write faults for breakpoints on user-mode pages.
        // IGNORE write protection.
        //
        // N.B. - The physical address must be less than 1GB to allow this
        //        short-cut mapping.
        //
        // N.B. - Any non-breakpoint modifications can get lost when the page
        //        is paged out because the PTE is not marked modified when
        //        the access is made through this alternate mapping.
        //

        return (PVOID)
           ((ULONG)MmGetPhysicalAddress(VirtualAddress).LowPart + KSEG0_BASE);
    }

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
MmDbgReadCheck64 (
    IN PVOID64 VirtualAddress
    )

/*++

Routine Description:


    ALPHA implementation specific:

    This routine returns the virtual address which is valid (mapped)
    for read access.

    If the address is valid and readable then the called address is returned.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    Returns NULL if the address is not valid or readable, otherwise
    returns the virtual address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
#ifdef VLM_SUPPORT

    if (!MmIsAddressValid64 (VirtualAddress)) {
        return NULL;
    }

    return VirtualAddress;
#else
    return NULL;
#endif
}

PVOID64
MmDbgWriteCheck64 (
    IN PVOID64 VirtualAddress
    )

/*++

Routine Description:

    ALPHA implementation specific:

    This routine returns the physical address for a virtual address
    which is valid (mapped) for write access.

    If the address is valid and writable then the called address is returned.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    Returns NULL if the address is not valid or readable, otherwise
    returns the virtual address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
#ifdef VLM_SUPPORT
    PMMPTE PointerPte;

    if (!MmIsAddressValid64 (VirtualAddress)) {
        return NULL;
    }

    PointerPte = MiGetPteAddress64 (VirtualAddress);

    if (PointerPte->u.Hard.Write == 0) {
        return NULL;
    }

    return VirtualAddress;
#else
    return NULL;
#endif
}

PVOID64
MmDbgTranslatePhysicalAddress64 (
    IN PHYSICAL_ADDRESS PhysicalAddress
    )

/*++

Routine Description:

    ALPHA implementation specific:

    The Alpha processor provides a direct-mapped address space called
    the superpage.  The entire physical address space can be
    addressed via the superpage.  This routine translates a physical
    address to its corresponding superpage address.  Unfortunately,
    the base superpage address is processor-dependent.  Therefore, we
    have to compute it based on the processor level.  As new processors are
    released, this routine will need to be updated.

    This routine does not use PTEs.

Arguments:

    PhysicalAddress - Supplies the physical address to translate.

Return Value:

    The virtual (superpage) address which corresponds to the physical address.

Environment:

    Kernel mode IRQL at DISPATCH_LEVEL or greater.

--*/

{
    switch (KeProcessorLevel) {

    case PROCESSOR_ALPHA_21064:
    case PROCESSOR_ALPHA_21066:
    case PROCESSOR_ALPHA_21068:
        PhysicalAddress.QuadPart &= 0x00000003ffffffff;
        PhysicalAddress.QuadPart |= 0xfffffc0000000000;
        break;

    case PROCESSOR_ALPHA_21164:
    case PROCESSOR_ALPHA_21164PC:
        PhysicalAddress.QuadPart &= 0x000000ffffffffff;
        PhysicalAddress.QuadPart |= 0xfffffc0000000000;
        break;

    case PROCESSOR_ALPHA_21264:
        PhysicalAddress.QuadPart &= 0x00000fffffffffff;
        PhysicalAddress.QuadPart |= 0xffff800000000000;
        break;

    default:
        return NULL64;

    }

    return (PVOID64)PhysicalAddress.QuadPart;
}
