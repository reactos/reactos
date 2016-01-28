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

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitializeBalancer)
#pragma alloc_text(INIT, MmInitializeMemoryConsumer)
#pragma alloc_text(INIT, MiInitBalancerThread)
#endif

#define TEST_PAGING


/* GLOBALS ******************************************************************/

MM_MEMORY_CONSUMER MiMemoryConsumers[MC_MAXIMUM];
static ULONG MiMinimumAvailablePages;
static ULONG MiNrTotalPages;
static ULONG MiMinimumPagesPerRun;

static CLIENT_ID MiBalancerThreadId;
static HANDLE MiBalancerThreadHandle = NULL;
static KEVENT MiBalancerEvent;
static KTIMER MiBalancerTimer;

/* FUNCTIONS ****************************************************************/

VOID
INIT_FUNCTION
NTAPI
MmInitializeBalancer(ULONG NrAvailablePages, ULONG NrSystemPages)
{
   memset(MiMemoryConsumers, 0, sizeof(MiMemoryConsumers));

   MiNrTotalPages = NrAvailablePages;

   /* Set up targets. */
   MiMinimumAvailablePages = 128;
   MiMinimumPagesPerRun = 256;
    if ((NrAvailablePages + NrSystemPages) >= 8192)
    {
        MiMemoryConsumers[MC_CACHE].PagesTarget = NrAvailablePages / 4 * 3;
    }
    else if ((NrAvailablePages + NrSystemPages) >= 4096)
    {
        MiMemoryConsumers[MC_CACHE].PagesTarget = NrAvailablePages / 3 * 2;
    }
    else
    {
        MiMemoryConsumers[MC_CACHE].PagesTarget = NrAvailablePages / 8;
    }
   MiMemoryConsumers[MC_USER].PagesTarget = NrAvailablePages - MiMinimumAvailablePages;
}

VOID
INIT_FUNCTION
NTAPI
MmInitializeMemoryConsumer(ULONG Consumer,
                           NTSTATUS (*Trim)(ULONG Target, ULONG Priority,
                                            PULONG NrFreed))
{
   MiMemoryConsumers[Consumer].Trim = Trim;
}

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

   if (MmGetReferenceCountPage(Page) == 1)
   {
      if(Consumer == MC_USER) MmRemoveLRUUserPage(Page);
      (void)InterlockedDecrementUL(&MiMemoryConsumers[Consumer].PagesUsed);
   }

   OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
   MmDereferencePage(Page);
   KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

   return(STATUS_SUCCESS);
}

ULONG
NTAPI
MiTrimMemoryConsumer(ULONG Consumer, ULONG InitialTarget)
{
    ULONG Target = InitialTarget;
    ULONG NrFreedPages = 0;
    NTSTATUS Status;

    /* Make sure we can trim this consumer */
    if (!MiMemoryConsumers[Consumer].Trim)
    {
        /* Return the unmodified initial target */
        return InitialTarget;
    }

    // if (MiFreeSwapPages == 0)
    {
        if (MiMemoryConsumers[Consumer].PagesUsed > MiMemoryConsumers[Consumer].PagesTarget)
        {
            /* Consumer page limit exceeded */
            Target = max(Target, MiMemoryConsumers[Consumer].PagesUsed - MiMemoryConsumers[Consumer].PagesTarget);
        }
    }
    if (MmAvailablePages < MiMinimumAvailablePages)
    {
        /* Global page limit exceeded */
        Target = (ULONG)max(Target, MiMinimumAvailablePages - MmAvailablePages);
    }

    if (Target)
    {
        /* Now swap the pages out */
        Status = MiMemoryConsumers[Consumer].Trim(Target, 0, &NrFreedPages);

        DPRINT("Trimming consumer %lu: Freed %lu pages with a target of %lu pages\n", Consumer, NrFreedPages, Target);

        if (!NT_SUCCESS(Status))
        {
            KeBugCheck(MEMORY_MANAGEMENT);
        }

        /* Update the target */
        if (NrFreedPages < Target)
            Target -= NrFreedPages;
        else
            Target = 0;

        /* Return the remaining pages needed to meet the target */
        return Target;
    }
    else
    {
        /* Initial target is zero and we don't have anything else to add */
        return 0;
    }
}

NTSTATUS
MmTrimUserMemory(ULONG Target, ULONG Priority, PULONG NrFreedPages)
{
    PFN_NUMBER CurrentPage;
    PFN_NUMBER NextPage;
    NTSTATUS Status;

    (*NrFreedPages) = 0;

    CurrentPage = MmGetLRUFirstUserPage();
    while (CurrentPage != 0 && Target > 0)
    {
        Status = MmPageOutPhysicalAddress(CurrentPage);
        if (NT_SUCCESS(Status))
        {
            DPRINT("Succeeded\n");
            Target--;
            (*NrFreedPages)++;
        }

        NextPage = MmGetLRUNextUserPage(CurrentPage);
        if (NextPage <= CurrentPage)
        {
            /* We wrapped around, so we're done */
            break;
        }
        CurrentPage = NextPage;
    }

    return STATUS_SUCCESS;
}

static BOOLEAN
MiIsBalancerThread(VOID)
{
   return (MiBalancerThreadHandle != NULL) &&
          (PsGetCurrentThreadId() == MiBalancerThreadId.UniqueThread);
}

VOID
NTAPI
MmRebalanceMemoryConsumers(VOID)
{
    if (MiBalancerThreadHandle != NULL &&
        !MiIsBalancerThread())
    {
        KeSetEvent(&MiBalancerEvent, IO_NO_INCREMENT, FALSE);
    }
}

NTSTATUS
NTAPI
MmRequestPageMemoryConsumer(ULONG Consumer, BOOLEAN CanWait,
                            PPFN_NUMBER AllocatedPage)
{
   ULONG PagesUsed;
   PFN_NUMBER Page;
   KIRQL OldIrql;

   /*
    * Make sure we don't exceed our individual target.
    */
   PagesUsed = InterlockedIncrementUL(&MiMemoryConsumers[Consumer].PagesUsed);
   if ((PagesUsed > MiMemoryConsumers[Consumer].PagesTarget) &&
       !MiIsBalancerThread() &&
       (MmNumberOfPagingFiles == 0))
   {
      MmRebalanceMemoryConsumers();
   }

   /*
    * Allocate always memory for the non paged pool and for the pager thread.
    */
   if ((Consumer == MC_SYSTEM) || MiIsBalancerThread())
   {
      OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
      Page = MmAllocPage(Consumer);
      KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
      if (Page == 0)
      {
         KeBugCheck(NO_PAGES_AVAILABLE);
      }
      if (Consumer == MC_USER) MmInsertLRULastUserPage(Page);
      *AllocatedPage = Page;
      return(STATUS_SUCCESS);
   }

   /*
    * Actually allocate the page.
    */
   OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
   Page = MmAllocPage(Consumer);
   KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
   if (Page == 0)
   {
      KeBugCheck(NO_PAGES_AVAILABLE);
   }
   if(Consumer == MC_USER) MmInsertLRULastUserPage(Page);
   *AllocatedPage = Page;

   return(STATUS_SUCCESS);
}

/* Old implementation of the balancer thread */
static
VOID
NTAPI
MiRosBalancerThread(VOID)
{
    ULONG InitialTarget = 0;

    DPRINT1("Legacy MM balancer in action!\n");

#if (_MI_PAGING_LEVELS == 2)
    if (!MiIsBalancerThread())
    {
        /* Clean up the unused PDEs */
        ULONG_PTR Address;
        PEPROCESS Process = PsGetCurrentProcess();

        /* Acquire PFN lock */
        KIRQL OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
        PMMPDE pointerPde;
        for (Address = (ULONG_PTR) MI_LOWEST_VAD_ADDRESS ;
                Address < (ULONG_PTR) MM_HIGHEST_VAD_ADDRESS; Address += (PAGE_SIZE * PTE_COUNT))
        {
            if (MiQueryPageTableReferences((PVOID) Address) == 0)
            {
                pointerPde = MiAddressToPde(Address);
                if (pointerPde->u.Hard.Valid)
                    MiDeletePte(pointerPde, MiPdeToPte(pointerPde), Process, NULL);
                ASSERT(pointerPde->u.Hard.Valid == 0);
            }
        }
        /* Release lock */
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    }
#endif
    do
    {
        ULONG OldTarget = InitialTarget;
        ULONG i;

        /* Trim each consumer */
        for (i = 0; i < MC_MAXIMUM; i++)
        {
            InitialTarget = MiTrimMemoryConsumer(i, InitialTarget);
        }

        /* No pages left to swap! */
        if (InitialTarget != 0 && InitialTarget == OldTarget)
        {
            /* Game over */
            KeBugCheck(NO_PAGES_AVAILABLE);
        }
    } while (InitialTarget != 0);
}

VOID NTAPI
MiBalancerThread(PVOID Unused)
{
    PVOID WaitObjects[2];
    NTSTATUS Status;
    ULONG i;
    PVOID LastPagedPoolAddress = MmPagedPoolStart;
#ifdef TEST_PAGING
    PULONG PagingTestBuffer;
    BOOLEAN TestPage = FALSE;
#endif

    WaitObjects[0] = &MiBalancerTimer;
    WaitObjects[1] = &MiBalancerEvent;

#ifdef TEST_PAGING
    PagingTestBuffer = ExAllocatePoolWithTag(PagedPool, PAGE_SIZE, 'tseT');
    DPRINT1("PagingTestBuffer: %p, PTE %p\n", PagingTestBuffer, MiAddressToPte(PagingTestBuffer));
    for (i = 0; i < (PAGE_SIZE / sizeof(ULONG)); i++)
        PagingTestBuffer[i] = i;
#endif

    while (1)
    {
        /* For now, we only age the paged pool, with 100 pages per run. */
        LONG Count = 100;
        PVOID Address = LastPagedPoolAddress;
        KIRQL OldIrql;
        PMMPTE PointerPte;
        PMMPDE PointerPde;
        ULONG PdeIndex;
        MMPTE TempPte;
        PMMPFN Pfn1;
        BOOLEAN Flush;

        Status = KeWaitForMultipleObjects(
            2,
            WaitObjects,
            WaitAny,
            Executive,
            KernelMode,
            FALSE,
            NULL,
            NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("KeWaitForMultipleObjects failed, status = %x\n", Status);
            KeBugCheck(MEMORY_MANAGEMENT);
        }

        DPRINT("MM Balancer: Starting to loop.\n");

        while (Count-- > 0)
        {
            Address = (PVOID)((ULONG_PTR) Address + PAGE_SIZE);

            /* Acquire PFN lock while we are cooking */
            OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

            if (Address > MmPagedPoolEnd)
            {
                Address = MmPagedPoolStart;
            }

            PointerPde = MiAddressToPde(Address);
            PointerPte = MiAddressToPte(Address);

            PdeIndex = ((ULONG_PTR)PointerPde & (SYSTEM_PD_SIZE - 1)) / sizeof(MMPTE);

            if (MmSystemPagePtes[PdeIndex].u.Hard.Valid == 0)
                TempPte.u.Long = 0;
            else
                TempPte = *PointerPte;

            if (TempPte.u.Hard.Valid == 0)
            {
                /* Bad luck, not a paged-in address */
#ifdef TEST_PAGING
                if (PointerPte == MiAddressToPte(PagingTestBuffer))
                {
                    /* Of course it should not magically become a prototype pte */
                    ASSERT(TempPte.u.Soft.Prototype == 0);
                    if (TempPte.u.Soft.Transition == 0)
                    {
                        /* It was paged out! */
                        ASSERT(TempPte.u.Soft.PageFileHigh != 0);
                        TestPage = TRUE;
                    }
                }
#endif
                KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
                continue;
            }

            Flush = FALSE;

            /* Get the Pfn */
            Pfn1 = MI_PFN_ELEMENT(PFN_FROM_PTE(&TempPte));
            ASSERT(Pfn1->PteAddress == PointerPte);
            ASSERT(Pfn1->u3.e1.PrototypePte == 0);

            /* First check if it was written to */
            if (TempPte.u.Hard.Dirty)
                Pfn1->u3.e1.Modified = 1;

            /* See if it was accessed since the last time we looked */
            if (TempPte.u.Hard.Accessed)
            {
                /* Yes! Mark the page as young and fresh again */
                Pfn1->Wsle.u1.e1.Age = 0;
                /* Tell the CPU we want to know for the next time */
                TempPte.u.Hard.Accessed = 0;
                MI_UPDATE_VALID_PTE(PointerPte, TempPte);
                Flush = TRUE;
            }
            else if (Pfn1->Wsle.u1.e1.Age == 3)
            {
                /* Page is getting old: Mark the PTE as transition */
                DPRINT1("MM Balancer: putting %p (page %x) as transition.\n", PointerPte,
                    PFN_FROM_PTE(&TempPte));
                TempPte.u.Hard.Valid = 0;
                TempPte.u.Soft.Transition = 1;
                TempPte.u.Trans.Protection = MM_READWRITE;
                MiDecrementShareCount(Pfn1, PFN_FROM_PTE(&TempPte));
                MI_WRITE_INVALID_PTE(PointerPte, TempPte);
                Flush = TRUE;
            }
            else
            {
                /* The page is just getting older */
                Pfn1->Wsle.u1.e1.Age++;
            }

            /* Flush the TLB entry if we have to */
            if (Flush)
            {
#ifdef CONFIG_SMP
                // FIXME: Should invalidate entry in every CPU TLB
                ASSERT(FALSE);
#endif
                KeInvalidateTlbEntry(Address);
            }

            /* Unlock, we're done for this address */
            KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
        }

        DPRINT("MM Balancer: End of the loop.\n");

#ifdef TEST_PAGING
        if (TestPage)
        {
            BOOLEAN Passed = TRUE;
            DPRINT1("Verifying data integrity of paged out data!\n");
            for (i = 0; i < (PAGE_SIZE / sizeof(ULONG)); i++)
            {
                if (PagingTestBuffer[i] != i)
                    Passed = FALSE;
            }
            if (Passed)
                DPRINT1("PASSED! \\o/\n");
            else
                DPRINT1("FAILED /o\\\n");
            TestPage = FALSE;
        }
#endif
        /* Remember this for the next run */
        LastPagedPoolAddress = Address;

        if (Status == STATUS_WAIT_1)
        {
            /* Balancer thread was woken up by legacy MM
             * This means we are really starving. Let's see if this still a problem */
            if ((MmAvailablePages >= MmMinimumFreePages) && (MmNumberOfPagingFiles != 0))
            {
                /* Yay! No need to trust the old balancer */
                continue;
            }

            MiRosBalancerThread();
        }
    }
}

VOID
INIT_FUNCTION
NTAPI
MiInitBalancerThread(VOID)
{
   KPRIORITY Priority;
   NTSTATUS Status;
#if !defined(__GNUC__)

   LARGE_INTEGER dummyJunkNeeded;
   dummyJunkNeeded.QuadPart = -20000000; /* 2 sec */
   ;
#endif


   KeInitializeEvent(&MiBalancerEvent, SynchronizationEvent, FALSE);
   KeInitializeTimerEx(&MiBalancerTimer, SynchronizationTimer);
   KeSetTimerEx(&MiBalancerTimer,
#if defined(__GNUC__)
                (LARGE_INTEGER)(LONGLONG)-20000000LL,     /* 2 sec */
#else
                dummyJunkNeeded,
#endif
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
