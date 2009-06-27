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
MiReleaseSystemPtes(IN PMMPTE StartingPte,
                    IN ULONG NumberOfPtes,
                    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType)
{
    ULONG_PTR ClusterSize, CurrentSize;
    PMMPTE CurrentPte, NextPte, PointerPte;
    
    //
    // Check to make sure the PTE address is within bounds.
    //
    ASSERT(NumberOfPtes != 0);
    ASSERT(StartingPte >= MmSystemPtesStart[SystemPtePoolType]);
    ASSERT(StartingPte <= MmSystemPtesEnd[SystemPtePoolType]);
    
    //
    // Zero PTEs.
    //
    RtlZeroMemory(StartingPte, NumberOfPtes * sizeof (MMPTE));
  
    //
    // Increase availability
    //
    MmTotalFreeSystemPtes[SystemPtePoolType] += NumberOfPtes;
    
    //
    // Get the free cluster and start going through them
    //
    CurrentSize = (ULONG_PTR)(StartingPte - MmSystemPteBase);
    CurrentPte = &MmFirstFreeSystemPte[SystemPtePoolType];
    while (TRUE)
    {
        //
        // Get the first real cluster of PTEs and check if it's ours
        //
        PointerPte = MmSystemPteBase + CurrentPte->u.List.NextEntry;
        if (CurrentSize < CurrentPte->u.List.NextEntry)
        {
            //
            // Sanity check
            //
            ASSERT(((StartingPte + NumberOfPtes) <= PointerPte) ||
                   (CurrentPte->u.List.NextEntry == -1));
            
            //
            // Get the next cluster in case it's the one
            //
            NextPte = CurrentPte + 1;
            
            //
            // Check if this was actually a single-PTE entry
            //
            if (CurrentPte->u.List.OneEntry)
            {
                //
                // We only have one page
                //
                ClusterSize = 1;
            }
            else
            {
                //
                // The next cluster will have the page count
                //
                ClusterSize = (ULONG_PTR)NextPte->u.List.NextEntry;
            }
            
            //
            // So check if this cluster actually describes the entire mapping
            //
            if ((CurrentPte + ClusterSize) == StartingPte)
            {
                //
                // It does -- collapse the free PTEs into the next cluster
                //
                NumberOfPtes += ClusterSize;
                NextPte->u.List.NextEntry = NumberOfPtes;
                CurrentPte->u.List.OneEntry = 0;
                
                //
                // Make another pass
                //
                StartingPte = CurrentPte;
            }
            else
            {
                //
                // There's still PTEs left -- make us into a cluster
                //
                StartingPte->u.List.NextEntry = CurrentPte->u.List.NextEntry;
                CurrentPte->u.List.NextEntry = CurrentSize;
                
                //
                // Is there just one page left?
                //
                if (NumberOfPtes == 1)
                {
                    //
                    // Then this actually becomes a single PTE entry
                    //
                    StartingPte->u.List.OneEntry = 1;
                }
                else
                {
                    //
                    // Otherwise, create a new cluster for the remaining pages
                    //
                    StartingPte->u.List.OneEntry = 0;
                    NextPte = StartingPte + 1;
                    NextPte->u.List.NextEntry = NumberOfPtes;
                }
            }
            
            //
            // Now check if we've arrived at yet another cluster
            //
            if ((StartingPte + NumberOfPtes) == PointerPte)
            {
                //
                // We'll collapse the next cluster into us
                //
                StartingPte->u.List.NextEntry = PointerPte->u.List.NextEntry;
                StartingPte->u.List.OneEntry = 0;
                NextPte = StartingPte + 1;
                
                //
                // Check if the cluster only had one page
                //
                if (PointerPte->u.List.OneEntry)
                {
                    //
                    // So will we...
                    //
                    ClusterSize = 1;
                }
                else
                {
                    //
                    // Otherwise, grab the page count from the next-next cluster
                    //
                    PointerPte++;
                    ClusterSize = (ULONG_PTR)PointerPte->u.List.NextEntry;
                }
                
                //
                // And create the final combined cluster
                //
                NextPte->u.List.NextEntry = NumberOfPtes + ClusterSize;
            }
            
            //
            // We released the PTEs into their cluster (and optimized the list)
            //
            break;
        }
        
        //
        // Try the next cluster of PTEs...
        //
        CurrentPte = PointerPte;
    }
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
