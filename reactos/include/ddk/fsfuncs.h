#ifndef __INCLUDE_DDK_FSFUNCS_H
#define __INCLUDE_DDK_FSFUNCS_H
/* $Id: fsfuncs.h,v 1.9 2000/03/10 22:09:16 ea Exp $ */
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
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	);
DWORD
STDCALL
FsRtlBalanceReads (
	DWORD	Unknown0
	);
BOOLEAN
STDCALL
FsRtlCheckLockForReadAccess (
	DWORD	Unknown0,
	DWORD	Unknown1
	);
BOOLEAN
STDCALL
FsRtlCheckLockForWriteAccess (
	DWORD	Unknown0,
	DWORD	Unknown1
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
	IN	DWORD		Unknown0
	);
BOOLEAN
STDCALL
FsRtlDoesNameContainWildCards (
	IN	PUNICODE_STRING	Name
	);
BOOLEAN
STDCALL
FsRtlFastCheckLockForRead (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5
	);
BOOLEAN
STDCALL
FsRtlFastCheckLockForWrite (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5
	);
NTSTATUS
STDCALL
FsRtlFastUnlockAll (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	);
NTSTATUS
STDCALL
FsRtlFastUnlockAllByKey (
	IN	DWORD	Unknown0,
	IN	DWORD	Unknown1,
	IN	DWORD	Unknown2,
	IN	DWORD	Unknown3,
	IN	DWORD	Key
	);
NTSTATUS
STDCALL
FsRtlFastUnlockSingle (
	IN	DWORD	Unknown0,
	IN	DWORD	Unknown1,
	IN	DWORD	Unknown2,
	IN	DWORD	Unknown3,
	IN	DWORD	Unknown4,
	IN	DWORD	Unknown5,
	IN	DWORD	Unknown6,
	IN	DWORD	Unknown7
	);
DWORD
STDCALL
FsRtlGetFileSize (
	DWORD	Unknown0,
	DWORD	Unknown1
	);
NTSTATUS
STDCALL
FsRtlGetNextFileLock (
	IN	DWORD	Unknown0,
	IN OUT	PVOID	Unknown1
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
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
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
	NTSTATUS	NtStatus
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
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5
	);
BOOLEAN
STDCALL
FsRtlMdlReadComplete (
	IN	PFILE_OBJECT	FileObject,
	IN OUT	PMDL		Mdl
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
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6
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
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5
	);
BOOLEAN
STDCALL
FsRtlPrepareMdlWriteDev (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6
	);
NTSTATUS
STDCALL
FsRtlNormalizeNtstatus (
	NTSTATUS	NtStatusToNormalize,
	NTSTATUS	NormalizedNtStatus
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
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6,
	DWORD	Unknown7,
	DWORD	Unknown8,
	DWORD	Unknown9,
	DWORD	Unknown10,
	DWORD	Unknown11
	);
NTSTATUS
STDCALL
FsRtlProcessFileLock (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
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
	IN OUT	PVOID	lpUnknown0
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
