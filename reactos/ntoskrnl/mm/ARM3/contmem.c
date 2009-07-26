/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/contmem.c
 * PURPOSE:         ARM Memory Manager Contiguous Memory Allocator
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#line 15 "ARM³::CONTMEM"
#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

/* PRIVATE FUNCTIONS **********************************************************/

PVOID
NTAPI
MiCheckForContiguousMemory(IN PVOID BaseAddress,
                           IN PFN_NUMBER BaseAddressPages,
                           IN PFN_NUMBER SizeInPages,
                           IN PFN_NUMBER LowestPfn,
                           IN PFN_NUMBER HighestPfn,
                           IN PFN_NUMBER BoundaryPfn,
                           IN MI_PFN_CACHE_ATTRIBUTE CacheAttribute)
{
    PMMPTE StartPte, EndPte;
    PFN_NUMBER PreviousPage = 0, Page, HighPage, BoundaryMask, Pages = 0;
    
    //
    // Okay, first of all check if the PFNs match our restrictions
    //
    if (LowestPfn > HighestPfn) return NULL;
    if (LowestPfn + SizeInPages <= LowestPfn) return NULL;
    if (LowestPfn + SizeInPages - 1 > HighestPfn) return NULL;
    if (BaseAddressPages < SizeInPages) return NULL;
    
    //
    // This is the last page we need to get to and the boundary requested
    //
    HighPage = HighestPfn + 1 - SizeInPages;
    BoundaryMask = ~(BoundaryPfn - 1);
    
    //
    // And here's the PTEs for this allocation. Let's go scan them.
    //
    StartPte = MiAddressToPte(BaseAddress);
    EndPte = StartPte + BaseAddressPages;
    while (StartPte < EndPte)
    {
        //
        // Get this PTE's page number
        //
        ASSERT (StartPte->u.Hard.Valid == 1);
        Page = PFN_FROM_PTE(StartPte);
        
        //
        // Is this the beginning of our adventure?
        //
        if (!Pages)
        {
            //
            // Check if this PFN is within our range
            //
            if ((Page >= LowestPfn) && (Page <= HighPage))
            {
                //
                // It is! Do you care about boundary (alignment)?
                //
                if (!(BoundaryPfn) ||
                    (!((Page ^ (Page + SizeInPages - 1)) & BoundaryMask)))
                {
                    //
                    // You don't care, or you do care but we deliver
                    //
                    Pages++;
                }
            }
            
            //
            // Have we found all the pages we need by now?
            // Incidently, this means you only wanted one page
            //
            if (Pages == SizeInPages)
            {
                //
                // Mission complete
                //
                return MiPteToAddress(StartPte);
            }
        }
        else
        {
            //
            // Have we found a page that doesn't seem to be contiguous?
            //
            if (Page != (PreviousPage + 1))
            {
                //
                // Ah crap, we have to start over
                //
                Pages = 0;
                continue;
            }
            
            //
            // Otherwise, we're still in the game. Do we have all our pages?
            //
            if (++Pages == SizeInPages)
            {
                //
                // We do! This entire range was contiguous, so we'll return it!
                //
                return MiPteToAddress(StartPte - Pages + 1);
            }
        }
        
        //
        // Try with the next PTE, remember this PFN
        //
        PreviousPage = Page;
        StartPte++;
        continue;
    }
    
    //
    // All good returns are within the loop...
    //
    return NULL;
}

PVOID
NTAPI
MiFindContiguousMemory(IN PFN_NUMBER LowestPfn,
                       IN PFN_NUMBER HighestPfn,
                       IN PFN_NUMBER BoundaryPfn,
                       IN PFN_NUMBER SizeInPages,
                       IN MEMORY_CACHING_TYPE CacheType)
{
    PFN_NUMBER Page;
    PHYSICAL_ADDRESS PhysicalAddress;
    PAGED_CODE ();
    ASSERT(SizeInPages != 0);

    //
    // Our last hope is to scan the free page list for contiguous pages
    //
    Page = MiFindContiguousPages(LowestPfn,
                                 HighestPfn,
                                 BoundaryPfn,
                                 SizeInPages,
                                 CacheType);
    if (!Page) return NULL;
    
    //
    // We'll just piggyback on the I/O memory mapper
    //
    PhysicalAddress.QuadPart = Page << PAGE_SHIFT;
    return MmMapIoSpace(PhysicalAddress, SizeInPages << PAGE_SHIFT, CacheType);
}

PVOID
NTAPI
MiAllocateContiguousMemory(IN SIZE_T NumberOfBytes,
                           IN PFN_NUMBER LowestAcceptablePfn,
                           IN PFN_NUMBER HighestAcceptablePfn,
                           IN PFN_NUMBER BoundaryPfn,
                           IN MEMORY_CACHING_TYPE CacheType)
{
    PVOID BaseAddress;
    PFN_NUMBER SizeInPages;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;   
    ASSERT(NumberOfBytes != 0);
    
    //
    // Compute size requested
    //
    SizeInPages = BYTES_TO_PAGES(NumberOfBytes);
    
    //
    // Convert the cache attribute and check for cached requests
    //
    CacheAttribute = MiPlatformCacheAttributes[FALSE][CacheType];
    if (CacheAttribute == MiCached)
    {
        //
        // Because initial nonpaged pool is supposed to be contiguous, go ahead
        // and try making a nonpaged pool allocation first.
        //
        BaseAddress = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                            NumberOfBytes,
                                            'mCmM');
        if (BaseAddress)
        {    
            //
            // Now make sure it's actually contiguous (if it came from expansion
            // it might not be).
            //
            if (MiCheckForContiguousMemory(BaseAddress,
                                           SizeInPages,
                                           SizeInPages,
                                           LowestAcceptablePfn,
                                           HighestAcceptablePfn,
                                           BoundaryPfn,
                                           CacheAttribute))
            {
                //
                // Sweet, we're in business!
                //
                return BaseAddress;
            }
            
            //
            // No such luck
            //
            ExFreePool(BaseAddress);
        }
    }
    
    //
    // According to MSDN, the system won't try anything else if you're higher
    // than APC level.
    //
    if (KeGetCurrentIrql() > APC_LEVEL) return NULL;
    
    //
    // Otherwise, we'll go try to find some
    //
    return MiFindContiguousMemory(LowestAcceptablePfn,
                                  HighestAcceptablePfn,
                                  BoundaryPfn,
                                  SizeInPages,
                                  CacheType);
}

VOID
NTAPI
MiFreeContiguousMemory(IN PVOID BaseAddress)
{
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex, LastPage, PageCount;
    PMMPFN Pfn1, StartPfn;
    PAGED_CODE();
    
    //
    // First, check if the memory came from initial nonpaged pool, or expansion
    //
    if (((BaseAddress >= MmNonPagedPoolStart) &&
         (BaseAddress < (PVOID)((ULONG_PTR)MmNonPagedPoolStart +
                                MmSizeOfNonPagedPoolInBytes))) ||
        ((BaseAddress >= MmNonPagedPoolExpansionStart) &&
         (BaseAddress < MmNonPagedPoolEnd)))
    {
        //
        // It did, so just use the pool to free this
        //
        ExFreePool(BaseAddress);
        return;
    }
    
    //
    // Otherwise, get the PTE and page number for the allocation
    //
    PageFrameIndex = PFN_FROM_PTE(MiAddressToPte(BaseAddress));
    
    //
    // Now get the PFN entry for this, and make sure it's the correct one
    //
    Pfn1 = MiGetPfnEntry(PageFrameIndex);        
    if (Pfn1->u3.e1.StartOfAllocation == 0)
    {
        //
        // This probably means you did a free on an address that was in between
        //
        KeBugCheckEx (BAD_POOL_CALLER,
                      0x60,
                      (ULONG_PTR)BaseAddress,
                      0,
                      0);
    }
    
    //
    // Now this PFN isn't the start of any allocation anymore, it's going out
    //
    StartPfn = Pfn1;
    Pfn1->u3.e1.StartOfAllocation = 0;
    
    //
    // Look the PFNs
    //
    do
    {
        //
        // Until we find the one that marks the end of the allocation
        //
    } while (Pfn1++->u3.e1.EndOfAllocation == 0);
    
    //
    // Found it, unmark it
    //
    Pfn1--;
    Pfn1->u3.e1.EndOfAllocation = 0;
    
    //
    // Now compute how many pages this represents
    //
    PageCount = (ULONG)(Pfn1 - StartPfn + 1);
    
    //
    // So we can know how much to unmap (recall we piggyback on I/O mappings)
    //
    MmUnmapIoSpace(BaseAddress, PageCount << PAGE_SHIFT);
    
    //
    // Lock the PFN database
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    
    //
    // Loop all the pages
    //
    LastPage = PageFrameIndex + PageCount;    
    do
    {
        //
        // Free each one, and move on
        //
        MmReleasePageMemoryConsumer(MC_NPPOOL, PageFrameIndex);
    } while (++PageFrameIndex < LastPage);
    
    //
    // Release the PFN lock
    //
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
PVOID
NTAPI
MmAllocateContiguousMemorySpecifyCache(IN SIZE_T NumberOfBytes,
                                       IN PHYSICAL_ADDRESS LowestAcceptableAddress OPTIONAL,
                                       IN PHYSICAL_ADDRESS HighestAcceptableAddress,
                                       IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
                                       IN MEMORY_CACHING_TYPE CacheType OPTIONAL)
{
    PFN_NUMBER LowestPfn, HighestPfn, BoundaryPfn;
    ASSERT (NumberOfBytes != 0);
    
    //
    // Convert the lowest address into a PFN
    //
    LowestPfn = (PFN_NUMBER)(LowestAcceptableAddress.QuadPart >> PAGE_SHIFT);
    if (BYTE_OFFSET(LowestAcceptableAddress.LowPart)) LowestPfn++;
    
    //
    // Convert and validate the boundary address into a PFN
    //
    if (BYTE_OFFSET(BoundaryAddressMultiple.LowPart)) return NULL;
    BoundaryPfn = (PFN_NUMBER)(BoundaryAddressMultiple.QuadPart >> PAGE_SHIFT);
    
    //
    // Convert the highest address into a PFN
    //
    HighestPfn = (PFN_NUMBER)(HighestAcceptableAddress.QuadPart >> PAGE_SHIFT);
    if (HighestPfn > MmHighestPhysicalPage) HighestPfn = MmHighestPhysicalPage;
    
    //
    // Validate the PFN bounds
    //
    if (LowestPfn > HighestPfn) return NULL;
    
    //
    // Let the contiguous memory allocator handle it
    //    
    return MiAllocateContiguousMemory(NumberOfBytes,
                                      LowestPfn,
                                      HighestPfn,
                                      BoundaryPfn,
                                      CacheType);
}

/*
 * @implemented
 */
PVOID
NTAPI
MmAllocateContiguousMemory(IN ULONG NumberOfBytes,
                           IN PHYSICAL_ADDRESS HighestAcceptableAddress)
{
    PFN_NUMBER HighestPfn;
    
    //
    // Convert and normalize the highest address into a PFN
    //
    HighestPfn = (PFN_NUMBER)(HighestAcceptableAddress.QuadPart >> PAGE_SHIFT);
    if (HighestPfn > MmHighestPhysicalPage) HighestPfn = MmHighestPhysicalPage;
    
    //
    // Let the contiguous memory allocator handle it
    //    
    return MiAllocateContiguousMemory(NumberOfBytes, 0, HighestPfn, 0, MmCached);
}

/*
 * @implemented
 */
VOID
NTAPI
MmFreeContiguousMemory(IN PVOID BaseAddress)
{
    //
    // Let the contiguous memory allocator handle it
    //
    MiFreeContiguousMemory(BaseAddress);
}

/*
 * @implemented
 */
VOID
NTAPI
MmFreeContiguousMemorySpecifyCache(IN PVOID BaseAddress,
                                   IN ULONG NumberOfBytes,
                                   IN MEMORY_CACHING_TYPE CacheType)
{
    //
    // Just call the non-cached version (there's no cache issues for freeing)
    //
    MiFreeContiguousMemory(BaseAddress);
}

/* EOF */
