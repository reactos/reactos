#ifndef __INCLUDE_INTERNAL_CC_H
#define __INCLUDE_INTERNAL_CC_H
/* $Id: cc.h,v 1.6 2001/10/10 21:55:13 hbirr Exp $ */
#include <ddk/ntifs.h>

typedef struct _BCB
{
  LIST_ENTRY CacheSegmentListHead;
  PFILE_OBJECT FileObject;
  KSPIN_LOCK BcbLock;
  ULONG CacheSegmentSize;
  LARGE_INTEGER AllocationSize;
  LARGE_INTEGER FileSize;
} BCB;

typedef struct _CACHE_SEGMENT
{
  PVOID BaseAddress;
  struct _MEMORY_AREA* MemoryArea;
  BOOLEAN Valid;
  LIST_ENTRY BcbListEntry;
  LIST_ENTRY DirtySegmentListEntry;
  ULONG FileOffset;
  KEVENT Lock;
  ULONG ReferenceCount;
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

#endif
