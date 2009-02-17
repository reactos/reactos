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

    /* So we know the initial request that was made */
    PFILE_OBJECT FileObject;
	PMEMORY_AREA MemoryArea;
    LARGE_INTEGER FileOffset;
    ULONG Length;
    PVOID BaseAddress;
    BOOLEAN Dirty;
	BOOLEAN Zero;
    BOOLEAN Pinned;
    PVOID OwnerPointer;
    
    /* Reference counts */
    ULONG RefCount;
    
    LIST_ENTRY ThisFileList;
    
    KEVENT ExclusiveWait;
    ULONG ExclusiveWaiter;
    BOOLEAN Exclusive;
} NOCC_BCB, *PNOCC_BCB;

typedef struct _NOCC_CACHE_MAP
{
    LIST_ENTRY AssociatedBcb;
    PFILE_OBJECT FileObject;
    ULONG NumberOfMaps;
    ULONG RefCount;
    CC_FILE_SIZES FileSizes;
} NOCC_CACHE_MAP, *PNOCC_CACHE_MAP;

/* io.c *****************************************************************/

PDEVICE_OBJECT
NTAPI
MmGetDeviceObjectForFile
(IN PFILE_OBJECT FileObject);

PFILE_OBJECT
NTAPI
MmGetFileObjectForSection
(IN PROS_SECTION_OBJECT Section);

NTSTATUS
NTAPI
MmGetFileNameForSection
(IN PROS_SECTION_OBJECT Section,
 OUT POBJECT_NAME_INFORMATION *ModuleName);

NTSTATUS
NTAPI
MmGetFileNameForAddress
(IN PVOID Address,
 OUT PUNICODE_STRING ModuleName);

NTSTATUS
NTAPI
MiSimpleRead
(PFILE_OBJECT FileObject,
 PLARGE_INTEGER FileOffset,
 PVOID Buffer,
 ULONG Length,
 PIO_STATUS_BLOCK ReadStatus);

NTSTATUS
NTAPI
MiSimpleWrite
(PFILE_OBJECT FileObject,
 PLARGE_INTEGER FileOffset,
 PVOID Buffer,
 ULONG Length,
 PIO_STATUS_BLOCK WriteStatus);

NTSTATUS
NTAPI
MiScheduleForWrite
(PFILE_OBJECT FileObject,
 PLARGE_INTEGER FileOffset,
 PFN_TYPE Page,
 ULONG Length);

NTSTATUS
NTAPI
MmWriteThreadInit();

/* other */

NTSTATUS
NTAPI
CcReplaceCachePage(
	PMEMORY_AREA MemoryArea, PVOID Address
);

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

VOID
NTAPI
CcInitView(VOID);

BOOLEAN
NTAPI
CcInitializeCacheManager(VOID);

VOID
NTAPI
CcInitCacheZeroPage(VOID);

/* Called by section.c */
BOOLEAN
NTAPI
CcFlushImageSection(PSECTION_OBJECT_POINTERS SectionObjectPointer, MMFLUSH_TYPE FlushType);

BOOLEAN
NTAPI
CcGetFileSizes(PFILE_OBJECT FileObject, PCC_FILE_SIZES FileSizes);

/*
 * Macro for generic cache manage bugchecking. Note that this macro assumes
 * that the file name including extension is always longer than 4 characters.
 */
#define KEBUGCHECKCC \
    KEBUGCHECKEX(CACHE_MANAGER, \
    (*(ULONG*)(__FILE__ + sizeof(__FILE__) - 4) << 16) | \
    (__LINE__ & 0xFFFF), 0, 0, 0)

/* Private data */

#define CACHE_SINGLE_FILE_MAX (16)
#define CACHE_OVERALL_SIZE (32 * 1024 * 1024)
#define CACHE_STRIPE VACB_MAPPING_GRANULARITY
#define CACHE_SHIFT 18
#define CACHE_NUM_SECTIONS (CACHE_OVERALL_SIZE / CACHE_STRIPE)
#define CACHE_ROUND_UP(x) (((x) + (CACHE_STRIPE-1)) & ~(CACHE_STRIPE-1))
#define CACHE_ROUND_DOWN(x) ((x) & ~(CACHE_STRIPE-1))
#define CACHE_NEED_SECTIONS(OFFSET,LENGTH) \
	((CACHE_ROUND_UP((OFFSET)->QuadPart + (LENGTH)) -		\
	  CACHE_ROUND_DOWN((OFFSET)->QuadPart)) >> CACHE_SHIFT)
#define INVALID_CACHE ((ULONG)~0)

extern NOCC_BCB CcCacheSections[CACHE_NUM_SECTIONS];
extern PRTL_BITMAP CcCacheBitmap;
extern FAST_MUTEX CcMutex;
extern KEVENT CcDeleteEvent;
extern ULONG CcCacheClockHand;
extern LIST_ENTRY CcPendingUnmap;
extern KEVENT CcpLazyWriteEvent;

extern VOID CcpLock();
extern VOID CcpUnlock();
extern VOID CcpDereferenceCache(ULONG Sector);
BOOLEAN
NTAPI
CcpMapData
(IN PNOCC_CACHE_MAP Map,
 IN PLARGE_INTEGER FileOffset,
 IN ULONG Length,
 IN ULONG Flags,
 IN BOOLEAN Zero,
 OUT PVOID *BcbResult,
 OUT PVOID *Buffer);

#endif
