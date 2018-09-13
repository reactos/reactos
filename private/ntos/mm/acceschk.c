/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   acceschk.c

Abstract:

    This module contains the access check routines for memory management.

Author:

    Lou Perazzoli (loup) 10-Apr-1989

Revision History:

--*/

#include "mi.h"

#if defined(_WIN64)
#include "wow64t.h"
#endif

//
// MmReadWrite yields 0 if no-access, 10 if read-only, 11 if read-write.
// It is indexed by a page protection.  The value of this array is added
// to the !WriteOperation value.  If the value is 10 or less an access
// violation is issued (read-only - write_operation) = 9,
// (read_only - read_operation) = 10, etc.
//

CCHAR MmReadWrite[32] = {1, 10, 10, 10, 11, 11, 11, 11,
                         1, 10, 10, 10, 11, 11, 11, 11,
                         1, 10, 10, 10, 11, 11, 11, 11,
                         1, 10, 10, 10, 11, 11, 11, 11 };


NTSTATUS
MiAccessCheck (
    IN PMMPTE PointerPte,
    IN BOOLEAN WriteOperation,
    IN KPROCESSOR_MODE PreviousMode,
    IN ULONG Protection,
    IN BOOLEAN CallerHoldsPfnLock
    )

/*++

Routine Description:



Arguments:

    PointerPte - Supplies the pointer to the PTE which caused the
                 page fault.

    WriteOperation - Supplies 1 if the operation is a write, 0 if
                     the operation is a read.

    PreviousMode - Supplies the previous mode, one of UserMode or KernelMode.

    Protection - Supplies the protection mask to check.

    CallerHoldsPfnLock - Supplies TRUE if the PFN lock is held, FALSE otherwise.

Return Value:

    Returns TRUE if access to the page is allowed, FALSE otherwise.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    MMPTE PteContents;
    KIRQL OldIrql;
    PMMPFN Pfn1;

    //
    // Check to see if the owner bit allows access to the previous mode.
    // Access is not allowed if the owner is kernel and the previous
    // mode is user.  Access is also disallowed if the write operation
    // is true and the write field in the PTE is false.
    //

    //
    // If both an access violation and a guard page violation could
    // occur for the page, the access violation must be returned.
    //

    if (PreviousMode == UserMode) {
        if (PointerPte > MiHighestUserPte) {
            return STATUS_ACCESS_VIOLATION;
        }
    }

    PteContents = *PointerPte;

    if (PteContents.u.Hard.Valid == 1) {

        //
        // Valid pages cannot be guard page violations.
        //

        if (WriteOperation) {
            if ((PteContents.u.Hard.Write == 1) ||
                (PteContents.u.Hard.CopyOnWrite == 1)) {
                return STATUS_SUCCESS;
            } else {
                return STATUS_ACCESS_VIOLATION;
            }
        } else {
            return STATUS_SUCCESS;
        }

    } else {

        if ((MmReadWrite[Protection] - (CCHAR)WriteOperation) < 10) {
            return STATUS_ACCESS_VIOLATION;
        } else {

            //
            // Check for a guard page fault.
            //

            if (Protection & MM_GUARD_PAGE) {

                //
                // If this thread is attached to a different process,
                // return an access violation rather than a guard
                // page exception.  The prevents problems with unwanted
                // stack expansion and unexpected guard page behavior
                // from debuggers.

                if (KeIsAttachedProcess()) {
                    return STATUS_ACCESS_VIOLATION;
                }

                //
                // Check to see if this is a transition PTE, if so,
                // the PFN database original contents field needs to be
                // updated.
                //

                if ((PteContents.u.Soft.Transition == 1) &&
                    (PteContents.u.Soft.Prototype == 0)) {

                    //
                    // Acquire the PFN mutex and check to see if the
                    // PTE is still in the transition state, and, if so
                    // update the original PTE in the pfn database.
                    //

                    if (CallerHoldsPfnLock == FALSE) {
                        LOCK_PFN (OldIrql);
                    }
                    PteContents = *(volatile MMPTE *)PointerPte;
                    if ((PteContents.u.Soft.Transition == 1) &&
                        (PteContents.u.Soft.Prototype == 0)) {

                        //
                        // Still in transition, update the PFN database.
                        //

                        Pfn1 = MI_PFN_ELEMENT (
                                    PteContents.u.Trans.PageFrameNumber);

                        ASSERT (Pfn1->u3.e1.PrototypePte == 0);
                        Pfn1->OriginalPte.u.Soft.Protection =
                                                  Protection & ~MM_GUARD_PAGE;
                    }
                    if (CallerHoldsPfnLock == FALSE) {
                        UNLOCK_PFN (OldIrql);
                    }
                }

                PointerPte->u.Soft.Protection = Protection & ~MM_GUARD_PAGE;

                return STATUS_GUARD_PAGE_VIOLATION;
            }
            return STATUS_SUCCESS;
        }
    }
}

NTSTATUS
FASTCALL
MiCheckForUserStackOverflow (
    IN PVOID FaultingAddress
    )

/*++

Routine Description:

    This routine checks to see if the faulting address is within
    the stack limits and if so tries to create another guard
    page on the stack.  A stack over flow is returned if the
    creation of a new guard page fails or if the stack is in
    the following form:


    stack   +----------------+
    growth  |                |  StackBase
      |     +----------------+
      v     |                |
            |   allocated    |
            |                |
            |    ...         |
            |                |
            +----------------+
            | old guard page | <- faulting address is in this page.
            +----------------+
            |                |
            +----------------+
            |                | last page of stack (always no access)
            +----------------+

    In this case, the page before the last page is committed, but
    not as a guard page and a STACK_OVERFLOW condition is returned.

Arguments:

    FaultingAddress - Supplies the virtual address of the page which
                      was a guard page.

Return Value:

    None.

Environment:

    Kernel mode. No mutexes held.

--*/

{
    PTEB Teb;
    ULONG_PTR NextPage;
    SIZE_T RegionSize;
    NTSTATUS status;
    KIRQL OldIrql;
    PMMLOCK_CONFLICT Next;
    PVOID DeallocationStack;
    PVOID *StackLimit;

#if defined(WX86) || defined(_AXP64_)
    PWX86TIB  Wx86Tib;
#endif
#if defined(_WIN64)
    PTEB32 Teb32;
#endif

    //
    // Make sure we are not recursing with the address space or
    // working set lock held.
    //

    if (!IsListEmpty (&MmLockConflictList)) {
        ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);
        Next = (PMMLOCK_CONFLICT)MmLockConflictList.Flink;

        while ((PVOID)Next != &MmLockConflictList) {

            if (Next->Thread == PsGetCurrentThread()) {
                ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);
                return STATUS_GUARD_PAGE_VIOLATION;
            }
            Next = (PMMLOCK_CONFLICT)Next->List.Flink;
        }
        ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);
    }

    //
    // Create an exception handler as the TEB is within the user's
    // address space.
    //

    try {

        Teb = NtCurrentTeb();

#if defined(_IA64_)

      if ((Teb->NtTib.StackBase <= FaultingAddress) &&
          (Teb->DeallocationBStore > FaultingAddress)) {

        //
        // check to see if the faulting address is within
        // the bstore limits and if so tries to create another guard
        // page on the bstore.
        //
        //
        //          +----------------+
        //          |                | last page of stack (always no access)
        //          +----------------+
        //          |                |
        //          |                |
        //          |                |
        //          +----------------+
        //          | old guard page | <- faulting address is in this page.               |
        //          +----------------+
        //  bstore  |                |
        //  growth  |    ......      |
        //          |                |
        //    ^     |   allocated    |
        //    |     |                |  StackBase
        //          +----------------+
        //
        //

        NextPage = (ULONG_PTR)PAGE_ALIGN(FaultingAddress) + PAGE_SIZE;

        RegionSize = PAGE_SIZE;

        if ((NextPage + PAGE_SIZE) >= (ULONG_PTR)PAGE_ALIGN(Teb->DeallocationBStore)) {

            //
            // There is no more room for expansion, attempt to
            // commit the page before the last page of the
            // stack.
            //

            NextPage = (ULONG_PTR)PAGE_ALIGN(Teb->DeallocationBStore) - PAGE_SIZE;

            status = ZwAllocateVirtualMemory (NtCurrentProcess(),
                                              (PVOID *)&NextPage,
                                              0,
                                              &RegionSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE);
            if ( NT_SUCCESS(status) ) {
                Teb->BStoreLimit = (PVOID)( (PUCHAR)NextPage);
            }

            return STATUS_STACK_OVERFLOW;
        }

        Teb->BStoreLimit = (PVOID)((PUCHAR)(NextPage));

      } else {

#endif

        DeallocationStack = Teb->DeallocationStack;
        StackLimit = &Teb->NtTib.StackLimit;

        //
        // The stack base and the stack limit are both within the stack.
        //

        if ((Teb->NtTib.StackBase <= FaultingAddress) ||
            (DeallocationStack > FaultingAddress)) {

#if defined(WX86)
            //
            // Also check the Wx86 i386 stack on risc.
            //
            Wx86Tib = Teb->Vdm;
            if (Wx86Tib) {
                ProbeForRead(Wx86Tib, sizeof(WX86TIB), sizeof(ULONG));
                if (Wx86Tib->Size == sizeof(WX86TIB) &&
                    Wx86Tib->StackBase > FaultingAddress &&
                    Wx86Tib->DeallocationStack <= FaultingAddress) {

                    DeallocationStack = Wx86Tib->DeallocationStack;
                    StackLimit = &Wx86Tib->StackLimit;
                } else {
                    //
                    // Not within the stack.
                    //

                    return STATUS_GUARD_PAGE_VIOLATION;
                }
            } else
#endif
#if defined(_WIN64)
            //
            // Also check for the 32-bit native stack on NT64
            //
            if ((Teb32 = (PTEB32)Teb->NtTib.ExceptionList) != NULL) {
                ProbeForRead(Teb32, sizeof(TEB32), sizeof(ULONG));
                if ((ULONG_PTR)Teb32->NtTib.StackBase > (ULONG_PTR)FaultingAddress &&
                    (ULONG_PTR)Teb32->DeallocationStack <= (ULONG_PTR)FaultingAddress) {
                    DeallocationStack = (PVOID)ULongToPtr(Teb32->DeallocationStack);

                    StackLimit = (PVOID *)&Teb32->NtTib.StackLimit;
                } else
#if defined(_AXP64_)
                //
                // Also check the Wx86 i386 stack on risc.
                //
                if (Wx86Tib = (PWX86TIB)ULongToPtr(Teb32->Vdm)) {
                    ProbeForRead(Wx86Tib, sizeof(WX86TIB), sizeof(ULONG));
                    if (Wx86Tib->Size == sizeof(WX86TIB) &&
                        (ULONG_PTR)Wx86Tib->StackBase > (ULONG_PTR)FaultingAddress &&
                        (ULONG_PTR)Wx86Tib->DeallocationStack <= (ULONG_PTR)FaultingAddress) {

                        DeallocationStack = Wx86Tib->DeallocationStack;
                        StackLimit = (PVOID *)(&Wx86Tib->StackLimit);
                    } else {
                        //
                        // Not within the stack.
                        //

                        return STATUS_GUARD_PAGE_VIOLATION;
                    }
                } else
#endif
                {
                    //
                    // Not within the stack.
                    //

                    return STATUS_GUARD_PAGE_VIOLATION;
                }
            } else
#endif
              {
                //
                // Not within the stack.
                //

                return STATUS_GUARD_PAGE_VIOLATION;
            }
        }

        //
        // This address is within the current stack, check to see
        // if there is ample room for another guard page and
        // if so attempt to commit a new guard page.
        //

        NextPage = ((ULONG_PTR)PAGE_ALIGN(FaultingAddress) - PAGE_SIZE);

        RegionSize = PAGE_SIZE;

        if ((NextPage - PAGE_SIZE) <= (ULONG_PTR)PAGE_ALIGN(DeallocationStack)) {

            //
            // There is no more room for expansion, attempt to
            // commit the page before the last page of the
            // stack.
            //

            NextPage = (ULONG_PTR)PAGE_ALIGN(DeallocationStack) + PAGE_SIZE;

            status = ZwAllocateVirtualMemory (NtCurrentProcess(),
                                              (PVOID *)&NextPage,
                                              0,
                                              &RegionSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE);
            if ( NT_SUCCESS(status) ) {

#if defined(_WIN64)
                if (Teb32) {
                    // update the 32-bit stacklimit
                    *(ULONG *)StackLimit = PtrToUlong((PUCHAR)NextPage);
                } else {
                    *StackLimit = (PVOID)( (PUCHAR)NextPage);
                }
#else
                *StackLimit = (PVOID)( (PUCHAR)NextPage);
#endif

            }

            return STATUS_STACK_OVERFLOW;
        }
#if defined(_WIN64)
        if (Teb32) {
            // Update the 32-bit stacklimit
            *(ULONG *)StackLimit = PtrToUlong((PUCHAR)(NextPage + PAGE_SIZE));
        } else {
            *StackLimit = (PVOID)((PUCHAR)(NextPage + PAGE_SIZE));
        }
#else
        *StackLimit = (PVOID)((PUCHAR)(NextPage + PAGE_SIZE));
#endif

#if defined(_IA64_)
      }
#endif // _IA64_

retry:
        status = ZwAllocateVirtualMemory (NtCurrentProcess(),
                                          (PVOID *)&NextPage,
                                          0,
                                          &RegionSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE | PAGE_GUARD);


        if (NT_SUCCESS(status) || (status == STATUS_ALREADY_COMMITTED)) {

            //
            // The guard page is now committed or stack space is
            // already present, return success.
            //

            return STATUS_PAGE_FAULT_GUARD_PAGE;
        }

        if (PsGetCurrentProcess() == ExpDefaultErrorPortProcess) {

            //
            // Don't let CSRSS process get any stack overflows due to
            // commitment.  Increase the commitment by a page and
            // try again.
            //

            ASSERT (status == STATUS_COMMITMENT_LIMIT);

            ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);
            MmTotalCommitLimit += 1;
            MmExtendedCommit += 1;
            ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);
            goto retry;
        }

        return STATUS_STACK_OVERFLOW;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception has occurred during the referencing of the
        // TEB or TIB, just return a guard page violation and
        // don't deal with the stack overflow.
        //

        return STATUS_GUARD_PAGE_VIOLATION;
    }
}
