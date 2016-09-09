/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL v2 - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/largemcb.c
 * PURPOSE:         Large Mapped Control Block (MCB) support for File System Drivers
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 *                  Jan Kratochvil <project-captive@jankratochvil.net>
 *                  Trevor Thompson
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

/*
static LARGE_MCB_MAPPING_ENTRY StaticRunBelow0 = {
    {{-1}}, // ignored
    {{0}},
    {{-1}}, // ignored
};
*/

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
    BOOLEAN Result = TRUE;
    BOOLEAN IntResult;
    PBASE_MCB_INTERNAL Mcb = (PBASE_MCB_INTERNAL)OpaqueMcb;
    LARGE_MCB_MAPPING_ENTRY Node, NeedleRun;
    PLARGE_MCB_MAPPING_ENTRY LowerRun, HigherRun;
    BOOLEAN NewElement;
    LONGLONG IntLbn;

    DPRINT("FsRtlAddBaseMcbEntry(%p, %I64d, %I64d, %I64d)\n", OpaqueMcb, Vbn, Lbn, SectorCount);

    if (Vbn < 0)
    {
        Result = FALSE;
        goto quit;
    }

    if (SectorCount <= 0)
    {
        Result = FALSE;
        goto quit;
    }

    IntResult = FsRtlLookupBaseMcbEntry(OpaqueMcb, Vbn, &IntLbn, NULL, NULL, NULL, NULL);
    if (IntResult)
    {
        if (IntLbn != -1 && IntLbn != Lbn)
        {
            Result = FALSE;
            goto quit;
        }
    }

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
    if ((LowerRun = RtlLookupElementGenericTable(&Mcb->Mapping->Table, &NeedleRun)) &&
        (LowerRun->StartingLbn.QuadPart + (LowerRun->RunEndVbn.QuadPart - LowerRun->RunStartVbn.QuadPart) == Node.StartingLbn.QuadPart))
    {
        ASSERT(LowerRun->RunEndVbn.QuadPart == Node.RunStartVbn.QuadPart);
        Node.RunStartVbn.QuadPart = LowerRun->RunStartVbn.QuadPart;
        Node.StartingLbn.QuadPart = LowerRun->StartingLbn.QuadPart;
        Mcb->Mapping->Table.CompareRoutine = McbMappingCompare;
        RtlDeleteElementGenericTable(&Mcb->Mapping->Table, LowerRun);
        --Mcb->PairCount;
        DPRINT("Intersecting lower run found (%I64d,%I64d) Lbn: %I64d\n", LowerRun->RunStartVbn.QuadPart, LowerRun->RunEndVbn.QuadPart, LowerRun->StartingLbn.QuadPart);
    }

    /* optionally merge with higher run */
    NeedleRun.RunStartVbn.QuadPart = Node.RunEndVbn.QuadPart;
    NeedleRun.RunEndVbn.QuadPart = NeedleRun.RunStartVbn.QuadPart + 1;
    Mcb->Mapping->Table.CompareRoutine = McbMappingIntersectCompare;
    if ((HigherRun = RtlLookupElementGenericTable(&Mcb->Mapping->Table, &NeedleRun)) &&
        (Node.StartingLbn.QuadPart <= HigherRun->StartingLbn.QuadPart))
    {
        ASSERT(HigherRun->RunStartVbn.QuadPart == Node.RunEndVbn.QuadPart);
        Node.RunEndVbn.QuadPart = HigherRun->RunEndVbn.QuadPart;
        Mcb->Mapping->Table.CompareRoutine = McbMappingCompare;
        RtlDeleteElementGenericTable(&Mcb->Mapping->Table, HigherRun);
        --Mcb->PairCount;
        DPRINT("Intersecting higher run found (%I64d,%I64d) Lbn: %I64d\n", HigherRun->RunStartVbn.QuadPart, HigherRun->RunEndVbn.QuadPart, HigherRun->StartingLbn.QuadPart);
    }
    Mcb->Mapping->Table.CompareRoutine = McbMappingCompare;

    /* finally insert the resulting run */
    RtlInsertElementGenericTable(&Mcb->Mapping->Table, &Node, sizeof(Node), &NewElement);
    ++Mcb->PairCount;
    ASSERT(NewElement);

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

quit:
    DPRINT("FsRtlAddBaseMcbEntry(%p, %I64d, %I64d, %I64d) = %d\n", Mcb, Vbn, Lbn, SectorCount, Result);
    return Result;
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

    DPRINT("FsRtlAddLargeMcbEntry(%p, %I64d, %I64d, %I64d)\n", Mcb, Vbn, Lbn, SectorCount);

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlAddBaseMcbEntry(&(Mcb->BaseMcb),
                                  Vbn,
                                  Lbn,
                                  SectorCount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    DPRINT("FsRtlAddLargeMcbEntry(%p, %I64d, %I64d, %I64d) = %d\n", Mcb, Vbn, Lbn, SectorCount, Result);

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
    BOOLEAN Result = FALSE;
    PBASE_MCB_INTERNAL Mcb = (PBASE_MCB_INTERNAL)OpaqueMcb;
    PLARGE_MCB_MAPPING_ENTRY Run = NULL;
    ULONG CurrentIndex = 0;
    ULONGLONG LastVbn = 0;
    ULONGLONG LastSectorCount = 0;

    // Traverse the tree 
    for (Run = (PLARGE_MCB_MAPPING_ENTRY)RtlEnumerateGenericTable(&Mcb->Mapping->Table, TRUE);
    Run;
        Run = (PLARGE_MCB_MAPPING_ENTRY)RtlEnumerateGenericTable(&Mcb->Mapping->Table, FALSE))
    {
        // is the current index a hole?
        if (Run->RunStartVbn.QuadPart > (LastVbn + LastSectorCount))
        {
            // Is this the index we're looking for?
            if (RunIndex == CurrentIndex)
            {
                *Vbn = LastVbn + LastSectorCount;
                *Lbn = -1;
                *SectorCount = Run->RunStartVbn.QuadPart - *Vbn;

                Result = TRUE;
                goto quit;
            }

            CurrentIndex++;
        }

        if (RunIndex == CurrentIndex)
        {
            *Vbn = Run->RunStartVbn.QuadPart;
            *Lbn = Run->StartingLbn.QuadPart;
            *SectorCount = Run->RunEndVbn.QuadPart - Run->RunStartVbn.QuadPart;

            Result = TRUE;
            goto quit;
        }

        CurrentIndex++;
        LastVbn = Run->RunStartVbn.QuadPart;
        LastSectorCount = Run->RunEndVbn.QuadPart - Run->RunStartVbn.QuadPart;
    }

    // these values are meaningless when returning false (but setting them can be helpful for debugging purposes)
    *Vbn = 0xdeadbeef;
    *Lbn = 0xdeadbeef;
    *SectorCount = 0xdeadbeef;

quit:
    DPRINT("FsRtlGetNextBaseMcbEntry(%p, %d, %p, %p, %p) = %d (%I64d, %I64d, %I64d)\n", Mcb, RunIndex, Vbn, Lbn, SectorCount, Result, *Vbn, *Lbn, *SectorCount);
    return Result;
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

    DPRINT("FsRtlGetNextLargeMcbEntry(%p, %d, %p, %p, %p)\n", Mcb, RunIndex, Vbn, Lbn, SectorCount);

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlGetNextBaseMcbEntry(&(Mcb->BaseMcb),
                                      RunIndex,
                                      Vbn,
                                      Lbn,
                                      SectorCount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    DPRINT("FsRtlGetNextLargeMcbEntry(%p, %d, %p, %p, %p) = %d (%I64d, %I64d, %I64d)\n", Mcb, RunIndex, Vbn, Lbn, SectorCount, Result, *Vbn, *Lbn, *SectorCount);

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
    DPRINT("FsRtlInitializeLargeMcb(%p, %d)\n", Mcb, PoolType);

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
 * @implemented
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
    BOOLEAN Result = FALSE;
    ULONG i;
    LONGLONG LastVbn = 0, LastLbn = 0, Count = 0;   // the last values we've found during traversal

    DPRINT("FsRtlLookupBaseMcbEntry(%p, %I64d, %p, %p, %p, %p, %p)\n", OpaqueMcb, Vbn, Lbn, SectorCountFromLbn, StartingLbn, SectorCountFromStartingLbn, Index);

    for (i = 0; FsRtlGetNextBaseMcbEntry(OpaqueMcb, i, &LastVbn, &LastLbn, &Count); i++)
    {
        // have we reached the target mapping?
        if (Vbn < LastVbn + Count)
        {
            if (Lbn)
            {
                if (LastLbn == -1)
                    *Lbn = -1;
                else
                    *Lbn = LastLbn + (Vbn - LastVbn);
            }

            if (SectorCountFromLbn)
                *SectorCountFromLbn = LastVbn + Count - Vbn;
            if (StartingLbn)
                *StartingLbn = LastLbn;
            if (SectorCountFromStartingLbn)
                *SectorCountFromStartingLbn = LastVbn + Count - LastVbn;
            if (Index)
                *Index = i;

            Result = TRUE;
            goto quit;
        }
    }

    if (Lbn)
        *Lbn = -1;
    if (StartingLbn)
        *StartingLbn = -1;

quit:
    DPRINT("FsRtlLookupBaseMcbEntry(%p, %I64d, %p, %p, %p, %p, %p) = %d (%I64d, %I64d, %I64d, %I64d, %d)\n",
           OpaqueMcb, Vbn, Lbn, SectorCountFromLbn, StartingLbn, SectorCountFromStartingLbn, Index, Result,
           (Lbn ? *Lbn : (ULONGLONG)-1), (SectorCountFromLbn ? *SectorCountFromLbn : (ULONGLONG)-1), (StartingLbn ? *StartingLbn : (ULONGLONG)-1),
           (SectorCountFromStartingLbn ? *SectorCountFromStartingLbn : (ULONGLONG)-1), (Index ? *Index : (ULONG)-1));

    return Result;
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

    DPRINT("FsRtlLookupLargeMcbEntry(%p, %I64d, %p, %p, %p, %p, %p)\n", Mcb, Vbn, Lbn, SectorCountFromLbn, StartingLbn, SectorCountFromStartingLbn, Index);

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlLookupBaseMcbEntry(&(Mcb->BaseMcb),
                                     Vbn,
                                     Lbn,
                                     SectorCountFromLbn,
                                     StartingLbn,
                                     SectorCountFromStartingLbn,
                                     Index);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    DPRINT("FsRtlLookupLargeMcbEntry(%p, %I64d, %p, %p, %p, %p, %p) = %d (%I64d, %I64d, %I64d, %I64d, %d)\n",
           Mcb, Vbn, Lbn, SectorCountFromLbn, StartingLbn, SectorCountFromStartingLbn, Index, Result,
           (Lbn ? *Lbn : (ULONGLONG)-1), (SectorCountFromLbn ? *SectorCountFromLbn : (ULONGLONG)-1), (StartingLbn ? *StartingLbn : (ULONGLONG)-1),
           (SectorCountFromStartingLbn ? *SectorCountFromStartingLbn : (ULONGLONG)-1), (Index ? *Index : (ULONG)-1));

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
    BOOLEAN Result;
    PBASE_MCB_INTERNAL Mcb = (PBASE_MCB_INTERNAL)OpaqueMcb;

    DPRINT("FsRtlLookupLastBaseMcbEntryAndIndex(%p, %p, %p, %p)\n", OpaqueMcb, LargeVbn, LargeLbn, Index);

    Result = FsRtlLookupLastLargeMcbEntryAndIndex_internal(Mcb, LargeVbn, LargeLbn, Index);

    DPRINT("FsRtlLookupLastBaseMcbEntryAndIndex(%p, %p, %p, %p) = %d (%I64d, %I64d, %d)\n", OpaqueMcb, LargeVbn, LargeLbn, Index, Result, *LargeVbn, *LargeLbn, *Index);

    return Result;
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

    DPRINT("FsRtlLookupLastLargeMcbEntryAndIndex(%p, %p, %p, %p)\n", OpaqueMcb, LargeVbn, LargeLbn, Index);

    KeAcquireGuardedMutex(OpaqueMcb->GuardedMutex);
    Result = FsRtlLookupLastBaseMcbEntryAndIndex(&(OpaqueMcb->BaseMcb),
                                                 LargeVbn,
                                                 LargeLbn,
                                                 Index);
    KeReleaseGuardedMutex(OpaqueMcb->GuardedMutex);

    DPRINT("FsRtlLookupLastLargeMcbEntryAndIndex(%p, %p, %p, %p) = %d (%I64d, %I64d, %d)\n", OpaqueMcb, LargeVbn, LargeLbn, Index, Result, *LargeVbn, *LargeLbn, *Index);

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
    BOOLEAN Result;
    PBASE_MCB_INTERNAL Mcb = (PBASE_MCB_INTERNAL)OpaqueMcb;

    DPRINT("FsRtlLookupLastBaseMcbEntry(%p, %p, %p)\n", OpaqueMcb, Vbn, Lbn);

    Result = FsRtlLookupLastLargeMcbEntryAndIndex_internal(Mcb, Vbn, Lbn, NULL); /* Index */

    DPRINT("FsRtlLookupLastBaseMcbEntry(%p, %p, %p) = %d (%I64d, %I64d)\n", Mcb, Vbn, Lbn, Result, *Vbn, *Lbn);

    return Result;
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

    DPRINT("FsRtlLookupLastLargeMcbEntry(%p, %p, %p)\n", Mcb, Vbn, Lbn);

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlLookupLastBaseMcbEntry(&(Mcb->BaseMcb),
                                         Vbn,
                                         Lbn);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    DPRINT("FsRtlLookupLastLargeMcbEntry(%p, %p, %p) = %d (%I64d, %I64d)\n", Mcb, Vbn, Lbn, Result, *Vbn, *Lbn);

    return Result;
}

/*
 * @implemented
 */
ULONG
NTAPI
FsRtlNumberOfRunsInBaseMcb(IN PBASE_MCB OpaqueMcb)
{
    ULONG NumberOfRuns = 0;
    LONGLONG Vbn, Lbn, Count;
    int i;

    DPRINT("FsRtlNumberOfRunsInBaseMcb(%p)\n", OpaqueMcb);

    // Count how many Mcb entries there are
    for (i = 0; FsRtlGetNextBaseMcbEntry(OpaqueMcb, i, &Vbn, &Lbn, &Count); i++)
    {
        NumberOfRuns++;
    }

    DPRINT("FsRtlNumberOfRunsInBaseMcb(%p) = %d\n", OpaqueMcb, NumberOfRuns);
    return NumberOfRuns;
}

/*
 * @implemented
 */
ULONG
NTAPI
FsRtlNumberOfRunsInLargeMcb(IN PLARGE_MCB Mcb)
{
    ULONG NumberOfRuns;

    DPRINT("FsRtlNumberOfRunsInLargeMcb(%p)\n", Mcb);

    /* Read the number of runs while holding the MCB lock */
    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    NumberOfRuns = FsRtlNumberOfRunsInBaseMcb(&(Mcb->BaseMcb));
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    DPRINT("FsRtlNumberOfRunsInLargeMcb(%p) = %d\n", Mcb, NumberOfRuns);

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
    BOOLEAN Result = TRUE;

    DPRINT("FsRtlRemoveBaseMcbEntry(%p, %I64d, %I64d)\n", OpaqueMcb, Vbn, SectorCount);

    if (Vbn < 0 || SectorCount <= 0)
    {
        Result = FALSE;
        goto quit;
    }

    if (Vbn + SectorCount <= Vbn)
    {
        Result = FALSE;
        goto quit;
    }

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
            //ASSERT(NeedleRun.RunStartVbn.QuadPart >= HaystackRun->RunStartVbn.QuadPart);
            //ASSERT(NeedleRun.RunEndVbn.QuadPart <= HaystackRun->RunEndVbn.QuadPart);
            Mcb->Mapping->Table.CompareRoutine = McbMappingCompare;
            RtlDeleteElementGenericTable(&Mcb->Mapping->Table, HaystackRun);
            --Mcb->PairCount;
            Mcb->Mapping->Table.CompareRoutine = McbMappingIntersectCompare;
        }
    }
    Mcb->Mapping->Table.CompareRoutine = McbMappingCompare;

quit:
    DPRINT("FsRtlRemoveBaseMcbEntry(%p, %I64d, %I64d) = %d\n", OpaqueMcb, Vbn, SectorCount, Result);
    return Result;
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
    DPRINT("FsRtlRemoveLargeMcbEntry(%p, %I64d, %I64d)\n", Mcb, Vbn, SectorCount);

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    FsRtlRemoveBaseMcbEntry(&(Mcb->BaseMcb), Vbn, SectorCount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);
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

    DPRINT("FsRtlResetBaseMcb(%p)\n", OpaqueMcb);

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
    DPRINT("FsRtlResetLargeMcb(%p, %d)\n", Mcb, SelfSynchronized);

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

    DPRINT("FsRtlSplitBaseMcb(%p, %I64d, %I64d)\n", OpaqueMcb, Vbn, Amount);

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
    {
        ExistingRun = RtlInsertElementGenericTable(&Mcb->Mapping->Table, InsertLowerRun, sizeof(*InsertLowerRun), &NewElement);
        ++Mcb->PairCount;
    }

    ASSERT(ExistingRun == NULL);

    DPRINT("FsRtlSplitBaseMcb(%p, %I64d, %I64d) = %d\n", OpaqueMcb, Vbn, Amount, TRUE);

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

    DPRINT("FsRtlSplitLargeMcb(%p, %I64d, %I64d)\n", Mcb, Vbn, Amount);

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlSplitBaseMcb(&(Mcb->BaseMcb),
                               Vbn,
                               Amount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    DPRINT("FsRtlSplitLargeMcb(%p, %I64d, %I64d) = %d\n", Mcb, Vbn, Amount, Result);

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
    DPRINT("FsRtlTruncateBaseMcb(%p, %I64d)\n", OpaqueMcb, Vbn);

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
    DPRINT("FsRtlTruncateLargeMcb(%p, %I64d)\n", Mcb, Vbn);

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    FsRtlTruncateBaseMcb(&(Mcb->BaseMcb), Vbn);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlUninitializeBaseMcb(IN PBASE_MCB Mcb)
{
    DPRINT("FsRtlUninitializeBaseMcb(%p)\n", Mcb);

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
    DPRINT("FsRtlUninitializeLargeMcb(%p)\n", Mcb);

    if (Mcb->GuardedMutex)
    {
        ExFreeToNPagedLookasideList(&FsRtlFastMutexLookasideList,
                                    Mcb->GuardedMutex);
        FsRtlUninitializeBaseMcb(&(Mcb->BaseMcb));
    }
}
