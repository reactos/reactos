/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/syspte.c
 * PURPOSE:         ARM Memory Manager System PTE Allocator
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#line 15 "ARM³::SYSPTE"
#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

/* GLOBALS ********************************************************************/

PMMPTE MmSystemPteBase;
PMMPTE MmSystemPtesStart[MaximumPtePoolTypes];
PMMPTE MmSystemPtesEnd[MaximumPtePoolTypes];
MMPTE MmFirstFreeSystemPte[MaximumPtePoolTypes];
ULONG MmTotalFreeSystemPtes[MaximumPtePoolTypes];

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
MiInitializeSystemPtes(IN PMMPTE StartingPte,
                       IN ULONG NumberOfPtes,
                       IN MMSYSTEM_PTE_POOL_TYPE PoolType)
{
    //
    // Sanity checks
    //
    ASSERT(NumberOfPtes >= 1);
    ASSERT(PoolType == NonPagedPoolExpansion);
    
    //
    // Set the starting and ending PTE addresses for this space
    //
    MmSystemPteBase = (PVOID)PAGETABLE_MAP;
    MmSystemPtesStart[PoolType] = StartingPte;
    MmSystemPtesEnd[PoolType] = StartingPte + NumberOfPtes - 1;
    DPRINT1("System PTE space for %d starting at: %p and ending at: %p\n",
            PoolType, MmSystemPtesStart[PoolType], MmSystemPtesEnd[PoolType]);
    
    //
    // Clear all the PTEs to start with
    //
    RtlZeroMemory(StartingPte, NumberOfPtes * sizeof(MMPTE));
    
    //
    // Make the first entry free and link it
    //
    StartingPte->u.List.NextEntry = -1;
    MmFirstFreeSystemPte[PoolType].u.Long = 0;
    MmFirstFreeSystemPte[PoolType].u.List.NextEntry = StartingPte -
                                                      MmSystemPteBase;
    
    //
    // The second entry stores the size of this PTE space
    //
    StartingPte++;
    StartingPte->u.Long = 0;
    StartingPte->u.List.NextEntry = NumberOfPtes;
    
    //
    // We also keep a global for it
    //
    MmTotalFreeSystemPtes[PoolType] = NumberOfPtes;
}

/* EOF */
