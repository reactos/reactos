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
/*static*/ ULONG MiMinimumAvailablePages = 128;
static ULONG MiNrTotalPages;
static LIST_ENTRY AllocationListHead;
static KSPIN_LOCK AllocationListLock;
static ULONG MiMinimumPagesPerRun = 192;

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
	if (Page == 0)
	{
		DPRINT1("Tried to release page zero.\n");
		KeBugCheck(MEMORY_MANAGEMENT);
	}
	
	MmDereferencePage(Page);
	
	return(STATUS_SUCCESS);
}

VOID
NTAPI
MiTrimMemoryConsumer(ULONG Consumer)
{
   LONG Target;
   ULONG NrFreedPages;

   Target = MiMemoryConsumers[Consumer].PagesUsed -
            MiMemoryConsumers[Consumer].PagesTarget;
   if (Target < MiMinimumPagesPerRun)
   {
      Target = MiMinimumPagesPerRun;
   }

   if (MiMemoryConsumers[Consumer].Trim != NULL)
   {
      MiMemoryConsumers[Consumer].Trim(Target, 0, &NrFreedPages);
   }
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
		//DPRINT("Trying page %x\n", CurrentPage);
        NextPage = MmGetLRUNextUserPage(CurrentPage);
        Status = MmPageOutPhysicalAddress(CurrentPage);
		//DPRINT("Status: %x\n", Status);
        if (NT_SUCCESS(Status))
        {
            Target--;
            (*NrFreedPages)++;
        }
		else
		{
			MmRemoveLRUUserPage(CurrentPage);
			MmInsertLRULastUserPage(CurrentPage);
		}
        
        CurrentPage = NextPage;
    }
	DPRINT("Done\n");
    return(STATUS_SUCCESS);
}

static BOOLEAN
MiIsBalancerThread(VOID)
{
   return 
	   PsGetCurrentThread()->Cid.UniqueThread == 
	   MiBalancerThreadId.UniqueThread;
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
	PVOID WaitObjects[2];
	
	WaitObjects[0] = &MiBalancerContinue;
	WaitObjects[1] = &MiBalancerTimer;
	
	Status = KeWaitForMultipleObjects
		(1, WaitObjects, WaitAny, Executive, KernelMode, FALSE, NULL, NULL);
	if (Status == STATUS_SUCCESS + 1)
	{
		DPRINT1("Deadlock broken by balancer tick :-(\n");
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

NTSTATUS
NTAPI
MmRequestPageMemoryConsumer(ULONG Consumer, BOOLEAN CanWait,
                            PPFN_TYPE AllocatedPage)
{
   BOOLEAN Broken;
   PFN_TYPE Page = 0;
   KIRQL OldIrql;

   (void)InterlockedIncrementUL(&MiMemoryConsumers[Consumer].PagesUsed);

   /*
    * Allocate always memory for the non paged pool and for the pager thread.
    */
   if (Consumer == MC_NPPOOL ||
	   Consumer == MC_SYSTEM ||
	   Consumer == MC_CACHE ||
	   MiIsBalancerThread() ||
	   (MmAvailablePages > MiMinimumAvailablePages))
   {
	  if (MmAvailablePages <= MiMinimumAvailablePages)
	  {
		  KeSetEvent(&MiBalancerEvent, IO_NO_INCREMENT, FALSE);
	  }
      OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
      Page = MmAllocPage(Consumer, 0);
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
	   DPRINT1("Waiting for balancer... (want %d, have %d)\n", MiMinimumAvailablePages, MmAvailablePages);
	   KeSetEvent(&MiBalancerEvent, IO_NO_INCREMENT, FALSE);
	   Broken = MiWaitForBalancer();

	   /*
		* Actually allocate the page.
		*/
	   OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
	   Page = MmAllocPage(Consumer, 0);
	   KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
	   
	   if (Broken)
		   DPRINT1("Page %x\n", Page);

	   if (!Page && !CanWait)
	   {
		   (void)InterlockedDecrementUL(&MiMemoryConsumers[Consumer].PagesUsed);
		   DPRINT1("Could not allocate and could not wait consumer %d\n", Consumer);
		   return(STATUS_NO_MEMORY);
	   }
   }

   if(BALANCER_CAN_EVICT(Consumer))
	   MmInsertLRULastUserPage(Page);

   DPRINT1("Successful alloc %x after balancer (consumer %d)\n", Page, Consumer);
   *AllocatedPage = Page;
   
   return(STATUS_SUCCESS);
}

VOID NTAPI
MiBalancerThread(PVOID Unused)
{
   PVOID WaitObjects[2];
   NTSTATUS Status;
   ULONG i;
   ULONG NrFreedPages;

   WaitObjects[0] = &MiBalancerEvent;
   WaitObjects[1] = &MiBalancerTimer;

   while (1)
   {
	  DPRINT1
		  ("BALANCER THREAD WAITING %d (Thread %x %x)\n", 
		   MmAvailablePages, 
		   PsGetCurrentThread()->Cid.UniqueThread, 
		   MiBalancerThreadId.UniqueThread);
      Status = KeWaitForMultipleObjects
		  (2, WaitObjects, WaitAny, Executive, KernelMode, FALSE, NULL, NULL);

      if (NT_SUCCESS(Status))
      {
		 DPRINT1("MmAvailablePages %x MiMinimumAvailablePages %x\n", MmAvailablePages, MiMinimumAvailablePages);
         while (MmAvailablePages < MiMinimumAvailablePages)
         {
            for (i = 0; i < MC_MAXIMUM; i++)
            {
               if (MiMemoryConsumers[i].Trim != NULL)
               {
                  NrFreedPages = 0;
				  ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
                  Status = MiMemoryConsumers[i].Trim(MiMinimumPagesPerRun, 0, &NrFreedPages);
				  ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
				  DPRINT("Trim %d: Status %x, Trimmed %d\n", Status, NrFreedPages);
                  if (!NT_SUCCESS(Status))
                  {
                     KeBugCheck(MEMORY_MANAGEMENT);
                  }
               }
            }
         }
		 DPRINT1("Done freeing pages\n");
         KeSetEvent(&MiBalancerContinue, IO_NO_INCREMENT, FALSE);
      }
      else
      {
         DPRINT1("KeWaitForMultipleObjects failed, status = %x\n", Status);
         KeBugCheck(MEMORY_MANAGEMENT);
      }
	  DPRINT1("BALANCER THREAD CYCLING\n");
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
