/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/largemcb.c
 * PURPOSE:         Large Mapped Control Block (MCB) support for File System Drivers
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Pierre Schweitzer (heis_spiter@hotmail.com) 
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

PAGED_LOOKASIDE_LIST FsRtlFirstMappingLookasideList;
NPAGED_LOOKASIDE_LIST FsRtlFastMutexLookasideList;

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlAddBaseMcbEntry(IN PBASE_MCB Mcb,
                     IN LONGLONG Vbn,
                     IN LONGLONG Lbn,
                     IN LONGLONG SectorCount)
{
    KEBUGCHECK(0);
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlAddLargeMcbEntry(IN PLARGE_MCB Mcb,
                      IN LONGLONG Vbn,
                      IN LONGLONG Lbn,
                      IN LONGLONG SectorCount)
{
    BOOLEAN Result;

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlAddBaseMcbEntry(&(Mcb->BaseMcb),
                                  Vbn,
                                  Lbn,
                                  SectorCount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    return Result;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlGetNextBaseMcbEntry(IN PBASE_MCB Mcb,
                         IN ULONG RunIndex,
                         OUT PLONGLONG Vbn,
                         OUT PLONGLONG Lbn,
                         OUT PLONGLONG SectorCount)
{
    KEBUGCHECK(0);
    *Vbn = 0;
    *Lbn = 0;
    *SectorCount= 0;
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlGetNextLargeMcbEntry(IN PLARGE_MCB Mcb,
                          IN ULONG RunIndex,
                          OUT PLONGLONG Vbn,
                          OUT PLONGLONG Lbn,
                          OUT PLONGLONG SectorCount)
{
    BOOLEAN Result;

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlGetNextBaseMcbEntry(&(Mcb->BaseMcb),
                                      RunIndex,
                                      Vbn,
                                      Lbn,
                                      SectorCount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    return Result;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlInitializeBaseMcb(IN PBASE_MCB Mcb,
                       IN POOL_TYPE PoolType)
{
    Mcb->PairCount = 0;

    if (PoolType == PagedPool)
    {
        Mcb->Mapping = ExAllocateFromPagedLookasideList(&FsRtlFirstMappingLookasideList);
    }
    else
    {
        Mcb->Mapping = FsRtlAllocatePoolWithTag(PoolType, sizeof(INT_MAPPING) * MAXIMUM_PAIR_COUNT, TAG('F', 'S', 'B', 'C'));
    }

    Mcb->PoolType = PoolType;
    Mcb->MaximumPairCount = MAXIMUM_PAIR_COUNT;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlInitializeLargeMcb(IN PLARGE_MCB Mcb,
                        IN POOL_TYPE PoolType)
{
    Mcb->GuardedMutex = ExAllocateFromNPagedLookasideList(&FsRtlFastMutexLookasideList);

    KeInitializeGuardedMutex(Mcb->GuardedMutex);

    _SEH_TRY
    {
        FsRtlInitializeBaseMcb(&(Mcb->BaseMcb), PoolType);
    }
    _SEH_HANDLE
    {
        ExFreeToNPagedLookasideList(&FsRtlFastMutexLookasideList,
                                    Mcb->GuardedMutex);
        Mcb->GuardedMutex = NULL;
    }
    _SEH_END;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlInitializeLargeMcbs(VOID)
{
    /* Initialize the list for the MCB */
    ExInitializePagedLookasideList(&FsRtlFirstMappingLookasideList,
                                   NULL,
                                   NULL,
                                   POOL_RAISE_IF_ALLOCATION_FAILURE,
                                   sizeof(INT_MAPPING) * MAXIMUM_PAIR_COUNT,
                                   IFS_POOL_TAG,
                                   0); /* FIXME: Should be 4 */

    /* Initialize the list for the guarded mutex */
    ExInitializeNPagedLookasideList(&FsRtlFastMutexLookasideList,
                                    NULL,
                                    NULL,
                                    POOL_RAISE_IF_ALLOCATION_FAILURE,
                                    sizeof(KGUARDED_MUTEX),
                                    IFS_POOL_TAG,
                                    0); /* FIXME: Should be 32 */
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlLookupBaseMcbEntry(IN PBASE_MCB Mcb,
                        IN LONGLONG Vbn,
                        OUT PLONGLONG Lbn OPTIONAL,
                        OUT PLONGLONG SectorCountFromLbn OPTIONAL,
                        OUT PLONGLONG StartingLbn OPTIONAL,
                        OUT PLONGLONG SectorCountFromStartingLbn OPTIONAL,
                        OUT PULONG Index OPTIONAL)
{
    KEBUGCHECK(0);
    *Lbn = 0;
    *SectorCountFromLbn = 0;
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlLookupLargeMcbEntry(IN PLARGE_MCB Mcb,
                         IN LONGLONG Vbn,
                         OUT PLONGLONG Lbn OPTIONAL,
                         OUT PLONGLONG SectorCountFromLbn OPTIONAL,
                         OUT PLONGLONG StartingLbn OPTIONAL,
                         OUT PLONGLONG SectorCountFromStartingLbn OPTIONAL,
                         OUT PULONG Index OPTIONAL)
{
    BOOLEAN Result;

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlLookupBaseMcbEntry(&(Mcb->BaseMcb),
                                     Vbn,
                                     Lbn,
                                     SectorCountFromLbn,
                                     StartingLbn,
                                     SectorCountFromStartingLbn,
                                     Index);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    return Result;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlLookupLastBaseMcbEntryAndIndex(IN PBASE_MCB OpaqueMcb,
                                    IN OUT PLONGLONG LargeVbn,
                                    IN OUT PLONGLONG LargeLbn,
                                    IN OUT PULONG Index)
{
    KEBUGCHECK(0);
    *LargeVbn = 0;
    *LargeLbn = 0;
    *Index = 0;
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlLookupLastLargeMcbEntryAndIndex(IN PLARGE_MCB OpaqueMcb,
                                     OUT PLONGLONG LargeVbn,
                                     OUT PLONGLONG LargeLbn,
                                     OUT PULONG Index)
{
    BOOLEAN Result;

    KeAcquireGuardedMutex(OpaqueMcb->GuardedMutex);
    Result = FsRtlLookupLastBaseMcbEntryAndIndex(&(OpaqueMcb->BaseMcb),
                                                 LargeVbn,
                                                 LargeLbn,
                                                 Index);
    KeReleaseGuardedMutex(OpaqueMcb->GuardedMutex);

    return Result;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlLookupLastBaseMcbEntry(IN PBASE_MCB Mcb,
                            OUT PLONGLONG Vbn,
                            OUT PLONGLONG Lbn)
{
    KEBUGCHECK(0);
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlLookupLastLargeMcbEntry(IN PLARGE_MCB Mcb,
                             OUT PLONGLONG Vbn,
                             OUT PLONGLONG Lbn)
{
    BOOLEAN Result;

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlLookupLastBaseMcbEntry(&(Mcb->BaseMcb),
                                         Vbn,
                                         Lbn);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    return Result;
}

/*
 * @implemented
 */
ULONG
NTAPI
FsRtlNumberOfRunsInBaseMcb(IN PBASE_MCB Mcb)
{
    /* Return the count */
    return Mcb->PairCount;
}

/*
 * @implemented
 */
ULONG
NTAPI
FsRtlNumberOfRunsInLargeMcb(IN PLARGE_MCB Mcb)
{
    ULONG NumberOfRuns;

    /* Read the number of runs while holding the MCB lock */
    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    NumberOfRuns = Mcb->BaseMcb.PairCount;
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    /* Return the count */
    return NumberOfRuns;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlRemoveBaseMcbEntry(IN PBASE_MCB Mcb,
                        IN LONGLONG Vbn,
                        IN LONGLONG SectorCount)
{
    KEBUGCHECK(0);
    return FALSE;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlRemoveLargeMcbEntry(IN PLARGE_MCB Mcb,
                         IN LONGLONG Vbn,
                         IN LONGLONG SectorCount)
{
    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    FsRtlRemoveBaseMcbEntry(&(Mcb->BaseMcb),
                            Vbn,
                            SectorCount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlResetBaseMcb(IN PBASE_MCB Mcb)
{
    Mcb->PairCount = 0;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlResetLargeMcb(IN PLARGE_MCB Mcb,
                   IN BOOLEAN SelfSynchronized)
{
    if (!SelfSynchronized)
    {
        KeAcquireGuardedMutex(Mcb->GuardedMutex);
        Mcb->BaseMcb.PairCount = 0;
        KeReleaseGuardedMutex(Mcb->GuardedMutex);
    }
    else
    {
        Mcb->BaseMcb.PairCount = 0;
    }
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlSplitBaseMcb(IN PBASE_MCB Mcb,
                  IN LONGLONG Vbn,
                  IN LONGLONG Amount)
{
    KEBUGCHECK(0);
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlSplitLargeMcb(IN PLARGE_MCB Mcb,
                   IN LONGLONG Vbn,
                   IN LONGLONG Amount)
{
    BOOLEAN Result;

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlSplitBaseMcb(&(Mcb->BaseMcb),
                               Vbn,
                               Amount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    return Result;
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlTruncateBaseMcb(IN PBASE_MCB Mcb,
                     IN LONGLONG Vbn)
{
    KEBUGCHECK(0);
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlTruncateLargeMcb(IN PLARGE_MCB Mcb,
                      IN LONGLONG Vbn)
{
    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    FsRtlTruncateBaseMcb(&(Mcb->BaseMcb),
                         Vbn);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlUninitializeBaseMcb(IN PBASE_MCB Mcb)
{
    if ((Mcb->PoolType == PagedPool) && (Mcb->MaximumPairCount == MAXIMUM_PAIR_COUNT))
    {
        ExFreeToPagedLookasideList(&FsRtlFirstMappingLookasideList,
                                   Mcb->Mapping);
    }
    else
    {
        ExFreePoolWithTag(Mcb->Mapping, TAG('F', 'S', 'B', 'C'));
    }
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlUninitializeLargeMcb(IN PLARGE_MCB Mcb)
{
    if (Mcb->GuardedMutex)
    {
        ExFreeToNPagedLookasideList(&FsRtlFastMutexLookasideList,
                                    Mcb->GuardedMutex);
        FsRtlUninitializeBaseMcb(&(Mcb->BaseMcb));
    }
}

