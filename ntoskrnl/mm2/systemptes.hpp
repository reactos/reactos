/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/systemptes.hpp
 * PURPOSE:         System PTEs header
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

#pragma once

extern "C" KSPIN_LOCK MmSystemSpaceLock;

typedef enum _MMSYSTEM_PTE_POOL_TYPE
{
    SystemPteSpace,
    NonPagedPoolExpansion,
    MaximumPtePoolTypes
} MMSYSTEM_PTE_POOL_TYPE;

class SYSTEM_PTES
{
public:
    VOID EstimateNumber(ULONG NumberOfPages);
    VOID Initialize(IN PTENTRY *StartingPte, IN PFN_COUNT NumberOfPtes, IN MMSYSTEM_PTE_POOL_TYPE PoolType);
    VOID Grow();
    PTENTRY *Reserve(IN ULONG NumberOfPtes, IN MMSYSTEM_PTE_POOL_TYPE PoolType);
    //PTENTRY *Reserve(IN ULONG NumberOfPtes, IN MMSYSTEM_PTE_POOL_TYPE PoolType, IN ULONG Alignment, IN ULONG Offset, IN BOOLEAN Crash);
    PTENTRY *Reserve(IN ULONG NumberOfPtes, IN MMSYSTEM_PTE_POOL_TYPE PoolType, IN ULONG Alignment, IN ULONG Offset, IN BOOLEAN Crash);
    VOID Release(PTENTRY *StartingPte, ULONG NumberOfPtes, MMSYSTEM_PTE_POOL_TYPE PoolType);
    VOID Release2(PTENTRY *StartingPte, ULONG NumberOfPtes, MMSYSTEM_PTE_POOL_TYPE PoolType);
    ULONG GetSystemPtesNumber() { return NumberOfSystemPtes; };
    VOID SetSystemPtesNumber(ULONG NewNumber) { NumberOfSystemPtes = NewNumber; };

    VOID AcquireSystemSpaceLock(KIRQL *OldIrql)
    {
        KeAcquireSpinLock(&MmSystemSpaceLock, OldIrql);
    }

    VOID ReleaseSystemSpaceLock(KIRQL OldIrql)
    {
        KeReleaseSpinLock(&MmSystemSpaceLock, OldIrql);
    }

    PVOID NonPagedSystemStart;

private:
    ULONG TotalFree[MaximumPtePoolTypes];
    PTENTRY FirstFree[MaximumPtePoolTypes];
    PTENTRY *Start[MaximumPtePoolTypes];
    PTENTRY *End[MaximumPtePoolTypes];
    ULONG NumberOfSystemPtes;
    PTENTRY *Base;

    // Flushing support (we don't use fancy PTE timestamps yet)
    PTENTRY *FlushPte;
    ULONG FlushEntry;

    PTENTRY *FindClusterUnaligned(PTENTRY *PointerPte, PTENTRY *Previous, ULONG Number);
    PTENTRY *FindClusterAligned(PTENTRY *PointerPte, PTENTRY *Previous, ULONG Number, ULONG Alignment, ULONG Offset);

    ULONG GetClusterSize(PTENTRY *Pte)
    {
        // First check for a single PTE
        if (Pte->u.List.OneEntry)
            return 1;

        // Then read the size from the trailing PTE
        Pte++;
        return Pte->GetNextEntry();
    };

    VOID SetClusterSize(PTENTRY *Pte, ULONG Size)
    {
        if (Size == 1)
        {
            // Single PTE
            Pte->u.List.OneEntry = 1;
        }
        else
        {
            // Size is stored in the next PTE
            Pte->u.List.OneEntry = 0;
            (Pte + 1)->SetNextEntry(Size);
        }
    };

};
