/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   mmquota.c

Abstract:

    This module contains the routines which implement the quota and
    commitment charging for memory management.

Author:

    Lou Perazzoli (loup) 12-December-89
    Landy Wang (landyw) 02-Jun-1997

Revision History:

--*/

#include "mi.h"

#define MM_MAXIMUM_QUOTA_OVERCHARGE 9

#define MM_DONT_EXTEND_SIZE 512

#define MM_COMMIT_POPUP_MAX ((512*1024)/PAGE_SIZE)

#define MM_EXTEND_COMMIT ((1024*1024)/PAGE_SIZE)

SIZE_T MmPeakCommitment;

SIZE_T MmExtendedCommit;

SIZE_T MmExtendedCommitLimit;

LOGICAL MiCommitExtensionActive = FALSE;

extern ULONG_PTR MmAllocatedPagedPool;

extern SIZE_T MmPageFileFullExtendPages;

ULONG MiOverCommitCallCount;
extern EPROCESS_QUOTA_BLOCK PspDefaultQuotaBlock;


LOGICAL
MiCauseOverCommitPopup(
    IN SIZE_T NumberOfPages,
    IN ULONG Extension
    );


ULONG
FASTCALL
MiChargePageFileQuota (
    IN SIZE_T QuotaCharge,
    IN PEPROCESS CurrentProcess
    )

/*++

Routine Description:

    This routine checks to ensure the user has sufficient page file
    quota remaining and, if so, charges the quota.  If not an exception
    is raised.

Arguments:

    QuotaCharge - Supplies the quota amount to charge.

    CurrentProcess - Supplies a pointer to the current process.

Return Value:

    TRUE if the quota was successfully charged, raises an exception
    otherwise.

Environment:

    Kernel mode, APCs disabled, WorkingSetLock and AddressCreation mutexes
    held.

--*/

{
    SIZE_T NewPagefileValue;
    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    KIRQL OldIrql;

    QuotaBlock = CurrentProcess->QuotaBlock;

retry_charge:
    if ( QuotaBlock != &PspDefaultQuotaBlock) {
        ExAcquireFastLock (&QuotaBlock->QuotaLock,&OldIrql);
do_charge:
        NewPagefileValue = QuotaBlock->PagefileUsage + QuotaCharge;

        if (NewPagefileValue > QuotaBlock->PagefileLimit) {
            ExReleaseFastLock (&QuotaBlock->QuotaLock,OldIrql);
            ExRaiseStatus (STATUS_PAGEFILE_QUOTA_EXCEEDED);
        }

        QuotaBlock->PagefileUsage = NewPagefileValue;

        if (NewPagefileValue > QuotaBlock->PeakPagefileUsage) {
            QuotaBlock->PeakPagefileUsage = NewPagefileValue;
        }

        NewPagefileValue = CurrentProcess->PagefileUsage + QuotaCharge;
        CurrentProcess->PagefileUsage = NewPagefileValue;

        if (NewPagefileValue > CurrentProcess->PeakPagefileUsage) {
            CurrentProcess->PeakPagefileUsage = NewPagefileValue;
        }
        ExReleaseFastLock (&QuotaBlock->QuotaLock,OldIrql);
    } else {
        ExAcquireFastLock (&PspDefaultQuotaBlock.QuotaLock,&OldIrql);

        if ( (QuotaBlock = CurrentProcess->QuotaBlock) != &PspDefaultQuotaBlock) {
            ExReleaseFastLock(&PspDefaultQuotaBlock.QuotaLock,OldIrql);
            goto retry_charge;
        }
        goto do_charge;
    }
    return TRUE;
}

VOID
MiReturnPageFileQuota (
    IN SIZE_T QuotaCharge,
    IN PEPROCESS CurrentProcess
    )

/*++

Routine Description:

    This routine releases page file quota.

Arguments:

    QuotaCharge - Supplies the quota amount to charge.

    CurrentProcess - Supplies a pointer to the current process.

Return Value:

    none.

Environment:

    Kernel mode, APCs disabled, WorkingSetLock and AddressCreation mutexes
    held.

--*/

{

    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    KIRQL OldIrql;

    QuotaBlock = CurrentProcess->QuotaBlock;

retry_return:
    if ( QuotaBlock != &PspDefaultQuotaBlock) {
        ExAcquireFastLock (&QuotaBlock->QuotaLock, &OldIrql);
do_return:
        ASSERT (CurrentProcess->PagefileUsage >= QuotaCharge);
        CurrentProcess->PagefileUsage -= QuotaCharge;

        ASSERT (QuotaBlock->PagefileUsage >= QuotaCharge);
        QuotaBlock->PagefileUsage -= QuotaCharge;
        ExReleaseFastLock(&QuotaBlock->QuotaLock,OldIrql);
    } else {
        ExAcquireFastLock (&PspDefaultQuotaBlock.QuotaLock, &OldIrql);
        if ( (QuotaBlock = CurrentProcess->QuotaBlock) != &PspDefaultQuotaBlock ) {
            ExReleaseFastLock(&PspDefaultQuotaBlock.QuotaLock,OldIrql);
            goto retry_return;
        }
        goto do_return;
    }
    return;
}

LOGICAL
FASTCALL
MiChargeCommitment (
    IN SIZE_T QuotaCharge,
    IN PEPROCESS Process OPTIONAL
    )

/*++

Routine Description:

    This routine checks to ensure the system has sufficient page file
    space remaining.

Arguments:

    QuotaCharge - Supplies the quota amount to charge.

    Process - Optionally supplies the current process IF AND ONLY IF
              the working set mutex is held.  If the paging file
              is being extended, the working set mutex is released if
              this is non-null.

Return Value:

    TRUE if there is sufficient space, FALSE if not.

Environment:

    Kernel mode, APCs disabled, WorkingSetLock and AddressCreation mutexes
    held.

--*/

{
    KIRQL OldIrql;
    SIZE_T NewCommitValue;
    MMPAGE_FILE_EXPANSION PageExtend;
    LOGICAL WsHeldSafe;

#if !defined (_WIN64)
    ASSERT (QuotaCharge < 0x100000);
#endif

    ExAcquireFastLock (&MmChargeCommitmentLock, &OldIrql);

    NewCommitValue = MmTotalCommittedPages + QuotaCharge;

    while (NewCommitValue > MmTotalCommitLimit) {

        ExReleaseFastLock (&MmChargeCommitmentLock, OldIrql);

        if (Process != NULL) {

            //
            // The working set lock may have been acquired safely or unsafely
            // by our caller.  Handle both cases here and below.
            //

            UNLOCK_WS_REGARDLESS(Process, WsHeldSafe);
        }

        //
        // Queue a message to the segment dereferencing / pagefile extending
        // thread to see if the page file can be extended.  This is done
        // in the context of a system thread due to mutexes which may
        // currently be held.
        //

        PageExtend.RequestedExpansionSize = QuotaCharge;
        PageExtend.Segment = NULL;
        PageExtend.PageFileNumber = MI_EXTEND_ANY_PAGEFILE;
        KeInitializeEvent (&PageExtend.Event, NotificationEvent, FALSE);

        if (MiIssuePageExtendRequest (&PageExtend) == FALSE) {

            if (Process != NULL) {
                LOCK_WS_REGARDLESS(Process, WsHeldSafe);
            }

            //
            // If the quota is small enough, commit it anyway.  Otherwise
            // return an error.
            //

            if (QuotaCharge < MM_MAXIMUM_QUOTA_OVERCHARGE) {

                //
                // Try the can't expand routine.
                //

                if (MiChargeCommitmentCantExpand (QuotaCharge, FALSE) == FALSE) {
                    return FALSE;
                }
            } else {

                //
                // Put up a popup and grant an extension if possible.
                //

                if (MiCauseOverCommitPopup (QuotaCharge, MM_EXTEND_COMMIT) == FALSE) {
                    return FALSE;
                }
            }
            return TRUE;
        }

        if (Process != NULL) {
            if (WsHeldSafe == TRUE) {
                LOCK_WS (Process);
            }
            else {
                LOCK_WS_UNSAFE (Process);
            }
        }

        if (PageExtend.ActualExpansion == 0) {
            if (MiCauseOverCommitPopup (QuotaCharge, MM_EXTEND_COMMIT) == FALSE) {
                return FALSE;
            }
            return TRUE;
        }

        ExAcquireFastLock (&MmChargeCommitmentLock, &OldIrql);
        NewCommitValue = MmTotalCommittedPages + QuotaCharge;
    }

    MmTotalCommittedPages = NewCommitValue;
    if ((MmTotalCommittedPages > MmPeakCommitment) &&
        (MmPageFileFullExtendPages == 0)) {
        MmPeakCommitment = MmTotalCommittedPages;
    }

    ExReleaseFastLock (&MmChargeCommitmentLock, OldIrql);

    return TRUE;
}

LOGICAL
FASTCALL
MiChargeCommitmentCantExpand (
    IN SIZE_T QuotaCharge,
    IN ULONG MustSucceed
    )

/*++

Routine Description:

    This routine charges the specified commitment without attempting
    to expand paging file and waiting for the expansion.  The routine
    determines if the paging file space is exhausted, and if so,
    it attempts to ascertain if the paging file space could be expanded.

Arguments:

    QuotaCharge - Supplies the quota amount to charge.

    MustSucceed - Supplies TRUE if the charge must succeed.

Return Value:

    TRUE if the commitment was permitted, FALSE if not.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    KIRQL OldIrql;
    SIZE_T NewCommitValue;
    SIZE_T ExtendAmount;

    ExAcquireFastLock (&MmChargeCommitmentLock, &OldIrql);

    //
    // If the overcommitment is bigger than 512 pages, don't extend.
    //

    NewCommitValue = MmTotalCommittedPages + QuotaCharge;

    if (!MustSucceed) {

        if (NewCommitValue > MmTotalCommitLimit) {

            if ((NewCommitValue - MmTotalCommitLimit > MM_DONT_EXTEND_SIZE) ||
                (NewCommitValue < MmTotalCommittedPages) ||
                (NewCommitValue > MmTotalCommitLimitMaximum)) {

                    ExReleaseFastLock (&MmChargeCommitmentLock, OldIrql);
                    return FALSE;
            }
        }
        else if (NewCommitValue > MmTotalCommitLimitMaximum) {
            ExReleaseFastLock (&MmChargeCommitmentLock, OldIrql);
            return FALSE;
        }
    }

    ExtendAmount = NewCommitValue - MmTotalCommitLimit;
    MmTotalCommittedPages = NewCommitValue;

    if (NewCommitValue > (MmTotalCommitLimit + 20)) {

        //
        // Attempt to expand the paging file, but don't wait
        // to see if it succeeds.
        //

        if (MmAttemptForCantExtend.InProgress != FALSE) {

            //
            // An expansion request is already in progress, assume
            // this will succeed.
            //

            ExReleaseFastLock (&MmChargeCommitmentLock, OldIrql);
            return TRUE;
        }

        MmAttemptForCantExtend.InProgress = TRUE;
        ExReleaseFastLock (&MmChargeCommitmentLock, OldIrql);

        //
        // Queue a message to the segment dereferencing / pagefile extending
        // thread to see if the page file can be extended.  This is done
        // in the context of a system thread due to mutexes which may
        // currently be held.
        //

        if (QuotaCharge > ExtendAmount) {
            ExtendAmount = QuotaCharge;
        }

        MmAttemptForCantExtend.RequestedExpansionSize = ExtendAmount;
        ExAcquireFastLock (&MmDereferenceSegmentHeader.Lock, &OldIrql);
        InsertTailList ( &MmDereferenceSegmentHeader.ListHead,
                         &MmAttemptForCantExtend.DereferenceList);
        ExReleaseFastLock (&MmDereferenceSegmentHeader.Lock, OldIrql);

        KeReleaseSemaphore (&MmDereferenceSegmentHeader.Semaphore, 0L, 1L, FALSE);
    }
    else {
        ExReleaseFastLock (&MmChargeCommitmentLock, OldIrql);
    }

    return TRUE;
}

VOID
FASTCALL
MiReturnCommitment (
    IN SIZE_T QuotaCharge
    )

/*++

Routine Description:

    This routine releases page file quota.

Arguments:

    QuotaCharge - Supplies the quota amount to charge.

    CurrentProcess - Supplies a pointer to the current process.

Return Value:

    none.

Environment:

    Kernel mode, APCs disabled, WorkingSetLock and AddressCreation mutexes
    held.

--*/

{
    KIRQL OldIrql;

    ExAcquireFastLock (&MmChargeCommitmentLock, &OldIrql);

    ASSERT (MmTotalCommittedPages >= QuotaCharge);

    MmTotalCommittedPages -= QuotaCharge;

    //
    // If commit allotments have been temporarily blocked then open the
    // floodgates provided either enough commit has been freed or the pagefile
    // extension has succeeded.
    //

    if (MmPageFileFullExtendPages) {
        ASSERT (MmTotalCommittedPages >= MmPageFileFullExtendPages);
        MmTotalCommittedPages -= MmPageFileFullExtendPages;
        MmPageFileFullExtendPages = 0;
    }

    //
    // If the system automatically granted an increase that can be returned
    // now, do it.
    //

    if (MiCommitExtensionActive == TRUE) {

        if ((MmExtendedCommitLimit != 0) &&
            (MmTotalCommitLimit > MmTotalCommittedPages) &&
            (MmTotalCommitLimit - MmTotalCommittedPages > MmExtendedCommitLimit)) {
                MmTotalCommitLimit -= MmExtendedCommitLimit;
                MmExtendedCommitLimit = 0;
        }

        if (MmExtendedCommit != 0) {
            MmTotalCommittedPages -= MmExtendedCommit;
            MmExtendedCommit = 0;
        }

        if ((MmExtendedCommitLimit == 0) && (MmExtendedCommit == 0)) {
            MiCommitExtensionActive = FALSE;
        }
    }

    ExReleaseFastLock (&MmChargeCommitmentLock, OldIrql);
    return;
}

SIZE_T
MiCalculatePageCommitment (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PMMVAD Vad,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine examines the range of pages from the starting address
    up to and including the ending address and returns the commit charge
    for  the pages within the range.

Arguments:

    StartingAddress - Supplies the starting address of the range.

    EndingAddress - Supplies the ending address of the range.

    Vad - Supplies the virtual address descriptor which describes the range.

    Process - Supplies the current process.

Return Value:

    Commitment charge for the range.

Environment:

    Kernel mode, APCs disabled, WorkingSetLock and AddressCreation mutexes
    held.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE TempEnd;
    SIZE_T NumberOfCommittedPages;
    ULONG Waited;

    NumberOfCommittedPages = 0;

    PointerPpe = MiGetPpeAddress (StartingAddress);
    PointerPde = MiGetPdeAddress (StartingAddress);
    PointerPte = MiGetPteAddress (StartingAddress);

    if (Vad->u.VadFlags.MemCommit == 1) {

        TempEnd = EndingAddress;

        //
        // All the pages are committed within this range.
        //

        NumberOfCommittedPages = BYTES_TO_PAGES ((PCHAR)TempEnd -
                                                       (PCHAR)StartingAddress);


        //
        // Examine the PTEs to determine how many pages are committed.
        //

        LastPte = MiGetPteAddress (TempEnd);

        do {

            while (!MiDoesPpeExistAndMakeValid (PointerPpe,
                                                Process,
                                                FALSE,
                                                &Waited)) {
    
                //
                // No PPE exists for the starting address, therefore the page
                // is not committed.
                //
    
                PointerPpe += 1;
                PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                if (PointerPte > LastPte) {
                    goto DoneCommit;
                }
            }

            Waited = 0;

            while (!MiDoesPdeExistAndMakeValid (PointerPde,
                                                Process,
                                                FALSE,
                                                &Waited)) {
    
                //
                // No PDE exists for the starting address, therefore the page
                // is not committed.
                //
    
                PointerPde += 1;
                PointerPpe = MiGetPteAddress (PointerPde);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                if (PointerPte > LastPte) {
                    goto DoneCommit;
                }
#if defined (_WIN64)
                if (MiIsPteOnPdeBoundary (PointerPde)) {
                    Waited = 1;
                    break;
                }
#endif
            }

        } while (Waited != 0);

restart:

        while (PointerPte <= LastPte) {

            if (MiIsPteOnPdeBoundary (PointerPte)) {

                //
                // This is a PDE boundary, check to see if the entire
                // PPE/PDE pages exist.
                //

                PointerPde = MiGetPteAddress (PointerPte);
                PointerPpe = MiGetPteAddress (PointerPde);

                do {

                    if (!MiDoesPpeExistAndMakeValid (PointerPpe,
                                                     Process,
                                                     FALSE,
                                                     &Waited)) {
    
                        //
                        // No PDE exists for the starting address, check the VAD
                        // to see if the pages are not committed.
                        //
    
                        PointerPpe += 1;
                        PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    
                        //
                        // Check next page.
                        //
    
                        goto restart;
                    }
    
                    Waited = 0;
    
                    if (!MiDoesPdeExistAndMakeValid (PointerPde,
                                                     Process,
                                                     FALSE,
                                                     &Waited)) {
    
                        //
                        // No PDE exists for the starting address, check the VAD
                        // to see if the pages are not committed.
                        //
    
                        PointerPde += 1;
                        PointerPpe = MiGetPteAddress (PointerPde);
                        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    
                        //
                        // Check next page.
                        //
    
                        goto restart;
                    }
                } while (Waited != 0);
            }

            //
            // The PDE exists, examine the PTE.
            //

            if (PointerPte->u.Long != 0) {

                //
                // Has this page been explicitly decommitted?
                //

                if (MiIsPteDecommittedPage (PointerPte)) {

                    //
                    // This page is decommitted, remove it from the count.
                    //

                    NumberOfCommittedPages -= 1;

                }
            }

            PointerPte += 1;
        }

DoneCommit:

        if (TempEnd == EndingAddress) {
            return NumberOfCommittedPages;
        }

    }

    //
    // Examine non committed range.
    //

    LastPte = MiGetPteAddress (EndingAddress);

    do {

        while (!MiDoesPpeExistAndMakeValid (PointerPpe,
                                            Process,
                                            FALSE,
                                            &Waited)) {
    
    
            //
            // No PDE exists for the starting address, therefore the page
            // is not committed.
            //
    
            PointerPpe += 1;
            PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
            if (PointerPte > LastPte) {
               return NumberOfCommittedPages;
            }
        }

        Waited = 0;

        while (!MiDoesPdeExistAndMakeValid (PointerPde,
                                            Process,
                                            FALSE,
                                            &Waited)) {
    
            //
            // No PDE exists for the starting address, therefore the page
            // is not committed.
            //
    
            PointerPde += 1;
            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
            if (PointerPte > LastPte) {
               return NumberOfCommittedPages;
            }
#if defined (_WIN64)
            if (MiIsPteOnPdeBoundary (PointerPde)) {
                PointerPpe = MiGetPteAddress (PointerPde);
                Waited = 1;
                break;
            }
#endif
        }

    } while (Waited != 0);

restart2:

    while (PointerPte <= LastPte) {

        if (MiIsPteOnPdeBoundary (PointerPte)) {

            //
            // This is a PDE boundary, check to see if the entire
            // PPE/PDE pages exist.
            //

            PointerPde = MiGetPteAddress (PointerPte);
            PointerPpe = MiGetPteAddress (PointerPde);

            do {

                if (!MiDoesPpeExistAndMakeValid (PointerPpe,
                                                 Process,
                                                 FALSE,
                                                 &Waited)) {
    
                    //
                    // No PPE exists for the starting address, check the VAD
                    // to see if the pages are not committed.
                    //
    
                    PointerPpe += 1;
                    PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    
                    //
                    // Check next page.
                    //
    
                    goto restart2;
                }
    
                Waited = 0;
    
                if (!MiDoesPdeExistAndMakeValid (PointerPde,
                                                 Process,
                                                 FALSE,
                                                 &Waited)) {
    
                    //
                    // No PDE exists for the starting address, check the VAD
                    // to see if the pages are not committed.
                    //
    
                    PointerPde += 1;
                    PointerPpe = MiGetPteAddress (PointerPde);
                    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    
                    //
                    // Check next page.
                    //
    
                    goto restart2;
                }

            } while (Waited != 0);
        }

        //
        // The PDE exists, examine the PTE.
        //

        if ((PointerPte->u.Long != 0) &&
             (!MiIsPteDecommittedPage (PointerPte))) {

            //
            // This page is committed, count it.
            //

            NumberOfCommittedPages += 1;
        }

        PointerPte += 1;
    }

    return NumberOfCommittedPages;
}

VOID
MiReturnPageTablePageCommitment (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PEPROCESS CurrentProcess,
    IN PMMVAD PreviousVad,
    IN PMMVAD NextVad
    )

/*++

Routine Description:

    This routine returns commitment for COMPLETE page table pages which
    span the virtual address range.  For example (assuming 4k pages),
    if the StartingAddress =  64k and the EndingAddress = 5mb, no
    page table charges would be freed as a complete page table page is
    not covered by the range.  However, if the StartingAddress was 4mb
    and the EndingAddress was 9mb, 1 page table page would be freed.

Arguments:

    StartingAddress - Supplies the starting address of the range.

    EndingAddress - Supplies the ending address of the range.

    CurrentProcess - Supplies a pointer to the current process.

    PreviousVad - Supplies a pointer to the previous VAD, NULL if none.

    NextVad - Supplies a pointer to the next VAD, NULL if none.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, WorkingSetLock and AddressCreation mutexes
    held.

--*/

{
#ifdef _WIN64
    DBG_UNREFERENCED_PARAMETER (StartingAddress);
    DBG_UNREFERENCED_PARAMETER (EndingAddress);
    DBG_UNREFERENCED_PARAMETER (CurrentProcess);
    DBG_UNREFERENCED_PARAMETER (PreviousVad);
    DBG_UNREFERENCED_PARAMETER (NextVad);
#else
    ULONG NumberToClear;
    LONG FirstPage;
    LONG LastPage;
    LONG PreviousPage;
    LONG NextPage;

    //
    // Check to see if any page table pages would be freed.
    //

    ASSERT (StartingAddress != EndingAddress);

    if (PreviousVad == NULL) {
        PreviousPage = -1;
    } else {
        PreviousPage = MiGetPpePdeOffset (MI_VPN_TO_VA (PreviousVad->EndingVpn));
    }

    if (NextVad == NULL) {
        NextPage = MiGetPpePdeOffset (MM_HIGHEST_USER_ADDRESS) + 1;
    } else {
        NextPage = MiGetPpePdeOffset (MI_VPN_TO_VA (NextVad->StartingVpn));
    }

    ASSERT (PreviousPage <= NextPage);

    FirstPage = MiGetPpePdeOffset  (StartingAddress);

    LastPage = MiGetPpePdeOffset (EndingAddress);

    if (PreviousPage == FirstPage) {

        //
        // A VAD is within the starting page table page.
        //

        FirstPage += 1;
    }

    if (NextPage == LastPage) {

        //
        // A VAD is within the ending page table page.
        //

        LastPage -= 1;
    }

    //
    // Indicate that the page table page is not in use.
    //

    if (FirstPage > LastPage) {
        return;
    }

    NumberToClear = 1 + LastPage - FirstPage;

    while (FirstPage <= LastPage) {
        ASSERT (MI_CHECK_BIT (MmWorkingSetList->CommittedPageTables,
                              FirstPage));

        MI_CLEAR_BIT (MmWorkingSetList->CommittedPageTables, FirstPage);
        FirstPage += 1;
    }

    MmWorkingSetList->NumberOfCommittedPageTables -= NumberToClear;
    MiReturnCommitment (NumberToClear);
    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_PAGETABLES, NumberToClear);
    MiReturnPageFileQuota (NumberToClear, CurrentProcess);

    if (CurrentProcess->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
        PsChangeJobMemoryUsage(-(SSIZE_T)NumberToClear);
    }
    CurrentProcess->CommitCharge -= NumberToClear;

    return;
#endif
}


LOGICAL
MiCauseOverCommitPopup(
    SIZE_T NumberOfPages,
    IN ULONG Extension
    )

/*++

Routine Description:

    This function causes an over commit popup to occur.  If a popup is pending
    it returns FALSE.  Otherwise, it queues a popup to a noncritical worker
    thread.

Arguments:

    NumberOfPages - Supplies the number of pages of commit requested.

    Extension - Supplies the extension to grant.

Return Value:

    TRUE - An overcommit popup was queued.

    FALSE - An overcommit popup is still pending and will not be queued.

--*/

{
    KIRQL OldIrql;
    BOOLEAN RaisedPopup;
    ULONG PopupNumber;

    if (NumberOfPages > MM_COMMIT_POPUP_MAX ||
        MmTotalCommittedPages + NumberOfPages > MmTotalCommitLimitMaximum) {
            return FALSE;
    }

    //
    // Give the user a meaningful message - either to increase the minimum,
    // maximum, or both.
    //

    if (MmTotalCommittedPages > MmTotalCommitLimitMaximum - 100) {
        PopupNumber = STATUS_COMMITMENT_LIMIT;
    }
    else {
        PopupNumber = STATUS_COMMITMENT_MINIMUM;
    }

    RaisedPopup = IoRaiseInformationalHardError (PopupNumber, NULL, NULL);

    ExAcquireFastLock (&MmChargeCommitmentLock, &OldIrql);

    if ((RaisedPopup == FALSE) && (MiOverCommitCallCount > 0)) {

        //
        // There is already a popup outstanding and we have not
        // returned any of the quota.
        //

        ExReleaseFastLock (&MmChargeCommitmentLock, OldIrql);
        return FALSE;
    }

    //
    // Now that the commitment lock is held, ensure the commit is not being
    // raised past the absolute maximum.
    //

    if (MmTotalCommittedPages + NumberOfPages > MmTotalCommitLimitMaximum) {
        ExReleaseFastLock (&MmChargeCommitmentLock, OldIrql);
        return FALSE;
    }

    //
    // Don't automatically grant increases forever as this can allow the
    // system to run out of available pages.
    //

    if (MmExtendedCommit > 1024) {
        ExReleaseFastLock (&MmChargeCommitmentLock, OldIrql);
        return FALSE;
    }

    MiOverCommitCallCount += 1;

    MiCommitExtensionActive = TRUE;

    MmTotalCommitLimit += Extension;
    MmExtendedCommitLimit += Extension;

    MmTotalCommittedPages += NumberOfPages;

    //
    // The caller will not release this commit, so we must earmark it now
    // for later release.
    //

    if (Extension == 0) {
        MmExtendedCommit += NumberOfPages;
    }

    if ((MmTotalCommittedPages > MmPeakCommitment) &&
        (MmPageFileFullExtendPages == 0)) {
        MmPeakCommitment = MmTotalCommittedPages;
    }

    ExReleaseFastLock (&MmChargeCommitmentLock, OldIrql);

    return TRUE;
}


SIZE_T MmTotalPagedPoolQuota;
SIZE_T MmTotalNonPagedPoolQuota;

BOOLEAN
MmRaisePoolQuota(
    IN POOL_TYPE PoolType,
    IN SIZE_T OldQuotaLimit,
    OUT PSIZE_T NewQuotaLimit
    )

/*++

Routine Description:

    This function is called (with a spinlock) whenever PS detects a quota
    limit has been exceeded. The purpose of this function is to attempt to
    increase the specified quota.

Arguments:

    PoolType - Supplies the pool type of the quota to be raised

    OldQuotaLimit - Supplies the current quota limit for this pool type

    NewQuotaLimit - Returns the new limit

Return Value:

    TRUE - The API succeeded and the quota limit was raised.

    FALSE - We were unable to raise the quota limit.

Environment:

    Kernel mode, QUOTA SPIN LOCK HELD!!

--*/

{
    SIZE_T Limit;
    PMM_PAGED_POOL_INFO PagedPoolInfo;

    if (PoolType == PagedPool) {

        //
        // Check commit limit and make sure at least 1mb is available.
        // Check to make sure 4mb of paged pool still exists.
        //

        PagedPoolInfo = &MmPagedPoolInfo;

        if ((MmSizeOfPagedPoolInBytes >> PAGE_SHIFT) <
            (PagedPoolInfo->AllocatedPagedPool + ((MMPAGED_QUOTA_CHECK) >> PAGE_SHIFT))) {

            return FALSE;
        }

        MmTotalPagedPoolQuota += (MMPAGED_QUOTA_INCREASE);
        *NewQuotaLimit = OldQuotaLimit + (MMPAGED_QUOTA_INCREASE);
        return TRUE;

    } else {

        if ( (ULONG_PTR)(MmAllocatedNonPagedPool + ((1*1024*1024) >> PAGE_SHIFT)) < (MmMaximumNonPagedPoolInBytes >> PAGE_SHIFT)) {
            goto aok;
            }

        //
        // Make sure 200 pages and 5mb of nonpaged pool expansion
        // available.  Raise quota by 64k.
        //

        if ((MmAvailablePages < 200) ||
            (MmResidentAvailablePages < ((MMNONPAGED_QUOTA_CHECK) >> PAGE_SHIFT))) {

            return FALSE;
        }

        if (MmAvailablePages > ((4*1024*1024) >> PAGE_SHIFT)) {
            Limit = (1*1024*1024) >> PAGE_SHIFT;
        } else {
            Limit = (4*1024*1024) >> PAGE_SHIFT;
        }

        if ((ULONG_PTR)((MmMaximumNonPagedPoolInBytes >> PAGE_SHIFT)) <
            (MmAllocatedNonPagedPool + Limit)) {

            return FALSE;
        }
aok:
        MmTotalNonPagedPoolQuota += (MMNONPAGED_QUOTA_INCREASE);
        *NewQuotaLimit = OldQuotaLimit + (MMNONPAGED_QUOTA_INCREASE);
        return TRUE;
    }
}


VOID
MmReturnPoolQuota(
    IN POOL_TYPE PoolType,
    IN SIZE_T ReturnedQuota
    )

/*++

Routine Description:

    Returns pool quota.

Arguments:

    PoolType - Supplies the pool type of the quota to be returned.

    ReturnedQuota - Number of bytes returned.

Return Value:

    NONE.

Environment:

    Kernel mode, QUOTA SPIN LOCK HELD!!

--*/

{

    if (PoolType == PagedPool) {
        MmTotalPagedPoolQuota -= ReturnedQuota;
    } else {
        MmTotalNonPagedPoolQuota -= ReturnedQuota;
    }

    return;
}
