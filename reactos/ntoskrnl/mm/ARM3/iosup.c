/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/iosup.c
 * PURPOSE:         ARM Memory Manager I/O Mapping Functionality
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* GLOBALS ********************************************************************/

//
// Each architecture has its own caching attributes for both I/O and Physical
// memory mappings.
//
// This describes the attributes for the x86 architecture. It eventually needs
// to go in the appropriate i386 directory.
//
MI_PFN_CACHE_ATTRIBUTE MiPlatformCacheAttributes[2][MmMaximumCacheType] =
{
    //
    // RAM
    //
    {MiNonCached,MiCached,MiWriteCombined,MiCached,MiNonCached,MiWriteCombined},

    //
    // Device Memory
    //
    {MiNonCached,MiCached,MiWriteCombined,MiCached,MiNonCached,MiWriteCombined},
};

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
PVOID
NTAPI
MmMapIoSpace(IN PHYSICAL_ADDRESS PhysicalAddress,
             IN SIZE_T NumberOfBytes,
             IN MEMORY_CACHING_TYPE CacheType)
{

    PFN_NUMBER Pfn;
    PFN_COUNT PageCount;
    PMMPTE PointerPte;
    PVOID BaseAddress;
    MMPTE TempPte;
    PMMPFN Pfn1 = NULL;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;
    BOOLEAN IsIoMapping;

    //
    // Must be called with a non-zero count
    //
    ASSERT(NumberOfBytes != 0);

    //
    // Make sure the upper bits are 0 if this system
    // can't describe more than 4 GB of physical memory.
    // FIXME: This doesn't respect PAE, but we currently don't
    // define a PAE build flag since there is no such build.
    //
#if !defined(_M_AMD64)
    ASSERT(PhysicalAddress.HighPart == 0);
#endif

    //
    // Normalize and validate the caching attributes
    //
    CacheType &= 0xFF;
    if (CacheType >= MmMaximumCacheType) return NULL;

    //
    // Calculate page count
    //
    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(PhysicalAddress.LowPart,
                                               NumberOfBytes);

    //
    // Compute the PFN and check if it's a known I/O mapping
    // Also translate the cache attribute
    //
    Pfn = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);
    Pfn1 = MiGetPfnEntry(Pfn);
    IsIoMapping = (Pfn1 == NULL) ? TRUE : FALSE;
    CacheAttribute = MiPlatformCacheAttributes[IsIoMapping][CacheType];

    //
    // Now allocate system PTEs for the mapping, and get the VA
    //
    PointerPte = MiReserveSystemPtes(PageCount, SystemPteSpace);
    if (!PointerPte) return NULL;
    BaseAddress = MiPteToAddress(PointerPte);

    //
    // Check if this is uncached
    //
    if (CacheAttribute != MiCached)
    {
        //
        // Flush all caches
        //
        KeFlushEntireTb(TRUE, TRUE);
        KeInvalidateAllCaches();
    }

    //
    // Now compute the VA offset
    //
    BaseAddress = (PVOID)((ULONG_PTR)BaseAddress +
                          BYTE_OFFSET(PhysicalAddress.LowPart));

    //
    // Get the template and configure caching
    //
    TempPte = ValidKernelPte;
    switch (CacheAttribute)
    {
        case MiNonCached:

            //
            // Disable the cache
            //
            MI_PAGE_DISABLE_CACHE(&TempPte);
            MI_PAGE_WRITE_THROUGH(&TempPte);
            break;

        case MiCached:

            //
            // Leave defaults
            //
            break;

        case MiWriteCombined:

            //
            // We don't support write combining yet
            //
            ASSERT(FALSE);
            break;

        default:

            //
            // Should never happen
            //
            ASSERT(FALSE);
            break;
    }

    //
    // Sanity check and re-flush
    //
    Pfn = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);
    ASSERT((Pfn1 == MiGetPfnEntry(Pfn)) || (Pfn1 == NULL));
    KeFlushEntireTb(TRUE, TRUE);
    KeInvalidateAllCaches();

    //
    // Do the mapping
    //
    do
    {
        //
        // Write the PFN
        //
        TempPte.u.Hard.PageFrameNumber = Pfn++;
        MI_WRITE_VALID_PTE(PointerPte++, TempPte);
    } while (--PageCount);

    //
    // We're done!
    //
    return BaseAddress;
}

/*
 * @implemented
 */
VOID
NTAPI
MmUnmapIoSpace(IN PVOID BaseAddress,
               IN SIZE_T NumberOfBytes)
{
    PFN_NUMBER Pfn;
    PFN_COUNT PageCount;
    PMMPTE PointerPte;

    //
    // Sanity check
    //
    ASSERT(NumberOfBytes != 0);

    //
    // Get the page count
    //
    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(BaseAddress, NumberOfBytes);

    //
    // Get the PTE and PFN
    //
    PointerPte = MiAddressToPte(BaseAddress);
    Pfn = PFN_FROM_PTE(PointerPte);

    //
    // Is this an I/O mapping?
    //
    if (!MiGetPfnEntry(Pfn))
    {
        //
        // Destroy the PTE
        //
        RtlZeroMemory(PointerPte, PageCount * sizeof(MMPTE));

        //
        // Blow the TLB
        //
        KeFlushEntireTb(TRUE, TRUE);
    }

    //
    // Release the PTEs
    //
    MiReleaseSystemPtes(PointerPte, PageCount, 0);
}

/*
 * @implemented
 */
PVOID
NTAPI
MmMapVideoDisplay(IN PHYSICAL_ADDRESS PhysicalAddress,
                  IN SIZE_T NumberOfBytes,
                  IN MEMORY_CACHING_TYPE CacheType)
{
    PAGED_CODE();

    //
    // Call the real function
    //
    return MmMapIoSpace(PhysicalAddress, NumberOfBytes, CacheType);
}

/*
 * @implemented
 */
VOID
NTAPI
MmUnmapVideoDisplay(IN PVOID BaseAddress,
                    IN SIZE_T NumberOfBytes)
{
    //
    // Call the real function
    //
    MmUnmapIoSpace(BaseAddress, NumberOfBytes);
}

LOGICAL
NTAPI
MmIsIoSpaceActive(IN PHYSICAL_ADDRESS StartAddress,
                  IN SIZE_T NumberOfBytes)
{
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */
