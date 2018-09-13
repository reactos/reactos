/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   wsmanage.c

Abstract:

    This module contains routines which manage the set of active working
    set lists.

    Working set management is accomplished by a parallel group of actions

        1. Writing modified pages

        2. Trimming working sets in one of two ways:

            a. Using claims

                1. Aging pages by turning off access bits and incrementing age
                   counts for pages which haven't been accessed.
                2. Estimating the number of unused pages in a working set and
                   keeping a global count of that estimate.
                3. When getting tight on memory, replacing rather than adding
                   pages in a working set when a fault occurs in a working set
                   that has a significant proportion of unused pages.
                4. When memory is tight, reducing (trimming) working sets which
                   are above their maximum towards their minimum.  This is done
                   especially if there are a large number of available pages
                   in it.
        
            b. Using page fault information

                1. Reducing (trimming) working sets which are above their
                   maximum towards their minimum.

    The metrics are set such that writing modified pages is typically
    accomplished before trimming working sets, however, under certain cases
    where modified pages are being generated at a very high rate, working
    set trimming will be initiated to free up more pages to modify.

    When the first thread in a process is created, the memory management
    system is notified that working set expansion is allowed.  This
    is noted by changing the FLINK field of the WorkingSetExpansionLink
    entry in the process control block from MM_NO_WS_EXPANSION to
    MM_ALLOW_WS_EXPANSION.  As threads fault, the working set is eligible
    for expansion if ample pages exist (MmAvailablePages is high enough).

    Once a process has had its working set raised above the minimum
    specified, the process is put on the Working Set Expanded list and
    is now eligible for trimming.  Note that at this time the FLINK field
    in the WorkingSetExpansionLink has an address value.

    When working set trimming is initiated, a process is removed from the
    list (the expansion lock guards this list) and the FLINK field is set
    to MM_NO_WS_EXPANSION, also, the BLINK field is set to
    MM_WS_EXPANSION_IN_PROGRESS.  The BLINK field value indicates to
    the MmCleanUserAddressSpace function that working set trimming is
    in progress for this process and it should wait until it completes.
    This is accomplished by creating an event, putting the address of the
    event in the BLINK field and then releasing the expansion lock and
    waiting on the event atomically.  When working set trimming is
    complete, the BLINK field is no longer MM_EXPANSION_IN_PROGRESS
    indicating that the event should be set.

Author:

    Lou Perazzoli (loup) 10-Apr-1990
    Landy Wang (landyw) 02-Jun-1997

Revision History:

--*/

#include "mi.h"

VOID
MiEmptyAllWorkingSetsWorker (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK, MiEmptyAllWorkingSetsWorker)
#pragma alloc_text(PAGELK, MiEmptyAllWorkingSets)
#pragma alloc_text(INIT, MiAdjustWorkingSetManagerParameters)
#endif

//
// Minimum number of page faults to take to avoid being trimmed on
// an "ideal pass".
//

ULONG MiIdealPassFaultCountDisable;

extern ULONG PsMinimumWorkingSet;

extern PEPROCESS ExpDefaultErrorPortProcess;

#define MM_TRIM_COUNTER_MAXIMUM_LARGE_MEM (6)

//
// Hydra working set emptying support.
//

KEVENT  MiWaitForEmptyEvent;
BOOLEAN MiWaitingForWorkingSetEmpty;

//
// Number of times to wake up and do nothing before trimming processes
// with no faulting activity.
//

#define MM_REDUCE_FAULT_COUNT (10000)

#define MM_IGNORE_FAULT_COUNT (100)

#ifdef _MI_USE_CLAIMS_
BOOLEAN MiReplacing = FALSE;
#endif

PFN_NUMBER MmMoreThanEnoughFreePages = 1000;

ULONG MmAmpleFreePages = 200;

ULONG MmWorkingSetReductionMin = 12;
ULONG MmWorkingSetReductionMinCacheWs = 12;

ULONG MmWorkingSetReductionMax = 60;
ULONG MmWorkingSetReductionMaxCacheWs = 60;

ULONG MmWorkingSetReductionHuge = (512*1024) >> PAGE_SHIFT;

ULONG MmWorkingSetVolReductionMin = 12;

ULONG MmWorkingSetVolReductionMax = 60;
ULONG MmWorkingSetVolReductionMaxCacheWs = 60;

ULONG MmWorkingSetVolReductionHuge = (2*1024*1024) >> PAGE_SHIFT;

ULONG MmWorkingSetSwapReduction = 75;

ULONG MmWorkingSetSwapReductionHuge = (4*1024*1024) >> PAGE_SHIFT;

ULONG MmNumberOfForegroundProcesses;

#ifdef _MI_USE_CLAIMS_

ULONG MiAgingShift = 4;
ULONG MiEstimationShift = 5;
ULONG MmTotalClaim = 0;
ULONG MmTotalEstimatedAvailable = 0;

LARGE_INTEGER MiLastAdjustmentOfClaimParams;
LARGE_INTEGER MmClaimParameterAdjustUpTime = {60 * 1000 * 1000 * 10, 0};    // Sixty seconds
LARGE_INTEGER MmClaimParameterAdjustDownTime = {20 * 1000 * 1000 * 10, 0};    // 20 seconds

ULONG MmPlentyFreePages = 400;

#else

ULONG MiCheckCounter;
ULONG MmLastFaultCount;

#endif

#if DBG
PETHREAD MmWorkingSetThread;
#endif

extern PVOID MmPagableKernelStart;
extern PVOID MmPagableKernelEnd;

PERFINFO_WSMANAGE_GLOBAL_DECL;

typedef union _MMWS_TRIM_CRITERIA {
#ifdef _MI_USE_CLAIMS_
    struct {
        ULONG NumPasses;
        PFN_NUMBER DesiredFreeGoal;
        PFN_NUMBER NewTotalClaim;
        PFN_NUMBER NewTotalEstimatedAvailable;
        ULONG TrimAge;
        BOOLEAN DoAging;
        ULONG NumberOfForegroundProcesses;
    } ClaimBased;
#else
    struct {
        ULONG NumPasses;
        PFN_NUMBER DesiredFreeGoal;
        PFN_NUMBER DesiredReductionGoal;
        ULONG FaultCount;
        PFN_NUMBER TotalReduction;
        ULONG NumberOfForegroundProcesses;
    } FaultBased;
#endif
} MMWS_TRIM_CRITERIA, *PMMWS_TRIM_CRITERIA;

LOGICAL
MiCheckAndSetSystemTrimCriteria(
    IN OUT PMMWS_TRIM_CRITERIA Criteria
    );

BOOLEAN
MiCheckSystemTrimEndCriteria(
    IN OUT PMMWS_TRIM_CRITERIA Criteria,
    IN KIRQL OldIrql
    );

BOOLEAN
MiCheckProcessTrimCriteria(
    IN PMMWS_TRIM_CRITERIA Criteria,
    IN PMMSUPPORT VmSupport,
    IN PEPROCESS Process,
    IN PLARGE_INTEGER CurrentTime
    );

#ifndef _MI_USE_CLAIMS_
LOGICAL
MiCheckSystemCacheWsTrimCriteria(
    IN PMMSUPPORT VmSupport
    );
#endif

ULONG
MiDetermineWsTrimAmount(
    IN PMMWS_TRIM_CRITERIA Criteria,
    IN PMMSUPPORT VmSupport,
    IN PEPROCESS Process
    );

#ifdef _MI_USE_CLAIMS_
VOID
MiAgePagesAndEstimateClaims(
    VOID
    );

VOID
MiAdjustClaimParameters(
    IN BOOLEAN EnoughPages
    );

VOID
MiAgeAndEstimateAvailableInWorkingSet(
    IN PMMSUPPORT VmSupport,
    IN BOOLEAN DoAging,
    IN OUT PULONG TotalClaim,
    IN OUT PULONG TotalEstimatedAvailable
    );
#endif

VOID
MiRearrangeWorkingSetExpansionList(
    VOID
    );

VOID
MiAdjustWorkingSetManagerParameters(
    BOOLEAN WorkStation
    )
/*++

Routine Description:

    This function is called from MmInitSystem to adjust the working set manager
    trim algorithms based on system type and size.

Arguments:

    WorkStation - TRUE if this is a workstation, FALSE if not.

Return Value:

    None.

Environment:

    Kernel mode

--*/
{

#ifdef _MI_USE_CLAIMS_

    if (WorkStation && MmNumberOfPhysicalPages <= 63*1024*1024/PAGE_SIZE) {
        MiAgingShift = 4;
        MiEstimationShift = 5;
    }
    else {
        MiAgingShift = 5;
        MiEstimationShift = 6;
    }

    if (MmNumberOfPhysicalPages >= 63*1024*1024/PAGE_SIZE) {
        MmPlentyFreePages *= 2;
    }
#else

    if (WorkStation && (MmNumberOfPhysicalPages <= (31*1024*1024/PAGE_SIZE))) {

        //
        // To get fault protection, you have to take 45 faults instead of
        // the old 15 fault protection threshold.
        //

        MiIdealPassFaultCountDisable = 45;

        //
        // Take more away when you are over your working set in both
        // forced and voluntary mode, but leave cache WS trim amounts
        // alone.
        //

        MmWorkingSetVolReductionMax = 100;
        MmWorkingSetReductionMax = 100;

        //
        // In forced mode, even if you are within your working set, take
        // memory away more aggressively.
        //

        MmWorkingSetReductionMin = 40;
    }
    else {
        MiIdealPassFaultCountDisable = 15;
    }
#endif

    MiWaitingForWorkingSetEmpty = FALSE;
    KeInitializeEvent (&MiWaitForEmptyEvent, NotificationEvent, TRUE);
}


VOID
MiObtainFreePages (
    VOID
    )

/*++

Routine Description:

    This function examines the size of the modified list and the
    total number of pages in use because of working set increments
    and obtains pages by writing modified pages and/or reducing
    working sets.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set and PFN mutexes held.

--*/

{

    //
    // Check to see if there are enough modified pages to institute a
    // write.
    //

    if ((MmModifiedPageListHead.Total >= MmModifiedWriteClusterSize) ||
        (MmModNoWriteInsert)) {

        //
        // Start the modified page writer.
        //

        KeSetEvent (&MmModifiedPageWriterEvent, 0, FALSE);
    }

    //
    // See if there are enough working sets above the minimum
    // threshold to make working set trimming worthwhile.
    //

    if ((MmPagesAboveWsMinimum > MmPagesAboveWsThreshold) ||
        (MmAvailablePages < 5)) {

        //
        // Start the working set manager to reduce working sets.
        //

        KeSetEvent (&MmWorkingSetManagerEvent, 0, FALSE);
    }
}

VOID
MmWorkingSetManager (
    VOID
    )

/*++

Routine Description:

    Implements the NT working set manager thread.  When the number
    of free pages becomes critical and ample pages can be obtained by
    reducing working sets, the working set manager's event is set, and
    this thread becomes active.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{

    PEPROCESS CurrentProcess;
    PEPROCESS ProcessToTrim;
    PLIST_ENTRY ListEntry;
    LOGICAL Attached;
    ULONG Trim;
    KIRQL OldIrql;
    PMMSUPPORT VmSupport;
    PMMWSL WorkingSetList;
    LARGE_INTEGER CurrentTime;
    ULONG count;
    LOGICAL DoTrimming;
    PMM_SESSION_SPACE SessionSpace;
    LOGICAL InformSessionOfRelease;
#if DBG
    ULONG LastTrimFaultCount;
#endif // DBG
    MMWS_TRIM_CRITERIA TrimCriteria;
    PERFINFO_WSMANAGE_DECL();

#if DBG
    MmWorkingSetThread = PsGetCurrentThread ();
#endif

    ASSERT (MiHydra == FALSE || MmIsAddressValid (MmSessionSpace) == FALSE);

    CurrentProcess = PsGetCurrentProcess ();

    Trim = 0;

    //
    // Set the trim criteria: If there are plenty of pages, the existing
    // sets are aged and FALSE is returned to signify no trim is necessary.
    // Otherwise, the working set expansion list is ordered so the best
    // candidates for trimming are placed at the front and TRUE is returned.
    //

    DoTrimming = MiCheckAndSetSystemTrimCriteria(&TrimCriteria);

    if (DoTrimming) {
 
        Attached = 0;

        KeQuerySystemTime (&CurrentTime);

        ASSERT (MiHydra == FALSE || MmIsAddressValid (MmSessionSpace) == FALSE);

        LOCK_EXPANSION (OldIrql);
        while (!IsListEmpty (&MmWorkingSetExpansionHead.ListHead)) {

            //
            // Remove the entry at the head and trim it.
            //

            ListEntry = RemoveHeadList (&MmWorkingSetExpansionHead.ListHead);

            if (ListEntry == &MmSystemCacheWs.WorkingSetExpansionLinks) {
                VmSupport = &MmSystemCacheWs;
                ASSERT (VmSupport->u.Flags.SessionSpace == 0);
                ASSERT (VmSupport->u.Flags.TrimHard == 0);
                SessionSpace = NULL;
            }
            else {
                VmSupport = CONTAINING_RECORD(ListEntry,
                                              MMSUPPORT,
                                              WorkingSetExpansionLinks);

                if (VmSupport->u.Flags.SessionSpace == 0) {
                    ProcessToTrim = CONTAINING_RECORD(VmSupport,
                                                      EPROCESS,
                                                      Vm);

                    ASSERT (VmSupport == &ProcessToTrim->Vm);
                    ASSERT (ProcessToTrim->AddressSpaceDeleted == 0);
                    SessionSpace = NULL;
                }
                else {
                    ASSERT (MiHydra == TRUE);
                    SessionSpace = CONTAINING_RECORD(VmSupport,
                                                     MM_SESSION_SPACE,
                                                     Vm);
                }
            }

            //
            // Note that other routines that set this bit must remove the
            // entry from the expansion list first.
            //

            ASSERT (VmSupport->u.Flags.BeingTrimmed == 0);

            //
            // Check to see if we've been here before.
            //

            if ((*(PLARGE_INTEGER)&VmSupport->LastTrimTime).QuadPart ==
                       (*(PLARGE_INTEGER)&CurrentTime).QuadPart) {

                InsertHeadList (&MmWorkingSetExpansionHead.ListHead,
                            &VmSupport->WorkingSetExpansionLinks);

                //
                // If we aren't finished we may sleep in this call.
                //

                if (MiCheckSystemTrimEndCriteria(&TrimCriteria, OldIrql)) {

                    //
                    // No more pages are needed so we're done.
                    //

                    break;
                }

                //
                // Start a new round of trimming.
                //

                KeQuerySystemTime (&CurrentTime);

                continue;
            }

            PERFINFO_WSMANAGE_TRIMWS(ProcessToTrim, SessionSpace, VmSupport);

            if (SessionSpace) {

                if (MiCheckProcessTrimCriteria(&TrimCriteria,
                                                VmSupport,
                                                NULL,
                                                &CurrentTime) == FALSE) {

                    InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                                    &VmSupport->WorkingSetExpansionLinks);
                    continue;
                }

                VmSupport->LastTrimTime = CurrentTime;
                VmSupport->u.Flags.BeingTrimmed = 1;

                VmSupport->WorkingSetExpansionLinks.Flink = MM_NO_WS_EXPANSION;
                VmSupport->WorkingSetExpansionLinks.Blink =
                                                    MM_WS_EXPANSION_IN_PROGRESS;
                UNLOCK_EXPANSION (OldIrql);

                ProcessToTrim = NULL;

                //
                // Attach directly to the session space to be trimmed.
                //

                MiAttachSession (SessionSpace);

                //
                // Try for the session working set lock.
                //

                WorkingSetList = VmSupport->VmWorkingSetList;

                KeRaiseIrql (APC_LEVEL, &OldIrql);

                if (!ExTryToAcquireResourceExclusiveLite (&SessionSpace->WsLock)) {
                    //
                    // This session space's working set lock was not
                    // granted, don't trim it.
                    //

                    KeLowerIrql (OldIrql);

                    MiDetachSession ();

                    LOCK_EXPANSION (OldIrql);

                    ASSERT (VmSupport->u.Flags.BeingTrimmed == 1);

                    VmSupport->u.Flags.BeingTrimmed = 0;

                    VmSupport->AllowWorkingSetAdjustment = MM_FORCE_TRIM;

                    goto WorkingSetLockFailed;
                }

                VmSupport->LastTrimFaultCount = VmSupport->PageFaultCount;

                MM_SET_SESSION_RESOURCE_OWNER();
                PERFINFO_WSMANAGE_PROCESS_RESET(VmSupport);
            }
            else if (VmSupport != &MmSystemCacheWs) {

                //
                // Check to see if this is a forced trim or
                // if we are trimming because check counter is
                // at the maximum.
                //

                if (MiCheckProcessTrimCriteria(&TrimCriteria,
                                                VmSupport,
                                                ProcessToTrim,
                                                &CurrentTime) == FALSE) {

                    InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                                    &VmSupport->WorkingSetExpansionLinks);
                    continue;
                }

                VmSupport->LastTrimTime = CurrentTime;
                VmSupport->u.Flags.BeingTrimmed = 1;

                VmSupport->WorkingSetExpansionLinks.Flink = MM_NO_WS_EXPANSION;
                VmSupport->WorkingSetExpansionLinks.Blink =
                                                    MM_WS_EXPANSION_IN_PROGRESS;
                UNLOCK_EXPANSION (OldIrql);
                WorkingSetList = MmWorkingSetList;
                InformSessionOfRelease = FALSE;

                //
                // Attach to the process in preparation for trimming.
                //

                if (ProcessToTrim != CurrentProcess) {

                    Attached = KeForceAttachProcess (&ProcessToTrim->Pcb);

                    if (Attached == 0) {
                        LOCK_EXPANSION (OldIrql);
                        VmSupport->u.Flags.BeingTrimmed = 0;
                        VmSupport->AllowWorkingSetAdjustment = MM_FORCE_TRIM;
                        goto WorkingSetLockFailed;
                    }
                    if (ProcessToTrim->ProcessOutswapEnabled == TRUE) {
                        ASSERT (ProcessToTrim->ProcessOutswapped == FALSE);
                        if (MiHydra == TRUE && VmSupport->u.Flags.ProcessInSession == 1 && VmSupport->u.Flags.SessionLeader == 0) {
                            InformSessionOfRelease = TRUE;
                        }
                    }
                }

                //
                // Attempt to acquire the working set lock. If the
                // lock cannot be acquired, skip over this process.
                //

                count = 0;
                do {
                    if (ExTryToAcquireFastMutex(&ProcessToTrim->WorkingSetLock) != FALSE) {
                        break;
                    }
                    KeDelayExecutionThread (KernelMode, FALSE, &MmShortTime);
                    count += 1;
                    if (count == 5) {

                        //
                        // Could not get the lock, skip this process.
                        //

                        if (InformSessionOfRelease == TRUE) {
                            LOCK_EXPANSION (OldIrql);
                            ASSERT (ProcessToTrim->ProcessOutswapEnabled == TRUE);
                            ProcessToTrim->ProcessOutswapEnabled = FALSE;
                            ASSERT (MmSessionSpace->ProcessOutSwapCount >= 1);
                            MmSessionSpace->ProcessOutSwapCount -= 1;
                            UNLOCK_EXPANSION (OldIrql);
                            InformSessionOfRelease = FALSE;
                        }

                        if (Attached) {
                            KeDetachProcess ();
                            Attached = 0;
                        }

                        LOCK_EXPANSION (OldIrql);
                        VmSupport->u.Flags.BeingTrimmed = 0;
                        VmSupport->AllowWorkingSetAdjustment = MM_FORCE_TRIM;
                        goto WorkingSetLockFailed;
                    }
                } while (TRUE);

                ASSERT (VmSupport->u.Flags.BeingTrimmed == 1);

#if DBG
                LastTrimFaultCount = VmSupport->LastTrimFaultCount;
#endif // DBG
                VmSupport->LastTrimFaultCount = VmSupport->PageFaultCount;

                PERFINFO_WSMANAGE_PROCESS_RESET(VmSupport);
            }
            else {

                //
                // System cache,
                //

#if DBG
                LastTrimFaultCount = VmSupport->LastTrimFaultCount;
#endif // DBG

                PERFINFO_WSMANAGE_PROCESS_RESET(VmSupport);

                //
                // Always try to trim the system cache when using claims.
                // Fault-based trimming might skip it from time to time.
                //

#ifndef _MI_USE_CLAIMS_

                if (!MiCheckSystemCacheWsTrimCriteria(VmSupport)) {

                    //
                    // Don't trim the system cache.
                    //

                    InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                                        &VmSupport->WorkingSetExpansionLinks);
                    continue;
                }
#endif

                VmSupport->LastTrimTime = CurrentTime;

                //
                // Indicate that this working set is being trimmed.
                //

                VmSupport->u.Flags.BeingTrimmed = 1;

                UNLOCK_EXPANSION (OldIrql);

                ProcessToTrim = NULL;
                WorkingSetList = MmSystemCacheWorkingSetList;

                KeRaiseIrql (APC_LEVEL, &OldIrql);
                if (!ExTryToAcquireResourceExclusiveLite (&MmSystemWsLock)) {

                    //
                    // System working set lock was not granted, don't trim
                    // the system cache.
                    //

                    KeLowerIrql (OldIrql);
                    LOCK_EXPANSION (OldIrql);
                    VmSupport->u.Flags.BeingTrimmed = 0;
                    InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                                    &VmSupport->WorkingSetExpansionLinks);
                    continue;
                }

                MmSystemLockOwner = PsGetCurrentThread();

                VmSupport->LastTrimFaultCount = VmSupport->PageFaultCount;

                VmSupport->WorkingSetExpansionLinks.Flink = MM_NO_WS_EXPANSION;
                VmSupport->WorkingSetExpansionLinks.Blink =
                                                    MM_WS_EXPANSION_IN_PROGRESS;
            }

            //
            // Determine how many pages we want to trim from this working set.
            //

            Trim = MiDetermineWsTrimAmount(&TrimCriteria,
                                           VmSupport,
                                           ProcessToTrim
                                           );

#if DBG
            if (MmDebug & MM_DBG_WS_EXPANSION) {
                if (Trim) {
                  if (VmSupport->u.Flags.SessionSpace == 0) {
                      DbgPrint("           Trimming        Process %16s %5d Faults, WS %6d, Trimming %5d ==> %5d\n",
                        ProcessToTrim ? ProcessToTrim->ImageFileName : (PUCHAR)"System Cache",
                        VmSupport->PageFaultCount - LastTrimFaultCount,
                        VmSupport->WorkingSetSize,
                        Trim,
                        VmSupport->WorkingSetSize-Trim
                        );
                  }
                  else {
                      DbgPrint("           Trimming        Session 0x%x (id %d) %5d Faults, WS %6d, Trimming %5d ==> %5d\n",
                        SessionSpace,
                        SessionSpace->SessionId,
                        VmSupport->PageFaultCount - LastTrimFaultCount,
                        VmSupport->WorkingSetSize,
                        Trim,
                        VmSupport->WorkingSetSize-Trim
                        );
                  }
                }
            }
#endif //DBG

#ifdef _MI_USE_CLAIMS_

            //
            // If there's something to trim...
            //

            if (Trim != 0 &&
                (MmAvailablePages < TrimCriteria.ClaimBased.DesiredFreeGoal)) {

                //
                // We haven't reached our goal, so trim now.
                //

                PERFINFO_WSMANAGE_TOTRIM(Trim);

                Trim = MiTrimWorkingSet (Trim,
                                         VmSupport,
                                         TrimCriteria.ClaimBased.TrimAge
                                         );

                PERFINFO_WSMANAGE_ACTUALTRIM(Trim);
            }

            //
            // Estimating the current claim is always done here by taking a
            // sample of the working set.  Aging is only done if the trim
            // pass warrants it (ie: the first pass only).
            //

            MiAgeAndEstimateAvailableInWorkingSet(
                                VmSupport,
                                TrimCriteria.ClaimBased.DoAging,
                                &TrimCriteria.ClaimBased.NewTotalClaim,
                                &TrimCriteria.ClaimBased.NewTotalEstimatedAvailable
                                );
#else
            if (Trim != 0) {

                PERFINFO_WSMANAGE_TOTRIM(Trim);

                Trim = MiTrimWorkingSet (
                            Trim,
                            VmSupport,
                            (BOOLEAN)(MiCheckCounter < MM_TRIM_COUNTER_MAXIMUM_LARGE_MEM)
                            );

                PERFINFO_WSMANAGE_ACTUALTRIM(Trim);
            }
#endif

            //
            // Set the quota to the current size.
            //

            WorkingSetList->Quota = VmSupport->WorkingSetSize;
            if (WorkingSetList->Quota < VmSupport->MinimumWorkingSetSize) {
                WorkingSetList->Quota = VmSupport->MinimumWorkingSetSize;
            }

            if (SessionSpace) {

                ASSERT (VmSupport->u.Flags.SessionSpace == 1);

                UNLOCK_SESSION_SPACE_WS (OldIrql);

                MiDetachSession ();
            }
            else if (VmSupport != &MmSystemCacheWs) {

                ASSERT (VmSupport->u.Flags.SessionSpace == 0);
                UNLOCK_WS (ProcessToTrim);

                if (InformSessionOfRelease == TRUE) {
                    LOCK_EXPANSION (OldIrql);
                    ASSERT (ProcessToTrim->ProcessOutswapEnabled == TRUE);
                    ProcessToTrim->ProcessOutswapEnabled = FALSE;
                    ASSERT (MmSessionSpace->ProcessOutSwapCount >= 1);
                    MmSessionSpace->ProcessOutSwapCount -= 1;
                    UNLOCK_EXPANSION (OldIrql);
                    InformSessionOfRelease = FALSE;
                }

                if (Attached) {
                    KeDetachProcess ();
                    Attached = 0;
                }

            }
            else {
                ASSERT (VmSupport->u.Flags.SessionSpace == 0);
                UNLOCK_SYSTEM_WS (OldIrql);
            }

            LOCK_EXPANSION (OldIrql);

            ASSERT (VmSupport->u.Flags.BeingTrimmed == 1);
            VmSupport->u.Flags.BeingTrimmed = 0;

WorkingSetLockFailed:

            ASSERT (VmSupport->WorkingSetExpansionLinks.Flink == MM_NO_WS_EXPANSION);

            if (VmSupport->WorkingSetExpansionLinks.Blink ==
                                                 MM_WS_EXPANSION_IN_PROGRESS) {

                //
                // If the working set size is still above the minimum,
                // add this back at the tail of the list.
                //

                InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                                &VmSupport->WorkingSetExpansionLinks);
            }
            else {

                //
                // The value in the blink is the address of an event
                // to set.
                //

                ASSERT (VmSupport != &MmSystemCacheWs);

                KeSetEvent ((PKEVENT)VmSupport->WorkingSetExpansionLinks.Blink,
                            0,
                            FALSE);
            }

#ifndef _MI_USE_CLAIMS_
            TrimCriteria.FaultBased.TotalReduction += Trim;

            //
            // Zero this in case the next attach fails.
            //

            Trim = 0;

            if (MiCheckCounter < MM_TRIM_COUNTER_MAXIMUM_LARGE_MEM) {
                if ((MmAvailablePages > TrimCriteria.FaultBased.DesiredFreeGoal) ||
                    (TrimCriteria.FaultBased.TotalReduction > TrimCriteria.FaultBased.DesiredReductionGoal)) {


                    //
                    // Ample pages now exist.
                    //

                    PERFINFO_WSMANAGE_FINALACTION(WS_ACTION_AMPLE_PAGES_EXIST);
                    break;
                }
            }
#endif

        }

#ifdef _MI_USE_CLAIMS_
        MmTotalClaim = TrimCriteria.ClaimBased.NewTotalClaim;
        MmTotalEstimatedAvailable = TrimCriteria.ClaimBased.NewTotalEstimatedAvailable;
        PERFINFO_WSMANAGE_TRIMEND_CLAIMS(&TrimCriteria);
#else
        MiCheckCounter = 0;
        PERFINFO_WSMANAGE_TRIMEND_FAULTS(&TrimCriteria);
#endif

        UNLOCK_EXPANSION (OldIrql);
    }

    //
    // Signal the modified page writer as we have moved pages
    // to the modified list and memory was critical.
    //

    if ((MmAvailablePages < MmMinimumFreePages) ||
        (MmModifiedPageListHead.Total >= MmModifiedPageMaximum)) {
        KeSetEvent (&MmModifiedPageWriterEvent, 0, FALSE);
    }

    ASSERT (CurrentProcess == PsGetCurrentProcess ());

    return;
}

LOGICAL
MiCheckAndSetSystemTrimCriteria(
    PMMWS_TRIM_CRITERIA Criteria
    )

/*++

Routine Description:

    Decide whether to trim at this time.  If using claims, then this
    routine may initiate aging and claim adjustments as well.

Arguments:

    Criteria - Supplies a pointer to the trim criteria information.  Various
               fields in this structure are set as needed by this routine.

Return Value:

    TRUE if the caller should initiate trimming, FALSE if not.

Environment:

    Kernel mode.  No locks held.  APC level or below.

--*/

{
    PFN_NUMBER Available;
    ULONG PageFaultCount;
    KIRQL OldIrql;
    BOOLEAN InitiateTrim;

    PERFINFO_WSMANAGE_DECL();

    //
    // See if an empty-all-working-sets request has been queued to us.
    // This only happens on Hydra.
    //

    if (MiWaitingForWorkingSetEmpty == TRUE) {

        MiEmptyAllWorkingSetsWorker ();

        LOCK_EXPANSION (OldIrql);

        KeSetEvent (&MiWaitForEmptyEvent, 0, FALSE);
        MiWaitingForWorkingSetEmpty = FALSE;

        UNLOCK_EXPANSION (OldIrql);

#ifdef _MI_USE_CLAIMS_
        MiReplacing = FALSE;
#endif

        return FALSE;
    }

    //
    // Check the number of pages available to see if any trimming
    // is really required.
    //

    LOCK_PFN (OldIrql);
    Available = MmAvailablePages;
    PageFaultCount = MmInfoCounters.PageFaultCount;
    UNLOCK_PFN (OldIrql);

#ifdef _MI_USE_CLAIMS_
    PERFINFO_WSMANAGE_STARTLOG_CLAIMS();
    if (Available > MmPlentyFreePages && MiReplacing == FALSE) {

        //
        // Don't trim but do age unused pages and estimate
        // the amount available in working sets.
        //

        MiAgePagesAndEstimateClaims ();
        MiAdjustClaimParameters (TRUE);
        PERFINFO_WSMANAGE_TRIMACTION (WS_ACTION_RESET_COUNTER);
    }
    else {

        //
        // Inform our caller to start trimming since we're below
        // plenty pages - order the list so the bigger working sets are
        // in front so our caller trims those first.
        //

        Criteria->ClaimBased.NumPasses = 0;
        Criteria->ClaimBased.DesiredFreeGoal = MmPlentyFreePages +
                                                    (MmPlentyFreePages / 2);
        Criteria->ClaimBased.NewTotalClaim = 0;
        Criteria->ClaimBased.NewTotalEstimatedAvailable = 0;
        Criteria->ClaimBased.NumberOfForegroundProcesses = 0;

        //
        // Start trimming the bigger working sets first.
        //

        MiRearrangeWorkingSetExpansionList ();

#if DBG
        if (MmDebug & MM_DBG_WS_EXPANSION) {
            DbgPrint("\nMM-wsmanage: Desired = %ld, Avail %ld\n",
                    Criteria->ClaimBased.DesiredFreeGoal, MmAvailablePages);
        }
#endif //DBG

        PERFINFO_WSMANAGE_WILLTRIM_CLAIMS(Criteria);

        MiReplacing = FALSE;

        return TRUE;
    }
    PERFINFO_WSMANAGE_DUMPENTRIES_CLAIMS();

    //
    // If we've been replacing within a given working set, it's worth doing
    // a trim.  No need to lock synchronize the MiReplacing clearing as it
    // gets set every time a page replacement happens anyway.
    //

    InitiateTrim = MiReplacing;

    MiReplacing = FALSE;

    return InitiateTrim;

#else

    PERFINFO_WSMANAGE_STARTLOG_FAULTS();

    if ((Available > (MmNumberOfPhysicalPages >> 2)) ||
        ((Available > MmMoreThanEnoughFreePages) &&
        ((PageFaultCount - MmLastFaultCount) < MM_REDUCE_FAULT_COUNT))) {

        //
        // Don't trim and zero the check counter.
        //

        MiCheckCounter = 0;
        PERFINFO_WSMANAGE_TRIMACTION(WS_ACTION_RESET_COUNTER);

    } else if ((Available > MmAmpleFreePages) &&
        ((PageFaultCount - MmLastFaultCount) < MM_IGNORE_FAULT_COUNT)) {

        //
        // Don't do anything.
        //

        NOTHING;
        PERFINFO_WSMANAGE_TRIMACTION(WS_ACTION_NOTHING);

    } else if ((Available > MmFreeGoal) &&
               (MiCheckCounter < MM_TRIM_COUNTER_MAXIMUM_LARGE_MEM)) {

        //
        // Don't trim, but increment the check counter.
        //

        MiCheckCounter += 1;
        PERFINFO_WSMANAGE_TRIMACTION(WS_ACTION_INCREMENT_COUNTER);

    }
    else {

        //
        // Initialize state variables.
        //

        Criteria->FaultBased.NumPasses = 0;
        Criteria->FaultBased.TotalReduction = 0;
        Criteria->FaultBased.NumberOfForegroundProcesses = 0;

        //
        // Set the total reduction goals.
        //

        Criteria->FaultBased.DesiredReductionGoal = MmPagesAboveWsMinimum >> 2;
        if (MmPagesAboveWsMinimum > (MmFreeGoal << 1)) {
            Criteria->FaultBased.DesiredFreeGoal = MmFreeGoal;
        }
        else {
            Criteria->FaultBased.DesiredFreeGoal = MmMinimumFreePages + 10;
        }

        //
        // Calculate the number of faults to be taken to not be trimmed.
        //

        if (Available > MmMoreThanEnoughFreePages) {
            Criteria->FaultBased.FaultCount = 1;
        }
        else {
            Criteria->FaultBased.FaultCount = MiIdealPassFaultCountDisable;
        }

#if DBG
        if (MmDebug & MM_DBG_WS_EXPANSION) {
            DbgPrint("\nMM-wsmanage: checkcounter = %ld, Desired = %ld, Free = %ld Avail %ld\n",
                MiCheckCounter, Criteria->FaultBased.DesiredReductionGoal,
                Criteria->FaultBased.DesiredFreeGoal, MmAvailablePages);
        }
#endif //DBG

        PERFINFO_WSMANAGE_WILLTRIM_FAULTS(Criteria);

        if (MiHydra == TRUE) {
            MiRearrangeWorkingSetExpansionList ();
        }

        MmLastFaultCount = PageFaultCount;

        return TRUE;
    }
    PERFINFO_WSMANAGE_DUMPENTRIES_FAULTS();

    return FALSE;
#endif
}

BOOLEAN
MiCheckSystemTrimEndCriteria(
    IN PMMWS_TRIM_CRITERIA Criteria,
    IN KIRQL OldIrql
    )

/*++

Routine Description:

     Check the ending criteria.  If we're not done, delay for a little
     bit to let the modified write catch up.

Arguments:

    Criteria - Supplies the trim criteria information.

    OldIrql - Supplies the old IRQL to lower to if the expansion lock needs
              to be released.

Return Value:

    TRUE if trimming can be stopped, FALSE otherwise.

Environment:

    Kernel mode.  Expansion lock held.  APC level or below.

--*/

{
    BOOLEAN FinishedTrimming;
    PERFINFO_WSMANAGE_DECL();

    FinishedTrimming = FALSE;

#ifdef _MI_USE_CLAIMS_
    if ((MmAvailablePages > Criteria->ClaimBased.DesiredFreeGoal) ||
        (Criteria->ClaimBased.NumPasses >= MI_MAX_TRIM_PASSES)) {

        //
        // We have enough pages or we trimmed as many as we're going to get.
        //

        FinishedTrimming = TRUE;
    }
    else {

        //
        // Update the global claim and estimate before we wait.
        //

        MmTotalClaim = Criteria->ClaimBased.NewTotalClaim;
        MmTotalEstimatedAvailable = Criteria->ClaimBased.NewTotalEstimatedAvailable;
    }
#else
    if (MmAvailablePages > MmMinimumFreePages) {

        //
        // Every process has been examined and ample pages
        // now exist.
        //

        MmNumberOfForegroundProcesses = Criteria->FaultBased.NumberOfForegroundProcesses;

        FinishedTrimming = TRUE;
    }
#endif

    if (FinishedTrimming == FALSE) {

        //
        // If we don't have enough pages wait 10 milliseconds
        // for the modified page writer to catch up.  The wait is also
        // important because a thread may have the system cache locked but
        // has been preempted by the balance set manager due to its higher
        // priority.  We must give this thread a shot at running so it can
        // release the system cache lock (all the trimmable pages may reside in
        // the system cache).
        //

        UNLOCK_EXPANSION (OldIrql);

        KeDelayExecutionThread (KernelMode,
                                FALSE,
                                &MmShortTime);

#ifdef _MI_USE_CLAIMS_
        PERFINFO_WSMANAGE_WAITFORWRITER_CLAIMS();
#else
        PERFINFO_WSMANAGE_WAITFORWRITER_FAULTS();
#endif

        //
        // Check again to see if we've met the criteria to stop trimming.
        //

#ifdef _MI_USE_CLAIMS_
        if (MmAvailablePages > Criteria->ClaimBased.DesiredFreeGoal) {

            //
            // Now we have enough pages so break out.
            //

            FinishedTrimming = TRUE;
        }
        else {

            //
            // We don't have enough pages so let's do another pass.
            // Go get the next working set list which is probably the
            // one we put back before we gave up the processor.
            //
    
            if (Criteria->ClaimBased.NumPasses == 0) {
                MiAdjustClaimParameters(FALSE);
            }

            Criteria->ClaimBased.NumPasses += 1;
            Criteria->ClaimBased.NewTotalClaim = 0;
            Criteria->ClaimBased.NewTotalEstimatedAvailable = 0;
    
            PERFINFO_WSMANAGE_TRIMACTION(WS_ACTION_FORCE_TRIMMING_PROCESS);
        }
#else

        if (MmAvailablePages > MmMinimumFreePages) {

            //
            // Now we have enough pages so break out.
            //

            FinishedTrimming = TRUE;
        }
        else {

            //
            // Change this to a forced trim, so we get pages
            // available, and reset the current time.
            //

            PERFINFO_WSMANAGE_TRIMACTION(WS_ACTION_FORCE_TRIMMING_PROCESS);

            MiCheckCounter = 0;
            Criteria->FaultBased.NumPasses += 1;
        }
#endif

        LOCK_EXPANSION (OldIrql);
    }

    return FinishedTrimming;
}

BOOLEAN
MiCheckProcessTrimCriteria(
    PMMWS_TRIM_CRITERIA Criteria,
    PMMSUPPORT VmSupport,
    PEPROCESS Process,
    PLARGE_INTEGER CurrentTime
    )

/*++

Routine Description:

    Determine whether the specified working set should be trimmed.

Arguments:

    Criteria - Supplies the trim criteria information.

    VmSupport - Supplies the working set information for the candidate process.

    Process - Supplies the process to trim.  NULL if this is for a session.

    CurrentTime - Supplies the time at the start of this trim round.

Return Value:

    TRUE if trimming should be done on this process, FALSE otherwise.

Environment:

    Kernel mode.  Expansion lock held.  APC level or below.

--*/

{
    BOOLEAN Trim;
    BOOLEAN Reset;
    BOOLEAN Responsive;

#if DBG
    if (Process) {
        ASSERT (VmSupport->u.Flags.SessionSpace == 0);
    }
    else {
        ASSERT (VmSupport->u.Flags.SessionSpace == 1);
    }
#endif

    if (VmSupport->u.Flags.TrimHard == 1 && VmSupport->WorkingSetSize) {
        return TRUE;
    }

#ifdef _MI_USE_CLAIMS_

    //
    // Always trim if there's anything left.  Note that the
    // foreground process is given priority by putting it last in the
    // working set list and stopping trimming when we have enough pages.
    //

    if (VmSupport->WorkingSetSize <= 3) {
        return FALSE;
    }

    return TRUE;
#else

    Trim = TRUE;
    Reset = FALSE;

    if (Process && Process->Vm.MemoryPriority == MEMORY_PRIORITY_FOREGROUND && Criteria->FaultBased.NumPasses == 0) {

        Criteria->FaultBased.NumberOfForegroundProcesses += 1;
    }

    if (MiCheckCounter >= MM_TRIM_COUNTER_MAXIMUM_LARGE_MEM) {

        //
        // This is a voluntary trim so don't trim if 
        //  - the page fault count is too high
        //  - or the size is too small
        //  - or not enough time has elapsed since it was last trimmed
        //

        if (((VmSupport->PageFaultCount - VmSupport->LastTrimFaultCount) >
                                               Criteria->FaultBased.FaultCount)
                                ||
              (VmSupport->WorkingSetSize <= 5)

                                ||
              ((CurrentTime->QuadPart - VmSupport->LastTrimTime.QuadPart) <
                        MmWorkingSetProtectionTime.QuadPart)) {

#if DBG
            if (MmDebug & MM_DBG_WS_EXPANSION) {
                if (VmSupport->WorkingSetSize > 5) {

                    if (Process) {
                        DbgPrint("     ***** Skipping Process %16s %5d Faults, WS %6d\n",
                        Process->ImageFileName,
                        VmSupport->PageFaultCount - VmSupport->LastTrimFaultCount,
                        VmSupport->WorkingSetSize
                        );
                    }
                    else {
                        PMM_SESSION_SPACE SessionSpace;

                        ASSERT (MiHydra == TRUE);
                        SessionSpace = CONTAINING_RECORD(VmSupport,
                                                         MM_SESSION_SPACE,
                                                         Vm);

                        DbgPrint("     ***** Skipping Session %d %5d Faults, WS %6d\n",
                        SessionSpace->SessionId,
                        VmSupport->PageFaultCount - VmSupport->LastTrimFaultCount,
                        VmSupport->WorkingSetSize
                        );
                    }
                }
            }
#endif //DBG

            Reset = TRUE;
            Trim = FALSE;
        }
    } else {

        //
        // This is a forced trim.  If this process is above its
        // minimum it's a candidate.
        //

        if (VmSupport->WorkingSetSize <= VmSupport->MinimumWorkingSetSize) {

            //
            // If this process is below its minimum, don't trim it
            // unless stacks are swapped out or it's paging a bit.
            //

            if (((MmAvailablePages + 5) >= MmFreeGoal) &&
                 (((VmSupport->LastTrimFaultCount !=
                                        VmSupport->PageFaultCount) ||
                  (Process && Process->ProcessOutswapEnabled == FALSE)))) {

                //
                // If we've almost reached our goal and either this process
                // has taken page faults since the last trim or it's
                // not swap enabled, then don't trim it but do reset it.
                //

                Reset = TRUE;
                Trim = FALSE;
            } 
            else if ((VmSupport->WorkingSetSize < 5) ||
                    ((CurrentTime->QuadPart - VmSupport->LastTrimTime.QuadPart) <
                          MmWorkingSetProtectionTime.QuadPart)) {

                //
                // If the working set is very small or it was trimmed
                // recently don't trim it again.
                //
    
                Reset = TRUE;
                Trim = FALSE;
            }
        }
    }

    if (Trim == TRUE) {
    
        //
        // Fix to supply foreground responsiveness by not trimming
        // foreground priority applications as aggressively.
        //
    
        Responsive = FALSE;
    
        if ((MmNumberOfForegroundProcesses <= 3) &&
            (Criteria->FaultBased.NumberOfForegroundProcesses <= 3) &&
            (VmSupport->MemoryPriority)) {
    
            if ((MmAvailablePages > (MmMoreThanEnoughFreePages >> 2)) ||
               (VmSupport->MemoryPriority >= MEMORY_PRIORITY_FOREGROUND)) {

                //
                // Indicate that memory responsiveness to the foreground
                // process is important (not so for large console trees).
                //
    
                Responsive = TRUE;
            }
        }
    
        if (Responsive == TRUE && Criteria->FaultBased.NumPasses == 0) {
    
            //
            // Note that NumPasses yields a measurement of how
            // desperate we are for memory, if NumPasses is nonzero,
            // we are in trouble.
            //
    
            Trim = FALSE;
        }
    }

    if (Trim == FALSE) {
        if (Reset == TRUE) {
            VmSupport->LastTrimTime = *CurrentTime;
            VmSupport->LastTrimFaultCount = VmSupport->PageFaultCount;
        }

        PERFINFO_WSMANAGE_PROCESS_RESET(VmSupport);
    }

    return Trim;
#endif
}
#ifndef _MI_USE_CLAIMS_

LOGICAL
MiCheckSystemCacheWsTrimCriteria(
    PMMSUPPORT VmSupport
    )

/*++

Routine Description:

     Determine whether the system cache should be trimmed

Arguments:

    VmSupport - Supplies the working set information for the system cache.

Return Value:

    TRUE if trimming should be done on this process, FALSE otherwise.

Environment:

    Kernel mode.  Expansion lock held.  APC level or below.

--*/

{
    LOGICAL Trim;

    //
    // Don't trim the system cache if this is a voluntary trim and
    // the working set is within 100 pages of the minimum or
    // if the cache is at its minimum.
    //

    if ((MiCheckCounter >= MM_TRIM_COUNTER_MAXIMUM_LARGE_MEM) &&
        (((LONG)VmSupport->WorkingSetSize -
            (LONG)VmSupport->MinimumWorkingSetSize) < 100)) {

        //
        // Don't trim it if it's near its minimum and it's not a forced trim.
        //

        Trim = FALSE;
    }
    else {
        Trim = TRUE;
    }

    return Trim;
}
#endif


ULONG
MiDetermineWsTrimAmount(
    PMMWS_TRIM_CRITERIA Criteria,
    PMMSUPPORT VmSupport,
    PEPROCESS Process
    )

/*++

Routine Description:

     Determine whether this process should be trimmed.

Arguments:

    Criteria - Supplies the trim criteria information.

    VmSupport - Supplies the working set information for the candidate process.

    Process - Supplies the candidate process to be trimmed.

Return Value:

    TRUE if trimming should be done on this process, FALSE if not.

Environment:

    Kernel mode.  Expansion lock held.  APC level or below.

--*/

{
    PMMWSL WorkingSetList;
    ULONG MaxTrim;
    ULONG Trim;
    BOOLEAN OutswapEnabled;

    if (Process) {
        OutswapEnabled = Process->ProcessOutswapEnabled;
    }
    else {
        if (VmSupport->u.Flags.TrimHard == 1) {
            OutswapEnabled = TRUE;
        }
        else {
            OutswapEnabled = FALSE;
        }
    }

    WorkingSetList = VmSupport->VmWorkingSetList;

    if (VmSupport->WorkingSetSize <= WorkingSetList->FirstDynamic) {
        return 0;
    }

#ifdef _MI_USE_CLAIMS_
    MaxTrim = VmSupport->WorkingSetSize;

    if (OutswapEnabled == FALSE) {
        
        //
        // Don't trim the cache or non-swapped sessions or processes
        // below their minimum.
        //

        MaxTrim -= VmSupport->MinimumWorkingSetSize;
    }

    switch (Criteria->ClaimBased.NumPasses) {
    case 0:
        Trim = VmSupport->Claim >> 
                    ((VmSupport->MemoryPriority == MEMORY_PRIORITY_FOREGROUND)
                        ? MI_FOREGROUND_CLAIM_AVAILABLE_SHIFT
                        : MI_BACKGROUND_CLAIM_AVAILABLE_SHIFT);
        Criteria->ClaimBased.TrimAge = MI_PASS0_TRIM_AGE;
        Criteria->ClaimBased.DoAging = TRUE;
        break;
    case 1:
        Trim = VmSupport->Claim >> 
                    ((VmSupport->MemoryPriority == MEMORY_PRIORITY_FOREGROUND)
                        ? MI_FOREGROUND_CLAIM_AVAILABLE_SHIFT
                        : MI_BACKGROUND_CLAIM_AVAILABLE_SHIFT);
        Criteria->ClaimBased.TrimAge = MI_PASS1_TRIM_AGE;
        Criteria->ClaimBased.DoAging = FALSE;
        break;
    case 2:
        Trim = VmSupport->Claim;
        Criteria->ClaimBased.TrimAge = MI_PASS2_TRIM_AGE;
        Criteria->ClaimBased.DoAging = FALSE;
        break;
    case 3:
        Trim = VmSupport->EstimatedAvailable;
        Criteria->ClaimBased.TrimAge = MI_PASS3_TRIM_AGE;
        Criteria->ClaimBased.DoAging = FALSE;
        break;
    default:
        Trim = VmSupport->EstimatedAvailable;
        Criteria->ClaimBased.TrimAge = MI_PASS3_TRIM_AGE;
        Criteria->ClaimBased.DoAging = FALSE;

        if (MmAvailablePages < 100) {
            if (VmSupport->WorkingSetSize > VmSupport->MinimumWorkingSetSize) {
                Trim = (VmSupport->WorkingSetSize - VmSupport->MinimumWorkingSetSize) >> 2;
            }
            Criteria->ClaimBased.TrimAge = MI_PASS4_TRIM_AGE;
            Criteria->ClaimBased.DoAging = TRUE;
        }

        break;
    }

    if (Trim > MaxTrim) {
        Trim = MaxTrim;
    }

#else

    UNREFERENCED_PARAMETER (Criteria);

    //
    // Calculate Trim size.
    //

    if (VmSupport->WorkingSetSize <= VmSupport->MinimumWorkingSetSize &&
        OutswapEnabled == TRUE) {

        //
        // Set the quota to the minimum and reduce the working
        // set size.
        //

        WorkingSetList->Quota = VmSupport->MinimumWorkingSetSize;
        Trim = VmSupport->WorkingSetSize - WorkingSetList->FirstDynamic;
        if (Trim > MmWorkingSetSwapReduction) {
            Trim = MmWorkingSetSwapReduction;
        }

        ASSERT ((LONG)Trim >= 0);

    } else {

        MaxTrim = VmSupport->WorkingSetSize -
                                     VmSupport->MinimumWorkingSetSize;

        if (OutswapEnabled == TRUE) {

            //
            // All thread stacks have been swapped out this process or
            // all the processes and threads have been swapped out of this
            // session.
            //

            ULONG i;

            Trim = MmWorkingSetSwapReduction;
            i = VmSupport->WorkingSetSize - VmSupport->MaximumWorkingSetSize;
            if ((LONG)i > 0) {
                Trim = i;
                if (Trim > MmWorkingSetSwapReductionHuge) {
                    Trim = MmWorkingSetSwapReductionHuge;
                }
            }

        } else if (MiCheckCounter >= MM_TRIM_COUNTER_MAXIMUM_LARGE_MEM) {

            //
            // Haven't faulted much, reduce a bit.
            //

            if (VmSupport->WorkingSetSize >
                  (VmSupport->MaximumWorkingSetSize +
                            (6 * MmWorkingSetVolReductionHuge))) {
                Trim = MmWorkingSetVolReductionHuge;

            } else if ( (VmSupport != &MmSystemCacheWs) &&
                    VmSupport->WorkingSetSize >
                        ( VmSupport->MaximumWorkingSetSize + (2 * MmWorkingSetReductionHuge))) {
                Trim = MmWorkingSetReductionHuge;
            } else if (VmSupport->WorkingSetSize > VmSupport->MaximumWorkingSetSize) {
                if (VmSupport != &MmSystemCacheWs) {
                    Trim = MmWorkingSetVolReductionMax;
                } else {
                    Trim = MmWorkingSetVolReductionMaxCacheWs;
                }
            } else {
                Trim = MmWorkingSetVolReductionMin;
            }

        } else {

            if (VmSupport->WorkingSetSize >
                  (VmSupport->MaximumWorkingSetSize +
                            (2 * MmWorkingSetReductionHuge))) {
                Trim = MmWorkingSetReductionHuge;

            } else if (VmSupport->WorkingSetSize > VmSupport->MaximumWorkingSetSize) {
                if (VmSupport != &MmSystemCacheWs) {
                    Trim = MmWorkingSetReductionMax;
                } else {
                    Trim = MmWorkingSetReductionMaxCacheWs;
                }
            } else {
                if (VmSupport != &MmSystemCacheWs) {
                    Trim = MmWorkingSetReductionMin;
                } else {
                    Trim = MmWorkingSetReductionMinCacheWs;
                }
            }
        }

        if (MaxTrim < Trim) {
            Trim = MaxTrim;
        }
    }
#endif

    return Trim;
}
#ifdef _MI_USE_CLAIMS_

VOID
MiAgePagesAndEstimateClaims(
    VOID
    )

/*++

Routine Description:

    Walk through the processes on the working set expansion list
    aging pages and estimating the number of pages that they
    aren't using, the claim.
    
Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled.  PFN lock NOT held.

--*/

{
    NTSTATUS status;
    PMMSUPPORT VmSupport;
    PMMSUPPORT FirstSeen;
    BOOLEAN SystemCacheSeen;
    LOGICAL Attached;
    BOOLEAN Locked;
    KIRQL OldIrql;
    PLIST_ENTRY ListEntry;
    PEPROCESS Process;
    ULONG NewTotalClaim;
    ULONG NewTotalEstimatedAvailable;
    PEPROCESS CurrentProcess;
    PMM_SESSION_SPACE SessionSpace;
    LOGICAL InformSessionOfRelease;
    ULONG LoopCount;

    FirstSeen = NULL;
    SystemCacheSeen = FALSE;
    Attached = 0;
    Locked = FALSE;
    NewTotalClaim = 0;
    NewTotalEstimatedAvailable = 0;
    status = STATUS_SUCCESS;
    LoopCount = 0;

    CurrentProcess = PsGetCurrentProcess ();

    ASSERT (MiHydra == FALSE || MmIsAddressValid (MmSessionSpace) == FALSE);

    LOCK_EXPANSION (OldIrql);

    while (!IsListEmpty (&MmWorkingSetExpansionHead.ListHead)) {

        //
        // Remove the entry at the head, try to lock it, if we can lock it
        // then age some pages and estimate the number of available pages.
        //

        ListEntry = RemoveHeadList (&MmWorkingSetExpansionHead.ListHead);

        ASSERT (MiHydra == FALSE || MmIsAddressValid (MmSessionSpace) == FALSE);

        if (ListEntry == &MmSystemCacheWs.WorkingSetExpansionLinks) {
            VmSupport = &MmSystemCacheWs;
            Process = NULL;
            if (SystemCacheSeen != FALSE) {
            
                //
                // Seen this one already.
                //
            
                FirstSeen = VmSupport;
            }
            SystemCacheSeen = TRUE;
        }
        else {
            VmSupport = CONTAINING_RECORD(ListEntry,
                                          MMSUPPORT,
                                          WorkingSetExpansionLinks);

            if (VmSupport->u.Flags.SessionSpace == 0) {

                Process = CONTAINING_RECORD(ListEntry,
                                                  EPROCESS,
                                                  Vm.WorkingSetExpansionLinks);
                ASSERT (Process->AddressSpaceDeleted == 0);
                ASSERT (VmSupport->VmWorkingSetList == MmWorkingSetList);
            }
            else {
                ASSERT (MiHydra == TRUE);
                SessionSpace = CONTAINING_RECORD(VmSupport,
                                                 MM_SESSION_SPACE,
                                                 Vm);

                Process = NULL;
            }
        }

        ASSERT (VmSupport->u.Flags.BeingTrimmed == 0);

        if (VmSupport == FirstSeen) {
            InsertHeadList (&MmWorkingSetExpansionHead.ListHead,
                       &VmSupport->WorkingSetExpansionLinks);
            break;
        }

        VmSupport->u.Flags.BeingTrimmed = 1;

        if (FirstSeen == NULL) {
            FirstSeen = VmSupport;
        }

        VmSupport->WorkingSetExpansionLinks.Flink = MM_NO_WS_EXPANSION;
        VmSupport->WorkingSetExpansionLinks.Blink =
                                           MM_WS_EXPANSION_IN_PROGRESS;
        UNLOCK_EXPANSION (OldIrql);

        Locked = FALSE;
        if (VmSupport == &MmSystemCacheWs) {
            KeRaiseIrql (APC_LEVEL, &OldIrql);
            if (!ExTryToAcquireResourceExclusiveLite (&MmSystemWsLock)) {
                KeLowerIrql (OldIrql);
                goto FailureBranch;
            }
            MmSystemLockOwner = PsGetCurrentThread();
            Locked = TRUE;
        }
        else if (VmSupport->u.Flags.SessionSpace == 0) {
            InformSessionOfRelease = FALSE;
            if (CurrentProcess != Process) {
                Attached = KeForceAttachProcess(&Process->Pcb);
                if (Attached == 0) {
                    goto FailureBranch;
                }
                if (Process->ProcessOutswapEnabled == TRUE) {
                    ASSERT (Process->ProcessOutswapped == FALSE);
                    if (MiHydra == TRUE && VmSupport->u.Flags.ProcessInSession == 1 && VmSupport->u.Flags.SessionLeader == 0) {
                        InformSessionOfRelease = TRUE;
                    }
                }
            }
            if (ExTryToAcquireFastMutex(&Process->WorkingSetLock) == FALSE) {

                if (InformSessionOfRelease == TRUE) {
                    LOCK_EXPANSION (OldIrql);
                    ASSERT (Process->ProcessOutswapEnabled == TRUE);
                    Process->ProcessOutswapEnabled = FALSE;
                    ASSERT (MmSessionSpace->ProcessOutSwapCount >= 1);
                    MmSessionSpace->ProcessOutSwapCount -= 1;
                    UNLOCK_EXPANSION (OldIrql);
                    InformSessionOfRelease = FALSE;
                }

                if (Attached) {
                    KeDetachProcess ();
                    Attached = 0;
                }
                goto FailureBranch;
            }

            Locked = TRUE;
        }
        else {

            ASSERT (MiHydra == TRUE);

            //
            // Attach directly to the session space to be trimmed.
            //

            MiAttachSession (SessionSpace);

            KeRaiseIrql (APC_LEVEL, &OldIrql);
            if (!ExTryToAcquireResourceExclusiveLite (&SessionSpace->WsLock)) {
    
                //
                // This session space's working set lock was not
                // granted, don't trim it.
                //
    
                KeLowerIrql (OldIrql);
    
                MiDetachSession ();

                goto FailureBranch;
            }
    
            Locked = TRUE;

            MM_SET_SESSION_RESOURCE_OWNER();
        }

FailureBranch:

        if (Locked) {
            MiAgeAndEstimateAvailableInWorkingSet (VmSupport,
                                                   TRUE,
                                                   &NewTotalClaim,
                                                   &NewTotalEstimatedAvailable
                                                   );
    
            if (VmSupport == &MmSystemCacheWs) {
               ASSERT (VmSupport->u.Flags.SessionSpace == 0);
               UNLOCK_SYSTEM_WS (OldIrql);
            }
            else if (VmSupport->u.Flags.SessionSpace == 0) {

                UNLOCK_WS (Process);

                if (InformSessionOfRelease == TRUE) {
                    LOCK_EXPANSION (OldIrql);
                    ASSERT (Process->ProcessOutswapEnabled == TRUE);
                    Process->ProcessOutswapEnabled = FALSE;
                    ASSERT (MmSessionSpace->ProcessOutSwapCount >= 1);
                    MmSessionSpace->ProcessOutSwapCount -= 1;
                    UNLOCK_EXPANSION (OldIrql);
                    InformSessionOfRelease = FALSE;
                }

                if (Attached) {
                    KeDetachProcess ();
                    Attached = 0;
                }
            }
            else {
                ASSERT (MiHydra == TRUE);
                UNLOCK_SESSION_SPACE_WS (OldIrql);

                MiDetachSession ();
            }
        }

        LOCK_EXPANSION (OldIrql);

        ASSERT (VmSupport->u.Flags.BeingTrimmed == 1);
        VmSupport->u.Flags.BeingTrimmed = 0;

        ASSERT (VmSupport->WorkingSetExpansionLinks.Flink == MM_NO_WS_EXPANSION);
        if (VmSupport->WorkingSetExpansionLinks.Blink ==
                                             MM_WS_EXPANSION_IN_PROGRESS) {

            //
            // If the working set size is still above the minimum,
            // add this back at the tail of the list.
            //

            InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                            &VmSupport->WorkingSetExpansionLinks);
        }
        else {

            //
            // The value in the blink is the address of an event
            // to set.
            //

            ASSERT (VmSupport != &MmSystemCacheWs);

            KeSetEvent ((PKEVENT)VmSupport->WorkingSetExpansionLinks.Blink,
                        0,
                        FALSE);
        }

        //
        // The initial working set that was chosen for FirstSeen may have
        // been trimmed down under its minimum and been removed from the
        // ExpansionHead links.  It is possible that the system cache is not
        // on the links either.  This check detects this extremely rare
        // situation so that the system does not spin forever.
        //

        LoopCount += 1;
        if (LoopCount > 200) {
            if (MmSystemCacheWs.WorkingSetExpansionLinks.Blink == MM_WS_EXPANSION_IN_PROGRESS) {
                break;
            }
        }
    }

    UNLOCK_EXPANSION(OldIrql);

    MmTotalClaim = NewTotalClaim;
    MmTotalEstimatedAvailable = NewTotalEstimatedAvailable;

}

VOID
MiAgeAndEstimateAvailableInWorkingSet(
    IN PMMSUPPORT VmSupport,
    IN BOOLEAN DoAging,
    IN OUT PULONG TotalClaim,
    IN OUT PULONG TotalEstimatedAvailable
    )

/*++

Routine Description:

    Age pages (clear the access bit or if the page hasn't been
    accessed, increment the age) for a portion of the working
    set.  Also, walk through a sample of the working set
    building a set of counts of how old the pages are.
    
    The counts are used to create a claim of the amount
    the system can steal from this process if memory
    becomes tight.

Arguments:

    VmSupport - Supplies the VM support structure to age and estimate.

    DoAging - TRUE if pages are to be aged.  Regardless, the pages will be
              added to the availablity estimation.

    TotalClaim - Supplies a pointer to system wide claim to update.

    TotalEstimatedAvailable - Supplies a pointer to system wide estimate
                              to update.

Return Value:

    None

Environment:

    Kernel mode, APCs disabled, working set lock.  PFN lock NOT held.

--*/

{
    ULONG LastEntry;
    ULONG StartEntry;
    ULONG FirstDynamic;
    ULONG CurrentEntry;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    PMMPTE PointerPte;
    ULONG NumberToExamine;
    ULONG Claim;
    ULONG Estimate;
    ULONG SampledAgeCounts[MI_USE_AGE_COUNT] = {0};
    MI_NEXT_ESTIMATION_SLOT_CONST NextConst;

    WorkingSetList = VmSupport->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

#if DBG
    if (VmSupport == &MmSystemCacheWs) {
        MM_SYSTEM_WS_LOCK_ASSERT();
    }
#endif //DBG

    LastEntry = WorkingSetList->LastEntry;
    FirstDynamic = WorkingSetList->FirstDynamic;

    if (DoAging == TRUE) {

        //
        // Clear the used bits or increment the age of a
        // portion of the working set.
        //
        // In Usage Estimation/WorkingSet Aging, walk the entire
        // working set every 2^MI_AGE_AGING_SHIFT seconds.
        //

        if (VmSupport->WorkingSetSize > WorkingSetList->FirstDynamic) {
            NumberToExamine = (VmSupport->WorkingSetSize - WorkingSetList->FirstDynamic) >> MiAgingShift;
        }
        else {
            NumberToExamine = 0;
        }

        if (NumberToExamine != 0) {

            CurrentEntry = VmSupport->NextAgingSlot;

            if (CurrentEntry > LastEntry || CurrentEntry < FirstDynamic) {
                CurrentEntry = FirstDynamic;
            }
    
            if (Wsle[CurrentEntry].u1.e1.Valid == 0) {
                MI_NEXT_VALID_AGING_SLOT(CurrentEntry, FirstDynamic, LastEntry, Wsle);
            }
    
            while (NumberToExamine != 0) {
        
                PointerPte = MiGetPteAddress (Wsle[CurrentEntry].u1.VirtualAddress);
        
                if (MI_GET_ACCESSED_IN_PTE(PointerPte) == 1) {
                    MI_SET_ACCESSED_IN_PTE(PointerPte, 0);
                    MI_RESET_WSLE_AGE(PointerPte, &Wsle[CurrentEntry]);
                }
                else {
                    MI_INC_WSLE_AGE(PointerPte, &Wsle[CurrentEntry]);
                }
        
                NumberToExamine -= 1;
                MI_NEXT_VALID_AGING_SLOT(CurrentEntry, FirstDynamic, LastEntry, Wsle);
            }
    
            VmSupport->NextAgingSlot = CurrentEntry + 1; // Start here next time
        }
    }

    //
    // Estimate the number of unused pages in the working set.
    //
    // The working set may have shrunk or the non-paged portion may have
    // grown since the last time.  Put the next counter at the FirstDynamic
    // if so.
    //

    CurrentEntry = VmSupport->NextEstimationSlot;

    if (CurrentEntry > LastEntry || CurrentEntry < FirstDynamic) {
        CurrentEntry = FirstDynamic;
    }

    //
    // In Usage Estimation/WorkingSet Aging, walk the entire
    // working set every 2^MiEstimationShift seconds.
    //

    if (VmSupport->WorkingSetSize > WorkingSetList->FirstDynamic) {
        NumberToExamine = (VmSupport->WorkingSetSize - WorkingSetList->FirstDynamic) >> MiEstimationShift;
    }
    else {
        NumberToExamine = 0;
    }

    if (NumberToExamine != 0) {

        MI_CALC_NEXT_ESTIMATION_SLOT_CONST(NextConst, WorkingSetList);

        StartEntry = FirstDynamic;

        if (Wsle[CurrentEntry].u1.e1.Valid == 0) {

            MI_NEXT_VALID_ESTIMATION_SLOT (CurrentEntry,
                                           StartEntry,
                                           FirstDynamic,
                                           LastEntry,
                                           NextConst,
                                           Wsle);
        }
    
        while (NumberToExamine != 0) {

            PointerPte = MiGetPteAddress (Wsle[CurrentEntry].u1.VirtualAddress);
    
            if (MI_GET_ACCESSED_IN_PTE(PointerPte) == 0) {
                MI_UPDATE_USE_ESTIMATE(PointerPte,
                                        &Wsle[CurrentEntry],
                                        SampledAgeCounts
                                        );
            }
    
            NumberToExamine -= 1;

            MI_NEXT_VALID_ESTIMATION_SLOT (CurrentEntry,
                                           StartEntry,
                                           FirstDynamic,
                                           LastEntry,
                                           NextConst,
                                           Wsle);
        }
    }
    
    //
    // Start estimation here next time.
    //

    VmSupport->NextEstimationSlot = CurrentEntry + 1;

    Estimate = MI_CALCULATE_USAGE_ESTIMATE(SampledAgeCounts);

    Claim = VmSupport->Claim + MI_CLAIM_INCR;

    if (Claim > Estimate) {
        Claim = Estimate;
    }

    VmSupport->Claim = Claim;
    VmSupport->EstimatedAvailable = Estimate;

    PERFINFO_WSMANAGE_DUMPWS(VmSupport, SampledAgeCounts);

    VmSupport->GrowthSinceLastEstimate = 0;
    *TotalClaim += Claim >> ((VmSupport->MemoryPriority == MEMORY_PRIORITY_FOREGROUND)
                                ? MI_FOREGROUND_CLAIM_AVAILABLE_SHIFT
                                : MI_BACKGROUND_CLAIM_AVAILABLE_SHIFT);

    *TotalEstimatedAvailable += Estimate;
    return;
}

ULONG MiClaimAdjustmentThreshold[7] = { 0, 0, 0, 1000, 2000, 4000, 8000};

VOID
MiAdjustClaimParameters(
    BOOLEAN EnoughPages
    )

/*++

Routine Description:

    Adjust the rate at which we walk through working sets.  If we have
    enough pages (we aren't trimming pages that aren't considered young),
    then we check to see whether we should decrease the aging rate and
    vice versa.

    The limits for the aging rate are 1/8 and 1/128 of the working sets.
    This means that the finest age granularities are 8 to 128 seconds in
    these cases.  With the current 2 bit counter, at the low end we would
    start trimming pages > 16 seconds old and at the high end > 4 minutes.

Arguments:

    EnoughPages - Supplies whether to increase the rate or decrease it.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    LARGE_INTEGER CurrentTime;

    KeQuerySystemTime (&CurrentTime);

    if (EnoughPages == TRUE) {

        //
        // Don't adjust the rate too frequently, don't go over the limit, and
        // make sure there are enough claimed and/or available.
        //

        if (((CurrentTime.QuadPart - MiLastAdjustmentOfClaimParams.QuadPart) >
                MmClaimParameterAdjustUpTime.QuadPart) &&
            (MiAgingShift < 7) && 
            ((MmTotalClaim + MmAvailablePages) > MiClaimAdjustmentThreshold[MiAgingShift])) {

            //
            // Set the time only when we change the rate.
            //

            MiLastAdjustmentOfClaimParams.QuadPart = CurrentTime.QuadPart;

            MiAgingShift += 1;
            MiEstimationShift += 1;
        }
    }
    else {

        //
        // Don't adjust the rate down too frequently.
        //

        if ((CurrentTime.QuadPart - MiLastAdjustmentOfClaimParams.QuadPart) >
                MmClaimParameterAdjustDownTime.QuadPart) {
            
            //
            // Always set the time so we don't adjust up too soon after
            // a 2nd pass trim.
            //
    
            MiLastAdjustmentOfClaimParams.QuadPart = CurrentTime.QuadPart;

            //
            // Don't go under the limit.
            //

            if (MiAgingShift > 3) {
                MiAgingShift -= 1;
                MiEstimationShift -= 1;
            }
        }
    }
}
#endif

#define MM_WS_REORG_BUCKETS_MAX 7

#if DBG
ULONG MiSessionIdleBuckets[MM_WS_REORG_BUCKETS_MAX];
#endif

VOID
MiRearrangeWorkingSetExpansionList(
    VOID
    )

/*++

Routine Description:

    This function arranges the working set list into different
    groups based upon the claim.  This is done so the working set
    trimming will take place on fat processes first.

    The working sets are sorted into buckets and then linked back up.

    Swapped out sessions and processes are put at the front.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, no locks held.

--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY ListEntry;
    PMMSUPPORT VmSupport;
    int Size;
    int PreviousNonEmpty;
    int NonEmpty;
    LIST_ENTRY ListHead[MM_WS_REORG_BUCKETS_MAX];
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER SessionIdleTime;
    ULONG IdleTime;
    PMM_SESSION_SPACE SessionGlobal;

    KeQuerySystemTime (&CurrentTime);

    if (IsListEmpty(&MmWorkingSetExpansionHead.ListHead)) {
        return;
    }

    for (Size = 0 ; Size < MM_WS_REORG_BUCKETS_MAX; Size++) {
        InitializeListHead(&ListHead[Size]);
    }

    LOCK_EXPANSION (OldIrql);

    while (!IsListEmpty (&MmWorkingSetExpansionHead.ListHead)) {
        ListEntry = RemoveHeadList (&MmWorkingSetExpansionHead.ListHead);

        VmSupport = CONTAINING_RECORD(ListEntry,
                                          MMSUPPORT,
                                          WorkingSetExpansionLinks);

        if (VmSupport->u.Flags.TrimHard == 1) {

            ASSERT (MiHydra == TRUE);
            ASSERT (VmSupport->u.Flags.SessionSpace == 1);

            SessionGlobal = CONTAINING_RECORD (VmSupport,
                                               MM_SESSION_SPACE,
                                               Vm);

            SessionIdleTime.QuadPart = CurrentTime.QuadPart - SessionGlobal->LastProcessSwappedOutTime.QuadPart;

#if DBG
            if (MmDebug & MM_DBG_SESSIONS) {
                DbgPrint ("Mm: Session %d heavily trim/aged - all its processes (%d) swapped out %d seconds ago\n",
                    SessionGlobal->SessionId,
                    SessionGlobal->ReferenceCount,
                    (ULONG)(SessionIdleTime.QuadPart / 10000000));
            }
#endif

            if (SessionIdleTime.QuadPart < 0) {

                //
                // The administrator has moved the system time backwards.
                // Give this session a fresh start.
                //

                SessionIdleTime.QuadPart = 0;
                KeQuerySystemTime (&SessionGlobal->LastProcessSwappedOutTime);
            }

            IdleTime = (ULONG) (SessionIdleTime.QuadPart / 10000000);
        }
        else {
            IdleTime = 0;
        }

        if (VmSupport->MemoryPriority == MEMORY_PRIORITY_FOREGROUND) {

            //
            // Put the foreground processes at the end of the list,
            // to give them priority.
            //

            Size = 6;
        }
#ifdef _MI_USE_CLAIMS_
        else {

            if (VmSupport->Claim > 400) {
                Size = 0;
            } else if (IdleTime > 30) {
                Size = 0;
#if DBG
                MiSessionIdleBuckets[Size] += 1;
#endif
            } else if (VmSupport->Claim > 200) {
                Size = 1;
            } else if (IdleTime > 20) {
                Size = 1;
#if DBG
                MiSessionIdleBuckets[Size] += 1;
#endif
            } else if (VmSupport->Claim > 100) {
                Size = 2;
            } else if (IdleTime > 10) {
                Size = 2;
#if DBG
                MiSessionIdleBuckets[Size] += 1;
#endif
            } else if (VmSupport->Claim > 50) {
                Size = 3;
            } else if (IdleTime) {
                Size = 3;
#if DBG
                MiSessionIdleBuckets[Size] += 1;
#endif
            } else if (VmSupport->Claim > 25) {
                Size = 4;
            } else {
                Size = 5;
#if DBG
                if (VmSupport->u.Flags.SessionSpace == 1) {
                    MiSessionIdleBuckets[Size] += 1;
                }
#endif
            }
        }
#else
        else {

            //
            // Just put swapped out entries at the front and keep all other
            // entries in the same order, just in front of the foreground
            // processes.
            //

            if (IdleTime > 40) {
                Size = 0;
            } else if (IdleTime > 30) {
                Size = 1;
            } else if (IdleTime > 20) {
                Size = 2;
            } else if (IdleTime > 10) {
                Size = 3;
            } else if (IdleTime) {
                Size = 4;
            } else {
                InsertTailList (&ListHead[5],
                                &VmSupport->WorkingSetExpansionLinks);
                continue;
            }
#if DBG
            ASSERT (MiHydra == TRUE);
            ASSERT (VmSupport->u.Flags.SessionSpace == 1);
            MiSessionIdleBuckets[Size] += 1;
#endif
        }
#endif

#if DBG
        if (MmDebug & MM_DBG_WS_EXPANSION) {
            DbgPrint("MM-rearrange: TrimHard = %d, WS Size = 0x%x, Claim 0x%x, Bucket %d\n",
                    VmSupport->u.Flags.TrimHard,
                    VmSupport->WorkingSetSize,
                    VmSupport->Claim,
                    Size);
        }
#endif //DBG

        //
        // Note: this reverses the bucket order each time we
        // reorganize the lists.  This may be good or bad -
        // if you change it you may want to think about it.
        //

        InsertHeadList (&ListHead[Size],
                        &VmSupport->WorkingSetExpansionLinks);
    }

    //
    // Find the first non-empty list.
    //

    for (NonEmpty = 0 ; NonEmpty < MM_WS_REORG_BUCKETS_MAX ; NonEmpty += 1) {
        if (!IsListEmpty (&ListHead[NonEmpty])) {
            break;
        }
    }

    //
    // Put the head of first non-empty list at the beginning
    // of the MmWorkingSetExpansion list.
    //

    MmWorkingSetExpansionHead.ListHead.Flink = ListHead[NonEmpty].Flink;
    ListHead[NonEmpty].Flink->Blink = &MmWorkingSetExpansionHead.ListHead;

    PreviousNonEmpty = NonEmpty;

    //
    // Link the rest of the lists together.
    //

    for (NonEmpty += 1; NonEmpty < MM_WS_REORG_BUCKETS_MAX; NonEmpty += 1) {

        if (!IsListEmpty (&ListHead[NonEmpty])) {
            
            ListHead[PreviousNonEmpty].Blink->Flink = ListHead[NonEmpty].Flink;
            ListHead[NonEmpty].Flink->Blink = ListHead[PreviousNonEmpty].Blink;
            PreviousNonEmpty = NonEmpty;
        }
    }

    //
    // Link the tail of last non-empty to the MmWorkingSetExpansion list.
    //
   
    MmWorkingSetExpansionHead.ListHead.Blink = ListHead[PreviousNonEmpty].Blink;
    ListHead[PreviousNonEmpty].Blink->Flink = &MmWorkingSetExpansionHead.ListHead;

    UNLOCK_EXPANSION (OldIrql);

    return;
}


VOID
MiEmptyAllWorkingSets (
    VOID
    )

/*++

Routine Description:

    This routine attempts to empty all the working sets on the
    expansion list.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.  No locks held.  APC level or below.

--*/

{
    KIRQL OldIrql;

    PAGED_CODE ();

    MmLockPagableSectionByHandle (ExPageLockHandle);

    if (MiHydra == FALSE) {
        MiEmptyAllWorkingSetsWorker ();
    }
    else {
        ASSERT (PsGetCurrentThread () != MmWorkingSetThread);

        //
        // For Hydra, we cannot attach directly to the session space to be
        // trimmed because it would result in session space references by
        // other threads in this process to the attached session instead
        // of the (currently) correct one.  In fact, we cannot even queue
        // this to a worker thread because the working set manager
        // (who shares the same page directory) may be attaching or
        // detaching from a session (any session).  So this must be queued
        // to the working set manager.
        //
    
        LOCK_EXPANSION (OldIrql);

        if (MiWaitingForWorkingSetEmpty == FALSE) {
            MiWaitingForWorkingSetEmpty = TRUE;
            KeClearEvent (&MiWaitForEmptyEvent);
        }

        UNLOCK_EXPANSION (OldIrql);

        KeSetEvent (&MmWorkingSetManagerEvent, 0, FALSE);

        KeWaitForSingleObject (&MiWaitForEmptyEvent,
                               WrVirtualMemory,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER)0);
    }

    MmUnlockPagableImageSection (ExPageLockHandle);

    return;
}

VOID
MiEmptyAllWorkingSetsWorker (
    VOID
    )

/*++

Routine Description:

    This routine attempts to empty all the working sets on the expansion list.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.  No locks held.  APC level or below.

--*/

{
    PMMSUPPORT VmSupport;
    PMMSUPPORT FirstSeen;
    ULONG SystemCacheSeen;
    KIRQL OldIrql;
    PLIST_ENTRY ListEntry;
    PEPROCESS ProcessToTrim;
    PMM_SESSION_SPACE SessionSpace;
    ULONG LoopCount;

    PAGED_CODE ();

    FirstSeen = NULL;
    SystemCacheSeen = FALSE;
    LoopCount = 0;

    LOCK_EXPANSION (OldIrql);

    while (!IsListEmpty (&MmWorkingSetExpansionHead.ListHead)) {

        //
        // Remove the entry at the head and trim it.
        //

        ListEntry = RemoveHeadList (&MmWorkingSetExpansionHead.ListHead);

        if (ListEntry == &MmSystemCacheWs.WorkingSetExpansionLinks) {
            VmSupport = &MmSystemCacheWs;
            ASSERT (VmSupport->u.Flags.SessionSpace == 0);
            ProcessToTrim = NULL;
            if (SystemCacheSeen != FALSE) {

                //
                // Seen this one already.
                //

                FirstSeen = VmSupport;
            }
            SystemCacheSeen = TRUE;
        }
        else {
            VmSupport = CONTAINING_RECORD(ListEntry,
                                          MMSUPPORT,
                                          WorkingSetExpansionLinks);

            if (VmSupport->u.Flags.SessionSpace == 0) {
                ProcessToTrim = CONTAINING_RECORD(VmSupport,
                                                  EPROCESS,
                                                  Vm);

                ASSERT (VmSupport->VmWorkingSetList == MmWorkingSetList);
                ASSERT (VmSupport == &ProcessToTrim->Vm);
                ASSERT (ProcessToTrim->AddressSpaceDeleted == 0);
                ASSERT (VmSupport->u.Flags.SessionSpace == 0);
            }
            else {
                ASSERT (MiHydra == TRUE);
                SessionSpace = CONTAINING_RECORD(VmSupport,
                                                 MM_SESSION_SPACE,
                                                 Vm);

                ProcessToTrim = NULL;
            }
        }

        if (VmSupport == FirstSeen) {
            InsertHeadList (&MmWorkingSetExpansionHead.ListHead,
                        &VmSupport->WorkingSetExpansionLinks);
            break;
        }

        ASSERT (VmSupport->u.Flags.BeingTrimmed == 0);
        VmSupport->u.Flags.BeingTrimmed = 1;

        VmSupport->WorkingSetExpansionLinks.Flink = MM_NO_WS_EXPANSION;
        VmSupport->WorkingSetExpansionLinks.Blink =
                                            MM_WS_EXPANSION_IN_PROGRESS;
        UNLOCK_EXPANSION (OldIrql);

        if (FirstSeen == NULL) {
            FirstSeen = VmSupport;
        }

        //
        // Empty the working set.
        //

        if (ProcessToTrim == NULL) {
            if (VmSupport->u.Flags.SessionSpace == 0) {
                MiEmptyWorkingSet (VmSupport, FALSE);
            }
            else {
                ASSERT (MiHydra == TRUE);
                MiAttachSession (SessionSpace);
                MiEmptyWorkingSet (VmSupport, FALSE);
                MiDetachSession ();
            }
        }
        else {

            if (VmSupport->WorkingSetSize > 4) {
                KeAttachProcess (&ProcessToTrim->Pcb);
                MiEmptyWorkingSet (VmSupport, FALSE);
                KeDetachProcess ();
            }
        }

        //
        // Add back to the list.
        //

        LOCK_EXPANSION (OldIrql);
        ASSERT (VmSupport->WorkingSetExpansionLinks.Flink == MM_NO_WS_EXPANSION);
        ASSERT (VmSupport->u.Flags.BeingTrimmed == 1);
        VmSupport->u.Flags.BeingTrimmed = 0;

        ASSERT (VmSupport->WorkingSetExpansionLinks.Flink == MM_NO_WS_EXPANSION);
        if (VmSupport->WorkingSetExpansionLinks.Blink ==
                                             MM_WS_EXPANSION_IN_PROGRESS) {

            //
            // If the working set size is still above the minimum,
            // add this back at the tail of the list.
            //

            InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                            &VmSupport->WorkingSetExpansionLinks);
        }
        else {

            //
            // The value in the blink is the address of an event
            // to set.
            //

            ASSERT (VmSupport != &MmSystemCacheWs);

            KeSetEvent ((PKEVENT)VmSupport->WorkingSetExpansionLinks.Blink,
                        0,
                        FALSE);
        }

        //
        // The initial working set that was chosen for FirstSeen may have
        // been trimmed down under its minimum and been removed from the
        // ExpansionHead links.  It is possible that the system cache is not
        // on the links either.  This check detects this extremely rare
        // situation so that the system does not spin forever.
        //

        LoopCount += 1;
        if (LoopCount > 200) {
            if (MmSystemCacheWs.WorkingSetExpansionLinks.Blink == MM_WS_EXPANSION_IN_PROGRESS) {
                break;
            }
        }
    }

    UNLOCK_EXPANSION (OldIrql);

    return;
}

//
// This is deliberately initialized to 1 and only cleared when we have
// initialized enough of the system working set to support a trim.
//

LONG MiTrimInProgressCount = 1;

ULONG MiTrimAllPageFaultCount;


LOGICAL
MmTrimAllSystemPagableMemory (
    IN LOGICAL PurgeTransition
    )

/*++

Routine Description:

    This routine unmaps all pagable system memory.  This does not unmap user
    memory or locked down kernel memory.  Thus, the memory being unmapped
    resides in paged pool, pagable kernel/driver code & data, special pool
    and the system cache.

    Note that pages with a reference count greater than 1 are skipped (ie:
    they remain valid, as they are assumed to be locked down).  This prevents
    us from unmapping all of the system cache entries, etc.

    Non-locked down kernel stacks must be outpaged by modifying the balance
    set manager to operate in conjunction with a support routine.  This is not
    done here.

Arguments:

    PurgeTransition - Supplies whether to purge all the clean pages from the
                      transition list.

Return Value:

    TRUE if accomplished, FALSE if not.

Environment:

    Kernel mode.  APC_LEVEL or below.

--*/

{
    KIRQL OldIrql;
    KIRQL OldIrql2;
    PLIST_ENTRY Next;
    PMMSUPPORT VmSupport;
    WSLE_NUMBER PagesInUse;
    PFN_NUMBER PageFrameIndex;
    LOGICAL LockAvailable;
    PMMPFN Pfn1;
    PETHREAD CurrentThread;
    ULONG flags;

    //
    // It's ok to check this without acquiring the system WS lock.
    //

    if (MiTrimAllPageFaultCount == MmSystemCacheWs.PageFaultCount) {
        return FALSE;
    }

    //
    // Working set mutexes will be acquired which require APC_LEVEL or below.
    //

    if (KeGetCurrentIrql() > APC_LEVEL) {
        return FALSE;
    }

    //
    // Just return if it's too early during system initialization or if
    // another thread/processor is racing here to do the work for us.
    //

    if (InterlockedIncrement (&MiTrimInProgressCount) > 1) {
        InterlockedDecrement (&MiTrimInProgressCount);
        return FALSE;
    }

#if defined(_X86_)

    _asm {
        pushfd
        pop     flags
    }

    if ((flags & EFLAGS_INTERRUPT_MASK) == 0) {
        InterlockedDecrement (&MiTrimInProgressCount);
        return FALSE;
    }

#endif

    LockAvailable = KeTryToAcquireSpinLock (&MmExpansionLock, &OldIrql);

    if (LockAvailable == FALSE) {
        InterlockedDecrement (&MiTrimInProgressCount);
        return FALSE;
    }

    MM_SET_EXPANSION_OWNER ();

    CurrentThread = PsGetCurrentThread();

    //
    // If the system cache resource is owned by this thread then don't bother
    // trying to trim now.  Note that checking the MmSystemLockOwner is not
    // sufficient as this flag is cleared just before actually releasing it.
    //

    if ((CurrentThread == MmSystemLockOwner) ||
        (ExTryToAcquireResourceExclusiveLite(&MmSystemWsLock) == FALSE)) {
        UNLOCK_EXPANSION (OldIrql);
        InterlockedDecrement (&MiTrimInProgressCount);
        return FALSE;
    }

    Next = MmWorkingSetExpansionHead.ListHead.Flink;

    while (Next != &MmWorkingSetExpansionHead.ListHead) {
        if (Next == &MmSystemCacheWs.WorkingSetExpansionLinks) {
            break;
        }
        Next = Next->Flink;
    }

    if (Next != &MmSystemCacheWs.WorkingSetExpansionLinks) {
        ExReleaseResourceLite(&MmSystemWsLock);
        UNLOCK_EXPANSION (OldIrql);
        InterlockedDecrement (&MiTrimInProgressCount);
        return FALSE;
    }

    RemoveEntryList (Next);

    VmSupport = &MmSystemCacheWs;
    VmSupport->WorkingSetExpansionLinks.Flink = MM_NO_WS_EXPANSION;
    VmSupport->WorkingSetExpansionLinks.Blink = MM_WS_EXPANSION_IN_PROGRESS;
    ASSERT (VmSupport->u.Flags.BeingTrimmed == 0);
    VmSupport->u.Flags.BeingTrimmed = 1;

    MiTrimAllPageFaultCount = VmSupport->PageFaultCount;

    PagesInUse = VmSupport->WorkingSetSize;

    //
    // There are 2 issues here that are carefully dealt with :
    //
    // 1.  APCs must be disabled while any resources are held to prevent
    //     suspend APCs from deadlocking the system.
    // 2.  Once the system cache has been marked MM_WS_EXPANSION_IN_PROGRESS,
    //     either the thread must not be preempted or the system cache working
    //     set lock must be held throughout.  Otherwise a high priority thread
    //     can fault on a system code and data address and the two pages will
    //     thrash forever (at high priority) because no system working set
    //     expansion is allowed while MM_WS_EXPANSION_IN_PROGRESS is set.
    //     The decision was to hold the system working set lock throughout.
    //

    MmSystemLockOwner = PsGetCurrentThread ();

    UNLOCK_EXPANSION (APC_LEVEL);

    MiEmptyWorkingSet (VmSupport, FALSE);

    LOCK_EXPANSION (OldIrql2);
    ASSERT (OldIrql2 == APC_LEVEL);

    ASSERT (VmSupport->WorkingSetExpansionLinks.Flink == MM_NO_WS_EXPANSION);

    ASSERT (VmSupport->u.Flags.BeingTrimmed == 1);
    VmSupport->u.Flags.BeingTrimmed = 0;

    ASSERT (VmSupport->WorkingSetExpansionLinks.Blink ==
                                        MM_WS_EXPANSION_IN_PROGRESS);

    InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                    &VmSupport->WorkingSetExpansionLinks);

    UNLOCK_EXPANSION (APC_LEVEL);

    //
    // Since MiEmptyWorkingSet will attempt to recursively acquire and release
    // the MmSystemWsLock, the MmSystemLockOwner field may get cleared.
    // This means here the resource must be explicitly released instead of
    // using UNLOCK_SYSTEM_WS.
    //

    MmSystemLockOwner = NULL;
    ExReleaseResourceLite (&MmSystemWsLock);
    KeLowerIrql (OldIrql);
    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    if (PurgeTransition == TRUE) {

        //
        // Run the transition list and free all the entries so transition
        // faults are not satisfied for any of the non modified pages that were
        // freed.
        //
    
        LOCK_PFN (OldIrql);
    
        while (MmStandbyPageListHead.Total != 0) {
    
            PageFrameIndex = MiRemovePageFromList (&MmStandbyPageListHead);
    
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    
            ASSERT (Pfn1->u2.ShareCount == 0);
            ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
    
            Pfn1->u3.e2.ReferenceCount += 1;
            Pfn1->OriginalPte = ZeroPte;
            Pfn1->u3.e1.Modified = 0;
            MI_SET_PFN_DELETED (Pfn1);
        
            MiDecrementReferenceCount (PageFrameIndex);
        }
    
        UNLOCK_PFN (OldIrql);
    }
    
    InterlockedDecrement (&MiTrimInProgressCount);

    return TRUE;
}
