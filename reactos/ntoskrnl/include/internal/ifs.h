#ifndef __INCLUDE_INTERNAL_IFS_H
#define __INCLUDE_INTERNAL_IFS_H
/* $Id: ifs.h,v 1.4 2002/11/13 06:01:11 robd Exp $ */

#include <ddk/ntifs.h>

/* Look for "FSrt" in mem view */
#define IFS_POOL_TAG 0x74725346

VOID STDCALL
FsRtlpInitFileLockingImplementation(VOID);

VOID STDCALL
FsRtlpPendingFileLockCancelRoutine(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

BOOLEAN STDCALL
FsRtlpCheckLockForReadOrWriteAccess(
    IN PFILE_LOCK           FileLock,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN ULONG                Key,
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process,
	IN BOOLEAN				Read	
   );

NTSTATUS STDCALL
FsRtlpFastUnlockAllByKey(
    IN PFILE_LOCK           FileLock,
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process,
    IN DWORD                Key,      /* FIXME: guess */
    IN BOOLEAN              UseKey,   /* FIXME: guess */
    IN PVOID                Context OPTIONAL
    );

NTSTATUS STDCALL
FsRtlpAddLock(
    IN PFILE_LOCK_TOC		LockToc,
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    IN BOOLEAN              ExclusiveLock
	);

VOID STDCALL
FsRtlpTryCompletePendingLocks(
	IN		PFILE_LOCK		FileLock,
	IN		PFILE_LOCK_TOC	LockToc,
	IN OUT	PKIRQL			oldirql
	);

NTSTATUS STDCALL
FsRtlpUnlockSingle(
    IN PFILE_LOCK           FileLock,
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    IN PVOID                Context OPTIONAL,
    IN BOOLEAN              AlreadySynchronized,
	IN BOOLEAN				CallUnlockRoutine
	);

VOID STDCALL
FsRtlpDumpFileLocks(
	IN PFILE_LOCK	FileLock
	);

#endif
