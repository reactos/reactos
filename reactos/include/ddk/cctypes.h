#ifndef __INCLUDE_DDK_CCTYPES_H
#define __INCLUDE_DDK_CCTYPES_H

typedef struct _CACHE_UNINITIALIZE_EVENT
{
	struct _CACHE_UNINITIALIZE_EVENT	* Next;
	KEVENT					Event;
} CACHE_UNINITIALIZE_EVENT, *PCACHE_UNINITIALIZE_EVENT;

typedef struct _CC_FILE_SIZES
{
	LARGE_INTEGER	AllocationSize;
	LARGE_INTEGER	FileSize;
	LARGE_INTEGER	ValidDataLength;
} CC_FILE_SIZES, *PCC_FILE_SIZES;


typedef VOID (*PCC_POST_DEFERRED_WRITE)(IN PVOID Context1, IN PVOID Context2);

typedef struct _PUBLIC_BCB
{
    CSHORT          NodeTypeCode;
    CSHORT          NodeByteSize;
    ULONG           MappedLength;
    LARGE_INTEGER   MappedFileOffset;
} PUBLIC_BCB, *PPUBLIC_BCB;

typedef VOID (*PDIRTY_PAGE_ROUTINE) (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN ULONG            Length,
    IN PLARGE_INTEGER   OldestLsn,
    IN PLARGE_INTEGER   NewestLsn,
    IN PVOID            Context1,
    IN PVOID            Context2
);

typedef BOOLEAN (*PACQUIRE_FOR_LAZY_WRITE) (
    IN PVOID    Context,
    IN BOOLEAN  Wait
);

typedef VOID (*PRELEASE_FROM_LAZY_WRITE) (
    IN PVOID Context
);

typedef BOOLEAN (*PACQUIRE_FOR_READ_AHEAD) (
    IN PVOID    Context,
    IN BOOLEAN  Wait
);

typedef VOID (*PRELEASE_FROM_READ_AHEAD) (
    IN PVOID Context
);

typedef struct _CACHE_MANAGER_CALLBACKS
{
	PACQUIRE_FOR_LAZY_WRITE		AcquireForLazyWrite;
	PRELEASE_FROM_LAZY_WRITE	ReleaseFromLazyWrite;
	PACQUIRE_FOR_READ_AHEAD		AcquireForReadAhead;
	PRELEASE_FROM_READ_AHEAD	ReleaseFromReadAhead;
} CACHE_MANAGER_CALLBACKS, *PCACHE_MANAGER_CALLBACKS;

/* this is already defined in iotypes.h */
/*
typedef struct _SECTION_OBJECT_POINTERS
{
	PVOID	DataSectionObject;
	PVOID	SharedCacheMap;
	PVOID	ImageSectionObject;
} SECTION_OBJECT_POINTERS, *PSECTION_OBJECT_POINTERS;
*/

typedef VOID (*PFLUSH_TO_LSN)(IN PVOID LogHandle, IN LARGE_INTEGER Lsn);

typedef struct _REACTOS_COMMON_FCB_HEADER
{
  PBCB Bcb;
} REACTOS_COMMON_FCB_HEADER;

#endif /* __INCLUDE_DDK_CCTYPES_H */
