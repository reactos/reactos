/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: balance.c,v 1.7.2.1 2002/05/13 20:37:00 chorns Exp $
 *
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel 
 * FILE:        ntoskrnl/mm/balance.c
 * PURPOSE:     kernel memory managment functions
 * PROGRAMMER:  David Welch (welch@cwcom.net)
 *              Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *              Created 27/12/01
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES ********************************************************************/

#define BALANCE_SET_MANAGER_TIMER_VALUE   10000
//#define BALANCE_SET_MANAGER_TIMER_VALUE   1000

#define DISABLE_TRIM 0xffffffff

typedef struct _MM_MEMORY_CONSUMER
{
  /* Number of physical pages currently used by this consumer */
  ULONG PagesUsed;
  /* Number of physical pages the balancer tries to limit the consumer to use */
  ULONG PagesTarget;
  /* Consumer's share in percent of total usable pages.
     0xffffffff to never trim the consumer */
  ULONG ShareTarget;
  /* Trim method for the consumer */
  NTSTATUS (*Trim)(ULONG Target, ULONG Priority, PULONG NrFreed);
} MM_MEMORY_CONSUMER, *PMM_MEMORY_CONSUMER;

typedef struct _MM_ALLOCATION_REQUEST
{
  ULONG Consumer;
  ULONG_PTR Page;
  LIST_ENTRY ListEntry;
  KEVENT Event;
} MM_ALLOCATION_REQUEST, *PMM_ALLOCATION_REQUEST;

/* GLOBALS ******************************************************************/

static MM_MEMORY_CONSUMER MiMemoryConsumers[MC_MAXIMUM];
static ULONG MiMinimumAvailablePages;
static ULONG MiNrAvailablePages;
static ULONG MiNrTotalPages;
static LIST_ENTRY AllocationListHead;
static KSPIN_LOCK AllocationListLock;
static ULONG NrWorkingThreads = 0;
static HANDLE WorkerThreadId;
static ULONG MiPagesRequired = 0;
static HANDLE MiBalanceSetManagerThreadHandle;
static CLIENT_ID MiBalanceSetManagerThreadId;
static KEVENT MiBalanceSetManagerEvent;
static KTIMER MiBalanceSetManagerTimer;
static ULONG MiBalanceSetManagerTimerExpireCount;
static volatile BOOLEAN MiBalanceSetManagerShouldTerminate;

/* FUNCTIONS ****************************************************************/

VOID
MiTrimMemoryConsumer(ULONG Consumer)
{
  LONG Target;

  Target = MiMemoryConsumers[Consumer].PagesUsed - 
    MiMemoryConsumers[Consumer].PagesTarget;
  if (Target < 0)
    {
      Target = 1;
    }

  if (MiMemoryConsumers[Consumer].Trim != NULL)
    {
      MiMemoryConsumers[Consumer].Trim(Target, 0, NULL);
    }
}

VOID
MiComputeTargets()
{
  ULONG Consumer;

  for (Consumer = 0; Consumer < MC_MAXIMUM; Consumer++)
    {
      ULONG ShareTarget;

      ShareTarget = MiMemoryConsumers[Consumer].ShareTarget;
      if (ShareTarget == DISABLE_TRIM)
        {
          MiMemoryConsumers[Consumer].PagesTarget = 0xffffffff;
        }
      else
        {
          MiMemoryConsumers[Consumer].PagesTarget = (MiNrAvailablePages * 100) / ShareTarget;
        }
    }
}


VOID
MiRebalanceMemoryConsumers()
{
  LONG Target;
  ULONG i;
  ULONG NrFreedPages;
  NTSTATUS Status;

  /* Compute targets for consumers */
  MiComputeTargets();

  for (i = 0; i < MC_MAXIMUM; i++)
    {
      if ((MiMemoryConsumers[i].Trim != NULL)
        && (MiMemoryConsumers[i].ShareTarget != DISABLE_TRIM))
      	{
          DPRINT("PagesTarget 0x%.08x  UsedPages 0x%.08x\n",
            MiMemoryConsumers[i].PagesTarget,
            MiMemoryConsumers[i].PagesUsed);
          Target = MiMemoryConsumers[i].PagesTarget - MiMemoryConsumers[i].PagesUsed;
          if (Target > 0)
            {
						  Status = MiMemoryConsumers[i].Trim(Target, 0, &NrFreedPages);
						  if (!NT_SUCCESS(Status))
						    {
					        DPRINT1("Status 0x%.08x\n", Status);
						      KeBugCheck(0);
				  	    }
            }
	      }
    }
}


NTSTATUS STDCALL
MiBalanceSetManager(IN PVOID  Context)
{
  PVOID WaitObjects[2];
  NTSTATUS Status;

  WaitObjects[0] = &MiBalanceSetManagerTimer;
  WaitObjects[1] = &MiBalanceSetManagerEvent;

  for (;;)
    {
			Status = KeWaitForMultipleObjects(2,
				&WaitObjects[0],
				WaitAny,
				0,
				KernelMode,
				FALSE,
				NULL,
				NULL);
		
			if (!NT_SUCCESS(Status))
				{
				  DPRINT1("(MiBalanceSetManager) Wait failed\n");
				  KeBugCheck(0);
				  return(STATUS_UNSUCCESSFUL);
				}

			if (MiBalanceSetManagerShouldTerminate)
				{
				  DPRINT("(MiBalanceSetManager) Terminating\n");
				  return(STATUS_SUCCESS);
				}

      if (Status == STATUS_WAIT_0)
        {
          DPRINT("(MiBalanceSetManager) Woke up due to 1 second timer\n");

		      MiBalanceSetManagerTimerExpireCount--;

		      if (MiBalanceSetManagerTimerExpireCount <= 0)
						{
		          MiBalanceSetManagerTimerExpireCount = 4;
		          /* FIXME: Signal swapper */
						}

		      /* FIXME: Adjust lookaside list depths */

		      /* FIXME: Priority boost to prevent starvation */
        }
			else
				{
          DPRINT("(MiBalanceSetManager) Woke up due to event signalled\n");
				}

      /* Rebalance the working set of memory consumers */
      MiRebalanceMemoryConsumers();
    }

  return STATUS_SUCCESS;
}


VOID
MmInitializeBalancer(ULONG NrAvailablePages)
{
  memset(MiMemoryConsumers, 0, sizeof(MiMemoryConsumers));
  InitializeListHead(&AllocationListHead);
  KeInitializeSpinLock(&AllocationListLock);

  MiNrAvailablePages = MiNrTotalPages = NrAvailablePages;

  /* Try to keep 5 percent of available pages free */
  MiMinimumAvailablePages = (MiNrAvailablePages / 20);
  //MiMinimumAvailablePages = ((MiNrAvailablePages / 10) * 8); // Stress it!

  MiMemoryConsumers[MC_CACHE].PagesUsed = 0;
  MiMemoryConsumers[MC_USER].PagesUsed = 0;
  MiMemoryConsumers[MC_PPOOL].PagesUsed = 0;
  MiMemoryConsumers[MC_NPPOOL].PagesUsed = 0;

  /* Set up targets */
  MiMemoryConsumers[MC_CACHE].ShareTarget  = DISABLE_TRIM;
  MiMemoryConsumers[MC_USER].ShareTarget   = 20;
  MiMemoryConsumers[MC_PPOOL].ShareTarget  = DISABLE_TRIM;
  /* We cannot page out pages from the non-paged pool */
  MiMemoryConsumers[MC_NPPOOL].ShareTarget = DISABLE_TRIM;
  MiComputeTargets();
}


VOID
MmInitializeBalanceSetManager()
{
  LARGE_INTEGER DueTime;
  NTSTATUS Status;

  MiBalanceSetManagerTimerExpireCount = 4;
  MiBalanceSetManagerShouldTerminate = FALSE;
	KeInitializeEvent(&MiBalanceSetManagerEvent,
		SynchronizationEvent,
		FALSE);

	Status = PsCreateSystemThread(&MiBalanceSetManagerThreadHandle,
		THREAD_ALL_ACCESS,
		NULL,
		NULL,
		&MiBalanceSetManagerThreadId,
		MiBalanceSetManager,
		NULL);

	if (!NT_SUCCESS(Status))
		{
      DPRINT1("Cannot initialize balance set manager thread (Status 0x%.08x)\n", Status);
  		KeBugCheck(0);
		}

	/* Set thread priority to LOW_REALTIME_PRIORITY + 1 */
  Status = PiSetPriorityThread(MiBalanceSetManagerThreadHandle, LOW_REALTIME_PRIORITY + 1);

	if (!NT_SUCCESS(Status))
		{
      DPRINT1("Cannot set priority of balance set manager thread (Status 0x%.08x)\n", Status);
  		KeBugCheck(0);
		}

	/* Initialize a periodic timer that the balance set manager thread waits upon */
  KeInitializeTimerEx(&MiBalanceSetManagerTimer, SynchronizationTimer);

  /* Start the periodic timer with an initial and periodic relative
     expiration time of BALANCE_SET_MANAGER_TIMER_VALUE milliseconds */
  DueTime.QuadPart = -(LONGLONG)BALANCE_SET_MANAGER_TIMER_VALUE * 10000;
  KeSetTimerEx(&MiBalanceSetManagerTimer, DueTime, BALANCE_SET_MANAGER_TIMER_VALUE, NULL);
}


VOID
MmInitializeMemoryConsumer(ULONG Consumer, 
			   NTSTATUS (*Trim)(ULONG Target, ULONG Priority, 
					    PULONG NrFreed))
{
  MiMemoryConsumers[Consumer].Trim = Trim;
}


NTSTATUS
MmReleasePageMemoryConsumer(IN ULONG  Consumer,
  IN ULONG_PTR  Page)
{
  assertmsg(Page != 0, ("Tried to release page zero.\n"));

  InterlockedIncrement(&MiNrAvailablePages);
  InterlockedDecrement(&MiPagesRequired);
  MmDereferencePage(Page);
  return(STATUS_SUCCESS);
}


NTSTATUS
MiFreePageMemoryConsumer(IN ULONG  Consumer,
  IN ULONG_PTR  Page)
{
  assertmsg(Page != 0, ("Tried to free page zero.\n"));

  assertmsg(MiMemoryConsumers[Consumer].PagesUsed > 0, ("Bad PagesUsed %d for consumer %d\n",
    MiMemoryConsumers[Consumer].PagesUsed, Consumer));

  InterlockedDecrement(&MiMemoryConsumers[Consumer].PagesUsed);
  MmDereferencePage(Page);
  return(STATUS_SUCCESS);
}


VOID
MiSatisfyAllocationRequest()
{
  KIRQL oldIrql;

  KeAcquireSpinLock(&AllocationListLock, &oldIrql);
  if (IsListEmpty(&AllocationListHead))
    {
      KeReleaseSpinLock(&AllocationListLock, oldIrql);
    }
  else
    {
      PMM_ALLOCATION_REQUEST Request;
      ULONG_PTR PhysicalPage;
      PLIST_ENTRY Entry;
      NTSTATUS Status;

      /* There are outstanding page allocation requests, so pull a request off
         the list and give it a page if one is available */
      Entry = RemoveHeadList(&AllocationListHead);
      Request = CONTAINING_RECORD(Entry, MM_ALLOCATION_REQUEST, ListEntry);
      Status = MmRequestPageMemoryConsumer(Request->Consumer, FALSE, &PhysicalPage);
      if (NT_SUCCESS(Status))
        {
		      KeReleaseSpinLock(&AllocationListLock, oldIrql);
		      Request->Page = PhysicalPage;
		      KeSetEvent(&Request->Event, IO_NO_INCREMENT, FALSE);
        }
			else
			  {
          InsertHeadList(&AllocationListHead, Entry);
		      KeReleaseSpinLock(&AllocationListLock, oldIrql);
        }
    }
}


VOID
MiLowMemoryCheck()
{
	if (MiNrAvailablePages < MiMinimumAvailablePages)
		{
			/* The system is running low on available pages so tell the modified page
			   writer to write out some modified pages so they can be freed */
			MiSignalModifiedPageWriter();
		}
}


NTSTATUS
MmRequestPageMemoryConsumer(IN ULONG  Consumer,
  IN BOOLEAN  CanWait,
  OUT PULONG_PTR  pPage)
{
  ULONG OldUsed;
  ULONG OldAvailable;
  ULONG_PTR Page;
  KIRQL oldIrql;

  /*
   * Make sure we don't exceed our individual target.
   */
  OldUsed = InterlockedIncrement(&MiMemoryConsumers[Consumer].PagesUsed);
  if (OldUsed >= (MiMemoryConsumers[Consumer].PagesTarget - 1) &&
      WorkerThreadId != PsGetCurrentThreadId())
    {
      if (!CanWait)
	{
	  InterlockedDecrement(&MiMemoryConsumers[Consumer].PagesUsed);
	  return(STATUS_NO_MEMORY);
	}
      MiTrimMemoryConsumer(Consumer);
    }

  /*
   * Make sure we don't exceed global targets.
   */
  OldAvailable = InterlockedDecrement(&MiNrAvailablePages);
  if (OldAvailable < MiMinimumAvailablePages)
    {
      MM_ALLOCATION_REQUEST Request;

      if (!CanWait)
	{
	  InterlockedIncrement(&MiNrAvailablePages);

    assertmsg(MiMemoryConsumers[Consumer].PagesUsed > 0, ("Bad PagesUsed 0x%.08x for consumer %d\n",
      MiMemoryConsumers[Consumer].PagesUsed, Consumer));

	  InterlockedDecrement(&MiMemoryConsumers[Consumer].PagesUsed);
	  return(STATUS_NO_MEMORY);
	}

      /* Insert an allocation request. */
      Request.Consumer = Consumer;
      Request.Page = 0;
      KeInitializeEvent(&Request.Event, NotificationEvent, FALSE);
      InterlockedIncrement(&MiPagesRequired);

      KeAcquireSpinLock(&AllocationListLock, &oldIrql);     
      if (NrWorkingThreads == 0)
	{
	  InsertTailList(&AllocationListHead, &Request.ListEntry);
	  NrWorkingThreads++;
	  KeReleaseSpinLock(&AllocationListLock, oldIrql);
	  WorkerThreadId = PsGetCurrentThreadId();
	  MiRebalanceMemoryConsumers();
	  KeAcquireSpinLock(&AllocationListLock, &oldIrql);
	  NrWorkingThreads--;
	  WorkerThreadId = 0;
	  KeReleaseSpinLock(&AllocationListLock, oldIrql);
	}
      else
	{
	  if (WorkerThreadId == PsGetCurrentThreadId())
	    {
	      Page = MmAllocPage(Consumer, 0);
	      KeReleaseSpinLock(&AllocationListLock, oldIrql);
	      if (Page == 0)
		{
          assert(FALSE);
		  KeBugCheck(0);
		}
	      *pPage = Page;
        MiLowMemoryCheck();
	      return(STATUS_SUCCESS);
	    }
	  InsertTailList(&AllocationListHead, &Request.ListEntry);
	  KeReleaseSpinLock(&AllocationListLock, oldIrql);
	}
      KeWaitForSingleObject(&Request.Event,
			    0,
			    KernelMode,
			    FALSE,
			    NULL);

      Page = Request.Page;
      if (Page == 0)
	{
      assert(FALSE);
	  KeBugCheck(0);
	}
      MmTransferOwnershipPage(Page, Consumer);
      *pPage = Page;
      return(STATUS_SUCCESS);
    }

  /*
   * Actually allocate the page.
   */
  Page = MmAllocPage(Consumer, 0);
  if (Page == 0)
    {
      assert(FALSE);
      KeBugCheck(0);
    }
  *pPage = Page;

  MiLowMemoryCheck();

  return(STATUS_SUCCESS);
}
