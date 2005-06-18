#ifndef __INCLUDE_INTERNAL_IFS_H
#define __INCLUDE_INTERNAL_IFS_H

/* Look for "FSrt" in mem view */
#define IFS_POOL_TAG 0x74725346

VOID
STDCALL INIT_FUNCTION
FsRtlpInitNotifyImplementation(VOID);


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

BOOLEAN FASTCALL
FsRtlpAddLock(
    IN PFILE_LOCK_TOC		LockToc,
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    IN BOOLEAN              ExclusiveLock,
    IN PVOID                UnlockContext
	);

VOID FASTCALL
FsRtlpCompletePendingLocks(
	IN		PFILE_LOCK		FileLock,
	IN		PFILE_LOCK_TOC	LockToc,
	IN OUT	PKIRQL			oldirql,
   IN    PVOID          Context
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
	IN BOOLEAN				CallUnlockRoutine
	);

VOID FASTCALL
FsRtlpDumpFileLocks(
	IN PFILE_LOCK	FileLock
	);

#endif
