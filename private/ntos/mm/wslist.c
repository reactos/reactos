/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   wslist.c

Abstract:

    This module contains routines which operate on the working
    set list structure.

Author:

    Lou Perazzoli (loup) 10-Apr-1989
    Landy Wang (landyw) 02-Jun-1997

Revision History:

--*/

#include "mi.h"

#pragma alloc_text(INIT, MiInitializeSessionWsSupport)
#pragma alloc_text(PAGE, MmAssignProcessToJob)
#pragma alloc_text(PAGEHYDRA, MiSessionInitializeWorkingSetList)

#define MM_SYSTEM_CACHE_THRESHOLD ((1024*1024) / PAGE_SIZE)

extern ULONG MmMaximumWorkingSetSize;

ULONG MmFaultsTakenToGoAboveMaxWs = 100;
ULONG MmFaultsTakenToGoAboveMinWs = 16;

ULONG MmSystemCodePage;
ULONG MmSystemCachePage;
ULONG MmPagedPoolPage;
ULONG MmSystemDriverPage;

#ifdef _MI_USE_CLAIMS_
extern BOOLEAN MiReplacing;
#endif

#define MM_RETRY_COUNT 2

extern ULONG MmTransitionSharedPages;
ULONG MmTransitionSharedPagesPeak;

extern LOGICAL MiTrimRemovalPagesOnly;

ULONG
MiDoReplacement(
    IN PMMSUPPORT WsInfo,
    IN BOOLEAN MustReplace
    );

#ifdef _MI_USE_CLAIMS_
VOID
MiReplaceWorkingSetEntryUsingClaim(
    IN PMMSUPPORT WsInfo,
    IN BOOLEAN MustReplace
    );
#else
VOID
MiReplaceWorkingSetEntryUsingFaultInfo(
    IN PMMSUPPORT WsInfo,
    IN BOOLEAN MustReplace
    );
#endif

VOID
MiCheckWsleHash (
    IN PMMWSL WorkingSetList
    );

VOID
MiEliminateWorkingSetEntry (
    IN ULONG WorkingSetIndex,
    IN PMMPTE PointerPte,
    IN PMMPFN Pfn,
    IN PMMWSLE Wsle
    );

ULONG
MiAddWorkingSetPage (
    IN PMMSUPPORT WsInfo
    );

VOID
MiRemoveWorkingSetPages (
    IN PMMWSL WorkingSetList,
    IN PMMSUPPORT WsInfo
    );

VOID
MiCheckNullIndex (
    IN PMMWSL WorkingSetList
    );

VOID
MiDumpWsleInCacheBlock (
    IN PMMPTE CachePte
    );

ULONG
MiDumpPteInCacheBlock (
    IN PMMPTE PointerPte
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK, MmAdjustWorkingSetSize)
#endif // ALLOC_PRAGMA


WSLE_NUMBER
MiLocateAndReserveWsle (
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This function examines the Working Set List for the current
    process and locates an entry to contain a new page.  If the
    working set is not currently at its quota, the new page is
    added without removing a page, if the working set is at its
    quota a page is removed from the working set and the new
    page added in its place.

Arguments:

    None.

Return Value:

    Returns the working set index which is now reserved for the
    next page to be added.

Environment:

    Kernel mode, APCs disabled, working set lock.  PFN lock NOT held.

--*/

{
    WSLE_NUMBER WorkingSetIndex;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    ULONG QuotaIncrement;
    BOOLEAN MustReplace;

    MustReplace = FALSE;
    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    //
    // Update page fault counts.
    //

    WsInfo->PageFaultCount += 1;
    MmInfoCounters.PageFaultCount += 1;

    //
    // Determine if a page should be removed from the working set to make
    // room for the new page.  If so, remove it.  In addition, determine
    // the size of the QuotaIncrement if the size needs to be boosted.
    //

retry_replacement:

    QuotaIncrement = MiDoReplacement(WsInfo, MustReplace);

    ASSERT (WsInfo->WorkingSetSize <= WorkingSetList->Quota);
    WsInfo->WorkingSetSize += 1;

    if (WsInfo->WorkingSetSize > WorkingSetList->Quota) {

        //
        // Increment the quota and check boundary conditions.
        //

        WorkingSetList->Quota += QuotaIncrement;

        WsInfo->LastTrimFaultCount = WsInfo->PageFaultCount;

        if (WorkingSetList->Quota > WorkingSetList->LastInitializedWsle) {

            //
            // Add more pages to the working set list structure.
            //

            if (MiAddWorkingSetPage (WsInfo) == TRUE) {
                ASSERT (WsInfo->WorkingSetSize <= WorkingSetList->Quota);
            }
            else {

                //
                // No page was added to the working set list structure.
                // We must replace a page within this working set.
                //

                WsInfo->WorkingSetSize -= 1;
                MustReplace = TRUE;
                goto retry_replacement;
            }
        }
    }

    //
    // Get the working set entry from the free list.
    //

    ASSERT (WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle);

    ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);

    WorkingSetIndex = WorkingSetList->FirstFree;
    WorkingSetList->FirstFree = (WSLE_NUMBER)(Wsle[WorkingSetIndex].u1.Long >> MM_FREE_WSLE_SHIFT);

    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

    if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
        MmPagesAboveWsMinimum += 1;
    }

    if (WsInfo->WorkingSetSize > WsInfo->PeakWorkingSetSize) {
        WsInfo->PeakWorkingSetSize = WsInfo->WorkingSetSize;
    }

    if (WsInfo == &MmSystemCacheWs) {
        if (WsInfo->WorkingSetSize + MmTransitionSharedPages > MmTransitionSharedPagesPeak) {
            MmTransitionSharedPagesPeak = WsInfo->WorkingSetSize + MmTransitionSharedPages;
        }
    }

    if (WorkingSetIndex > WorkingSetList->LastEntry) {
        WorkingSetList->LastEntry = WorkingSetIndex;
    }

    //
    // The returned entry is guaranteed to be available at this point.
    //

    ASSERT (Wsle[WorkingSetIndex].u1.e1.Valid == 0);

    return WorkingSetIndex;
}


ULONG
MiDoReplacement(
    IN PMMSUPPORT WsInfo,
    IN BOOLEAN MustReplace
    )

/*++

Routine Description:

    This function determines whether the working set should be
    grown or if a page should be replaced.  Replacement is
    done here if deemed necessary.

Arguments:

    WsInfo - Supplies the working set information structure to replace within.

    MustReplace - Supplies TRUE if replacement must succeed.

Return Value:

    Quota increment if growth is necessary.

Environment:

    Kernel mode, APCs disabled, working set lock.  PFN lock NOT held.

--*/

{
    PMMWSL WorkingSetList;
    ULONG CurrentSize;
    LARGE_INTEGER CurrentTime;
#if defined(_ALPHA_) && !defined(_AXP64_)
    KIRQL OldIrql;
#endif
    ULONG QuotaIncrement;
    PFN_NUMBER AvailablePageThreshold;
#ifdef _MI_USE_CLAIMS_
    PEPROCESS ProcessToTrim;
    ULONG Dummy1;
    ULONG Dummy2;
    ULONG Trim;
    ULONG TrimAge;
#endif

    WorkingSetList = WsInfo->VmWorkingSetList;

    if (WsInfo == &MmSystemCacheWs) {
        MM_SYSTEM_WS_LOCK_ASSERT();
        AvailablePageThreshold = MM_SYSTEM_CACHE_THRESHOLD;
    }

    //
    // Determine the number of pages that need to be available to
    // grow the working set and how much the quota should be
    // boosted if the working set grows over it.
    //
    // If below the Minimum use the defaults.
    //

recheck:

    AvailablePageThreshold = 0;
    QuotaIncrement = 1;

    if (WsInfo->WorkingSetSize >= WsInfo->MinimumWorkingSetSize) {

        if (WsInfo->AllowWorkingSetAdjustment == MM_FORCE_TRIM) {

            //
            // The working set manager cannot attach to this process
            // to trim it.  Force a trim now and update the working
            // set manager's fields properly to indicate a trim occurred.
            //

#ifdef _MI_USE_CLAIMS_

            Trim = WsInfo->Claim >>
                            ((WsInfo->MemoryPriority == MEMORY_PRIORITY_FOREGROUND)
                                ? MI_FOREGROUND_CLAIM_AVAILABLE_SHIFT
                                : MI_BACKGROUND_CLAIM_AVAILABLE_SHIFT);

            if (WsInfo == &MmSystemCacheWs) {
                ProcessToTrim = NULL;
            }
            else if (WsInfo->u.Flags.SessionSpace == 0) {
                ProcessToTrim = CONTAINING_RECORD(WsInfo, EPROCESS, Vm);
            }
            else {
                ProcessToTrim = NULL;       // Hydra session.
            }

            if (MmAvailablePages < 100) {
                if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
                    Trim = (WsInfo->WorkingSetSize - WsInfo->MinimumWorkingSetSize) >> 2;
                }
                TrimAge = MI_PASS4_TRIM_AGE;
            }
            else {
                TrimAge = MI_PASS0_TRIM_AGE;
            }

            MiTrimWorkingSet (Trim, WsInfo, TrimAge);

            MiAgeAndEstimateAvailableInWorkingSet (WsInfo,
                                                   TRUE,
                                                   &Dummy1,
                                                   &Dummy2
                                                   );
#else
            MiTrimWorkingSet(20, WsInfo, FALSE);
#endif

            KeQuerySystemTime (&CurrentTime);
            WsInfo->LastTrimTime = CurrentTime;
            WsInfo->LastTrimFaultCount = WsInfo->PageFaultCount;
#if defined(_ALPHA_) && !defined(_AXP64_)
            LOCK_EXPANSION_IF_ALPHA (OldIrql);
#endif
            WsInfo->AllowWorkingSetAdjustment = TRUE;
#if defined(_ALPHA_) && !defined(_AXP64_)
            UNLOCK_EXPANSION_IF_ALPHA (OldIrql);
#endif

            //
            // Set the quota to the current size.
            //

            WorkingSetList->Quota = WsInfo->WorkingSetSize;
            if (WorkingSetList->Quota < WsInfo->MinimumWorkingSetSize) {
                WorkingSetList->Quota = WsInfo->MinimumWorkingSetSize;
            }

            goto recheck;
        }

        CurrentSize = WsInfo->WorkingSetSize;
        ASSERT (CurrentSize <= (WorkingSetList->LastInitializedWsle + 1));

        if ((WsInfo->u.Flags.WorkingSetHard) && (CurrentSize >= WsInfo->MaximumWorkingSetSize)) {

            //
            // This is an enforced working set maximum triggering a replace.
            //

#ifdef _MI_USE_CLAIMS_
            MiReplaceWorkingSetEntryUsingClaim (WsInfo, MustReplace);
#else
            MiReplaceWorkingSetEntryUsingFaultInfo (WsInfo, MustReplace);
#endif
            return 0;
        }

#ifdef _MI_USE_CLAIMS_

        //
        // If using claims, don't grow if
        //      - we're over the max
        //      - there aren't any pages to take
        //      - or if we are growing too much in this
        //        time interval and there isn't much
        //        memory available
        //

        if (CurrentSize > MM_MAXIMUM_WORKING_SET || MmAvailablePages == 0 || MustReplace == TRUE) {

            //
            // Can't grow this one.
            //

            AvailablePageThreshold = 0xffffffff;
            MiReplacing = TRUE;
        }
#if defined (_X86PAE_)
        else if ((WsInfo->u.Flags.SessionSpace == 1) && (CurrentSize > MI_SESSION_MAXIMUM_WORKING_SET)) {

            //
            // Can't grow this one.
            //

            AvailablePageThreshold = 0xffffffff;
            MiReplacing = TRUE;
        }
#endif
        else if (MmAvailablePages < 10000 && MI_WS_GROWING_TOO_FAST(WsInfo)) {

            //
            // Can't grow this one either.
            //

            AvailablePageThreshold = 0xffffffff;
            MiReplacing = TRUE;
        }
#else
        //
        // Not using claims, base the growth on how much faulting
        // the process is doing.
        //

        if (MustReplace == TRUE) {
            AvailablePageThreshold = 0xffffffff;
        }
        if (CurrentSize < WorkingSetList->Quota) {

            //
            // Working set is below quota, allow it to grow with few pages
            // available.
            //

            AvailablePageThreshold = 10;
            QuotaIncrement = 1;
        } else if (CurrentSize < WsInfo->MaximumWorkingSetSize) {

            //
            // Working set is between min and max.  Allow it to grow if enough
            // faults have been taken since last adjustment.
            //

            if ((WsInfo->PageFaultCount - WsInfo->LastTrimFaultCount) <
                    MmFaultsTakenToGoAboveMinWs) {
                AvailablePageThreshold = MmMoreThanEnoughFreePages + 200;
                if (WsInfo->MemoryPriority == MEMORY_PRIORITY_FOREGROUND) {
                    AvailablePageThreshold -= 250;
                }
            } else {
                AvailablePageThreshold = MmWsAdjustThreshold;
            }
            QuotaIncrement = (ULONG)MmWorkingSetSizeIncrement;
        } else {

            //
            // Working set is above max.
            //

            if ((WsInfo->PageFaultCount - WsInfo->LastTrimFaultCount) <
                    (CurrentSize >> 3))
            {
                AvailablePageThreshold = MmMoreThanEnoughFreePages + 200;
                if (WsInfo->MemoryPriority == MEMORY_PRIORITY_FOREGROUND) {
                    AvailablePageThreshold -= 250;
                }
            } else {
                AvailablePageThreshold += MmWsExpandThreshold;
            }
            QuotaIncrement = (ULONG)MmWorkingSetSizeExpansion;

            if (CurrentSize > MM_MAXIMUM_WORKING_SET) {
                AvailablePageThreshold = 0xffffffff;
                QuotaIncrement = 1;
            }
#if defined (_X86PAE_)
            else if ((WsInfo->u.Flags.SessionSpace == 1) && (CurrentSize > MI_SESSION_MAXIMUM_WORKING_SET)) {
                AvailablePageThreshold = 0xffffffff;
                QuotaIncrement = 1;
            }
#endif
        }
#endif
    }

    //
    // If there isn't enough memory to allow growth, find a good page
    // to remove and remove it.
    //

    if (WsInfo->AddressSpaceBeingDeleted == 0 && AvailablePageThreshold != 0) {
        if ((MmAvailablePages <= AvailablePageThreshold) ||
             (WsInfo->WorkingSetExpansionLinks.Flink == MM_NO_WS_EXPANSION)) {

#ifdef _MI_USE_CLAIMS_
            MiReplaceWorkingSetEntryUsingClaim (WsInfo, MustReplace);
#else
            MiReplaceWorkingSetEntryUsingFaultInfo (WsInfo, MustReplace);
#endif
        }
        else {
            WsInfo->GrowthSinceLastEstimate += 1;
        }
    }
    else {
        WsInfo->GrowthSinceLastEstimate += 1;
    }

    return QuotaIncrement;
}


LOGICAL
MmEnforceWorkingSetLimit(
    IN PMMSUPPORT WsInfo,
    IN LOGICAL Enable
    )

/*++

Routine Description:

    This function enables hard enforcement of the working set maximum for
    the specified WsInfo.

Arguments:

    WsInfo - Supplies the working set info pointer.

    Enable - Supplies TRUE if enabling hard enforcement, FALSE if not.

Return Value:

    The previous state of the working set enforcement.

Environment:

    Kernel mode, APCs disabled.  The working set lock must NOT be held.
    The caller guarantees that the target WsInfo cannot go away.

--*/

{
    KIRQL OldIrql;

    LOGICAL PreviousWorkingSetEnforcement;

    LOCK_EXPANSION (OldIrql);

    PreviousWorkingSetEnforcement = WsInfo->u.Flags.WorkingSetHard;

    WsInfo->u.Flags.WorkingSetHard = Enable;

    UNLOCK_EXPANSION (OldIrql);

#if 0

    PEPROCESS CurrentProcess;

    //
    // Get the working set lock and disable APCs.
    // The working set could be trimmed at this point if it is excessive.
    //
    // The working set lock cannot be acquired at this point without updating
    // ps in order to avoid deadlock.
    //

    if (WsInfo == &MmSystemCacheWs) {
        LOCK_SYSTEM_WS (OldIrql2);
        UNLOCK_SYSTEM_WS (OldIrql2);
    }
    else if (WsInfo->u.Flags.SessionSpace == 0) {
        CurrentProcess = PsGetCurrentProcess ();
        LOCK_WS (CurrentProcess);

        UNLOCK_WS (CurrentProcess);
    }
#endif

    return PreviousWorkingSetEnforcement;
}

#ifdef _MI_USE_CLAIMS_

VOID
MiReplaceWorkingSetEntryUsingClaim(
    IN PMMSUPPORT WsInfo,
    IN BOOLEAN MustReplace
    )

/*++

Routine Description:

    This function tries to find a good working set entry to replace.

Arguments:

    WsInfo - Supplies the working set info pointer.

    MustReplace - Supplies TRUE if replacement must succeed.

Return Value:

    None

Environment:

    Kernel mode, APCs disabled, working set lock.  PFN lock NOT held.

--*/

{
    ULONG WorkingSetIndex;
    ULONG FirstDynamic;
    ULONG LastEntry;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    ULONG NumberOfCandidates;
    PMMPTE PointerPte;
    ULONG TheNextSlot;
    ULONG OldestWorkingSetIndex;
    LONG OldestAge;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    //
    // Toss a page out of the working set.
    //

    LastEntry = WorkingSetList->LastEntry;
    FirstDynamic = WorkingSetList->FirstDynamic;
    WorkingSetIndex = WorkingSetList->NextSlot;
    if (WorkingSetIndex > LastEntry || WorkingSetIndex < FirstDynamic) {
        WorkingSetIndex = FirstDynamic;
    }
    TheNextSlot = WorkingSetIndex;
    NumberOfCandidates = 0;

    OldestWorkingSetIndex = WSLE_NULL_INDEX;

    while (TRUE) {

        //
        // Keep track of the oldest page along the way in case we
        // don't find one that's >= MI_IMMEDIATE_REPLACEMENT_AGE
        // before we've looked at MM_WORKING_SET_LIST_SEARCH
        // entries.
        //

        while (Wsle[WorkingSetIndex].u1.e1.Valid == 0) {
            WorkingSetIndex += 1;
            if (WorkingSetIndex > LastEntry) {
                WorkingSetIndex = FirstDynamic;
            }
            if (WorkingSetIndex == TheNextSlot && MustReplace == FALSE) {
    
                //
                // Entire working set list has been searched, increase
                // the working set size.
                //
    
                WsInfo->GrowthSinceLastEstimate += 1;
                return;
            }
        }

        if (OldestWorkingSetIndex == WSLE_NULL_INDEX) {

            //
            // First time through, so initialize the OldestWorkingSetIndex
            // to the first valid WSLE.  As we go along, this will be repointed
            // at the oldest candidate we come across.
            //

            OldestWorkingSetIndex = WorkingSetIndex;
            OldestAge = -1;
        }

        PointerPte = MiGetPteAddress(Wsle[WorkingSetIndex].u1.VirtualAddress);

        if (MustReplace == TRUE ||
            ((MI_GET_ACCESSED_IN_PTE(PointerPte) == 0) &&
            (OldestAge < (LONG) MI_GET_WSLE_AGE(PointerPte, &Wsle[WorkingSetIndex])))) {

            //
            // This one is not used and it's older.
            //

            OldestAge = MI_GET_WSLE_AGE(PointerPte, &Wsle[WorkingSetIndex]);
            OldestWorkingSetIndex = WorkingSetIndex;
        }

        //
        // If it's old enough or we've searched too much then use this entry.
        //

        if (MustReplace == TRUE ||
            OldestAge >= MI_IMMEDIATE_REPLACEMENT_AGE ||
            NumberOfCandidates > MM_WORKING_SET_LIST_SEARCH) {

            PERFINFO_PAGE_INFO_REPLACEMENT_DECL();

            if (OldestWorkingSetIndex != WorkingSetIndex) {
                WorkingSetIndex = OldestWorkingSetIndex;
                PointerPte = MiGetPteAddress(Wsle[WorkingSetIndex].u1.VirtualAddress);
            }

            PERFINFO_GET_PAGE_INFO_REPLACEMENT(PointerPte);

            if (MiFreeWsle(WorkingSetIndex, WsInfo, PointerPte)) {

                PERFINFO_LOG_WS_REPLACEMENT(WsInfo);

                //
                // This entry was removed.
                //

                WorkingSetList->NextSlot = WorkingSetIndex + 1;
                break;
            }

            //
            // We failed to remove a page, try the next one.
            //
            // Clear the OldestWorkingSetIndex so that
            // it gets set to the next valid entry above like the
            // first time around.
            //

            WorkingSetIndex = OldestWorkingSetIndex + 1;

            OldestWorkingSetIndex = WSLE_NULL_INDEX;
        }
        else {
            WorkingSetIndex += 1;
        }

        if (WorkingSetIndex > LastEntry) {
            WorkingSetIndex = FirstDynamic;
        }

        NumberOfCandidates += 1;

        if (WorkingSetIndex == TheNextSlot && MustReplace == FALSE) {

            //
            // Entire working set list has been searched, increase
            // the working set size.
            //

            WsInfo->GrowthSinceLastEstimate += 1;
            break;
        }
    }
}
#else

VOID
MiReplaceWorkingSetEntryUsingFaultInfo(
    IN PMMSUPPORT WsInfo,
    IN BOOLEAN MustReplace
    )

/*++

Routine Description:

    This function tries to find a reasonable working set entry to replace.

Arguments:

    WsInfo - Supplies the working set info pointer.

    MustReplace - Supplies TRUE if replacement must succeed.

Return Value:

    None

Environment:

    Kernel mode, APCs disabled, working set lock.  PFN lock NOT held.

--*/

{
    ULONG WorkingSetIndex;
    ULONG FirstDynamic;
    ULONG LastEntry;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    ULONG NumberOfCandidates;
    PMMPTE PointerPte;
    ULONG TheNextSlot;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    //
    // Toss a page out of the working set.
    //

    LastEntry = WorkingSetList->LastEntry;
    FirstDynamic = WorkingSetList->FirstDynamic;
    WorkingSetIndex = WorkingSetList->NextSlot;

    if ((WorkingSetIndex > LastEntry) || (WorkingSetIndex < FirstDynamic)) {
        WorkingSetIndex = FirstDynamic;
    }

    TheNextSlot = WorkingSetIndex;
    NumberOfCandidates = 0;

    do {

        //
        // Find a valid entry within the set.
        //

        if (Wsle[WorkingSetIndex].u1.e1.Valid != 0) {

            PointerPte = MiGetPteAddress (
                              Wsle[WorkingSetIndex].u1.VirtualAddress);

            if ((MI_GET_ACCESSED_IN_PTE(PointerPte) == 0) ||
                (NumberOfCandidates > MM_WORKING_SET_LIST_SEARCH)) {

                PERFINFO_PAGE_INFO_REPLACEMENT_DECL();
                PERFINFO_GET_PAGE_INFO_REPLACEMENT(PointerPte);

                //
                // Don't pick the same entry we replaced the last time
                // unless we're under extreme pressure.
                //

                if ((WorkingSetIndex != TheNextSlot || MustReplace == TRUE) &&
                    MiFreeWsle (WorkingSetIndex, WsInfo, PointerPte)) {

                    PERFINFO_LOG_WS_REPLACEMENT(WsInfo);

                    //
                    // This entry was removed.
                    //

                    WorkingSetList->NextSlot = WorkingSetIndex + 1;
                    break;
                }
            }
            MI_SET_ACCESSED_IN_PTE (PointerPte, 0);
            NumberOfCandidates += 1;
        }

        WorkingSetIndex += 1;
        if (WorkingSetIndex > LastEntry) {
            WorkingSetIndex = FirstDynamic;
        }

    } while (WorkingSetIndex != TheNextSlot || MustReplace == TRUE);

    //
    // Entire working set list has been searched.  If an entry wasn't
    // removed, our caller can increase the working set size.
    //
}
#endif

ULONG
MiRemovePageFromWorkingSet (
    IN PMMPTE PointerPte,
    IN PMMPFN Pfn1,
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This function removes the page mapped by the specified PTE from
    the process's working set list.

Arguments:

    PointerPte - Supplies a pointer to the PTE mapping the page to
                 be removed from the working set list.

    Pfn1 - Supplies a pointer to the PFN database element referred to
           by the PointerPte.

Return Value:

    Returns TRUE if the specified page was locked in the working set,
    FALSE otherwise.

Environment:

    Kernel mode, APCs disabled, working set mutex held.

--*/

{
    WSLE_NUMBER WorkingSetIndex;
    PVOID VirtualAddress;
    WSLE_NUMBER Entry;
    PVOID SwapVa;
    MMWSLENTRY Locked;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    KIRQL OldIrql;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
    WorkingSetIndex = MiLocateWsle (VirtualAddress,
                                    WorkingSetList,
                                    Pfn1->u1.WsIndex);

    ASSERT (WorkingSetIndex != WSLE_NULL_INDEX);
    LOCK_PFN (OldIrql);
    MiEliminateWorkingSetEntry (WorkingSetIndex,
                                PointerPte,
                                Pfn1,
                                Wsle);

    UNLOCK_PFN (OldIrql);

    //
    // Check to see if this entry is locked in the working set
    // or locked in memory.
    //

    Locked = Wsle[WorkingSetIndex].u1.e1;
    MiRemoveWsle (WorkingSetIndex, WorkingSetList);

    //
    // Add this entry to the list of free working set entries
    // and adjust the working set count.
    //

    MiReleaseWsle ((WSLE_NUMBER)WorkingSetIndex, WsInfo);

    if ((Locked.LockedInWs == 1) || (Locked.LockedInMemory == 1)) {

        //
        // This entry is locked.
        //

        WorkingSetList->FirstDynamic -= 1;

        if (WorkingSetIndex != WorkingSetList->FirstDynamic) {

            SwapVa = Wsle[WorkingSetList->FirstDynamic].u1.VirtualAddress;
            SwapVa = PAGE_ALIGN (SwapVa);

            PointerPte = MiGetPteAddress (SwapVa);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
#if 0
            Entry = MiLocateWsleAndParent (SwapVa,
                                           &Parent,
                                           WorkingSetList,
                                           Pfn1->u1.WsIndex);

            //
            // Swap the removed entry with the last locked entry
            // which is located at first dynamic.
            //

            MiSwapWslEntries (Entry, Parent, WorkingSetIndex, WorkingSetList);
#endif //0

            Entry = MiLocateWsle (SwapVa, WorkingSetList, Pfn1->u1.WsIndex);

            MiSwapWslEntries (Entry, WorkingSetIndex, WsInfo);

        }
        return TRUE;
    } else {
        ASSERT (WorkingSetIndex >= WorkingSetList->FirstDynamic);
    }
    return FALSE;
}


VOID
MiReleaseWsle (
    IN WSLE_NUMBER WorkingSetIndex,
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This function releases a previously reserved working set entry to
    be reused.  A release occurs when a page fault is retried due to
    changes in PTEs and working sets during an I/O operation.

Arguments:

    WorkingSetIndex - Supplies the index of the working set entry to
                      release.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set lock held and PFN lock held.

--*/

{
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;
#if DBG
    if (WsInfo == &MmSystemCacheWs) {
        MM_SYSTEM_WS_LOCK_ASSERT();
    }
#endif //DBG

    ASSERT (WorkingSetIndex <= WorkingSetList->LastInitializedWsle);

    //
    // Put the entry on the free list and decrement the current
    // size.
    //

    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));
    Wsle[WorkingSetIndex].u1.Long = WorkingSetList->FirstFree << MM_FREE_WSLE_SHIFT;
    WorkingSetList->FirstFree = WorkingSetIndex;
    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));
    if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
        MmPagesAboveWsMinimum -= 1;
    }
    WsInfo->WorkingSetSize -= 1;
    return;

}

VOID
MiUpdateWsle (
    IN OUT PWSLE_NUMBER DesiredIndex,
    IN PVOID VirtualAddress,
    PMMWSL WorkingSetList,
    IN PMMPFN Pfn
    )

/*++

Routine Description:

    This routine updates a reserved working set entry to place it into
    the valid state.

Arguments:

    DesiredIndex - Supplies the index of the working set entry to update.

    VirtualAddress - Supplies the virtual address which the working set
                     entry maps.

    WsInfo - Supplies a pointer to the working set info block for the
             process (or system cache).

    Pfn - Supplies a pointer to the PFN element for the page.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set lock held and PFN lock held.

--*/

{
    PMMWSLE Wsle;
    WSLE_NUMBER Index;
    WSLE_NUMBER WorkingSetIndex;
#if PFN_CONSISTENCY
    KIRQL OldIrql;
#endif

    WorkingSetIndex = *DesiredIndex;
    Wsle = WorkingSetList->Wsle;

    if (WorkingSetList == MmSystemCacheWorkingSetList) {

        //
        // This assert doesn't hold for NT64 as we can be adding page
        // directories and page tables for the system cache WSLE hash tables.
        //

        ASSERT32 ((VirtualAddress < (PVOID)PTE_BASE) ||
                  (VirtualAddress >= (PVOID)MM_SYSTEM_SPACE_START));
    } else {
        ASSERT ((VirtualAddress < (PVOID)MM_SYSTEM_SPACE_START) ||
               (MI_IS_SESSION_ADDRESS (VirtualAddress)));
    }

    ASSERT (WorkingSetIndex >= WorkingSetList->FirstDynamic);

    if (WorkingSetList == MmSystemCacheWorkingSetList) {

        MM_SYSTEM_WS_LOCK_ASSERT();

        //
        // count system space inserts and removals.
        //

#if defined(_X86_)
        if (MI_IS_SYSTEM_CACHE_ADDRESS(VirtualAddress)) {
            MmSystemCachePage += 1;
        } else
#endif
        if (VirtualAddress < MmSystemCacheStart) {
            MmSystemCodePage += 1;
        } else if (VirtualAddress < MM_PAGED_POOL_START) {
            MmSystemCachePage += 1;
        } else if (VirtualAddress < MmNonPagedSystemStart) {
            MmPagedPoolPage += 1;
        } else {
            MmSystemDriverPage += 1;
        }
    }

    //
    // Make the wsle valid, referring to the corresponding virtual
    // page number.
    //

    //
    // The value 0 is invalid.  This is due to the fact that the working
    // set lock is a process wide lock and two threads in different
    // processes could be adding the same physical page to their working
    // sets.  Each one could see the WsIndex field in the PFN as 0, and
    // set the direct bit.  To solve this, the WsIndex field is set to
    // the current thread pointer.
    //

    ASSERT (Pfn->u1.WsIndex != 0);

#if DBG
    if (Pfn->u1.WsIndex <= WorkingSetList->LastInitializedWsle) {
        ASSERT ((PAGE_ALIGN(VirtualAddress) !=
                PAGE_ALIGN(Wsle[Pfn->u1.WsIndex].u1.VirtualAddress)) ||
                (Wsle[Pfn->u1.WsIndex].u1.e1.Valid == 0));
    }
#endif //DBG

    Wsle[WorkingSetIndex].u1.VirtualAddress = VirtualAddress;
    Wsle[WorkingSetIndex].u1.Long &= ~(PAGE_SIZE - 1);
    Wsle[WorkingSetIndex].u1.e1.Valid = 1;

    if ((ULONG_PTR)Pfn->u1.Event == (ULONG_PTR)PsGetCurrentThread()) {

        //
        // Directly index into the WSL for this entry via the PFN database
        // element.
        //

        CONSISTENCY_LOCK_PFN (OldIrql);

#if defined(_WIN64)

        //
        // The entire working set index union must be zeroed on Win64 systems.
        //

        MI_ZERO_WSINDEX (Pfn);

#endif

        Pfn->u1.WsIndex = WorkingSetIndex;

        CONSISTENCY_UNLOCK_PFN (OldIrql);

        Wsle[WorkingSetIndex].u1.e1.Direct = 1;

        return;

    } else if (WorkingSetList->HashTable == NULL) {

        //
        // Try to insert at WsIndex.
        //

        Index = Pfn->u1.WsIndex;

        if ((Index < WorkingSetList->LastInitializedWsle) &&
            (Index > WorkingSetList->FirstDynamic) &&
            (Index != WorkingSetIndex)) {

            if (Wsle[Index].u1.e1.Valid) {

                if (Wsle[Index].u1.e1.Direct) {

                    //
                    // Only move direct indexed entries.
                    //

                    PMMSUPPORT WsInfo;

                    if (Wsle == MmWsle) {
                        WsInfo = &PsGetCurrentProcess()->Vm;
                    }
                    else if (Wsle == MmSystemCacheWsle) {
                        WsInfo = &MmSystemCacheWs;
                    }
                    else {
                        WsInfo = &MmSessionSpace->Vm;
                    }

                    MiSwapWslEntries (Index, WorkingSetIndex, WsInfo);
                    WorkingSetIndex = Index;
                }
            } else {

                //
                // On free list, try to remove quickly without walking
                // all the free pages.
                //

                ULONG FreeIndex;
                MMWSLE Temp;

                FreeIndex = 0;

                ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);
                ASSERT (WorkingSetIndex >= WorkingSetList->FirstDynamic);

                if (WorkingSetList->FirstFree == Index) {
                    WorkingSetList->FirstFree = WorkingSetIndex;
                    Temp = Wsle[WorkingSetIndex];
                    Wsle[WorkingSetIndex] = Wsle[Index];
                    Wsle[Index] = Temp;
                    WorkingSetIndex = Index;
                    ASSERT (((Wsle[WorkingSetList->FirstFree].u1.Long >> MM_FREE_WSLE_SHIFT)
                                     <= WorkingSetList->LastInitializedWsle) ||
                            ((Wsle[WorkingSetList->FirstFree].u1.Long >> MM_FREE_WSLE_SHIFT)
                                    == WSLE_NULL_INDEX));
                } else if (Wsle[Index - 1].u1.e1.Valid == 0) {
                    if ((Wsle[Index - 1].u1.Long >> MM_FREE_WSLE_SHIFT) == Index) {
                        FreeIndex = Index - 1;
                    }
                } else if (Wsle[Index + 1].u1.e1.Valid == 0) {
                    if ((Wsle[Index + 1].u1.Long >> MM_FREE_WSLE_SHIFT) == Index) {
                        FreeIndex = Index + 1;
                    }
                }
                if (FreeIndex != 0) {

                    //
                    // Link the Wsle into the free list.
                    //

                    Temp = Wsle[WorkingSetIndex];
                    Wsle[FreeIndex].u1.Long = WorkingSetIndex << MM_FREE_WSLE_SHIFT;
                    Wsle[WorkingSetIndex] = Wsle[Index];
                    Wsle[Index] = Temp;
                    WorkingSetIndex = Index;

                    ASSERT (((Wsle[FreeIndex].u1.Long >> MM_FREE_WSLE_SHIFT)
                                     <= WorkingSetList->LastInitializedWsle) ||
                            ((Wsle[FreeIndex].u1.Long >> MM_FREE_WSLE_SHIFT)
                                    == WSLE_NULL_INDEX));
                }

            }
            *DesiredIndex = WorkingSetIndex;

            if (WorkingSetIndex > WorkingSetList->LastEntry) {
                WorkingSetList->LastEntry = WorkingSetIndex;
            }
        }
    }

    //
    // Insert the valid WSLE into the working set list.
    //

    MiInsertWsle (WorkingSetIndex, WorkingSetList);
    return;
}


ULONG
MiFreeWsle (
    IN WSLE_NUMBER WorkingSetIndex,
    IN PMMSUPPORT WsInfo,
    IN PMMPTE PointerPte
    )

/*++

Routine Description:

    This routine frees the specified WSLE and decrements the share
    count for the corresponding page, putting the PTE into a transition
    state if the share count goes to 0.

Arguments:

    WorkingSetIndex - Supplies the index of the working set entry to free.

    WsInfo - Supplies a pointer to the working set structure (process or
             system cache).

    PointerPte - Supplies a pointer to the PTE for the working set entry.

Return Value:

    Returns TRUE if the WSLE was removed, FALSE if it was not removed.
        Pages with valid PTEs are not removed (i.e. page table pages
        that contain valid or transition PTEs).

Environment:

    Kernel mode, APCs disabled, working set lock.  PFN lock NOT held.

--*/

{
    PMMPFN Pfn1;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    KIRQL OldIrql;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

#if DBG
    if (WsInfo == &MmSystemCacheWs) {
        MM_SYSTEM_WS_LOCK_ASSERT();
    }
#endif //DBG

    ASSERT (Wsle[WorkingSetIndex].u1.e1.Valid == 1);

    //
    // Check to see if the located entry is eligible for removal.
    //

    ASSERT (PointerPte->u.Hard.Valid == 1);

    ASSERT (WorkingSetIndex >= WorkingSetList->FirstDynamic);

    //
    // Check to see if this is a page table with valid PTEs.
    //
    // Note, don't clear the access bit for page table pages
    // with valid PTEs as this could cause an access trap fault which
    // would not be handled (it is only handled for PTEs not PDEs).
    //

    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    LOCK_PFN (OldIrql);

    //
    // If the PTE is a page table page with non-zero share count or
    // within the system cache with its reference count greater
    // than 0, don't remove it.
    //

    if (WsInfo == &MmSystemCacheWs) {
        if (Pfn1->u3.e2.ReferenceCount > 1) {
            UNLOCK_PFN (OldIrql);
            return FALSE;
        }
    } else {
        if ((Pfn1->u2.ShareCount > 1) &&
            (Pfn1->u3.e1.PrototypePte == 0)) {

#if DBG
            if (WsInfo->u.Flags.SessionSpace == 1) {
                ASSERT (MI_IS_SESSION_ADDRESS (Wsle[WorkingSetIndex].u1.VirtualAddress));
            }
            else {
                ASSERT ((Wsle[WorkingSetIndex].u1.VirtualAddress >= (PVOID)PTE_BASE) &&
                 (Wsle[WorkingSetIndex].u1.VirtualAddress<= (PVOID)PDE_TOP));
            }
#endif

            //
            // Don't remove page table pages from the working set until
            // all transition pages have exited.
            //

            UNLOCK_PFN (OldIrql);
            return FALSE;
        }
    }

    //
    // Found a candidate, remove the page from the working set.
    //

    MiEliminateWorkingSetEntry (WorkingSetIndex,
                                PointerPte,
                                Pfn1,
                                Wsle);

    UNLOCK_PFN (OldIrql);

    //
    // Remove the working set entry from the working set.
    //

    MiRemoveWsle (WorkingSetIndex, WorkingSetList);

    ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);

    ASSERT (WorkingSetIndex >= WorkingSetList->FirstDynamic);

    //
    // Put the entry on the free list and decrement the current
    // size.
    //

    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));
    Wsle[WorkingSetIndex].u1.Long = WorkingSetList->FirstFree << MM_FREE_WSLE_SHIFT;
    WorkingSetList->FirstFree = WorkingSetIndex;
    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

    if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
        MmPagesAboveWsMinimum -= 1;
    }
    WsInfo->WorkingSetSize -= 1;

#if 0
    if ((WsInfo == &MmSystemCacheWs) &&
       (Pfn1->u3.e1.Modified == 1))  {
        MiDumpWsleInCacheBlock (PointerPte);
    }
#endif //0
    return TRUE;
}

VOID
MiInitializeWorkingSetList (
    IN PEPROCESS CurrentProcess
    )

/*++

Routine Description:

    This routine initializes a process's working set to the empty
    state.

Arguments:

    CurrentProcess - Supplies a pointer to the process to initialize.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    ULONG i;
    PMMWSLE WslEntry;
    ULONG CurrentEntry;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    WSLE_NUMBER NumberOfEntriesMapped;
    ULONG_PTR CurrentVa;
    PFN_NUMBER WorkingSetPage;
    MMPTE TempPte;
    KIRQL OldIrql;

    WslEntry = MmWsle;

    //
    // Initialize the temporary double mapping portion of hyperspace, if
    // it has not already been done.
    //
    // Initialize the working set list control cells.
    //

    MmWorkingSetList->LastEntry = CurrentProcess->Vm.MinimumWorkingSetSize;
    MmWorkingSetList->Quota = MmWorkingSetList->LastEntry;
    MmWorkingSetList->WaitingForImageMapping = (PKEVENT)NULL;
    MmWorkingSetList->HashTable = NULL;
    MmWorkingSetList->HashTableSize = 0;
    MmWorkingSetList->Wsle = MmWsle;
    MmWorkingSetList->HashTableStart = 
       (PVOID)((PCHAR)PAGE_ALIGN (&MmWsle[MM_MAXIMUM_WORKING_SET]) + PAGE_SIZE);

    MmWorkingSetList->HighestPermittedHashAddress = (PVOID)(HYPER_SPACE_END + 1);

    //
    // Fill in the reserved slots.  Start with the page directory page.
    //

#if !defined (_X86PAE_)

    WslEntry->u1.Long = PDE_BASE;
    WslEntry->u1.e1.Valid = 1;
    WslEntry->u1.e1.LockedInWs = 1;
    WslEntry->u1.e1.Direct = 1;

    PointerPte = MiGetPteAddress (WslEntry->u1.VirtualAddress);
    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    CONSISTENCY_LOCK_PFN (OldIrql);

#if !defined (_WIN64)
    Pfn1->u1.Event = (PVOID)CurrentProcess;
#endif

    CONSISTENCY_UNLOCK_PFN (OldIrql);

#else

    //
    // Fill in all the page directory entries.
    //

    for (i = 0; i < PD_PER_SYSTEM; i += 1) {

        WslEntry->u1.VirtualAddress = (PVOID)(PDE_BASE + i * PAGE_SIZE);
        WslEntry->u1.e1.Valid = 1;
        WslEntry->u1.e1.LockedInWs = 1;
        WslEntry->u1.e1.Direct = 1;

        PointerPte = MiGetPteAddress (WslEntry->u1.VirtualAddress);
        Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
    
        ASSERT (Pfn1->u1.WsIndex == 0);
    
        CONSISTENCY_LOCK_PFN (OldIrql);
    
        Pfn1->u1.WsIndex = (WSLE_NUMBER)(WslEntry - MmWsle);
    
        CONSISTENCY_UNLOCK_PFN (OldIrql);

        WslEntry += 1;
    }
    WslEntry -= 1;

#endif


#if defined (_WIN64)

    //
    // Fill in the entry for the page directory parent page.
    //

    WslEntry += 1;

    WslEntry->u1.Long = PDE_TBASE;
    WslEntry->u1.e1.Valid = 1;
    WslEntry->u1.e1.LockedInWs = 1;
    WslEntry->u1.e1.Direct = 1;

    PointerPte = MiGetPteAddress (WslEntry->u1.VirtualAddress);
    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    ASSERT (Pfn1->u1.WsIndex == 0);

    CONSISTENCY_LOCK_PFN (OldIrql);

    Pfn1->u1.WsIndex = (WSLE_NUMBER)(WslEntry - MmWsle);

    CONSISTENCY_UNLOCK_PFN (OldIrql);

    //
    // Fill in the entry for the hyper space page directory page.
    //

    WslEntry += 1;

    WslEntry->u1.VirtualAddress = (PVOID)MiGetPdeAddress (HYPER_SPACE);
    WslEntry->u1.e1.Valid = 1;
    WslEntry->u1.e1.LockedInWs = 1;
    WslEntry->u1.e1.Direct = 1;

    PointerPte = MiGetPteAddress (WslEntry->u1.VirtualAddress);
    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    ASSERT (Pfn1->u1.WsIndex == 0);

    CONSISTENCY_LOCK_PFN (OldIrql);

    Pfn1->u1.WsIndex = (WSLE_NUMBER)(WslEntry - MmWsle);

    CONSISTENCY_UNLOCK_PFN (OldIrql);

#if defined (_IA64_)

    //
    // Fill in the entry for the session space page directory parent page.
    //

    WslEntry += 1;

    WslEntry->u1.VirtualAddress = (PVOID)MiGetPpeAddress (MM_SESSION_SPACE_DEFAULT);
    WslEntry->u1.e1.Valid = 1;
    WslEntry->u1.e1.LockedInWs = 1;
    WslEntry->u1.e1.Direct = 1;

    PointerPte = MiGetPteAddress (WslEntry->u1.VirtualAddress);
    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    ASSERT (Pfn1->u1.WsIndex == 0);

    CONSISTENCY_LOCK_PFN (OldIrql);

    Pfn1->u1.WsIndex = (WSLE_NUMBER)(WslEntry - MmWsle);

    CONSISTENCY_UNLOCK_PFN (OldIrql);
    
#endif

#endif

    //
    // Fill in the entry for the page table page which maps hyper space.
    //

    WslEntry += 1;

    WslEntry->u1.VirtualAddress = (PVOID)MiGetPteAddress (HYPER_SPACE);
    WslEntry->u1.e1.Valid = 1;
    WslEntry->u1.e1.LockedInWs = 1;
    WslEntry->u1.e1.Direct = 1;

    PointerPte = MiGetPteAddress (WslEntry->u1.VirtualAddress);
    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    ASSERT (Pfn1->u1.WsIndex == 0);

    CONSISTENCY_LOCK_PFN (OldIrql);

    Pfn1->u1.WsIndex = (WSLE_NUMBER)(WslEntry - MmWsle);

    CONSISTENCY_UNLOCK_PFN (OldIrql);

#if defined (_X86PAE_)

    //
    // Fill in the entry for the second page table page which maps hyper space.
    //

    WslEntry += 1;

    WslEntry->u1.VirtualAddress = (PVOID)MiGetPteAddress (HYPER_SPACE2);
    WslEntry->u1.e1.Valid = 1;
    WslEntry->u1.e1.LockedInWs = 1;
    WslEntry->u1.e1.Direct = 1;

    PointerPte = MiGetPteAddress (WslEntry->u1.VirtualAddress);
    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    ASSERT (Pfn1->u1.WsIndex == 0);

    CONSISTENCY_LOCK_PFN (OldIrql);

    Pfn1->u1.WsIndex = (WSLE_NUMBER)(WslEntry - MmWsle);

    CONSISTENCY_UNLOCK_PFN (OldIrql);

#endif

    //
    // Fill in the entry for the page which contains the working set list.
    //

    WslEntry += 1;

    WslEntry->u1.VirtualAddress = (PVOID)MmWorkingSetList;
    WslEntry->u1.e1.Valid = 1;
    WslEntry->u1.e1.LockedInWs = 1;
    WslEntry->u1.e1.Direct = 1;

    PointerPte = MiGetPteAddress (WslEntry->u1.VirtualAddress);
    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    ASSERT (Pfn1->u1.WsIndex == 0);

    CONSISTENCY_LOCK_PFN (OldIrql);

    Pfn1->u1.WsIndex = (WSLE_NUMBER)(WslEntry - MmWsle);

    CONSISTENCY_UNLOCK_PFN (OldIrql);

    CurrentEntry = (WSLE_NUMBER)((WslEntry - MmWsle) + 1);

    //
    // Check to see if more pages are required in the working set list
    // to map the current maximum working set size.
    //

    NumberOfEntriesMapped = (WSLE_NUMBER)(((PMMWSLE)((PCHAR)WORKING_SET_LIST + PAGE_SIZE)) -
                                MmWsle);

    if (CurrentProcess->Vm.MaximumWorkingSetSize >= NumberOfEntriesMapped) {

        PointerPte = MiGetPteAddress (&MmWsle[0]);

        CurrentVa = (ULONG_PTR)MmWorkingSetList + PAGE_SIZE;

        //
        // The working set requires more than a single page.
        //

        LOCK_PFN (OldIrql);

        do {

            MiEnsureAvailablePageOrWait (NULL, NULL);

            PointerPte += 1;
            WorkingSetPage = MiRemoveZeroPage (
                                    MI_PAGE_COLOR_PTE_PROCESS (PointerPte,
                                              &CurrentProcess->NextPageColor));
            PointerPte->u.Long = MM_DEMAND_ZERO_WRITE_PTE;

            MiInitializePfn (WorkingSetPage, PointerPte, 1);

            MI_MAKE_VALID_PTE (TempPte,
                               WorkingSetPage,
                               MM_READWRITE,
                               PointerPte );

            MI_SET_PTE_DIRTY (TempPte);

            MI_SET_PTE_IN_WORKING_SET (&TempPte, CurrentEntry);

            MI_WRITE_VALID_PTE (PointerPte, TempPte);

            WslEntry += 1;

            WslEntry->u1.Long = CurrentVa;
            WslEntry->u1.e1.Valid = 1;
            WslEntry->u1.e1.LockedInWs = 1;
            WslEntry->u1.e1.Direct = 1;

            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

            ASSERT (Pfn1->u1.WsIndex == 0);
            Pfn1->u1.WsIndex = CurrentEntry;

            // MiInsertWsle(CurrentEntry, MmWorkingSetList);

            CurrentEntry += 1;
            CurrentVa += PAGE_SIZE;

            NumberOfEntriesMapped += PAGE_SIZE / sizeof(MMWSLE);

        } while (CurrentProcess->Vm.MaximumWorkingSetSize >= NumberOfEntriesMapped);

        UNLOCK_PFN (OldIrql);
    }

    CurrentProcess->Vm.WorkingSetSize = CurrentEntry;
    MmWorkingSetList->FirstFree = CurrentEntry;
    MmWorkingSetList->FirstDynamic = CurrentEntry;
    MmWorkingSetList->NextSlot = CurrentEntry;

    //
    // Initialize the following slots as free.
    //

    i = CurrentEntry + 1;
    do {

        //
        // Build the free list, note that the first working
        // set entries (CurrentEntry) are not on the free list.
        // These entries are reserved for the pages which
        // map the working set and the page which contains the PDE.
        //

        WslEntry += 1;
        WslEntry->u1.Long = i << MM_FREE_WSLE_SHIFT;
        i += 1;
    } while (i <= NumberOfEntriesMapped);

    WslEntry->u1.Long = WSLE_NULL_INDEX << MM_FREE_WSLE_SHIFT;  // End of list.

    MmWorkingSetList->LastInitializedWsle =
                                NumberOfEntriesMapped - 1;

    if (CurrentProcess->Vm.MaximumWorkingSetSize > ((1536*1024) >> PAGE_SHIFT)) {

        //
        // The working set list consists of more than a single page.
        //

        MiGrowWsleHash (&CurrentProcess->Vm);
    }

    return;
}


VOID
MiInitializeSessionWsSupport(
    VOID
    )

/*++

Routine Description:

    This routine initializes the session space working set support.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL or below, no mutexes held.

--*/

{
    //
    // This is the list of all session spaces ordered in a working set list.
    //

    InitializeListHead (&MiSessionWsList);
}


NTSTATUS
MiSessionInitializeWorkingSetList (
    VOID
    )

/*++

Routine Description:

    This function initializes the working set for the session space and adds
    it to the list of session space working sets.

Arguments:

    None.

Return Value:

    NT_SUCCESS if success or STATUS_NO_MEMORY on failure.

Environment:

    Kernel mode, APC_LEVEL or below, no mutexes held.

--*/

{
    ULONG i;
    KIRQL OldIrql;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    MMPTE  TempPte;
    PMMWSLE WslEntry;
    PMMPFN Pfn1;
    ULONG PageColor;
    PFN_NUMBER ResidentPages;
    ULONG Index;
    PFN_NUMBER PageFrameIndex;
    ULONG CurrentEntry;
    ULONG NumberOfEntriesMapped;
    ULONG_PTR AdditionalBytes;
    ULONG NumberOfEntriesMappedByFirstPage;
    ULONG WorkingSetMaximum;
    PMM_SESSION_SPACE SessionGlobal;
    LOGICAL AllocatedPageTable;

    //
    // Use the global address for pointer references by
    // MmWorkingSetManager before it attaches to the address space.
    //

    SessionGlobal = SESSION_GLOBAL (MmSessionSpace);

    //
    // Set up the working set variables.
    //

    WorkingSetMaximum = MI_SESSION_SPACE_WORKING_SET_MAXIMUM;

    MmSessionSpace->Vm.VmWorkingSetList = (PMMWSL)MI_SESSION_SPACE_WS;
#if defined (_WIN64)
    MmSessionSpace->Wsle = (PMMWSLE)(MmSessionSpace->Vm.VmWorkingSetList + 1);
#else
    MmSessionSpace->Wsle =
            (PMMWSLE)(&MmSessionSpace->Vm.VmWorkingSetList->UsedPageTableEntries[0]);
#endif

    ASSERT (MmSessionSpace->WorkingSetLockOwner == NULL);
    ASSERT (MmSessionSpace->WorkingSetLockOwnerCount == 0);

    //
    // Build the PDE entry for the working set - note that the global bit
    // must be turned off.
    //

    PointerPde = MiGetPdeAddress (MmSessionSpace->Vm.VmWorkingSetList);

    //
    // The page table page for the working set and its first data page
    // are charged against MmResidentAvailablePages and for commitment.
    //

    if (PointerPde->u.Hard.Valid == 1) {

        //
        // The page directory entry for the working set is the same
        // as for another range in the session space.  Share the PDE.
        //

#ifndef _IA64_
        ASSERT (PointerPde->u.Hard.Global == 0);
#endif
        AllocatedPageTable = FALSE;
        ResidentPages = 1;
    }
    else {
        AllocatedPageTable = TRUE;
        ResidentPages = 2;
    }


    PointerPte = MiGetPteAddress (MmSessionSpace->Vm.VmWorkingSetList);

    //
    // The data pages needed to map up to maximum working set size are also
    // charged against MmResidentAvailablePages and for commitment.
    //

    NumberOfEntriesMappedByFirstPage = (WSLE_NUMBER)(
        ((PMMWSLE)((ULONG_PTR)MmSessionSpace->Vm.VmWorkingSetList + PAGE_SIZE)) -
            MmSessionSpace->Wsle);

    if (WorkingSetMaximum > NumberOfEntriesMappedByFirstPage) {
        AdditionalBytes = (WorkingSetMaximum - NumberOfEntriesMappedByFirstPage) * sizeof (MMWSLE);
        ResidentPages += BYTES_TO_PAGES (AdditionalBytes);
    }

    if (MiChargeCommitment (ResidentPages, NULL) == FALSE) {
#if DBG
        DbgPrint("MiSessionInitializeWorkingSetList: No commit for %d pages\n",
            ResidentPages);

#endif
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_COMMIT);
        return STATUS_NO_MEMORY;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_WS_INIT, ResidentPages);

    //
    // Use the global address for resources since they are linked
    // into the global system wide resource list.
    //

    ExInitializeResource (&SessionGlobal->WsLock);

    LOCK_PFN (OldIrql);

    //
    // Check to make sure the physical pages are available.
    //

    if ((SPFN_NUMBER)ResidentPages > MI_NONPAGABLE_MEMORY_AVAILABLE() - 20) {
#if DBG
        DbgPrint("MiSessionInitializeWorkingSetList: No Resident Pages %d, Need %d\n",
            MmResidentAvailablePages,
            ResidentPages);
#endif
        UNLOCK_PFN (OldIrql);

        MiReturnCommitment (ResidentPages);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_SESSION_WSL_FAILURE, ResidentPages);
        ExDeleteResource (&SessionGlobal->WsLock);
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_RESIDENT);
        return STATUS_NO_MEMORY;
    }

    MmResidentAvailablePages -= ResidentPages;

    MM_BUMP_COUNTER(50, ResidentPages);

    if (AllocatedPageTable == TRUE) {

        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_WS_PAGETABLE_ALLOC, 1);

        MiEnsureAvailablePageOrWait (NULL, NULL);

        PageColor = MI_GET_PAGE_COLOR_FROM_VA (NULL);

        PageFrameIndex = MiRemoveZeroPageIfAny (PageColor);
        if (PageFrameIndex == 0) {
            PageFrameIndex = MiRemoveAnyPage (PageColor);
            UNLOCK_PFN (OldIrql);
            MiZeroPhysicalPage (PageFrameIndex, PageColor);
            LOCK_PFN (OldIrql);
        }

        //
        // The global bit is masked off since we need to make sure the TB entry
        // is flushed when we switch to a process in a different session space.
        //

        TempPte.u.Long = ValidKernelPdeLocal.u.Long;
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        MI_WRITE_VALID_PTE (PointerPde, TempPte);

#if !defined (_WIN64)

        //
        // Add this to the session structure so other processes can fault it in.
        //

        Index = MiGetPdeSessionIndex (MmSessionSpace->Vm.VmWorkingSetList);

        MmSessionSpace->PageTables[Index] = TempPte;

#endif

        //
        // This page frame references the session space page table page.
        //

        MiInitializePfnForOtherProcess (PageFrameIndex,
                                        PointerPde,
                                        MmSessionSpace->SessionPageDirectoryIndex);

        MiFillMemoryPte (PointerPte, PAGE_SIZE, ZeroKernelPte.u.Long);

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // This page is never paged, ensure that its WsIndex stays clear so the
        // release of the page will be handled correctly.
        //

        ASSERT (Pfn1->u1.WsIndex == 0);

        KeFillEntryTb ((PHARDWARE_PTE) PointerPde, PointerPte, FALSE);
    }

    MiEnsureAvailablePageOrWait (NULL, NULL);

    PageColor = MI_GET_PAGE_COLOR_FROM_VA (NULL);

    PageFrameIndex = MiRemoveZeroPageIfAny (PageColor);
    if (PageFrameIndex == 0) {
        PageFrameIndex = MiRemoveAnyPage (PageColor);
        UNLOCK_PFN (OldIrql);
        MiZeroPhysicalPage (PageFrameIndex, PageColor);
        LOCK_PFN (OldIrql);
    }

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_WS_PAGE_ALLOC, ResidentPages - 1);

#if DBG
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    ASSERT (Pfn1->u1.WsIndex == 0);
#endif

    //
    // The global bit is masked off since we need to make sure the TB entry
    // is flushed when we switch to a process in a different session space.
    //

    TempPte.u.Long = ValidKernelPteLocal.u.Long;
    MI_SET_PTE_DIRTY (TempPte);
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    MiInitializePfn (PageFrameIndex, PointerPte, 1);

#if DBG
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    ASSERT (Pfn1->u1.WsIndex == 0);
#endif

    UNLOCK_PFN (OldIrql);

    KeFillEntryTb ((PHARDWARE_PTE) PointerPte,
                   (PMMPTE)MmSessionSpace->Vm.VmWorkingSetList,
                   FALSE);

    //
    // Fill in the reserved slots starting with the session data page.
    //

    WslEntry = MmSessionSpace->Wsle;

    WslEntry->u1.VirtualAddress = (PVOID)MmSessionSpace;
    WslEntry->u1.e1.Valid = 1;
    WslEntry->u1.e1.LockedInWs = 1;
    WslEntry->u1.e1.Direct = 1;

    Pfn1 = MI_PFN_ELEMENT (MiGetPteAddress (WslEntry->u1.VirtualAddress)->u.Hard.PageFrameNumber);

    ASSERT (Pfn1->u1.WsIndex == 0);

    //
    // The next reserved slot is for the page table page mapping
    // the session data page.
    //

    WslEntry += 1;

    WslEntry->u1.VirtualAddress = MiGetPteAddress (MmSessionSpace);
    WslEntry->u1.e1.Valid = 1;
    WslEntry->u1.e1.LockedInWs = 1;
    WslEntry->u1.e1.Direct = 1;

    Pfn1 = MI_PFN_ELEMENT (MiGetPteAddress (WslEntry->u1.VirtualAddress)->u.Hard.PageFrameNumber);

    ASSERT (Pfn1->u1.WsIndex == 0);

    CONSISTENCY_LOCK_PFN (OldIrql);

    Pfn1->u1.WsIndex = (WSLE_NUMBER)(WslEntry - MmSessionSpace->Wsle);

    CONSISTENCY_UNLOCK_PFN (OldIrql);

    //
    // The next reserved slot is for the working set page.
    //

    WslEntry += 1;

    WslEntry->u1.VirtualAddress = MmSessionSpace->Vm.VmWorkingSetList;
    WslEntry->u1.e1.Valid = 1;
    WslEntry->u1.e1.LockedInWs = 1;
    WslEntry->u1.e1.Direct = 1;

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    ASSERT (Pfn1->u1.WsIndex == 0);

    CONSISTENCY_LOCK_PFN (OldIrql);

    Pfn1->u1.WsIndex = (WSLE_NUMBER)(WslEntry - MmSessionSpace->Wsle);

    CONSISTENCY_UNLOCK_PFN (OldIrql);

    if (AllocatedPageTable == TRUE) {

        //
        // The next reserved slot is for the page table page
        // mapping the working set page.
        //

        WslEntry += 1;

        WslEntry->u1.VirtualAddress = (PVOID)PointerPte;
        WslEntry->u1.e1.Valid = 1;
        WslEntry->u1.e1.LockedInWs = 1;
        WslEntry->u1.e1.Direct = 1;

        Pfn1 = MI_PFN_ELEMENT (MiGetPteAddress (WslEntry->u1.VirtualAddress)->u.Hard.PageFrameNumber);

        ASSERT (Pfn1->u1.WsIndex == 0);

        CONSISTENCY_LOCK_PFN (OldIrql);

        Pfn1->u1.WsIndex = (WSLE_NUMBER)(WslEntry - MmSessionSpace->Wsle);

        CONSISTENCY_UNLOCK_PFN (OldIrql);
    }

    //
    // The next reserved slot is for the page table page
    // mapping the first session paged pool page.
    //

    WslEntry += 1;

    WslEntry->u1.VirtualAddress = (PVOID)MiGetPteAddress (MmSessionSpace->PagedPoolStart);
    WslEntry->u1.e1.Valid = 1;
    WslEntry->u1.e1.LockedInWs = 1;
    WslEntry->u1.e1.Direct = 1;

    Pfn1 = MI_PFN_ELEMENT (MiGetPteAddress (WslEntry->u1.VirtualAddress)->u.Hard.PageFrameNumber);

    ASSERT (Pfn1->u1.WsIndex == 0);

    CONSISTENCY_LOCK_PFN (OldIrql);

    Pfn1->u1.WsIndex = (WSLE_NUMBER)(WslEntry - MmSessionSpace->Wsle);

    CONSISTENCY_UNLOCK_PFN (OldIrql);

    CurrentEntry = (WSLE_NUMBER)(WslEntry + 1 - MmSessionSpace->Wsle);

    MmSessionSpace->Vm.u.Flags.SessionSpace = 1;
    MmSessionSpace->Vm.MinimumWorkingSetSize = MI_SESSION_SPACE_WORKING_SET_MINIMUM;
    MmSessionSpace->Vm.MaximumWorkingSetSize = WorkingSetMaximum;

    //
    // Don't trim from this session till we're finished setting up and
    // it's got some pages in it...
    //

    MmSessionSpace->Vm.AllowWorkingSetAdjustment = FALSE;

    MmSessionSpace->Vm.VmWorkingSetList->LastEntry = MI_SESSION_SPACE_WORKING_SET_MINIMUM;
    MmSessionSpace->Vm.VmWorkingSetList->Quota = MmSessionSpace->Vm.VmWorkingSetList->LastEntry;
    MmSessionSpace->Vm.VmWorkingSetList->HashTable = NULL;
    MmSessionSpace->Vm.VmWorkingSetList->HashTableSize = 0;
    MmSessionSpace->Vm.VmWorkingSetList->Wsle = MmSessionSpace->Wsle;

    MmSessionSpace->Vm.VmWorkingSetList->HashTableStart =
       (PVOID)((PCHAR)PAGE_ALIGN (&MmSessionSpace->Wsle[MI_SESSION_MAXIMUM_WORKING_SET]) + PAGE_SIZE);

#if defined (_X86PAE_)

    //
    // One less page table page is needed on PAE systems.
    //

    MmSessionSpace->Vm.VmWorkingSetList->HighestPermittedHashAddress =
        (PVOID)(MI_SESSION_VIEW_START - MM_VA_MAPPED_BY_PDE);
#else
    MmSessionSpace->Vm.VmWorkingSetList->HighestPermittedHashAddress =
        (PVOID)MI_SESSION_VIEW_START;
#endif

    NumberOfEntriesMapped = (WSLE_NUMBER)(((PMMWSLE)((ULONG_PTR)MmSessionSpace->Vm.VmWorkingSetList +
                                PAGE_SIZE)) - MmSessionSpace->Wsle);

    LOCK_PFN (OldIrql);

    while (NumberOfEntriesMapped < WorkingSetMaximum) {

        PointerPte += 1;

        MiEnsureAvailablePageOrWait (NULL, NULL);

        PageFrameIndex = MiRemoveZeroPage(MI_GET_PAGE_COLOR_FROM_VA (NULL));

        PointerPte->u.Long = MM_DEMAND_ZERO_WRITE_PTE;

        MiInitializePfn (PageFrameIndex, PointerPte, 1);

        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

        MI_SET_PTE_IN_WORKING_SET (&TempPte, CurrentEntry);

        MI_WRITE_VALID_PTE (PointerPte, TempPte);

        WslEntry += 1;

        WslEntry->u1.VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
        WslEntry->u1.e1.Valid = 1;
        WslEntry->u1.e1.LockedInWs = 1;
        WslEntry->u1.e1.Direct = 1;

        Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

        ASSERT (Pfn1->u1.WsIndex == 0);
        Pfn1->u1.WsIndex = CurrentEntry;

        // MiInsertWsle(CurrentEntry, MmWorkingSetList);

        CurrentEntry += 1;

        NumberOfEntriesMapped += PAGE_SIZE / sizeof(MMWSLE);
    }

    UNLOCK_PFN (OldIrql);

    MmSessionSpace->Vm.WorkingSetSize = CurrentEntry;
    MmSessionSpace->Vm.VmWorkingSetList->FirstFree = CurrentEntry;
    MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic = CurrentEntry;
    MmSessionSpace->Vm.VmWorkingSetList->NextSlot = CurrentEntry;

    MmSessionSpace->NonPagablePages += ResidentPages;
    MmSessionSpace->CommittedPages += ResidentPages;

    //
    // Initialize the following slots as free.
    //

    WslEntry = MmSessionSpace->Wsle + CurrentEntry;

    for (i = CurrentEntry + 1; i < NumberOfEntriesMapped; i += 1) {

        //
        // Build the free list, note that the first working
        // set entries (CurrentEntry) are not on the free list.
        // These entries are reserved for the pages which
        // map the working set and the page which contains the PDE.
        //

        WslEntry->u1.Long = i << MM_FREE_WSLE_SHIFT;
        WslEntry += 1;
    }

    WslEntry->u1.Long = WSLE_NULL_INDEX << MM_FREE_WSLE_SHIFT;  // End of list.

    MmSessionSpace->Vm.VmWorkingSetList->LastInitializedWsle = NumberOfEntriesMapped - 1;

    if (WorkingSetMaximum > ((1536*1024) >> PAGE_SHIFT)) {

        //
        // The working set list consists of more than a single page.
        //

        MiGrowWsleHash (&MmSessionSpace->Vm);
    }

    //
    // Put this session's working set in lists using its global address.
    //

    LOCK_EXPANSION (OldIrql);

    InsertTailList (&MiSessionWsList, &SessionGlobal->WsListEntry);

    MmSessionSpace->u.Flags.HasWsLock = 1;

    MmSessionSpace->u.Flags.SessionListInserted = 1;

    UNLOCK_EXPANSION (OldIrql);

    return STATUS_SUCCESS;
}


LOGICAL
MmAssignProcessToJob (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine acquires the working set lock so a consistent snapshot of
    the argument process' commit charges and working set size can be used
    when adding this process to a job.

Arguments:

    Process - Supplies a pointer to the process to operate upon.

Return Value:

    TRUE if the process is allowed to join the job, FALSE otherwise.

    Note that FALSE cannot be returned without changing the code in ps.

Environment:

    Kernel mode, IRQL APC_LEVEL or below.  The caller provides protection
    from the target process going away.

--*/

{
    LOGICAL Attached;
    LOGICAL Status;

    PAGED_CODE ();

    Attached = FALSE;

    if (PsGetCurrentProcess() != Process) {
        KeAttachProcess (&Process->Pcb);
        Attached = TRUE;
    }

    LOCK_WS_AND_ADDRESS_SPACE (Process);

    Status = PsChangeJobMemoryUsage (Process->CommitCharge);

    //
    // Join the job unconditionally.  If the process is over any limits, it
    // will be caught on its next request.
    //

    Process->JobStatus |= PS_JOB_STATUS_REPORT_COMMIT_CHANGES;

    UNLOCK_WS_AND_ADDRESS_SPACE (Process);

    if (Attached) {
        KeDetachProcess();
    }

    return TRUE;
}


NTSTATUS
MmAdjustWorkingSetSize (
    IN SIZE_T WorkingSetMinimumInBytes,
    IN SIZE_T WorkingSetMaximumInBytes,
    IN ULONG SystemCache
    )

/*++

Routine Description:

    This routine adjusts the current size of a process's working set
    list.  If the maximum value is above the current maximum, pages
    are removed from the working set list.

    An exception is raised if the limit cannot be granted.  This
    could occur if too many pages were locked in the process's
    working set.

    Note: if the minimum and maximum are both (SIZE_T)-1, the working set
          is purged, but the default sizes are not changed.

Arguments:

    WorkingSetMinimumInBytes - Supplies the new minimum working set size in
                               bytes.

    WorkingSetMaximumInBytes - Supplies the new maximum working set size in
                               bytes.

    SystemCache - Supplies TRUE if the system cache working set is being
                  adjusted, FALSE for all other working sets.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode, IRQL APC_LEVEL or below.

--*/


{
    PEPROCESS CurrentProcess;
    ULONG Entry;
    ULONG LastFreed;
    PMMWSLE Wsle;
    KIRQL OldIrql;
    KIRQL OldIrql2;
    SPFN_NUMBER i;
    PMMPTE PointerPte;
    NTSTATUS ReturnStatus;
    LONG PagesAbove;
    LONG NewPagesAbove;
    ULONG FreeTryCount;
    PMMSUPPORT WsInfo;
    PMMWSL WorkingSetList;
    WSLE_NUMBER WorkingSetMinimum;
    WSLE_NUMBER WorkingSetMaximum;

    PERFINFO_PAGE_INFO_DECL();

    FreeTryCount = 0;

    if (SystemCache) {
        WsInfo = &MmSystemCacheWs;
    } else {
        CurrentProcess = PsGetCurrentProcess ();
        WsInfo = &CurrentProcess->Vm;
    }

    if ((WorkingSetMinimumInBytes == (SIZE_T)-1) &&
        (WorkingSetMaximumInBytes == (SIZE_T)-1)) {
        return MiEmptyWorkingSet (WsInfo, TRUE);
    }

    if (WorkingSetMinimumInBytes == 0) {
        WorkingSetMinimum = WsInfo->MinimumWorkingSetSize;
    }
    else {
        WorkingSetMinimum = (WSLE_NUMBER)(WorkingSetMinimumInBytes >> PAGE_SHIFT);
    }

    if (WorkingSetMaximumInBytes == 0) {
        WorkingSetMaximum = WsInfo->MaximumWorkingSetSize;
    }
    else {
        WorkingSetMaximum = (WSLE_NUMBER)(WorkingSetMaximumInBytes >> PAGE_SHIFT);
    }

    if (WorkingSetMinimum > WorkingSetMaximum) {
        return STATUS_BAD_WORKING_SET_LIMIT;
    }

    MmLockPagableSectionByHandle(ExPageLockHandle);

    ReturnStatus = STATUS_SUCCESS;

    //
    // Get the working set lock and disable APCs.
    //

    if (SystemCache) {
        LOCK_SYSTEM_WS (OldIrql2);
    } else {
        LOCK_WS (CurrentProcess);

        if (CurrentProcess->AddressSpaceDeleted != 0) {
            ReturnStatus = STATUS_PROCESS_IS_TERMINATING;
            goto Returns;
        }
    }

    if (WorkingSetMaximum > MmMaximumWorkingSetSize) {
        WorkingSetMaximum = MmMaximumWorkingSetSize;
        ReturnStatus = STATUS_WORKING_SET_LIMIT_RANGE;
    }

    if (WorkingSetMinimum > MmMaximumWorkingSetSize) {
        WorkingSetMinimum = MmMaximumWorkingSetSize;
        ReturnStatus = STATUS_WORKING_SET_LIMIT_RANGE;
    }

    if (WorkingSetMinimum < MmMinimumWorkingSetSize) {
        WorkingSetMinimum = (ULONG)MmMinimumWorkingSetSize;
        ReturnStatus = STATUS_WORKING_SET_LIMIT_RANGE;
    }

    //
    // Make sure that the number of locked pages will not
    // make the working set not fluid.
    //

    if ((WsInfo->VmWorkingSetList->FirstDynamic + MM_FLUID_WORKING_SET) >=
         WorkingSetMaximum) {
        ReturnStatus = STATUS_BAD_WORKING_SET_LIMIT;
        goto Returns;
    }

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    //
    // Check to make sure ample resident physical pages exist for
    // this operation.
    //

    LOCK_PFN (OldIrql);

    i = WorkingSetMinimum - WsInfo->MinimumWorkingSetSize;

    if (i > 0) {

        //
        // New minimum working set is greater than the old one.  Ensure that
        // we don't allow this process' working set minimum to increase to
        // a point where subsequent nonpaged pool allocations could cause
        // us to run out of pages.  Additionally, leave 100 extra pages around
        // so the user can later bring up tlist and kill processes if necessary.
        //

        if (MmAvailablePages < (20 + (i / (PAGE_SIZE / sizeof (MMWSLE))))) {
            UNLOCK_PFN (OldIrql);
            ReturnStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Returns;
        }

        if (MI_NONPAGABLE_MEMORY_AVAILABLE() - 100 < i) {
            UNLOCK_PFN (OldIrql);
            ReturnStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Returns;
        }
    }

    //
    // Adjust the number of resident pages up or down dependent on
    // the size of the new minimum working set size versus the previous
    // minimum size.
    //

    MmResidentAvailablePages -= i;
    MM_BUMP_COUNTER(27, i);

    UNLOCK_PFN (OldIrql);

    if (WsInfo->AllowWorkingSetAdjustment == FALSE) {
        MmAllowWorkingSetExpansion ();
    }

    if (WorkingSetMaximum > WorkingSetList->LastInitializedWsle) {

         do {

            //
            // The maximum size of the working set is being increased, check
            // to ensure the proper number of pages are mapped to cover
            // the complete working set list.
            //

            if (!MiAddWorkingSetPage (WsInfo)) {
                WorkingSetMaximum = WorkingSetList->LastInitializedWsle - 1;
                break;
            }
        } while (WorkingSetMaximum > WorkingSetList->LastInitializedWsle);

    } else {

        //
        // The new working set maximum is less than the current working set
        // maximum.
        //

        if (WsInfo->WorkingSetSize > WorkingSetMaximum) {

            //
            // Remove some pages from the working set.
            //

            //
            // Make sure that the number of locked pages will not
            // make the working set not fluid.
            //

            if ((WorkingSetList->FirstDynamic + MM_FLUID_WORKING_SET) >=
                 WorkingSetMaximum) {

                ReturnStatus = STATUS_BAD_WORKING_SET_LIMIT;

                LOCK_PFN (OldIrql);

                MmResidentAvailablePages += i;
                MM_BUMP_COUNTER(54, i);

                UNLOCK_PFN (OldIrql);

                goto Returns;
            }

            //
            // Attempt to remove the pages from the Maximum downward.
            //

            LastFreed = WorkingSetList->LastEntry;
            if (WorkingSetList->LastEntry > WorkingSetMaximum) {

                while (LastFreed >= WorkingSetMaximum) {

                    PointerPte = MiGetPteAddress(
                                        Wsle[LastFreed].u1.VirtualAddress);

                    PERFINFO_GET_PAGE_INFO(PointerPte);

                    if ((Wsle[LastFreed].u1.e1.Valid != 0) &&
                        (!MiFreeWsle (LastFreed,
                                      WsInfo,
                                      PointerPte))) {

                        //
                        // This LastFreed could not be removed.
                        //

                        break;
                    }
                    PERFINFO_LOG_WS_REMOVAL(PERFINFO_LOG_TYPE_OUTWS_ADJUSTWS, WsInfo);
                    LastFreed -= 1;
                }
                WorkingSetList->LastEntry = LastFreed;
            }

            //
            // Remove pages.
            //

            Entry = WorkingSetList->FirstDynamic;

            while (WsInfo->WorkingSetSize > WorkingSetMaximum) {
                if (Wsle[Entry].u1.e1.Valid != 0) {
                    PointerPte = MiGetPteAddress (
                                            Wsle[Entry].u1.VirtualAddress);
                    PERFINFO_GET_PAGE_INFO(PointerPte);

                    if (MiFreeWsle(Entry, WsInfo, PointerPte)) {
                        PERFINFO_LOG_WS_REMOVAL(PERFINFO_LOG_TYPE_OUTWS_ADJUSTWS,
                                              WsInfo);
                    }
                }
                Entry += 1;
                if (Entry > LastFreed) {
                    FreeTryCount += 1;
                    if (FreeTryCount > MM_RETRY_COUNT) {

                        //
                        // Page table pages are not becoming free, give up
                        // and return an error.
                        //

                        ReturnStatus = STATUS_BAD_WORKING_SET_LIMIT;

                        break;
                    }
                    Entry = WorkingSetList->FirstDynamic;
                }
            }

            if (FreeTryCount <= MM_RETRY_COUNT) {
                WorkingSetList->Quota = WorkingSetMaximum;
            }
        }
    }

    //
    // Adjust the number of pages above the working set minimum.
    //

    PagesAbove = (LONG)WsInfo->WorkingSetSize -
                               (LONG)WsInfo->MinimumWorkingSetSize;
    NewPagesAbove = (LONG)WsInfo->WorkingSetSize -
                               (LONG)WorkingSetMinimum;

    LOCK_PFN (OldIrql);
    if (PagesAbove > 0) {
        MmPagesAboveWsMinimum -= (ULONG)PagesAbove;
    }
    if (NewPagesAbove > 0) {
        MmPagesAboveWsMinimum += (ULONG)NewPagesAbove;
    }
    UNLOCK_PFN (OldIrql);

    if (FreeTryCount <= MM_RETRY_COUNT) {
        WsInfo->MaximumWorkingSetSize = WorkingSetMaximum;
        WsInfo->MinimumWorkingSetSize = WorkingSetMinimum;

        if (WorkingSetMinimum >= WorkingSetList->Quota) {
            WorkingSetList->Quota = WorkingSetMinimum;
        }
    }
    else {
        LOCK_PFN (OldIrql);

        MmResidentAvailablePages += i;
        MM_BUMP_COUNTER(55, i);

        UNLOCK_PFN (OldIrql);
    }


    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

    if ((WorkingSetList->HashTable == NULL) &&
        (WsInfo->MaximumWorkingSetSize > ((1536*1024) >> PAGE_SHIFT))) {

        //
        // The working set list consists of more than a single page.
        //

        MiGrowWsleHash (WsInfo);
    }

Returns:

    if (SystemCache) {
        UNLOCK_SYSTEM_WS (OldIrql2);
    } else {
        UNLOCK_WS (CurrentProcess);
    }

    MmUnlockPagableImageSection(ExPageLockHandle);

    return ReturnStatus;
}

ULONG
MiAddWorkingSetPage (
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This function grows the working set list above working set
    maximum during working set adjustment.  At most one page
    can be added at a time.

Arguments:

    None.

Return Value:

    Returns FALSE if no working set page could be added.

Environment:

    Kernel mode, APCs disabled, working set mutexes held.

--*/

{
    ULONG SwapEntry;
    ULONG CurrentEntry;
    PMMWSLE WslEntry;
    ULONG i;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE Va;
    MMPTE TempPte;
    WSLE_NUMBER NumberOfEntriesMapped;
    PFN_NUMBER WorkingSetPage;
    WSLE_NUMBER WorkingSetIndex;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    LOGICAL PageTablePageAllocated;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

#if DBG
    if (WsInfo == &MmSystemCacheWs) {
        MM_SYSTEM_WS_LOCK_ASSERT();
    }
#endif //DBG

    //
    // The maximum size of the working set is being increased, check
    // to ensure the proper number of pages are mapped to cover
    // the complete working set list.
    //

    PointerPte = MiGetPteAddress (&Wsle[WorkingSetList->LastInitializedWsle]);

    ASSERT (PointerPte->u.Hard.Valid == 1);

    PointerPte += 1;

    Va = (PMMPTE)MiGetVirtualAddressMappedByPte (PointerPte);

    if ((PVOID)Va >= WorkingSetList->HashTableStart) {

        //
        // Adding this entry would overrun the hash table.  The caller
        // must replace instead.
        //

        WorkingSetList->Quota = WorkingSetList->LastInitializedWsle;
        return FALSE;
    }

    PageTablePageAllocated = FALSE;

#if defined (_WIN64)
    PointerPde = MiGetPteAddress (PointerPte);
    if (PointerPde->u.Hard.Valid == 0) {

        ASSERT (WsInfo->u.Flags.SessionSpace == 0);

        //
        // Map in a new working set page.
        //
    
        LOCK_PFN (OldIrql);
        if (MmAvailablePages < 21) {
    
            //
            // No pages are available, set the quota to the last
            // initialized WSLE and return.
            //
    
            WorkingSetList->Quota = WorkingSetList->LastInitializedWsle;
            UNLOCK_PFN (OldIrql);
            return FALSE;
        }
    
        PageTablePageAllocated = TRUE;
        WorkingSetPage = MiRemoveZeroPage (MI_GET_PAGE_COLOR_FROM_PTE (PointerPde));
        PointerPde->u.Long = MM_DEMAND_ZERO_WRITE_PTE;
        MiInitializePfn (WorkingSetPage, PointerPde, 1);
        UNLOCK_PFN (OldIrql);
    
        MI_MAKE_VALID_PTE (TempPte,
                           WorkingSetPage,
                           MM_READWRITE,
                           PointerPde);
    
        MI_SET_PTE_DIRTY (TempPte);
        MI_WRITE_VALID_PTE (PointerPde, TempPte);
    
        //
        // Further down in this routine (once an actual working set page
        // has been allocated) the quota will be increased by 1 to reflect
        // the working set size entry for the new page table page.
        // The page table page will be put in a working set entry which will
        // be locked into the working set.
        //
    }
#endif

    ASSERT (PointerPte->u.Hard.Valid == 0);

    NumberOfEntriesMapped = (WSLE_NUMBER)(((PMMWSLE)((PCHAR)Va + PAGE_SIZE)) - Wsle);

    //
    // Map in a new working set page.
    //

    LOCK_PFN (OldIrql);
    if ((PageTablePageAllocated == FALSE) && (MmAvailablePages < 20)) {

        //
        // No pages are available, set the quota to the last
        // initialized WSLE and return.
        //

        WorkingSetList->Quota = WorkingSetList->LastInitializedWsle;
        UNLOCK_PFN (OldIrql);
        return FALSE;
    }

    WorkingSetPage = MiRemoveZeroPage (MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));
    PointerPte->u.Long = MM_DEMAND_ZERO_WRITE_PTE;
    MiInitializePfn (WorkingSetPage, PointerPte, 1);
    UNLOCK_PFN (OldIrql);

    MI_MAKE_VALID_PTE (TempPte,
                       WorkingSetPage,
                       MM_READWRITE,
                       PointerPte);

    MI_SET_PTE_DIRTY (TempPte);
    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    if (WsInfo->u.Flags.SessionSpace == 1) {
        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_WS_PAGE_ALLOC_GROWTH, 1);
        MmSessionSpace->NonPagablePages += 1;
        MmSessionSpace->CommittedPages += 1;
        MiChargeCommitmentCantExpand (1, TRUE);
        LOCK_PFN (OldIrql);
        MM_BUMP_COUNTER (48, 1);
        MmResidentAvailablePages -= 1;
        UNLOCK_PFN (OldIrql);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_ADDITIONAL_WS_PAGES, 1);
    }

    CurrentEntry = WorkingSetList->LastInitializedWsle + 1;

    ASSERT (NumberOfEntriesMapped > CurrentEntry);

    WslEntry = &Wsle[CurrentEntry - 1];

    for (i = CurrentEntry; i < NumberOfEntriesMapped; i += 1) {

        //
        // Build the free list, note that the first working
        // set entries (CurrentEntry) are not on the free list.
        // These entries are reserved for the pages which
        // map the working set and the page which contains the PDE.
        //

        WslEntry += 1;
        WslEntry->u1.Long = (i + 1) << MM_FREE_WSLE_SHIFT;
    }

    WslEntry->u1.Long = WorkingSetList->FirstFree << MM_FREE_WSLE_SHIFT;

    ASSERT (CurrentEntry >= WorkingSetList->FirstDynamic);

    WorkingSetList->FirstFree = CurrentEntry;

    WorkingSetList->LastInitializedWsle = (NumberOfEntriesMapped - 1);

    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

    //
    // As we are growing the working set, make sure the quota is
    // above the working set size by adding 1 to the quota.
    //

    WorkingSetList->Quota += 1;

    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    CONSISTENCY_LOCK_PFN (OldIrql);

    Pfn1->u1.Event = (PVOID)PsGetCurrentThread();

    CONSISTENCY_UNLOCK_PFN (OldIrql);

    //
    // Get a working set entry.
    //

    WsInfo->WorkingSetSize += 1;

    ASSERT (WorkingSetList->FirstFree != WSLE_NULL_INDEX);
    ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);

    WorkingSetIndex = WorkingSetList->FirstFree;
    WorkingSetList->FirstFree = (WSLE_NUMBER)(Wsle[WorkingSetIndex].u1.Long >> MM_FREE_WSLE_SHIFT);
    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

    if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
        MmPagesAboveWsMinimum += 1;
    }
    if (WorkingSetIndex > WorkingSetList->LastEntry) {
        WorkingSetList->LastEntry = WorkingSetIndex;
    }

    MiUpdateWsle (&WorkingSetIndex, Va, WorkingSetList, Pfn1);

    MI_SET_PTE_IN_WORKING_SET (PointerPte, WorkingSetIndex);

    //
    // Lock any created page table pages into the working set.
    //

    if (WorkingSetIndex >= WorkingSetList->FirstDynamic) {

        SwapEntry = WorkingSetList->FirstDynamic;

        if (WorkingSetIndex != WorkingSetList->FirstDynamic) {

            //
            // Swap this entry with the one at first dynamic.
            //

            MiSwapWslEntries (WorkingSetIndex, SwapEntry, WsInfo);
        }

        WorkingSetList->FirstDynamic += 1;

        Wsle[SwapEntry].u1.e1.LockedInWs = 1;
        ASSERT (Wsle[SwapEntry].u1.e1.Valid == 1);
    }

#if defined (_WIN64)
    if (PageTablePageAllocated == TRUE) {
    
        //
        // As we are growing the working set, make sure the quota is
        // above the working set size by adding 1 to the quota.
        //
    
        WorkingSetList->Quota += 1;
    
        Pfn1 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);
    
        CONSISTENCY_LOCK_PFN (OldIrql);
    
        Pfn1->u1.Event = (PVOID)PsGetCurrentThread();
    
        CONSISTENCY_UNLOCK_PFN (OldIrql);
    
        //
        // Get a working set entry.
        //
    
        WsInfo->WorkingSetSize += 1;
    
        ASSERT (WorkingSetList->FirstFree != WSLE_NULL_INDEX);
        ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);
    
        WorkingSetIndex = WorkingSetList->FirstFree;
        WorkingSetList->FirstFree = (WSLE_NUMBER)(Wsle[WorkingSetIndex].u1.Long >> MM_FREE_WSLE_SHIFT);
        ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
                (WorkingSetList->FirstFree == WSLE_NULL_INDEX));
    
        if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
            MmPagesAboveWsMinimum += 1;
        }
        if (WorkingSetIndex > WorkingSetList->LastEntry) {
            WorkingSetList->LastEntry = WorkingSetIndex;
        }
    
        MiUpdateWsle (&WorkingSetIndex, PointerPte, WorkingSetList, Pfn1);
    
        MI_SET_PTE_IN_WORKING_SET (PointerPde, WorkingSetIndex);
    
        //
        // Lock the created page table page into the working set.
        //
    
        if (WorkingSetIndex >= WorkingSetList->FirstDynamic) {
    
            SwapEntry = WorkingSetList->FirstDynamic;
    
            if (WorkingSetIndex != WorkingSetList->FirstDynamic) {
    
                //
                // Swap this entry with the one at first dynamic.
                //
    
                MiSwapWslEntries (WorkingSetIndex, SwapEntry, WsInfo);
            }
    
            WorkingSetList->FirstDynamic += 1;
    
            Wsle[SwapEntry].u1.e1.LockedInWs = 1;
            ASSERT (Wsle[SwapEntry].u1.e1.Valid == 1);
        }
    }
#endif

    ASSERT ((MiGetPteAddress(&Wsle[WorkingSetList->LastInitializedWsle]))->u.Hard.Valid == 1);

    if ((WorkingSetList->HashTable == NULL) &&
        (MmAvailablePages > 20)) {

        //
        // Add a hash table to support shared pages in the working set to
        // eliminate costly lookups.
        //

        LOCK_EXPANSION_IF_ALPHA (OldIrql);
        ASSERT (WsInfo->AllowWorkingSetAdjustment != FALSE);
        WsInfo->AllowWorkingSetAdjustment = MM_GROW_WSLE_HASH;
        UNLOCK_EXPANSION_IF_ALPHA (OldIrql);
    }

    ASSERT (WsInfo->WorkingSetSize <= WorkingSetList->Quota);
    return TRUE;
}

LOGICAL
MiAddWsleHash (
    IN PMMSUPPORT WsInfo,
    IN PMMPTE PointerPte
    )

/*++

Routine Description:

    This function adds a page directory, page table or actual mapping page
    for hash table creation (or expansion) for the current process.

Arguments:

    WsInfo - Supplies a pointer to the working set info block for the
             process (or system cache).

    PointerPte - Supplies a pointer to the PTE to be filled.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set lock held.

--*/
{
    KIRQL OldIrql;
    PMMPFN Pfn1;
    ULONG SwapEntry;
    MMPTE TempPte;
    PVOID Va;
    PMMWSLE Wsle;
    PFN_NUMBER WorkingSetPage;
    WSLE_NUMBER WorkingSetIndex;
    PMMWSL WorkingSetList;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    ASSERT (PointerPte->u.Hard.Valid == 0);

    LOCK_PFN (OldIrql);

    if (MmAvailablePages < 10) {
        UNLOCK_PFN (OldIrql);
        return FALSE;
    }

    WorkingSetPage = MiRemoveZeroPage (
                                MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    PointerPte->u.Long = MM_DEMAND_ZERO_WRITE_PTE;
    MiInitializePfn (WorkingSetPage, PointerPte, 1);

    MI_MAKE_VALID_PTE (TempPte,
                       WorkingSetPage,
                       MM_READWRITE,
                       PointerPte );

    MI_SET_PTE_DIRTY (TempPte);
    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    UNLOCK_PFN (OldIrql);

    //
    // As we are growing the working set, we know that quota
    // is above the current working set size.  Just take the
    // next free WSLE from the list and use it.
    //

    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    CONSISTENCY_LOCK_PFN (OldIrql);

    Pfn1->u1.Event = (PVOID)PsGetCurrentThread();

    CONSISTENCY_UNLOCK_PFN (OldIrql);

    Va = (PMMPTE)MiGetVirtualAddressMappedByPte (PointerPte);

    WorkingSetIndex = MiLocateAndReserveWsle (WsInfo);
    MiUpdateWsle (&WorkingSetIndex, Va, WorkingSetList, Pfn1);
    MI_SET_PTE_IN_WORKING_SET (PointerPte, WorkingSetIndex);

    //
    // Lock any created page table pages into the working set.
    //

    if (WorkingSetIndex >= WorkingSetList->FirstDynamic) {

        SwapEntry = WorkingSetList->FirstDynamic;

        if (WorkingSetIndex != WorkingSetList->FirstDynamic) {

            //
            // Swap this entry with the one at first dynamic.
            //

            MiSwapWslEntries (WorkingSetIndex, SwapEntry, WsInfo);
        }

        WorkingSetList->FirstDynamic += 1;

        Wsle[SwapEntry].u1.e1.LockedInWs = 1;
        ASSERT (Wsle[SwapEntry].u1.e1.Valid == 1);
    }

    if (WsInfo->u.Flags.SessionSpace == 1) {
        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_WS_HASHPAGE_ALLOC, 1);
        MmSessionSpace->NonPagablePages += 1;
        MmSessionSpace->CommittedPages += 1;
        MiChargeCommitmentCantExpand (1, TRUE);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_ADDITIONAL_WS_HASHPAGES, 1);
    }
    return TRUE;
}

VOID
MiGrowWsleHash (
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This function grows (or adds) a hash table to the working set list
    to allow direct indexing for WSLEs than cannot be located via the
    PFN database WSINDEX field.

    The hash table is located AFTER the WSLE array and the pages are
    locked into the working set just like standard WSLEs.

    Note that the hash table is expanded by setting the hash table
    field in the working set to NULL, but leaving the size as non-zero.
    This indicates that the hash should be expanded and the initial
    portion of the table zeroed.

Arguments:

    WsInfo - Supplies a pointer to the working set info block for the
             process (or system cache).

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set lock held.

--*/
{
    LONG Size;
    PMMWSLE Wsle;
    PMMPTE StartPte;
    PMMPTE EndPte;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE AllocatedPde;
    PMMPTE AllocatedPpe;
    ULONG First;
    ULONG Hash;
    ULONG NewSize;
    PMMWSLE_HASH Table;
    PMMWSLE_HASH OriginalTable;
    ULONG j;
    PMMWSL WorkingSetList;
    KIRQL OldIrql;
    ULONG Count;
    LOGICAL LoopStart;
    PVOID EntryHashTableEnd;
    PVOID TempVa;
    PEPROCESS CurrentProcess;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    Table = WorkingSetList->HashTable;
    OriginalTable = WorkingSetList->HashTable;

    First = WorkingSetList->HashTableSize;

    if (Table == NULL) {

        NewSize = PtrToUlong(PAGE_ALIGN (((1 + WorkingSetList->NonDirectCount) *
                            2 * sizeof(MMWSLE_HASH)) + PAGE_SIZE - 1));

        //
        // Note that the Table may be NULL and the HashTableSize/PTEs nonzero
        // in the case where the hash has been contracted.
        //

        j = First * sizeof(MMWSLE_HASH);

        //
        // Don't try for additional hash pages if we already have
        // the right amount (or too many).
        //

        if ((j + PAGE_SIZE > NewSize) && (j != 0)) {
            return;
        }

        Table = (PMMWSLE_HASH)(WorkingSetList->HashTableStart);
        EntryHashTableEnd = &Table[WorkingSetList->HashTableSize];

        WorkingSetList->HashTableSize = 0;

    } else {

        //
        // Attempt to add 4 pages, make sure the working set list has
        // 4 free entries.
        //

        if ((WorkingSetList->LastInitializedWsle + 5) > WsInfo->WorkingSetSize) {
            NewSize = PAGE_SIZE * 4;
        } else {
            NewSize = PAGE_SIZE;
        }
        EntryHashTableEnd = &Table[WorkingSetList->HashTableSize];
    }

    if ((PCHAR)EntryHashTableEnd + NewSize > (PCHAR)WorkingSetList->HighestPermittedHashAddress) {
        NewSize =
            (ULONG)((PCHAR)(WorkingSetList->HighestPermittedHashAddress) -
                ((PCHAR)EntryHashTableEnd));
        if (NewSize == 0) {
            if (OriginalTable == NULL) {
                WorkingSetList->HashTableSize = First;
            }
            return;
        }
    }

    ASSERT64 ((MiGetPpeAddress(EntryHashTableEnd)->u.Hard.Valid == 0) ||
              (MiGetPdeAddress(EntryHashTableEnd)->u.Hard.Valid == 0) ||
              (MiGetPteAddress(EntryHashTableEnd)->u.Hard.Valid == 0));

    ASSERT32 (MiGetPteAddress(EntryHashTableEnd)->u.Hard.Valid == 0);

    Size = NewSize;
    PointerPte = MiGetPteAddress (EntryHashTableEnd);
    StartPte = PointerPte;
    EndPte = PointerPte + (NewSize >> PAGE_SHIFT);

#if defined (_WIN64)
    LoopStart = TRUE;
    AllocatedPde = NULL;
    AllocatedPpe = NULL;
#endif

    do {

#if defined (_WIN64)
        if (LoopStart == TRUE || MiIsPteOnPdeBoundary(PointerPte)) {

            PointerPpe = MiGetPdeAddress(PointerPte);
            PointerPde = MiGetPteAddress(PointerPte);

            if (PointerPpe->u.Hard.Valid == 0) {
                if (MiAddWsleHash (WsInfo, PointerPpe) == FALSE) {
                    break;
                }
                AllocatedPpe = PointerPpe;
            }

            if (PointerPde->u.Hard.Valid == 0) {
                if (MiAddWsleHash (WsInfo, PointerPde) == FALSE) {
                    break;
                }
                AllocatedPde = PointerPde;
            }

            LoopStart = FALSE;
        }
        else {
            AllocatedPde = NULL;
            AllocatedPpe = NULL;
        }
#endif

        if (PointerPte->u.Hard.Valid == 0) {
            if (MiAddWsleHash (WsInfo, PointerPte) == FALSE) {
                break;
            }
        }

        PointerPte += 1;
        Size -= PAGE_SIZE;
    } while (Size > 0);

    //
    // If MiAddWsleHash was unable to allocate memory above, then roll back
    // any extra PPEs & PDEs that may have been created.  Note NewSize must
    // be recalculated to handle the fact that memory may have run out.
    //

#if !defined (_WIN64)
    if (PointerPte == StartPte) {
        if (OriginalTable == NULL) {
            WorkingSetList->HashTableSize = First;
        }
        return;
    }
#else
    if (PointerPte != EndPte) {

        //
        // Clean up the last allocated PPE/PDE as they are not needed.
        // Note that the system cache and the session space working sets
        // have no current process (which MiDeletePte requires) which is
        // needed for WSLE and PrivatePages adjustments.
        //

        if (WsInfo != &MmSystemCacheWs && WsInfo->u.Flags.SessionSpace == 0) {
            CurrentProcess = PsGetCurrentProcess();
    
            if (AllocatedPde != NULL) {
                ASSERT (AllocatedPde->u.Hard.Valid == 1);
                TempVa = MiGetVirtualAddressMappedByPte(AllocatedPde);
                LOCK_PFN (OldIrql);
                MiDeletePte (AllocatedPde,
                             TempVa,
                             FALSE,
                             CurrentProcess,
                             NULL,
                             NULL);
                //
                // Add back in the private page MiDeletePte subtracted.
                //

                CurrentProcess->NumberOfPrivatePages += 1;
                UNLOCK_PFN (OldIrql);
            }
    
            if (AllocatedPpe != NULL) {
                ASSERT (AllocatedPpe->u.Hard.Valid == 1);
                TempVa = MiGetVirtualAddressMappedByPte(AllocatedPpe);
                LOCK_PFN (OldIrql);
                MiDeletePte (AllocatedPpe,
                             TempVa,
                             FALSE,
                             CurrentProcess,
                             NULL,
                             NULL);
                //
                // Add back in the private page MiDeletePte subtracted.
                //

                CurrentProcess->NumberOfPrivatePages += 1;
                UNLOCK_PFN (OldIrql);
            }
        }

        if (PointerPte == StartPte) {
            if (OriginalTable == NULL) {
                WorkingSetList->HashTableSize = First;
            }
        }

        return;
    }
#endif

    NewSize = (ULONG)((PointerPte - StartPte) << PAGE_SHIFT);

    ASSERT ((MiGetVirtualAddressMappedByPte(PointerPte) == WorkingSetList->HighestPermittedHashAddress) ||
            (PointerPte->u.Hard.Valid == 0));

    WorkingSetList->HashTableSize = First + NewSize / sizeof (MMWSLE_HASH);
    WorkingSetList->HashTable = Table;

    ASSERT ((&Table[WorkingSetList->HashTableSize] == WorkingSetList->HighestPermittedHashAddress) ||
        (MiGetPteAddress(&Table[WorkingSetList->HashTableSize])->u.Hard.Valid == 0));

    if (First != 0) {
        RtlZeroMemory (Table, First * sizeof(MMWSLE_HASH));
    }

    //
    // Fill hash table
    //

    j = 0;
    Count = WorkingSetList->NonDirectCount;

    Size = WorkingSetList->HashTableSize;

    do {
        if ((Wsle[j].u1.e1.Valid == 1) &&
            (Wsle[j].u1.e1.Direct == 0)) {

            //
            // Hash this.
            //

            Count -= 1;

            Hash = MI_WSLE_HASH(Wsle[j].u1.Long, WorkingSetList);

            while (Table[Hash].Key != 0) {
                Hash += 1;
                if (Hash >= (ULONG)Size) {
                    Hash = 0;
                }
            }

            Table[Hash].Key = Wsle[j].u1.Long & ~(PAGE_SIZE - 1);
            Table[Hash].Index = j;
#if DBG
            PointerPte = MiGetPteAddress(Wsle[j].u1.VirtualAddress);
            ASSERT (PointerPte->u.Hard.Valid);
#endif //DBG

        }
        ASSERT (j <= WorkingSetList->LastEntry);
        j += 1;
    } while (Count);

#if DBG
    MiCheckWsleHash (WorkingSetList);
#endif //DBG
    return;
}


ULONG
MiTrimWorkingSet (
    ULONG Reduction,
    IN PMMSUPPORT WsInfo,
    IN ULONG ForcedReductionOrTrimAge
    )

/*++

Routine Description:

    This function reduces the working set by the specified amount.

Arguments:

    Reduction - Supplies the number of pages to remove from the working
                set.

    WsInfo - Supplies a pointer to the working set information for the
             process (or system cache) to trim.

    ForcedReductionOrTrimAge - If using fault-based trimming, this is set to
                               TRUE if the reduction is being done to free up
                               pages in which case we should try to reduce
                               working set pages as well.  Set to FALSE when
                               the reduction is trying to increase the fault
                               rates in which case the policy should be more
                               like locate and reserve.

                               If using claim-based trimming, this is the age
                               value to use - ie: pages of this age or older
                               will be removed.

Return Value:

    Returns the actual number of pages removed.

Environment:

    Kernel mode, APCs disabled, working set lock.  PFN lock NOT held.

--*/

{
    ULONG TryToFree;
    ULONG StartEntry;
    ULONG LastEntry;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    PMMPTE PointerPte;
    ULONG NumberLeftToRemove;
    ULONG LoopCount;
    ULONG EndCount;
    BOOLEAN StartFromZero;

    NumberLeftToRemove = Reduction;
    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

#if DBG
    if (WsInfo == &MmSystemCacheWs) {
        MM_SYSTEM_WS_LOCK_ASSERT();
    }
#endif //DBG

    LastEntry = WorkingSetList->LastEntry;

    TryToFree = WorkingSetList->NextSlot;
    if (TryToFree > LastEntry || TryToFree < WorkingSetList->FirstDynamic) {
        TryToFree = WorkingSetList->FirstDynamic;
    }

    StartEntry = TryToFree;

#ifdef _MI_USE_CLAIMS_

    while (NumberLeftToRemove != 0) {
        if (Wsle[TryToFree].u1.e1.Valid == 1) {
            PointerPte = MiGetPteAddress (Wsle[TryToFree].u1.VirtualAddress);

            if ((ForcedReductionOrTrimAge == 0) ||
                ((MI_GET_ACCESSED_IN_PTE (PointerPte) == 0) &&
                (MI_GET_WSLE_AGE(PointerPte, &Wsle[TryToFree]) >= ForcedReductionOrTrimAge))) {

                PERFINFO_GET_PAGE_INFO_WITH_DECL(PointerPte);

                if (MiFreeWsle (TryToFree, WsInfo, PointerPte)) {
                    PERFINFO_LOG_WS_REMOVAL(PERFINFO_LOG_TYPE_OUTWS_VOLUNTRIM, WsInfo);
                    NumberLeftToRemove -= 1;
                }
            }
        }
        TryToFree += 1;

        if (TryToFree > LastEntry) {
            TryToFree = WorkingSetList->FirstDynamic;
        }

        if (TryToFree == StartEntry) {
            break;
        }
    }

#else

    LoopCount = 0;

    if (ForcedReductionOrTrimAge) {
        EndCount = 4;
    } else {
        EndCount = 1;
    }

    StartFromZero = FALSE;

    while (NumberLeftToRemove != 0 && LoopCount != EndCount) {
        while ((NumberLeftToRemove != 0) && (TryToFree <= LastEntry)) {

            if (Wsle[TryToFree].u1.e1.Valid == 1) {
                PointerPte = MiGetPteAddress (Wsle[TryToFree].u1.VirtualAddress);
                if (MI_GET_ACCESSED_IN_PTE (PointerPte)) {

                    //
                    // If accessed bit is set, clear it.  If accessed
                    // bit is clear, remove from working set.
                    //

                    MI_SET_ACCESSED_IN_PTE (PointerPte, 0);
                } else {
                    PERFINFO_GET_PAGE_INFO_WITH_DECL(PointerPte);
                    if (MiFreeWsle (TryToFree, WsInfo, PointerPte)) {
                        PERFINFO_LOG_WS_REMOVAL(PERFINFO_LOG_TYPE_OUTWS_VOLUNTRIM, WsInfo);
                        NumberLeftToRemove -= 1;
                    }
                }
            }
            TryToFree += 1;

            if (StartFromZero == TRUE && EndCount == 1 && TryToFree >= StartEntry) {
                LoopCount = EndCount;
                if (TryToFree > LastEntry) {
                    TryToFree = WorkingSetList->FirstDynamic;
                }
                break;
            }
        }

        if (TryToFree > LastEntry) {
            TryToFree = WorkingSetList->FirstDynamic;
            if (StartFromZero == TRUE) {

                //
                // We've already wrapped once but didn't get back to
                // the StartEntry.  Use the first dynamic as our base now.
                //

                StartEntry = TryToFree;
            }
            else {
                StartFromZero = TRUE;
            }

            if (TryToFree >= StartEntry) {

                //
                // We've wrapped. If this is not a forced trim, then bail
                // now so we don't cannibalize entries we just cleared the
                // access bit for because they haven't had a fair chance to
                // be re-accessed yet.
                //

                LoopCount += 1;
                StartFromZero = FALSE;
            }
        }
    }

#endif

    WorkingSetList->NextSlot = TryToFree;

    //
    // If this is not the system cache or a session working set, see if the
    // working set list can be contracted.
    //

    if (WsInfo != &MmSystemCacheWs && WsInfo->u.Flags.SessionSpace == 0) {

        //
        // Make sure we are at least a page above the working set maximum.
        //

        if (WorkingSetList->FirstDynamic == WsInfo->WorkingSetSize) {
                MiRemoveWorkingSetPages (WorkingSetList, WsInfo);
        } else {

            if ((WorkingSetList->Quota + 15 + (PAGE_SIZE / sizeof(MMWSLE))) <
                                                    WorkingSetList->LastEntry) {
                if ((WsInfo->MaximumWorkingSetSize + 15 + (PAGE_SIZE / sizeof(MMWSLE))) <
                     WorkingSetList->LastEntry ) {
                    MiRemoveWorkingSetPages (WorkingSetList, WsInfo);
                }
            }
        }
    }
    return Reduction - NumberLeftToRemove;
}

#if 0 //COMMENTED OUT.
VOID
MmPurgeWorkingSet (
     IN PEPROCESS Process,
     IN PVOID BaseAddress,
     IN ULONG RegionSize
     )

/*++

Routine Description:

    This function removes any valid pages with a reference count
    of 1 within the specified address range of the specified process.

    If the address range is within the system cache, the process
    parameter is ignored.

Arguments:

    Process - Supplies a pointer to the process to operate upon.

    BaseAddress - Supplies the base address of the range to operate upon.

    RegionSize - Supplies the size of the region to operate upon.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL or below.

--*/

{
    PMMSUPPORT WsInfo;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE LastPte;
    PMMPFN Pfn1;
    MMPTE PteContents;
    PEPROCESS CurrentProcess;
    PVOID EndingAddress;
    ULONG SystemCache;
    KIRQL OldIrql;

    //
    // Determine if the specified base address is within the system
    // cache and if so, don't attach, the working set lock is still
    // required to "lock" paged pool pages (proto PTEs) into the
    // working set.
    //

    CurrentProcess = PsGetCurrentProcess ();

    ASSERT (RegionSize != 0);

    EndingAddress = (PVOID)((PCHAR)BaseAddress + RegionSize - 1);

    if ((BaseAddress <= MM_HIGHEST_USER_ADDRESS) ||
        ((BaseAddress >= (PVOID)PTE_BASE) &&
         (BaseAddress < (PVOID)MM_SYSTEM_SPACE_START)) ||
        ((BaseAddress >= MM_PAGED_POOL_START) &&
         (BaseAddress <= MmPagedPoolEnd))) {

        SystemCache = FALSE;

        //
        // Attach to the specified process.
        //

        KeAttachProcess (&Process->Pcb);

        WsInfo = &Process->Vm,

        LOCK_WS (Process);
    } else {

        SystemCache = TRUE;
        Process = CurrentProcess;
        WsInfo = &MmSystemCacheWs;
    }

    PointerPde = MiGetPdeAddress (BaseAddress);
    PointerPte = MiGetPteAddress (BaseAddress);
    LastPte = MiGetPteAddress (EndingAddress);

    while (!MiDoesPdeExistAndMakeValid(PointerPde, Process, FALSE)) {

        //
        // No page table page exists for this address.
        //

        PointerPde += 1;

        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);

        if (PointerPte > LastPte) {
            break;
        }
    }

    LOCK_PFN (OldIrql);

    while (PointerPte <= LastPte) {

        PteContents = *PointerPte;

        if (PteContents.u.Hard.Valid == 1) {

            //
            // Remove this page from the working set.
            //

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

            if (Pfn1->u3.e2.ReferenceCount == 1) {
                MiRemovePageFromWorkingSet (PointerPte, Pfn1, WsInfo);
            }
        }

        PointerPte += 1;

        if (((ULONG_PTR)PointerPte & (PAGE_SIZE - 1)) == 0) {

            PointerPde = MiGetPteAddress (PointerPte);

            while ((PointerPte <= LastPte) &&
                   (!MiDoesPdeExistAndMakeValid(PointerPde, Process, TRUE))) {

                //
                // No page table page exists for this address.
                //

                PointerPde += 1;

                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
            }
        }
    }

    UNLOCK_PFN (OldIrql);

    if (!SystemCache) {

        UNLOCK_WS (Process);
        KeDetachProcess();
    }
    return;
}
#endif //0

VOID
MiEliminateWorkingSetEntry (
    IN ULONG WorkingSetIndex,
    IN PMMPTE PointerPte,
    IN PMMPFN Pfn,
    IN PMMWSLE Wsle
    )

/*++

Routine Description:

    This routine removes the specified working set list entry
    from the working set, flushes the TB for the page, decrements
    the share count for the physical page, and, if necessary turns
    the PTE into a transition PTE.

Arguments:

    WorkingSetIndex - Supplies the working set index to remove.

    PointerPte - Supplies a pointer to the PTE corresponding to the virtual
                 address in the working set.

    Pfn - Supplies a pointer to the PFN element corresponding to the PTE.

    Wsle - Supplies a pointer to the first working set list entry for this
           working set.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held, APCs disabled.

--*/

{
    PMMPTE ContainingPageTablePage;
    MMPTE TempPte;
    MMPTE PreviousPte;
    PFN_NUMBER PageFrameIndex;
    PEPROCESS Process;
    PVOID VirtualAddress;

    //
    // Remove the page from the working set.
    //

    MM_PFN_LOCK_ASSERT ();

    TempPte = *PointerPte;
    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&TempPte);

#ifdef _X86_
#if DBG
#if !defined(NT_UP)
    if (TempPte.u.Hard.Writable == 1) {
        ASSERT (TempPte.u.Hard.Dirty == 1);
    }
    ASSERT (TempPte.u.Hard.Accessed == 1);
#endif //NTUP
#endif //DBG
#endif //X86

    MI_MAKING_VALID_PTE_INVALID (FALSE);

    if (Pfn->u3.e1.PrototypePte) {

        //
        // This is a prototype PTE.  The PFN database does not contain
        // the contents of this PTE it contains the contents of the
        // prototype PTE.  This PTE must be reconstructed to contain
        // a pointer to the prototype PTE.
        //
        // The working set list entry contains information about
        // how to reconstruct the PTE.
        //

        if (Wsle[WorkingSetIndex].u1.e1.SameProtectAsProto == 0) {

            //
            // The protection for the prototype PTE is in the
            // WSLE.
            //

            ASSERT (Wsle[WorkingSetIndex].u1.e1.Protection != 0);

            if (!MI_IS_SESSION_IMAGE_ADDRESS (MiGetVirtualAddressMappedByPte(PointerPte))) {
                TempPte.u.Long = 0;
                TempPte.u.Soft.Protection =
                    MI_GET_PROTECTION_FROM_WSLE (&Wsle[WorkingSetIndex]);
                TempPte.u.Soft.PageFileHigh = MI_PTE_LOOKUP_NEEDED;
            }
            else {

                //
                // The session PTE protection must be carefully preserved.
                //
    
                TempPte.u.Long = MiProtoAddressForPte (Pfn->PteAddress);
            }
        } else {

            //
            // The protection is in the prototype PTE.
            //

            TempPte.u.Long = MiProtoAddressForPte (Pfn->PteAddress);

            if (MI_IS_SESSION_IMAGE_ADDRESS (MiGetVirtualAddressMappedByPte(PointerPte))) {
    
                //
                // The session PTE protection must be carefully preserved.
                //
    
                TempPte.u.Proto.ReadOnly = 1;
            }
        }
    
        TempPte.u.Proto.Prototype = 1;

        //
        // Decrement the share count of the containing page table
        // page as the PTE for the removed page is no longer valid
        // or in transition
        //

        ContainingPageTablePage = MiGetPteAddress (PointerPte);
#if defined (_WIN64)
        ASSERT (ContainingPageTablePage->u.Hard.Valid == 1);
#else
        if (ContainingPageTablePage->u.Hard.Valid == 0) {
            if (!NT_SUCCESS(MiCheckPdeForPagedPool (PointerPte))) {
                KeBugCheckEx (MEMORY_MANAGEMENT,
                              0x61940, 
                              (ULONG_PTR)PointerPte,
                              (ULONG_PTR)ContainingPageTablePage->u.Long,
                              (ULONG_PTR)MiGetVirtualAddressMappedByPte(PointerPte));
            }
        }
#endif
        MiDecrementShareAndValidCount (MI_GET_PAGE_FRAME_FROM_PTE (ContainingPageTablePage));

    } else {

        //
        // This is a private page, make it transition.
        //

        //
        // Assert that the share count is 1 for all user mode pages.
        //

        ASSERT ((Pfn->u2.ShareCount == 1) ||
                (Wsle[WorkingSetIndex].u1.VirtualAddress >
                        (PVOID)MM_HIGHEST_USER_ADDRESS));

        //
        // Set the working set index to zero.  This allows page table
        // pages to be brought back in with the proper WSINDEX.
        //

        ASSERT (Pfn->u1.WsIndex != 0);
        MI_ZERO_WSINDEX (Pfn);
        MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                      Pfn->OriginalPte.u.Soft.Protection);
    }

    if (Wsle == MmWsle) {
        PreviousPte.u.Flush = KeFlushSingleTb (
                                    Wsle[WorkingSetIndex].u1.VirtualAddress,
                                    TRUE,
                                    FALSE,
                                    (PHARDWARE_PTE)PointerPte,
                                    TempPte.u.Flush);
    }
    else if (Wsle == MmSystemCacheWsle) {

        //
        // Must be the system cache.
        //

        PreviousPte.u.Flush = KeFlushSingleTb (
                                    Wsle[WorkingSetIndex].u1.VirtualAddress,
                                    TRUE,
                                    TRUE,
                                    (PHARDWARE_PTE)PointerPte,
                                    TempPte.u.Flush);
    }
    else {

        //
        // Must be a session space.
        //

        MI_FLUSH_SINGLE_SESSION_TB (Wsle[WorkingSetIndex].u1.VirtualAddress,
                                    TRUE,
                                    FALSE,
                                    (PHARDWARE_PTE)PointerPte,
                                    TempPte.u.Flush,
                                    PreviousPte);
    }

    ASSERT (PreviousPte.u.Hard.Valid == 1);

    //
    // A page is being removed from the working set, on certain
    // hardware the dirty bit should be ORed into the modify bit in
    // the PFN element.
    //

    MI_CAPTURE_DIRTY_BIT_TO_PFN (&PreviousPte, Pfn);

    //
    // If the PTE indicates the page has been modified (this is different
    // from the PFN indicating this), then ripple it back to the write watch
    // bitmap now since we are still in the correct process context.
    //

    if (MiActiveWriteWatch != 0) {
        if ((Pfn->u3.e1.PrototypePte == 0) &&
            (MI_IS_PTE_DIRTY(PreviousPte))) {

            Process = PsGetCurrentProcess();

            if (Process->Vm.u.Flags.WriteWatch == 1) {

                //
                // This process has (or had) write watch VADs.  Search now
                // for a write watch region encapsulating the PTE being
                // invalidated.
                //

                VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                MiCaptureWriteWatchDirtyBit (Process, VirtualAddress);
            }
        }
    }

    //
    // Flush the translation buffer and decrement the number of valid
    // PTEs within the containing page table page.  Note that for a
    // private page, the page table page is still needed because the
    // page is in transition.
    //

    MiDecrementShareCount (PageFrameIndex);

    return;
}

VOID
MiRemoveWorkingSetPages (
    IN PMMWSL WorkingSetList,
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This routine compresses the WSLEs into the front of the working set
    and frees the pages for unneeded working set entries.

Arguments:

    WorkingSetList - Supplies a pointer to the working set list to compress.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock held, APCs disabled.

--*/

{
    PMMWSLE FreeEntry;
    PMMWSLE LastEntry;
    PMMWSLE Wsle;
    ULONG FreeIndex;
    ULONG LastIndex;
    ULONG LastInvalid;
    PMMPTE LastPte;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE WsPte;
    PMMPFN Pfn1;
    PEPROCESS CurrentProcess;
    MMPTE_FLUSH_LIST PteFlushList;
    ULONG NewSize;
    PMMWSLE_HASH Table;
    KIRQL OldIrql;

    ASSERT (WsInfo != &MmSystemCacheWs && WsInfo->u.Flags.SessionSpace == 0);

    PteFlushList.Count = 0;
    CurrentProcess = PsGetCurrentProcess();

#if DBG
    MiCheckNullIndex (WorkingSetList);
#endif //DBG

    //
    // Check to see if the wsle hash table should be contracted.
    //

    if (WorkingSetList->HashTable) {

        Table = WorkingSetList->HashTable;

#if DBG
        if ((PVOID)(&Table[WorkingSetList->HashTableSize]) < WorkingSetList->HighestPermittedHashAddress) {
            ASSERT (MiGetPteAddress(&Table[WorkingSetList->HashTableSize])->u.Hard.Valid == 0);
        }
#endif

        if (WsInfo->WorkingSetSize < 200) {
            NewSize = 0;
        }
        else {
            NewSize = PtrToUlong(PAGE_ALIGN ((WorkingSetList->NonDirectCount * 2 *
                                       sizeof(MMWSLE_HASH)) + PAGE_SIZE - 1));
    
            NewSize = NewSize / sizeof(MMWSLE_HASH);
        }

        if (NewSize < WorkingSetList->HashTableSize) {

#if defined(_ALPHA_) && !defined(_AXP64_)
            LOCK_EXPANSION_IF_ALPHA (OldIrql);
#endif
            if (NewSize && WsInfo->AllowWorkingSetAdjustment) {
                WsInfo->AllowWorkingSetAdjustment = MM_GROW_WSLE_HASH;
            }
#if defined(_ALPHA_) && !defined(_AXP64_)
            UNLOCK_EXPANSION_IF_ALPHA (OldIrql);
#endif

            //
            // Remove pages from hash table.
            //

            ASSERT (((ULONG_PTR)&WorkingSetList->HashTable[NewSize] &
                                                    (PAGE_SIZE - 1)) == 0);

            PointerPte = MiGetPteAddress (&WorkingSetList->HashTable[NewSize]);

            LastPte = MiGetPteAddress (WorkingSetList->HighestPermittedHashAddress);
            //
            // Set the hash table to null indicating that no hashing
            // is going on.
            //

            WorkingSetList->HashTable = NULL;
            WorkingSetList->HashTableSize = NewSize;

            LOCK_PFN (OldIrql);
            while ((PointerPte < LastPte) && (PointerPte->u.Hard.Valid == 1)) {

                MiDeletePte (PointerPte,
                             MiGetVirtualAddressMappedByPte (PointerPte),
                             FALSE,
                             CurrentProcess,
                             NULL,
                             &PteFlushList);

                //
                // Add back in the private page MiDeletePte subtracted.
                //

                CurrentProcess->NumberOfPrivatePages += 1;

                PointerPte += 1;

#if defined (_WIN64)
                //
                // If all the entries have been removed from the previous page
                // table page, delete the page table page itself.  Likewise with
                // the page directory page.
                //
    
                if ((MiIsPteOnPdeBoundary(PointerPte)) ||
                    ((MiGetPdeAddress(PointerPte))->u.Hard.Valid == 0) ||
                    ((MiGetPteAddress(PointerPte))->u.Hard.Valid == 0) ||
                    (PointerPte->u.Hard.Valid == 0)) {
    
                    MiFlushPteList (&PteFlushList, FALSE, ZeroPte);

                    PointerPde = MiGetPteAddress (PointerPte - 1);
    
                    ASSERT (PointerPde->u.Hard.Valid == 1);
    
                    Pfn1 = MI_PFN_ELEMENT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPde));
    
                    if (Pfn1->u2.ShareCount == 1 && Pfn1->u3.e2.ReferenceCount == 1)
                    {
                        MiDeletePte (PointerPde,
                                     PointerPte - 1,
                                     FALSE,
                                     CurrentProcess,
                                     NULL,
                                     NULL);

                        //
                        // Add back in the private page MiDeletePte subtracted.
                        //
        
                        CurrentProcess->NumberOfPrivatePages += 1;
                    }
                
                    if (MiIsPteOnPpeBoundary(PointerPte)) {
        
                        PointerPpe = MiGetPteAddress (PointerPde);
        
                        ASSERT (PointerPpe->u.Hard.Valid == 1);
        
                        Pfn1 = MI_PFN_ELEMENT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPpe));
        
                        if (Pfn1->u2.ShareCount == 1 && Pfn1->u3.e2.ReferenceCount == 1)
                        {
                            MiDeletePte (PointerPpe,
                                         PointerPde,
                                         FALSE,
                                         CurrentProcess,
                                         NULL,
                                         NULL);

                            //
                            // Add back in the private page MiDeletePte subtracted.
                            //
            
                            CurrentProcess->NumberOfPrivatePages += 1;
                        }
                    }
                    PointerPde = MiGetPteAddress (PointerPte);
                    PointerPpe = MiGetPdeAddress (PointerPte);
                    if ((PointerPpe->u.Hard.Valid == 0) ||
                        (PointerPde->u.Hard.Valid == 0)) {
                            break;
                    }
                }
#endif
            }
            MiFlushPteList (&PteFlushList, FALSE, ZeroPte);

            if (WsInfo->u.Flags.SessionSpace == 1) {
        
                //
                // Session space has no ASN - flush the entire TB.
                //
            
                MI_FLUSH_ENTIRE_SESSION_TB (TRUE, TRUE);
            }
        
            UNLOCK_PFN (OldIrql);
        }
#if defined (_WIN64)

        //
        // For 64-bit NT, the page tables and page directories are also
        // deleted during contraction.
        //

        ASSERT ((MiGetPpeAddress(&Table[WorkingSetList->HashTableSize])->u.Hard.Valid == 0) ||
                (MiGetPdeAddress(&Table[WorkingSetList->HashTableSize])->u.Hard.Valid == 0) ||
                (MiGetPteAddress(&Table[WorkingSetList->HashTableSize])->u.Hard.Valid == 0));

#else

        ASSERT (MiGetPteAddress(&Table[WorkingSetList->HashTableSize])->u.Hard.Valid == 0);

#endif
    }

    //
    // If the only pages in the working set are locked pages (that
    // is all pages are BEFORE first dynamic, just reorganize the
    // free list).
    //

    Wsle = WorkingSetList->Wsle;
    if (WorkingSetList->FirstDynamic == WsInfo->WorkingSetSize) {

        LastIndex = WorkingSetList->FirstDynamic;
        LastEntry = &Wsle[LastIndex];

    } else {

        //
        // Start from the first dynamic and move towards the end looking
        // for free entries.  At the same time start from the end and
        // move towards first dynamic looking for valid entries.
        //

        LastInvalid = 0;
        FreeIndex = WorkingSetList->FirstDynamic;
        FreeEntry = &Wsle[FreeIndex];
        LastIndex = WorkingSetList->LastEntry;
        LastEntry = &Wsle[LastIndex];

        while (FreeEntry < LastEntry) {
            if (FreeEntry->u1.e1.Valid == 1) {
                FreeEntry += 1;
                FreeIndex += 1;
            } else if (LastEntry->u1.e1.Valid == 0) {
                LastEntry -= 1;
                LastIndex -= 1;
            } else {

                //
                // Move the WSLE at LastEntry to the free slot at FreeEntry.
                //

                LastInvalid = 1;
                *FreeEntry = *LastEntry;
                PointerPte = MiGetPteAddress (LastEntry->u1.VirtualAddress);

                if (LastEntry->u1.e1.Direct) {

                    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

                    CONSISTENCY_LOCK_PFN (OldIrql);

                    Pfn1->u1.WsIndex = FreeIndex;

                    CONSISTENCY_UNLOCK_PFN (OldIrql);

                } else {

                    //
                    // This entry is in the working set.  Remove it
                    // and then add the entry add the free slot.
                    //

                    MiRemoveWsle (LastIndex, WorkingSetList);
                    MiInsertWsle (FreeIndex, WorkingSetList);
                }

                MI_SET_PTE_IN_WORKING_SET (PointerPte, FreeIndex);
                LastEntry->u1.Long = 0;
                LastEntry -= 1;
                LastIndex -= 1;
                FreeEntry += 1;
                FreeIndex += 1;
            }
        }

        //
        // If no entries were freed, just return.
        //

        if (LastInvalid == 0) {
#if DBG
            MiCheckNullIndex (WorkingSetList);
#endif //DBG
            return;
        }
    }

    //
    // Reorganize the free list.  Make last entry the first free.
    //

    ASSERT ((LastEntry - 1)->u1.e1.Valid == 1);

    if (LastEntry->u1.e1.Valid == 1) {
        LastEntry += 1;
        LastIndex += 1;
    }

    WorkingSetList->LastEntry = LastIndex - 1;
    WorkingSetList->FirstFree = LastIndex;

    ASSERT ((LastEntry - 1)->u1.e1.Valid == 1);
    ASSERT ((LastEntry)->u1.e1.Valid == 0);

    //
    // Point free entry to the first invalid page.
    //

    FreeEntry = LastEntry;

    while (LastIndex < WorkingSetList->LastInitializedWsle) {

        //
        // Put the remainder of the WSLEs on the free list.
        //

        ASSERT (LastEntry->u1.e1.Valid == 0);
        LastIndex += 1;
        LastEntry->u1.Long = LastIndex << MM_FREE_WSLE_SHIFT;
        LastEntry += 1;
    }

    //LastEntry->u1.Long = WSLE_NULL_INDEX << MM_FREE_WSLE_SHIFT;  // End of list.

    //
    // Delete the working set pages at the end.
    //

    PointerPte = MiGetPteAddress (&Wsle[WorkingSetList->LastInitializedWsle]);
    if (&Wsle[WsInfo->MinimumWorkingSetSize] > FreeEntry) {
        FreeEntry = &Wsle[WsInfo->MinimumWorkingSetSize];
    }

    WsPte = MiGetPteAddress (FreeEntry);

#if 0
    if (MiGetPteAddress (FreeEntry) != MiGetPteAddress (FreeEntry + 1)) {
        DbgPrint ("MiRemoveWorkingSetPages caught boundary case %x\n", FreeEntry);
        DbgBreakPoint();

        WsPte = MiGetPteAddress (FreeEntry + 1);
    }
#endif

    ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);

    LOCK_PFN (OldIrql);
    while (PointerPte > WsPte) {
        ASSERT (PointerPte->u.Hard.Valid == 1);

        MiDeletePte (PointerPte,
                     MiGetVirtualAddressMappedByPte (PointerPte),
                     FALSE,
                     CurrentProcess,
                     NULL,
                     &PteFlushList);

        //
        // Add back in the private page MiDeletePte subtracted.
        //

        CurrentProcess->NumberOfPrivatePages += 1;

#if defined (_WIN64)
        //
        // If all the entries have been removed from the previous page
        // table page, delete the page table page itself.  Likewise with
        // the page directory page.
        //

        if (MiIsPteOnPdeBoundary(PointerPte)) {

            PointerPde = MiGetPteAddress (PointerPte);

            ASSERT (PointerPde->u.Hard.Valid == 1);

            Pfn1 = MI_PFN_ELEMENT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPde));

            if (Pfn1->u2.ShareCount == 1 && Pfn1->u3.e2.ReferenceCount == 1) {

                MiFlushPteList (&PteFlushList, FALSE, ZeroPte);

                MiDeletePte (PointerPde,
                             PointerPte,
                             FALSE,
                             CurrentProcess,
                             NULL,
                             NULL);

                //
                // Add back in the private page MiDeletePte subtracted.
                //

                CurrentProcess->NumberOfPrivatePages += 1;
            }
        
            if (MiIsPteOnPpeBoundary(PointerPte)) {

                PointerPpe = MiGetPteAddress (PointerPde);

                ASSERT (PointerPpe->u.Hard.Valid == 1);

                Pfn1 = MI_PFN_ELEMENT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPpe));

                if (Pfn1->u2.ShareCount == 1 && Pfn1->u3.e2.ReferenceCount == 1)
                {

                    MiFlushPteList (&PteFlushList, FALSE, ZeroPte);

                    MiDeletePte (PointerPpe,
                                 PointerPde,
                                 FALSE,
                                 CurrentProcess,
                                 NULL,
                                 NULL);

                    //
                    // Add back in the private page MiDeletePte subtracted.
                    //
    
                    CurrentProcess->NumberOfPrivatePages += 1;
                }
            }
        }
#endif

        PointerPte -= 1;
    }

    MiFlushPteList (&PteFlushList, FALSE, ZeroPte);

    if (WsInfo->u.Flags.SessionSpace == 1) {

        //
        // Session space has no ASN - flush the entire TB.
        //
    
        MI_FLUSH_ENTIRE_SESSION_TB (TRUE, TRUE);
    }

    UNLOCK_PFN (OldIrql);

    ASSERT (WorkingSetList->FirstFree >= WorkingSetList->FirstDynamic);

    //
    // Mark the last PTE in the list as free.
    //

    LastEntry = (PMMWSLE)((PCHAR)(PAGE_ALIGN(FreeEntry)) + PAGE_SIZE);
    LastEntry -= 1;

    ASSERT (LastEntry->u1.e1.Valid == 0);
    LastEntry->u1.Long = WSLE_NULL_INDEX << MM_FREE_WSLE_SHIFT; //End of List.
    ASSERT (LastEntry > &Wsle[0]);
    WorkingSetList->LastInitializedWsle = (WSLE_NUMBER)(LastEntry - &Wsle[0]);
    WorkingSetList->NextSlot = WorkingSetList->FirstDynamic;

    ASSERT (WorkingSetList->LastEntry <= WorkingSetList->LastInitializedWsle);

    if (WorkingSetList->Quota < WorkingSetList->LastInitializedWsle) {
        WorkingSetList->Quota = WorkingSetList->LastInitializedWsle;
    }

    ASSERT ((MiGetPteAddress(&Wsle[WorkingSetList->LastInitializedWsle]))->u.Hard.Valid == 1);
    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));
#if DBG
    MiCheckNullIndex (WorkingSetList);
#endif //DBG
    return;
}


NTSTATUS
MiEmptyWorkingSet (
    IN PMMSUPPORT WsInfo,
    IN LOGICAL WaitOk
    )

/*++

Routine Description:

    This routine frees all pages from the working set.

Arguments:

    WsInfo - Supplies the working set information entry to trim.

    WaitOk - Supplies TRUE if the caller can wait, FALSE if not.

Return Value:

    Status of operation.

Environment:

    Kernel mode. No locks.  For session operations, the caller is responsible
    for attaching into the proper session.

--*/

{
    PEPROCESS Process;
    KIRQL OldIrql;
    PMMPTE PointerPte;
    ULONG Entry;
    ULONG Count;
    ULONG LastFreed;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    ULONG Last;
    NTSTATUS Status;
    PMM_SESSION_SPACE SessionSpace;

    if (WsInfo == &MmSystemCacheWs) {
        if (WaitOk == TRUE) {
            LOCK_SYSTEM_WS (OldIrql);
        }
        else {
            KeRaiseIrql (APC_LEVEL, &OldIrql);
            if (!ExTryToAcquireResourceExclusiveLite (&MmSystemWsLock)) {

                //
                // System working set lock was not granted, don't trim
                // the system cache.
                //

                KeLowerIrql (OldIrql);
                return STATUS_SUCCESS;
            }

            MmSystemLockOwner = PsGetCurrentThread();
        }
    }
    else if (WsInfo->u.Flags.SessionSpace == 0) {
        Process = PsGetCurrentProcess ();
        if (WaitOk == TRUE) {
            LOCK_WS (Process);
        }
        else {
            Count = 0;
            do {
                if (ExTryToAcquireFastMutex(&Process->WorkingSetLock) != FALSE) {
                    break;
                }
                KeDelayExecutionThread (KernelMode, FALSE, &MmShortTime);
                Count += 1;
                if (Count == 5) {

                    //
                    // Could not get the lock, don't trim this process.
                    //

                    return STATUS_SUCCESS;
                }
            } while (TRUE);
        }
        if (Process->AddressSpaceDeleted != 0) {
            Status = STATUS_PROCESS_IS_TERMINATING;
            goto Deleted;
        }
    }
    else {
        if (WaitOk == TRUE) {
            LOCK_SESSION_SPACE_WS (OldIrql);
        }
        else {
            ASSERT (MiHydra == TRUE);
            SessionSpace = CONTAINING_RECORD(WsInfo,
                                             MM_SESSION_SPACE,
                                             Vm);

            KeRaiseIrql (APC_LEVEL, &OldIrql);

            if (!ExTryToAcquireResourceExclusiveLite (&SessionSpace->WsLock)) {

                //
                // This session space's working set lock was not
                // granted, don't trim it.
                //

                KeLowerIrql (OldIrql);
                return STATUS_SUCCESS;
            }

            MM_SET_SESSION_RESOURCE_OWNER();
        }
    }

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    //
    // Attempt to remove the pages starting at the bottom.
    //

    LastFreed = WorkingSetList->LastEntry;
    for (Entry = WorkingSetList->FirstDynamic; Entry <= LastFreed; Entry += 1) {

        if (Wsle[Entry].u1.e1.Valid != 0) {
            PERFINFO_PAGE_INFO_DECL();

            PointerPte = MiGetPteAddress (Wsle[Entry].u1.VirtualAddress);

            PERFINFO_GET_PAGE_INFO(PointerPte);

            if (MiTrimRemovalPagesOnly == TRUE) {
                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                if (Pfn1->u3.e1.RemovalRequested == 0) {
                    Pfn1 = MI_PFN_ELEMENT (Pfn1->PteFrame);
                    if (Pfn1->u3.e1.RemovalRequested == 0) {
#if defined (_WIN64)
                        Pfn1 = MI_PFN_ELEMENT (Pfn1->PteFrame);
                        if (Pfn1->u3.e1.RemovalRequested == 0) {
                            continue;
                        }
#else
                        continue;
#endif
                    }
                }
            }

            if (MiFreeWsle (Entry, WsInfo, PointerPte)) {
                PERFINFO_LOG_WS_REMOVAL(PERFINFO_LOG_TYPE_OUTWS_EMPTYQ, WsInfo);
            }
        }
    }

    if (WsInfo != &MmSystemCacheWs && WsInfo->u.Flags.SessionSpace == 0) {
        MiRemoveWorkingSetPages (WorkingSetList,WsInfo);
    }
    WorkingSetList->Quota = WsInfo->WorkingSetSize;
    WorkingSetList->NextSlot = WorkingSetList->FirstDynamic;

    //
    // Attempt to remove the pages from the front to the end.
    //

    //
    // Reorder the free list.
    //

    Last = 0;
    Entry = WorkingSetList->FirstDynamic;
    LastFreed = WorkingSetList->LastInitializedWsle;
    while (Entry <= LastFreed) {
        if (Wsle[Entry].u1.e1.Valid == 0) {
            if (Last == 0) {
                WorkingSetList->FirstFree = Entry;
            } else {
                Wsle[Last].u1.Long = Entry << MM_FREE_WSLE_SHIFT;
            }
            Last = Entry;
        }
        Entry += 1;
    }
    if (Last != 0) {
        Wsle[Last].u1.Long = WSLE_NULL_INDEX << MM_FREE_WSLE_SHIFT;  // End of list.
    }
    Status = STATUS_SUCCESS;
Deleted:

    if (WsInfo == &MmSystemCacheWs) {
        UNLOCK_SYSTEM_WS (OldIrql);
    }
    else if (WsInfo->u.Flags.SessionSpace == 0) {
        UNLOCK_WS (Process);
    }
    else {
        UNLOCK_SESSION_SPACE_WS (OldIrql);
    }

    return Status;
}

#if 0

#define x256k_pte_mask (((256*1024) >> (PAGE_SHIFT - PTE_SHIFT)) - (sizeof(MMPTE)))

VOID
MiDumpWsleInCacheBlock (
    IN PMMPTE CachePte
    )

/*++

Routine Description:

    The routine checks the prototype PTEs adjacent to the supplied
    PTE and if they are modified, in the system cache working set,
    and have a reference count of 1, removes it from the system
    cache working set.

Arguments:

    CachePte - Supplies a pointer to the cache PTE.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held, APCs disabled.

--*/

{
    PMMPTE LoopPte;
    PMMPTE PointerPte;

    LoopPte = (PMMPTE)((ULONG_PTR)CachePte & ~x256k_pte_mask);
    PointerPte = CachePte - 1;

    while (PointerPte >= LoopPte ) {

        if (MiDumpPteInCacheBlock (PointerPte) == FALSE) {
            break;
        }
        PointerPte -= 1;
    }

    PointerPte = CachePte + 1;
    LoopPte = (PMMPTE)((ULONG_PTR)CachePte | x256k_pte_mask);

    while (PointerPte <= LoopPte ) {

        if (MiDumpPteInCacheBlock (PointerPte) == FALSE) {
            break;
        }
        PointerPte += 1;
    }
    return;
}

ULONG
MiDumpPteInCacheBlock (
    IN PMMPTE PointerPte
    )

{
    PMMPFN Pfn1;
    MMPTE PteContents;
    ULONG WorkingSetIndex;

    PteContents = *PointerPte;

    if (PteContents.u.Hard.Valid == 1) {

        Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

        //
        // If the PTE is valid and dirty (or pfn indicates dirty)
        // and the Wsle is direct index via the pfn wsindex element
        // and the reference count is one, then remove this page from
        // the cache manager's working set list.
        //

        if ((Pfn1->u3.e2.ReferenceCount == 1) &&
            ((Pfn1->u3.e1.Modified == 1) ||
                (MI_IS_PTE_DIRTY (PteContents))) &&
                (MiGetPteAddress (
                    MmSystemCacheWsle[Pfn1->u1.WsIndex].u1.VirtualAddress) ==
                    PointerPte)) {

            //
            // Found a candidate, remove the page from the working set.
            //

            WorkingSetIndex = Pfn1->u1.WsIndex;
            LOCK_PFN (OldIrql);
            MiEliminateWorkingSetEntry (WorkingSetIndex,
                                        PointerPte,
                                        Pfn1,
                                        MmSystemCacheWsle);
            UNLOCK_PFN (OldIrql);

            //
            // Remove the working set entry from the working set.
            //

            MiRemoveWsle (WorkingSetIndex, MmSystemCacheWorkingSetList);

            //
            // Put the entry on the free list and decrement the current
            // size.
            //

            MmSystemCacheWsle[WorkingSetIndex].u1.Long =
                  MmSystemCacheWorkingSetList->FirstFree << MM_FREE_WSLE_SHIFT;
            MmSystemCacheWorkingSetList->FirstFree = WorkingSetIndex;

            if (MmSystemCacheWs.WorkingSetSize > MmSystemCacheWs.MinimumWorkingSetSize) {
                MmPagesAboveWsMinimum -= 1;
            }
            MmSystemCacheWs.WorkingSetSize -= 1;
            return TRUE;
        }
    }
    return FALSE;
}
#endif //0

#if DBG
VOID
MiCheckNullIndex (
    IN PMMWSL WorkingSetList
    )

{
    PMMWSLE Wsle;
    ULONG j;
    ULONG Nulls = 0;

    Wsle = WorkingSetList->Wsle;
    for (j = 0;j <= WorkingSetList->LastInitializedWsle; j += 1) {
        if ((((Wsle[j].u1.Long)) >> MM_FREE_WSLE_SHIFT) == WSLE_NULL_INDEX) {
            Nulls += 1;
        }
    }
    ASSERT (Nulls == 1);
    return;
}

#endif //DBG
