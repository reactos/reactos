#ifndef __INCLUDE_DDK_FSFUNCS_H
#define __INCLUDE_DDK_FSFUNCS_H
/* $Id: fsfuncs.h,v 1.5 2000/02/25 23:27:50 ea Exp $ */
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
DWORD
STDCALL
FsRtlGetFileSize (
	DWORD	Unknown0,
	DWORD	Unknown1
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
FsRtlUninitializeLargeMcb (
	DWORD	Unknown0
	);
VOID
STDCALL
FsRtlUninitializeMcb (
	DWORD	Unknown0
	);

#endif /* __INCLUDE_DDK_FSFUNCS_H */
