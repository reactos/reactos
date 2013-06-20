$if (_NTIFS_)
/* Common Cache Types */

#define VACB_MAPPING_GRANULARITY        (0x40000)
#define VACB_OFFSET_SHIFT               (18)

typedef struct _PUBLIC_BCB {
  CSHORT NodeTypeCode;
  CSHORT NodeByteSize;
  ULONG MappedLength;
  LARGE_INTEGER MappedFileOffset;
} PUBLIC_BCB, *PPUBLIC_BCB;

typedef struct _CC_FILE_SIZES {
  LARGE_INTEGER AllocationSize;
  LARGE_INTEGER FileSize;
  LARGE_INTEGER ValidDataLength;
} CC_FILE_SIZES, *PCC_FILE_SIZES;

typedef BOOLEAN
(NTAPI *PACQUIRE_FOR_LAZY_WRITE) (
  _In_ PVOID Context,
  _In_ BOOLEAN Wait);

typedef VOID
(NTAPI *PRELEASE_FROM_LAZY_WRITE) (
  _In_ PVOID Context);

typedef BOOLEAN
(NTAPI *PACQUIRE_FOR_READ_AHEAD) (
  _In_ PVOID Context,
  _In_ BOOLEAN Wait);

typedef VOID
(NTAPI *PRELEASE_FROM_READ_AHEAD) (
  _In_ PVOID Context);

typedef struct _CACHE_MANAGER_CALLBACKS {
  PACQUIRE_FOR_LAZY_WRITE AcquireForLazyWrite;
  PRELEASE_FROM_LAZY_WRITE ReleaseFromLazyWrite;
  PACQUIRE_FOR_READ_AHEAD AcquireForReadAhead;
  PRELEASE_FROM_READ_AHEAD ReleaseFromReadAhead;
} CACHE_MANAGER_CALLBACKS, *PCACHE_MANAGER_CALLBACKS;

typedef struct _CACHE_UNINITIALIZE_EVENT {
  struct _CACHE_UNINITIALIZE_EVENT *Next;
  KEVENT Event;
} CACHE_UNINITIALIZE_EVENT, *PCACHE_UNINITIALIZE_EVENT;

typedef VOID
(NTAPI *PDIRTY_PAGE_ROUTINE) (
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ PLARGE_INTEGER OldestLsn,
  _In_ PLARGE_INTEGER NewestLsn,
  _In_ PVOID Context1,
  _In_ PVOID Context2);

typedef VOID
(NTAPI *PFLUSH_TO_LSN) (
  _In_ PVOID LogHandle,
  _In_ LARGE_INTEGER Lsn);

typedef VOID
(NTAPI *PCC_POST_DEFERRED_WRITE) (
  _In_ PVOID Context1,
  _In_ PVOID Context2);

#define UNINITIALIZE_CACHE_MAPS          (1)
#define DO_NOT_RETRY_PURGE               (2)
#define DO_NOT_PURGE_DIRTY_PAGES         (0x4)

#define CC_FLUSH_AND_PURGE_NO_PURGE     (0x1)
$endif (_NTIFS_)
