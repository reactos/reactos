/* $Id: mpw.c,v 1.7.2.1 2002/05/13 20:37:00 chorns Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/mpw.c
 * PURPOSE:      Writes data that has been modified in memory but not on
 *               the disk
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 *               Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY: 
 *               27/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>
#include <internal/mm.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static HANDLE MiModifiedPageWriterThreadHandle;
static CLIENT_ID MiModifiedPageWriterThreadId;
static KEVENT MiModifiedPageWriterEvent;
static volatile BOOLEAN MiModifiedPageWriterShouldTerminate;

static HANDLE MiMappedPageWriterThreadHandle;
static CLIENT_ID MiMappedPageWriterThreadId;
static KEVENT MiMappedPageWriterEvent;
static volatile BOOLEAN MiMappedPageWriterShouldTerminate;

/* FUNCTIONS *****************************************************************/

VOID
MiGatherModifiedPages(IN OUT PLIST_ENTRY  TransferListHead)
{
  PLIST_ENTRY ModifiedPageListHead;
  PLIST_ENTRY CurrentEntry;
  PPHYSICAL_PAGE Current;
  PLIST_ENTRY NextEntry;
  NTSTATUS Status;
  KIRQL OldIrql;
  ULONG Count;

  DPRINT("MiGatherModifiedPages()\n");

  InitializeListHead(TransferListHead);

  KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
  MiAcquirePageListLock(PAGE_LIST_MODIFIED, &ModifiedPageListHead);

  Count = 0;
  CurrentEntry = ModifiedPageListHead->Flink;
  while (CurrentEntry != ModifiedPageListHead)
    {
      NextEntry = CurrentEntry->Flink;
      Current = CONTAINING_RECORD(CurrentEntry, PHYSICAL_PAGE, ListEntry);

      if (Current->Flags != MM_PHYSICAL_PAGE_MODIFIED)
				{
          DPRINT("Non-modified page on modified page list. Flags is 0x%.08x\n",
            Current->Flags);
          KeBugCheck(0);
				}

      /* FIXME: Gather only about 1/4 of modified pages */
      /* FIXME: Prefer continuous pages */
      /* FIXME: Start from where we left off */

      Status = MmPrepareFlushPhysicalAddress(MiPageFromDescriptor(Current));
      if (Status == STATUS_SUCCESS)
        {
		      /* Put on modified page writer list */
		      Current->Flags = MM_PHYSICAL_PAGE_MPW;
		      RemoveEntryList(&Current->ListEntry);
		      InterlockedDecrement(&MiModifiedPageListSize);
		      InsertTailList(TransferListHead, CurrentEntry);
		      Count++;
        }

      CurrentEntry = NextEntry;
    }

  MiReleasePageListLock();
  KeLowerIrql(OldIrql);

  DPRINT("(MiGatherModifiedPages) %d pages on MPW list\n", Count);
}


NTSTATUS STDCALL
MiModifiedPageWriter(IN PVOID  Context)
{
#if 1
  LIST_ENTRY GatherListHead;
  PLIST_ENTRY CurrentEntry;
  PPHYSICAL_PAGE Current;
  PLIST_ENTRY NextEntry;
#endif
  NTSTATUS Status;

	for(;;)
		{
      DPRINT("(MiModifiedPageWriter) Waiting\n");

			Status = KeWaitForSingleObject(&MiModifiedPageWriterEvent,
				0,
				KernelMode,
				FALSE,
				NULL);

      DPRINT("(MiModifiedPageWriter) Waited\n");

			if (!NT_SUCCESS(Status))
				{
          assertmsg(FALSE, ("Wait failed\n"));
				  return(STATUS_UNSUCCESSFUL);
				}

			if (MiModifiedPageWriterShouldTerminate)
				{
				  DPRINT("(MiModifiedPageWriter) Terminating\n");
				  return(STATUS_SUCCESS);
				}
#if 1
      /* FIXME: Locate contiguous modified pages to write out in a single I/O request */

      MiGatherModifiedPages(&GatherListHead);

      CurrentEntry = GatherListHead.Flink;
			while (CurrentEntry != &GatherListHead)
				{
          /* Very important to do this here since the page is put back on
             one of the free page lists/or modified page list */
          NextEntry = CurrentEntry->Flink;

          Current = CONTAINING_RECORD(CurrentEntry, PHYSICAL_PAGE, ListEntry);
#if 0
          DPRINT("Flags 0x%.08x\n", Current->Flags);
          DPRINT("ReferenceCount 0x%.08x\n", Current->ReferenceCount);
          DPRINT("SavedSwapEntry 0x%.08x\n", Current->SavedSwapEntry);
          DPRINT("LockCount 0x%.08x\n", Current->LockCount);
          DPRINT("MapCount 0x%.08x\n", Current->MapCount);
          DPRINT("RmapListHead 0x%.08x\n", Current->RmapListHead);
#endif
          Status = MmFlushPhysicalAddress(MiPageFromDescriptor(Current));
					if (!NT_SUCCESS(Status))
						{
              assertmsg(FALSE, ("MmFlushPhysicalAddress() failed with status 0x%.08x\n", Status));
						}

          CurrentEntry = NextEntry;
        }
#endif
	}
}


NTSTATUS STDCALL
MiMappedPageWriter(IN PVOID  Context)
{
	NTSTATUS Status;

	for(;;)
		{
			Status = KeWaitForSingleObject(&MiMappedPageWriterEvent,
				0,
				KernelMode,
				FALSE,
				NULL);

			if (!NT_SUCCESS(Status))
				{
          assertmsg(FALSE, ("Wait failed\n"));
				  return(STATUS_UNSUCCESSFUL);
				}

			if (MiMappedPageWriterShouldTerminate)
				{
				  DPRINT("(MiMappedPageWriter) Terminating\n");
				  return(STATUS_SUCCESS);
				}
		}
}

static int DelayCount = 0;

VOID
MiSignalModifiedPageWriter()
{
  DelayCount++;
  if (DelayCount > 50) {
    DelayCount = 0;
    KeSetEvent(&MiModifiedPageWriterEvent, IO_NO_INCREMENT, FALSE);
  }
}


VOID
MiSignalMappedPageWriter()
{
  KeSetEvent(&MiMappedPageWriterEvent, IO_NO_INCREMENT, FALSE);
}


VOID
MmInitMpwThreads()
{
	NTSTATUS Status;

	MiModifiedPageWriterShouldTerminate = FALSE;
	KeInitializeEvent(&MiModifiedPageWriterEvent,
		SynchronizationEvent,
		FALSE);

	Status = PsCreateSystemThread(&MiModifiedPageWriterThreadHandle,
		THREAD_ALL_ACCESS,
		NULL,
		NULL,
		&MiModifiedPageWriterThreadId,
		MiModifiedPageWriter,
		NULL);

	if (!NT_SUCCESS(Status))
		{
      assertmsg(FALSE, ("Cannot initialize modified page writer (Status 0x%.08x)\n", Status));
  		return;
		}

  /* Set thread priority to LOW_REALTIME_PRIORITY + 1 */
  Status = PiSetPriorityThread(MiModifiedPageWriterThreadHandle,
    LOW_REALTIME_PRIORITY + 1);

	if (!NT_SUCCESS(Status))
		{
      assertmsg(FALSE, ("Cannot set priority of modified page writer (Status 0x%.08x)\n", Status));
  		return;
		}

	MiMappedPageWriterShouldTerminate = FALSE;
	KeInitializeEvent(&MiMappedPageWriterEvent,
		SynchronizationEvent,
		FALSE);
	
	Status = PsCreateSystemThread(&MiMappedPageWriterThreadHandle,
		THREAD_ALL_ACCESS,
		NULL,
		NULL,
		&MiMappedPageWriterThreadId,
		MiMappedPageWriter,
		NULL);

	if (!NT_SUCCESS(Status))
		{
      assertmsg(FALSE, ("Cannot initialize mapped page writer (Status 0x%.08x)\n", Status));
      return;
		}

  /* Set thread priority to LOW_REALTIME_PRIORITY + 1 */
  Status = PiSetPriorityThread(MiMappedPageWriterThreadHandle,
    LOW_REALTIME_PRIORITY + 1);

	if (!NT_SUCCESS(Status))
		{
      assertmsg(FALSE, ("Cannot set priority of mapped page writer (Status 0x%.08x)\n", Status));
      return;
		}
}


VOID
MiShutdownMpwThreads()
{
  PVOID WaitObjects[2];
  NTSTATUS Status;

  MiModifiedPageWriterShouldTerminate = TRUE;
  MiMappedPageWriterShouldTerminate = TRUE;
  MiSignalModifiedPageWriter();
  MiSignalMappedPageWriter();

	Status = ObReferenceObjectByHandle(MiModifiedPageWriterThreadHandle,
	  THREAD_ALL_ACCESS,
	  PsThreadType,
	  KernelMode,
	  (PVOID *) &WaitObjects[0],
	  NULL);

  if (!NT_SUCCESS(Status))
		{
      /* Silently ignore... */
      return;
		}

	Status = ObReferenceObjectByHandle(MiMappedPageWriterThreadHandle,
	  THREAD_ALL_ACCESS,
	  PsThreadType,
	  KernelMode,
	  (PVOID *) &WaitObjects[1],
	  NULL);

  if (!NT_SUCCESS(Status))
		{
      /* Silently ignore... */
      ObDereferenceObject(WaitObjects[0]);
      return;
		}

	Status = KeWaitForMultipleObjects(2,
		&WaitObjects[0],
		WaitAll,
		0,
		KernelMode,
		FALSE,
		NULL,
		NULL);

	if (!NT_SUCCESS(Status))
		{
      /* Silently ignore... */
		}

  ObDereferenceObject(WaitObjects[0]);
  ObDereferenceObject(WaitObjects[1]);
}
