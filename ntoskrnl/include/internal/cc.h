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
extern LIST_ENTRY DirtyVacbListHead;
extern ULONG CcDirtyPageThreshold;
extern ULONG CcTotalDirtyPages;
extern LIST_ENTRY CcDeferredWrites;
extern KSPIN_LOCK CcDeferredWriteSpinLock;
extern ULONG CcNumberWorkerThreads;
extern LIST_ENTRY CcIdleWorkerThreadList;
extern LIST_ENTRY CcExpressWorkQueue;
extern LIST_ENTRY CcRegularWorkQueue;
extern LIST_ENTRY CcPostTickWorkQueue;
extern NPAGED_LOOKASIDE_LIST CcTwilightLookasideList;
extern LARGE_INTEGER CcIdleDelay;

//
// Counters
//
extern ULONG CcLazyWritePages;
extern ULONG CcLazyWriteIos;
extern ULONG CcMapDataWait;
extern ULONG CcMapDataNoWait;
extern ULONG CcPinReadWait;
extern ULONG CcPinReadNoWait;
extern ULONG CcPinMappedDataCount;
extern ULONG CcDataPages;
extern ULONG CcDataFlushes;

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
    CSHORT NodeTypeCode;
    CSHORT NodeByteSize;
    ULONG OpenCount;
    LARGE_INTEGER FileSize;
    LIST_ENTRY BcbList;
    LARGE_INTEGER SectionSize;
    LARGE_INTEGER ValidDataLength;
    PFILE_OBJECT FileObject;
    ULONG DirtyPages;
    LIST_ENTRY SharedCacheMapLinks;
    ULONG Flags;
    PVOID Section;
    PKEVENT CreateEvent;
    PCACHE_MANAGER_CALLBACKS Callbacks;
    PVOID LazyWriteContext;
    LIST_ENTRY PrivateList;
    ULONG DirtyPageThreshold;
    KSPIN_LOCK BcbSpinLock;
    PRIVATE_CACHE_MAP PrivateCacheMap;

    /* ROS specific */
    LIST_ENTRY CacheMapVacbListHead;
    BOOLEAN PinAccess;
    KSPIN_LOCK CacheMapLock;
#if DBG
    BOOLEAN Trace; /* enable extra trace output for this cache map and it's VACBs */
#endif
} ROS_SHARED_CACHE_MAP, *PROS_SHARED_CACHE_MAP;

#define READAHEAD_DISABLED 0x1
#define WRITEBEHIND_DISABLED 0x2
#define SHARED_CACHE_MAP_IN_CREATION 0x4
#define SHARED_CACHE_MAP_IN_LAZYWRITE 0x8

typedef struct _ROS_VACB
{
    /* Base address of the region where the view's data is mapped. */
    PVOID BaseAddress;
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
    /* Number of references. */
    volatile ULONG ReferenceCount;
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
    ULONG PinCount;
    CSHORT RefCount; /* (At offset 0x34 on WinNT4) */
    LIST_ENTRY BcbEntry;
} INTERNAL_BCB, *PINTERNAL_BCB;

typedef struct _LAZY_WRITER
{
    LIST_ENTRY WorkQueue;
    KDPC ScanDpc;
    KTIMER ScanTimer;
    BOOLEAN ScanActive;
    BOOLEAN OtherWork;
    BOOLEAN PendingTeardown;
} LAZY_WRITER, *PLAZY_WRITER;

typedef struct _WORK_QUEUE_ENTRY
{
    LIST_ENTRY WorkQueueLinks;
    union
    {
        struct
        {
            FILE_OBJECT *FileObject;
        } Read;
        struct
        {
            SHARED_CACHE_MAP *SharedCacheMap;
        } Write;
        struct
        {
            KEVENT *Event;
        } Event;
        struct
        {
            unsigned long Reason;
        } Notification;
    } Parameters;
    unsigned char Function;
} WORK_QUEUE_ENTRY, *PWORK_QUEUE_ENTRY;

typedef enum _WORK_QUEUE_FUNCTIONS
{
    ReadAhead = 1,
    WriteBehind = 2,
    LazyScan = 3,
    SetDone = 4,
} WORK_QUEUE_FUNCTIONS, *PWORK_QUEUE_FUNCTIONS;

extern LAZY_WRITER LazyWriter;

#define NODE_TYPE_DEFERRED_WRITE 0x02FC
#define NODE_TYPE_PRIVATE_MAP    0x02FE
#define NODE_TYPE_SHARED_MAP     0x02FF

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
CcRosFlushVacb(PROS_VACB Vacb, PIO_STATUS_BLOCK Iosb);

NTSTATUS
CcRosGetVacb(
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset,
    PROS_VACB *Vacb
);

BOOLEAN
CcRosEnsureVacbResident(
    _In_ PROS_VACB Vacb,
    _In_ BOOLEAN Wait,
    _In_ BOOLEAN NoRead,
    _In_ ULONG Offset,
    _In_ ULONG Length
);

VOID
NTAPI
CcInitView(VOID);

VOID
NTAPI
CcShutdownLazyWriter(VOID);

BOOLEAN
CcInitializeCacheManager(VOID);

PROS_VACB
CcRosLookupVacb(
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset
);

VOID
NTAPI
CcInitCacheZeroPage(VOID);

VOID
CcRosMarkDirtyVacb(
    PROS_VACB Vacb);

VOID
CcRosUnmarkDirtyVacb(
    PROS_VACB Vacb,
    BOOLEAN LockViews);

NTSTATUS
CcRosFlushDirtyPages(
    ULONG Target,
    PULONG Count,
    BOOLEAN Wait,
    BOOLEAN CalledFromLazy
);

VOID
CcRosDereferenceCache(PFILE_OBJECT FileObject);

VOID
CcRosReferenceCache(PFILE_OBJECT FileObject);

NTSTATUS
CcRosReleaseVacb(
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    PROS_VACB Vacb,
    BOOLEAN Dirty,
    BOOLEAN Mapped
);

NTSTATUS
CcRosRequestVacb(
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset,
    PROS_VACB *Vacb
);

NTSTATUS
CcRosInitializeFileCache(
    PFILE_OBJECT FileObject,
    PCC_FILE_SIZES FileSizes,
    BOOLEAN PinAccess,
    PCACHE_MANAGER_CALLBACKS CallBacks,
    PVOID LazyWriterContext
);

NTSTATUS
CcRosReleaseFileCache(
    PFILE_OBJECT FileObject
);

VOID
NTAPI
CcShutdownSystem(VOID);

VOID
NTAPI
CcWorkerThread(PVOID Parameter);

VOID
NTAPI
CcScanDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2);

VOID
CcScheduleLazyWriteScan(BOOLEAN NoDelay);

VOID
CcPostDeferredWrites(VOID);

VOID
CcPostWorkQueue(
    IN PWORK_QUEUE_ENTRY WorkItem,
    IN PLIST_ENTRY WorkQueue);

VOID
CcPerformReadAhead(
    IN PFILE_OBJECT FileObject);

NTSTATUS
CcRosInternalFreeVacb(
    IN PROS_VACB Vacb);

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

#define CcBugCheck(A, B, C) KeBugCheckEx(CACHE_MANAGER, BugCheckFileId | ((ULONG)(__LINE__)), A, B, C)

#if DBG
#define CcRosVacbIncRefCount(vacb) CcRosVacbIncRefCount_(vacb,__FILE__,__LINE__)
#define CcRosVacbDecRefCount(vacb) CcRosVacbDecRefCount_(vacb,__FILE__,__LINE__)
#define CcRosVacbGetRefCount(vacb) CcRosVacbGetRefCount_(vacb,__FILE__,__LINE__)

ULONG
CcRosVacbIncRefCount_(
    PROS_VACB vacb,
    PCSTR file,
    INT line);

ULONG
CcRosVacbDecRefCount_(
    PROS_VACB vacb,
    PCSTR file,
    INT line);

ULONG
CcRosVacbGetRefCount_(
    PROS_VACB vacb,
    PCSTR file,
    INT line);

#else
#define CcRosVacbIncRefCount(vacb) InterlockedIncrement((PLONG)&(vacb)->ReferenceCount)
FORCEINLINE
ULONG
CcRosVacbDecRefCount(
    PROS_VACB vacb)
{
    ULONG Refs;

    Refs = InterlockedDecrement((PLONG)&vacb->ReferenceCount);
    if (Refs == 0)
    {
        CcRosInternalFreeVacb(vacb);
    }
    return Refs;
}
#define CcRosVacbGetRefCount(vacb) InterlockedCompareExchange((PLONG)&(vacb)->ReferenceCount, 0, 0)
#endif
