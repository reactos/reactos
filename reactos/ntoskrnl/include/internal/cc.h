#ifndef __INCLUDE_INTERNAL_CC_H
#define __INCLUDE_INTERNAL_CC_H
/* $Id: cc.h,v 1.4 2001/04/09 02:45:03 dwelch Exp $ */
#include <ddk/ntifs.h>

typedef struct _BCB
{
  LIST_ENTRY CacheSegmentListHead;
  PFILE_OBJECT FileObject;
  KSPIN_LOCK BcbLock;
  ULONG CacheSegmentSize;
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
CcGetCacheSegment(PBCB Bcb,
		  ULONG FileOffset,
		  PULONG BaseOffset,
		  PVOID* BaseAddress,
		  PBOOLEAN UptoDate,
		  PCACHE_SEGMENT* CacheSeg);
VOID
CcInitView(VOID);

#endif
