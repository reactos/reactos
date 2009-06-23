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
ULONG MmTotalSystemPtes;

/* PRIVATE FUNCTIONS **********************************************************/

PMMPTE
NTAPI
MiReserveAlignedSystemPtes(IN ULONG NumberOfPtes,
                           IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType,
                           IN ULONG Alignment)
{
    PMMPTE PointerPte, NextPte, PreviousPte;
    ULONG_PTR ClusterSize;
    
    //
    // Sanity check
    //
    ASSERT(Alignment <= PAGE_SIZE);
    
    //
    // Get the first free cluster and make sure we have PTEs available
    //
    PointerPte = &MmFirstFreeSystemPte[SystemPtePoolType];
    if (PointerPte->u.List.NextEntry == -1) return NULL;
    
    //
    // Now move to the first free system PTE cluster
    //
    PreviousPte = PointerPte;    
    PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
    
    //
    // Loop each cluster
    //
    while (TRUE)
    {
        //
        // Check if we're done to only one PTE left
        //
        if (!PointerPte->u.List.OneEntry)
        {
            //
            // Keep track of the next cluster in case we have to relink
            //
            NextPte = PointerPte + 1;
            
            //
            // Can this cluster satisfy the request?
            //
            ClusterSize = (ULONG_PTR)NextPte->u.List.NextEntry;
            if (NumberOfPtes < ClusterSize)
            {
                //
                // It can, and it will leave just one PTE left
                //
                if ((ClusterSize - NumberOfPtes) == 1)
                {
                    //
                    // This cluster becomes a single system PTE entry
                    //
                    PointerPte->u.List.OneEntry = 1;
                }
                else
                {
                    //
                    // Otherwise, the next cluster aborbs what's left
                    //
                    NextPte->u.List.NextEntry = ClusterSize - NumberOfPtes;
                }
                
                //
                // Decrement the free count and move to the next starting PTE
                //
                MmTotalFreeSystemPtes[SystemPtePoolType] -= NumberOfPtes;
                PointerPte += (ClusterSize - NumberOfPtes);
                break;
            }
            
            //
            // Did we find exactly what you wanted?
            //
            if (NumberOfPtes == ClusterSize)
            {
                //
                // Yes, fixup the cluster and decrease free system PTE count
                //
                PreviousPte->u.List.NextEntry = PointerPte->u.List.NextEntry;
                MmTotalFreeSystemPtes[SystemPtePoolType] -= NumberOfPtes;
                break;
            }            
        }
        else if (NumberOfPtes == 1)
        {
            //
            // We have one PTE in this cluster, and it's all you want
            //
            PreviousPte->u.List.NextEntry = PointerPte->u.List.NextEntry;
            MmTotalFreeSystemPtes[SystemPtePoolType]--;
            break;
        }
        
        //
        // We couldn't find what you wanted -- is this the last cluster?
        //
        if (PointerPte->u.List.NextEntry == -1) return NULL;

        //
        // Go to the next cluster
        //
        PreviousPte = PointerPte;
        PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
        ASSERT(PointerPte > PreviousPte);
    }   
    
    //
    // Flush the TLB and return the first PTE
    //
    KeFlushProcessTb();
    return PointerPte;
}

PMMPTE
NTAPI
MiReserveSystemPtes(IN ULONG NumberOfPtes,
                    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType)
{
    PMMPTE PointerPte;
    
    //
    // Use the extended function
    //
    PointerPte = MiReserveAlignedSystemPtes(NumberOfPtes, SystemPtePoolType, 0);
    ASSERT(PointerPte != NULL);
    return PointerPte;
}

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
    
    //
    // Check if this is the system PTE space
    //
    if (PoolType == SystemPteSpace)
    {
        //
        // Remember how many PTEs we have
        //
        MmTotalSystemPtes = NumberOfPtes;
    }
}

/* EOF */
