/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/ncache.c
 * PURPOSE:         ARM Memory Manager Noncached Memory Allocator
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* GLOBALS ********************************************************************/

/*
 * @implemented
 */
PVOID
NTAPI
MmAllocateNonCachedMemory(IN SIZE_T NumberOfBytes)
{
    PFN_COUNT PageCount, MdlPageCount;
    PFN_NUMBER PageFrameIndex;
    PHYSICAL_ADDRESS LowAddress, HighAddress, SkipBytes;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;
    PMDL Mdl;
    PVOID BaseAddress;
    PPFN_NUMBER MdlPages;
    PMMPTE PointerPte;
    MMPTE TempPte;

    //
    // Get the page count
    //
    ASSERT(NumberOfBytes != 0);
    PageCount = (PFN_COUNT)BYTES_TO_PAGES(NumberOfBytes);

    //
    // Use the MDL allocator for simplicity, so setup the parameters
    //
    LowAddress.QuadPart = 0;
    HighAddress.QuadPart = -1;
    SkipBytes.QuadPart = 0;
    CacheAttribute = MiPlatformCacheAttributes[0][MmNonCached];

    //
    // Now call the MDL allocator
    //
    Mdl = MiAllocatePagesForMdl(LowAddress,
                                HighAddress,
                                SkipBytes,
                                NumberOfBytes,
                                CacheAttribute,
                                0);
    if (!Mdl) return NULL;

    //
    // Get the MDL VA and check how many pages we got (could be partial)
    //
    BaseAddress = (PVOID)((ULONG_PTR)Mdl->StartVa + Mdl->ByteOffset);
    MdlPageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(BaseAddress, Mdl->ByteCount);
    if (PageCount != MdlPageCount)
    {
        //
        // Unlike MDLs, partial isn't okay for a noncached allocation, so fail
        //
        ASSERT(PageCount > MdlPageCount);
        MmFreePagesFromMdl(Mdl);
        ExFreePoolWithTag(Mdl, TAG_MDL);
        return NULL;
    }

    //
    // Allocate system PTEs for the base address
    // We use an extra page to store the actual MDL pointer for the free later
    //
    PointerPte = MiReserveSystemPtes(PageCount + 1, SystemPteSpace);
    if (!PointerPte)
    {
        //
        // Out of memory...
        //
        MmFreePagesFromMdl(Mdl);
        ExFreePoolWithTag(Mdl, TAG_MDL);
        return NULL;
    }

    //
    // Store the MDL pointer
    //
    *(PMDL*)PointerPte++ = Mdl;

    //
    // Okay, now see what range we got
    //
    BaseAddress = MiPteToAddress(PointerPte);

    //
    // This is our array of pages
    //
    MdlPages = (PPFN_NUMBER)(Mdl + 1);

    //
    // Setup the template PTE
    //
    TempPte = ValidKernelPte;

    //
    // Now check what kind of caching we should use
    //
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
    // Now loop the MDL pages
    //
    do
    {
        //
        // Get the PFN
        //
        PageFrameIndex = *MdlPages++;

        //
        // Set the PFN in the page and write it
        //
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        MI_WRITE_VALID_PTE(PointerPte++, TempPte);
    } while (--PageCount);

    //
    // Return the base address
    //
    return BaseAddress;

}

/*
 * @implemented
 */
VOID
NTAPI
MmFreeNonCachedMemory(IN PVOID BaseAddress,
                      IN SIZE_T NumberOfBytes)
{
    PMDL Mdl;
    PMMPTE PointerPte;
    PFN_COUNT PageCount;

    //
    // Sanity checks
    //
    ASSERT(NumberOfBytes != 0);
    ASSERT(PAGE_ALIGN(BaseAddress) == BaseAddress);

    //
    // Get the page count
    //
    PageCount = (PFN_COUNT)BYTES_TO_PAGES(NumberOfBytes);

    //
    // Get the first PTE
    //
    PointerPte = MiAddressToPte(BaseAddress);

    //
    // Remember this is where we store the shadow MDL pointer
    //
    Mdl = *(PMDL*)(--PointerPte);

    //
    // Kill the MDL (and underlying pages)
    //
    MmFreePagesFromMdl(Mdl);
    ExFreePoolWithTag(Mdl, TAG_MDL);

    //
    // Now free the system PTEs for the underlying VA
    //
    MiReleaseSystemPtes(PointerPte, PageCount + 1, SystemPteSpace);
}

/* EOF */
