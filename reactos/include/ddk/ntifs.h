#ifndef __INCLUDE_DDK_NTIFS_H
#define __INCLUDE_DDK_NTIFS_H

typedef struct _BCB
{
  LIST_ENTRY CacheSegmentListHead;
  PFILE_OBJECT FileObject;
  KSPIN_LOCK BcbLock;
  ULONG CacheSegmentSize;
} BCB, *PBCB;

struct _MEMORY_AREA;

typedef struct _CACHE_SEGMENT
{
  PVOID BaseAddress;
  struct _MEMORY_AREA* MemoryArea;
  BOOLEAN Valid;
  LIST_ENTRY ListEntry;
  ULONG FileOffset;
  KEVENT Lock;
  ULONG ReferenceCount;
  PBCB Bcb;
} CACHE_SEGMENT, *PCACHE_SEGMENT;

NTSTATUS STDCALL
CcFlushCacheSegment (PCACHE_SEGMENT	CacheSeg);
NTSTATUS STDCALL
CcReleaseCacheSegment (PBCB		Bcb,
		    PCACHE_SEGMENT	CacheSeg,
		    BOOLEAN		Valid);
NTSTATUS STDCALL
CcRequestCacheSegment (PBCB		Bcb,
		       ULONG		FileOffset,
		       PVOID* BaseAddress,
		       PBOOLEAN	UptoDate,
		       PCACHE_SEGMENT* CacheSeg);
NTSTATUS STDCALL
CcInitializeFileCache (PFILE_OBJECT	FileObject,
		       PBCB* Bcb,
		       ULONG CacheSegmentSize);
NTSTATUS STDCALL
CcReleaseFileCache (PFILE_OBJECT	FileObject,
		    PBCB		Bcb);

#include <ddk/cctypes.h>

#include <ddk/ccfuncs.h>

#include <ddk/fsfuncs.h>

#endif /* __INCLUDE_DDK_NTIFS_H */
