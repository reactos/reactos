#ifndef __INCLUDE_INTERNAL_CC_H
#define __INCLUDE_INTERNAL_CC_H
/* $Id: cc.h,v 1.7 2001/12/27 23:56:42 dwelch Exp $ */
#include <ddk/ntifs.h>

typedef struct _BCB
{
  LIST_ENTRY BcbSegmentListHead;
  PFILE_OBJECT FileObject;
  ULONG CacheSegmentSize;
  LARGE_INTEGER AllocationSize;
  LARGE_INTEGER FileSize;
  KSPIN_LOCK BcbLock;
} BCB;

typedef struct _CACHE_SEGMENT
{
  /* Base address of the region where the cache segment data is mapped. */
  PVOID BaseAddress;
  /* Memory area representing the region where the cache segment data is mapped. */
  struct _MEMORY_AREA* MemoryArea;
  /* Are the contents of the cache segment data valid. */
  BOOLEAN Valid;
  /* Are the contents of the cache segment data newer than those on disk. */
  BOOLEAN Dirty;
  /* Entry in the list of segments for this BCB. */
  LIST_ENTRY BcbSegmentListEntry;
  /* Entry in the list of segments which are dirty. */
  LIST_ENTRY DirtySegmentListEntry;
  /* Entry in the list of segments. */
  LIST_ENTRY CacheSegmentListEntry;
  /* Offset in the file which this cache segment maps. */
  ULONG FileOffset;
  /* Lock. */
  FAST_MUTEX Lock;
  /* Number of references. */
  ULONG ReferenceCount;
  /* Pointer to the BCB for the file which this cache segment maps data for. */
  PBCB Bcb;
} CACHE_SEGMENT;

VOID STDCALL
CcMdlReadCompleteDev (IN	PMDL		MdlChain,
		      IN	PDEVICE_OBJECT	DeviceObject);
NTSTATUS
CcRosGetCacheSegment(PBCB Bcb,
		  ULONG FileOffset,
		  PULONG BaseOffset,
		  PVOID* BaseAddress,
		  PBOOLEAN UptoDate,
		  PCACHE_SEGMENT* CacheSeg);
VOID
CcInitView(VOID);


NTSTATUS STDCALL CcRosFreeCacheSegment(PBCB, PCACHE_SEGMENT);

NTSTATUS ReadCacheSegment(PCACHE_SEGMENT CacheSeg);

NTSTATUS WriteCacheSegment(PCACHE_SEGMENT CacheSeg);

VOID CcInit(VOID);

#endif
