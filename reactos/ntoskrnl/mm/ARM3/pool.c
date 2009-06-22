/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/pool.c
 * PURPOSE:         ARM Memory Manager Pool Allocator
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#line 15 "ARM³::POOL"
#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

/* GLOBALS ********************************************************************/

LIST_ENTRY MmNonPagedPoolFreeListHead[MI_MAX_FREE_PAGE_LISTS];
PFN_NUMBER MmNumberOfFreeNonPagedPool, MiExpansionPoolPagesInitialCharge;
PVOID MmNonPagedPoolEnd0;
PFN_NUMBER MiStartOfInitialPoolFrame, MiEndOfInitialPoolFrame;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
MiInitializeArmPool(VOID)
{
    ULONG i;
    PFN_NUMBER PoolPages;
    PMMFREE_POOL_ENTRY FreeEntry, FirstEntry;
    PMMPTE PointerPte;
    PAGED_CODE();

    //
    // We keep 4 lists of free pages (4 lists help avoid contention)
    //
    for (i = 0; i < MI_MAX_FREE_PAGE_LISTS; i++)
    {
        //
        // Initialize each of them
        //
        InitializeListHead(&MmNonPagedPoolFreeListHead[i]);
    }

    //
    // Calculate how many pages the initial nonpaged pool has
    //
    PoolPages = BYTES_TO_PAGES(MmSizeOfNonPagedPoolInBytes);
    MmNumberOfFreeNonPagedPool = PoolPages;
       
    //
    // Initialize the first free entry
    //
    FreeEntry = MmNonPagedPoolStart;
    FirstEntry = FreeEntry;
    FreeEntry->Size = PoolPages;
    FreeEntry->Owner = FirstEntry;

    //
    // Insert it into the last list
    //
    InsertHeadList(&MmNonPagedPoolFreeListHead[MI_MAX_FREE_PAGE_LISTS - 1],
                   &FreeEntry->List);
    
    //
    // Now create free entries for every single other page
    //
    while (PoolPages-- > 1)
    {
        //
        // Link them all back to the original entry
        //
        FreeEntry = (PMMFREE_POOL_ENTRY)((ULONG_PTR)FreeEntry + PAGE_SIZE);
        FreeEntry->Owner = FirstEntry;
    }

    //
    // Validate and remember first allocated pool page
    //
    PointerPte = MiAddressToPte(MmNonPagedPoolStart);
    ASSERT(PointerPte->u.Hard.Valid == 1);
    MiStartOfInitialPoolFrame = PFN_FROM_PTE(PointerPte);
    
    //
    // Keep track of where initial nonpaged pool ends
    //
    MmNonPagedPoolEnd0 = (PVOID)((ULONG_PTR)MmNonPagedPoolStart +
                                 MmSizeOfNonPagedPoolInBytes);
    
    //
    // Validate and remember last allocated pool page
    //
    PointerPte = MiAddressToPte((PVOID)((ULONG_PTR)MmNonPagedPoolEnd0 - 1));
    ASSERT(PointerPte->u.Hard.Valid == 1);
    MiEndOfInitialPoolFrame = PFN_FROM_PTE(PointerPte);
    
    //
    // Validate the first nonpaged pool expansion page (which is a guard page)
    //
    PointerPte = MiAddressToPte(MmNonPagedPoolExpansionStart);
    ASSERT(PointerPte->u.Hard.Valid == 0);
    
    //
    // Calculate the size of the expansion region alone
    //
    MiExpansionPoolPagesInitialCharge =
    BYTES_TO_PAGES(MmMaximumNonPagedPoolInBytes - MmSizeOfNonPagedPoolInBytes);
    
    //
    // Remove 2 pages, since there's a guard page on top and on the bottom
    //
    MiExpansionPoolPagesInitialCharge -= 2;
    
    //
    // Now initialize the nonpaged pool expansion PTE space. Remember there's a
    // guard page on top so make sure to skip it. The bottom guard page will be
    // guaranteed by the fact our size is off by one.
    //
    MiInitializeSystemPtes(PointerPte + 1,
                           MiExpansionPoolPagesInitialCharge,
                           NonPagedPoolExpansion);
}

/* EOF */
