
#ifndef _NTOS_CCFUNCS_H
#define _NTOS_CCFUNCS_H
/* $Id: ccfuncs.h,v 1.5 2000/06/12 14:51:26 ekohl Exp $ */

/* exported variables */
/*
CcFastMdlReadWait
CcFastReadNotPossible
CcFastReadWait
*/

BOOLEAN
STDCALL
CcCanIWrite (
	IN	PFILE_OBJECT	FileObject,
	IN	ULONG		BytesToWrite,
	IN	BOOLEAN		Wait,
	IN	BOOLEAN		Retrying
	);

BOOLEAN
STDCALL
CcCopyRead (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	BOOLEAN			Wait,
	OUT	PVOID			Buffer,
	OUT	PIO_STATUS_BLOCK	IoStatus
	);

BOOLEAN
STDCALL
CcCopyWrite (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	BOOLEAN			Wait,
	IN	PVOID			Buffer
	);

VOID
STDCALL
CcDeferWrite (
	IN	PFILE_OBJECT		FileObject,
	IN	PCC_POST_DEFERRED_WRITE	PostRoutine,
	IN	PVOID			Context1,
	IN	PVOID			Context2,
	IN	ULONG			BytesToWrite,
	IN	BOOLEAN			Retrying
	);

BOOLEAN
STDCALL
CcFastCopyRead (
	IN	PFILE_OBJECT		FileObject,
	IN	ULONG			FileOffset,
	IN	ULONG			Length,
	IN	ULONG			PageCount,
	OUT	PVOID			Buffer,
	OUT	PIO_STATUS_BLOCK	IoStatus
	);

BOOLEAN
STDCALL
CcFastCopyWrite (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	PVOID			Buffer
	);

VOID
STDCALL
CcFlushCache (
	IN	PSECTION_OBJECT_POINTERS	SectionObjectPointer,
	IN	PLARGE_INTEGER			FileOffset OPTIONAL,
	IN	ULONG				Length,
	OUT	PIO_STATUS_BLOCK		IoStatus OPTIONAL
	);

LARGE_INTEGER
STDCALL
CcGetDirtyPages (
	IN	PVOID			LogHandle,
	IN	PDIRTY_PAGE_ROUTINE	DirtyPageRoutine,
	IN	PVOID			Context1,
	IN	PVOID			Context2
	);

PFILE_OBJECT
STDCALL
CcGetFileObjectFromBcb (
	IN	PVOID	Bcb
	);

PFILE_OBJECT
STDCALL
CcGetFileObjectFromSectionPtrs (
	IN	PSECTION_OBJECT_POINTERS	SectionObjectPointer
	);

LARGE_INTEGER
STDCALL
CcGetLsnForFileObject (
	IN	PFILE_OBJECT	FileObject,
	OUT	PLARGE_INTEGER	OldestLsn OPTIONAL
	);

VOID
STDCALL
CcInitializeCacheMap (
	IN	PFILE_OBJECT			FileObject,
	IN	PCC_FILE_SIZES			FileSizes,
	IN	BOOLEAN				PinAccess,
	IN	PCACHE_MANAGER_CALLBACKS	CallBacks,
	IN	PVOID				LazyWriterContext
	);

BOOLEAN
STDCALL
CcIsThereDirtyData (
	IN	PVPB	Vpb
	);

BOOLEAN
STDCALL
CcMapData (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	BOOLEAN			Wait,
	OUT	PVOID			* Bcb,
	OUT	PVOID			* Buffer
	);

VOID
STDCALL
CcMdlRead (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	OUT	PMDL			* MdlChain,
	OUT	PIO_STATUS_BLOCK	IoStatus
	);

VOID
STDCALL
CcMdlReadComplete (
	IN	PFILE_OBJECT	FileObject,
	IN	PMDL		MdlChain
	);

VOID
STDCALL
CcMdlWriteComplete (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	PMDL			MdlChain
	);

BOOLEAN
STDCALL
CcPinMappedData (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	BOOLEAN			Wait,
	OUT	PVOID			* Bcb
	);

BOOLEAN
STDCALL
CcPinRead (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	BOOLEAN			Wait,
	OUT	PVOID			* Bcb,
	OUT	PVOID			* Buffer
	);

VOID
STDCALL
CcPrepareMdlWrite (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	OUT	PMDL			* MdlChain,
	OUT	PIO_STATUS_BLOCK	IoStatus
	);

BOOLEAN
STDCALL
CcPreparePinWrite (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	BOOLEAN			Zero,
	IN	BOOLEAN			Wait,
	OUT	PVOID			* Bcb,
	OUT	PVOID			* Buffer
	);

BOOLEAN
STDCALL
CcPurgeCacheSection (
	IN	PSECTION_OBJECT_POINTERS	SectionObjectPointer,
	IN	PLARGE_INTEGER			FileOffset OPTIONAL,
	IN	ULONG				Length,
	IN	BOOLEAN				UninitializeCacheMaps
	);

#define CcReadAhead(FO,FOFF,LEN) \
{ \
	if ((LEN) >= 256) \
	{ \
		CcScheduleReadAhead((FO),(FOFF),(LEN)); \
	} \
} 

VOID
STDCALL
CcRepinBcb (
	IN	PVOID	Bcb
	);

VOID
STDCALL
CcScheduleReadAhead (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length
	);

VOID
STDCALL
CcSetAdditionalCacheAttributes (
	IN	PFILE_OBJECT	FileObject,
	IN	BOOLEAN		DisableReadAhead,
	IN	BOOLEAN		DisableWriteBehind
	);

VOID
STDCALL
CcSetBcbOwnerPointer (
	IN	PVOID	Bcb,
	IN	PVOID	Owner
	);

VOID
STDCALL
CcSetDirtyPageThreshold (
	IN	PFILE_OBJECT	FileObject,
	IN	ULONG		DirtyPageThreshold
	);

VOID
STDCALL
CcSetDirtyPinnedData (
	IN	PVOID		Bcb,
	IN	PLARGE_INTEGER	Lsn	OPTIONAL
	);

VOID
STDCALL
CcSetFileSizes (
	IN	PFILE_OBJECT	FileObject,
	IN	PCC_FILE_SIZES	FileSizes
	);

VOID
STDCALL
CcSetLogHandleForFile (
	IN	PFILE_OBJECT	FileObject,
	IN	PVOID		LogHandle,
	IN	PFLUSH_TO_LSN	FlushToLsnRoutine
	);

VOID
STDCALL
CcSetReadAheadGranularity (
	IN	PFILE_OBJECT	FileObject,
	IN	ULONG		Granularity
	);

BOOLEAN
STDCALL
CcUninitializeCacheMap (
	IN	PFILE_OBJECT			FileObject,
	IN	PLARGE_INTEGER			TruncateSize OPTIONAL,
	IN	PCACHE_UNINITIALIZE_EVENT	UninitializeCompleteEvent OPTIONAL
	);

VOID
STDCALL
CcUnpinData (
	IN	PVOID	Bcb
	);

VOID
STDCALL
CcUnpinDataForThread (
	IN	PVOID			Bcb,
	IN	ERESOURCE_THREAD	ResourceThreadId
	);

VOID
STDCALL
CcUnpinRepinnedBcb (
	IN	PVOID			Bcb,
	IN	BOOLEAN			WriteThrough,
	IN	PIO_STATUS_BLOCK	IoStatus
	);

BOOLEAN
STDCALL
CcZeroData (
	IN	PFILE_OBJECT	FileObject,
	IN	PLARGE_INTEGER	StartOffset,
	IN	PLARGE_INTEGER	EndOffset,
	IN	BOOLEAN		Wait
	);

#endif

/* EOF */