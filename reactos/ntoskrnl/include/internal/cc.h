#ifndef __INCLUDE_INTERNAL_CC_H
#define __INCLUDE_INTERNAL_CC_H

/* $Id: cc.h,v 1.15 2002/09/07 15:12:51 chorns Exp $ */

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifndef AS_INVOKED

#include <ddk/ntifs.h>

typedef struct _ROS_CACHE_SEGMENT
{
  /* Base address of the region where the cache segment data is mapped. */
  PVOID BaseAddress;
  /* 
   * Memory area representing the region where the cache segment data is 
   * mapped. 
   */
  struct _MEMORY_AREA* MemoryArea;
  /* Are the contents of the cache segment data valid. */
  BOOLEAN Valid;
  /* Are the contents of the cache segment data newer than those on disk. */
  BOOLEAN Dirty;
  ULONG MappedCount;
  /* Entry in the list of segments for this BCB. */
  LIST_ENTRY BcbSegmentListEntry;
  /* Entry in the list of segments which are dirty. */
  LIST_ENTRY DirtySegmentListEntry;
  /* Entry in the list of segments. */
  LIST_ENTRY CacheSegmentListEntry;
  LIST_ENTRY CacheSegmentLRUListEntry;
  /* Offset in the file which this cache segment maps. */
  ULONG FileOffset;
  /* Lock. */
  FAST_MUTEX Lock;
  /* Number of references. */
  ULONG ReferenceCount;
  /* Pointer to the BCB for the file which this cache segment maps data for. */
  PROS_BCB Bcb;
  /* Pointer to the next cache segment in a chain. */
  struct _ROS_CACHE_SEGMENT* NextInChain;
} ROS_CACHE_SEGMENT, *PROS_CACHE_SEGMENT;

VOID STDCALL
CcMdlReadCompleteDev (IN	PMDL		MdlChain,
		      IN	PDEVICE_OBJECT	DeviceObject);
NTSTATUS
CcRosGetCacheSegment(PROS_BCB Bcb,
		  ULONG FileOffset,
		  PULONG BaseOffset,
		  PVOID* BaseAddress,
		  PBOOLEAN UptoDate,
		  PROS_CACHE_SEGMENT* CacheSeg);

NTSTATUS STDCALL 
CcRosFreeCacheSegment(PROS_BCB Bcb, PROS_CACHE_SEGMENT CacheSeg);

VOID
CcInitView(VOID);

NTSTATUS STDCALL CcRosFreeCacheSegment(PROS_BCB, PROS_CACHE_SEGMENT);

NTSTATUS STDCALL 
CcRosRequestCacheSegment(PROS_BCB Bcb,
		      ULONG FileOffset,
		      PVOID* BaseAddress,
		      PBOOLEAN UptoDate,
		      PROS_CACHE_SEGMENT* CacheSeg);

NTSTATUS STDCALL 
CcRosReleaseCacheSegment(PROS_BCB Bcb,
			 PROS_CACHE_SEGMENT CacheSeg,
			 BOOLEAN Valid,
			 BOOLEAN Dirty,
			 BOOLEAN Mapped);

NTSTATUS ReadCacheSegment(PROS_CACHE_SEGMENT CacheSeg);

NTSTATUS WriteCacheSegment(PROS_CACHE_SEGMENT CacheSeg);

VOID CcInit(VOID);
NTSTATUS
CcRosUnmapCacheSegment(PROS_BCB Bcb, ULONG FileOffset, BOOLEAN NowDirty);
NTSTATUS
CcRosSuggestFreeCacheSegment(PROS_BCB Bcb, ULONG FileOffset, BOOLEAN NowDirty);
NTSTATUS
CcRosGetCacheSegmentChain(PROS_BCB Bcb,
			  ULONG FileOffset,
			  ULONG Length,
			  PROS_CACHE_SEGMENT* CacheSeg);

VOID CcInitCacheZeroPage(VOID);

NTSTATUS
CcRosMarkDirtyCacheSegment(PROS_BCB Bcb, ULONG FileOffset);
NTSTATUS
CcRosFlushDirtyPages(ULONG Target, PULONG Count);

VOID CcRosDereferenceCache(PFILE_OBJECT FileObject);
VOID CcRosReferenceCache(PFILE_OBJECT FileObject);

#endif /* !AS_INVOKED */

#endif
