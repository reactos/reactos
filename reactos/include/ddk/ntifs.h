#ifndef __INCLUDE_DDK_NTIFS_H
#define __INCLUDE_DDK_NTIFS_H


/*
 * Cache manager interfaces (unused)
 */
#if 0

typedef struct _CC_FILE_SIZES
{
   LARGE_INTEGER AllocationSize;
   LARGE_INTEGER FileSize;
   LARGE_INTEGER ValidDataLength;
} CC_FILE_SIZES, *PCC_FILE_SIZES;

typedef struct _CACHE_MANAGER_CALLBACKS
{
   PACQUIRE_FOR_LAZY_WRITE AcquireForLazyWrite;
   PRELEASE_FROM_LAZY_WRITE ReleaseFromLazyWrite;
   PACQUIRE_FOR_READ_AHEAD AcquireForReadAhead;
   PRELEASE_FROM_READ_AHREAD ReleaseFromReadAhead;
} CACHE_MANAGER_CALLBACKS, *PCACHE_MANAGER_CALLBACKS;

VOID CcInitializeCacheMap(PFILE_OBJECT FileObject,
			  PCC_FILE_SIZES FIleSizes,
			  BOOLEAN PinAccess,
			  PCACHE_MANAGER_CALLBACKS CallBacks,
			  PVOID LazyWriterContext);

BOOLEAN CcCopyRead(PFILE_OBJECT FileObject,
		   PLARGE_INTEGER FileOffset,
		   ULONG Length,
		   BOOLEAN Wait,
		   PVOID Buffer,
		   PIO_STATUS_BLOCK IoStatus);

VOID CcFastCopyRead(PFILE_OBJECT FileObject,
		    ULONG FileOffset,
		    ULONG Length,
		    ULONG PageCount,
		    PVOID Buffer,
		    PIO_STATUS_BLOCK IoStatus);

BOOLEAN CcCopyWrite(PFILE_OBJECT FileObject,
		    PLARGE_INTEGER FileOffset,
		    ULONG Length,
		    BOOLEAN Wait,
		    PVOID Buffer);

VOID CcFastCopyWrite(PFILE_OBJECT FileObject,
		     ULONG FileOffset,
		     ULONG Length,
		     PVOID Buffer);

BOOLEAN CcCanIWrite(PFILE_OBJECT FileObject,
		    ULONG BytesToWrite,
		    BOOLEAN Wait,
		    BOOLEAN Retrying);

typedef VOID (*PCC_DEFERRED_WRITE)(PVOID Context1,
				   PVOID Context2);

VOID CcDeferWrite(PFILE_OBJECT FileObject,
		  PCC_POST_DEFERRED_WRITE PostRoutine,
		  PVOID Context1,
		  PVOID Context2,
		  ULONG BytesToWrite,
		  BOOLEAN Retrying);

VOID CcSetReadAheadGranularity(PFILE_OBJECT FileObject,
			       ULONG Granularity);

VOID CcScheduleReadAhead(PFILE_OBJECT FileObject,
			 PLARGE_INTEGER FileObject,
			 ULONG Length);

#define CcReadAhead(FO,FOFF,LEN) { \
        if ((LEN) >= 256) { \
            CcScheduleReadAhead((FO),(FOFF),(LEN)); \
        } \
        } \

BOOLEAN CcMapData(PFILE_OBJECT FileObject,
		  PLARGE_INTEGER FileOffset,
		  ULONG Length,
		  BOOLEAN Wait,
		  PVOID* Bcb,
		  PVOID* Buffer);

BOOLEAN CcPinMappedData(PFILE_OBJECT FileObject,
			PLARGE_INTEGER FileOffset,
			ULONG Length,
			BOOLEAN Wait,
			PVOID* Bcb);

BOOLEAN CcPinRead(PFILE_OBJECT FileObject,
		  PLARGE_INTEGER FIleOffset,
		  ULONG Length,
		  BOOLEAN Wait,
		  PVOID* Bcb,
		  PVOID* Buffer);

VOID CcSetDirtyPinnedData(PVOID Bcb,
			  PLARGE_INTEGER Lsn);

BOOLEAN CcPreparePinWrite(PFILE_OBJECT FileObject,
			  PLARGE_INTEGER FileOffset,
			  ULONG Length,
			  BOOLEAN Zero,
			  BOOLEAN Wait,
			  PVOID* Bcb,
			  PVOID* Buffer);

VOID CcUnpinData(PVOID Bcb);

VOID CcUnpinDataForThread(PVOID Bcb,
			  ERESOURCE_THREAD ResourceThreadId);

VOID CcRepinBcb(PVOID Bcb);

VOID CcUnpinRepinnedBcb(PVOID Bcb,
			BOOLEAN WriteThrough,
			PIO_STATUS_BLOCK IoStatus);

PFILE_OBJECT CcGetFileObjectFromBcb(PVOID Bcb);

VOID CcMdlRead(PFILE_OBJECT FileObject,
	       PLARGE_INTEGER FileOffset,
	       ULONG Length,
	       PMDL* MdlChain,
	       PIO_STATUS_BLOCK IoStatus);

VOID CcMdlReadComplete(PFILE_OBJECT FileObject,
		       PMDL MdlChain);

VOID CcPrepareMdlWrite(PFILE_OBJECT FileObject,
		       PLARGE_INTEGER FileOffset,
		       ULONG Length,
		       PMDL* MdlChain,
		       PIO_STATUS_BLOCK IoStatus);

VOID CcMdlWriteComplete(PFILE_OBJECT FileObject,
			PLARGE_INTEGER FileOffset,
			PMDL MdlChain);

VOID CcFlushCache(PSECTION_OBJECT_POINTERS SectionObjectPointer,
		  PLARGE_INTEGER FileOffset,
		  ULONG Length,
		  PIO_STATUS_BLOCK IoStatus);

typedef struct _CACHE_UNINITIALIZE_EVENT
{
   struct _CACHE_UNINITIALIZE_EVENT* Next;
   KEVENT Event;
} CACHE_UNINITIALIZE_EVENT, *PCACHE_UNINITIALIZE_EVENT;

BOOLEAN CcUninitializeCacheMap(PFILE_OBJECT FileObject,
			  PLARGE_INTEGER TruncateSize,
			  PCACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent);

VOID CcSetFileSizes(PFILE_OBJECT FileObject,
		    PCC_FILE_SIZES FileSizes);

BOOLEAN CcPurgeCacheSection(PSECTION_OBJECT_POINTERS SectionObjectPointer,
			    PLARGE_INTEGER FileObject,
			    ULONG Length,
			    BOOLEAN UninitializeCacheMaps);

VOID CcSetDirtyPageThreshold(PFILE_OBJECT FileObject,
			     ULONG DirtyPageThreshold);

BOOLEAN CcZeroData(PFILE_OBJECT FileObject,
		   PLARGE_INTEGER StartOffset,
		   PLARGE_INTEGER EndOffset,
		   BOOLEAN Wait);

PFILE_OBJECT CcGetFileObjectFromSectionPtrs(
			      PSECTION_OBJECT_POINTERS SectionObjectPointer);

typedef VOID (*PFLUSH_TO_LSN)(PVOID LogHandle, LARGE_INTEGER Lsn);

VOID CcSetLogHandle(PFILE_OBJECT FIleObject,
		    PVOID LogHandle,
		    PFLUSH_TO_LSN FlushToLsnRoutine);

VOID CcSetAdditionalCacheAttributes(PFILE_OBJECT FileObject,
				    BOOLEAN DisableReadAhread,
				    BOOLEAN DisableWriteBehind);

typedef VOID (*PDIRTY_PAGE_ROUTINE)(PFILE_OBJECT FileObject,
				    PLARGE_INTEGER FileOffset,
				    ULONG Length,
				    PLARGE_INTEGER OldestLsn,
				    PLARGE_INTEGER NewestLsn,
				    PVOID Context1,
				    PVOID Context2);

LARGE_INTEGER CcGetDirtyPages(PVOID LogHandle,
			      PDIRTY_PAGE_ROUTINE DirtyPageRoutine,
			      PVOID Context1,
			      PVOID Context2);

BOOLEAN CcIsThereDirtyData(PVPB Vpb);

LARGE_INTEGER CcGetLsnForFileObject(PFILE_OBJECT FileObject,
				    PLARGE_INTEGER OldestLsn);



#endif

#if 0
typedef struct
{
   BOOLEAN Replace;
   HANDLE RootDir;
   ULONG FileNameLength;
   WCHAR FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;
#endif 

#include <ddk/cctypes.h>

#include <ddk/ccfuncs.h>

#endif /* __INCLUDE_DDK_NTIFS_H */
