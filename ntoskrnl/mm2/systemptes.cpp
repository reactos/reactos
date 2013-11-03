/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/systemptes.cpp
 * PURPOSE:         System PTEs implementation
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "memorymanager.hpp"

//#define NDEBUG
#include <debug.h>

// http://msdn.microsoft.com/en-us/magazine/dd420461.aspx

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

VOID
SYSTEM_PTES::
EstimateNumber(ULONG NumberOfPages)
{
    // For now set to default. Fine-tuning possible later.
    NumberOfSystemPtes = 22000;
}


VOID
SYSTEM_PTES::
Initialize(IN PTENTRY *StartingPte, IN PFN_COUNT NumberOfPtes, IN MMSYSTEM_PTE_POOL_TYPE PoolType)
{
    // Preinit some stuff
    FlushEntry = 0;
    FlushPte = NULL;

    // Set the starting and ending PTE addresses for this space
    Base = PTENTRY::AddressToPte(0xC0000000);
    Start[PoolType] = StartingPte;
    End[PoolType] = StartingPte + NumberOfPtes - 1;
    DPRINT("System PTE space for %d starting at: %p and ending at: %p\n", PoolType, Start[PoolType], End[PoolType]);

    // Check if the system PTEs are exhausted
    if (NumberOfPtes == 0 || NumberOfPtes == 1)
    {
        DPRINT1("Not enough free system PTEs left!\n");
        FirstFree[PoolType].SetZero();
        FirstFree[PoolType].SetNextEntryEmpty();
        return;
    }

    // Clear all the PTEs to start with
    RtlZeroMemory(StartingPte, NumberOfPtes * sizeof(PTENTRY));

    // Make the first entry free and link it
    StartingPte->SetNextEntryEmpty();
    FirstFree[PoolType].SetZero();
    FirstFree[PoolType].SetNextEntry(StartingPte - Base);

    // The second entry stores the size of this PTE space
    StartingPte++;
    StartingPte->SetZero();
    StartingPte->SetNextEntry(NumberOfPtes);

    // We also keep a global for it
    TotalFree[PoolType] = NumberOfPtes;
}

VOID
SYSTEM_PTES::
Grow()
{
    // Start from the paged pool end
    ULONG_PTR EndPointer = (ULONG_PTR)((PCHAR)MemoryManager->Pools.PagedPoolPtr.End + 1);
    PTENTRY *PointerPde = PTENTRY::AddressToPde(EndPointer);
    PTENTRY *StartingPte = PTENTRY::AddressToPte(EndPointer);
    ULONG j = 0;

    PTENTRY TempPte = MemoryManager->ValidKernelPde;

    // Acquire PFN DB lock
    KIRQL OldIrql = MemoryManager->PfnDb.AcquireLock();

    while (!PointerPde->IsValid())
    {
        // Get new page from the PFN DB and map it
        ULONG PageFrameIndex = MemoryManager->PfnDb.RemoveAnyPage(MemoryManager->GetPageColor());
        TempPte.SetPfn(PageFrameIndex);
        *PointerPde = TempPte;
        MemoryManager->PfnDb.InitializeElement(PageFrameIndex, PointerPde, 1);
        PointerPde++;
        StartingPte += PAGE_SIZE / sizeof(PTENTRY);
        j += PAGE_SIZE / sizeof(PTENTRY);
    }

    // Release PFN DB lock
    MemoryManager->PfnDb.ReleaseLock(OldIrql);

    // If we mapped anything
    if (j > 0)
    {
        // Get starting PTE pointer again
        StartingPte = PTENTRY::AddressToPte(EndPointer);

        // Set it as the new start
        Start[SystemPteSpace] = StartingPte;
        NonPagedSystemStart = StartingPte->PteToAddress();

        // Increase total number of system PTEs available and release them
        NumberOfSystemPtes += j;
        Release(StartingPte, j, SystemPteSpace);
    }
}

PTENTRY *
SYSTEM_PTES::
Reserve(IN ULONG Number, IN MMSYSTEM_PTE_POOL_TYPE PoolType, IN ULONG Alignment, IN ULONG Offset, IN BOOLEAN Crash)
{
    KIRQL OldIrql;

    AcquireSystemSpaceLock(&OldIrql);

    // Get pointer to the first free cluster
    PTENTRY *PointerPte = &FirstFree[PoolType];
    PTENTRY *FirstFree = PointerPte;

    // If it's empty - we have serious problem
    if (PointerPte->IsNextEntryEmpty())
    {
        // Crash if required, or return NULL
        if (Crash)
        {
            KeBugCheckEx (NO_MORE_SYSTEM_PTES,
                          (ULONG)PoolType,
                          Number,
                          TotalFree[PoolType],
                          NumberOfSystemPtes);
        }

        ReleaseSystemSpaceLock(OldIrql);
        return NULL;
    }

    // Get the next cluster
    PointerPte = Base + PointerPte->GetNextEntry();

    // Find the PTE from that cluster
    if (Alignment <= PAGE_SIZE)
        PointerPte = FindClusterUnaligned(PointerPte, FirstFree, Number);
    else
        PointerPte = FindClusterAligned(PointerPte, FirstFree, Number, Alignment, Offset);

    // Reduce amount of total free pages
    if (PointerPte)
        TotalFree[PoolType] -= Number;
    else if (Crash)
    {
        KeBugCheckEx(NO_MORE_SYSTEM_PTES,
                        (ULONG)PoolType,
                        Number,
                        TotalFree[PoolType],
                        NumberOfSystemPtes);
    }

    // The system space lock can be released now
    ReleaseSystemSpaceLock(OldIrql);

    if (PointerPte && (PoolType == SystemPteSpace))
    {
        // FIXME: Flush individual PTEs if possible
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        KeFlushEntireTb(TRUE, TRUE);
        KeLowerIrql(OldIrql);
    }

    DPRINT("StartingPte %p, Number %d, PoolType %d\n", PointerPte, Number, PoolType);

    return PointerPte;
}

PTENTRY *
SYSTEM_PTES::
FindClusterUnaligned(PTENTRY *PointerPte, PTENTRY *Previous, ULONG Number)
{
    while (TRUE)
    {
        // Get size of this cluster
        ULONG ClusterSize = GetClusterSize(PointerPte);

        // Depending on the required number of pages
        if (Number < ClusterSize)
        {
            // So make it smaller, and check for the one paged cluster!
            ULONG NewSize = ClusterSize - Number;
            if (NewSize == 1)
            {
                // This becomes one paged cluster
                PointerPte->u.List.OneEntry = 1;
            }
            else
            {
                // Beginning of the cluster remains free, update size
                PTENTRY *Next = PointerPte + 1;
                Next->SetNextEntry(ClusterSize - Number);
            }

            // We get required pages from the end of this cluster
            PointerPte += NewSize;
            break;
        }
        else if (Number == ClusterSize)
        {
            // We take the entire cluster. Unlink it.
            Previous->u.List.NextEntry = PointerPte->u.List.NextEntry;
            break;
        }

        // Make sure we didn't reach the end of the cluster list
        if (PointerPte->IsNextEntryEmpty())
            return NULL;

        // Move to the next one
        Previous = PointerPte;
        PointerPte = Base + PointerPte->u.List.NextEntry;
        ASSERT (PointerPte > Previous);
    }

    return PointerPte;
}

PTENTRY *
SYSTEM_PTES::
FindClusterAligned(PTENTRY *PointerPte, PTENTRY *Previous, ULONG Number, ULONG Alignment, ULONG Offset)
{
    // Calculate size mask and offset
    ULONG SizeMask = (Alignment - 1) >> (PAGE_SHIFT - PTE_SHIFT);
    ULONG AddOffset = (Offset >> (PAGE_SHIFT - PTE_SHIFT)) | (Alignment >> (PAGE_SHIFT - PTE_SHIFT));

    DPRINT1("FindClusterAligned %p %p %d 0x%x %d\n", PointerPte, Previous, Number, Alignment, Offset);

    while (TRUE)
    {
        // Get size of this cluster
        ULONG ClusterSize = GetClusterSize(PointerPte);

        // Calculate additional PTEs needed due to alignment requirements
        ULONG AdditionalPtes = (((AddOffset - ((ULONG_PTR)PointerPte & SizeMask)) & SizeMask) >> PTE_SHIFT);
        ULONG PtesNeeded = Number + AdditionalPtes;

        if (PtesNeeded < ClusterSize)
        {
            //  | Additional PTEs || User's cluster || NextCluster |

            // Calculate remaining size 
            ULONG RemainingSize = ClusterSize - PtesNeeded;

            // Create next cluster in the end of this one
            PTENTRY *NextCluster = PointerPte + PtesNeeded;
            NextCluster->u.List.NextEntry = PointerPte->u.List.NextEntry;

            if (AdditionalPtes == 0)
            {
                // PointerPte's alignment fits, so consume all needed PTEs
                Previous->SetNextEntry(Previous->GetNextEntry() + PtesNeeded);
            }
            else
            {
                // Update with information about new cluster at the end of the one returned to the user
                PointerPte->SetNextEntry(NextCluster - Base);

                // Set size of the current cluster
                if (AdditionalPtes == 1)
                {
                    PointerPte->u.List.OneEntry = 1;
                }
                else
                {
                    // Size is stored in the next PTE
                    (PointerPte + 1)->SetNextEntry(AdditionalPtes);
                }
            }

            // Set size of the next cluster
            if (RemainingSize == 1)
            {
                NextCluster->u.List.OneEntry = 1;
            }
            else
            {
                NextCluster->u.List.OneEntry = 0;
                NextCluster++;
                NextCluster->u.List.NextEntry = RemainingSize;
            }

            // Offset user's cluster to comply with the required alignment
            PointerPte = PointerPte + AdditionalPtes;
            break;
        }
        else if (PtesNeeded == ClusterSize)
        {
            // Check alignment
            if (AdditionalPtes == 0)
            {
                // Very good: unlink and return this block
                Previous->u.List.NextEntry = PointerPte->u.List.NextEntry;
            }
            else
            {
                //  | Additional PTEs || User's cluster |
                if (AdditionalPtes == 1)
                {
                    // Create one-paged cluster
                    PointerPte->u.List.OneEntry = 1;
                }
                else
                {
                    (PointerPte + 1)->SetNextEntry(AdditionalPtes);
                }
            }

            // Offset user's cluster to comply with the required alignment
            PointerPte = PointerPte + AdditionalPtes;
            break;
        }

        // Make sure we didn't reach the end of the cluster list
        if (PointerPte->IsNextEntryEmpty())
            return NULL;

        // Move to the next one
        Previous = PointerPte;
        PointerPte = Base + PointerPte->u.List.NextEntry;
        ASSERT(PointerPte > Previous);
    }

    return PointerPte;
}

VOID
SYSTEM_PTES::
Release2(PTENTRY *StartingPte, ULONG NumberToRelease, MMSYSTEM_PTE_POOL_TYPE PoolType)
{
    ULONG ClusterSize;
    PTENTRY *PreviousPte, *NextPte;
    ULONG PteIndex;

    // Increase availability
    TotalFree[PoolType] += NumberToRelease;

    // Step through the cluster list to find where to insert the PTEs
    PreviousPte = &FirstFree[PoolType];
    //NextPte = Base + PreviousPte->GetNextEntry();
    PteIndex = StartingPte - Base;

    // Find insertion point
    while (TRUE)
    {
        // Get the next cluster
        NextPte = Base + PreviousPte->GetNextEntry();

        //DPRINT("PreviousPte %p, NextPte %p, PteIndex %d, PreviousPte->GetNextEntry() %d\n", PreviousPte, NextPte, PteIndex, PreviousPte->GetNextEntry());

        // Get out of the loop if we found suitable previous PTE
        if (PteIndex < PreviousPte->GetNextEntry()) break;

        // On to the next cluster
        PreviousPte = NextPte;
    }

    DPRINT("Found insertion point NextPte = 0x%x, PreviousPte = 0x%x\n", NextPte, PreviousPte);
    DPRINT("StartingPte %p, NumberToRelease %d, PoolType %d\n", StartingPte, NumberToRelease, PoolType);

    // Make sure we didn't jump over
    ASSERT((StartingPte + NumberToRelease) <= NextPte);

    // Get the next cluster's size
    ClusterSize = GetClusterSize(NextPte);

    // Check if this cluster is adjacent to the PTEs being released
    if ((NextPte + ClusterSize) == StartingPte)
    {
        // Add the PTEs in the cluster to the PTEs being released
        NumberToRelease += ClusterSize;

        // Unlink this cluster and zero it
        (PreviousPte + 1)->SetNextEntry(NumberToRelease);
        PreviousPte->u.List.OneEntry = 0;

        // Set StartingPte to this cluster (so that it may me combined with the following one)
        StartingPte = PreviousPte;
    }
    else
    {
        // Current PTE creates a new cluster
        StartingPte->SetNextEntry(PreviousPte->GetNextEntry());

        // Link it
        PreviousPte->SetNextEntry(PteIndex);

        // And set its size
        SetClusterSize(StartingPte, NumberToRelease);
    }

    // Try to merge two clusters if possible
    if ((StartingPte + NumberToRelease) == NextPte)
    {
        // Yes, possible
        StartingPte->SetNextEntry(NextPte->GetNextEntry());
        StartingPte->u.List.OneEntry = 0;

        // Set size
        if (NextPte->u.List.OneEntry)
        {
            // Single PTE - just add 1 to the new size
            (StartingPte + 1)->SetNextEntry(NumberToRelease + 1);
        }
        else
        {
            // Get size from the next pte and add it up
            (StartingPte + 1)->SetNextEntry(NumberToRelease + (NextPte + 1)->GetNextEntry());
        }
    }
}


VOID
SYSTEM_PTES::
Release(PTENTRY *StartingPte, ULONG NumberToRelease, MMSYSTEM_PTE_POOL_TYPE PoolType)
{
    KIRQL OldIrql;

    // Check to make sure the PTE address is within bounds
    ASSERT(NumberToRelease != 0);
    ASSERT(StartingPte >= Start[PoolType]);
    ASSERT(StartingPte + NumberToRelease - 1 <= End[PoolType]);

    // Zero PTEs
    RtlZeroMemory(StartingPte, NumberToRelease * sizeof(MMPTE));

    // Acquire the System PTE lock
    AcquireSystemSpaceLock(&OldIrql);

    // Call the generic routine operating on PTE clusters
    Release2(StartingPte, NumberToRelease, PoolType);

    // Release the System PTE lock
    ReleaseSystemSpaceLock(OldIrql);
}

