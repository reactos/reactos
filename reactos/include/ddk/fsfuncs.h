#ifndef __INCLUDE_DDK_FSFUNCS_H
#define __INCLUDE_DDK_FSFUNCS_H
/* $Id: fsfuncs.h,v 1.13 2002/03/13 20:41:11 ekohl Exp $ */
#define FlagOn(x,f) ((x) & (f))
VOID
STDCALL
FsRtlAddLargeMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6
	);
VOID
STDCALL
FsRtlAddMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	);
VOID
STDCALL
FsRtlAddToTunnelCache (
    IN PTUNNEL          Cache,
    IN ULONGLONG        DirectoryKey,
    IN PUNICODE_STRING  ShortName,
    IN PUNICODE_STRING  LongName,
    IN BOOLEAN          KeyByShortName,
    IN ULONG            DataLength,
    IN PVOID            Data
    );
PVOID
STDCALL
FsRtlAllocatePool (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes
	);
PVOID
STDCALL
FsRtlAllocatePoolWithQuota (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes
	);
PVOID
STDCALL
FsRtlAllocatePoolWithQuotaTag (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes,
	IN	ULONG		Tag
	);
PVOID
STDCALL
FsRtlAllocatePoolWithTag (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes,
	IN	ULONG		Tag
	);
DWORD
STDCALL
FsRtlAllocateResource (
	VOID
	);
BOOLEAN
STDCALL
FsRtlAreNamesEqual (
	IN	PUNICODE_STRING	Name1,
	IN	PUNICODE_STRING	Name2,
	IN	BOOLEAN		IgnoreCase,
	IN	PWCHAR		UpcaseTable	OPTIONAL
	);
DWORD
STDCALL
FsRtlBalanceReads (
	DWORD	Unknown0
	);
BOOLEAN
STDCALL
FsRtlCheckLockForReadAccess (
	IN PFILE_LOCK   FileLock,
	IN PIRP         Irp
	);
BOOLEAN
STDCALL
FsRtlCheckLockForWriteAccess (
	IN PFILE_LOCK   FileLock,
	IN PIRP         Irp
	);
DWORD
STDCALL
FsRtlCheckOplock (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
BOOLEAN
STDCALL
FsRtlCopyRead (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	BOOLEAN			Wait,
	IN	ULONG			LockKey,
	OUT	PVOID			Buffer,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject
	);
BOOLEAN
STDCALL
FsRtlCopyWrite (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	BOOLEAN			Wait,
	IN	ULONG			LockKey,
	IN	PVOID			Buffer,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject
	);
BOOLEAN
STDCALL
FsRtlCurrentBatchOplock (
	DWORD	Unknown0
	);
VOID
STDCALL
FsRtlDeleteKeyFromTunnelCache (
    IN PTUNNEL      Cache,
    IN ULONGLONG    DirectoryKey
    );
VOID
STDCALL
FsRtlDeleteTunnelCache (
    IN PTUNNEL Cache
    );
VOID
STDCALL
FsRtlDeregisterUncProvider (
	DWORD	Unknown0
	);
VOID
STDCALL
FsRtlDissectDbcs (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	);
VOID
STDCALL
FsRtlDissectName (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	);
BOOLEAN
STDCALL
FsRtlDoesDbcsContainWildCards (
	IN	DWORD	Unknown0
	);
BOOLEAN
STDCALL
FsRtlDoesNameContainWildCards (
	IN	PUNICODE_STRING	Name
	);
BOOLEAN
STDCALL
FsRtlFastCheckLockForRead (
	IN	PFILE_LOCK		FileLock,
	IN	PLARGE_INTEGER		FileOffset,
	IN	PLARGE_INTEGER		Length,
	IN	ULONG			Key,
	IN	PFILE_OBJECT		FileObject,
	IN	PEPROCESS		Process
	);
BOOLEAN
STDCALL
FsRtlFastCheckLockForWrite (
	IN	PFILE_LOCK		FileLock,
	IN	PLARGE_INTEGER		FileOffset,
	IN	PLARGE_INTEGER		Length,
	IN	ULONG			Key,
	IN	PFILE_OBJECT		FileObject,
	IN	PEPROCESS		Process
	);
NTSTATUS
STDCALL
FsRtlFastUnlockAll (
	IN	PFILE_LOCK           FileLock,
	IN	PFILE_OBJECT         FileObject,
	IN	PEPROCESS            Process,
	IN	PVOID                Context OPTIONAL
	);
NTSTATUS
STDCALL
FsRtlFastUnlockAllByKey (
	IN	PFILE_LOCK	FileLock,
	IN	PFILE_OBJECT	FileObject,
	IN	PEPROCESS	Process,
	IN	DWORD		Key,
	IN	PVOID		Context OPTIONAL
	);
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
    );
VOID
STDCALL
FsRtlFindInTunnelCache (
    IN PTUNNEL          Cache,
    IN ULONGLONG        DirectoryKey,
    IN PUNICODE_STRING  Name,
    OUT PUNICODE_STRING ShortName,
    OUT PUNICODE_STRING LongName,
    IN OUT PULONG       DataLength,
    OUT PVOID           Data
    );
NTSTATUS
STDCALL
FsRtlGetFileSize (
	IN	PFILE_OBJECT	FileObject,
	IN OUT	PLARGE_INTEGER	FileSize
	);
PFILE_LOCK_INFO
STDCALL
FsRtlGetNextFileLock (
	IN	PFILE_LOCK	FileLock,
	IN	BOOLEAN		Restart
	);
VOID
STDCALL
FsRtlGetNextLargeMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
VOID
STDCALL
FsRtlGetNextMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
#define FsRtlEnterFileSystem    KeEnterCriticalRegion
#define FsRtlExitFileSystem     KeLeaveCriticalRegion
VOID
STDCALL
FsRtlInitializeFileLock (
	IN PFILE_LOCK                   FileLock,
	IN PCOMPLETE_LOCK_IRP_ROUTINE   CompleteLockIrpRoutine OPTIONAL,
	IN PUNLOCK_ROUTINE              UnlockRoutine OPTIONAL
	);
VOID
STDCALL
FsRtlInitializeLargeMcb (
	DWORD	Unknown0,
	DWORD	Unknown1
	);
VOID
STDCALL
FsRtlInitializeMcb (
	DWORD	Unknown0,
	DWORD	Unknown1
	);
VOID
STDCALL
FsRtlInitializeOplock (
	DWORD	Unknown0
	);
VOID
STDCALL
FsRtlInitializeTunnelCache (
    IN PTUNNEL Cache
    );
BOOLEAN
STDCALL
FsRtlIsDbcsInExpression (
	DWORD	Unknown0,
	DWORD	Unknown1
	);
BOOLEAN
STDCALL
FsRtlIsFatDbcsLegal (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
BOOLEAN
STDCALL
FsRtlIsHpfsDbcsLegal (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
BOOLEAN
STDCALL
FsRtlIsNameInExpression (
	IN	PUNICODE_STRING	Expression,
	IN	PUNICODE_STRING	Name,
	IN	BOOLEAN		IgnoreCase,
	IN	PWCHAR		UpcaseTable	OPTIONAL
	);
BOOLEAN
STDCALL
FsRtlIsNtstatusExpected (
	IN NTSTATUS	NtStatus
	);
BOOLEAN
STDCALL
FsRtlIsTotalDeviceFailure (
	NTSTATUS	NtStatus
	);
#define FsRtlIsUnicodeCharacterWild(C) ( \
    (((C) >= 0x40) ? \
    FALSE : \
    FlagOn((*FsRtlLegalAnsiCharacterArray)[(C)], FSRTL_WILD_CHARACTER )) \
    )
VOID
STDCALL
FsRtlLookupLargeMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6,
	DWORD	Unknown7
	);
VOID
STDCALL
FsRtlLookupLastLargeMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
VOID
STDCALL
FsRtlLookupLastMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
VOID
STDCALL
FsRtlLookupMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
BOOLEAN
STDCALL
FsRtlMdlRead (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	ULONG			LockKey,
	OUT	PMDL			*MdlChain,
	OUT	PIO_STATUS_BLOCK	IoStatus
	);
BOOLEAN
STDCALL
FsRtlMdlReadComplete (
	IN	PFILE_OBJECT	FileObject,
	IN OUT	PMDL		MdlChain
	);
BOOLEAN
STDCALL
FsRtlMdlReadCompleteDev (
	IN	PFILE_OBJECT	FileObject,
	IN	PMDL		MdlChain,
	IN	PDEVICE_OBJECT	DeviceObject
	);
BOOLEAN
STDCALL
FsRtlMdlReadDev (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	ULONG			LockKey,
	OUT	PMDL			*MdlChain,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject
	);
BOOLEAN
STDCALL
FsRtlMdlWriteComplete (
	IN	PFILE_OBJECT	FileObject,
	IN	PLARGE_INTEGER	FileOffset,
	IN	PMDL		MdlChain
	);
BOOLEAN
STDCALL
FsRtlMdlWriteCompleteDev (
	IN	PFILE_OBJECT	FileObject,
	IN	PLARGE_INTEGER	FileOffset,
	IN	PMDL		MdlChain,
	IN	PDEVICE_OBJECT	DeviceObject
	);
VOID
STDCALL
FsRtlNotifyChangeDirectory (
	IN	PNOTIFY_SYNC	NotifySync,
	IN	PVOID		FsContext,
	IN	PSTRING		FullDirectoryName,
	IN	PLIST_ENTRY	NotifyList,
	IN	BOOLEAN		WatchTree,
	IN	ULONG		CompletionFilter,
	IN	PIRP		NotifyIrp
	);
VOID
STDCALL
FsRtlNotifyCleanup (
	IN	PNOTIFY_SYNC	NotifySync,
	IN	PLIST_ENTRY	NotifyList,
	IN	PVOID		FsContext
	);
typedef
BOOLEAN (*PCHECK_FOR_TRAVERSE_ACCESS) (
	IN	PVOID				NotifyContext,
	IN	PVOID				TargetContext,
	IN	PSECURITY_SUBJECT_CONTEXT	SubjectContext
	);
VOID
STDCALL
FsRtlNotifyFullChangeDirectory (
	IN	PNOTIFY_SYNC			NotifySync,
	IN	PLIST_ENTRY			NotifyList,
	IN	PVOID				FsContext,
	IN	PSTRING				FullDirectoryName,
	IN	BOOLEAN				WatchTree,
	IN	BOOLEAN				IgnoreBuffer,
	IN	ULONG				CompletionFilter,
	IN	PIRP				NotifyIrp,
	IN	PCHECK_FOR_TRAVERSE_ACCESS	TraverseCallback	OPTIONAL,
	IN	PSECURITY_SUBJECT_CONTEXT	SubjectContext		OPTIONAL
	);
VOID
STDCALL
FsRtlNotifyFullReportChange (
	IN	PNOTIFY_SYNC	NotifySync,
	IN	PLIST_ENTRY	NotifyList,
	IN	PSTRING		FullTargetName,
	IN	USHORT		TargetNameOffset,
	IN	PSTRING		StreamName OPTIONAL,
	IN	PSTRING		NormalizedParentName	OPTIONAL,
	IN	ULONG		FilterMatch,
	IN	ULONG		Action,
	IN	PVOID		TargetContext
	);
VOID
STDCALL
FsRtlNotifyUninitializeSync (
	IN OUT	PNOTIFY_SYNC	* NotifySync
	);
VOID
STDCALL
FsRtlNotifyReportChange (
	IN	PNOTIFY_SYNC	NotifySync,
	IN	PLIST_ENTRY	NotifyList,
	IN	PSTRING		FullTargetName,
	IN	PUSHORT		FileNamePartLength,
	IN	ULONG		FilterMatch
	);
VOID
STDCALL
FsRtlNotifyInitializeSync (
	IN OUT	PNOTIFY_SYNC	* NotifySync
	);
NTSTATUS
STDCALL
FsRtlNotifyVolumeEvent (
	IN	PFILE_OBJECT	FileObject,
	IN	ULONG	EventCode
	);
NTSTATUS
STDCALL
FsRtlOplockFsctrl (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
BOOLEAN
STDCALL
FsRtlOplockIsFastIoPossible (
	DWORD	Unknown0
	);
BOOLEAN
STDCALL
FsRtlPrepareMdlWrite (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	ULONG			LockKey,
	OUT	PMDL			*MdlChain,
	OUT	PIO_STATUS_BLOCK	IoStatus
	);
BOOLEAN
STDCALL
FsRtlPrepareMdlWriteDev (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	ULONG			LockKey,
	OUT	PMDL			*MdlChain,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject
	);
NTSTATUS
STDCALL
FsRtlNormalizeNtstatus (
	IN	NTSTATUS	NtStatusToNormalize,
	IN	NTSTATUS	NormalizedNtStatus
	);
VOID
STDCALL
FsRtlNumberOfRunsInLargeMcb (
	DWORD	Unknown0
	);
VOID
STDCALL
FsRtlNumberOfRunsInMcb (
	DWORD	Unknown0
	);
VOID
STDCALL
FsRtlPostPagingFileStackOverflow (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
VOID
STDCALL
FsRtlPostStackOverflow (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
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
    );
NTSTATUS
STDCALL
FsRtlProcessFileLock (
    IN PFILE_LOCK   FileLock,
    IN PIRP         Irp,
    IN PVOID        Context OPTIONAL
    );
DWORD
STDCALL
FsRtlRegisterUncProvider (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
VOID
STDCALL
FsRtlRemoveLargeMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
VOID
STDCALL
FsRtlRemoveMcbEntry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
VOID
STDCALL
FsRtlSplitLargeMcb (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
NTSTATUS
STDCALL
FsRtlSyncVolumes (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
VOID
STDCALL
FsRtlTruncateLargeMcb (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);
VOID
STDCALL
FsRtlTruncateMcb (
	DWORD	Unknown0,
	DWORD	Unknown1
	);
VOID
STDCALL
FsRtlUninitializeFileLock (
    IN PFILE_LOCK FileLock
    );
VOID
STDCALL
FsRtlUninitializeLargeMcb (
	DWORD	Unknown0
	);
VOID
STDCALL
FsRtlUninitializeMcb (
	DWORD	Unknown0
	);
DWORD
STDCALL
FsRtlUninitializeOplock (
	DWORD	Unknown0
	);

#endif /* __INCLUDE_DDK_FSFUNCS_H */
