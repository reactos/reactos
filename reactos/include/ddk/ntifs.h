#ifndef __INCLUDE_DDK_NTIFS_H
#define __INCLUDE_DDK_NTIFS_H

#if 0
typedef struct
{
   BOOLEAN Replace;
   HANDLE RootDir;
   ULONG FileNameLength;
   WCHAR FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;
#endif 

typedef struct _BCB
{
   LIST_ENTRY CacheSegmentListHead;
   PFILE_OBJECT FileObject;
   KSPIN_LOCK BcbLock;
} BCB, *PBCB;

#define CACHE_SEGMENT_SIZE (0x1000)

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

NTSTATUS
STDCALL
CcFlushCachePage (
	PCACHE_SEGMENT	CacheSeg
	);
NTSTATUS
STDCALL
CcReleaseCachePage (
	PBCB		Bcb,
	PCACHE_SEGMENT	CacheSeg,
	BOOLEAN		Valid
	);
NTSTATUS
STDCALL
CcRequestCachePage (
	PBCB		Bcb,
	ULONG		FileOffset,
	PVOID		* BaseAddress,
	PBOOLEAN	UptoDate,
	PCACHE_SEGMENT	* CacheSeg
	);
NTSTATUS
STDCALL
CcInitializeFileCache (
	PFILE_OBJECT	FileObject,
	PBCB		* Bcb
	);
NTSTATUS
STDCALL
CcReleaseFileCache (
	PFILE_OBJECT	FileObject,
	PBCB		Bcb
	);

#include <ddk/cctypes.h>

#include <ddk/ccfuncs.h>

#include <ddk/fsfuncs.h>

#endif /* __INCLUDE_DDK_NTIFS_H */
