/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/filelock.c
 * PURPOSE:         File Locking implementation for File System Drivers
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
//#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

PAGED_LOOKASIDE_LIST FsRtlFileLockLookasideList;

/* Note: this aligns the two types of lock entry structs so we can access the 
   FILE_LOCK_INFO part in common.  Add elements after Shared if new stuff is needed.
*/
typedef union _COMBINED_LOCK_ELEMENT
{
	struct
	{
		LIST_ENTRY dummy;
		FILE_SHARED_LOCK_ENTRY Shared;
	};
	FILE_EXCLUSIVE_LOCK_ENTRY Exclusive;
}
COMBINED_LOCK_ELEMENT, *PCOMBINED_LOCK_ELEMENT;

typedef struct _LOCK_INFORMATION
{
	RTL_GENERIC_TABLE RangeTable;
	IO_CSQ Csq;
	KSPIN_LOCK CsqLock;
	PFILE_LOCK BelongsTo;
}
LOCK_INFORMATION, *PLOCK_INFORMATION;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
FsRtlCompleteLockIrpReal(IN PCOMPLETE_LOCK_IRP_ROUTINE CompleteRoutine,
                         IN PVOID Context,
                         IN PIRP Irp,
                         IN NTSTATUS Status,
                         OUT PNTSTATUS NewStatus,
                         IN PFILE_OBJECT FileObject OPTIONAL);

/* Generic table methods */

static PVOID NTAPI LockAllocate(PRTL_GENERIC_TABLE Table, CLONG Bytes)
{
	PVOID Result;
	Result = ExAllocatePoolWithTag(NonPagedPool, Bytes, 'FLCK');
	DPRINT("LockAllocate(%d) => %p\n", Bytes, Result);
	return Result;
}

static VOID NTAPI LockFree(PRTL_GENERIC_TABLE Table, PVOID Buffer)
{
	DPRINT("LockFree(%p)\n", Buffer);
	ExFreePoolWithTag(Buffer, 'FLCK');
}

static RTL_GENERIC_COMPARE_RESULTS NTAPI LockCompare
(PRTL_GENERIC_TABLE Table, PVOID PtrA, PVOID PtrB)
{
	PCOMBINED_LOCK_ELEMENT A = PtrA, B = PtrB;
	RTL_GENERIC_COMPARE_RESULTS Result;
	DPRINT("Starting to compare element %x to element %x\n", PtrA, PtrB);
	Result =
		(A->Exclusive.FileLock.EndingByte.QuadPart < 
		 B->Exclusive.FileLock.StartingByte.QuadPart) ? GenericLessThan :
		(A->Exclusive.FileLock.StartingByte.QuadPart > 
		 B->Exclusive.FileLock.EndingByte.QuadPart) ? GenericGreaterThan :
		GenericEqual;
	DPRINT("Compare(%x:%x) %x-%x to %x-%x => %d\n",
		   A,B,
		   A->Exclusive.FileLock.StartingByte.LowPart, 
		   A->Exclusive.FileLock.EndingByte.LowPart,
		   B->Exclusive.FileLock.StartingByte.LowPart, 
		   B->Exclusive.FileLock.EndingByte.LowPart,
		   Result);
	return Result;
}

/* CSQ methods */

static NTSTATUS NTAPI LockInsertIrpEx
(PIO_CSQ Csq,
 PIRP Irp,
 PVOID InsertContext)
{
	PCOMBINED_LOCK_ELEMENT LockElement = InsertContext;
	InsertTailList(&LockElement->Exclusive.ListEntry, &Irp->Tail.Overlay.ListEntry);
	return STATUS_SUCCESS;
}

static VOID NTAPI LockRemoveIrp(PIO_CSQ Csq, PIRP Irp)
{
	RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
}

static PIRP NTAPI LockPeekNextIrp(PIO_CSQ Csq, PIRP Irp, PVOID PeekContext)
{
	// Context will be a COMBINED_LOCK_ELEMENT.  We're looking for a 
	// lock that can be acquired, now that the lock matching PeekContext
	// has been removed.
	COMBINED_LOCK_ELEMENT LockElement;
	PCOMBINED_LOCK_ELEMENT WhereUnlocked = PeekContext, Matching;
	PLOCK_INFORMATION LockInfo = CONTAINING_RECORD(Csq, LOCK_INFORMATION, Csq);
	PFILE_LOCK FileLock = LockInfo->BelongsTo;
	if (!PeekContext)
		return CONTAINING_RECORD
			(Irp->Tail.Overlay.ListEntry.Flink, 
			 IRP, 
			 Tail.Overlay.ListEntry);
	else
	{
		PLIST_ENTRY Following;
		if (!FileLock->LockInformation)
		{
			return CONTAINING_RECORD
				(Irp->Tail.Overlay.ListEntry.Flink,
				 IRP,
				 Tail.Overlay.ListEntry);
		}
		for (Following = Irp->Tail.Overlay.ListEntry.Flink;
			 Following != &WhereUnlocked->Exclusive.ListEntry;
			 Following = Following->Flink)
		{
			PIRP Irp = CONTAINING_RECORD(Following, IRP, Tail.Overlay.ListEntry);
			PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
			LockElement.Exclusive.FileLock.StartingByte = 
				IoStack->Parameters.LockControl.ByteOffset;
			LockElement.Exclusive.FileLock.EndingByte.QuadPart = 
				LockElement.Exclusive.FileLock.StartingByte.QuadPart + 
				IoStack->Parameters.LockControl.Length->QuadPart;
			Matching = RtlLookupElementGenericTable
				(FileLock->LockInformation, &LockElement);
			if (!Matching)
			{
				// This IRP is fine...
				return Irp;
			}
		}
		return NULL;
	}
}

static VOID NTAPI
LockAcquireQueueLock(PIO_CSQ Csq, PKIRQL Irql)
{
	PLOCK_INFORMATION LockInfo = CONTAINING_RECORD(Csq, LOCK_INFORMATION, Csq);
	KeAcquireSpinLock(&LockInfo->CsqLock, Irql);
}

static VOID NTAPI
LockReleaseQueueLock(PIO_CSQ Csq, KIRQL Irql)
{
	PLOCK_INFORMATION LockInfo = CONTAINING_RECORD(Csq, LOCK_INFORMATION, Csq);
	KeReleaseSpinLock(&LockInfo->CsqLock, Irql);
}

static VOID NTAPI
LockCompleteCanceledIrp(PIO_CSQ Csq, PIRP Irp)
{
	NTSTATUS Status;
	PLOCK_INFORMATION LockInfo = CONTAINING_RECORD(Csq, LOCK_INFORMATION, Csq);
	FsRtlCompleteLockIrpReal
		(LockInfo->BelongsTo->CompleteLockIrpRoutine,
		 NULL,
		 Irp,
		 STATUS_CANCELLED,
		 &Status,
		 NULL);
}

VOID
NTAPI
FsRtlCompleteLockIrpReal(IN PCOMPLETE_LOCK_IRP_ROUTINE CompleteRoutine,
                         IN PVOID Context,
                         IN PIRP Irp,
                         IN NTSTATUS Status,
                         OUT PNTSTATUS NewStatus,
                         IN PFILE_OBJECT FileObject OPTIONAL)
{
    /* Check if we have a complete routine */
    if (CompleteRoutine)
    {
        /* Check if we have a file object */
        if (FileObject) FileObject->LastLock = NULL;

        /* Set the I/O Status and do completion */
        Irp->IoStatus.Status = Status;
        *NewStatus = CompleteRoutine(Context, Irp);
    }
    else
    {
        /* Otherwise do a normal I/O complete request */
        FsRtlCompleteRequest(Irp, Status);
        *NewStatus = Status;
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
PFILE_LOCK_INFO
NTAPI
FsRtlGetNextFileLock(IN PFILE_LOCK FileLock,
                     IN BOOLEAN Restart)
{
	PCOMBINED_LOCK_ELEMENT Entry;
	DPRINT("FsRtlGetNextFileLock(%x,%d)\n", FileLock, Restart);
	if (!FileLock->LockInformation)
	{
		DPRINT("No locks at all\n");
		return NULL;
	}
	Entry = RtlEnumerateGenericTable(FileLock->LockInformation, Restart);
	if (!Entry)
	{
		DPRINT("No next entry\n");
		return NULL;
	}
	else
	{
		DPRINT("Lock info %x\n", &Entry->Exclusive.FileLock);
		return &Entry->Exclusive.FileLock;
	}
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlPrivateLock(IN PFILE_LOCK FileLock,
                 IN PFILE_OBJECT FileObject,
                 IN PLARGE_INTEGER FileOffset,
                 IN PLARGE_INTEGER Length,
                 IN PEPROCESS Process,
                 IN ULONG Key,
                 IN BOOLEAN FailImmediately,
                 IN BOOLEAN ExclusiveLock,
                 OUT PIO_STATUS_BLOCK IoStatus,
                 IN PIRP Irp OPTIONAL,
                 IN PVOID Context OPTIONAL,
                 IN BOOLEAN AlreadySynchronized)
{
    NTSTATUS Status;
	COMBINED_LOCK_ELEMENT ToInsert;
	PCOMBINED_LOCK_ELEMENT Conflict;
	PLOCK_INFORMATION LockInfo;
	BOOLEAN InsertedNew;

    DPRINT1("FsRtlPrivateLock(FileLock %x,FileObject %x,FileOffset %x,Length %x,Process %x, Key %x, FailImmediately %x, Exclusive %x)\n",
			FileLock,
			FileObject,
			FileOffset->LowPart,
			Length->LowPart,
			Process,
			Key,
			FailImmediately,
			ExclusiveLock);
	ASSERT(AlreadySynchronized);

    /* Initialize the lock, if necessary */
    if (!FileLock->LockInformation)
    {
		LockInfo = ExAllocatePool(PagedPool, sizeof(LOCK_INFORMATION));
		FileLock->LockInformation = LockInfo;
		if (!FileLock)
		{
			DPRINT("out of memory\n");
			return FALSE;
		}

		LockInfo->BelongsTo = FileLock;

		RtlInitializeGenericTable
			(&LockInfo->RangeTable,
			 LockCompare,
			 LockAllocate,
			 LockFree,
			 NULL);

		KeInitializeSpinLock(&LockInfo->CsqLock);

		IoCsqInitializeEx
			(&LockInfo->Csq, 
			 LockInsertIrpEx,
			 LockRemoveIrp,
			 LockPeekNextIrp,
			 LockAcquireQueueLock,
			 LockReleaseQueueLock,
			 LockCompleteCanceledIrp);
    }

	LockInfo = FileLock->LockInformation;
	ToInsert.Exclusive.FileLock.FileObject = FileObject;
	ToInsert.Exclusive.FileLock.StartingByte = *FileOffset;
	ToInsert.Exclusive.FileLock.EndingByte.QuadPart = FileOffset->QuadPart + Length->QuadPart;
	ToInsert.Exclusive.FileLock.ProcessId = Process->UniqueProcessId;
	ToInsert.Exclusive.FileLock.Key = Key;
	ToInsert.Exclusive.FileLock.ExclusiveLock = ExclusiveLock;

	Conflict = RtlInsertElementGenericTable
		(FileLock->LockInformation,
		 &ToInsert,
		 sizeof(ToInsert),
		 &InsertedNew);

	if (Conflict && !InsertedNew)
	{
		if (Conflict->Exclusive.FileLock.ExclusiveLock || ExclusiveLock)
		{
			if (FailImmediately)
			{
				IoStatus->Status = STATUS_FILE_LOCK_CONFLICT;
				if (Irp)
				{
					FsRtlCompleteLockIrpReal
						(FileLock->CompleteLockIrpRoutine,
						 Context,
						 Irp,
						 IoStatus->Status,
						 &Status,
						 FileObject);
				}
				DPRINT("fail immediately and conflicting lock (%x-%x,%x)\n",
					   Conflict->Exclusive.FileLock.StartingByte.LowPart,
					   Conflict->Exclusive.FileLock.EndingByte.LowPart,
					   Conflict->Exclusive.FileLock.Key);
				return FALSE;
			}
			else
			{
				IoStatus->Status = STATUS_PENDING;
				if (Irp)
				{
					IoMarkIrpPending(Irp);
					IoCsqInsertIrpEx
						(&LockInfo->Csq,
						 Irp,
						 NULL,
						 NULL);
				}
				DPRINT("Pending ...\n");
				return TRUE;
			}
		}
		else
		{
			IoStatus->Status = STATUS_SUCCESS;
			if (Irp)
			{
				FsRtlCompleteLockIrpReal
					(FileLock->CompleteLockIrpRoutine,
					 Context,
					 Irp,
					 IoStatus->Status,
					 &Status,
					 FileObject);
			}
			DPRINT("Yes, completed the lock\n");
			return TRUE;
		}
	}
	else if (!Conflict)
	{
		/* Conflict here is (or would be) the newly inserted element, but we ran
		 * out of space probably. */
		IoStatus->Status = STATUS_NO_MEMORY;
		DPRINT("ran out of memory adding the new lock node\n");
		return FALSE;
	}
	else
	{
		/* Assume all is cool, and lock is set */
		IoStatus->Status = STATUS_SUCCESS;
	
		// Initialize our resource ... We'll use this to mediate access to the
		// irp queue.
		ExInitializeResourceLite(&Conflict->Exclusive.Resource);
	
		if (Irp)
		{
			/* Complete the request */
			FsRtlCompleteLockIrpReal(FileLock->CompleteLockIrpRoutine,
									 Context,
									 Irp,
									 IoStatus->Status,
									 &Status,
									 FileObject);
			
			/* Update the status */
			IoStatus->Status = Status;
		}
		DPRINT("Added %x successfully\n", Conflict);
		return TRUE;
	}
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlCheckLockForReadAccess(IN PFILE_LOCK FileLock,
                            IN PIRP Irp)
{
	BOOLEAN Result;
	PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
	COMBINED_LOCK_ELEMENT ToFind;
	PCOMBINED_LOCK_ELEMENT Found;
	if (!FileLock->LockInformation)
	{
		DPRINT("No locks\n");
		return TRUE;
	}
	ToFind.Exclusive.FileLock.StartingByte = IoStack->Parameters.Read.ByteOffset;
	ToFind.Exclusive.FileLock.EndingByte.QuadPart = 
		ToFind.Exclusive.FileLock.StartingByte.QuadPart + 
		IoStack->Parameters.Read.Length;
	DPRINT("FsRtlCheckLockForReadAccess(%x,%x-%x)\n",
		   FileLock, 
		   ToFind.Exclusive.FileLock.StartingByte.LowPart,
		   ToFind.Exclusive.FileLock.EndingByte.LowPart);
	Found = RtlLookupElementGenericTable
		(FileLock->LockInformation,
		 &ToFind);
	if (!Found)
	{
		DPRINT("Not matched\n");
		return TRUE;
	}
	Result = !Found->Exclusive.FileLock.ExclusiveLock || 
		IoStack->Parameters.Read.Key == Found->Exclusive.FileLock.Key;
	DPRINT("Allowed %d\n", Result);
	return Result;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlCheckLockForWriteAccess(IN PFILE_LOCK FileLock,
                             IN PIRP Irp)
{
	BOOLEAN Result;
	PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
	COMBINED_LOCK_ELEMENT ToFind;
	PCOMBINED_LOCK_ELEMENT Found;
	PEPROCESS Process = Irp->Tail.Overlay.Thread->ThreadsProcess;
	if (!FileLock->LockInformation)
	{
		DPRINT("No locks\n");
		return TRUE;
	}
	ToFind.Exclusive.FileLock.StartingByte = IoStack->Parameters.Write.ByteOffset;
	ToFind.Exclusive.FileLock.EndingByte.QuadPart = 
		ToFind.Exclusive.FileLock.StartingByte.QuadPart + 
		IoStack->Parameters.Write.Length;
	DPRINT("FsRtlCheckLockForReadAccess(%x,%x-%x)\n",
		   FileLock, 
		   ToFind.Exclusive.FileLock.StartingByte.LowPart,
		   ToFind.Exclusive.FileLock.EndingByte.LowPart);
	Found = RtlLookupElementGenericTable
		(FileLock->LockInformation,
		 &ToFind);
	if (!Found) 
	{
		DPRINT("Not matched\n");
		return TRUE;
	}
	Result = Process->UniqueProcessId == Found->Exclusive.FileLock.ProcessId;
	DPRINT("Allowed %d\n", Result);
	return Result;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlFastCheckLockForRead(IN PFILE_LOCK FileLock,
                          IN PLARGE_INTEGER FileOffset,
                          IN PLARGE_INTEGER Length,
                          IN ULONG Key,
                          IN PFILE_OBJECT FileObject,
                          IN PVOID Process)
{
	PEPROCESS EProcess = Process;
	COMBINED_LOCK_ELEMENT ToFind;
	PCOMBINED_LOCK_ELEMENT Found;
	DPRINT("FsRtlFastCheckLockForRead(FileLock %x, FileOffset %x, Length %x, Key %x, FileObject %x, Process %x)\n",
		   FileLock,
		   FileOffset->LowPart,
		   Length->LowPart,
		   Key,
		   FileObject,
		   Process);
	ToFind.Exclusive.FileLock.StartingByte = *FileOffset;
	ToFind.Exclusive.FileLock.EndingByte.QuadPart = 
		FileOffset->QuadPart + Length->QuadPart;
	if (!FileLock->LockInformation)
	{
		DPRINT("Not found\n");
		return TRUE;
	}
	Found = RtlLookupElementGenericTable
		(FileLock->LockInformation,
		 &ToFind);
	if (!Found || !Found->Exclusive.FileLock.ExclusiveLock)
	{
		DPRINT("Not matched\n");
		return TRUE;
	}
	return Found->Exclusive.FileLock.Key == Key && 
		Found->Exclusive.FileLock.ProcessId == EProcess->UniqueProcessId;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlFastCheckLockForWrite(IN PFILE_LOCK FileLock,
                           IN PLARGE_INTEGER FileOffset,
                           IN PLARGE_INTEGER Length,
                           IN ULONG Key,
                           IN PFILE_OBJECT FileObject,
                           IN PVOID Process)
{
	BOOLEAN Result;
	PEPROCESS EProcess = Process;
	COMBINED_LOCK_ELEMENT ToFind;
	PCOMBINED_LOCK_ELEMENT Found;
	DPRINT("FsRtlFastCheckLockForWrite(FileLock %x,FileOffset %x,Length %x,Key %x,FileObject %x, Process %x)\n",
		   FileLock,
		   FileOffset->LowPart,
		   Length->LowPart,
		   Key,
		   FileObject,
		   Process);
	ToFind.Exclusive.FileLock.StartingByte = *FileOffset;
	ToFind.Exclusive.FileLock.EndingByte.QuadPart = 
		FileOffset->QuadPart + Length->QuadPart;
	if (!FileLock->LockInformation)
	{
		DPRINT("Not Found\n");
		return TRUE;
	}
	Found = RtlLookupElementGenericTable
		(FileLock->LockInformation,
		 &ToFind);
	if (!Found)
	{
		DPRINT("Not Matched\n");
		return TRUE;
	}
	Result = Found->Exclusive.FileLock.Key == Key && 
		Found->Exclusive.FileLock.ProcessId == EProcess->UniqueProcessId;
	DPRINT("Allowed: %d\n", Result);
	return Result;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
FsRtlFastUnlockSingle(IN PFILE_LOCK FileLock,
                      IN PFILE_OBJECT FileObject,
                      IN PLARGE_INTEGER FileOffset,
                      IN PLARGE_INTEGER Length,
                      IN PEPROCESS Process,
                      IN ULONG Key,
                      IN PVOID Context OPTIONAL,
                      IN BOOLEAN AlreadySynchronized)
{
	COMBINED_LOCK_ELEMENT Find;
	PCOMBINED_LOCK_ELEMENT Entry;
	PIRP NextMatchingLockIrp;
	PLOCK_INFORMATION InternalInfo = FileLock->LockInformation;
	// The region to unlock must correspond exactly to a previously locked region
	// -- msdn
	DPRINT("FsRtlFastUnlockSingle(FileLock %x, FileObject %x,FileOffset %x,Length %x,Process %x, Key %x)\n",
		   FileLock,
		   FileObject,
		   FileOffset->LowPart,
		   Length->LowPart,
		   Process,
		   Key);
	ASSERT(AlreadySynchronized);
	Find.Exclusive.FileLock.StartingByte = *FileOffset;
	Find.Exclusive.FileLock.EndingByte.QuadPart = 
		FileOffset->QuadPart + Length->QuadPart;
	Entry = RtlLookupElementGenericTable(&InternalInfo->RangeTable, &Find);
	if (!Entry) 
	{
		DPRINT("STATUS_RANGE_NOT_LOCKED\n");
		return STATUS_RANGE_NOT_LOCKED;
	}
	if (Entry->Exclusive.FileLock.Key != Key || 
		Entry->Exclusive.FileLock.ProcessId != Process->UniqueProcessId)
	{
		DPRINT("STATUS_RANGE_NOT_LOCKED\n");
		return STATUS_RANGE_NOT_LOCKED;
	}
	if (Entry->Exclusive.FileLock.StartingByte.QuadPart != FileOffset->QuadPart ||
		Entry->Exclusive.FileLock.EndingByte.QuadPart != 
		FileOffset->QuadPart + Length->QuadPart)
	{
		DPRINT("STATUS_RANGE_NOT_LOCKED\n");	
		return STATUS_RANGE_NOT_LOCKED;
	}
	// this is definitely the thing we want
	RtlCopyMemory(&Find, Entry, sizeof(Find));
	DPRINT("Deleting last IRP\n");
	RtlDeleteElementGenericTable(&InternalInfo->RangeTable, Entry);
	NextMatchingLockIrp = IoCsqRemoveNextIrp(&InternalInfo->Csq, &Find);
	if (NextMatchingLockIrp)
	{
		// Got a new lock irp... try to do the new lock operation
		// Note that we pick an operation that would succeed at the time
		// we looked, but can't guarantee that it won't just be re-queued
		// because somebody else snatched part of the range in a new thread.
		DPRINT("Locking next IRP\n");
		FsRtlProcessFileLock(InternalInfo->BelongsTo, NextMatchingLockIrp, NULL);
	}
	DPRINT("Done\n");
	return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
FsRtlFastUnlockAll(IN PFILE_LOCK FileLock,
                   IN PFILE_OBJECT FileObject,
                   IN PEPROCESS Process,
                   IN PVOID Context OPTIONAL)
{
	PCOMBINED_LOCK_ELEMENT Entry;
	PRTL_GENERIC_TABLE InternalInfo = FileLock->LockInformation;

	DPRINT("FsRtlFastUnlockAll(%x,%x,%x)\n", FileLock, FileObject, Process);

	// XXX Synchronize somehow
	if (!FileLock->LockInformation)
	{
		DPRINT("No locks to unlock!\n");
		return STATUS_SUCCESS;
	}
	for (Entry = RtlEnumerateGenericTable(InternalInfo, TRUE);
			 Entry;
		 Entry = RtlEnumerateGenericTable(InternalInfo, FALSE))
	{
		LARGE_INTEGER Length;
		// We'll take the first one to be the list head, and free the others first...
		Length.QuadPart = 
			Entry->Exclusive.FileLock.EndingByte.QuadPart - 
			Entry->Exclusive.FileLock.StartingByte.QuadPart;
		FsRtlFastUnlockSingle
			(FileLock, 
			 Entry->Exclusive.FileLock.FileObject,
			 &Entry->Exclusive.FileLock.StartingByte,
			 &Length,
			 Entry->Exclusive.FileLock.ProcessId,
			 Entry->Exclusive.FileLock.Key,
			 Context,
			 TRUE);
	}

	DPRINT("Done\n");
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
FsRtlFastUnlockAllByKey(IN PFILE_LOCK FileLock,
                        IN PFILE_OBJECT FileObject,
                        IN PEPROCESS Process,
                        IN ULONG Key,
                        IN PVOID Context OPTIONAL)
{
	PCOMBINED_LOCK_ELEMENT Entry;
	PRTL_GENERIC_TABLE InternalInfo = FileLock->LockInformation;

	DPRINT("FsRtlFastUnlockAllByKey(%x,%x,%x,%x)\n",
		   FileLock, FileObject, Process, Key);
	// XXX Synchronize somehow
	if (!FileLock->LockInformation) return STATUS_RANGE_NOT_LOCKED; // no locks
	for (Entry = RtlEnumerateGenericTable(InternalInfo, TRUE);
			 Entry;
		 Entry = RtlEnumerateGenericTable(InternalInfo, FALSE))
	{
		LARGE_INTEGER Length;
		// We'll take the first one to be the list head, and free the others first...
		Length.QuadPart = 
			Entry->Exclusive.FileLock.EndingByte.QuadPart - 
			Entry->Exclusive.FileLock.StartingByte.QuadPart;
		if (Entry->Exclusive.FileLock.Key == Key && 
			Entry->Exclusive.FileLock.ProcessId == Process->UniqueProcessId)
		{
			FsRtlFastUnlockSingle
				(FileLock, 
				 Entry->Exclusive.FileLock.FileObject,
				 &Entry->Exclusive.FileLock.StartingByte,
				 &Length,
				 Entry->Exclusive.FileLock.ProcessId,
				 Entry->Exclusive.FileLock.Key,
				 Context,
				 TRUE);
		}
	}

	DPRINT("Done\n");
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
FsRtlProcessFileLock(IN PFILE_LOCK FileLock,
                     IN PIRP Irp,
                     IN PVOID Context OPTIONAL)
{
    PIO_STACK_LOCATION IoStackLocation;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Get the I/O Stack location */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStackLocation->MajorFunction == IRP_MJ_LOCK_CONTROL);
	DPRINT("FsRtlProcessFileLock(%x,%x,%x)\n", 
		   FileLock, 
		   IoStackLocation->MajorFunction,
		   IoStackLocation->MinorFunction);

    /* Clear the I/O status block and check what function this is */
    IoStatusBlock.Information = 0;
    switch(IoStackLocation->MinorFunction)
    {
        /* A lock */
        case IRP_MN_LOCK:
			DPRINT("IRP_MN_LOCK\n");
            /* Call the private lock routine */
            FsRtlPrivateLock(FileLock,
                             IoStackLocation->FileObject,
                             &IoStackLocation->
                             Parameters.LockControl.ByteOffset,
                             IoStackLocation->Parameters.LockControl.Length,
                             IoGetRequestorProcess(Irp),
                             IoStackLocation->Parameters.LockControl.Key,
                             IoStackLocation->Flags & SL_FAIL_IMMEDIATELY,
                             IoStackLocation->Flags & SL_EXCLUSIVE_LOCK,
                             &IoStatusBlock,
                             Irp,
                             Context,
                             FALSE);
            break;

        /* A single unlock */
        case IRP_MN_UNLOCK_SINGLE:
			DPRINT("IRP_MN_UNLOCK_SINGLE\n");
            /* Call fast unlock */
            IoStatusBlock.Status =
                FsRtlFastUnlockSingle(FileLock,
                                      IoStackLocation->FileObject,
                                      &IoStackLocation->Parameters.LockControl.
                                      ByteOffset,
                                      IoStackLocation->Parameters.LockControl.
                                      Length,
                                      IoGetRequestorProcess(Irp),
                                      IoStackLocation->Parameters.LockControl.
                                      Key,
                                      Context,
                                      FALSE);

            /* Complete the IRP */
            FsRtlCompleteLockIrpReal(FileLock->CompleteLockIrpRoutine,
                                     Context,
                                     Irp,
                                     IoStatusBlock.Status,
                                     &Status,
                                     NULL);
            break;

        /* Total unlock */
        case IRP_MN_UNLOCK_ALL:
			DPRINT("IRP_MN_UNLOCK_ALL\n");
            /* Do a fast unlock */
            IoStatusBlock.Status = FsRtlFastUnlockAll(FileLock,
                                                      IoStackLocation->
                                                      FileObject,
                                                      IoGetRequestorProcess(Irp),
                                                      Context);

            /* Complete the IRP */
            FsRtlCompleteLockIrpReal(FileLock->CompleteLockIrpRoutine,
                                     Context,
                                     Irp,
                                     IoStatusBlock.Status,
                                     &Status,
                                     NULL);
            break;

        /* Unlock by key */
        case IRP_MN_UNLOCK_ALL_BY_KEY:
			DPRINT("IRP_MN_UNLOCK_ALL_BY_KEY\n");
            /* Do it */
            IoStatusBlock.Status =
                FsRtlFastUnlockAllByKey(FileLock,
                                        IoStackLocation->FileObject,
                                        IoGetRequestorProcess(Irp),
                                        IoStackLocation->Parameters.
                                        LockControl.Key,
                                        Context);

            /* Complete the IRP */
            FsRtlCompleteLockIrpReal(FileLock->CompleteLockIrpRoutine,
                                     Context,
                                     Irp,
                                     IoStatusBlock.Status,
                                     &Status,
                                     NULL);
            break;

        /* Invalid request */
        default:
			DPRINT("Invalid request\n");
            /* Complete it */
            FsRtlCompleteRequest(Irp, STATUS_INVALID_DEVICE_REQUEST);
            IoStatusBlock.Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    /* Return the status */
	DPRINT("Status %x\n", Status);
    return IoStatusBlock.Status;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlInitializeFileLock (IN PFILE_LOCK FileLock,
                         IN PCOMPLETE_LOCK_IRP_ROUTINE CompleteLockIrpRoutine OPTIONAL,
                         IN PUNLOCK_ROUTINE UnlockRoutine OPTIONAL)
{
    /* Setup the lock */
	DPRINT("Initialize %x\n", FileLock);
    FileLock->FastIoIsQuestionable = FALSE;
    FileLock->CompleteLockIrpRoutine = CompleteLockIrpRoutine;
    FileLock->UnlockRoutine = UnlockRoutine;
    FileLock->LockInformation = NULL;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlUninitializeFileLock(IN PFILE_LOCK FileLock)
{
	if (FileLock->LockInformation)
	{
		ASSERT(!RtlNumberGenericTableElements(FileLock->LockInformation));
		ExFreePool(FileLock->LockInformation);
		FileLock->LockInformation = NULL;
	}
}

/*
 * @implemented
 */
PFILE_LOCK
NTAPI
FsRtlAllocateFileLock(IN PCOMPLETE_LOCK_IRP_ROUTINE CompleteLockIrpRoutine OPTIONAL,
                      IN PUNLOCK_ROUTINE UnlockRoutine OPTIONAL)
{
    PFILE_LOCK FileLock;

    /* Try to allocate it */
    FileLock = ExAllocateFromPagedLookasideList(&FsRtlFileLockLookasideList);
    if (FileLock)
    {
        /* Initialize it */
        FsRtlInitializeFileLock(FileLock,
                                CompleteLockIrpRoutine,
                                UnlockRoutine);
    }

    /* Return the lock */
    return FileLock;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlFreeFileLock(IN PFILE_LOCK FileLock)
{
    /* Uninitialize and free the lock */
	DPRINT("FsRtlFreeFileLock(%x)\n", FileLock);
    FsRtlUninitializeFileLock(FileLock);
    ExFreeToPagedLookasideList(&FsRtlFileLockLookasideList, FileLock);
}
