#ifndef __INCLUDE_DDK_NTIFS_H
#define __INCLUDE_DDK_NTIFS_H

struct _BCB;

typedef struct _BCB* PBCB;

struct _MEMORY_AREA;

struct _CACHE_SEGMENT;

typedef struct _CACHE_SEGMENT* PCACHE_SEGMENT;

NTSTATUS STDCALL
CcRosFlushCacheSegment (struct _CACHE_SEGMENT*	CacheSeg);
NTSTATUS STDCALL
CcRosReleaseCacheSegment (struct _BCB*		Bcb,
		    struct _CACHE_SEGMENT*	CacheSeg,
		    BOOLEAN		Valid);
NTSTATUS STDCALL
CcRosRequestCacheSegment (struct _BCB*		Bcb,
		       ULONG		FileOffset,
		       PVOID* BaseAddress,
		       PBOOLEAN	UptoDate,
		       struct _CACHE_SEGMENT** CacheSeg);
NTSTATUS STDCALL
CcRosInitializeFileCache (PFILE_OBJECT	FileObject,
		       struct _BCB** Bcb,
		       ULONG CacheSegmentSize);
NTSTATUS STDCALL
CcRosReleaseFileCache (PFILE_OBJECT	FileObject,
		    struct _BCB*		Bcb);

#include <ddk/cctypes.h>

#include <ddk/ccfuncs.h>

#include <ddk/fstypes.h>
#include <ddk/fsfuncs.h>

#endif /* __INCLUDE_DDK_NTIFS_H */
