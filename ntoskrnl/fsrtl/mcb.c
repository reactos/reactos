/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/mcb.c
 * PURPOSE:         Mapped Control Block (MCB) support for File System Drivers
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

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
FsRtlNumberOfRunsInMcb(IN PMCB Mcb)
{
    /* Call the newer function */
    return FsRtlNumberOfRunsInLargeMcb(
        &Mcb->DummyFieldThatSizesThisStructureCorrectly);
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
 * @implemented
 */
VOID
NTAPI
FsRtlTruncateMcb(IN PMCB Mcb,
                 IN VBN  Vbn)
{
    /* Call the newer function */
    FsRtlTruncateLargeMcb(&Mcb->DummyFieldThatSizesThisStructureCorrectly,
                          (LONGLONG)Vbn);
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
