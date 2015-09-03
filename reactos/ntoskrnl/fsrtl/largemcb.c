/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL v2 - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/largemcb.c
 * PURPOSE:         Large Mapped Control Block (MCB) support for File System Drivers
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 *                  Jan Kratochvil <project-captive@jankratochvil.net>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))

/* GLOBALS *******************************************************************/

PAGED_LOOKASIDE_LIST FsRtlFirstMappingLookasideList;
NPAGED_LOOKASIDE_LIST FsRtlFastMutexLookasideList;

/* We use only real 'mapping' runs; we do not store 'holes' to our GTree. */
typedef struct _LARGE_MCB_MAPPING_ENTRY // run
{
    LARGE_INTEGER RunStartVbn;
    LARGE_INTEGER RunEndVbn;   /* RunStartVbn+SectorCount; that means +1 after the last sector */
    LARGE_INTEGER StartingLbn; /* Lbn of 'RunStartVbn' */
} LARGE_MCB_MAPPING_ENTRY, *PLARGE_MCB_MAPPING_ENTRY;

typedef struct _LARGE_MCB_MAPPING // mcb_priv
{
    RTL_GENERIC_TABLE Table;
} LARGE_MCB_MAPPING, *PLARGE_MCB_MAPPING;

typedef struct _BASE_MCB_INTERNAL {
    ULONG MaximumPairCount;
    ULONG PairCount;
    USHORT PoolType;
    USHORT Flags;
    PLARGE_MCB_MAPPING Mapping;
} BASE_MCB_INTERNAL, *PBASE_MCB_INTERNAL;

static LARGE_MCB_MAPPING_ENTRY StaticRunBelow0 = {
    {{-1}}, /* ignored */
    {{0}},
    {{-1}}, /* ignored */
};

static PVOID NTAPI McbMappingAllocate(PRTL_GENERIC_TABLE Table, CLONG Bytes)
{
    PVOID Result;
    PBASE_MCB Mcb = (PBASE_MCB)Table->TableContext;
    Result = ExAllocatePoolWithTag(Mcb->PoolType, Bytes, 'LMCB');
    DPRINT("McbMappingAllocate(%lu) => %p\n", Bytes, Result);
    return Result;
}

static VOID NTAPI McbMappingFree(PRTL_GENERIC_TABLE Table, PVOID Buffer)
{
    DPRINT("McbMappingFree(%p)\n", Buffer);
    ExFreePoolWithTag(Buffer, 'LMCB');
}

static
RTL_GENERIC_COMPARE_RESULTS
NTAPI
McbMappingCompare(PRTL_GENERIC_TABLE Table,
                  PVOID PtrA,
                  PVOID PtrB)
{
    PLARGE_MCB_MAPPING_ENTRY A = PtrA, B = PtrB;
    RTL_GENERIC_COMPARE_RESULTS Res;

    ASSERT(A);
    ASSERT(B);

    if (A->RunStartVbn.QuadPart == B->RunStartVbn.QuadPart && A->RunEndVbn.QuadPart == B->RunEndVbn.QuadPart)
        Res = GenericEqual;
    else if (A->RunEndVbn.QuadPart <= B->RunStartVbn.QuadPart)
        Res = GenericLessThan;
    else if (A->RunEndVbn.QuadPart >= B->RunStartVbn.QuadPart)
        Res = GenericGreaterThan;
    else
    {
        ASSERT(FALSE);
        Res = GenericEqual;
    }

    return Res;
}

static RTL_GENERIC_COMPARE_RESULTS NTAPI McbMappingIntersectCompare(PRTL_GENERIC_TABLE Table, PVOID PtrA, PVOID PtrB)
{
    PLARGE_MCB_MAPPING_ENTRY A = PtrA, B = PtrB;
    RTL_GENERIC_COMPARE_RESULTS Res;

    if (A->RunStartVbn.QuadPart <= B->RunStartVbn.QuadPart && A->RunEndVbn.QuadPart > B->RunStartVbn.QuadPart)
        Res = GenericEqual;
    else if (A->RunStartVbn.QuadPart >= B->RunStartVbn.QuadPart && B->RunEndVbn.QuadPart > A->RunStartVbn.QuadPart)
        Res = GenericEqual;
    else if (A->RunStartVbn.QuadPart < B->RunStartVbn.QuadPart)
        Res = GenericLessThan;
    else if (A->RunStartVbn.QuadPart > B->RunStartVbn.QuadPart)
        Res = GenericGreaterThan;
    else
        Res = GenericEqual;

    return Res;
}


/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 * @Mcb: #PLARGE_MCB initialized by FsRtlInitializeLargeMcb().
 * %NULL value is forbidden.
 * @Vbn: Starting virtual block number of the wished range.
 * @Lbn: Starting logical block number of the wished range.
 * @SectorCount: Length of the wished range.
 * Value less or equal to %0 is forbidden; FIXME: Is the reject of %0 W32 compliant?
 *
 * Adds the specified range @Vbn ... @Vbn+@SectorCount-1 to @Mcb.
 * Any mappings previously in this range are deleted first.
 *
 * Returns: %TRUE if successful.
 */
BOOLEAN
NTAPI
FsRtlAddBaseMcbEntry(IN PBASE_MCB OpaqueMcb,
                     IN LONGLONG Vbn,
                     IN LONGLONG Lbn,
                     IN LONGLONG SectorCount)
{
    PBASE_MCB_INTERNAL Mcb = (PBASE_MCB_INTERNAL)OpaqueMcb;
    LARGE_MCB_MAPPING_ENTRY Node, NeedleRun;
    PLARGE_MCB_MAPPING_ENTRY LowerRun, HigherRun;
    BOOLEAN NewElement;

    if (Vbn < 0) return FALSE;
    if (SectorCount <= 0) return FALSE;

    /* clean any possible previous entries in our range */
    FsRtlRemoveBaseMcbEntry(OpaqueMcb, Vbn, SectorCount);

    // We need to map [Vbn, Vbn+SectorCount) to [Lbn, Lbn+SectorCount),
    // taking in account the fact that we need to merge these runs if
    // they are adjacent or overlap, but fail if new run fully fits into another run

    /* initially we think we will be inserted as a separate run */
    Node.RunStartVbn.QuadPart = Vbn;
    Node.RunEndVbn.QuadPart = Vbn + SectorCount;
    Node.StartingLbn.QuadPart = Lbn;

    /* optionally merge with lower run */
    NeedleRun.RunStartVbn.QuadPart = Node.RunStartVbn.QuadPart - 1;
    NeedleRun.RunEndVbn.QuadPart = NeedleRun.RunStartVbn.QuadPart + 1;
    NeedleRun.StartingLbn.QuadPart = ~0ULL;
    Mcb->Mapping->Table.CompareRoutine = McbMappingIntersectCompare;
    if ((LowerRun = RtlLookupElementGenericTable(&Mcb->Mapping->Table, &NeedleRun)))
    {
        ASSERT(LowerRun->RunEndVbn.QuadPart == Node.RunStartVbn.QuadPart);
        Node.RunStartVbn.QuadPart = LowerRun->RunStartVbn.QuadPart;
        Node.StartingLbn.QuadPart = LowerRun->StartingLbn.QuadPart;
        Mcb->Mapping->Table.CompareRoutine = McbMappingCompare;
        RtlDeleteElementGenericTable(&Mcb->Mapping->Table, LowerRun);
        DPRINT("Intersecting lower run found (%I64d,%I64d) Lbn: %I64d\n", LowerRun->RunStartVbn.QuadPart, LowerRun->RunEndVbn.QuadPart, LowerRun->StartingLbn.QuadPart);
    }

    /* optionally merge with higher run */
    NeedleRun.RunStartVbn.QuadPart = Node.RunEndVbn.QuadPart;
    NeedleRun.RunEndVbn.QuadPart = NeedleRun.RunStartVbn.QuadPart + 1;
    Mcb->Mapping->Table.CompareRoutine = McbMappingIntersectCompare;
    if ((HigherRun = RtlLookupElementGenericTable(&Mcb->Mapping->Table, &NeedleRun)))
    {
        ASSERT(HigherRun->RunStartVbn.QuadPart == Node.RunEndVbn.QuadPart);
        Node.RunEndVbn.QuadPart = HigherRun->RunEndVbn.QuadPart;
        Mcb->Mapping->Table.CompareRoutine = McbMappingCompare;
        RtlDeleteElementGenericTable(&Mcb->Mapping->Table, HigherRun);
        DPRINT("Intersecting higher run found (%I64d,%I64d) Lbn: %I64d\n", HigherRun->RunStartVbn.QuadPart, HigherRun->RunEndVbn.QuadPart, HigherRun->StartingLbn.QuadPart);
    }
    Mcb->Mapping->Table.CompareRoutine = McbMappingCompare;

    /* finally insert the resulting run */
    RtlInsertElementGenericTable(&Mcb->Mapping->Table, &Node, sizeof(Node), &NewElement);
    ASSERT(NewElement);
    Node.RunStartVbn.QuadPart = Vbn;
    Node.RunEndVbn.QuadPart = Vbn + SectorCount;
    Node.StartingLbn.QuadPart = Lbn;

    // NB: Two consecutive runs can only be merged, if actual LBNs also match!

    /* 1.
            Existing->RunStartVbn
            |
            |///////|
                |/////////////|
                |
                Node->RunStartVbn

        2.
            Existing->RunStartVbn
            |
            |///////|
        |//////|
        |
        Node->RunStartVbn

        3.
            Existing->RunStartVbn
            |
            |///////|
                |///|
                |
                Node->RunStartVbn

        4.
            Existing->RunStartVbn
            |
            |///////|
        |///////////////|
        |
        Node->RunStartVbn


    Situation with holes:
    1. Holes at both ends
    2. Hole at the right, new run merged with the previous run
    3. Hole at the right, new run is not merged with the previous run
    4. Hole at the left, new run merged with the next run
    5. Hole at the left, new run is not merged with the next run
    6. No holes, exact fit to merge with both previous and next runs
    7. No holes, merges only with the next run
    8. No holes, merges only with the previous run
    9. No holes, does not merge with next or prev runs


    Overwriting existing mapping is not possible and results in FALSE being returned
    */
    return TRUE;
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

    DPRINT("Mcb %p Vbn %I64d Lbn %I64d SectorCount %I64d\n", Mcb, Vbn, Lbn, SectorCount);

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlAddBaseMcbEntry(&(Mcb->BaseMcb),
                                  Vbn,
                                  Lbn,
                                  SectorCount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    DPRINT("Done %u\n", Result);

    return Result;
}

/*
 * @implemented
 * @Mcb: #PLARGE_MCB initialized by FsRtlInitializeLargeMcb().
 * %NULL value is forbidden.
 * @RunIndex: Requested range index to retrieve.
 * @Vbn: Returns the starting virtual block number of the wished range.
 * %NULL pointer is forbidden.
 * @Lbn: Returns the starting logical block number of the wished range (or -1 if it is a hole).
 * %NULL pointer is forbidden.
 * @SectorCount: Returns the length of the wished range.
 * %NULL pointer is forbidden.
 * Value less or equal to %0 is forbidden; FIXME: Is the reject of %0 W32 compliant?
 *
 * Retrieves the parameters of the specified run with index @RunIndex.
 * 
 * Mapping %0 always starts at virtual block %0, either as 'hole' or as 'real' mapping.
 * libcaptive does not store 'hole' information to its #GTree.
 * Last run is always a 'real' run. 'hole' runs appear as mapping to constant @Lbn value %-1.
 *
 * Returns: %TRUE if successful.
 */
BOOLEAN
NTAPI
FsRtlGetNextBaseMcbEntry(IN PBASE_MCB OpaqueMcb,
                         IN ULONG RunIndex,
                         OUT PLONGLONG Vbn,
                         OUT PLONGLONG Lbn,
                         OUT PLONGLONG SectorCount)
{
    PBASE_MCB_INTERNAL Mcb = (PBASE_MCB_INTERNAL)OpaqueMcb;
    ULONG RunIndexRemaining;
    PLARGE_MCB_MAPPING_ENTRY Run, RunFound = NULL, RunFoundLower = NULL, RunFoundHigher = NULL;
    BOOLEAN First = TRUE;

    RunIndexRemaining = RunIndex;

    /* Traverse the tree */
    for (Run = (PLARGE_MCB_MAPPING_ENTRY)RtlEnumerateGenericTable(&Mcb->Mapping->Table, TRUE);
        Run;
        Run = (PLARGE_MCB_MAPPING_ENTRY)RtlEnumerateGenericTable(&Mcb->Mapping->Table, FALSE))
    {
        if (First)
        {
            /* Take care when we must emulate missing 'hole' run at start of our run list. */
            if (Run->RunStartVbn.QuadPart > 0)
            {
                if (RunIndexRemaining == 0)
                {
                    RunFoundLower = &StaticRunBelow0;
                    RunFoundHigher = Run;

                    /* stop the traversal */
                    break;
                }
                /* If someone wants RunIndex #1 we are already on it. */
                RunIndexRemaining--;
            }
            First = FALSE;
        }

        if (RunIndexRemaining > 0)
        {
            /* FIXME: performance: non-linear direct seek to the requested RunIndex */
            RunIndexRemaining--;
            if (RunIndexRemaining == 0)
                RunFoundLower = Run;
            else
                RunIndexRemaining--;

            /* continue the traversal */
            continue;
        }

        if (RunFoundLower)
            RunFoundHigher = Run;
        else
            RunFound = Run;

        /* stop the traversal */
        break;
    }

    if (RunFound) DPRINT("RunFound(%lu %lu %lu)\n", RunFound->RunStartVbn.LowPart, RunFound->RunEndVbn.LowPart, RunFound->StartingLbn.LowPart);
    if (RunFoundLower) DPRINT("RunFoundLower(%lu %lu %lu)\n", RunFoundLower->RunStartVbn.LowPart, RunFoundLower->RunEndVbn.LowPart, RunFoundLower->StartingLbn.LowPart);
    if (RunFoundHigher) DPRINT("RunFoundHigher(%lu %lu %lu)\n", RunFoundHigher->RunStartVbn.LowPart, RunFoundHigher->RunEndVbn.LowPart, RunFoundHigher->StartingLbn.LowPart);

    if (RunFound)
    {
        ASSERT(RunFoundLower == NULL);
        ASSERT(RunFoundHigher == NULL);

        if (Vbn)
            *Vbn = RunFound->RunStartVbn.QuadPart;
        if (Lbn)
            *Lbn = RunFound->StartingLbn.QuadPart;
        if (SectorCount)
            *SectorCount = RunFound->RunEndVbn.QuadPart - RunFound->RunStartVbn.QuadPart;

        return TRUE;
    }

    if (RunFoundLower && RunFoundHigher)
    {
        //ASSERT(RunFoundHigher != NULL);

        if (Vbn)
            *Vbn = RunFoundLower->RunEndVbn.QuadPart;
        if (Lbn)
            *Lbn = -1;
        if (SectorCount)
            *SectorCount = RunFoundHigher->RunStartVbn.QuadPart - RunFoundLower->RunEndVbn.QuadPart;

        return TRUE;
    }

    ASSERT(RunFoundHigher == NULL);
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

    DPRINT("FsRtlGetNextLargeMcbEntry Mcb %p RunIndex %lu\n", Mcb, RunIndex);

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlGetNextBaseMcbEntry(&(Mcb->BaseMcb),
                                      RunIndex,
                                      Vbn,
                                      Lbn,
                                      SectorCount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    DPRINT("Done %u\n", Result);

    return Result;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlInitializeBaseMcb(IN PBASE_MCB OpaqueMcb,
                       IN POOL_TYPE PoolType)
{
    PBASE_MCB_INTERNAL Mcb = (PBASE_MCB_INTERNAL)OpaqueMcb;

    if (PoolType == PagedPool)
    {
        Mcb->Mapping = ExAllocateFromPagedLookasideList(&FsRtlFirstMappingLookasideList);
    }
    else
    {
        Mcb->Mapping = ExAllocatePoolWithTag(PoolType | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                             sizeof(LARGE_MCB_MAPPING),
                                             'FSBC');
    }

    Mcb->PoolType = PoolType;
    Mcb->PairCount = 0;
    Mcb->MaximumPairCount = MAXIMUM_PAIR_COUNT;
    RtlInitializeGenericTable(&Mcb->Mapping->Table,
                              McbMappingCompare,
                              McbMappingAllocate,
                              McbMappingFree,
                              Mcb);
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

    _SEH2_TRY
    {
        FsRtlInitializeBaseMcb(&(Mcb->BaseMcb), PoolType);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ExFreeToNPagedLookasideList(&FsRtlFastMutexLookasideList,
                                    Mcb->GuardedMutex);
        Mcb->GuardedMutex = NULL;
    }
    _SEH2_END;
}

/*
 * @implemented
 */
INIT_FUNCTION
VOID
NTAPI
FsRtlInitializeLargeMcbs(VOID)
{
    /* Initialize the list for the MCB */
    ExInitializePagedLookasideList(&FsRtlFirstMappingLookasideList,
                                   NULL,
                                   NULL,
                                   POOL_RAISE_IF_ALLOCATION_FAILURE,
                                   sizeof(LARGE_MCB_MAPPING),
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
FsRtlLookupBaseMcbEntry(IN PBASE_MCB OpaqueMcb,
                        IN LONGLONG Vbn,
                        OUT PLONGLONG Lbn OPTIONAL,
                        OUT PLONGLONG SectorCountFromLbn OPTIONAL,
                        OUT PLONGLONG StartingLbn OPTIONAL,
                        OUT PLONGLONG SectorCountFromStartingLbn OPTIONAL,
                        OUT PULONG Index OPTIONAL)
{
    PBASE_MCB_INTERNAL Mcb = (PBASE_MCB_INTERNAL)OpaqueMcb;


    ULONG RunIndex = 0;
    PLARGE_MCB_MAPPING_ENTRY Run, RunFound = NULL, RunFoundLower = NULL, RunFoundHigher = NULL;
    BOOLEAN First = TRUE;

    /* Traverse the tree */
    for (Run = (PLARGE_MCB_MAPPING_ENTRY)RtlEnumerateGenericTable(&Mcb->Mapping->Table, TRUE);
        Run;
        Run = (PLARGE_MCB_MAPPING_ENTRY)RtlEnumerateGenericTable(&Mcb->Mapping->Table, FALSE))
    {
        if (First)
        {
            /* Take care when we must emulate missing 'hole' run at start of our run list. */
            if (Run->RunStartVbn.QuadPart > 0)
            {
                RunIndex++;
                RunFoundLower = &StaticRunBelow0;
            }
            First = FALSE;
        }

        if (Run->RunStartVbn.QuadPart <= Vbn && Vbn < Run->RunEndVbn.QuadPart)
        {
            RunFound = Run;
            RunFoundLower = NULL;
            /* stop the traversal; hit */
            break;
        }

        if (Run->RunEndVbn.QuadPart <= Vbn)
        {
            RunFoundLower = Run;
            if (Run->StartingLbn.QuadPart > 0)
            {
              RunIndex += 2;
            }
            /* continue the traversal; not yet crossed by the run */
            continue;
        }

        if (Vbn < Run->RunStartVbn.QuadPart)
        {
            RunFoundHigher = Run;
            RunIndex++;
            /* stop the traversal; the run skipped us */
            break;
        }

        ASSERT(FALSE);
        /* stop the traversal */
        break;
    }

    if (RunFound)
    {
        ASSERT(RunFoundLower == NULL);
        ASSERT(RunFoundHigher == NULL);

        if (Lbn)
            *Lbn = RunFound->StartingLbn.QuadPart + (Vbn - RunFound->RunStartVbn.QuadPart);

        if (SectorCountFromLbn)	/* FIXME: 'after' means including current 'Lbn' or without it? */
            *SectorCountFromLbn = RunFound->RunEndVbn.QuadPart - Vbn;
        if (StartingLbn)
            *StartingLbn = RunFound->StartingLbn.QuadPart;
        if (SectorCountFromStartingLbn)
            *SectorCountFromStartingLbn = RunFound->RunEndVbn.QuadPart - RunFound->RunStartVbn.QuadPart;
        if (Index)
            *Index = RunIndex;

        return TRUE;
    }

    if (RunFoundHigher)
    {
        /* search for hole */
        ASSERT(RunFoundLower != NULL);

        if (Lbn)
            *Lbn = ~0ull;
        if (SectorCountFromLbn)	/* FIXME: 'after' means including current 'Lbn' or without it? */
            *SectorCountFromLbn = RunFoundHigher->RunStartVbn.QuadPart - Vbn;
        if (StartingLbn)
            *StartingLbn = ~0ull;
        if (SectorCountFromStartingLbn)
            *SectorCountFromStartingLbn = RunFoundHigher->RunStartVbn.QuadPart - RunFoundLower->RunEndVbn.QuadPart;
        if (Index)
            *Index = RunIndex - 2;

        return TRUE;
    }

    /* We may have some 'RunFoundLower'. */
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

    DPRINT("FsRtlLookupLargeMcbEntry Mcb %p Vbn %I64d\n", Mcb, Vbn);

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlLookupBaseMcbEntry(&(Mcb->BaseMcb),
                                     Vbn,
                                     Lbn,
                                     SectorCountFromLbn,
                                     StartingLbn,
                                     SectorCountFromStartingLbn,
                                     Index);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    DPRINT("Done %u\n", Result);

    return Result;
}

static BOOLEAN
NTAPI
FsRtlLookupLastLargeMcbEntryAndIndex_internal(IN PBASE_MCB_INTERNAL Mcb,
                                              OUT PLONGLONG Vbn,
                                              OUT PLONGLONG Lbn,
                                              OUT PULONG Index OPTIONAL)
{
    ULONG RunIndex = 0;
    PLARGE_MCB_MAPPING_ENTRY Run, RunFound = NULL;
    LONGLONG LastVbn = 0;

    for (Run = (PLARGE_MCB_MAPPING_ENTRY)RtlEnumerateGenericTable(&Mcb->Mapping->Table, TRUE);
        Run;
        Run = (PLARGE_MCB_MAPPING_ENTRY)RtlEnumerateGenericTable(&Mcb->Mapping->Table, FALSE))
    {
        /* Take care when we must emulate missing 'hole' runs. */
        if (Run->RunStartVbn.QuadPart > LastVbn)
        {
            RunIndex++;
        }
        LastVbn = Run->RunEndVbn.QuadPart;
        RunIndex++;
        RunFound = Run;
    }

    if (!RunFound)
    {
        return FALSE;
    }

    if (Vbn)
    {
        *Vbn = RunFound->RunEndVbn.QuadPart - 1;
    }
    if (Lbn)
    {
        if (1)
        {
            *Lbn = RunFound->StartingLbn.QuadPart + (RunFound->RunEndVbn.QuadPart - RunFound->RunStartVbn.QuadPart) - 1;
        }
        else
        {
            *Lbn = ~0ULL;
        }
    }
    if (Index)
    {
        *Index = RunIndex - 1;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlLookupLastBaseMcbEntryAndIndex(IN PBASE_MCB OpaqueMcb,
                                    IN OUT PLONGLONG LargeVbn,
                                    IN OUT PLONGLONG LargeLbn,
                                    IN OUT PULONG Index)
{
    PBASE_MCB_INTERNAL Mcb = (PBASE_MCB_INTERNAL)OpaqueMcb;

    return FsRtlLookupLastLargeMcbEntryAndIndex_internal(Mcb, LargeVbn, LargeLbn, Index);
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

    DPRINT("FsRtlLookupLastLargeMcbEntryAndIndex %p\n", OpaqueMcb);

    KeAcquireGuardedMutex(OpaqueMcb->GuardedMutex);
    Result = FsRtlLookupLastBaseMcbEntryAndIndex(&(OpaqueMcb->BaseMcb),
                                                 LargeVbn,
                                                 LargeLbn,
                                                 Index);
    KeReleaseGuardedMutex(OpaqueMcb->GuardedMutex);

    DPRINT("Done %u\n", Result);

    return Result;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlLookupLastBaseMcbEntry(IN PBASE_MCB OpaqueMcb,
                            OUT PLONGLONG Vbn,
                            OUT PLONGLONG Lbn)
{
    PBASE_MCB_INTERNAL Mcb = (PBASE_MCB_INTERNAL)OpaqueMcb;

    return FsRtlLookupLastLargeMcbEntryAndIndex_internal(Mcb, Vbn, Lbn, NULL); /* Index */
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

    DPRINT("FsRtlLookupLastLargeMcbEntry Mcb %p\n", Mcb);

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlLookupLastBaseMcbEntry(&(Mcb->BaseMcb),
                                         Vbn,
                                         Lbn);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    DPRINT("Done %u\n", Result);

    return Result;
}

/*
 * @implemented
 */
ULONG
NTAPI
FsRtlNumberOfRunsInBaseMcb(IN PBASE_MCB OpaqueMcb)
{
    PBASE_MCB_INTERNAL Mcb = (PBASE_MCB_INTERNAL)OpaqueMcb;
    LONGLONG LbnAtVbn0 = -1;
    ULONG Nodes = RtlNumberGenericTableElements(&Mcb->Mapping->Table);

    if (Nodes == 0) return 0;

    FsRtlLookupBaseMcbEntry(OpaqueMcb,
        0,                           /* Vbn */
        &LbnAtVbn0,                  /* Lbn */
        NULL, NULL, NULL, NULL);     /* 4 output arguments - not interested in them */


    /* Return the count */
    //return Mcb->PairCount;
	/* Return the number of 'real' and 'hole' runs.
	 * If we do not have sector 0 as 'real' emulate a 'hole' there.
	 */
	return Nodes * 2 - (LbnAtVbn0 != -1 ? 1 : 0);	/* include holes as runs */
}

/*
 * @implemented
 */
ULONG
NTAPI
FsRtlNumberOfRunsInLargeMcb(IN PLARGE_MCB Mcb)
{
    ULONG NumberOfRuns;

    DPRINT("FsRtlNumberOfRunsInLargeMcb Mcb %p\n", Mcb);

    /* Read the number of runs while holding the MCB lock */
    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    NumberOfRuns = FsRtlNumberOfRunsInBaseMcb(&(Mcb->BaseMcb));
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    DPRINT("Done %lu\n", NumberOfRuns);

    /* Return the count */
    return NumberOfRuns;
}

/*
 * @implemented
 * @Mcb: #PLARGE_MCB initialized by FsRtlInitializeLargeMcb().
 * %NULL value is forbidden.
 * @Vbn: Starting virtual block number to specify the range to delete.
 * @SectorCount: Length of the range to delete.
 * Value less or equal to %0 is forbidden; FIXME: Is the reject of %0 W32 compliant?
 *
 * Deletes any possible @Mcb mappings in the given range @Vbn ... @Vbn+@SectorCount-1.
 * This call has no problems if no mappings exist there yet.
 */
BOOLEAN
NTAPI
FsRtlRemoveBaseMcbEntry(IN PBASE_MCB OpaqueMcb,
                        IN LONGLONG Vbn,
                        IN LONGLONG SectorCount)
{
    PBASE_MCB_INTERNAL Mcb = (PBASE_MCB_INTERNAL)OpaqueMcb;
    LARGE_MCB_MAPPING_ENTRY NeedleRun;
    PLARGE_MCB_MAPPING_ENTRY HaystackRun;

    if (Vbn < 0 || SectorCount <= 0) return FALSE;
    if (Vbn + SectorCount <= Vbn) return FALSE;

    NeedleRun.RunStartVbn.QuadPart = Vbn;
    NeedleRun.RunEndVbn.QuadPart = Vbn + SectorCount;
    NeedleRun.StartingLbn.QuadPart = ~0ULL;

    /* adjust/destroy all intersecting ranges */
    Mcb->Mapping->Table.CompareRoutine = McbMappingIntersectCompare;
    while ((HaystackRun = RtlLookupElementGenericTable(&Mcb->Mapping->Table, &NeedleRun)))
    {
        if (HaystackRun->RunStartVbn.QuadPart < NeedleRun.RunStartVbn.QuadPart)
        {
            ASSERT(HaystackRun->RunEndVbn.QuadPart > NeedleRun.RunStartVbn.QuadPart);
            HaystackRun->RunEndVbn.QuadPart = NeedleRun.RunStartVbn.QuadPart;
        }
        else if (HaystackRun->RunEndVbn.QuadPart > NeedleRun.RunEndVbn.QuadPart)
        {
            ASSERT(HaystackRun->RunStartVbn.QuadPart < NeedleRun.RunEndVbn.QuadPart);
            HaystackRun->RunStartVbn.QuadPart = NeedleRun.RunEndVbn.QuadPart;
        }
        else
        {
            ASSERT(NeedleRun.RunStartVbn.QuadPart >= HaystackRun->RunStartVbn.QuadPart);
            //ASSERT(NeedleRun.RunEndVbn.QuadPart <= HaystackRun->RunEndVbn.QuadPart);
            Mcb->Mapping->Table.CompareRoutine = McbMappingCompare;
            RtlDeleteElementGenericTable(&Mcb->Mapping->Table, HaystackRun);
            Mcb->Mapping->Table.CompareRoutine = McbMappingIntersectCompare;
        }
    }
    Mcb->Mapping->Table.CompareRoutine = McbMappingCompare;

    return TRUE;
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
    DPRINT("FsRtlRemoveLargeMcbEntry Mcb %p, Vbn %I64d, SectorCount %I64d\n", Mcb, Vbn, SectorCount);

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    FsRtlRemoveBaseMcbEntry(&(Mcb->BaseMcb), Vbn, SectorCount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    DPRINT("Done\n");
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlResetBaseMcb(IN PBASE_MCB OpaqueMcb)
{
    PBASE_MCB_INTERNAL Mcb = (PBASE_MCB_INTERNAL)OpaqueMcb;
    PLARGE_MCB_MAPPING_ENTRY Element;

    while (RtlNumberGenericTableElements(&Mcb->Mapping->Table) &&
           (Element = (PLARGE_MCB_MAPPING_ENTRY)RtlGetElementGenericTable(&Mcb->Mapping->Table, 0)))
    {
        RtlDeleteElementGenericTable(&Mcb->Mapping->Table, Element);
    }

    Mcb->PairCount = 0;
    Mcb->MaximumPairCount = 0;
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
        KeAcquireGuardedMutex(Mcb->GuardedMutex);

    FsRtlResetBaseMcb(&Mcb->BaseMcb);

    if (!SelfSynchronized)
        KeReleaseGuardedMutex(Mcb->GuardedMutex);
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlSplitBaseMcb(IN PBASE_MCB OpaqueMcb,
                  IN LONGLONG Vbn,
                  IN LONGLONG Amount)
{
    PBASE_MCB_INTERNAL Mcb = (PBASE_MCB_INTERNAL)OpaqueMcb;
    PLARGE_MCB_MAPPING_ENTRY Run, InsertLowerRun = NULL, ExistingRun = NULL;
    BOOLEAN NewElement;

    /* Traverse the tree */
    for (Run = (PLARGE_MCB_MAPPING_ENTRY)RtlEnumerateGenericTable(&Mcb->Mapping->Table, TRUE);
        Run;
        Run = (PLARGE_MCB_MAPPING_ENTRY)RtlEnumerateGenericTable(&Mcb->Mapping->Table, FALSE))
    {
        /* unaffected run? */
        /* FIXME: performance: effective skip of all 'lower' runs without traversing them */
        if (Vbn >= Run->RunEndVbn.QuadPart) { DPRINT("Skipping it\n"); continue; }

        /* crossing run to be split?
        * 'lower_run' is created on the original place; just shortened.
        * current 'run' is shifted up later
        */
        if (Vbn < Run->RunEndVbn.QuadPart)
        {
            /* FIXME: shift 'run->Lbn_start' ? */
            Run->RunStartVbn.QuadPart = Vbn;

            InsertLowerRun = NULL;
        }

        /* Shift the current 'run'.
        * Ordering is not changed in Generic Tree so I hope I do not need to reinsert it.
        */
        Run->RunStartVbn.QuadPart += Amount;
        ASSERT(Run->RunEndVbn.QuadPart + Amount > Run->RunEndVbn.QuadPart); /* overflow? */
        Run->RunEndVbn.QuadPart += Amount;
        /* FIXME: shift 'run->Lbn_start' ? */

        /* continue the traversal */
    }

    if (InsertLowerRun)
        ExistingRun = RtlInsertElementGenericTable(&Mcb->Mapping->Table, InsertLowerRun, sizeof(*InsertLowerRun), &NewElement);

    ASSERT(ExistingRun == NULL);

    return TRUE;
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

    DPRINT("FsRtlSplitLargeMcb %p, Vbn %I64d, Amount %I64d\n", Mcb, Vbn, Amount);

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlSplitBaseMcb(&(Mcb->BaseMcb),
                               Vbn,
                               Amount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    DPRINT("Done %u\n", Result);

    return Result;
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlTruncateBaseMcb(IN PBASE_MCB OpaqueMcb,
                     IN LONGLONG Vbn)
{
    DPRINT("Mcb=%p, Vbn=%I64d\n", OpaqueMcb, Vbn);
    FsRtlRemoveBaseMcbEntry(OpaqueMcb, Vbn, MAXLONG - Vbn + 1);
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlTruncateLargeMcb(IN PLARGE_MCB Mcb,
                      IN LONGLONG Vbn)
{
    DPRINT("FsRtlTruncateLargeMcb %p Vbn %I64d\n", Mcb, Vbn);
    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    FsRtlTruncateBaseMcb(&(Mcb->BaseMcb), Vbn);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);
    DPRINT("Done\n");
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlUninitializeBaseMcb(IN PBASE_MCB Mcb)
{
    FsRtlResetBaseMcb(Mcb);

    if ((Mcb->PoolType == PagedPool)/* && (Mcb->MaximumPairCount == MAXIMUM_PAIR_COUNT)*/)
    {
        ExFreeToPagedLookasideList(&FsRtlFirstMappingLookasideList,
                                   Mcb->Mapping);
    }
    else
    {
        ExFreePoolWithTag(Mcb->Mapping, 'FSBC');
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
