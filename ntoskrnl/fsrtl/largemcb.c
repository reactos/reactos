/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/largemcb.c
 * PURPOSE:         Mapping Control Block (MCB) support for File System Drivers
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlAddLargeMcbEntry(IN PLARGE_MCB Mcb,
                      IN LONGLONG Vbn,
                      IN LONGLONG Lbn,
                      IN LONGLONG SectorCount)
{
    KeBugCheck(FILE_SYSTEM);
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlAddMcbEntry(IN PMCB Mcb,
                 IN VBN Vbn,
                 IN LBN Lbn,
                 IN ULONG SectorCount)
{
    /* Call the newer function */
    return FsRtlAddLargeMcbEntry(&Mcb->
                                 DummyFieldThatSizesThisStructureCorrectly,
                                 (LONGLONG)Vbn,
                                 (LONGLONG)Lbn,
                                 (LONGLONG)SectorCount);
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlGetNextLargeMcbEntry(IN PLARGE_MCB Mcb,
                          IN ULONG RunIndex,
                          OUT PLONGLONG Vbn,
                          OUT PLONGLONG Lbn,
                          OUT PLONGLONG SectorCount)
{
    KeBugCheck(FILE_SYSTEM);
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
FsRtlGetNextMcbEntry(IN PMCB Mcb,
                     IN ULONG RunIndex,
                     OUT PVBN Vbn,
                     OUT PLBN Lbn,
                     OUT PULONG SectorCount)
{
    BOOLEAN Return = FALSE;
    LONGLONG llVbn;
    LONGLONG llLbn;
    LONGLONG llSectorCount;

    /* Call the Large version */
    Return = FsRtlGetNextLargeMcbEntry(
        &Mcb->DummyFieldThatSizesThisStructureCorrectly,
        RunIndex,
        &llVbn,
        &llLbn,
        &llSectorCount);

    /* Return the lower 32 bits */
    *Vbn = (ULONG)llVbn;
    *Lbn = (ULONG)llLbn;
    *SectorCount = (ULONG)llSectorCount;

    /* And return the original value */
    return Return;
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlInitializeLargeMcb(IN PLARGE_MCB Mcb,
                        IN POOL_TYPE PoolType)
{
    KeBugCheck(FILE_SYSTEM);
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlInitializeMcb(IN PMCB Mcb,
                   IN POOL_TYPE PoolType)
{
    /* Call the newer function */
    FsRtlInitializeLargeMcb(&Mcb->DummyFieldThatSizesThisStructureCorrectly,
                            PoolType);
}

/*
 * @unimplemented
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
    KeBugCheck(FILE_SYSTEM);
    *Lbn = 0;
    *SectorCountFromLbn = 0;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlLookupLastLargeMcbEntryAndIndex(IN PLARGE_MCB OpaqueMcb,
                                     OUT PLONGLONG LargeVbn,
                                     OUT PLONGLONG LargeLbn,
                                     OUT PULONG Index)
{
    KeBugCheck(FILE_SYSTEM);
    *LargeVbn = 0;
    *LargeLbn = 0;
    *Index = 0;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlLookupLastLargeMcbEntry(IN PLARGE_MCB Mcb,
                             OUT PLONGLONG Vbn,
                             OUT PLONGLONG Lbn)
{
    KeBugCheck(FILE_SYSTEM);
    return(FALSE);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlLookupLastMcbEntry(IN PMCB Mcb,
                        OUT PVBN Vbn,
                        OUT PLBN Lbn)
{
    BOOLEAN Return = FALSE;
    LONGLONG llVbn = 0;
    LONGLONG llLbn = 0;

    /* Call the Large version */
    Return = FsRtlLookupLastLargeMcbEntry(
        &Mcb->DummyFieldThatSizesThisStructureCorrectly,
        &llVbn,
        &llLbn);

    /* Return the lower 32-bits */
    *Vbn = (ULONG)llVbn;
    *Lbn = (ULONG)llLbn;

    /* And return the original value */
    return Return;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlLookupMcbEntry(IN PMCB Mcb,
                    IN VBN Vbn,
                    OUT PLBN Lbn,
                    OUT PULONG SectorCount OPTIONAL,
                    OUT PULONG Index)
{
    BOOLEAN Return = FALSE;
    LONGLONG llLbn;
    LONGLONG llSectorCount;

    /* Call the Large version */
    Return = FsRtlLookupLargeMcbEntry(&Mcb->
                                      DummyFieldThatSizesThisStructureCorrectly,
                                      (LONGLONG)Vbn,
                                      &llLbn,
                                      &llSectorCount,
                                      NULL,
                                      NULL,
                                      Index);

    /* Return the lower 32-bits */
    *Lbn = (ULONG)llLbn;
    if (SectorCount) *SectorCount = (ULONG)llSectorCount;

    /* And return the original value */
    return Return;
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
 * @implemented
 */
ULONG
NTAPI
FsRtlNumberOfRunsInMcb (IN PMCB Mcb)
{
    /* Call the newer function */
    return FsRtlNumberOfRunsInLargeMcb(
        &Mcb->DummyFieldThatSizesThisStructureCorrectly);
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlRemoveLargeMcbEntry(IN PLARGE_MCB Mcb,
                         IN LONGLONG Vbn,
                         IN LONGLONG SectorCount)
{
    KeBugCheck(FILE_SYSTEM);
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlRemoveMcbEntry(IN PMCB Mcb,
                    IN VBN Vbn,
                    IN ULONG SectorCount)
{
    /* Call the large function */
    FsRtlRemoveLargeMcbEntry(&Mcb->DummyFieldThatSizesThisStructureCorrectly,
                             (LONGLONG)Vbn,
                             (LONGLONG)SectorCount);
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlResetLargeMcb(IN PLARGE_MCB Mcb,
                   IN BOOLEAN SelfSynchronized)
{
    KeBugCheck(FILE_SYSTEM);
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlSplitLargeMcb(IN PLARGE_MCB Mcb,
                   IN LONGLONG Vbn,
                   IN LONGLONG Amount)
{
    KeBugCheck(FILE_SYSTEM);
    return FALSE;
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlTruncateLargeMcb(IN PLARGE_MCB Mcb,
                      IN LONGLONG Vbn)
{
    KeBugCheck(FILE_SYSTEM);
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlTruncateMcb (IN PMCB Mcb,
                  IN VBN  Vbn)
{
    /* Call the newer function */
    FsRtlTruncateLargeMcb(&Mcb->DummyFieldThatSizesThisStructureCorrectly,
                          (LONGLONG)Vbn);
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlUninitializeLargeMcb(IN PLARGE_MCB Mcb)
{
    KeBugCheck(FILE_SYSTEM);
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlUninitializeMcb(IN PMCB Mcb)
{
    /* Call the newer function */
    FsRtlUninitializeLargeMcb(&Mcb->DummyFieldThatSizesThisStructureCorrectly);
}

