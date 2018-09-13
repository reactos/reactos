/*++

Copyright (c) 1998  Digital Equipment Corporation

Module Name:

   debugsup.c

Abstract:

    This module contains routines which provide support for the
    kernel debugger on 64-bit Alpha systems.

Author:

    David N. Cutler (davec) 24-Feb-1998

Revision History:

--*/

#include "mi.h"

PHARDWARE_PTE
MiCheckAddress (
    IN PVOID VirtualAddress,
    OUT PVOID *AccessAddress
    )

/*++

Routine Description:


    This function checks the specified virtual address for accessibility
    and returns a pointer to the hardware PTE that maps the virtual address
    and a 43-bit superpage address at which the address can be accessed.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

    AccessAddress - Supplies a pointer to a variable that receives the
         43-bit superpage address at which the address can be accessed.

Return Value:

    A value of NULL is returned if the address is invalid or not accessible.
    Otherwise, a pointer to the PTE that maps the specified virtual address
    is returned as the function value.

--*/

{

    ULONG Index;
    PHARDWARE_PTE Pdr1;
    PHARDWARE_PTE Pdr2;
    PHARDWARE_PTE Ptp;

    //
    // If the high order 21 bits of the virtual address are not all 1's or
    // 0's, then the address is invalid. Otherwise, attempt to tranverse
    // the page directory hierarchy.
    //

    if ((ULONG_PTR)(((LONG_PTR)VirtualAddress >> 43) + 1) > 1) {
        return NULL;
    }

    //
    // Compute the 43-bit superpage address of the level one page
    // directory page and check to see if the level two page directory
    // page is valid.
    //

    Pdr1 = (PHARDWARE_PTE)(KSEG43_BASE | (((PHARDWARE_PTE)PDE_SELFMAP)->PageFrameNumber << PAGE_SHIFT));
    Index = (ULONG)(((ULONG_PTR)VirtualAddress >> PDI1_SHIFT) & PDI_MASK);
    if (Pdr1[Index].Valid == 0) {
        return NULL;
    }

    //
    // Compute the 43-bit superpage address of the level two page
    // directory page and check to see if the page table page is
    // valid.
    //

    Pdr2 = (PHARDWARE_PTE)(KSEG43_BASE | (Pdr1[Index].PageFrameNumber << PAGE_SHIFT));
    Index = (ULONG)(((ULONG_PTR)VirtualAddress >> PDI2_SHIFT) & PDI_MASK);
    if (Pdr2[Index].Valid == 0) {
        return NULL;
    }

    //
    // Compute the 43-bit superpage address of the page table page and
    // check to see if the data page is valid.

    Ptp = (PHARDWARE_PTE)(KSEG43_BASE | (Pdr2[Index].PageFrameNumber << PAGE_SHIFT));
    Index = (ULONG)(((ULONG_PTR)VirtualAddress >> PTI_SHIFT) & PDI_MASK);
    if (Ptp[Index].Valid == 0) {
        return NULL;
    }

    //
    // Compute the 43-bit superpage address of the data page and return the
    // address of the PTE that maps the data page as the function value.
    //

    *AccessAddress = (PVOID)(KSEG43_BASE |
                                (Ptp[Index].PageFrameNumber << PAGE_SHIFT) |
                                (ULONG_PTR)VirtualAddress & (PAGE_SIZE - 1));

    return &Ptp[Index];
}

PVOID
MmDbgReadCheck (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This routine returns the 43-bit superpage address which corresponds
    to the specified virtual address if the specified virtual address
    is readable.

    N.B. This function should only be called at IRQL DISPATCH_LEVEL or
         above.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    If the specified virtual address is invalid or not readable, then a
    value of NULL is returned. Otherwise, the 43-bit superpage address
    which corresponds to the specified virtual address is returned.

--*/

{

    PVOID AccessAddress;

    //
    // If the address is within the 32- or 43-bit superpage regions, then
    // the address is returned as the function value.
    //

    if (MI_IS_PHYSICAL_ADDRESS(VirtualAddress)) {
        return VirtualAddress;
    }

    //
    // Check if the specified virtual address if accessible. If the virtual
    // address is accessible, then return the 43-bit superpage address which
    // corresponds to the virtual address. Otherwise, return NULL.
    //

    if (MiCheckAddress(VirtualAddress, &AccessAddress) == NULL) {
        return NULL;

    } else {
        return AccessAddress;
    }
}

PVOID
MmDbgWriteCheck (
    IN PVOID VirtualAddress,
    IN PHARDWARE_PTE Opaque
    )

/*++

Routine Description:

    This routine returns the 43-bit superpage address which corresponds
    to the specified virtual address if the specified virtual address is
    writable.

    N.B. This function should only be called at IRQL DISPATCH_LEVEL or
         above.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

    Opaque - Supplies a pointer to fill with an opaque value.

Return Value:

    If the specified virtual address is invalid or not writable, then a
    value of NULL is returned. Otherwise, the 43-bit superpage address
    which corresponds to the specified virtual address is returned.

--*/

{
    MMPTE PteContents;
    PVOID AccessAddress;
    PHARDWARE_PTE Pte;
    PMMPTE InputPte;

    InputPte = (PMMPTE)Opaque;

    InputPte->u.Long = 0;

    //
    // If the address is within the 32- or 43-bit superpage regions, then
    // the address is returned as the function value.
    //

    if (MI_IS_PHYSICAL_ADDRESS(VirtualAddress)) {
        return VirtualAddress;
    }

    //
    // Check if the specified virtual address if accessible. If the virtual
    // address is accessible, then return the 43-bit superpage address which
    // corresponds to the virtual address. Otherwise, return NULL.
    //
    //

    Pte = MiCheckAddress(VirtualAddress, &AccessAddress);
    if (Pte == NULL) {
        return NULL;
    }

    if (Pte->Write == 0) {

        //
        // PTE is not writable, make it so.
        //

        PteContents = *(PMMPTE)Pte;
    
        *InputPte = PteContents;
    
        //
        // Modify the PTE to ensure write permissions :
        // turn on write and turn off fault on write.
        //
    
        PteContents.u.Hard.Write = 1;
        PteContents.u.Hard.FaultOnWrite = 0;

        *(PMMPTE)Pte = PteContents;
    
        //
        // BUGBUG John Vert (jvert) 3/4/1999
        //   KeFillEntryTb is liable to IPI the other processors. This is
        //   definitely NOT what we want as the other processors are frozen
        //   in the debugger and we will deadlock if we try and IPI them.
        //   Just flush the the current processor instead.
        //KeFillEntryTb ((PHARDWARE_PTE)PointerPte, VirtualAddress, TRUE);
        KiFlushSingleTb(TRUE, VirtualAddress);
    }
    else {
        return AccessAddress;
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

        // LWFIX: Need to make the write go to disk on trim but can't
        // make it dirty here ! TempPte.u.Hard.Dirty = MM_PTE_DIRTY;
    
        *PointerPte = TempPte;
    
        //
        // BUGBUG John Vert (jvert) 3/4/1999
        //   KeFillEntryTb is liable to IPI the other processors. This is
        //   definitely NOT what we want as the other processors are frozen
        //   in the debugger and we will deadlock if we try and IPI them.
        //   Just flush the the current processor instead.
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

    This routine returns the 43-bit superpage address which corresponds
    to the specified virtual address if the specified virtual address
    is readable.

    N.B. This function should only be called at IRQL DISPATCH_LEVEL or
         above.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    If the specified virtual address is invalid or not readable, then a
    value of NULL is returned. Otherwise, the 43-bit superpage address
    which corresponds to the specified virtual address is returned.

--*/

{

    return MmDbgReadCheck(VirtualAddress);
}

PVOID64
MmDbgWriteCheck64 (
    IN PVOID64 VirtualAddress
    )

/*++

Routine Description:

    This routine returns the 43-bit superpage address which corresponds
    to the specified virtual address if the specified virtual address is
    writable.

    N.B. This function should only be called at IRQL DISPATCH_LEVEL or
         above.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    If the specified virtual address is invalid or not writable, then a
    value of NULL is returned. Otherwise, the 43-bit superpage address
    which corresponds to the specified virtual address is returned.

--*/

{
    HARDWARE_PTE Opaque;

    return MmDbgWriteCheck(VirtualAddress, &Opaque);
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
