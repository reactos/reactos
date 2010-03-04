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
//#define NDEBUG
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

BOOLEAN MiBalancerInitialized = FALSE;
MM_MEMORY_CONSUMER MiMemoryConsumers[MC_MAXIMUM];
ULONG MiPagesRequired;
/*static*/ ULONG MiMinimumAvailablePages = 256;
static ULONG MiNrTotalPages;
static LIST_ENTRY AllocationListHead;
static KSPIN_LOCK AllocationListLock;
static ULONG MiMinimumPagesPerRun = 64;

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
MmTrimUserMemory(ULONG Target, ULONG Priority, PULONG NrFreedPages)
{
	static PFN_TYPE Page = 0;
    NTSTATUS Status;
    
    (*NrFreedPages) = 0;
	DPRINT("Trimming user memory (want %d)\n", Target);

	// Current page is referenced
	for (Page = MmGetLRUNextUserPage(Page); *NrFreedPages < Target && Page; Page = MmGetLRUNextUserPage(Page))
    {
        Status = MmPageOutPhysicalAddress(Page);
        if (NT_SUCCESS(Status))
        {
            Target--;
            (*NrFreedPages)++;
			DPRINT("Successful Pageout: %x\n", Page);
        } else {
			DPRINT("Unsuccessful Pageout: %x\n", Page);
		}
    }

	DPRINT("Done\n");
    return(STATUS_SUCCESS);
}

static BOOLEAN
MiIsBalancerThread(VOID)
{
   return 
	   !MiBalancerInitialized ||
	   (PsGetCurrentThread()->Cid.UniqueThread == 
		MiBalancerThreadId.UniqueThread);
}

/*
 * A note about this:
 *
 * We wait for either balancer thread return or the timer tick on the basis that, if the
 * balancer has deadlocked against us due to code outside mm's control, we can proceed on
 * the next timer tick knowing that we're not consuming pages faster than the balancer
 * can act.
 */
BOOLEAN
NTAPI
MiWaitForBalancer()
{
	NTSTATUS Status;
	ULONG TimerTicks = 0;
	PVOID WaitObjects[2];
	
	WaitObjects[0] = &MiBalancerContinue;
	WaitObjects[1] = &MiBalancerTimer;
	
	DPRINT1("WaitForBalancer\n");
	do {
		Status = KeWaitForMultipleObjects
			(2, WaitObjects, WaitAny, Executive, KernelMode, FALSE, NULL, NULL);
		if (Status == STATUS_SUCCESS + 1)
		{
			TimerTicks++;
			DPRINT1("Deadlock broken by balancer tick :-(\n");
		}
		else
		{
			return FALSE;
		}
	} while (TimerTicks < 3 && Status == STATUS_SUCCESS + 1);
	ASSERT(TimerTicks < 3);
	DPRINT1("Wait complete\n");
	return TRUE;
}

VOID NTAPI MiSetConsumer(IN PFN_TYPE Pfn, IN ULONG Consumer);

NTSTATUS
NTAPI
MmRequestPageMemoryConsumer(ULONG Consumer, BOOLEAN CanWait,
                            PPFN_TYPE AllocatedPage)
{
   PFN_TYPE Page = 0;
   KIRQL OldIrql;

   (void)InterlockedIncrementUL(&MiMemoryConsumers[Consumer].PagesUsed);

   /*
    * Allocate always memory for the non paged pool and for the pager thread.
    */
   if (Consumer == MC_NPPOOL ||
	   Consumer == MC_SYSTEM ||
	   MiIsBalancerThread() ||
	   (MmAvailablePages > MiMinimumAvailablePages))
   {
      OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
	  DPRINT("MmAllocPage(%d) (%d left)\n", Consumer, MmAvailablePages);
      Page = MmAllocPage(Consumer, 0);
	  DPRINT("Page %x\n", Page);
      KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
      if (!Page && MiIsBalancerThread())
      {
         KeBugCheck(NO_PAGES_AVAILABLE);
      }
   }

   /*
    * Make sure we don't exceed global targets.
    */
   while (!Page)
   {
	  if (!CanWait || MiIsBalancerThread())
      {
         (void)InterlockedDecrementUL(&MiMemoryConsumers[Consumer].PagesUsed);
         return(STATUS_NO_MEMORY);
      }
	  else
	  {
		  DPRINT1("Set balancer and await\n");
		  KeSetEvent(&MiBalancerEvent, IO_NO_INCREMENT, FALSE);
		  MiWaitForBalancer();
		  DPRINT1("Balancer returned\n");
	  }

	  Page = MmAllocPage(Consumer, 0);
   }

   if(BALANCER_CAN_EVICT(Consumer))
	   MmInsertLRULastUserPage(Page);

   DPRINT("Successful alloc %x after balancer (consumer %d)\n", Page, Consumer);
   *AllocatedPage = Page;
   
   return(STATUS_SUCCESS);
}

VOID NTAPI
MiBalancerThread(PVOID Unused)
{
   PVOID WaitObjects[2];
   NTSTATUS Status;
   ULONG NrFreedPages;

   WaitObjects[0] = &MiBalancerEvent;
   WaitObjects[1] = &MiBalancerTimer;

   MiBalancerInitialized = TRUE;

   while (1)
   {
	  DPRINT
		  ("BALANCER THREAD WAITING %d (Thread %x %x)\n", 
		   MmAvailablePages, 
		   PsGetCurrentThread()->Cid.UniqueThread, 
		   MiBalancerThreadId.UniqueThread);
      Status = KeWaitForMultipleObjects
		  (2, WaitObjects, WaitAny, Executive, KernelMode, FALSE, NULL, NULL);

      if (MmAvailablePages < MiMinimumAvailablePages + MiMinimumPagesPerRun)
      {
		 DPRINT("MmAvailablePages %x MiMinimumAvailablePages %x\n", MmAvailablePages, MiMinimumAvailablePages);

		 NrFreedPages = 0;
		 ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
		 MmTrimUserMemory(MiMinimumPagesPerRun, 0, &NrFreedPages);
		 ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

		 DPRINT("Trim: Status %x, Trimmed %d\n", Status, NrFreedPages);
      }
	  KeSetEvent(&MiBalancerContinue, IO_NO_INCREMENT, FALSE);
	  DPRINT("BALANCER THREAD CYCLING\n");
   }
}

VOID
INIT_FUNCTION
NTAPI
MiInitBalancerThread(VOID)
{
   KPRIORITY Priority;
   NTSTATUS Status;
   LARGE_INTEGER BalancerInterval;
   BalancerInterval.QuadPart = -20000000; /* 2 sec */

   KeInitializeEvent(&MiBalancerEvent, SynchronizationEvent, FALSE);
   KeInitializeEvent(&MiBalancerContinue, SynchronizationEvent, FALSE);
   KeInitializeTimerEx(&MiBalancerTimer, SynchronizationTimer);
   KeSetTimerEx(&MiBalancerTimer,
                BalancerInterval,
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
