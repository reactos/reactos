#ifndef __INCLUDE_DDK_NTIFS_H
#define __INCLUDE_DDK_NTIFS_H

struct _BCB;

typedef struct _BCB* PBCB;

struct _MEMORY_AREA;

struct _CACHE_SEGMENT;

typedef struct _CACHE_SEGMENT* PCACHE_SEGMENT;

NTSTATUS STDCALL
CcFlushCacheSegment (struct _CACHE_SEGMENT*	CacheSeg);
NTSTATUS STDCALL
CcReleaseCacheSegment (struct _BCB*		Bcb,
		    struct _CACHE_SEGMENT*	CacheSeg,
		    BOOLEAN		Valid);
NTSTATUS STDCALL
CcRequestCacheSegment (struct _BCB*		Bcb,
		       ULONG		FileOffset,
		       PVOID* BaseAddress,
		       PBOOLEAN	UptoDate,
		       struct _CACHE_SEGMENT** CacheSeg);
NTSTATUS STDCALL
CcInitializeFileCache (PFILE_OBJECT	FileObject,
		       struct _BCB** Bcb,
		       ULONG CacheSegmentSize);
NTSTATUS STDCALL
CcReleaseFileCache (PFILE_OBJECT	FileObject,
		    struct _BCB*		Bcb);

#include <ddk/cctypes.h>

#include <ddk/ccfuncs.h>

#include <ddk/fsfuncs.h>

#endif /* __INCLUDE_DDK_NTIFS_H */
