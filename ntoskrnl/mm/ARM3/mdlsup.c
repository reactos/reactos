/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/mdlsup.c
 * PURPOSE:         ARM Memory Manager Memory Descriptor List (MDL) Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* GLOBALS ********************************************************************/

BOOLEAN MmTrackPtes;
BOOLEAN MmTrackLockedPages;
SIZE_T MmSystemLockPagesCount;

ULONG MiCacheOverride[MiNotMapped + 1];

/* INTERNAL FUNCTIONS *********************************************************/
static
PVOID
NTAPI
MiMapLockedPagesInUserSpace(
    _In_ PMDL Mdl,
    _In_ PVOID StartVa,
    _In_ MEMORY_CACHING_TYPE CacheType,
    _In_opt_ PVOID BaseAddress)
{
    NTSTATUS Status;
    PEPROCESS Process = PsGetCurrentProcess();
    PETHREAD Thread = PsGetCurrentThread();
    TABLE_SEARCH_RESULT Result;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;
    MI_PFN_CACHE_ATTRIBUTE EffectiveCacheAttribute;
    BOOLEAN IsIoMapping;
    KIRQL OldIrql;
    ULONG_PTR StartingVa;
    ULONG_PTR EndingVa;
    PMMADDRESS_NODE Parent;
    PMMVAD_LONG Vad;
    ULONG NumberOfPages;
    PMMPTE PointerPte;
    PMMPDE PointerPde;
    MMPTE TempPte;
    PPFN_NUMBER MdlPages;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    BOOLEAN AddressSpaceLocked = FALSE;

    PAGED_CODE();

    DPRINT("MiMapLockedPagesInUserSpace(%p, %p, 0x%x, %p)\n",
           Mdl, StartVa, CacheType, BaseAddress);

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartVa,
                                                   MmGetMdlByteCount(Mdl));
    MdlPages = MmGetMdlPfnArray(Mdl);

    ASSERT(CacheType <= MmWriteCombined);

    IsIoMapping = (Mdl->MdlFlags & MDL_IO_SPACE) != 0;
    CacheAttribute = MiPlatformCacheAttributes[IsIoMapping][CacheType];

    /* Large pages are always cached, make sure we're not asking for those */
    if (CacheAttribute != MiCached)
    {
        DPRINT1("FIXME: Need to check for large pages\n");
    }

    /* Allocate a VAD for our mapped region */
    Vad = ExAllocatePoolWithTag(NonPagedPool, sizeof(MMVAD_LONG), 'ldaV');
    if (Vad == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    /* Initialize PhysicalMemory VAD */
    RtlZeroMemory(Vad, sizeof(*Vad));
    Vad->u2.VadFlags2.LongVad = 1;
    Vad->u.VadFlags.VadType = VadDevicePhysicalMemory;
    Vad->u.VadFlags.Protection = MM_READWRITE;
    Vad->u.VadFlags.PrivateMemory = 1;

    /* Did the caller specify an address? */
    if (BaseAddress == NULL)
    {
        /* We get to pick the address */
        MmLockAddressSpace(&Process->Vm);
        AddressSpaceLocked = TRUE;
        if (Process->VmDeleted)
        {
            Status = STATUS_PROCESS_IS_TERMINATING;
            goto Error;
        }

        Result = MiFindEmptyAddressRangeInTree(NumberOfPages << PAGE_SHIFT,
                                               MM_VIRTMEM_GRANULARITY,
                                               &Process->VadRoot,
                                               &Parent,
                                               &StartingVa);
        if (Result == TableFoundNode)
        {
            Status = STATUS_NO_MEMORY;
            goto Error;
        }
        EndingVa = StartingVa + NumberOfPages * PAGE_SIZE - 1;
        BaseAddress = (PVOID)StartingVa;
    }
    else
    {
        /* Caller specified a base address */
        StartingVa = (ULONG_PTR)BaseAddress;
        EndingVa = StartingVa + NumberOfPages * PAGE_SIZE - 1;

        /* Make sure it's valid */
        if (BYTE_OFFSET(StartingVa) != 0 ||
            EndingVa <= StartingVa ||
            EndingVa > (ULONG_PTR)MM_HIGHEST_VAD_ADDRESS)
        {
            Status = STATUS_INVALID_ADDRESS;
            goto Error;
        }

        MmLockAddressSpace(&Process->Vm);
        AddressSpaceLocked = TRUE;
        if (Process->VmDeleted)
        {
            Status = STATUS_PROCESS_IS_TERMINATING;
            goto Error;
        }

        /* Check if it's already in use */
        Result = MiCheckForConflictingNode(StartingVa >> PAGE_SHIFT,
                                           EndingVa >> PAGE_SHIFT,
                                           &Process->VadRoot,
                                           &Parent);
        if (Result == TableFoundNode)
        {
            Status = STATUS_CONFLICTING_ADDRESSES;
            goto Error;
        }
    }

    Vad->StartingVpn = StartingVa >> PAGE_SHIFT;
    Vad->EndingVpn = EndingVa >> PAGE_SHIFT;

    MiLockProcessWorkingSetUnsafe(Process, Thread);

    ASSERT(Vad->EndingVpn >= Vad->StartingVpn);

    MiInsertVad((PMMVAD)Vad, &Process->VadRoot);

    /* Check if this is uncached */
    if (CacheAttribute != MiCached)
    {
        /* Flush all caches */
        KeFlushEntireTb(TRUE, TRUE);
        KeInvalidateAllCaches();
    }

    PointerPte = MiAddressToPte(BaseAddress);
    while (NumberOfPages != 0 &&
           *MdlPages != LIST_HEAD)
    {
        PointerPde = MiPteToPde(PointerPte);
        MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
        ASSERT(PointerPte->u.Hard.Valid == 0);

        /* Add a PDE reference for each page */
        MiIncrementPageTableReferences(BaseAddress);

        /* Set up our basic user PTE */
        MI_MAKE_HARDWARE_PTE_USER(&TempPte,
                                  PointerPte,
                                  MM_READWRITE,
                                  *MdlPages);

        EffectiveCacheAttribute = CacheAttribute;

        /* We need to respect the PFN's caching information in some cases */
        Pfn2 = MiGetPfnEntry(*MdlPages);
        if (Pfn2 != NULL)
        {
            ASSERT(Pfn2->u3.e2.ReferenceCount != 0);

            switch (Pfn2->u3.e1.CacheAttribute)
            {
                case MiNonCached:
                    if (CacheAttribute != MiNonCached)
                    {
                        MiCacheOverride[1]++;
                        EffectiveCacheAttribute = MiNonCached;
                    }
                    break;

                case MiCached:
                    if (CacheAttribute != MiCached)
                    {
                        MiCacheOverride[0]++;
                        EffectiveCacheAttribute = MiCached;
                    }
                    break;

                case MiWriteCombined:
                    if (CacheAttribute != MiWriteCombined)
                    {
                        MiCacheOverride[2]++;
                        EffectiveCacheAttribute = MiWriteCombined;
                    }
                    break;

                default:
                    /* We don't support AWE magic (MiNotMapped) */
                    DPRINT1("FIXME: MiNotMapped is not supported\n");
                    ASSERT(FALSE);
                    break;
            }
        }

        /* Configure caching */
        switch (EffectiveCacheAttribute)
        {
            case MiNonCached:
                MI_PAGE_DISABLE_CACHE(&TempPte);
                MI_PAGE_WRITE_THROUGH(&TempPte);
                break;
            case MiCached:
                break;
            case MiWriteCombined:
                MI_PAGE_DISABLE_CACHE(&TempPte);
                MI_PAGE_WRITE_COMBINED(&TempPte);
                break;
            default:
                ASSERT(FALSE);
                break;
        }

        /* Make the page valid */
        MI_WRITE_VALID_PTE(PointerPte, TempPte);

        /* Acquire a share count */
        Pfn1 = MI_PFN_ELEMENT(PointerPde->u.Hard.PageFrameNumber);
        OldIrql = MiAcquirePfnLock();
        Pfn1->u2.ShareCount++;
        MiReleasePfnLock(OldIrql);

        /* Next page */
        MdlPages++;
        PointerPte++;
        NumberOfPages--;
        BaseAddress = (PVOID)((ULONG_PTR)BaseAddress + PAGE_SIZE);
    }

    MiUnlockProcessWorkingSetUnsafe(Process, Thread);
    ASSERT(AddressSpaceLocked);
    MmUnlockAddressSpace(&Process->Vm);

    ASSERT(StartingVa != 0);
    return (PVOID)((ULONG_PTR)StartingVa + MmGetMdlByteOffset(Mdl));

Error:
    if (AddressSpaceLocked)
    {
        MmUnlockAddressSpace(&Process->Vm);
    }
    if (Vad != NULL)
    {
        ExFreePoolWithTag(Vad, 'ldaV');
    }
    ExRaiseStatus(Status);
}

static
VOID
NTAPI
MiUnmapLockedPagesInUserSpace(
    _In_ PVOID BaseAddress,
    _In_ PMDL Mdl)
{
    PEPROCESS Process = PsGetCurrentProcess();
    PETHREAD Thread = PsGetCurrentThread();
    PMMVAD Vad;
    PMMPTE PointerPte;
    PMMPDE PointerPde;
    KIRQL OldIrql;
    ULONG NumberOfPages;
    PPFN_NUMBER MdlPages;
    PFN_NUMBER PageTablePage;

    DPRINT("MiUnmapLockedPagesInUserSpace(%p, %p)\n", BaseAddress, Mdl);

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(Mdl),
                                                   MmGetMdlByteCount(Mdl));
    ASSERT(NumberOfPages != 0);
    MdlPages = MmGetMdlPfnArray(Mdl);

    /* Find the VAD */
    MmLockAddressSpace(&Process->Vm);
    Vad = MiLocateAddress(BaseAddress);
    if (!Vad ||
        Vad->u.VadFlags.VadType != VadDevicePhysicalMemory)
    {
        DPRINT1("MiUnmapLockedPagesInUserSpace invalid for %p\n", BaseAddress);
        MmUnlockAddressSpace(&Process->Vm);
        return;
    }

    MiLockProcessWorkingSetUnsafe(Process, Thread);

    /* Remove it from the process VAD tree */
    ASSERT(Process->VadRoot.NumberGenericTableElements >= 1);
    MiRemoveNode((PMMADDRESS_NODE)Vad, &Process->VadRoot);

    /* MiRemoveNode should have removed us if we were the hint */
    ASSERT(Process->VadRoot.NodeHint != Vad);

    PointerPte = MiAddressToPte(BaseAddress);
    OldIrql = MiAcquirePfnLock();
    while (NumberOfPages != 0 &&
           *MdlPages != LIST_HEAD)
    {
        ASSERT(MiAddressToPte(PointerPte)->u.Hard.Valid == 1);
        ASSERT(PointerPte->u.Hard.Valid == 1);

        /* Dereference the page */
        MiDecrementPageTableReferences(BaseAddress);

        /* Invalidate it */
        MI_ERASE_PTE(PointerPte);

        /* We invalidated this PTE, so dereference the PDE */
        PointerPde = MiAddressToPde(BaseAddress);
        PageTablePage = PointerPde->u.Hard.PageFrameNumber;
        MiDecrementShareCount(MiGetPfnEntry(PageTablePage), PageTablePage);

        /* Next page */
        PointerPte++;
        NumberOfPages--;
        BaseAddress = (PVOID)((ULONG_PTR)BaseAddress + PAGE_SIZE);
        MdlPages++;

        /* Moving to a new PDE? */
        if (PointerPde != MiAddressToPde(BaseAddress))
        {
            /* See if we should delete it */
            KeFlushProcessTb();
            PointerPde = MiPteToPde(PointerPte - 1);
            ASSERT(PointerPde->u.Hard.Valid == 1);
            if (MiQueryPageTableReferences(BaseAddress) == 0)
            {
                ASSERT(PointerPde->u.Long != 0);
                MiDeletePte(PointerPde,
                            MiPteToAddress(PointerPde),
                            Process,
                            NULL);
            }
        }
    }

    KeFlushProcessTb();
    MiReleasePfnLock(OldIrql);
    MiUnlockProcessWorkingSetUnsafe(Process, Thread);
    MmUnlockAddressSpace(&Process->Vm);
    ExFreePoolWithTag(Vad, 'ldaV');
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
PMDL
NTAPI
MmCreateMdl(IN PMDL Mdl,
            IN PVOID Base,
            IN SIZE_T Length)
{
    SIZE_T Size;

    //
    // Check if we don't have an MDL built
    //
    if (!Mdl)
    {
        //
        // Calculate the size we'll need  and allocate the MDL
        //
        Size = MmSizeOfMdl(Base, Length);
        Mdl = ExAllocatePoolWithTag(NonPagedPool, Size, TAG_MDL);
        if (!Mdl) return NULL;
    }

    //
    // Initialize it
    //
    MmInitializeMdl(Mdl, Base, Length);
    return Mdl;
}

/*
 * @implemented
 */
SIZE_T
NTAPI
MmSizeOfMdl(IN PVOID Base,
            IN SIZE_T Length)
{
    //
    // Return the MDL size
    //
    return sizeof(MDL) +
           (ADDRESS_AND_SIZE_TO_SPAN_PAGES(Base, Length) * sizeof(PFN_NUMBER));
}

/*
 * @implemented
 */
VOID
NTAPI
MmBuildMdlForNonPagedPool(IN PMDL Mdl)
{
    PPFN_NUMBER MdlPages, EndPage;
    PFN_NUMBER Pfn, PageCount;
    PVOID Base;
    PMMPTE PointerPte;

    //
    // Sanity checks
    //
    ASSERT(Mdl->ByteCount != 0);
    ASSERT((Mdl->MdlFlags & (MDL_PAGES_LOCKED |
                             MDL_MAPPED_TO_SYSTEM_VA |
                             MDL_SOURCE_IS_NONPAGED_POOL |
                             MDL_PARTIAL)) == 0);

    //
    // We know the MDL isn't associated to a process now
    //
    Mdl->Process = NULL;

    //
    // Get page and VA information
    //
    MdlPages = (PPFN_NUMBER)(Mdl + 1);
    Base = Mdl->StartVa;

    //
    // Set the system address and now get the page count
    //
    Mdl->MappedSystemVa = (PVOID)((ULONG_PTR)Base + Mdl->ByteOffset);
    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Mdl->MappedSystemVa,
                                               Mdl->ByteCount);
    ASSERT(PageCount != 0);
    EndPage = MdlPages + PageCount;

    //
    // Loop the PTEs
    //
    PointerPte = MiAddressToPte(Base);
    do
    {
        //
        // Write the PFN
        //
        Pfn = PFN_FROM_PTE(PointerPte++);
        *MdlPages++ = Pfn;
    } while (MdlPages < EndPage);

    //
    // Set the nonpaged pool flag
    //
    Mdl->MdlFlags |= MDL_SOURCE_IS_NONPAGED_POOL;

    //
    // Check if this is an I/O mapping
    //
    if (!MiGetPfnEntry(Pfn)) Mdl->MdlFlags |= MDL_IO_SPACE;
}

/*
 * @implemented
 */
PMDL
NTAPI
MmAllocatePagesForMdl(IN PHYSICAL_ADDRESS LowAddress,
                      IN PHYSICAL_ADDRESS HighAddress,
                      IN PHYSICAL_ADDRESS SkipBytes,
                      IN SIZE_T TotalBytes)
{
    //
    // Call the internal routine
    //
    return MiAllocatePagesForMdl(LowAddress,
                                 HighAddress,
                                 SkipBytes,
                                 TotalBytes,
                                 MiNotMapped,
                                 0);
}

/*
 * @implemented
 */
PMDL
NTAPI
MmAllocatePagesForMdlEx(IN PHYSICAL_ADDRESS LowAddress,
                        IN PHYSICAL_ADDRESS HighAddress,
                        IN PHYSICAL_ADDRESS SkipBytes,
                        IN SIZE_T TotalBytes,
                        IN MEMORY_CACHING_TYPE CacheType,
                        IN ULONG Flags)
{
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;

    //
    // Check for invalid cache type
    //
    if (CacheType > MmWriteCombined)
    {
        //
        // Normalize to default
        //
        CacheAttribute = MiNotMapped;
    }
    else
    {
        //
        // Conver to internal caching attribute
        //
        CacheAttribute = MiPlatformCacheAttributes[FALSE][CacheType];
    }

    //
    // Only these flags are allowed
    //
    if (Flags & ~(MM_DONT_ZERO_ALLOCATION | MM_ALLOCATE_FROM_LOCAL_NODE_ONLY))
    {
        //
        // Silently fail
        //
        return NULL;
    }

    //
    // Call the internal routine
    //
    return MiAllocatePagesForMdl(LowAddress,
                                 HighAddress,
                                 SkipBytes,
                                 TotalBytes,
                                 CacheAttribute,
                                 Flags);
}

/*
 * @implemented
 */
VOID
NTAPI
MmFreePagesFromMdl(IN PMDL Mdl)
{
    PVOID Base;
    PPFN_NUMBER Pages;
    LONG NumberOfPages;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    DPRINT("Freeing MDL: %p\n", Mdl);

    //
    // Sanity checks
    //
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    ASSERT((Mdl->MdlFlags & MDL_IO_SPACE) == 0);
    ASSERT(((ULONG_PTR)Mdl->StartVa & (PAGE_SIZE - 1)) == 0);

    //
    // Get address and page information
    //
    Base = (PVOID)((ULONG_PTR)Mdl->StartVa + Mdl->ByteOffset);
    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Base, Mdl->ByteCount);

    //
    // Acquire PFN lock
    //
    OldIrql = MiAcquirePfnLock();

    //
    // Loop all the MDL pages
    //
    Pages = (PPFN_NUMBER)(Mdl + 1);
    do
    {
        //
        // Reached the last page
        //
        if (*Pages == LIST_HEAD) break;

        //
        // Get the page entry
        //
        Pfn1 = MiGetPfnEntry(*Pages);
        ASSERT(Pfn1);
        ASSERT(Pfn1->u2.ShareCount == 1);
        ASSERT(MI_IS_PFN_DELETED(Pfn1) == TRUE);
        if (Pfn1->u4.PteFrame != 0x1FFEDCB)
        {
            /* Corrupted PFN entry or invalid free */
            KeBugCheckEx(MEMORY_MANAGEMENT, 0x1236, (ULONG_PTR)Mdl, (ULONG_PTR)Pages, *Pages);
        }

        //
        // Clear it
        //
        Pfn1->u3.e1.StartOfAllocation = 0;
        Pfn1->u3.e1.EndOfAllocation = 0;
        Pfn1->u3.e1.PageLocation = StandbyPageList;
        Pfn1->u2.ShareCount = 0;

        //
        // Dereference it
        //
        ASSERT(Pfn1->u3.e2.ReferenceCount != 0);
        if (Pfn1->u3.e2.ReferenceCount != 1)
        {
            /* Just take off one reference */
            InterlockedDecrement16((PSHORT)&Pfn1->u3.e2.ReferenceCount);
        }
        else
        {
            /* We'll be nuking the whole page */
            MiDecrementReferenceCount(Pfn1, *Pages);
        }

        //
        // Clear this page and move on
        //
        *Pages++ = LIST_HEAD;
    } while (--NumberOfPages != 0);

    //
    // Release the lock
    //
    MiReleasePfnLock(OldIrql);

    //
    // Remove the pages locked flag
    //
    Mdl->MdlFlags &= ~MDL_PAGES_LOCKED;
}

/*
 * @implemented
 */
PVOID
NTAPI
MmMapLockedPagesSpecifyCache(IN PMDL Mdl,
                             IN KPROCESSOR_MODE AccessMode,
                             IN MEMORY_CACHING_TYPE CacheType,
                             IN PVOID BaseAddress,
                             IN ULONG BugCheckOnFailure,
                             IN ULONG Priority) // MM_PAGE_PRIORITY
{
    PVOID Base;
    PPFN_NUMBER MdlPages, LastPage;
    PFN_COUNT PageCount;
    BOOLEAN IsIoMapping;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;
    PMMPTE PointerPte;
    MMPTE TempPte;

    //
    // Sanity check
    //
    ASSERT(Mdl->ByteCount != 0);

    //
    // Get the base
    //
    Base = (PVOID)((ULONG_PTR)Mdl->StartVa + Mdl->ByteOffset);

    //
    // Handle kernel case first
    //
    if (AccessMode == KernelMode)
    {
        //
        // Get the list of pages and count
        //
        MdlPages = (PPFN_NUMBER)(Mdl + 1);
        PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Base, Mdl->ByteCount);
        LastPage = MdlPages + PageCount;

        //
        // Sanity checks
        //
        ASSERT((Mdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA |
                                 MDL_SOURCE_IS_NONPAGED_POOL |
                                 MDL_PARTIAL_HAS_BEEN_MAPPED)) == 0);
        ASSERT((Mdl->MdlFlags & (MDL_PAGES_LOCKED | MDL_PARTIAL)) != 0);

        //
        // Get the correct cache type
        //
        IsIoMapping = (Mdl->MdlFlags & MDL_IO_SPACE) != 0;
        CacheAttribute = MiPlatformCacheAttributes[IsIoMapping][CacheType];

        //
        // Reserve the PTEs
        //
        PointerPte = MiReserveSystemPtes(PageCount, SystemPteSpace);
        if (!PointerPte)
        {
            //
            // If it can fail, return NULL
            //
            if (Mdl->MdlFlags & MDL_MAPPING_CAN_FAIL) return NULL;

            //
            // Should we bugcheck?
            //
            if (!BugCheckOnFailure) return NULL;

            //
            // Yes, crash the system
            //
            KeBugCheckEx(NO_MORE_SYSTEM_PTES, 0, PageCount, 0, 0);
        }

        //
        // Get the mapped address
        //
        Base = (PVOID)((ULONG_PTR)MiPteToAddress(PointerPte) + Mdl->ByteOffset);

        //
        // Get the template
        //
        TempPte = ValidKernelPte;
        switch (CacheAttribute)
        {
            case MiNonCached:

                //
                // Disable caching
                //
                MI_PAGE_DISABLE_CACHE(&TempPte);
                MI_PAGE_WRITE_THROUGH(&TempPte);
                break;

            case MiWriteCombined:

                //
                // Enable write combining
                //
                MI_PAGE_DISABLE_CACHE(&TempPte);
                MI_PAGE_WRITE_COMBINED(&TempPte);
                break;

            default:
                //
                // Nothing to do
                //
                break;
        }

        //
        // Loop all PTEs
        //
        do
        {
            //
            // We're done here
            //
            if (*MdlPages == LIST_HEAD) break;

            //
            // Write the PTE
            //
            TempPte.u.Hard.PageFrameNumber = *MdlPages;
            MI_WRITE_VALID_PTE(PointerPte++, TempPte);
        } while (++MdlPages < LastPage);

        //
        // Mark it as mapped
        //
        ASSERT((Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0);
        Mdl->MappedSystemVa = Base;
        Mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;

        //
        // Check if it was partial
        //
        if (Mdl->MdlFlags & MDL_PARTIAL)
        {
            //
            // Write the appropriate flag here too
            //
            Mdl->MdlFlags |= MDL_PARTIAL_HAS_BEEN_MAPPED;
        }

        //
        // Return the mapped address
        //
        return Base;
    }

    return MiMapLockedPagesInUserSpace(Mdl, Base, CacheType, BaseAddress);
}

/*
 * @implemented
 */
PVOID
NTAPI
MmMapLockedPages(IN PMDL Mdl,
                 IN KPROCESSOR_MODE AccessMode)
{
    //
    // Call the extended version
    //
    return MmMapLockedPagesSpecifyCache(Mdl,
                                        AccessMode,
                                        MmCached,
                                        NULL,
                                        TRUE,
                                        HighPagePriority);
}

/*
 * @implemented
 */
VOID
NTAPI
MmUnmapLockedPages(IN PVOID BaseAddress,
                   IN PMDL Mdl)
{
    PVOID Base;
    PFN_COUNT PageCount, ExtraPageCount;
    PPFN_NUMBER MdlPages;
    PMMPTE PointerPte;

    //
    // Sanity check
    //
    ASSERT(Mdl->ByteCount != 0);

    //
    // Check if this is a kernel request
    //
    if (BaseAddress > MM_HIGHEST_USER_ADDRESS)
    {
        //
        // Get base and count information
        //
        Base = (PVOID)((ULONG_PTR)Mdl->StartVa + Mdl->ByteOffset);
        PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Base, Mdl->ByteCount);

        //
        // Sanity checks
        //
        ASSERT((Mdl->MdlFlags & MDL_PARENT_MAPPED_SYSTEM_VA) == 0);
        ASSERT(PageCount != 0);
        ASSERT(Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA);

        //
        // Get the PTE
        //
        PointerPte = MiAddressToPte(BaseAddress);

        //
        // This should be a resident system PTE
        //
        ASSERT(PointerPte >= MmSystemPtesStart[SystemPteSpace]);
        ASSERT(PointerPte <= MmSystemPtesEnd[SystemPteSpace]);
        ASSERT(PointerPte->u.Hard.Valid == 1);

        //
        // Check if the caller wants us to free advanced pages
        //
        if (Mdl->MdlFlags & MDL_FREE_EXTRA_PTES)
        {
            //
            // Get the MDL page array
            //
            MdlPages = MmGetMdlPfnArray(Mdl);

            /* Number of extra pages stored after the PFN array */
            ExtraPageCount = (PFN_COUNT)*(MdlPages + PageCount);

            //
            // Do the math
            //
            PageCount += ExtraPageCount;
            PointerPte -= ExtraPageCount;
            ASSERT(PointerPte >= MmSystemPtesStart[SystemPteSpace]);
            ASSERT(PointerPte <= MmSystemPtesEnd[SystemPteSpace]);

            //
            // Get the new base address
            //
            BaseAddress = (PVOID)((ULONG_PTR)BaseAddress -
                                  (ExtraPageCount << PAGE_SHIFT));
        }

        //
        // Remove flags
        //
        Mdl->MdlFlags &= ~(MDL_MAPPED_TO_SYSTEM_VA |
                           MDL_PARTIAL_HAS_BEEN_MAPPED |
                           MDL_FREE_EXTRA_PTES);

        //
        // Release the system PTEs
        //
        MiReleaseSystemPtes(PointerPte, PageCount, SystemPteSpace);
    }
    else
    {
        MiUnmapLockedPagesInUserSpace(BaseAddress, Mdl);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
MmProbeAndLockPages(IN PMDL Mdl,
                    IN KPROCESSOR_MODE AccessMode,
                    IN LOCK_OPERATION Operation)
{
    PPFN_NUMBER MdlPages;
    PVOID Base, Address, LastAddress, StartAddress;
    ULONG LockPages, TotalPages;
    NTSTATUS Status = STATUS_SUCCESS;
    PEPROCESS CurrentProcess;
    NTSTATUS ProbeStatus;
    PMMPTE PointerPte, LastPte;
    PMMPDE PointerPde;
#if (_MI_PAGING_LEVELS >= 3)
    PMMPDE PointerPpe;
#endif
#if (_MI_PAGING_LEVELS == 4)
    PMMPDE PointerPxe;
#endif
    PFN_NUMBER PageFrameIndex;
    BOOLEAN UsePfnLock;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    DPRINT("Probing MDL: %p\n", Mdl);

    //
    // Sanity checks
    //
    ASSERT(Mdl->ByteCount != 0);
    ASSERT(((ULONG)Mdl->ByteOffset & ~(PAGE_SIZE - 1)) == 0);
    ASSERT(((ULONG_PTR)Mdl->StartVa & (PAGE_SIZE - 1)) == 0);
    ASSERT((Mdl->MdlFlags & (MDL_PAGES_LOCKED |
                             MDL_MAPPED_TO_SYSTEM_VA |
                             MDL_SOURCE_IS_NONPAGED_POOL |
                             MDL_PARTIAL |
                             MDL_IO_SPACE)) == 0);

    //
    // Get page and base information
    //
    MdlPages = (PPFN_NUMBER)(Mdl + 1);
    Base = Mdl->StartVa;

    //
    // Get the addresses and how many pages we span (and need to lock)
    //
    Address = (PVOID)((ULONG_PTR)Base + Mdl->ByteOffset);
    LastAddress = (PVOID)((ULONG_PTR)Address + Mdl->ByteCount);
    LockPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Address, Mdl->ByteCount);
    ASSERT(LockPages != 0);

    /* Block invalid access */
    if ((AccessMode != KernelMode) &&
        ((LastAddress > (PVOID)MM_USER_PROBE_ADDRESS) || (Address >= LastAddress)))
    {
        /* Caller should be in SEH, raise the error */
        *MdlPages = LIST_HEAD;
        ExRaiseStatus(STATUS_ACCESS_VIOLATION);
    }

    //
    // Get the process
    //
    if (Address <= MM_HIGHEST_USER_ADDRESS)
    {
        //
        // Get the process
        //
        CurrentProcess = PsGetCurrentProcess();
    }
    else
    {
        //
        // No process
        //
        CurrentProcess = NULL;
    }

    //
    // Save the number of pages we'll have to lock, and the start address
    //
    TotalPages = LockPages;
    StartAddress = Address;

    /* Large pages not supported */
    ASSERT(!MI_IS_PHYSICAL_ADDRESS(Address));

    //
    // Now probe them
    //
    ProbeStatus = STATUS_SUCCESS;
    _SEH2_TRY
    {
        //
        // Enter probe loop
        //
        do
        {
            //
            // Assume failure
            //
            *MdlPages = LIST_HEAD;

            //
            // Read
            //
            *(volatile CHAR*)Address;

            //
            // Check if this is write access (only probe for user-mode)
            //
            if ((Operation != IoReadAccess) &&
                (Address <= MM_HIGHEST_USER_ADDRESS))
            {
                //
                // Probe for write too
                //
                ProbeForWriteChar(Address);
            }

            //
            // Next address...
            //
            Address = PAGE_ALIGN((ULONG_PTR)Address + PAGE_SIZE);

            //
            // Next page...
            //
            LockPages--;
            MdlPages++;
        } while (Address < LastAddress);

        //
        // Reset back to the original page
        //
        ASSERT(LockPages == 0);
        MdlPages = (PPFN_NUMBER)(Mdl + 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Oops :(
        //
        ProbeStatus = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    //
    // So how did that go?
    //
    if (ProbeStatus != STATUS_SUCCESS)
    {
        //
        // Fail
        //
        DPRINT1("MDL PROBE FAILED!\n");
        Mdl->Process = NULL;
        ExRaiseStatus(ProbeStatus);
    }

    //
    // Get the PTE and PDE
    //
    PointerPte = MiAddressToPte(StartAddress);
    PointerPde = MiAddressToPde(StartAddress);
#if (_MI_PAGING_LEVELS >= 3)
    PointerPpe = MiAddressToPpe(StartAddress);
#endif
#if (_MI_PAGING_LEVELS == 4)
    PointerPxe = MiAddressToPxe(StartAddress);
#endif

    //
    // Sanity check
    //
    ASSERT(MdlPages == (PPFN_NUMBER)(Mdl + 1));

    //
    // Check what kind of operation this is
    //
    if (Operation != IoReadAccess)
    {
        //
        // Set the write flag
        //
        Mdl->MdlFlags |= MDL_WRITE_OPERATION;
    }
    else
    {
        //
        // Remove the write flag
        //
        Mdl->MdlFlags &= ~(MDL_WRITE_OPERATION);
    }

    //
    // Mark the MDL as locked *now*
    //
    Mdl->MdlFlags |= MDL_PAGES_LOCKED;

    //
    // Check if this came from kernel mode
    //
    if (Base > MM_HIGHEST_USER_ADDRESS)
    {
        //
        // We should not have a process
        //
        ASSERT(CurrentProcess == NULL);
        Mdl->Process = NULL;

        //
        // In kernel mode, we don't need to check for write access
        //
        Operation = IoReadAccess;

        //
        // Use the PFN lock
        //
        UsePfnLock = TRUE;
        OldIrql = MiAcquirePfnLock();
    }
    else
    {
        //
        // Sanity checks
        //
        ASSERT(TotalPages != 0);
        ASSERT(CurrentProcess == PsGetCurrentProcess());

        //
        // Track locked pages
        //
        InterlockedExchangeAddSizeT(&CurrentProcess->NumberOfLockedPages,
                                    TotalPages);

        //
        // Save the process
        //
        Mdl->Process = CurrentProcess;

        /* Lock the process working set */
        MiLockProcessWorkingSet(CurrentProcess, PsGetCurrentThread());
        UsePfnLock = FALSE;
        OldIrql = MM_NOIRQL;
    }

    //
    // Get the last PTE
    //
    LastPte = MiAddressToPte((PVOID)((ULONG_PTR)LastAddress - 1));

    //
    // Loop the pages
    //
    do
    {
        //
        // Assume failure and check for non-mapped pages
        //
        *MdlPages = LIST_HEAD;
        while (
#if (_MI_PAGING_LEVELS == 4)
               (PointerPxe->u.Hard.Valid == 0) ||
#endif
#if (_MI_PAGING_LEVELS >= 3)
               (PointerPpe->u.Hard.Valid == 0) ||
#endif
               (PointerPde->u.Hard.Valid == 0) ||
               (PointerPte->u.Hard.Valid == 0))
        {
            //
            // What kind of lock were we using?
            //
            if (UsePfnLock)
            {
                //
                // Release PFN lock
                //
                MiReleasePfnLock(OldIrql);
            }
            else
            {
                /* Release process working set */
                MiUnlockProcessWorkingSet(CurrentProcess, PsGetCurrentThread());
            }

            //
            // Access the page
            //
            Address = MiPteToAddress(PointerPte);

            //HACK: Pass a placeholder TrapInformation so the fault handler knows we're unlocked
            Status = MmAccessFault(FALSE, Address, KernelMode, (PVOID)(ULONG_PTR)0xBADBADA3BADBADA3ULL);
            if (!NT_SUCCESS(Status))
            {
                //
                // Fail
                //
                DPRINT1("Access fault failed\n");
                goto Cleanup;
            }

            //
            // What lock should we use?
            //
            if (UsePfnLock)
            {
                //
                // Grab the PFN lock
                //
                OldIrql = MiAcquirePfnLock();
            }
            else
            {
                /* Lock the process working set */
                MiLockProcessWorkingSet(CurrentProcess, PsGetCurrentThread());
            }
        }

        //
        // Check if this was a write or modify
        //
        if (Operation != IoReadAccess)
        {
            //
            // Check if the PTE is not writable
            //
            if (MI_IS_PAGE_WRITEABLE(PointerPte) == FALSE)
            {
                //
                // Check if it's copy on write
                //
                if (MI_IS_PAGE_COPY_ON_WRITE(PointerPte))
                {
                    //
                    // Get the base address and allow a change for user-mode
                    //
                    Address = MiPteToAddress(PointerPte);
                    if (Address <= MM_HIGHEST_USER_ADDRESS)
                    {
                        //
                        // What kind of lock were we using?
                        //
                        if (UsePfnLock)
                        {
                            //
                            // Release PFN lock
                            //
                            MiReleasePfnLock(OldIrql);
                        }
                        else
                        {
                            /* Release process working set */
                            MiUnlockProcessWorkingSet(CurrentProcess, PsGetCurrentThread());
                        }

                        //
                        // Access the page
                        //

                        //HACK: Pass a placeholder TrapInformation so the fault handler knows we're unlocked
                        Status = MmAccessFault(TRUE, Address, KernelMode, (PVOID)(ULONG_PTR)0xBADBADA3BADBADA3ULL);
                        if (!NT_SUCCESS(Status))
                        {
                            //
                            // Fail
                            //
                            DPRINT1("Access fault failed\n");
                            goto Cleanup;
                        }

                        //
                        // Re-acquire the lock
                        //
                        if (UsePfnLock)
                        {
                            //
                            // Grab the PFN lock
                            //
                            OldIrql = MiAcquirePfnLock();
                        }
                        else
                        {
                            /* Lock the process working set */
                            MiLockProcessWorkingSet(CurrentProcess, PsGetCurrentThread());
                        }

                        //
                        // Start over
                        //
                        continue;
                    }
                }

                //
                // Fail, since we won't allow this
                //
                Status = STATUS_ACCESS_VIOLATION;
                goto CleanupWithLock;
            }
        }

        //
        // Grab the PFN
        //
        PageFrameIndex = PFN_FROM_PTE(PointerPte);
        Pfn1 = MiGetPfnEntry(PageFrameIndex);
        if (Pfn1)
        {
            /* Either this is for kernel-mode, or the working set is held */
            ASSERT((CurrentProcess == NULL) || (UsePfnLock == FALSE));

            /* No Physical VADs supported yet */
            if (CurrentProcess) ASSERT(CurrentProcess->PhysicalVadRoot == NULL);

            /* This address should already exist and be fully valid */
            MiReferenceProbedPageAndBumpLockCount(Pfn1);
        }
        else
        {
            //
            // For I/O addresses, just remember this
            //
            Mdl->MdlFlags |= MDL_IO_SPACE;
        }

        //
        // Write the page and move on
        //
        *MdlPages++ = PageFrameIndex;
        PointerPte++;

        /* Check if we're on a PDE boundary */
        if (MiIsPteOnPdeBoundary(PointerPte)) PointerPde++;
#if (_MI_PAGING_LEVELS >= 3)
        if (MiIsPteOnPpeBoundary(PointerPte)) PointerPpe++;
#endif
#if (_MI_PAGING_LEVELS == 4)
        if (MiIsPteOnPxeBoundary(PointerPte)) PointerPxe++;
#endif

    } while (PointerPte <= LastPte);

    //
    // What kind of lock were we using?
    //
    if (UsePfnLock)
    {
        //
        // Release PFN lock
        //
        MiReleasePfnLock(OldIrql);
    }
    else
    {
        /* Release process working set */
        MiUnlockProcessWorkingSet(CurrentProcess, PsGetCurrentThread());
    }

    //
    // Sanity check
    //
    ASSERT((Mdl->MdlFlags & MDL_DESCRIBES_AWE) == 0);
    return;

CleanupWithLock:
    //
    // This is the failure path
    //
    ASSERT(!NT_SUCCESS(Status));

    //
    // What kind of lock were we using?
    //
    if (UsePfnLock)
    {
        //
        // Release PFN lock
        //
        MiReleasePfnLock(OldIrql);
    }
    else
    {
        /* Release process working set */
        MiUnlockProcessWorkingSet(CurrentProcess, PsGetCurrentThread());
    }
Cleanup:
    //
    // Pages must be locked so MmUnlock can work
    //
    ASSERT(Mdl->MdlFlags & MDL_PAGES_LOCKED);
    MmUnlockPages(Mdl);

    //
    // Raise the error
    //
    ExRaiseStatus(Status);
}

/*
 * @implemented
 */
VOID
NTAPI
MmUnlockPages(IN PMDL Mdl)
{
    PPFN_NUMBER MdlPages, LastPage;
    PEPROCESS Process;
    PVOID Base;
    ULONG Flags, PageCount;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    DPRINT("Unlocking MDL: %p\n", Mdl);

    //
    // Sanity checks
    //
    ASSERT((Mdl->MdlFlags & MDL_PAGES_LOCKED) != 0);
    ASSERT((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0);
    ASSERT((Mdl->MdlFlags & MDL_PARTIAL) == 0);
    ASSERT(Mdl->ByteCount != 0);

    //
    // Get the process associated and capture the flags which are volatile
    //
    Process = Mdl->Process;
    Flags = Mdl->MdlFlags;

    //
    // Automagically undo any calls to MmGetSystemAddressForMdl's for this MDL
    //
    if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
    {
        //
        // Unmap the pages from system space
        //
        MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
    }

    //
    // Get the page count
    //
    MdlPages = (PPFN_NUMBER)(Mdl + 1);
    Base = (PVOID)((ULONG_PTR)Mdl->StartVa + Mdl->ByteOffset);
    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Base, Mdl->ByteCount);
    ASSERT(PageCount != 0);

    //
    // We don't support AWE
    //
    if (Flags & MDL_DESCRIBES_AWE) ASSERT(FALSE);

    //
    // Check if the buffer is mapped I/O space
    //
    if (Flags & MDL_IO_SPACE)
    {
        //
        // Acquire PFN lock
        //
        OldIrql = MiAcquirePfnLock();

        //
        // Loop every page
        //
        LastPage = MdlPages + PageCount;
        do
        {
            //
            // Last page, break out
            //
            if (*MdlPages == LIST_HEAD) break;

            //
            // Check if this page is in the PFN database
            //
            Pfn1 = MiGetPfnEntry(*MdlPages);
            if (Pfn1) MiDereferencePfnAndDropLockCount(Pfn1);
        } while (++MdlPages < LastPage);

        //
        // Release the lock
        //
        MiReleasePfnLock(OldIrql);

        //
        // Check if we have a process
        //
        if (Process)
        {
            //
            // Handle the accounting of locked pages
            //
            ASSERT(Process->NumberOfLockedPages > 0);
            InterlockedExchangeAddSizeT(&Process->NumberOfLockedPages,
                                        -(LONG_PTR)PageCount);
        }

        //
        // We're done
        //
        Mdl->MdlFlags &= ~MDL_IO_SPACE;
        Mdl->MdlFlags &= ~MDL_PAGES_LOCKED;
        return;
    }

    //
    // Check if we have a process
    //
    if (Process)
    {
        //
        // Handle the accounting of locked pages
        //
        ASSERT(Process->NumberOfLockedPages > 0);
        InterlockedExchangeAddSizeT(&Process->NumberOfLockedPages,
                                    -(LONG_PTR)PageCount);
    }

    //
    // Loop every page
    //
    LastPage = MdlPages + PageCount;
    do
    {
        //
        // Last page reached
        //
        if (*MdlPages == LIST_HEAD)
        {
            //
            // Were there no pages at all?
            //
            if (MdlPages == (PPFN_NUMBER)(Mdl + 1))
            {
                //
                // We're already done
                //
                Mdl->MdlFlags &= ~MDL_PAGES_LOCKED;
                return;
            }

            //
            // Otherwise, stop here
            //
            LastPage = MdlPages;
            break;
        }

        /* Save the PFN entry instead for the secondary loop */
        *MdlPages = (PFN_NUMBER)MiGetPfnEntry(*MdlPages);
        ASSERT(*MdlPages != 0);
    } while (++MdlPages < LastPage);

    //
    // Reset pointer
    //
    MdlPages = (PPFN_NUMBER)(Mdl + 1);

    //
    // Now grab the PFN lock for the actual unlock and dereference
    //
    OldIrql = MiAcquirePfnLock();
    do
    {
        /* Get the current entry and reference count */
        Pfn1 = (PMMPFN)*MdlPages;
        MiDereferencePfnAndDropLockCount(Pfn1);
    } while (++MdlPages < LastPage);

    //
    // Release the lock
    //
    MiReleasePfnLock(OldIrql);

    //
    // We're done
    //
    Mdl->MdlFlags &= ~MDL_PAGES_LOCKED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmAdvanceMdl(IN PMDL Mdl,
             IN ULONG NumberOfBytes)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
MmMapLockedPagesWithReservedMapping(IN PVOID MappingAddress,
                                    IN ULONG PoolTag,
                                    IN PMDL MemoryDescriptorList,
                                    IN MEMORY_CACHING_TYPE CacheType)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
VOID
NTAPI
MmUnmapReservedMapping(IN PVOID BaseAddress,
                       IN ULONG PoolTag,
                       IN PMDL MemoryDescriptorList)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmPrefetchPages(IN ULONG NumberOfLists,
                IN PREAD_LIST *ReadLists)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmProtectMdlSystemAddress(IN PMDL MemoryDescriptorList,
                          IN ULONG NewProtect)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
MmProbeAndLockProcessPages(IN OUT PMDL MemoryDescriptorList,
                           IN PEPROCESS Process,
                           IN KPROCESSOR_MODE AccessMode,
                           IN LOCK_OPERATION Operation)
{
    UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID
NTAPI
MmProbeAndLockSelectedPages(IN OUT PMDL MemoryDescriptorList,
                            IN LARGE_INTEGER PageList[],
                            IN KPROCESSOR_MODE AccessMode,
                            IN LOCK_OPERATION Operation)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
MmMapMemoryDumpMdl(IN PMDL Mdl)
{
    UNIMPLEMENTED;
}

/* EOF */
