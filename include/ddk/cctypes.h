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


typedef VOID STDCALL_FUNC
(*PCC_POST_DEFERRED_WRITE)(IN PVOID Context1,
			   IN PVOID Context2);

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

typedef BOOLEAN STDCALL_FUNC
(*PACQUIRE_FOR_LAZY_WRITE)(IN PVOID Context,
			   IN BOOLEAN Wait);

typedef VOID STDCALL_FUNC
(*PRELEASE_FROM_LAZY_WRITE)(IN PVOID Context);

typedef BOOLEAN STDCALL_FUNC
(*PACQUIRE_FOR_READ_AHEAD)(IN PVOID Context,
			   IN BOOLEAN Wait);

typedef VOID STDCALL_FUNC
(*PRELEASE_FROM_READ_AHEAD)(IN PVOID Context);

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

typedef VOID STDCALL_FUNC
(*PFLUSH_TO_LSN)(IN PVOID LogHandle,
		 IN LARGE_INTEGER Lsn);

typedef struct _FSRTL_COMMON_FCB_HEADER {
    CSHORT          NodeTypeCode;
    CSHORT          NodeByteSize;
    UCHAR           Flags;
    UCHAR           IsFastIoPossible;
    UCHAR           Flags2;
    UCHAR           Reserved;
    PERESOURCE      Resource;
    PERESOURCE      PagingIoResource;
    LARGE_INTEGER   AllocationSize;
    LARGE_INTEGER   FileSize;
    LARGE_INTEGER   ValidDataLength;
} FSRTL_COMMON_FCB_HEADER, *PFSRTL_COMMON_FCB_HEADER;

#endif /* __INCLUDE_DDK_CCTYPES_H */
