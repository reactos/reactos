#ifndef __INCLUDE_INTERNAL_CC_H
#define __INCLUDE_INTERNAL_CC_H

typedef struct _PF_SCENARIO_ID
{
    WCHAR ScenName[30];
    ULONG HashId;
} PF_SCENARIO_ID, *PPF_SCENARIO_ID;

typedef struct _PF_LOG_ENTRY
{
    ULONG FileOffset:30;
    ULONG Type:2;
    union
    {
        ULONG FileKey;
        ULONG FileSequenceNumber;
    };
} PF_LOG_ENTRY, *PPF_LOG_ENTRY;

typedef struct _PFSN_LOG_ENTRIES
{
    LIST_ENTRY TraceBuffersLink;
    LONG NumEntries;
    LONG MaxEntries;
    PF_LOG_ENTRY Entries[ANYSIZE_ARRAY];
} PFSN_LOG_ENTRIES, *PPFSN_LOG_ENTRIES;

typedef struct _PF_SECTION_INFO
{
    ULONG FileKey;
    ULONG FileSequenceNumber;
    ULONG FileIdLow;
    ULONG FileIdHigh;
} PF_SECTION_INFO, *PPF_SECTION_INFO;

typedef struct _PF_TRACE_HEADER
{
    ULONG Version;
    ULONG MagicNumber;
    ULONG Size;
    PF_SCENARIO_ID ScenarioId;
    ULONG ScenarioType; // PF_SCENARIO_TYPE
    ULONG EventEntryIdxs[8];
    ULONG NumEventEntryIdxs;
    ULONG TraceBufferOffset;
    ULONG NumEntries;
    ULONG SectionInfoOffset;
    ULONG NumSections;
    ULONG FaultsPerPeriod[10];
    LARGE_INTEGER LaunchTime;
    ULONGLONG Reserved[5];
} PF_TRACE_HEADER, *PPF_TRACE_HEADER;

typedef struct _PFSN_TRACE_DUMP
{
    LIST_ENTRY CompletedTracesLink;
    PF_TRACE_HEADER Trace;
} PFSN_TRACE_DUMP, *PPFSN_TRACE_DUMP;

typedef struct _PFSN_TRACE_HEADER
{
    ULONG Magic;
    LIST_ENTRY ActiveTracesLink;
    PF_SCENARIO_ID ScenarioId;
    ULONG ScenarioType; // PF_SCENARIO_TYPE
    ULONG EventEntryIdxs[8];
    ULONG NumEventEntryIdxs;
    PPFSN_LOG_ENTRIES CurrentTraceBuffer;
    LIST_ENTRY TraceBuffersList;
    ULONG NumTraceBuffers;
    KSPIN_LOCK TraceBufferSpinLock;
    KTIMER TraceTimer;
    LARGE_INTEGER TraceTimerPeriod;
    KDPC TraceTimerDpc;
    KSPIN_LOCK TraceTimerSpinLock;
    ULONG FaultsPerPeriod[10];
    LONG LastNumFaults;
    LONG CurPeriod;
    LONG NumFaults;
    LONG MaxFaults;
    PEPROCESS Process;
    EX_RUNDOWN_REF RefCount;
    WORK_QUEUE_ITEM EndTraceWorkItem;
    LONG EndTraceCalled;
    PPFSN_TRACE_DUMP TraceDump;
    NTSTATUS TraceDumpStatus;
    LARGE_INTEGER LaunchTime;
    PPF_SECTION_INFO SectionInfo;
    ULONG SectionInfoCount;
} PFSN_TRACE_HEADER, *PPFSN_TRACE_HEADER;

typedef struct _PFSN_PREFETCHER_GLOBALS
{
    LIST_ENTRY ActiveTraces;
    KSPIN_LOCK ActiveTracesLock;
    PPFSN_TRACE_HEADER SystemWideTrace;
    LIST_ENTRY CompletedTraces;
    FAST_MUTEX CompletedTracesLock;
    LONG NumCompletedTraces;
    PKEVENT CompletedTracesEvent;
    LONG ActivePrefetches;
} PFSN_PREFETCHER_GLOBALS, *PPFSN_PREFETCHER_GLOBALS;

typedef struct _BCB
{
    LIST_ENTRY BcbSegmentListHead;
    LIST_ENTRY BcbRemoveListEntry;
    BOOLEAN RemoveOnClose;
    ULONG TimeStamp;
    PFILE_OBJECT FileObject;
    ULONG CacheSegmentSize;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER FileSize;
    PCACHE_MANAGER_CALLBACKS Callbacks;
    PVOID LazyWriteContext;
    KSPIN_LOCK BcbLock;
    ULONG RefCount;
#if defined(DBG) || defined(KDBG)
	BOOLEAN Trace; /* enable extra trace output for this BCB and it's cache segments */
#endif
} BCB, *PBCB;

typedef struct _CACHE_SEGMENT
{
    /* Base address of the region where the cache segment data is mapped. */
    PVOID BaseAddress;
    /*
     * Memory area representing the region where the cache segment data is
     * mapped.
     */
    struct _MEMORY_AREA* MemoryArea;
    /* Are the contents of the cache segment data valid. */
    BOOLEAN Valid;
    /* Are the contents of the cache segment data newer than those on disk. */
    BOOLEAN Dirty;
    /* Page out in progress */
    BOOLEAN PageOut;
    ULONG MappedCount;
    /* Entry in the list of segments for this BCB. */
    LIST_ENTRY BcbSegmentListEntry;
    /* Entry in the list of segments which are dirty. */
    LIST_ENTRY DirtySegmentListEntry;
    /* Entry in the list of segments. */
    LIST_ENTRY CacheSegmentListEntry;
    LIST_ENTRY CacheSegmentLRUListEntry;
    /* Offset in the file which this cache segment maps. */
    ULONG FileOffset;
    /* Lock. */
    EX_PUSH_LOCK Lock;
    /* Number of references. */
    ULONG ReferenceCount;
    /* Pointer to the BCB for the file which this cache segment maps data for. */
    PBCB Bcb;
    /* Pointer to the next cache segment in a chain. */
    struct _CACHE_SEGMENT* NextInChain;
} CACHE_SEGMENT, *PCACHE_SEGMENT;

typedef struct _INTERNAL_BCB
{
    PUBLIC_BCB PFCB;
    PCACHE_SEGMENT CacheSegment;
    BOOLEAN Dirty;
    CSHORT RefCount; /* (At offset 0x34 on WinNT4) */
} INTERNAL_BCB, *PINTERNAL_BCB;

VOID
NTAPI
CcPfInitializePrefetcher(
    VOID
);

VOID
NTAPI
CcMdlReadComplete2(
    IN PMDL MemoryDescriptorList,
    IN PFILE_OBJECT FileObject
);

VOID
NTAPI
CcMdlWriteComplete2(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain
);

NTSTATUS
NTAPI
CcRosFlushCacheSegment(PCACHE_SEGMENT CacheSegment);

NTSTATUS
NTAPI
CcRosGetCacheSegment(
    PBCB Bcb,
    ULONG FileOffset,
    PULONG BaseOffset,
    PVOID *BaseAddress,
    PBOOLEAN UptoDate,
    PCACHE_SEGMENT *CacheSeg
);

VOID
NTAPI
CcInitView(VOID);

NTSTATUS
NTAPI
CcRosFreeCacheSegment(
    PBCB,
    PCACHE_SEGMENT
);

NTSTATUS
NTAPI
ReadCacheSegment(PCACHE_SEGMENT CacheSeg);

NTSTATUS
NTAPI
WriteCacheSegment(PCACHE_SEGMENT CacheSeg);

BOOLEAN
NTAPI
CcInitializeCacheManager(VOID);

NTSTATUS
NTAPI
CcRosUnmapCacheSegment(
    PBCB Bcb,
    ULONG FileOffset,
    BOOLEAN NowDirty
);

PCACHE_SEGMENT
NTAPI
CcRosLookupCacheSegment(
    PBCB Bcb,
    ULONG FileOffset
);

NTSTATUS
NTAPI
CcRosGetCacheSegmentChain(
    PBCB Bcb,
    ULONG FileOffset,
    ULONG Length,
    PCACHE_SEGMENT* CacheSeg
);

VOID
NTAPI
CcInitCacheZeroPage(VOID);

NTSTATUS
NTAPI
CcRosMarkDirtyCacheSegment(
    PBCB Bcb,
    ULONG FileOffset
);

NTSTATUS
NTAPI
CcRosFlushDirtyPages(
    ULONG Target,
    PULONG Count
);

VOID
NTAPI
CcRosDereferenceCache(PFILE_OBJECT FileObject);

VOID
NTAPI
CcRosReferenceCache(PFILE_OBJECT FileObject);

VOID
NTAPI
CcRosSetRemoveOnClose(PSECTION_OBJECT_POINTERS SectionObjectPointer);

NTSTATUS
NTAPI
CcRosReleaseCacheSegment(
    BCB* Bcb,
    CACHE_SEGMENT *CacheSeg,
    BOOLEAN Valid,
    BOOLEAN Dirty,
    BOOLEAN Mapped
);

NTSTATUS
NTAPI
CcRosRequestCacheSegment(
    BCB *Bcb,
    ULONG FileOffset,
    PVOID* BaseAddress,
    PBOOLEAN UptoDate,
    CACHE_SEGMENT **CacheSeg
);

NTSTATUS
NTAPI
CcTryToInitializeFileCache(PFILE_OBJECT FileObject);

#endif
