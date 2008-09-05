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

/* PRIVATE FUNCTIONS *********************************************************/

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
    KeBugCheck(FILE_SYSTEM);
    return NULL;
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

    DPRINT1("FsRtlPrivateLock() is stubplemented!\n");

    /* Initialize the lock, if necessary */
    if (!FileLock->LockInformation)
    {
        DPRINT("LockInformation is uninitialized!\n");
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
    KeBugCheck(FILE_SYSTEM);
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlCheckLockForWriteAccess(IN PFILE_LOCK FileLock,
                             IN PIRP Irp)
{
    KeBugCheck(FILE_SYSTEM);
    return FALSE;
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
    KeBugCheck(FILE_SYSTEM);
    return FALSE;
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
    KeBugCheck(FILE_SYSTEM);
    return FALSE;
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
    DPRINT1("FsRtlFastUnlockSingle() is stubplemented!\n");

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
    KeBugCheck(FILE_SYSTEM);
    return STATUS_UNSUCCESSFUL;
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
    KeBugCheck(FILE_SYSTEM);
    return STATUS_UNSUCCESSFUL;
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
    return;
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

