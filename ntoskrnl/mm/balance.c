/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/balance.c
 * PURPOSE:         kernel memory managment functions
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitializeBalancer)
#pragma alloc_text(INIT, MmInitializeMemoryConsumer)
#pragma alloc_text(INIT, MiInitBalancerThread)
#endif


/* TYPES ********************************************************************/
typedef struct _MM_ALLOCATION_REQUEST
{
   PFN_TYPE Page;
   LIST_ENTRY ListEntry;
   KEVENT Event;
}
MM_ALLOCATION_REQUEST, *PMM_ALLOCATION_REQUEST;

/* GLOBALS ******************************************************************/

MM_MEMORY_CONSUMER MiMemoryConsumers[MC_MAXIMUM];
static ULONG MiMinimumAvailablePages = 512;
static ULONG MiNrTotalPages;
static LIST_ENTRY AllocationListHead;
static KSPIN_LOCK AllocationListLock;
static ULONG MiMinimumPagesPerRun = 128;

static CLIENT_ID MiBalancerThreadId;
static HANDLE MiBalancerThreadHandle = NULL;
static KEVENT MiBalancerEvent;
static KEVENT MiBalancerContinue;
static KTIMER MiBalancerTimer;

/* FUNCTIONS ****************************************************************/

VOID MmPrintMemoryStatistic(VOID)
{
   DbgPrint("MC_CACHE %d, MC_USER %d, MC_PPOOL %d, MC_NPPOOL %d, MmAvailablePages %d\n",
            MiMemoryConsumers[MC_CACHE].PagesUsed, MiMemoryConsumers[MC_USER].PagesUsed,
            MiMemoryConsumers[MC_PPOOL].PagesUsed, MiMemoryConsumers[MC_NPPOOL].PagesUsed,
            MmAvailablePages);
}

VOID
INIT_FUNCTION
NTAPI
MmInitializeBalancer(ULONG NrAvailablePages, ULONG NrSystemPages)
{
   memset(MiMemoryConsumers, 0, sizeof(MiMemoryConsumers));
   InitializeListHead(&AllocationListHead);
   KeInitializeSpinLock(&AllocationListLock);

   MiNrTotalPages = NrAvailablePages;

   /* Set up targets. */
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
   MiMemoryConsumers[MC_USER].PagesTarget =
      NrAvailablePages - MiMinimumAvailablePages;
   MiMemoryConsumers[MC_PPOOL].PagesTarget = NrAvailablePages / 2;
   MiMemoryConsumers[MC_NPPOOL].PagesTarget = 0xFFFFFFFF;
   MiMemoryConsumers[MC_NPPOOL].PagesUsed = NrSystemPages;
   MiMemoryConsumers[MC_SYSTEM].PagesTarget = 0xFFFFFFFF;
   MiMemoryConsumers[MC_SYSTEM].PagesUsed = 0;
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
MmReleasePageMemoryConsumer(ULONG Consumer, PFN_TYPE Page)
{
   PMM_ALLOCATION_REQUEST Request;
   PLIST_ENTRY Entry;
   KIRQL OldIrql;

   if (Page == 0)
   {
      DPRINT1("Tried to release page zero.\n");
      KeBugCheck(MEMORY_MANAGEMENT);
   }

   KeAcquireSpinLock(&AllocationListLock, &OldIrql);
   if (MmGetReferenceCountPage(Page) == 1)
   {
      (void)InterlockedDecrementUL(&MiMemoryConsumers[Consumer].PagesUsed);
      if (IsListEmpty(&AllocationListHead))
      {
         KeReleaseSpinLock(&AllocationListLock, OldIrql);
         OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
         MmDereferencePage(Page);
         KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
      }
      else
      {
         Entry = RemoveHeadList(&AllocationListHead);
         Request = CONTAINING_RECORD(Entry, MM_ALLOCATION_REQUEST, ListEntry);
         KeReleaseSpinLock(&AllocationListLock, OldIrql);
         if(Consumer == MC_USER || Consumer == MC_PPOOL) 
			 MmRemoveLRUUserPage(Page);
         MiZeroPage(Page);
         Request->Page = Page;
         KeSetEvent(&Request->Event, IO_NO_INCREMENT, FALSE);
      }
   }
   else
   {
      KeReleaseSpinLock(&AllocationListLock, OldIrql);
      MmDereferencePage(Page);
   }

   return(STATUS_SUCCESS);
}

NTSTATUS
MmTrimUserMemory(ULONG Target, ULONG Priority, PULONG NrFreedPages)
{
    PFN_TYPE CurrentPage;
    PFN_TYPE NextPage;
    NTSTATUS Status;
    
    (*NrFreedPages) = 0;
	DPRINT("Trimming user memory (want %d)\n", Target);
    
    CurrentPage = MmGetLRUFirstUserPage();
    while (CurrentPage != 0 && Target > 0)
    {
        NextPage = MmGetLRUNextUserPage(CurrentPage);

		DPRINT("Page Out %x\n", CurrentPage);
        Status = MmPageOutPhysicalAddress(CurrentPage);
		DPRINT("Done %x\n", Status);
        if (NT_SUCCESS(Status))
        {
            Target--;
            (*NrFreedPages)++;
        }
        else if (Status == STATUS_PAGEFILE_QUOTA)
        {
			MmRemoveLRUUserPage(CurrentPage);
			MmInsertLRULastUserPage(CurrentPage);
			return STATUS_SUCCESS;
        }
        
        CurrentPage = NextPage;
    }
	if (CurrentPage)
		MmDereferencePage(CurrentPage);
	
	DPRINT("Done: %d\n", NrFreedPages);

    return(STATUS_SUCCESS);
}

VOID
NTAPI
MmRebalanceMemoryConsumers(VOID)
{
   LONG Target;
   ULONG i;
   ULONG NrFreedPages;
   NTSTATUS Status;

   Target = (MiMinimumAvailablePages - MmAvailablePages);
   Target = max(Target, (LONG) MiMinimumPagesPerRun);

   for (i = 0; i < MC_MAXIMUM && Target > 0; i++)
   {
      if (MiMemoryConsumers[i].Trim != NULL)
      {
		 DPRINT("Trimming %d\n");
		 Status = MiMemoryConsumers[i].Trim(Target, 0, &NrFreedPages);
		 DPRINT("Got %d pages\n", NrFreedPages);
         if (!NT_SUCCESS(Status))
         {
            KeBugCheck(MEMORY_MANAGEMENT);
         }
         Target = Target - NrFreedPages;
      }
   }
}

static BOOLEAN
MiIsBalancerThread(VOID)
{
   return MiBalancerThreadHandle != NULL &&
          PsGetCurrentThreadId() == MiBalancerThreadId.UniqueThread;
}

NTSTATUS
NTAPI
MmRequestPageMemoryConsumer(ULONG Consumer, BOOLEAN CanWait,
                            PPFN_TYPE AllocatedPage)
{
   PFN_TYPE Page = 0;
   KIRQL OldIrql;

   (void)InterlockedIncrementUL(&MiMemoryConsumers[Consumer].PagesUsed);

   if (!MiIsBalancerThread() && MmAvailablePages <= MiMinimumAvailablePages && CanWait)
   {
	   DPRINT("MmAvailablePages %d MiMinimumAvailablePages %d\n", 
			   MmAvailablePages, MiMinimumAvailablePages);
	   KeSetEvent(&MiBalancerEvent, IO_NO_INCREMENT, FALSE);
	   KeWaitForSingleObject(&MiBalancerContinue, 0, KernelMode, FALSE, NULL);
   }

   /*
    * Allocate always memory for the non paged pool and for the pager thread.
    */
   if ((Consumer == MC_NPPOOL) || 
	   (Consumer == MC_SYSTEM) || 
	   MiIsBalancerThread() ||
	   (MmAvailablePages > MiMinimumAvailablePages))
   {
      OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
      Page = MmAllocPage(Consumer, 0);
      KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
      if (!Page && MiIsBalancerThread())
      {
		  DPRINT1("Can't allocate for balancer: %d vs %d\n",
				  MmAvailablePages, MiMinimumAvailablePages);
         KeBugCheck(NO_PAGES_AVAILABLE);
      }
   }

   /*
    * Make sure we don't exceed global targets.
    */
   while (!Page)
   {
	   if (!MiIsBalancerThread() || CanWait)
	   {
		   KeSetEvent(&MiBalancerEvent, IO_NO_INCREMENT, FALSE);
		   KeWaitForSingleObject(&MiBalancerContinue, 0, KernelMode, FALSE, NULL);
	   }

	   /*
		* Actually allocate the page.
		*/
	   OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
	   Page = MmAllocPage(Consumer, 0);
	   KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
	   
	   if (!Page && !CanWait)
	   {
		   (void)InterlockedDecrementUL(&MiMemoryConsumers[Consumer].PagesUsed);
		   return(STATUS_NO_MEMORY);
	   }
   }

   if(Consumer == MC_USER || Consumer == MC_PPOOL)
	   MmInsertLRULastUserPage(Page);
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

   ASSERT(MiIsBalancerThread());

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

	  MmRebalanceMemoryConsumers();
	  KeSetEvent(&MiBalancerContinue, IO_NO_INCREMENT, FALSE);
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
   KeInitializeEvent(&MiBalancerContinue, SynchronizationEvent, FALSE);
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
                                 (PKSTART_ROUTINE) MiBalancerThread,
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
