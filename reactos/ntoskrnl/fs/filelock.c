/* $Id: filelock.c,v 1.5 2002/09/08 10:23:20 chorns Exp $
 *
 * reactos/ntoskrnl/fs/filelock.c
 *
 */
#include <ntos.h>
#include <ddk/ntifs.h>


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlCheckLockForReadAccess@8
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTE (Bo Branten)
 *	All this really does is pick out the lock parameters from
 *	the irp (io stack location?), get IoGetRequestorProcess,
 *	and pass values on to FsRtlFastCheckLockForRead.
 *
 */
BOOLEAN
STDCALL
FsRtlCheckLockForReadAccess (
    IN PFILE_LOCK   FileLock,
    IN PIRP         Irp
    )
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlCheckLockForWriteAccess@8
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTE (Bo Branten)
 *	All this really does is pick out the lock parameters from
 *	the irp (io stack location?), get IoGetRequestorProcess,
 *	and pass values on to FsRtlFastCheckLockForWrite.
 */
BOOLEAN
STDCALL
FsRtlCheckLockForWriteAccess (
    IN PFILE_LOCK   FileLock,
    IN PIRP         Irp
    )
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlFastCheckLockForRead@24
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
BOOLEAN
STDCALL
FsRtlFastCheckLockForRead (
    IN PFILE_LOCK           FileLock,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN ULONG                Key,
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process
    )
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlFastCheckLockForWrite@24
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
BOOLEAN
STDCALL
FsRtlFastCheckLockForWrite (
    IN PFILE_LOCK           FileLock,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN ULONG                Key,
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process
    )
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlFastUnlockAll@16
 *	FsRtlFastUnlockAllByKey@20
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
static
NTSTATUS
STDCALL
FsRtlpFastUnlockAllByKey (
    IN PFILE_LOCK           FileLock,
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process,
    IN DWORD                Key,      /* FIXME: guess */
    IN BOOLEAN              UseKey,   /* FIXME: guess */
    IN PVOID                Context OPTIONAL
    )
{
	/* FIXME: */
	return (STATUS_RANGE_NOT_LOCKED);
}


NTSTATUS
STDCALL
FsRtlFastUnlockAll (
    IN PFILE_LOCK           FileLock,
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process,
    IN PVOID                Context OPTIONAL
    )
{
	return FsRtlpFastUnlockAllByKey (
			FileLock,
			FileObject,
			Process,
			0,     /* Key */
			FALSE, /* Do NOT use Key */
			Context
			);
}


NTSTATUS
STDCALL
FsRtlFastUnlockAllByKey (
    IN PFILE_LOCK           FileLock,
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    IN PVOID                Context OPTIONAL
    )
{
	return FsRtlpFastUnlockAllByKey (
			FileLock,
			FileObject,
			Process,
			Key,
			TRUE, /* Use Key */
			Context
			);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlFastUnlockSingle@32
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
NTSTATUS
STDCALL
FsRtlFastUnlockSingle (
    IN PFILE_LOCK           FileLock,
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    IN PVOID                Context OPTIONAL,
    IN BOOLEAN              AlreadySynchronized
    )
{
	return (STATUS_RANGE_NOT_LOCKED);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlGetNextFileLock@8
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *	NULL if no more locks.
 *
 * NOTE (Bo Branten)
 *	Internals: FsRtlGetNextFileLock uses
 *	FileLock->LastReturnedLockInfo and FileLock->LastReturnedLock
 *	as storage. LastReturnedLock is a pointer to the 'raw' lock
 *	inkl. double linked list, and FsRtlGetNextFileLock needs this
 *	to get next lock on subsequent calls with Restart = FALSE.
 */
PFILE_LOCK_INFO
STDCALL
FsRtlGetNextFileLock (
    IN PFILE_LOCK   FileLock,
    IN BOOLEAN      Restart
    )
{
	return (NULL);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlInitializeFileLock@12
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
VOID
STDCALL
FsRtlInitializeFileLock (
    IN PFILE_LOCK                   FileLock,
    IN PCOMPLETE_LOCK_IRP_ROUTINE   CompleteLockIrpRoutine OPTIONAL,
    IN PUNLOCK_ROUTINE              UnlockRoutine OPTIONAL
    )
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlPrivateLock@48
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *	IoStatus->Status: STATUS_PENDING, STATUS_LOCK_NOT_GRANTED
 *
 * NOTE (Bo Branten)
 *	-Calls IoCompleteRequest if Irp
 *	-Uses exception handling / ExRaiseStatus with
 *	 STATUS_INSUFFICIENT_RESOURCES
 */
BOOLEAN
STDCALL
FsRtlPrivateLock (
    IN PFILE_LOCK           FileLock,
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    IN BOOLEAN              FailImmediately, 
    IN BOOLEAN              ExclusiveLock,
    OUT PIO_STATUS_BLOCK    IoStatus, 
    IN PIRP                 Irp OPTIONAL,
    IN PVOID                Context,
    IN BOOLEAN              AlreadySynchronized
    )
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlProcessFileLock@12
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *	-STATUS_INVALID_DEVICE_REQUEST
 *	-STATUS_RANGE_NOT_LOCKED from unlock routines.
 *	-STATUS_PENDING, STATUS_LOCK_NOT_GRANTED from FsRtlPrivateLock
 *	 (redirected IoStatus->Status).
 *
 * NOTE (Bo Branten)
 *	-switch ( Irp->CurrentStackLocation->MinorFunction )
 * 	 lock: return FsRtlPrivateLock;
 *	 unlocksingle: return FsRtlFastUnlockSingle;
 *	 unlockall: return FsRtlFastUnlockAll;
 *	 unlockallbykey: return FsRtlFastUnlockAllByKey;
 *	 default: IofCompleteRequest with STATUS_INVALID_DEVICE_REQUEST;
 *	  return STATUS_INVALID_DEVICE_REQUEST;
 *
 *	-'AllwaysZero' is passed thru as 'AllwaysZero' to lock / unlock routines.
 *	-'Irp' is passet thru as 'Irp' to FsRtlPrivateLock.
 */
NTSTATUS
STDCALL
FsRtlProcessFileLock (
    IN PFILE_LOCK   FileLock,
    IN PIRP         Irp,
    IN PVOID        Context OPTIONAL
    )
{
	return (STATUS_NOT_IMPLEMENTED);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlUninitializeFileLock@4
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
VOID
STDCALL
FsRtlUninitializeFileLock (
    IN PFILE_LOCK FileLock
    )
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlAllocateFileLock@8
 *
 * DESCRIPTION
 * 	Only present in NT 5.0 or later.
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
PFILE_LOCK
STDCALL
FsRtlAllocateFileLock (
    IN PCOMPLETE_LOCK_IRP_ROUTINE   CompleteLockIrpRoutine OPTIONAL,
    IN PUNLOCK_ROUTINE              UnlockRoutine OPTIONAL
    )
{
	return NULL;
}

/* EOF */
