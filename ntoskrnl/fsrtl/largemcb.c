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
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlAddLargeMcbEntry(IN PLARGE_MCB Mcb,
                      IN LONGLONG Vbn,
                      IN LONGLONG Lbn,
                      IN LONGLONG SectorCount)
{
    KEBUGCHECK(0);
    return FALSE;
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
    KEBUGCHECK(0);
    *Vbn = 0;
    *Lbn = 0;
    *SectorCount= 0;
    return FALSE;
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlInitializeBaseMcb(IN PBASE_MCB Mcb,
                       IN POOL_TYPE PoolType)
{
    KEBUGCHECK(0);
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlInitializeLargeMcb(IN PLARGE_MCB Mcb,
                        IN POOL_TYPE PoolType)
{
    KEBUGCHECK(0);
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
    KEBUGCHECK(0);
    *Lbn = 0;
    *SectorCountFromLbn = 0;
    return FALSE;
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
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlLookupLastLargeMcbEntryAndIndex(IN PLARGE_MCB OpaqueMcb,
                                     OUT PLONGLONG LargeVbn,
                                     OUT PLONGLONG LargeLbn,
                                     OUT PULONG Index)
{
    KEBUGCHECK(0);
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
FsRtlLookupLastBaseMcbEntry(IN PBASE_MCB Mcb,
                            OUT PLONGLONG Vbn,
                            OUT PLONGLONG Lbn)
{
    KEBUGCHECK(0);
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
    KEBUGCHECK(0);
    return FALSE;
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
 * @unimplemented
 */
VOID
NTAPI
FsRtlRemoveLargeMcbEntry(IN PLARGE_MCB Mcb,
                         IN LONGLONG Vbn,
                         IN LONGLONG SectorCount)
{
    KEBUGCHECK(0);
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlResetBaseMcb(IN PBASE_MCB Mcb)
{
    KEBUGCHECK(0);
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlResetLargeMcb(IN PLARGE_MCB Mcb,
                   IN BOOLEAN SelfSynchronized)
{
    KEBUGCHECK(0);
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
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlSplitLargeMcb(IN PLARGE_MCB Mcb,
                   IN LONGLONG Vbn,
                   IN LONGLONG Amount)
{
    KEBUGCHECK(0);
    return FALSE;
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
 * @unimplemented
 */
VOID
NTAPI
FsRtlTruncateLargeMcb(IN PLARGE_MCB Mcb,
                      IN LONGLONG Vbn)
{
    KEBUGCHECK(0);
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlUninitializeBaseMcb(IN PBASE_MCB Mcb)
{
    KEBUGCHECK(0);
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlUninitializeLargeMcb(IN PLARGE_MCB Mcb)
{
    KEBUGCHECK(0);
}

