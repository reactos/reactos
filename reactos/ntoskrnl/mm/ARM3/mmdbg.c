/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/mmdbg.c
 * PURPOSE:         Memory Manager support routines for the Kernel Debugger
 * PROGRAMMERS:     Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

#ifndef _WINKD_
#define KdpDprintf DPRINT
#elif defined(NDEBUG)
#define KdpDprintf(...)
#endif

/* GLOBALS ********************************************************************/

PVOID MiDebugMapping = MI_DEBUG_MAPPING;
PMMPTE MmDebugPte = NULL;

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
MmIsSessionAddress(IN PVOID Address)
{
    //
    // No session space support yet
    //
    return FALSE;
}

PVOID
NTAPI
MiDbgTranslatePhysicalAddress(IN ULONG64 PhysicalAddress,
                              IN ULONG Flags)
{
    PFN_NUMBER Pfn;
    MMPTE TempPte;
    PVOID MappingBaseAddress;

    //
    // Check if we are called too early
    //
    if (MmDebugPte == NULL)
    {
        //
        // The structures we require aren't initialized yet, fail
        //
        KdpDprintf("MiDbgTranslatePhysicalAddress called too early! "
                   "Address: 0x%I64x\n", PhysicalAddress);
        return NULL;
    }

    //
    // FIXME: No support for cache flags yet
    //
    if ((Flags & (MMDBG_COPY_CACHED |
                  MMDBG_COPY_UNCACHED |
                  MMDBG_COPY_WRITE_COMBINED)) != 0)
    {
        //
        // Fail
        //
        KdpDprintf("MiDbgTranslatePhysicalAddress: Cache Flags not yet supported. "
                   "Flags: 0x%lx\n", Flags & (MMDBG_COPY_CACHED |
                                              MMDBG_COPY_UNCACHED |
                                              MMDBG_COPY_WRITE_COMBINED));
        return NULL;
    }

    //
    // Save the base address of our mapping page
    //
    MappingBaseAddress = MiPteToAddress(MmDebugPte);

    //
    // Get the template
    //
    TempPte = ValidKernelPte;

    //
    // Convert physical address to PFN
    //
    Pfn = (PFN_NUMBER)(PhysicalAddress >> PAGE_SHIFT);

    /* Check if this could be an I/O mapping */
    if (!MiGetPfnEntry(Pfn))
    {
        //
        // FIXME: We don't support this yet
        //
        KdpDprintf("MiDbgTranslatePhysicalAddress: I/O Space not yet supported. "
                   "PFN: 0x%I64x\n", (ULONG64)Pfn);
        return NULL;
    }
    else
    {
        //
        // Set the PFN in the PTE
        //
        TempPte.u.Hard.PageFrameNumber = Pfn;
    }

    //
    // Map the PTE and invalidate its TLB entry
    //
    *MmDebugPte = TempPte;
    KeInvalidateTlbEntry(MappingBaseAddress);

    //
    // Calculate and return the virtual offset into our mapping page
    //
    return (PVOID)((ULONG_PTR)MappingBaseAddress +
                    BYTE_OFFSET(PhysicalAddress));
}

VOID
NTAPI
MiDbgUnTranslatePhysicalAddress(VOID)
{
    PVOID MappingBaseAddress = MiPteToAddress(MmDebugPte);

    //
    // The address must still be valid at this point
    //
    ASSERT(MmIsAddressValid(MappingBaseAddress));

    //
    // Clear the mapping PTE and invalidate its TLB entry
    //
    MmDebugPte->u.Long = 0;
    KeInvalidateTlbEntry(MappingBaseAddress);
}

NTSTATUS
NTAPI
MmDbgCopyMemory(IN ULONG64 Address,
                IN PVOID Buffer,
                IN ULONG Size,
                IN ULONG Flags)
{
    NTSTATUS Status;
    PVOID TargetAddress;
    ULONG64 PhysicalAddress;
    PMMPTE PointerPte;

    //
    // No local kernel debugging support yet, so don't worry about locking
    //
    ASSERT(Flags & MMDBG_COPY_UNSAFE);

    //
    // We only handle 1, 2, 4 and 8 byte requests
    //
    if ((Size != 1) &&
        (Size != 2) &&
        (Size != 4) &&
        (Size != MMDBG_COPY_MAX_SIZE))
    {
        //
        // Invalid size, fail
        //
        KdpDprintf("MmDbgCopyMemory: Received Illegal Size 0x%lx\n", Size);
        return STATUS_INVALID_PARAMETER_3;
    }

    //
    // The copy must be aligned
    //
    if ((Address & (Size - 1)) != 0)
    {
        //
        // Fail
        //
        KdpDprintf("MmDbgCopyMemory: Received Unaligned Address 0x%I64x Size %lx\n",
                  Address, Size);
        return STATUS_INVALID_PARAMETER_3;
    }

    //
    // Check if this is physical or virtual copy
    //
    if (Flags & MMDBG_COPY_PHYSICAL)
    {
        //
        // Physical: translate and map it to our mapping space
        //
        TargetAddress = MiDbgTranslatePhysicalAddress(Address, Flags);

        //
        // Check if translation failed
        //
        if (!TargetAddress)
        {
            //
            // Fail
            //
            KdpDprintf("MmDbgCopyMemory: Failed to Translate Physical Address "
                       "%I64x\n", Address);
            return STATUS_UNSUCCESSFUL;
        }

        //
        // The address we received must be valid!
        //
        ASSERT(MmIsAddressValid(TargetAddress));
    }
    else
    {
        //
        // Virtual; truncate it to avoid casts later down
        //
        TargetAddress = (PVOID)(ULONG_PTR)Address;

        //
        // Check if the address is invalid
        //
        if (!MmIsAddressValid(TargetAddress))
        {
            //
            // Fail
            //
            KdpDprintf("MmDbgCopyMemory: Failing %s for invalid "
                       "Virtual Address 0x%p\n",
                       Flags & MMDBG_COPY_WRITE ? "write" : "read",
                       TargetAddress);
            return STATUS_UNSUCCESSFUL;
        }

        //
        // No session space support yet
        //
        ASSERT(MmIsSessionAddress(TargetAddress) == FALSE);

        /* If we are going to write to the address, then check if its writable */
        PointerPte = MiAddressToPte(TargetAddress);
        if ((Flags & MMDBG_COPY_WRITE) && !MI_IS_PAGE_WRITEABLE(PointerPte))
        {
            /* Not writable, we need to do a physical copy */
            Flags |= MMDBG_COPY_PHYSICAL;

            /* Calculate the physical address */
            PhysicalAddress = PointerPte->u.Hard.PageFrameNumber << PAGE_SHIFT;
            PhysicalAddress += BYTE_OFFSET(Address);

            /* Translate the physical address */
            TargetAddress = MiDbgTranslatePhysicalAddress(PhysicalAddress, Flags);

            /* Check if translation failed */
            if (!TargetAddress)
            {
                /* Fail */
                KdpDprintf("MmDbgCopyMemory: Failed to translate for write "
                           "%I64x (%I64x)\n", PhysicalAddress, Address);
                return STATUS_UNSUCCESSFUL;
            }
        }
    }

    //
    // Use SEH to try to catch anything else somewhat cleanly
    //
    _SEH2_TRY
    {
        //
        // Check if this is read or write
        //
        if (Flags & MMDBG_COPY_WRITE)
        {
            //
            // Do the write
            //
            RtlCopyMemory(TargetAddress,
                          Buffer,
                          Size);
        }
        else
        {
            //
            // Do the read
            //
            RtlCopyMemory(Buffer,
                          TargetAddress,
                          Size);
        }

        //
        // Copy succeeded
        //
        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Get the exception code
        //
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    //
    // Get rid of the mapping if this was a physical copy
    //
    if (Flags & MMDBG_COPY_PHYSICAL)
    {
        //
        // Unmap and flush it
        //
        MiDbgUnTranslatePhysicalAddress();
    }

    //
    // Return status to caller
    //
    return Status;
}
