/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    vadtree.c

Abstract:

    This module contains the routine to manipulate the virtual address
    descriptor tree.

Author:

    Lou Perazzoli (loup) 19-May-1989
    Landy Wang (landyw) 02-June-1997

Environment:

    Kernel mode only, working set mutex held, APCs disabled.

Revision History:

--*/

#include "mi.h"

VOID
MiInsertVad (
    IN PMMVAD Vad
    )

/*++

Routine Description:

    This function inserts a virtual address descriptor into the tree and
    reorders the splay tree as appropriate.

Arguments:

    Vad - Supplies a pointer to a virtual address descriptor


Return Value:

    None - An exception is raised if quota is exceeded.

--*/

{
    PMMADDRESS_NODE *Root;
    PEPROCESS CurrentProcess;
    SIZE_T RealCharge;
    SIZE_T PageCharge;
    SIZE_T PagedQuotaCharged;
    ULONG FirstPage;
    ULONG LastPage;
    SIZE_T PagedPoolCharge;
    LOGICAL ChargedPageFileQuota;
    LOGICAL ChargedJobCommit;

    ASSERT (Vad->EndingVpn >= Vad->StartingVpn);

    CurrentProcess = PsGetCurrentProcess();

    //
    // Commit charge of MAX_COMMIT means don't charge quota.
    //

    if (Vad->u.VadFlags.CommitCharge != MM_MAX_COMMIT) {

        PageCharge = 0;
        PagedQuotaCharged = 0;
        ChargedPageFileQuota = FALSE;
        ChargedJobCommit = FALSE;

        //
        // Charge quota for the nonpaged pool for the VAD.  This is
        // done here rather than by using ExAllocatePoolWithQuota
        // so the process object is not referenced by the quota charge.
        //

        PsChargePoolQuota (CurrentProcess, NonPagedPool, sizeof(MMVAD));

        try {

            //
            // Charge quota for the prototype PTEs if this is a mapped view.
            //

            if ((Vad->u.VadFlags.PrivateMemory == 0) &&
                (Vad->ControlArea != NULL)) {
                PagedPoolCharge =
                  (Vad->EndingVpn - Vad->StartingVpn) << PTE_SHIFT;
                PsChargePoolQuota (CurrentProcess, PagedPool, PagedPoolCharge);
                PagedQuotaCharged = PagedPoolCharge;
            }

#if !defined (_WIN64)
            //
            // Add in the charge for page table pages.
            //

            FirstPage = MiGetPpePdeOffset (MI_VPN_TO_VA (Vad->StartingVpn));
            LastPage = MiGetPpePdeOffset (MI_VPN_TO_VA (Vad->EndingVpn));

            while (FirstPage <= LastPage) {

                if (!MI_CHECK_BIT (MmWorkingSetList->CommittedPageTables,
                                   FirstPage)) {
                    PageCharge += 1;
                }
                FirstPage += 1;
            }
#endif

            RealCharge = Vad->u.VadFlags.CommitCharge + PageCharge;

            if (RealCharge != 0) {

                MiChargePageFileQuota (RealCharge, CurrentProcess);
                ChargedPageFileQuota = TRUE;

#if 0 //commented out so page file quota is meaningful.
                if (Vad->u.VadFlags.PrivateMemory == 0) {

                    if ((Vad->ControlArea->FilePointer == NULL) &&
                        (Vad->u.VadFlags.PhysicalMapping == 0)) {

                        //
                        // Don't charge commitment for the page file space
                        // occupied by a page file section.  This will be
                        // charged as the shared memory is committed.
                        //

                        RealCharge -= 1 + (Vad->EndingVpn -
                                                     Vad->StartingVpn);
                    }
                }
#endif //0
                if (CurrentProcess->CommitChargeLimit) {
                    if (CurrentProcess->CommitCharge + RealCharge > CurrentProcess->CommitChargeLimit) {
                        if (CurrentProcess->Job) {
                            PsReportProcessMemoryLimitViolation ();
                        }
                        ExRaiseStatus (STATUS_COMMITMENT_LIMIT);
                    }
                }
                if (CurrentProcess->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
                    if (PsChangeJobMemoryUsage(RealCharge) == FALSE) {
                        ExRaiseStatus (STATUS_COMMITMENT_LIMIT);
                    }
                    ChargedJobCommit = TRUE;
                }

                if (MiChargeCommitment (RealCharge, CurrentProcess) == FALSE) {
                    ExRaiseStatus (STATUS_COMMITMENT_LIMIT);
                }

                CurrentProcess->CommitCharge += RealCharge;
                if (CurrentProcess->CommitCharge > CurrentProcess->CommitChargePeak) {
                    CurrentProcess->CommitChargePeak = CurrentProcess->CommitCharge;
                }
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // Return any quotas charged thus far.
            //

            PsReturnPoolQuota (CurrentProcess, NonPagedPool, sizeof(MMVAD));

            if (PagedQuotaCharged != 0) {
                PsReturnPoolQuota (CurrentProcess, PagedPool, PagedPoolCharge);
            }

            if (ChargedPageFileQuota == TRUE) {

                MiReturnPageFileQuota (RealCharge,
                                       CurrentProcess);
            }

            if (ChargedJobCommit == TRUE) {

                //
                // Temporarily up the process commit charge as the
                // job code will be referencing it as though everything
                // has succeeded.
                //

                CurrentProcess->CommitCharge += RealCharge;
                PsChangeJobMemoryUsage(-(SSIZE_T)RealCharge);
                CurrentProcess->CommitCharge -= RealCharge;
            }

            ExRaiseStatus (GetExceptionCode());
        }

        MM_TRACK_COMMIT (MM_DBG_COMMIT_INSERT_VAD, RealCharge);

#if !defined (_WIN64)
        if (PageCharge != 0) {

            //
            // Since the commitment was successful, charge the page
            // table pages.
            //

            FirstPage = MiGetPpePdeOffset (MI_VPN_TO_VA (Vad->StartingVpn));

            while (FirstPage <= LastPage) {

                if (!MI_CHECK_BIT (MmWorkingSetList->CommittedPageTables,
                                   FirstPage)) {
                    MI_SET_BIT (MmWorkingSetList->CommittedPageTables,
                                FirstPage);
                    MmWorkingSetList->NumberOfCommittedPageTables += 1;
#if defined (_X86PAE_)
                    ASSERT (MmWorkingSetList->NumberOfCommittedPageTables <
                                                 PD_PER_SYSTEM * PDE_PER_PAGE);
#else
                    ASSERT (MmWorkingSetList->NumberOfCommittedPageTables <
                                                                 PDE_PER_PAGE);
#endif
                }
                FirstPage += 1;
            }
        }
#endif
    }

    Root = (PMMADDRESS_NODE *)&CurrentProcess->VadRoot;

    //
    // Set the hint field in the process to this Vad.
    //

    CurrentProcess->VadHint = Vad;

    if (CurrentProcess->VadFreeHint != NULL) {
        if (((ULONG)((PMMVAD)CurrentProcess->VadFreeHint)->EndingVpn +
                MI_VA_TO_VPN (X64K)) >=
                Vad->StartingVpn) {
            CurrentProcess->VadFreeHint = Vad;
        }
    }

    MiInsertNode ( (PMMADDRESS_NODE)Vad, Root);
    return;
}


VOID
MiRemoveVad (
    IN PMMVAD Vad
    )

/*++

Routine Description:

    This function removes a virtual address descriptor from the tree and
    reorders the splay tree as appropriate.  If any quota or commitment
    was charged by the VAD (as indicated by the CommitCharge field) it
    is released.

Arguments:

    Vad - Supplies a pointer to a virtual address descriptor.

Return Value:

    None.

--*/

{
    PMMADDRESS_NODE *Root;
    PEPROCESS CurrentProcess;
    SIZE_T RealCharge;
    PLIST_ENTRY Next;
    PMMSECURE_ENTRY Entry;

    CurrentProcess = PsGetCurrentProcess();


    //
    // Commit charge of MAX_COMMIT means don't charge quota.
    //

    if (Vad->u.VadFlags.CommitCharge != MM_MAX_COMMIT) {

        //
        // Return the quota charge to the process.
        //

        PsReturnPoolQuota (CurrentProcess, NonPagedPool, sizeof(MMVAD));

        if ((Vad->u.VadFlags.PrivateMemory == 0) &&
            (Vad->ControlArea != NULL)) {
            PsReturnPoolQuota (CurrentProcess,
                               PagedPool,
                               (Vad->EndingVpn - Vad->StartingVpn) << PTE_SHIFT);
        }

        RealCharge = Vad->u.VadFlags.CommitCharge;

        if (RealCharge != 0) {

            MiReturnPageFileQuota (RealCharge, CurrentProcess);

            if ((Vad->u.VadFlags.PrivateMemory == 0) &&
                (Vad->ControlArea != NULL)) {

#if 0 //commented out so page file quota is meaningful.
                if (Vad->ControlArea->FilePointer == NULL) {

                    //
                    // Don't release commitment for the page file space
                    // occupied by a page file section.  This will be charged
                    // as the shared memory is committed.
                    //

                    RealCharge -= BYTES_TO_PAGES ((ULONG)Vad->EndingVa -
                                                   (ULONG)Vad->StartingVa);
                }
#endif
            }

            MiReturnCommitment (RealCharge);
            MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_VAD, RealCharge);
            if (CurrentProcess->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
                PsChangeJobMemoryUsage(-(SSIZE_T)RealCharge);
            }
            CurrentProcess->CommitCharge -= RealCharge;
        }
    }

    if (Vad == CurrentProcess->VadFreeHint) {
        CurrentProcess->VadFreeHint = MiGetPreviousVad (Vad);
    }

    Root = (PMMADDRESS_NODE *)&CurrentProcess->VadRoot;

    MiRemoveNode ( (PMMADDRESS_NODE)Vad, Root);

    if (Vad->u.VadFlags.NoChange) {
        if (Vad->u2.VadFlags2.MultipleSecured) {

           //
           // Free the oustanding pool allocations.
           //

            Next = Vad->u3.List.Flink;
            do {
                Entry = CONTAINING_RECORD( Next,
                                           MMSECURE_ENTRY,
                                           List);

                Next = Entry->List.Flink;
                ExFreePool (Entry);
            } while (Next != &Vad->u3.List);
        }
    }

    //
    // If the VadHint was the removed Vad, change the Hint.

    if (CurrentProcess->VadHint == Vad) {
        CurrentProcess->VadHint = CurrentProcess->VadRoot;
    }

    return;
}

PMMVAD
FASTCALL
MiLocateAddress (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    The function locates the virtual address descriptor which describes
    a given address.

Arguments:

    VirtualAddress - Supplies the virtual address to locate a descriptor
                     for.

Return Value:

    Returns a pointer to the virtual address descriptor which contains
    the supplied virtual address or NULL if none was located.

--*/

{
    PMMVAD FoundVad;
    PEPROCESS CurrentProcess;
    ULONG_PTR Vpn;

    CurrentProcess = PsGetCurrentProcess();

    if (CurrentProcess->VadHint == NULL) {
        return NULL;
    }

    Vpn = MI_VA_TO_VPN (VirtualAddress);
    if ((Vpn >= ((PMMADDRESS_NODE)CurrentProcess->VadHint)->StartingVpn) &&
        (Vpn <= ((PMMADDRESS_NODE)CurrentProcess->VadHint)->EndingVpn)) {

        return (PMMVAD)CurrentProcess->VadHint;
    }

    FoundVad = (PMMVAD)MiLocateAddressInTree ( Vpn,
                   (PMMADDRESS_NODE *)&(CurrentProcess->VadRoot));

    if (FoundVad != NULL) {
        CurrentProcess->VadHint = (PVOID)FoundVad;
    }
    return FoundVad;
}

PVOID
MiFindEmptyAddressRange (
    IN SIZE_T SizeOfRange,
    IN ULONG_PTR Alignment,
    IN ULONG QuickCheck
    )

/*++

Routine Description:

    The function examines the virtual address descriptors to locate
    an unused range of the specified size and returns the starting
    address of the range.

Arguments:

    SizeOfRange - Supplies the size in bytes of the range to locate.

    Alignment - Supplies the alignment for the address.  Must be
                 a power of 2 and greater than the page_size.

    QuickCheck - Supplies a zero if a quick check for free memory
                 after the VadFreeHint exists, non-zero if checking
                 should start at the lowest address.

Return Value:

    Returns the starting address of a suitable range.

--*/

{
    PMMVAD NextVad;
    PMMVAD FreeHint;
    PEPROCESS CurrentProcess;
    PVOID StartingVa;
    PVOID EndingVa;

    CurrentProcess = PsGetCurrentProcess();
    FreeHint = CurrentProcess->VadFreeHint;

    if ((QuickCheck == 0) && (FreeHint != NULL)) {

        EndingVa = MI_VPN_TO_VA_ENDING (FreeHint->EndingVpn);
        NextVad = MiGetNextVad (FreeHint);
        if (NextVad == NULL) {

            if (SizeOfRange <
                (((ULONG_PTR)MM_HIGHEST_USER_ADDRESS + 1) -
                         MI_ROUND_TO_SIZE((ULONG_PTR)EndingVa, Alignment))) {
                return (PMMADDRESS_NODE)MI_ROUND_TO_SIZE((ULONG_PTR)EndingVa,
                                                         Alignment);
            }
        } else {
            StartingVa = MI_VPN_TO_VA (NextVad->StartingVpn);

            if (SizeOfRange <
                ((ULONG_PTR)StartingVa -
                         MI_ROUND_TO_SIZE((ULONG_PTR)EndingVa, Alignment))) {

                //
                // Check to ensure that the ending address aligned upwards
                // is not greater than the starting address.
                //

                if ((ULONG_PTR)StartingVa >
                         MI_ROUND_TO_SIZE((ULONG_PTR)EndingVa,Alignment)) {
                    return (PMMADDRESS_NODE)MI_ROUND_TO_SIZE((ULONG_PTR)EndingVa,
                                                           Alignment);
                }
            }
        }
    }

    return (PMMVAD)MiFindEmptyAddressRangeInTree (
                   SizeOfRange,
                   Alignment,
                   (PMMADDRESS_NODE)(CurrentProcess->VadRoot),
                   (PMMADDRESS_NODE *)&CurrentProcess->VadFreeHint);

}

#if DBG
VOID
VadTreeWalk (
    PMMVAD Start
    )

{
    UNREFERENCED_PARAMETER (Start);

    NodeTreeWalk ( (PMMADDRESS_NODE)(PsGetCurrentProcess()->VadRoot));

    return;
}
#endif //DBG
