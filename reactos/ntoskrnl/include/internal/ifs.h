#ifndef __INCLUDE_INTERNAL_IFS_H
#define __INCLUDE_INTERNAL_IFS_H
/* $Id: ifs.h,v 1.6.32.1 2004/10/24 22:59:44 ion Exp $ */

#include <ddk/ntifs.h>

typedef struct _FILE_LOCK_TOC {
	KSPIN_LOCK			SpinLock;
	LIST_ENTRY			GrantedListHead;
	LIST_ENTRY			PendingListHead;
	LIST_ENTRY			CompletedListHead;
	LIST_ENTRY			UnlockedListHead;
} FILE_LOCK_TOC, *PFILE_LOCK_TOC;

/* Look for "FSrt" in mem view */
#define IFS_POOL_TAG 0x74725346

VOID STDCALL
FsRtlpInitFileLockingImplementation(VOID);

VOID STDCALL
FsRtlpFileLockCancelRoutine(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

BOOLEAN FASTCALL
FsRtlpCheckLockForReadOrWriteAccess(
    IN PFILE_LOCK           FileLock,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN ULONG                Key,
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process,
	IN BOOLEAN				Read	
   );

NTSTATUS FASTCALL
FsRtlpFastUnlockAllByKey(
    IN PFILE_LOCK           FileLock,
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process,
    IN DWORD                Key,      /* FIXME: guess */
    IN BOOLEAN              UseKey,   /* FIXME: guess */
    IN PVOID                Context OPTIONAL
    );

NTSTATUS FASTCALL
FsRtlpAddLock(
    IN PFILE_LOCK_TOC		LockToc,
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    IN BOOLEAN              ExclusiveLock
	);

VOID FASTCALL
FsRtlpCompletePendingLocks(
	IN		PFILE_LOCK		FileLock,
	IN		PFILE_LOCK_TOC	LockToc,
	IN OUT	PKIRQL			oldirql
	);

NTSTATUS FASTCALL
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

VOID FASTCALL
FsRtlpDumpFileLocks(
	IN PFILE_LOCK	FileLock
	);

#endif
