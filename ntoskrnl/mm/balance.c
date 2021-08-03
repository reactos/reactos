/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/balance.c
 * PURPOSE:         kernel memory managment functions
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 *                  Cameron Gutman (cameron.gutman@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#include "ARM3/miarm.h"

/* GLOBALS ******************************************************************/

static ULONG MiMinimumAvailablePages = 256;
static CLIENT_ID MiBalancerThreadId;
static HANDLE MiBalancerThreadHandle = NULL;
static KEVENT MiBalancerEvent;
static KTIMER MiBalancerTimer;
static ULONG MiPagesUsed;

static LONG PageOutThreadActive;

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
MmReleasePageMemoryConsumer(ULONG Consumer, PFN_NUMBER Page)
{
    KIRQL OldIrql;

    if (Page == 0)
    {
        DPRINT1("Tried to release page zero.\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    (void)InterlockedDecrementUL(&MiPagesUsed);

    OldIrql = MiAcquirePfnLock();

    MmDereferencePage(Page);

    MiReleasePfnLock(OldIrql);

    return(STATUS_SUCCESS);
}

static
NTSTATUS
MmTrimUserMemory(ULONG Target, ULONG Priority, PULONG NrFreedPages)
{
    PFN_NUMBER CurrentPage;
    NTSTATUS Status;

    (*NrFreedPages) = 0;

    DPRINT1("MM BALANCER: %s\n", Priority ? "Paging out!" : "Removing access bit!");

    CurrentPage = MmGetLRUFirstUserPage();
    while (CurrentPage != 0 && Target > 0)
    {
        if (Priority)
        {
            Status = MmPageOutPhysicalAddress(CurrentPage);
            if (NT_SUCCESS(Status))
            {
                DPRINT("Succeeded\n");
                Target--;
                (*NrFreedPages)++;
            }
        }
        else
        {
            /* When not paging-out agressively, just reset the accessed bit */
            PEPROCESS Process = NULL;
            PVOID Address = NULL;
            BOOLEAN Accessed = FALSE;

            /*
             * We have a lock-ordering problem here. We cant lock the PFN DB before the Process address space.
             * So we must use circonvoluted loops.
             * Well...
             */
            while (TRUE)
            {
                KAPC_STATE ApcState;
                KIRQL OldIrql = MiAcquirePfnLock();
                PMM_RMAP_ENTRY Entry = MmGetRmapListHeadPage(CurrentPage);
                while (Entry)
                {
                    if (RMAP_IS_SEGMENT(Entry->Address))
                    {
                        Entry = Entry->Next;
                        continue;
                    }

                    /* Check that we didn't treat this entry before */
                    if (Entry->Address < Address)
                    {
                        Entry = Entry->Next;
                        continue;
                    }

                    if ((Entry->Address == Address) && (Entry->Process <= Process))
                    {
                        Entry = Entry->Next;
                        continue;
                    }

                    break;
                }

                if (!Entry)
                {
                    MiReleasePfnLock(OldIrql);
                    break;
                }

                Process = Entry->Process;
                Address = Entry->Address;

                ObReferenceObject(Process);

                if (!ExAcquireRundownProtection(&Process->RundownProtect))
                {
                    ObDereferenceObject(Process);
                    MiReleasePfnLock(OldIrql);
                    continue;
                }

                MiReleasePfnLock(OldIrql);

                KeStackAttachProcess(&Process->Pcb, &ApcState);
                MiLockProcessWorkingSet(Process, PsGetCurrentThread());

                /* Be sure this is still valid. */
                if (MmIsAddressValid(Address))
                {
                    PMMPTE Pte = MiAddressToPte(Address);
                    Accessed = Accessed || Pte->u.Hard.Accessed;
                    Pte->u.Hard.Accessed = 0;

                    /* There is no need to invalidate, the balancer thread is never on a user process */
                    //KeInvalidateTlbEntry(Address);
                }

                MiUnlockProcessWorkingSet(Process, PsGetCurrentThread());

                KeUnstackDetachProcess(&ApcState);
                ExReleaseRundownProtection(&Process->RundownProtect);
                ObDereferenceObject(Process);
            }

            if (!Accessed)
            {
                /* Nobody accessed this page since the last time we check. Time to clean up */

                Status = MmPageOutPhysicalAddress(CurrentPage);
                // DPRINT1("Paged-out one page: %s\n", NT_SUCCESS(Status) ? "Yes" : "No");
                (void)Status;
            }

            /* Done for this page. */
            Target--;
        }

        CurrentPage = MmGetLRUNextUserPage(CurrentPage, TRUE);
    }

    if (CurrentPage)
    {
        KIRQL OldIrql = MiAcquirePfnLock();
        MmDereferencePage(CurrentPage);
        MiReleasePfnLock(OldIrql);
    }

    return STATUS_SUCCESS;
}

static
ULONG
MiTrimMemory(ULONG InitialTarget)
{
    ULONG Target = InitialTarget;
    ULONG NrFreedPages = 0;
    NTSTATUS Status;

    if (MmAvailablePages < MiMinimumAvailablePages)
    {
        /* Global page limit exceeded */
        Target = (ULONG)max(Target, MiMinimumAvailablePages - MmAvailablePages);
    }

    if (Target)
    {
        /* Now swap the pages out */
        Status = MmTrimUserMemory(Target, MmAvailablePages < MiMinimumAvailablePages, &NrFreedPages);

        DPRINT("Trimming memory from legacy MM: Freed %lu pages with a target of %lu pages\n", NrFreedPages, Target);

        if (!NT_SUCCESS(Status))
        {
            KeBugCheck(MEMORY_MANAGEMENT);
        }
    }

    /* Return the page count needed to be freed to meet the initial target */
    return (InitialTarget > NrFreedPages) ? (InitialTarget - NrFreedPages) : 0;
}

VOID
NTAPI
MmRebalanceMemoryConsumers(VOID)
{
    // if (InterlockedCompareExchange(&PageOutThreadActive, 0, 1) == 0)
    {
        KeSetEvent(&MiBalancerEvent, IO_NO_INCREMENT, FALSE);
    }
}

NTSTATUS
NTAPI
MmRequestPageMemoryConsumer(ULONG Consumer, BOOLEAN Canwait, PPFN_NUMBER AllocatedPage)
{
    PFN_NUMBER Page;

    /* Update the target */
    InterlockedIncrementUL(&MiPagesUsed);

    /*
     * Actually allocate the page.
     */
    Page = MmAllocPage(0);
    if (Page == 0)
    {
        KeBugCheck(NO_PAGES_AVAILABLE);
    }
    *AllocatedPage = Page;

    return(STATUS_SUCCESS);
}


VOID NTAPI
MiBalancerThread(PVOID Unused)
{
    PVOID WaitObjects[2];
    NTSTATUS Status;

    WaitObjects[0] = &MiBalancerEvent;
    WaitObjects[1] = &MiBalancerTimer;

    while (1)
    {
        Status = KeWaitForMultipleObjects(2,
                                          WaitObjects,
                                          WaitAny,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL,
                                          NULL);

        if (Status == STATUS_WAIT_0 || Status == STATUS_WAIT_1)
        {
            ULONG InitialTarget = 0;

            do
            {
                ULONG OldTarget = InitialTarget;

                /* Trim each consumer */
                InitialTarget = MiTrimMemory(InitialTarget);

                /* No pages left to swap! */
                if (InitialTarget != 0 &&
                        InitialTarget == OldTarget)
                {
                    /* Game over */
                    KeBugCheck(NO_PAGES_AVAILABLE);
                }
            }
            while (InitialTarget != 0);

            if (Status == STATUS_WAIT_0)
                InterlockedDecrement(&PageOutThreadActive);
        }
        else
        {
            DPRINT1("KeWaitForMultipleObjects failed, status = %x\n", Status);
            KeBugCheck(MEMORY_MANAGEMENT);
        }
    }
}

CODE_SEG("INIT")
VOID
NTAPI
MiInitBalancerThread(VOID)
{
    KPRIORITY Priority;
    NTSTATUS Status;
    LARGE_INTEGER Timeout;

    KeInitializeEvent(&MiBalancerEvent, SynchronizationEvent, FALSE);
    KeInitializeTimerEx(&MiBalancerTimer, SynchronizationTimer);

    Timeout.QuadPart = -20000000; /* 2 sec */
    KeSetTimerEx(&MiBalancerTimer,
                 Timeout,
                 2000,         /* 2 sec */
                 NULL);

    Status = PsCreateSystemThread(&MiBalancerThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  NULL,
                                  NULL,
                                  &MiBalancerThreadId,
                                  MiBalancerThread,
                                  NULL);
    if (!NT_SUCCESS(Status))
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    Priority = LOW_REALTIME_PRIORITY + 1;
    NtSetInformationThread(MiBalancerThreadHandle,
                           ThreadPriority,
                           &Priority,
                           sizeof(Priority));

}


/* EOF */
