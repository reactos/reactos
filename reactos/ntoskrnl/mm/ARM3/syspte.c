/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/syspte.c
 * PURPOSE:         ARM Memory Manager System PTE Allocator
 * PROGRAMMERS:     ReactOS Portable Systems Group
 *                  Roel Messiant (roel.messiant@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* GLOBALS ********************************************************************/

PMMPTE MmSystemPteBase;
PMMPTE MmSystemPtesStart[MaximumPtePoolTypes];
PMMPTE MmSystemPtesEnd[MaximumPtePoolTypes];
MMPTE MmFirstFreeSystemPte[MaximumPtePoolTypes];
ULONG MmTotalFreeSystemPtes[MaximumPtePoolTypes];
ULONG MmTotalSystemPtes;
ULONG MiNumberOfExtraSystemPdes;

/* PRIVATE FUNCTIONS **********************************************************/

//
// The free System Page Table Entries are stored in a bunch of clusters,
// each consisting of one or more PTEs.  These PTE clusters are connected
// in a singly linked list, ordered by increasing cluster size.
//
// A cluster consisting of a single PTE is marked by having the OneEntry flag
// of its PTE set.  The forward link is contained in the NextEntry field.
//
// Clusters containing multiple PTEs have the OneEntry flag of their first PTE
// reset.  The NextEntry field of the first PTE contains the forward link, and
// the size of the cluster is stored in the NextEntry field of its second PTE.
//
// Reserving PTEs currently happens by walking the linked list until a cluster
// is found that contains the requested amount of PTEs or more.  This cluster
// is removed from the list, and the requested amount of PTEs is taken from the
// tail of this cluster.  If any PTEs remain in the cluster, the linked list is
// walked again until a second cluster is found that contains the same amount
// of PTEs or more.  The first cluster is then inserted in front of the second
// one.
//
// Releasing PTEs currently happens by walking the whole linked list, recording
// the first cluster that contains the amount of PTEs to release or more. When
// a cluster is found that is adjacent to the PTEs being released, this cluster
// is removed from the list and subsequently added to the PTEs being released.
// This ensures no two clusters are adjacent, which maximizes their size.
// After the walk is complete, a new cluster is created that contains the PTEs
// being released, which is then inserted in front of the recorded cluster.
//

FORCEINLINE
ULONG
MI_GET_CLUSTER_SIZE(IN PMMPTE Pte)
{
    //
    // First check for a single PTE
    //
    if (Pte->u.List.OneEntry)
        return 1;

    //
    // Then read the size from the trailing PTE
    //
    Pte++;
    return (ULONG)Pte->u.List.NextEntry;
}

PMMPTE
NTAPI
MiReserveAlignedSystemPtes(IN ULONG NumberOfPtes,
                           IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType,
                           IN ULONG Alignment)
{
    KIRQL OldIrql;
    PMMPTE PreviousPte, NextPte, ReturnPte;
    ULONG ClusterSize;

    //
    // Sanity check
    //
    ASSERT(Alignment <= PAGE_SIZE);

    //
    // Acquire the System PTE lock
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueueSystemSpaceLock);

    //
    // Find the last cluster in the list that doesn't contain enough PTEs
    //
    PreviousPte = &MmFirstFreeSystemPte[SystemPtePoolType];

    while (PreviousPte->u.List.NextEntry != MM_EMPTY_PTE_LIST)
    {
        //
        // Get the next cluster and its size
        //
        NextPte = MmSystemPteBase + PreviousPte->u.List.NextEntry;
        ClusterSize = MI_GET_CLUSTER_SIZE(NextPte);

        //
        // Check if this cluster contains enough PTEs
        //
        if (NumberOfPtes <= ClusterSize)
            break;

        //
        // On to the next cluster
        //
        PreviousPte = NextPte;
    }

    //
    // Make sure we didn't reach the end of the cluster list
    //
    if (PreviousPte->u.List.NextEntry == MM_EMPTY_PTE_LIST)
    {
        //
        // Release the System PTE lock and return failure
        //
        KeReleaseQueuedSpinLock(LockQueueSystemSpaceLock, OldIrql);
        return NULL;
    }

    //
    // Unlink the cluster
    //
    PreviousPte->u.List.NextEntry = NextPte->u.List.NextEntry;

    //
    // Check if the reservation spans the whole cluster
    //
    if (ClusterSize == NumberOfPtes)
    {
        //
        // Return the first PTE of this cluster
        //
        ReturnPte = NextPte;

        //
        // Zero the cluster
        //
        if (NextPte->u.List.OneEntry == 0)
        {
            NextPte->u.Long = 0;
            NextPte++;
        }
        NextPte->u.Long = 0;
    }
    else
    {
        //
        // Divide the cluster into two parts
        //
        ClusterSize -= NumberOfPtes;
        ReturnPte = NextPte + ClusterSize;

        //
        // Set the size of the first cluster, zero the second if needed
        //
        if (ClusterSize == 1)
        {
            NextPte->u.List.OneEntry = 1;
            ReturnPte->u.Long = 0;
        }
        else
        {
            NextPte++;
            NextPte->u.List.NextEntry = ClusterSize;
        }

        //
        // Step through the cluster list to find out where to insert the first
        //
        PreviousPte = &MmFirstFreeSystemPte[SystemPtePoolType];

        while (PreviousPte->u.List.NextEntry != MM_EMPTY_PTE_LIST)
        {
            //
            // Get the next cluster
            //
            NextPte = MmSystemPteBase + PreviousPte->u.List.NextEntry;

            //
            // Check if the cluster to insert is smaller or of equal size
            //
            if (ClusterSize <= MI_GET_CLUSTER_SIZE(NextPte))
                break;

            //
            // On to the next cluster
            //
            PreviousPte = NextPte;
        }

        //
        // Retrieve the first cluster and link it back into the cluster list
        //
        NextPte = ReturnPte - ClusterSize;

        NextPte->u.List.NextEntry = PreviousPte->u.List.NextEntry;
        PreviousPte->u.List.NextEntry = NextPte - MmSystemPteBase;
    }

    //
    // Decrease availability
    //
    MmTotalFreeSystemPtes[SystemPtePoolType] -= NumberOfPtes;

    //
    // Release the System PTE lock
    //
    KeReleaseQueuedSpinLock(LockQueueSystemSpaceLock, OldIrql);

    //
    // Flush the TLB
    //
    KeFlushProcessTb();

    //
    // Return the reserved PTEs
    //
    return ReturnPte;
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

    //
    // Check if allocation failed
    //
    if (!PointerPte)
    {
        //
        // Warn that we are out of memory
        //
        DPRINT1("MiReserveSystemPtes: Failed to reserve %lu PTE(s)!\n", NumberOfPtes);
    }

    //
    // Return the PTE Pointer
    //
    return PointerPte;
}

VOID
NTAPI
MiReleaseSystemPtes(IN PMMPTE StartingPte,
                    IN ULONG NumberOfPtes,
                    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType)
{
    KIRQL OldIrql;
    ULONG ClusterSize;
    PMMPTE PreviousPte, NextPte, InsertPte;

    //
    // Check to make sure the PTE address is within bounds
    //
    ASSERT(NumberOfPtes != 0);
    ASSERT(StartingPte >= MmSystemPtesStart[SystemPtePoolType]);
    ASSERT(StartingPte + NumberOfPtes - 1 <= MmSystemPtesEnd[SystemPtePoolType]);

    //
    // Zero PTEs
    //
    RtlZeroMemory(StartingPte, NumberOfPtes * sizeof(MMPTE));

    //
    // Acquire the System PTE lock
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueueSystemSpaceLock);

    //
    // Increase availability
    //
    MmTotalFreeSystemPtes[SystemPtePoolType] += NumberOfPtes;

    //
    // Step through the cluster list to find where to insert the PTEs
    //
    PreviousPte = &MmFirstFreeSystemPte[SystemPtePoolType];
    InsertPte = NULL;

    while (PreviousPte->u.List.NextEntry != MM_EMPTY_PTE_LIST)
    {
        //
        // Get the next cluster and its size
        //
        NextPte = MmSystemPteBase + PreviousPte->u.List.NextEntry;
        ClusterSize = MI_GET_CLUSTER_SIZE(NextPte);

        //
        // Check if this cluster is adjacent to the PTEs being released
        //
        if ((NextPte + ClusterSize == StartingPte) ||
            (StartingPte + NumberOfPtes == NextPte))
        {
            //
            // Add the PTEs in the cluster to the PTEs being released
            //
            NumberOfPtes += ClusterSize;

            if (NextPte < StartingPte)
                StartingPte = NextPte;

            //
            // Unlink this cluster and zero it
            //
            PreviousPte->u.List.NextEntry = NextPte->u.List.NextEntry;

            if (NextPte->u.List.OneEntry == 0)
            {
                NextPte->u.Long = 0;
                NextPte++;
            }
            NextPte->u.Long = 0;

            //
            // Invalidate the previously found insertion location, if any
            //
            InsertPte = NULL;
        }
        else
        {
            //
            // Check if the insertion location is right before this cluster
            //
            if ((InsertPte == NULL) && (NumberOfPtes <= ClusterSize))
                InsertPte = PreviousPte;

            //
            // On to the next cluster
            //
            PreviousPte = NextPte;
        }
    }

    //
    // If no insertion location was found, use the tail of the list
    //
    if (InsertPte == NULL)
        InsertPte = PreviousPte;

    //
    // Create a new cluster using the PTEs being released
    //
    if (NumberOfPtes != 1)
    {
        StartingPte->u.List.OneEntry = 0;

        NextPte = StartingPte + 1;
        NextPte->u.List.NextEntry = NumberOfPtes;
    }
    else
        StartingPte->u.List.OneEntry = 1;

    //
    // Link the new cluster into the cluster list at the insertion location
    //
    StartingPte->u.List.NextEntry = InsertPte->u.List.NextEntry;
    InsertPte->u.List.NextEntry = StartingPte - MmSystemPteBase;

    //
    // Release the System PTE lock
    //
    KeReleaseQueuedSpinLock(LockQueueSystemSpaceLock, OldIrql);
}

VOID
NTAPI
INIT_FUNCTION
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
    MmSystemPteBase = MI_SYSTEM_PTE_BASE;
    MmSystemPtesStart[PoolType] = StartingPte;
    MmSystemPtesEnd[PoolType] = StartingPte + NumberOfPtes - 1;
    DPRINT("System PTE space for %d starting at: %p and ending at: %p\n",
           PoolType, MmSystemPtesStart[PoolType], MmSystemPtesEnd[PoolType]);

    //
    // Clear all the PTEs to start with
    //
    RtlZeroMemory(StartingPte, NumberOfPtes * sizeof(MMPTE));

    //
    // Make the first entry free and link it
    //
    StartingPte->u.List.NextEntry = MM_EMPTY_PTE_LIST;
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
