/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/filelock.c
 * PURPOSE:         File Locking implementation for File System Drivers
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
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
    LIST_ENTRY CsqList;
    PFILE_LOCK BelongsTo;
    LIST_ENTRY SharedLocks;
}
    LOCK_INFORMATION, *PLOCK_INFORMATION;

typedef struct _LOCK_SHARED_RANGE
{
    LIST_ENTRY Entry;
    LARGE_INTEGER Start, End;
    ULONG Key;
    PVOID ProcessId;
}
    LOCK_SHARED_RANGE, *PLOCK_SHARED_RANGE;

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
	Result = ExAllocatePoolWithTag(NonPagedPool, Bytes, 'LTAB');
	DPRINT("LockAllocate(%d) => %p\n", Bytes, Result);
	return Result;
}

static VOID NTAPI LockFree(PRTL_GENERIC_TABLE Table, PVOID Buffer)
{
	DPRINT("LockFree(%p)\n", Buffer);
	ExFreePoolWithTag(Buffer, 'LTAB');
}

static RTL_GENERIC_COMPARE_RESULTS NTAPI LockCompare
(PRTL_GENERIC_TABLE Table, PVOID PtrA, PVOID PtrB)
{
	PCOMBINED_LOCK_ELEMENT A = PtrA, B = PtrB;
	RTL_GENERIC_COMPARE_RESULTS Result;
#if 0
    DPRINT("Starting to compare element %x to element %x\n", PtrA, PtrB);
#endif
    /* Match if we overlap */
    if (((A->Exclusive.FileLock.StartingByte.QuadPart <
          B->Exclusive.FileLock.EndingByte.QuadPart) &&
         (A->Exclusive.FileLock.StartingByte.QuadPart >=
          B->Exclusive.FileLock.StartingByte.QuadPart)) ||
        ((B->Exclusive.FileLock.StartingByte.QuadPart <
          A->Exclusive.FileLock.EndingByte.QuadPart) &&
         (B->Exclusive.FileLock.StartingByte.QuadPart >=
          A->Exclusive.FileLock.StartingByte.QuadPart)))
        return GenericEqual;
    /* Otherwise, key on the starting byte */
	Result =
		(A->Exclusive.FileLock.StartingByte.QuadPart < 
		 B->Exclusive.FileLock.StartingByte.QuadPart) ? GenericLessThan :
		(A->Exclusive.FileLock.StartingByte.QuadPart > 
		 B->Exclusive.FileLock.StartingByte.QuadPart) ? GenericGreaterThan :
		GenericEqual;
#if 0
	DPRINT("Compare(%x:%x) %x-%x to %x-%x => %d\n",
		   A,B,
		   A->Exclusive.FileLock.StartingByte.LowPart, 
		   A->Exclusive.FileLock.EndingByte.LowPart,
		   B->Exclusive.FileLock.StartingByte.LowPart, 
		   B->Exclusive.FileLock.EndingByte.LowPart,
		   Result);
#endif
	return Result;
}

/* CSQ methods */

static NTSTATUS NTAPI LockInsertIrpEx
(PIO_CSQ Csq,
 PIRP Irp,
 PVOID InsertContext)
{
	PLOCK_INFORMATION LockInfo = CONTAINING_RECORD(Csq, LOCK_INFORMATION, Csq);
	InsertTailList(&LockInfo->CsqList, &Irp->Tail.Overlay.ListEntry);
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
    PCOMBINED_LOCK_ELEMENT WhereUnlock = PeekContext;
	PLOCK_INFORMATION LockInfo = CONTAINING_RECORD(Csq, LOCK_INFORMATION, Csq);
    PLIST_ENTRY Following;
    DPRINT("PeekNextIrp(IRP %p, Context %p)\n", Irp, PeekContext);
    if (!Irp)
    {
        Following = LockInfo->CsqList.Flink;
    }
    else
        Following = Irp->Tail.Overlay.ListEntry.Flink;
    
    DPRINT("ListEntry %p Head %p\n", Following, &LockInfo->CsqList);
    for (;
         Following != &LockInfo->CsqList;
         Following = Following->Flink)
    {
        PIO_STACK_LOCATION IoStack;
        BOOLEAN Matching;
        Irp = CONTAINING_RECORD(Following, IRP, Tail.Overlay.ListEntry);
        DPRINT("Irp %p\n", Irp);
        IoStack = IoGetCurrentIrpStackLocation(Irp);
        LockElement.Exclusive.FileLock.StartingByte = 
            IoStack->Parameters.LockControl.ByteOffset;
        LockElement.Exclusive.FileLock.EndingByte.QuadPart = 
            LockElement.Exclusive.FileLock.StartingByte.QuadPart + 
            IoStack->Parameters.LockControl.Length->QuadPart;
        /* If a context was specified, it's a range to check to unlock */
        if (WhereUnlock)
        {
            Matching = LockCompare
                (&LockInfo->RangeTable, &LockElement, WhereUnlock) != GenericEqual;
        }
        /* Else get any completable IRP */
        else
        {
            Matching = FALSE;
        }
        if (!Matching)
        {
            // This IRP is fine...
            DPRINT("Returning the IRP %p\n", Irp);
            return Irp;
        }
    }
    DPRINT("Return NULL\n");
    return NULL;
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
	DPRINT("Complete cancelled IRP %p Status %x\n", Irp, STATUS_CANCELLED);
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
        DPRINT("Calling completion routine %p Status %x\n", Irp, Status);
        *NewStatus = CompleteRoutine(Context, Irp);
    }
    else
    {
        /* Otherwise do a normal I/O complete request */
        DPRINT("Completing IRP %p Status %x\n", Irp, Status);
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
	if (!FileLock->LockInformation) return NULL;
	Entry = RtlEnumerateGenericTable(FileLock->LockInformation, Restart);
	if (!Entry) return NULL;
	else return &Entry->Exclusive.FileLock;
}

/* This function expands the conflicting range Conflict by removing and reinserting it,
   then adds a shared range of the same size */
PCOMBINED_LOCK_ELEMENT
NTAPI
FsRtlpSubsumeSharedLock
(PFILE_LOCK FileLock,
 PLOCK_INFORMATION LockInfo,
 PCOMBINED_LOCK_ELEMENT ToInsert,
 PCOMBINED_LOCK_ELEMENT Conflict)
{
    BOOLEAN InsertedNew = FALSE, RemovedOld;
    COMBINED_LOCK_ELEMENT NewElement;
    PLOCK_SHARED_RANGE SharedRange = 
        ExAllocatePoolWithTag(NonPagedPool, sizeof(*SharedRange), 'FSRA');

    if (!SharedRange)
        return NULL;
    
    ASSERT(!Conflict->Exclusive.FileLock.ExclusiveLock);
    ASSERT(!ToInsert->Exclusive.FileLock.ExclusiveLock);
    SharedRange->Start = ToInsert->Exclusive.FileLock.StartingByte;
    SharedRange->End = ToInsert->Exclusive.FileLock.EndingByte;
    SharedRange->Key = ToInsert->Exclusive.FileLock.Key;
    SharedRange->ProcessId = ToInsert->Exclusive.FileLock.ProcessId;
    InsertTailList(&LockInfo->SharedLocks, &SharedRange->Entry);
    
    NewElement = *Conflict;
    if (ToInsert->Exclusive.FileLock.StartingByte.QuadPart >
        Conflict->Exclusive.FileLock.StartingByte.QuadPart)
    {
        NewElement.Exclusive.FileLock.StartingByte = 
            Conflict->Exclusive.FileLock.StartingByte;
    }
    if (ToInsert->Exclusive.FileLock.EndingByte.QuadPart <
        Conflict->Exclusive.FileLock.EndingByte.QuadPart)
    {
        NewElement.Exclusive.FileLock.EndingByte =
            Conflict->Exclusive.FileLock.EndingByte;
    }
    RemovedOld = RtlDeleteElementGenericTable
        (&LockInfo->RangeTable,
         Conflict);
    ASSERT(RemovedOld);
    Conflict = RtlInsertElementGenericTable
        (&LockInfo->RangeTable,
         ToInsert,
         sizeof(*ToInsert),
         &InsertedNew);
    ASSERT(InsertedNew && Conflict);
    return Conflict;
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
    
    DPRINT("FsRtlPrivateLock(%wZ, Offset %08x%08x, Length %08x%08x, Key %x, FailImmediately %d, Exclusive %d)\n", 
           &FileObject->FileName, 
           FileOffset->HighPart,
           FileOffset->LowPart, 
           Length->HighPart,
           Length->LowPart, 
           Key,
           FailImmediately, 
           ExclusiveLock);
    
    if (FileOffset->QuadPart < 0ll || 
        FileOffset->QuadPart + Length->QuadPart < FileOffset->QuadPart)
    {
        DPRINT("File offset out of range\n");
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        if (Irp)
        {
            DPRINT("Complete lock %p Status %x\n", Irp, IoStatus->Status);
            FsRtlCompleteLockIrpReal
                (FileLock->CompleteLockIrpRoutine,
                 Context,
                 Irp,
                 IoStatus->Status,
                 &Status,
                 FileObject);
        }
        return FALSE;
    }
    
    /* Initialize the lock, if necessary */
    if (!FileLock->LockInformation)
    {
		LockInfo = ExAllocatePoolWithTag(NonPagedPool, sizeof(LOCK_INFORMATION), 'FLCK');
		FileLock->LockInformation = LockInfo;
		if (!FileLock) {
            IoStatus->Status = STATUS_NO_MEMORY;
			return FALSE;
        }
        
		LockInfo->BelongsTo = FileLock;
        InitializeListHead(&LockInfo->SharedLocks);
        
		RtlInitializeGenericTable
			(&LockInfo->RangeTable,
			 LockCompare,
			 LockAllocate,
			 LockFree,
			 NULL);
        
		KeInitializeSpinLock(&LockInfo->CsqLock);
        InitializeListHead(&LockInfo->CsqList);
        
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
			}
			return FALSE;
		}
		else
		{
            ULONG i;
            /* We know of at least one lock in range that's shared.  We need to
             * find out if any more exist and any are exclusive. */
            for (i = 0; i < RtlNumberGenericTableElements(&LockInfo->RangeTable); i++)
            {
                Conflict = RtlGetElementGenericTable(&LockInfo->RangeTable, i);
                /* The first argument will be inserted as a shared range */
                if (LockCompare(&LockInfo->RangeTable, Conflict, &ToInsert) == GenericEqual)
                {
                    if (Conflict->Exclusive.FileLock.ExclusiveLock)
                    {
                        /* Found an exclusive match */
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
                        }
                        return FALSE;
                    }
                    else
                    {
                        /* We've made all overlapping shared ranges into one big range
                         * now we need to add a range entry for the new range */
                        DPRINT("Overlapping shared lock %wZ %08x%08x %08x%08x\n",
                               &FileObject->FileName,
                               Conflict->Exclusive.FileLock.StartingByte.HighPart,
                               Conflict->Exclusive.FileLock.StartingByte.LowPart,
                               Conflict->Exclusive.FileLock.EndingByte.HighPart,
                               Conflict->Exclusive.FileLock.EndingByte.LowPart);
                        Conflict = FsRtlpSubsumeSharedLock
                            (FileLock, LockInfo, &ToInsert, Conflict);
                        if (!Conflict)
                        {
                            IoStatus->Status = STATUS_NO_MEMORY;
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
                            return FALSE;
                        }
                    }
                }
            }

            /* We got here because there were only overlapping shared locks */
            DPRINT("Acquired shared lock %wZ %08x%08x %08x%08x\n",
                   &FileObject->FileName,
                   Conflict->Exclusive.FileLock.StartingByte.HighPart,
                   Conflict->Exclusive.FileLock.StartingByte.LowPart,
                   Conflict->Exclusive.FileLock.EndingByte.HighPart,
                   Conflict->Exclusive.FileLock.EndingByte.LowPart);
            if (!FsRtlpSubsumeSharedLock(FileLock, LockInfo, &ToInsert, Conflict))
			{
                IoStatus->Status = STATUS_NO_MEMORY;
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
                return FALSE;
			}
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
			return TRUE;
		}
	}
	else if (!Conflict)
	{
		/* Conflict here is (or would be) the newly inserted element, but we ran
		 * out of space probably. */
		IoStatus->Status = STATUS_NO_MEMORY;
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
		return FALSE;
	}
	else
	{
        DPRINT("Inserted new lock %wZ %08x%08x %08x%08x exclusive %d\n",
               &FileObject->FileName,
               Conflict->Exclusive.FileLock.StartingByte.HighPart,
               Conflict->Exclusive.FileLock.StartingByte.LowPart,
               Conflict->Exclusive.FileLock.EndingByte.HighPart,
               Conflict->Exclusive.FileLock.EndingByte.LowPart,
               Conflict->Exclusive.FileLock.ExclusiveLock);
        if (!ExclusiveLock)
        {
            PLOCK_SHARED_RANGE NewSharedRange;
            NewSharedRange = 
                ExAllocatePoolWithTag(NonPagedPool, sizeof(*NewSharedRange), 'FSRA');
            if (!NewSharedRange)
            {
                IoStatus->Status = STATUS_NO_MEMORY;
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
                return FALSE;
            }
            DPRINT("Adding shared lock %wZ\n", &FileObject->FileName);
            NewSharedRange->Start = ToInsert.Exclusive.FileLock.StartingByte;
            NewSharedRange->End = ToInsert.Exclusive.FileLock.EndingByte;
            NewSharedRange->Key = Key;
            NewSharedRange->ProcessId = ToInsert.Exclusive.FileLock.ProcessId;
            InsertTailList(&LockInfo->SharedLocks, &NewSharedRange->Entry);
        }
        
		/* Assume all is cool, and lock is set */
		IoStatus->Status = STATUS_SUCCESS;
        
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
	}
    
    return TRUE;
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
    DPRINT("CheckLockForReadAccess(%wZ, Offset %08x%08x, Length %x)\n", 
           &IoStack->FileObject->FileName,
           IoStack->Parameters.Read.ByteOffset.HighPart,
           IoStack->Parameters.Read.ByteOffset.LowPart,
           IoStack->Parameters.Read.Length);
	if (!FileLock->LockInformation) {
        DPRINT("CheckLockForReadAccess(%wZ) => TRUE\n", &IoStack->FileObject->FileName);
        return TRUE;
    }
	ToFind.Exclusive.FileLock.StartingByte = IoStack->Parameters.Read.ByteOffset;
	ToFind.Exclusive.FileLock.EndingByte.QuadPart = 
		ToFind.Exclusive.FileLock.StartingByte.QuadPart + 
		IoStack->Parameters.Read.Length;
	Found = RtlLookupElementGenericTable
		(FileLock->LockInformation,
		 &ToFind);
	if (!Found) {
        DPRINT("CheckLockForReadAccess(%wZ) => TRUE\n", &IoStack->FileObject->FileName);
        return TRUE;
    }
	Result = !Found->Exclusive.FileLock.ExclusiveLock || 
		IoStack->Parameters.Read.Key == Found->Exclusive.FileLock.Key;
    DPRINT("CheckLockForReadAccess(%wZ) => %s\n", &IoStack->FileObject->FileName, Result ? "TRUE" : "FALSE");
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
    DPRINT("CheckLockForWriteAccess(%wZ, Offset %08x%08x, Length %x)\n", 
           &IoStack->FileObject->FileName,
           IoStack->Parameters.Write.ByteOffset.HighPart,
           IoStack->Parameters.Write.ByteOffset.LowPart,
           IoStack->Parameters.Write.Length);
	if (!FileLock->LockInformation) {
        DPRINT("CheckLockForWriteAccess(%wZ) => TRUE\n", &IoStack->FileObject->FileName);
        return TRUE;
    }
	ToFind.Exclusive.FileLock.StartingByte = IoStack->Parameters.Write.ByteOffset;
	ToFind.Exclusive.FileLock.EndingByte.QuadPart = 
		ToFind.Exclusive.FileLock.StartingByte.QuadPart + 
		IoStack->Parameters.Write.Length;
	Found = RtlLookupElementGenericTable
		(FileLock->LockInformation,
		 &ToFind);
	if (!Found) {
        DPRINT("CheckLockForWriteAccess(%wZ) => TRUE\n", &IoStack->FileObject->FileName);
        return TRUE;
    }
	Result = Process->UniqueProcessId == Found->Exclusive.FileLock.ProcessId;
    DPRINT("CheckLockForWriteAccess(%wZ) => %s\n", &IoStack->FileObject->FileName, Result ? "TRUE" : "FALSE");
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
    DPRINT("FsRtlFastCheckLockForRead(%wZ, Offset %08x%08x, Length %08x%08x, Key %x)\n", 
           &FileObject->FileName, 
           FileOffset->HighPart,
           FileOffset->LowPart, 
           Length->HighPart,
           Length->LowPart,
           Key);
	ToFind.Exclusive.FileLock.StartingByte = *FileOffset;
	ToFind.Exclusive.FileLock.EndingByte.QuadPart = 
		FileOffset->QuadPart + Length->QuadPart;
	if (!FileLock->LockInformation) return TRUE;
	Found = RtlLookupElementGenericTable
		(FileLock->LockInformation,
		 &ToFind);
	if (!Found || !Found->Exclusive.FileLock.ExclusiveLock) return TRUE;
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
    DPRINT("FsRtlFastCheckLockForWrite(%wZ, Offset %08x%08x, Length %08x%08x, Key %x)\n", 
           &FileObject->FileName, 
           FileOffset->HighPart,
           FileOffset->LowPart, 
           Length->HighPart,
           Length->LowPart,
           Key);
	ToFind.Exclusive.FileLock.StartingByte = *FileOffset;
	ToFind.Exclusive.FileLock.EndingByte.QuadPart = 
		FileOffset->QuadPart + Length->QuadPart;
	if (!FileLock->LockInformation) {
        DPRINT("CheckForWrite(%wZ) => TRUE\n", &FileObject->FileName);
        return TRUE;
    }
	Found = RtlLookupElementGenericTable
		(FileLock->LockInformation,
		 &ToFind);
	if (!Found) {
        DPRINT("CheckForWrite(%wZ) => TRUE\n", &FileObject->FileName);
        return TRUE;
    }
	Result = Found->Exclusive.FileLock.Key == Key && 
		Found->Exclusive.FileLock.ProcessId == EProcess->UniqueProcessId;
    DPRINT("CheckForWrite(%wZ) => %s\n", &FileObject->FileName, Result ? "TRUE" : "FALSE");
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
    BOOLEAN FoundShared = FALSE;
    PLIST_ENTRY SharedEntry;
    PLOCK_SHARED_RANGE SharedRange = NULL;
	COMBINED_LOCK_ELEMENT Find;
	PCOMBINED_LOCK_ELEMENT Entry;
	PIRP NextMatchingLockIrp;
	PLOCK_INFORMATION InternalInfo = FileLock->LockInformation;
    DPRINT("FsRtlFastUnlockSingle(%wZ, Offset %08x%08x, Length %08x%08x, Key %x)\n", 
           &FileObject->FileName, 
           FileOffset->HighPart,
           FileOffset->LowPart, 
           Length->HighPart,
           Length->LowPart,
           Key);
	// The region to unlock must correspond exactly to a previously locked region
	// -- msdn
	// But Windows 2003 doesn't assert on it and simply ignores that parameter
	// ASSERT(AlreadySynchronized);
	Find.Exclusive.FileLock.StartingByte = *FileOffset;
	Find.Exclusive.FileLock.EndingByte.QuadPart = 
		FileOffset->QuadPart + Length->QuadPart;
	Entry = RtlLookupElementGenericTable(&InternalInfo->RangeTable, &Find);
	if (!Entry) {
        DPRINT("Range not locked %wZ\n", &FileObject->FileName);
        return STATUS_RANGE_NOT_LOCKED;
    }

    DPRINT("Found lock entry: Exclusive %d %08x%08x:%08x%08x %wZ\n",
           Entry->Exclusive.FileLock.ExclusiveLock,
           Entry->Exclusive.FileLock.StartingByte.HighPart,
           Entry->Exclusive.FileLock.StartingByte.LowPart,
           Entry->Exclusive.FileLock.EndingByte.HighPart,
           Entry->Exclusive.FileLock.EndingByte.LowPart,
           &FileObject->FileName);
    
    if (Entry->Exclusive.FileLock.ExclusiveLock)
    {
        if (Entry->Exclusive.FileLock.Key != Key ||
            Entry->Exclusive.FileLock.ProcessId != Process->UniqueProcessId ||
            Entry->Exclusive.FileLock.StartingByte.QuadPart != FileOffset->QuadPart ||
            Entry->Exclusive.FileLock.EndingByte.QuadPart != 
            FileOffset->QuadPart + Length->QuadPart)
        {
            DPRINT("Range not locked %wZ\n", &FileObject->FileName);
            return STATUS_RANGE_NOT_LOCKED;
        }
        RtlCopyMemory(&Find, Entry, sizeof(Find));
        // Remove the old exclusive lock region
        RtlDeleteElementGenericTable(&InternalInfo->RangeTable, Entry);
    }
    else
    {
        DPRINT("Shared lock %wZ Start %08x%08x End %08x%08x\n", 
               &FileObject->FileName,
               Entry->Exclusive.FileLock.StartingByte.HighPart,
               Entry->Exclusive.FileLock.StartingByte.LowPart,
               Entry->Exclusive.FileLock.EndingByte.HighPart,
               Entry->Exclusive.FileLock.EndingByte.LowPart);
        for (SharedEntry = InternalInfo->SharedLocks.Flink;
             SharedEntry != &InternalInfo->SharedLocks;
             SharedEntry = SharedEntry->Flink)
        {
            SharedRange = CONTAINING_RECORD(SharedEntry, LOCK_SHARED_RANGE, Entry);
            if (SharedRange->Start.QuadPart == FileOffset->QuadPart &&
                SharedRange->End.QuadPart == FileOffset->QuadPart + Length->QuadPart &&
                SharedRange->Key == Key &&
                SharedRange->ProcessId == Process->UniqueProcessId)
            {
                FoundShared = TRUE;
                DPRINT("Found shared element to delete %wZ Start %08x%08x End %08x%08x Key %x\n",
                       &FileObject->FileName,
                       SharedRange->Start.HighPart,
                       SharedRange->Start.LowPart,
                       SharedRange->End.HighPart,
                       SharedRange->End.LowPart,
                       SharedRange->Key);
                break;
            }
        }
        if (FoundShared)
        {
            Find.Exclusive.FileLock.StartingByte = SharedRange->Start;
            Find.Exclusive.FileLock.EndingByte = SharedRange->End;
            SharedEntry = SharedRange->Entry.Flink;
            RemoveEntryList(&SharedRange->Entry);
            ExFreePool(SharedRange);
        }
        else
        {
            return STATUS_RANGE_NOT_LOCKED;
        }
    }
    
    if (IsListEmpty(&InternalInfo->SharedLocks)) {
        DPRINT("Removing the lock entry %wZ (%08x%08x:%08x%08x)\n", 
               &FileObject->FileName, 
               Entry->Exclusive.FileLock.StartingByte.HighPart, 
               Entry->Exclusive.FileLock.StartingByte.LowPart,
               Entry->Exclusive.FileLock.EndingByte.HighPart, 
               Entry->Exclusive.FileLock.EndingByte.LowPart);
        RtlDeleteElementGenericTable(&InternalInfo->RangeTable, Entry);
    } else {
        DPRINT("Lock still has:\n");
        for (SharedEntry = InternalInfo->SharedLocks.Flink;
             SharedEntry != &InternalInfo->SharedLocks;
             SharedEntry = SharedEntry->Flink)
        {
            SharedRange = CONTAINING_RECORD(SharedEntry, LOCK_SHARED_RANGE, Entry);
            DPRINT("Shared element %wZ Offset %08x%08x Length %08x%08x Key %x\n",
                   &FileObject->FileName,
                   SharedRange->Start.HighPart,
                   SharedRange->Start.LowPart,
                   SharedRange->End.HighPart,
                   SharedRange->End.LowPart,
                   SharedRange->Key);
        }
    }
    
	// this is definitely the thing we want
    NextMatchingLockIrp = IoCsqRemoveNextIrp(&InternalInfo->Csq, &Find);
    while (NextMatchingLockIrp)
    {
        // Got a new lock irp... try to do the new lock operation
        // Note that we pick an operation that would succeed at the time
        // we looked, but can't guarantee that it won't just be re-queued
        // because somebody else snatched part of the range in a new thread.
        DPRINT("Locking another IRP %p for %p %wZ\n", 
               &FileObject->FileName, FileLock, NextMatchingLockIrp);
        FsRtlProcessFileLock(InternalInfo->BelongsTo, NextMatchingLockIrp, NULL);
    }
    
    DPRINT("Success %wZ\n", &FileObject->FileName);
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
    DPRINT("FsRtlFastUnlockAll(%wZ)\n", &FileObject->FileName);
	// XXX Synchronize somehow
	if (!FileLock->LockInformation) {
        DPRINT("Not locked %wZ\n", &FileObject->FileName);
        return STATUS_RANGE_NOT_LOCKED; // no locks
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
    DPRINT("Done %wZ\n", &FileObject->FileName);
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
	PLOCK_INFORMATION InternalInfo = FileLock->LockInformation;
    
    DPRINT("FsRtlFastUnlockAllByKey(%wZ,Key %x)\n", &FileObject->FileName, Key);
    
	// XXX Synchronize somehow
	if (!FileLock->LockInformation) return STATUS_RANGE_NOT_LOCKED; // no locks
	for (Entry = RtlEnumerateGenericTable(&InternalInfo->RangeTable, TRUE);
         Entry;
		 Entry = RtlEnumerateGenericTable(&InternalInfo->RangeTable, FALSE))
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
    
    /* Clear the I/O status block and check what function this is */
    IoStatusBlock.Information = 0;
    
    DPRINT("FsRtlProcessFileLock(%wZ, MinorFunction %x)\n", 
           &IoStackLocation->FileObject->FileName,
           IoStackLocation->MinorFunction);
    
    switch(IoStackLocation->MinorFunction)
    {
        /* A lock */
    case IRP_MN_LOCK:
        
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
        return IoStatusBlock.Status;
        
        /* A single unlock */
    case IRP_MN_UNLOCK_SINGLE:
        
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
        break;
        
        /* Total unlock */
    case IRP_MN_UNLOCK_ALL:
        
        /* Do a fast unlock */
        IoStatusBlock.Status = FsRtlFastUnlockAll(FileLock,
                                                  IoStackLocation->
                                                  FileObject,
                                                  IoGetRequestorProcess(Irp),
                                                  Context);
        break;
        
        /* Unlock by key */
    case IRP_MN_UNLOCK_ALL_BY_KEY:
        
        /* Do it */
        IoStatusBlock.Status =
            FsRtlFastUnlockAllByKey(FileLock,
                                    IoStackLocation->FileObject,
                                    IoGetRequestorProcess(Irp),
                                    IoStackLocation->Parameters.
                                    LockControl.Key,
                                    Context);
        break;
        
        /* Invalid request */
    default:
        
        /* Complete it */
        FsRtlCompleteRequest(Irp, STATUS_INVALID_DEVICE_REQUEST);
        IoStatusBlock.Status = STATUS_INVALID_DEVICE_REQUEST;
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    
    /* Return the status */
    DPRINT("Lock IRP %p %x\n", Irp, IoStatusBlock.Status);
    FsRtlCompleteLockIrpReal
        (FileLock->CompleteLockIrpRoutine,
         Context,
         Irp,
         IoStatusBlock.Status,
         &Status,
         NULL);
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
    RtlZeroMemory(FileLock, sizeof(*FileLock));
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
        PIRP Irp;
        PLOCK_INFORMATION InternalInfo = FileLock->LockInformation;
        PCOMBINED_LOCK_ELEMENT Entry;
        PLIST_ENTRY SharedEntry;
        PLOCK_SHARED_RANGE SharedRange;
        // MSDN: this completes any remaining lock IRPs
        for (SharedEntry = InternalInfo->SharedLocks.Flink;
             SharedEntry != &InternalInfo->SharedLocks;)
        {
            SharedRange = CONTAINING_RECORD(SharedEntry, LOCK_SHARED_RANGE, Entry);
            SharedEntry = SharedEntry->Flink;
            RemoveEntryList(SharedEntry);
            ExFreePool(SharedRange);
        }
        while ((Entry = RtlGetElementGenericTable(&InternalInfo->RangeTable, 0)) != NULL)
        {
            RtlDeleteElementGenericTable(&InternalInfo->RangeTable, Entry);
        }
        while ((Irp = IoCsqRemoveNextIrp(&InternalInfo->Csq, NULL)) != NULL)
        {
            FsRtlProcessFileLock(FileLock, Irp, NULL);
        }
		ExFreePoolWithTag(InternalInfo, 'FLCK');
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
    FsRtlUninitializeFileLock(FileLock);
    ExFreeToPagedLookasideList(&FsRtlFileLockLookasideList, FileLock);
}
