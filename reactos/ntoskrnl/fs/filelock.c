/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/fs/filelock.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     No programmer listed.
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/*
NOTE:
I'm not using resource syncronization here, since FsRtlFastCheckLockForRead/Write
are allowed to be called at DISPATCH_LEVEL. Must therefore use nonpaged memory for
the lists.
UPDATE: I'm not sure about this! -Gunnar
*/

FAST_MUTEX              LockTocMutex;
NPAGED_LOOKASIDE_LIST   GrantedLookaside;
NPAGED_LOOKASIDE_LIST   LockTocLookaside;
PAGED_LOOKASIDE_LIST    LockLookaside;

__inline BOOLEAN
IsOverlappingLock(PFILE_LOCK_INFO Lock,
                  PLARGE_INTEGER StartOffset,
                  PLARGE_INTEGER EndOffset)
{
   if ((ULONGLONG)StartOffset->QuadPart > (ULONGLONG)Lock->EndingByte.QuadPart)
   {
      return FALSE;
   }

   if ((ULONGLONG)EndOffset->QuadPart < (ULONGLONG)Lock->StartingByte.QuadPart)
   {
      return FALSE;
   }

   return TRUE;
}

__inline BOOLEAN
IsSurroundingLock(PFILE_LOCK_INFO Lock,
                  PLARGE_INTEGER StartOffset,
                  PLARGE_INTEGER EndOffset)
{
   if ((ULONGLONG)StartOffset->QuadPart >= (ULONGLONG)Lock->StartingByte.QuadPart &&
       (ULONGLONG)EndOffset->QuadPart <= (ULONGLONG)Lock->EndingByte.QuadPart)
   {
      return TRUE;
   }

   return FALSE;
}

VOID
NTAPI
FsRtlInitSystem(VOID)
{
    /* Initialize the list for all lock information structures */
    ExInitializeNPagedLookasideList(&LockTocLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(FILE_LOCK_TOC),
                                    IFS_POOL_TAG,
                                    0);

    /* Initialize the list for granted locks */
    ExInitializeNPagedLookasideList(&GrantedLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(FILE_LOCK_GRANTED),
                                    IFS_POOL_TAG,
                                    0);

    /* Initialize the list for lock allocations */
    ExInitializePagedLookasideList(&LockLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(FILE_LOCK),
                                    IFS_POOL_TAG,
                                    0);

    /* Initialize the lock information mutex */
    ExInitializeFastMutex(&LockTocMutex);
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

VOID
STDCALL
FsRtlpFileLockCancelRoutine(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp)
{
   KIRQL                         oldIrql;
   PKSPIN_LOCK                   SpinLock;

   //don't need this since we have our own sync. protecting irp cancellation
   IoReleaseCancelSpinLock(Irp->CancelIrql);

   SpinLock = Irp->Tail.Overlay.DriverContext[3];

   KeAcquireSpinLock(SpinLock, &oldIrql);

   RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

   KeReleaseSpinLock(SpinLock, oldIrql);

   Irp->IoStatus.Status = STATUS_CANCELLED;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

BOOLEAN
FASTCALL
FsRtlpCheckLockForReadOrWriteAccess(IN PFILE_LOCK FileLock,
                                    IN PLARGE_INTEGER FileOffset,
                                    IN PLARGE_INTEGER Length,
                                    IN ULONG Key,
                                    IN PFILE_OBJECT FileObject,
                                    IN PEPROCESS Process,
                                    IN BOOLEAN Read)
{
   KIRQL                oldirql;
   PFILE_LOCK_TOC       LockToc;
   PFILE_LOCK_GRANTED   Granted;
   LARGE_INTEGER        EndOffset;

   ASSERT(FileLock);

   LockToc = FileLock->LockInformation;

   if (LockToc == NULL || Length->QuadPart == 0)
   {
      return TRUE;
   }

   EndOffset.QuadPart = FileOffset->QuadPart + Length->QuadPart - 1;

   KeAcquireSpinLock(&LockToc->SpinLock, &oldirql);

   LIST_FOR_EACH(Granted, &LockToc->GrantedListHead, FILE_LOCK_GRANTED, ListEntry)
   {
      //if overlapping
      if(IsOverlappingLock(&Granted->Lock, FileOffset, &EndOffset))
      {
         //No read conflict if (shared lock) OR (exclusive + our lock)
         //No write conflict if exclusive lock AND our lock
         if ((Read && !Granted->Lock.ExclusiveLock) ||
            (Granted->Lock.ExclusiveLock &&
            Granted->Lock.ProcessId == Process &&
            Granted->Lock.FileObject == FileObject &&
            Granted->Lock.Key == Key ) )
         {
            //AND if lock surround request region, stop searching and grant
            if (IsSurroundingLock(&Granted->Lock, FileOffset, &EndOffset) )
            {
               KeReleaseSpinLock(&LockToc->SpinLock, oldirql);
               return TRUE;
            }

            //else continue searching for conflicts
            continue;
         }

         //found conflict
         KeReleaseSpinLock(&LockToc->SpinLock, oldirql);
         return FALSE;
      }

   }

   //no conflict
   KeReleaseSpinLock(&LockToc->SpinLock, oldirql);
   return TRUE;
}

NTSTATUS
FASTCALL
FsRtlpFastUnlockAllByKey(IN PFILE_LOCK           FileLock,
                         IN PFILE_OBJECT         FileObject,
                         IN PEPROCESS            Process,
                         IN ULONG                Key,
                         IN BOOLEAN              UseKey,
                         IN PVOID                Context OPTIONAL)
{
    KIRQL				      oldirql;
    PFILE_LOCK_TOC		   LockToc;
    PFILE_LOCK_GRANTED	Granted, tmp;
    BOOLEAN				   Unlock = FALSE;
    //must make local copy since FILE_LOCK struct is allowed to be paged
    BOOLEAN        		GotUnlockRoutine;
    LIST_ENTRY           UnlockedListHead;
    PLIST_ENTRY          EnumEntry;

    ASSERT(FileLock);
    LockToc = FileLock->LockInformation;

    if (LockToc == NULL)
    {
        return STATUS_RANGE_NOT_LOCKED;
    }

    InitializeListHead(&UnlockedListHead);
    GotUnlockRoutine = FileLock->UnlockRoutine != NULL;
    KeAcquireSpinLock(&LockToc->SpinLock, &oldirql);

    LIST_FOR_EACH_SAFE(Granted, tmp, &LockToc->GrantedListHead, FILE_LOCK_GRANTED, ListEntry)
    {

        if (Granted->Lock.ProcessId == Process &&
            Granted->Lock.FileObject == FileObject &&
            (!UseKey || (UseKey && Granted->Lock.Key == Key)) )
        {
            RemoveEntryList(&Granted->ListEntry);
            Unlock = TRUE;

            if (GotUnlockRoutine)
            {
                /*
                Put on unlocked list and call unlock routine for them afterwards.
                This way we don't have to restart enum after each call
                */
                InsertHeadList(&UnlockedListHead,&Granted->ListEntry);
            }
            else
            {
                ExFreeToNPagedLookasideList(&GrantedLookaside,Granted);
            }
        }
    }

    KeReleaseSpinLock(&LockToc->SpinLock, oldirql);

    if (Unlock)
    {
        //call unlock routine for each unlocked lock (if any)
        while (!IsListEmpty(&UnlockedListHead))
        {
            EnumEntry = RemoveTailList(&UnlockedListHead);
            Granted = CONTAINING_RECORD(EnumEntry,FILE_LOCK_GRANTED, ListEntry);

            FileLock->UnlockRoutine(Granted->UnlockContext, &Granted->Lock);
            ExFreeToNPagedLookasideList(&GrantedLookaside,Granted);
        }

        //NOTE: holding spinlock while calling this
        KeAcquireSpinLock(&LockToc->SpinLock, &oldirql);
        FsRtlpCompletePendingLocks(FileLock, LockToc, &oldirql, Context);

        if (IsListEmpty(&LockToc->GrantedListHead))
        {
            KeReleaseSpinLock(&LockToc->SpinLock, oldirql);
            FsRtlAreThereCurrentFileLocks(FileLock) = FALSE;
        }
        else
        {
            KeReleaseSpinLock(&LockToc->SpinLock, oldirql);
        }

        return STATUS_SUCCESS;
    }

    return STATUS_RANGE_NOT_LOCKED;
}

BOOLEAN
FASTCALL
FsRtlpAddLock(IN PFILE_LOCK_TOC LockToc,
              IN PFILE_OBJECT FileObject,
              IN PLARGE_INTEGER FileOffset,
              IN PLARGE_INTEGER Length,
              IN PEPROCESS Process,
              IN ULONG Key,
              IN BOOLEAN ExclusiveLock,
              IN PVOID Context)
{
    PFILE_LOCK_GRANTED   Granted;
    LARGE_INTEGER        EndOffset;

    EndOffset.QuadPart = FileOffset->QuadPart + Length->QuadPart - 1;

    //loop and try to find conflicking locks
    LIST_FOR_EACH(Granted, &LockToc->GrantedListHead, FILE_LOCK_GRANTED, ListEntry)
    {
        if (IsOverlappingLock(&Granted->Lock, FileOffset, &EndOffset))
        {
            //we found a locks that overlap with the new lock

            //if both locks are shared, we might have a fast path outa here...
            if (!Granted->Lock.ExclusiveLock && !ExclusiveLock)
            {
                //if existing lock surround new lock, we know that no other exclusive lock
                //may overlap with our new lock;-D
                if (IsSurroundingLock(&Granted->Lock, FileOffset, &EndOffset))
                {
                    break;
                }

                //else keep locking for conflicts
                continue;
            }

            //we found a conflict:
            //we want shared access to an excl. lock OR exlc. access to a shared lock
            return FALSE;
        }
    }

    Granted = ExAllocateFromNPagedLookasideList(&GrantedLookaside);

    //starting offset
    Granted->Lock.StartingByte = *FileOffset;
    Granted->Lock.Length = *Length;
    Granted->Lock.ExclusiveLock = ExclusiveLock;
    Granted->Lock.Key = Key;
    Granted->Lock.FileObject = FileObject;
    Granted->Lock.ProcessId = Process;
    //ending offset
    Granted->Lock.EndingByte = EndOffset;
    Granted->UnlockContext = Context;

    InsertHeadList(&LockToc->GrantedListHead,&Granted->ListEntry);
    return TRUE;
}

VOID
FASTCALL
FsRtlpCompletePendingLocks(IN PFILE_LOCK FileLock,
                           IN PFILE_LOCK_TOC LockToc,
                           IN OUT PKIRQL oldirql,
                           IN PVOID          Context)
{
    //walk pending list, FIFO order, try 2 complete locks
    PLIST_ENTRY                   EnumEntry;
    PIRP                          Irp, tmp;
    PIO_STACK_LOCATION            Stack;
    LIST_ENTRY                    CompletedListHead;

    InitializeListHead(&CompletedListHead);

    LIST_FOR_EACH_SAFE(Irp, tmp, &LockToc->PendingListHead, IRP, Tail.Overlay.ListEntry)
    {
        Stack = IoGetCurrentIrpStackLocation(Irp);
        if (FsRtlpAddLock(LockToc,
            Stack->FileObject,
            &Stack->Parameters.LockControl.ByteOffset,
            Stack->Parameters.LockControl.Length,
            IoGetRequestorProcess(Irp),
            Stack->Parameters.LockControl.Key,
            Stack->Flags & SL_EXCLUSIVE_LOCK,
            Irp->Tail.Overlay.DriverContext[2] //Context
        ) )
        {
            RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

            if (!IoSetCancelRoutine(Irp, NULL))
            {
                //irp is canceled and cancelroutine will run when we release the lock
                InitializeListHead(&Irp->Tail.Overlay.ListEntry);
                continue;
            }

            /*
            Put on completed list and complete them all afterwards.
            This way we don't have to restart enum after each completion.
            */
            InsertHeadList(&CompletedListHead, &Irp->Tail.Overlay.ListEntry);
        }
    }

    KeReleaseSpinLock(&LockToc->SpinLock, *oldirql);

    //complete irp's (if any)
    while (!IsListEmpty(&CompletedListHead))
    {
        EnumEntry = RemoveTailList(&CompletedListHead);

        Irp = CONTAINING_RECORD(EnumEntry, IRP, Tail.Overlay.ListEntry);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        if (FileLock->CompleteLockIrpRoutine)
        {
            if (FileLock->CompleteLockIrpRoutine(Context, Irp)!=STATUS_SUCCESS)
            {
                Stack = IoGetCurrentIrpStackLocation(Irp);

                //revert
                FsRtlpUnlockSingle ( FileLock,
                    Stack->FileObject,
                    &Stack->Parameters.LockControl.ByteOffset,
                    Stack->Parameters.LockControl.Length,
                    IoGetRequestorProcess(Irp),
                    Stack->Parameters.LockControl.Key,
                    NULL, /* unused context */
                    FALSE /* don't call unlock copletion rout.*/
                    );
            }
        }
        else
        {
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }

    }

    KeAcquireSpinLock(&LockToc->SpinLock, oldirql);
}

NTSTATUS
FASTCALL
FsRtlpUnlockSingle(IN PFILE_LOCK FileLock,
                   IN PFILE_OBJECT FileObject,
                   IN PLARGE_INTEGER FileOffset,
                   IN PLARGE_INTEGER Length,
                   IN PEPROCESS Process,
                   IN ULONG Key,
                   IN PVOID Context OPTIONAL,
                   IN BOOLEAN CallUnlockRoutine)
{
    KIRQL                oldirql;
    PFILE_LOCK_TOC       LockToc;
    PFILE_LOCK_GRANTED   Granted, tmp;

    ASSERT(FileLock);
    LockToc = FileLock->LockInformation;

    if (LockToc == NULL)
    {
        return STATUS_RANGE_NOT_LOCKED;
    }

    KeAcquireSpinLock(&LockToc->SpinLock, &oldirql );

    LIST_FOR_EACH_SAFE(Granted, tmp, &LockToc->GrantedListHead, FILE_LOCK_GRANTED,ListEntry)
    {

        //must be exact match
        if (FileOffset->QuadPart == Granted->Lock.StartingByte.QuadPart &&
            Length->QuadPart == Granted->Lock.Length.QuadPart &&
            Granted->Lock.ProcessId == Process &&
            Granted->Lock.FileObject == FileObject &&
            Granted->Lock.Key == Key)
        {
            RemoveEntryList(&Granted->ListEntry);
            FsRtlpCompletePendingLocks(FileLock, LockToc, &oldirql, Context);

            if (IsListEmpty(&LockToc->GrantedListHead))
            {
                KeReleaseSpinLock(&LockToc->SpinLock, oldirql);

                FsRtlAreThereCurrentFileLocks(FileLock) = FALSE; //paged data
            }
            else
            {
                KeReleaseSpinLock(&LockToc->SpinLock, oldirql);
            }

            if (FileLock->UnlockRoutine && CallUnlockRoutine)
            {
                FileLock->UnlockRoutine(Granted->UnlockContext, &Granted->Lock);
            }

            ExFreeToNPagedLookasideList(&GrantedLookaside, Granted);

            return STATUS_SUCCESS;
        }
    }

    KeReleaseSpinLock(&LockToc->SpinLock, oldirql);

    return STATUS_RANGE_NOT_LOCKED;

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
    /*
    Messy enumeration of granted locks.
    What our last ptr. in LastReturnedLock points at, might have been freed between
    calls, so we have to scan thru the list every time, searching for our last lock.
    If it's not there anymore, restart the enumeration...
    */
    KIRQL                oldirql;
    PLIST_ENTRY          EnumEntry;
    PFILE_LOCK_GRANTED   Granted;
    PFILE_LOCK_TOC       LockToc;
    BOOLEAN              FoundPrevious = FALSE;
    //must make local copy since FILE_LOCK struct is allowed to be in paged mem
    FILE_LOCK_INFO       LocalLastReturnedLockInfo;
    PVOID                LocalLastReturnedLock;

    ASSERT(FileLock);
    LockToc = FileLock->LockInformation;
    if (LockToc == NULL)
    {
        return NULL;
    }

    LocalLastReturnedLock = FileLock->LastReturnedLock;

    KeAcquireSpinLock(&LockToc->SpinLock,&oldirql);

restart:;

    EnumEntry = LockToc->GrantedListHead.Flink;

    if (Restart)
    {
        if (EnumEntry != &LockToc->GrantedListHead)
        {
            Granted = CONTAINING_RECORD(EnumEntry,FILE_LOCK_GRANTED,ListEntry);
            LocalLastReturnedLockInfo = Granted->Lock;
            KeReleaseSpinLock(&LockToc->SpinLock,oldirql);

            FileLock->LastReturnedLockInfo = LocalLastReturnedLockInfo;
            FileLock->LastReturnedLock = EnumEntry;
            return &FileLock->LastReturnedLockInfo;
        }
        else
        {
            KeReleaseSpinLock(&LockToc->SpinLock,oldirql);
            return NULL;
        }
    }

    //else: continue enum
    while (EnumEntry != &LockToc->GrantedListHead)
    {
        //found previous lock?
        if (EnumEntry == LocalLastReturnedLock)
        {
            FoundPrevious = TRUE;
            //get next
            EnumEntry = EnumEntry->Flink;
            if (EnumEntry != &LockToc->GrantedListHead)
            {
                Granted = CONTAINING_RECORD(EnumEntry,FILE_LOCK_GRANTED,ListEntry);
                LocalLastReturnedLockInfo = Granted->Lock;
                KeReleaseSpinLock(&LockToc->SpinLock,oldirql);

                FileLock->LastReturnedLockInfo = LocalLastReturnedLockInfo;
                FileLock->LastReturnedLock = EnumEntry;
                return &FileLock->LastReturnedLockInfo;
            }
            break;
        }
        EnumEntry = EnumEntry->Flink;
    }

    if (!FoundPrevious)
    {
        //got here? uh no, didn't find our last lock..must have been freed...restart
        Restart = TRUE;
        goto restart;
    }

    KeReleaseSpinLock(&LockToc->SpinLock,oldirql);

    return NULL;//no (more) locks
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
                 IN BOOLEAN FailImmediately, //seems meaningless for fast io
                 IN BOOLEAN ExclusiveLock,
                 OUT PIO_STATUS_BLOCK IoStatus,
                 IN PIRP Irp OPTIONAL,
                 IN PVOID Context OPTIONAL,
                 IN BOOLEAN AlreadySynchronized)
{
    PFILE_LOCK_TOC       LockToc;
    KIRQL                oldirql;

    ASSERT(FileLock);
    if (FileLock->LockInformation == NULL)
    {
        ExAcquireFastMutex(&LockTocMutex);
        //still NULL?
        if (FileLock->LockInformation == NULL)
        {
            FileLock->LockInformation = ExAllocateFromNPagedLookasideList(&LockTocLookaside);
            LockToc =  FileLock->LockInformation;
            KeInitializeSpinLock(&LockToc->SpinLock);
            InitializeListHead(&LockToc->GrantedListHead);
            InitializeListHead(&LockToc->PendingListHead);
        }
        ExReleaseFastMutex(&LockTocMutex);
    }

    LockToc =  FileLock->LockInformation;
    KeAcquireSpinLock(&LockToc->SpinLock, &oldirql);

    //try add new lock (while holding spin lock)
    if (FsRtlpAddLock(LockToc,
        FileObject,
        FileOffset,
        Length,
        Process,
        Key,
        ExclusiveLock,
        Context
        ) )
    {
        IoStatus->Status = STATUS_SUCCESS;
    }
    else if (Irp && !FailImmediately)
    {
        //failed + irp + no fail = make. pending

        Irp->Tail.Overlay.DriverContext[3] = &LockToc->SpinLock;
        Irp->Tail.Overlay.DriverContext[2] = Context;

        (void)IoSetCancelRoutine(Irp, FsRtlpFileLockCancelRoutine);
        if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
        {
            //irp was canceled
            KeReleaseSpinLock(&LockToc->SpinLock, oldirql);

            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            return TRUE;
        }

        IoMarkIrpPending(Irp);
        Irp->IoStatus.Status = IoStatus->Status = STATUS_PENDING;
        Irp->IoStatus.Information = 0;
        InsertHeadList(&LockToc->PendingListHead,&Irp->Tail.Overlay.ListEntry);

    }
    else
    {
        IoStatus->Status = STATUS_LOCK_NOT_GRANTED;
    }

    KeReleaseSpinLock(&LockToc->SpinLock, oldirql);	//fires cancel routine

    //never pending if no irp;-)
    ASSERT(!(IoStatus->Status == STATUS_PENDING && !Irp));

    if (IoStatus->Status != STATUS_PENDING)
    {
        if (IoStatus->Status == STATUS_SUCCESS)
        {
            FsRtlAreThereCurrentFileLocks(FileLock) = TRUE;
        }

        if (Irp)
        {
            Irp->IoStatus.Status = IoStatus->Status;
            Irp->IoStatus.Information = 0;
            if (FileLock->CompleteLockIrpRoutine)
            {
                if (FileLock->CompleteLockIrpRoutine(Context,Irp)!=STATUS_SUCCESS)
                {
                    //CompleteLockIrpRoutine complain: revert changes
                    FsRtlpUnlockSingle(  FileLock,
                        FileObject,
                        FileOffset,
                        Length,
                        Process,
                        Key,
                        NULL, /* context */
                        FALSE  /* don't call unlock copletion routine */
                        );
                }
            }
            else
            {
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
        }
    }

    //NOTE: only fast io seems to care about this return value
    return (IoStatus->Status == STATUS_SUCCESS || FailImmediately);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlCheckLockForReadAccess(IN PFILE_LOCK FileLock,
                            IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    LARGE_INTEGER LocalLength;

    /* Get the I/O Stack location and length */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    LocalLength.QuadPart = Stack->Parameters.Read.Length;

    /* Call the internal API */
    return FsRtlpCheckLockForReadOrWriteAccess(FileLock,
                                               &Stack->Parameters.
                                               Read.ByteOffset,
                                               &LocalLength,
                                               Stack->Parameters.Read.Key,
                                               Stack->FileObject,
                                               IoGetRequestorProcess(Irp),
                                               TRUE);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlCheckLockForWriteAccess(IN PFILE_LOCK FileLock,
                             IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    LARGE_INTEGER LocalLength;

    /* Get the I/O Stack location and length */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    LocalLength.QuadPart = Stack->Parameters.Read.Length;

    /* Call the internal API */
    return FsRtlpCheckLockForReadOrWriteAccess(FileLock,
                                               &Stack->Parameters.
                                               Read.ByteOffset,
                                               &LocalLength,
                                               Stack->Parameters.Read.Key,
                                               Stack->FileObject,
                                               IoGetRequestorProcess(Irp),
                                               FALSE);
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
                          IN PEPROCESS Process)
{
    /* Call the internal API */
    return FsRtlpCheckLockForReadOrWriteAccess(FileLock,
                                               FileOffset,
                                               Length,
                                               Key,
                                               FileObject,
                                               Process,
                                               TRUE);
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
                           IN PEPROCESS Process)
{
    /* Call the internal API */
    return FsRtlpCheckLockForReadOrWriteAccess(FileLock,
                                               FileOffset,
                                               Length,
                                               Key,
                                               FileObject,
                                               Process,
                                               FALSE);
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
    /* Call the internal API */
    return FsRtlpUnlockSingle(FileLock,
                              FileObject,
                              FileOffset,
                              Length,
                              Process,
                              Key,
                              Context,
                              TRUE);
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
    /* Call the generic function by process */
    return FsRtlpFastUnlockAllByKey(FileLock,
                                    FileObject,
                                    Process,
                                    0,
                                    FALSE,
                                    Context);
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
    /* Call the generic function by key */
    return FsRtlpFastUnlockAllByKey(FileLock,
                                    FileObject,
                                    Process,
                                    Key,
                                    TRUE,
                                    Context);
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
            break;

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

            /* Complete it */
            FsRtlCompleteRequest(Irp, STATUS_INVALID_DEVICE_REQUEST);
            IoStatusBlock.Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    /* Return the status */
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
    PFILE_LOCK_TOC LockToc;
    PIRP Irp;
    PFILE_LOCK_GRANTED Granted;
    PLIST_ENTRY EnumEntry;
    KIRQL OldIrql;

    /* Get the lock information */
    LockToc = FileLock->LockInformation;
    if (!FileLock->LockInformation) return;

    /* Acquire the lock queue */
    KeAcquireSpinLock(&LockToc->SpinLock, &OldIrql);

    /* Loop the lock tree */
    while (!IsListEmpty(&LockToc->GrantedListHead))
    {
        /* Get the entry */
        EnumEntry = RemoveTailList(&LockToc->GrantedListHead);

        /* Get the granted lock and free it */
        Granted = CONTAINING_RECORD(EnumEntry, FILE_LOCK_GRANTED, ListEntry);
        ExFreeToNPagedLookasideList(&GrantedLookaside, Granted);
    }

    /* Loop the waiting locks */
    while (!IsListEmpty(&LockToc->PendingListHead))
    {
        /* Get the entry and IRP */
        EnumEntry = RemoveTailList(&LockToc->PendingListHead);
        Irp = CONTAINING_RECORD(EnumEntry, IRP, Tail.Overlay.ListEntry);

        /* Release the lock */
        KeReleaseSpinLock(&LockToc->SpinLock, OldIrql);

        /* Acquire cancel spinlock and clear the cancel routine */
        IoAcquireCancelSpinLock(&Irp->CancelIrql);
        (void)IoSetCancelRoutine(Irp, NULL);
        IoReleaseCancelSpinLock(Irp->CancelIrql);

        /* Complete the IRP */
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_RANGE_NOT_LOCKED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        /* Acquire the lock again */
        KeAcquireSpinLock(&LockToc->SpinLock, &OldIrql);
    }

    /* Release the lock and free it */
    KeReleaseSpinLock(&LockToc->SpinLock, OldIrql);
    ExFreeToNPagedLookasideList(&LockTocLookaside, LockToc);

    /* Remove the lock information pointer */
    FileLock->LockInformation = NULL;
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
    FileLock = ExAllocateFromPagedLookasideList(&LockLookaside);
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
    ExFreeToPagedLookasideList(&LockLookaside, FileLock);
}

/* EOF */
