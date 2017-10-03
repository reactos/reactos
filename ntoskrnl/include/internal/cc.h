#pragma once

//
// Define this if you want debugging support
//
#define _CC_DEBUG_                                      0x00

//
// These define the Debug Masks Supported
//
#define CC_API_DEBUG                                    0x01

//
// Debug/Tracing support
//
#if _CC_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define CCTRACE(x, ...)                                     \
    {                                                       \
        DbgPrintEx("%s [%.16s] - ",                         \
                   __FUNCTION__,                            \
                   PsGetCurrentProcess()->ImageFileName);   \
        DbgPrintEx(__VA_ARGS__);                            \
    }
#else
#define CCTRACE(x, ...)                                     \
    if (x & CcRosTraceLevel)                                \
    {                                                       \
        DbgPrint("%s [%.16s] - ",                           \
                 __FUNCTION__,                              \
                 PsGetCurrentProcess()->ImageFileName);     \
        DbgPrint(__VA_ARGS__);                              \
    }
#endif
#else
#define CCTRACE(x, fmt, ...) DPRINT(fmt, ##__VA_ARGS__)
#endif

//
// Global Cc Data
//
extern ULONG CcRosTraceLevel;

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

typedef struct _ROS_SHARED_CACHE_MAP
{
    LIST_ENTRY CacheMapVacbListHead;
    ULONG TimeStamp;
    PFILE_OBJECT FileObject;
    LARGE_INTEGER SectionSize;
    LARGE_INTEGER FileSize;
    BOOLEAN PinAccess;
    PCACHE_MANAGER_CALLBACKS Callbacks;
    PVOID LazyWriteContext;
    KSPIN_LOCK CacheMapLock;
    ULONG OpenCount;
#if DBG
    BOOLEAN Trace; /* enable extra trace output for this cache map and it's VACBs */
#endif
} ROS_SHARED_CACHE_MAP, *PROS_SHARED_CACHE_MAP;

typedef struct _ROS_VACB
{
    /* Base address of the region where the view's data is mapped. */
    PVOID BaseAddress;
    /* Memory area representing the region where the view's data is mapped. */
    struct _MEMORY_AREA* MemoryArea;
    /* Are the contents of the view valid. */
    BOOLEAN Valid;
    /* Are the contents of the view newer than those on disk. */
    BOOLEAN Dirty;
    /* Page out in progress */
    BOOLEAN PageOut;
    ULONG MappedCount;
    /* Entry in the list of VACBs for this shared cache map. */
    LIST_ENTRY CacheMapVacbListEntry;
    /* Entry in the list of VACBs which are dirty. */
    LIST_ENTRY DirtyVacbListEntry;
    /* Entry in the list of VACBs. */
    LIST_ENTRY VacbLruListEntry;
    /* Offset in the file which this view maps. */
    LARGE_INTEGER FileOffset;
    /* Mutex */
    KMUTEX Mutex;
    /* Number of references. */
    ULONG ReferenceCount;
    /* How many times was it pinned? */
    _Guarded_by_(Mutex)
    LONG PinCount;
    /* Pointer to the shared cache map for the file which this view maps data for. */
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    /* Pointer to the next VACB in a chain. */
} ROS_VACB, *PROS_VACB;

typedef struct _INTERNAL_BCB
{
    /* Lock */
    ERESOURCE Lock;
    PUBLIC_BCB PFCB;
    PROS_VACB Vacb;
    BOOLEAN Dirty;
    BOOLEAN Pinned;
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
    IN PFILE_OBJECT FileObject,
    IN PMDL MemoryDescriptorList
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
CcRosFlushVacb(PROS_VACB Vacb);

NTSTATUS
NTAPI
CcRosGetVacb(
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset,
    PLONGLONG BaseOffset,
    PVOID *BaseAddress,
    PBOOLEAN UptoDate,
    PROS_VACB *Vacb
);

VOID
NTAPI
CcInitView(VOID);

NTSTATUS
NTAPI
CcReadVirtualAddress(PROS_VACB Vacb);

NTSTATUS
NTAPI
CcWriteVirtualAddress(PROS_VACB Vacb);

BOOLEAN
NTAPI
CcInitializeCacheManager(VOID);

NTSTATUS
NTAPI
CcRosUnmapVacb(
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset,
    BOOLEAN NowDirty
);

PROS_VACB
NTAPI
CcRosLookupVacb(
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset
);

VOID
NTAPI
CcInitCacheZeroPage(VOID);

NTSTATUS
NTAPI
CcRosMarkDirtyVacb(
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset
);

NTSTATUS
NTAPI
CcRosFlushDirtyPages(
    ULONG Target,
    PULONG Count,
    BOOLEAN Wait
);

VOID
NTAPI
CcRosDereferenceCache(PFILE_OBJECT FileObject);

VOID
NTAPI
CcRosReferenceCache(PFILE_OBJECT FileObject);

VOID
NTAPI
CcRosRemoveIfClosed(PSECTION_OBJECT_POINTERS SectionObjectPointer);

NTSTATUS
NTAPI
CcRosReleaseVacb(
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    PROS_VACB Vacb,
    BOOLEAN Valid,
    BOOLEAN Dirty,
    BOOLEAN Mapped
);

NTSTATUS
NTAPI
CcRosRequestVacb(
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset,
    PVOID* BaseAddress,
    PBOOLEAN UptoDate,
    PROS_VACB *Vacb
);

NTSTATUS
NTAPI
CcRosInitializeFileCache(
    PFILE_OBJECT FileObject,
    PCC_FILE_SIZES FileSizes,
    BOOLEAN PinAccess,
    PCACHE_MANAGER_CALLBACKS CallBacks,
    PVOID LazyWriterContext
);

NTSTATUS
NTAPI
CcRosReleaseFileCache(
    PFILE_OBJECT FileObject
);

NTSTATUS
NTAPI
CcTryToInitializeFileCache(PFILE_OBJECT FileObject);

FORCEINLINE
NTSTATUS
CcRosAcquireVacbLock(
    _Inout_ PROS_VACB Vacb,
    _In_ PLARGE_INTEGER Timeout)
{
    NTSTATUS Status;
    Status = KeWaitForSingleObject(&Vacb->Mutex,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   Timeout);
    return Status;
}

FORCEINLINE
VOID
CcRosReleaseVacbLock(
    _Inout_ PROS_VACB Vacb)
{
    KeReleaseMutex(&Vacb->Mutex, FALSE);
}

FORCEINLINE
BOOLEAN
DoRangesIntersect(
    _In_ LONGLONG Offset1,
    _In_ LONGLONG Length1,
    _In_ LONGLONG Offset2,
    _In_ LONGLONG Length2)
{
    if (Offset1 + Length1 <= Offset2)
        return FALSE;
    if (Offset2 + Length2 <= Offset1)
        return FALSE;
    return TRUE;
}

FORCEINLINE
BOOLEAN
IsPointInRange(
    _In_ LONGLONG Offset1,
    _In_ LONGLONG Length1,
    _In_ LONGLONG Point)
{
    return DoRangesIntersect(Offset1, Length1, Point, 1);
}
