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

typedef struct _NOCC_BCB
{
    /* Public part */
    PUBLIC_BCB Bcb;

    /* So we know where the buffer is */
    PVOID CacheBuffer;

    /* So we know the initial request that was made */
    PFILE_OBJECT FileObject;

    /* Actual data we ended up reading */
    ULONG RealLength;
} NOCC_BCB, *PNOCC_BCB;

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

BOOLEAN
NTAPI
CcInitializeCacheManager(
    VOID
);

#endif
