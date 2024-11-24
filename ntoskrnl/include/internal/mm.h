#pragma once

#include <internal/arch/mm.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES *********************************************************************/

struct _EPROCESS;

extern PMMSUPPORT MmKernelAddressSpace;
extern PFN_COUNT MiFreeSwapPages;
extern PFN_COUNT MiUsedSwapPages;
extern PFN_COUNT MmNumberOfPhysicalPages;
extern UCHAR MmDisablePagingExecutive;
extern PFN_NUMBER MmLowestPhysicalPage;
extern PFN_NUMBER MmHighestPhysicalPage;
extern PFN_NUMBER MmAvailablePages;
extern PFN_NUMBER MmResidentAvailablePages;
extern ULONG MmThrottleTop;
extern ULONG MmThrottleBottom;

extern LIST_ENTRY MmLoadedUserImageList;

extern KMUTANT MmSystemLoadLock;

extern ULONG MmNumberOfPagingFiles;

extern SIZE_T MmTotalNonPagedPoolQuota;
extern SIZE_T MmTotalPagedPoolQuota;

extern PVOID MmUnloadedDrivers;
extern PVOID MmLastUnloadedDrivers;
extern PVOID MmTriageActionTaken;
extern PVOID KernelVerifier;
extern MM_DRIVER_VERIFIER_DATA MmVerifierData;

extern SIZE_T MmTotalCommitLimit;
extern SIZE_T MmTotalCommittedPages;
extern SIZE_T MmSharedCommit;
extern SIZE_T MmDriverCommit;
extern SIZE_T MmProcessCommit;
extern SIZE_T MmPagedPoolCommit;
extern SIZE_T MmPeakCommitment;
extern SIZE_T MmtotalCommitLimitMaximum;

extern PVOID MiDebugMapping; // internal
extern PMMPTE MmDebugPte; // internal

extern KSPIN_LOCK MmPfnLock;

struct _KTRAP_FRAME;
struct _EPROCESS;
struct _MM_RMAP_ENTRY;
typedef ULONG_PTR SWAPENTRY;

//
// Pool Quota values
//
#define MI_QUOTA_NON_PAGED_NEEDED_PAGES             64
#define MI_NON_PAGED_QUOTA_MIN_RESIDENT_PAGES       200
#define MI_CHARGE_PAGED_POOL_QUOTA                  0x80000
#define MI_CHARGE_NON_PAGED_POOL_QUOTA              0x10000

//
// Special IRQL value (found in assertions)
//
#define MM_NOIRQL ((KIRQL)0xFFFFFFFF)

//
// MmDbgCopyMemory Flags
//
#define MMDBG_COPY_WRITE            0x00000001
#define MMDBG_COPY_PHYSICAL         0x00000002
#define MMDBG_COPY_UNSAFE           0x00000004
#define MMDBG_COPY_CACHED           0x00000008
#define MMDBG_COPY_UNCACHED         0x00000010
#define MMDBG_COPY_WRITE_COMBINED   0x00000020

//
// Maximum chunk size per copy
//
#define MMDBG_COPY_MAX_SIZE         0x8

#if defined(_X86_) // intenal for marea.c
#define MI_STATIC_MEMORY_AREAS              (14)
#else
#define MI_STATIC_MEMORY_AREAS              (13)
#endif

#define MEMORY_AREA_SECTION_VIEW            (1)
#ifdef NEWCC
#define MEMORY_AREA_CACHE                   (2)
#endif
#define MEMORY_AREA_OWNED_BY_ARM3           (15)
#define MEMORY_AREA_STATIC                  (0x80000000)

/* Although Microsoft says this isn't hardcoded anymore,
   they won't be able to change it. Stuff depends on it */
#define MM_VIRTMEM_GRANULARITY              (64 * 1024)

#define STATUS_MM_RESTART_OPERATION         ((NTSTATUS)0xD0000001)

/*
 * Additional flags for protection attributes
 */
#define PAGE_WRITETHROUGH                   (1024)
#define PAGE_SYSTEM                         (2048)

#define SEC_PHYSICALMEMORY                  (0x80000000)

#define MC_USER                             (0)
#define MC_SYSTEM                           (1)
#define MC_MAXIMUM                          (2)

#define PAGED_POOL_MASK                     1
#define MUST_SUCCEED_POOL_MASK              2
#define CACHE_ALIGNED_POOL_MASK             4
#define QUOTA_POOL_MASK                     8
#define SESSION_POOL_MASK                   32
#define VERIFIER_POOL_MASK                  64

#define MAX_PAGING_FILES                    (16)

// FIXME: use ALIGN_UP_BY
#define MM_ROUND_UP(x,s)                    \
    ((PVOID)(((ULONG_PTR)(x)+(s)-1) & ~((ULONG_PTR)(s)-1)))

#define MM_ROUND_DOWN(x,s)                  \
    ((PVOID)(((ULONG_PTR)(x)) & ~((ULONG_PTR)(s)-1)))

#define PAGE_FLAGS_VALID_FOR_SECTION \
    (PAGE_READONLY | \
     PAGE_READWRITE | \
     PAGE_WRITECOPY | \
     PAGE_EXECUTE | \
     PAGE_EXECUTE_READ | \
     PAGE_EXECUTE_READWRITE | \
     PAGE_EXECUTE_WRITECOPY | \
     PAGE_NOACCESS | \
     PAGE_NOCACHE)

#define PAGE_IS_READABLE                    \
    (PAGE_READONLY | \
    PAGE_READWRITE | \
    PAGE_WRITECOPY | \
    PAGE_EXECUTE_READ | \
    PAGE_EXECUTE_READWRITE | \
    PAGE_EXECUTE_WRITECOPY)

#define PAGE_IS_WRITABLE                    \
    (PAGE_READWRITE | \
    PAGE_WRITECOPY | \
    PAGE_EXECUTE_READWRITE | \
    PAGE_EXECUTE_WRITECOPY)

#define PAGE_IS_EXECUTABLE                  \
    (PAGE_EXECUTE | \
    PAGE_EXECUTE_READ | \
    PAGE_EXECUTE_READWRITE | \
    PAGE_EXECUTE_WRITECOPY)

#define PAGE_IS_WRITECOPY                   \
    (PAGE_WRITECOPY | \
    PAGE_EXECUTE_WRITECOPY)

//
// Wait entry for marking pages that are being serviced
//
#ifdef _M_IX86
#define MM_WAIT_ENTRY            0x7ffffc00
#elif defined(_M_AMD64)
#define MM_WAIT_ENTRY            0x7FFFFFFFFFFFFC00ULL
#else
#error Unsupported architecture!
#endif

#ifdef _M_AMD64
#define InterlockedCompareExchangePte(PointerPte, Exchange, Comperand) \
    InterlockedCompareExchange64((PLONG64)(PointerPte), Exchange, Comperand)

#define InterlockedExchangePte(PointerPte, Value) \
    InterlockedExchange64((PLONG64)(PointerPte), Value)
#else
#define InterlockedCompareExchangePte(PointerPte, Exchange, Comperand) \
    InterlockedCompareExchange((PLONG)(PointerPte), Exchange, Comperand)

#define InterlockedExchangePte(PointerPte, Value) \
    InterlockedExchange((PLONG)(PointerPte), Value)
#endif

typedef struct _MM_SECTION_SEGMENT
{
    LONG64 RefCount;
    PFILE_OBJECT FileObject;

    FAST_MUTEX Lock;		/* lock which protects the page directory */
    LARGE_INTEGER RawLength;		/* length of the segment which is part of the mapped file */
    LARGE_INTEGER Length;			/* absolute length of the segment */
    PLONG64 ReferenceCount;
	ULONG SectionCount;
    ULONG Protection;
    PULONG Flags;
    BOOLEAN WriteCopy;
	BOOLEAN Locked;

	struct
	{
		ULONGLONG FileOffset;		/* start offset into the file for image sections */
		ULONG_PTR VirtualAddress;	/* start offset into the address range for image sections */
		ULONG Characteristics;
	} Image;

	ULONG SegFlags;

    ULONGLONG LastPage;

	RTL_GENERIC_TABLE PageTable;
} MM_SECTION_SEGMENT, *PMM_SECTION_SEGMENT;

typedef struct _MM_IMAGE_SECTION_OBJECT
{
    LONG64 RefCount;
    PFILE_OBJECT FileObject;
    ULONG SectionCount;
    LONG MapCount;
    ULONG SegFlags;

    SECTION_IMAGE_INFORMATION ImageInformation;
    PVOID BasedAddress;
    ULONG NrSegments;
    PMM_SECTION_SEGMENT Segments;
} MM_IMAGE_SECTION_OBJECT, *PMM_IMAGE_SECTION_OBJECT;

#define MM_PHYSICALMEMORY_SEGMENT           (0x1)
#define MM_DATAFILE_SEGMENT                 (0x2)
#define MM_SEGMENT_INDELETE                 (0x4)
#define MM_SEGMENT_INCREATE                 (0x8)
#define MM_IMAGE_SECTION_FLUSH_DELETE       (0x10)


#define MA_GetStartingAddress(_MemoryArea) ((_MemoryArea)->VadNode.StartingVpn << PAGE_SHIFT)
#define MA_GetEndingAddress(_MemoryArea) (((_MemoryArea)->VadNode.EndingVpn + 1) << PAGE_SHIFT)

typedef struct _MEMORY_AREA
{
    MMVAD VadNode;

    ULONG Type;
    ULONG Flags;
    BOOLEAN DeleteInProgress;
    ULONG Magic;
    PVOID Vad;

    struct
    {
        LONGLONG ViewOffset;
        PMM_SECTION_SEGMENT Segment;
        LIST_ENTRY RegionListHead;
    } SectionData;
} MEMORY_AREA, *PMEMORY_AREA;

typedef struct _MM_RMAP_ENTRY
{
   struct _MM_RMAP_ENTRY* Next;
   PEPROCESS Process;
   PVOID Address;
#if DBG
   PVOID Caller;
#endif
}
MM_RMAP_ENTRY, *PMM_RMAP_ENTRY;

#if MI_TRACE_PFNS
extern ULONG MI_PFN_CURRENT_USAGE;
extern CHAR MI_PFN_CURRENT_PROCESS_NAME[16];
#define MI_SET_USAGE(x)     MI_PFN_CURRENT_USAGE = x
#define MI_SET_PROCESS2(x)  memcpy(MI_PFN_CURRENT_PROCESS_NAME, x, min(sizeof(x), sizeof(MI_PFN_CURRENT_PROCESS_NAME)))
FORCEINLINE
void
MI_SET_PROCESS(PEPROCESS Process)
{
    if (!Process)
        MI_SET_PROCESS2("Kernel");
    else if (Process == (PEPROCESS)1)
        MI_SET_PROCESS2("Hydra");
    else
        MI_SET_PROCESS2(Process->ImageFileName);
}

FORCEINLINE
void
MI_SET_PROCESS_USTR(PUNICODE_STRING ustr)
{
    PWSTR pos, strEnd;
    ULONG i;

    if (!ustr->Buffer || ustr->Length == 0)
    {
        MI_PFN_CURRENT_PROCESS_NAME[0] = 0;
        return;
    }

    pos = strEnd = &ustr->Buffer[ustr->Length / sizeof(WCHAR)];
    while ((*pos != L'\\') && (pos >  ustr->Buffer))
        pos--;

    if (*pos == L'\\')
        pos++;

    for (i = 0; i < sizeof(MI_PFN_CURRENT_PROCESS_NAME) && pos <= strEnd; i++, pos++)
        MI_PFN_CURRENT_PROCESS_NAME[i] = (CHAR)*pos;
}
#else
#define MI_SET_USAGE(x)
#define MI_SET_PROCESS(x)
#define MI_SET_PROCESS2(x)
#endif

typedef enum _MI_PFN_USAGES
{
    MI_USAGE_NOT_SET = 0,
    MI_USAGE_PAGED_POOL,
    MI_USAGE_NONPAGED_POOL,
    MI_USAGE_NONPAGED_POOL_EXPANSION,
    MI_USAGE_KERNEL_STACK,
    MI_USAGE_KERNEL_STACK_EXPANSION,
    MI_USAGE_SYSTEM_PTE,
    MI_USAGE_VAD,
    MI_USAGE_PEB_TEB,
    MI_USAGE_SECTION,
    MI_USAGE_PAGE_TABLE,
    MI_USAGE_PAGE_DIRECTORY,
    MI_USAGE_LEGACY_PAGE_DIRECTORY,
    MI_USAGE_DRIVER_PAGE,
    MI_USAGE_CONTINOUS_ALLOCATION,
    MI_USAGE_MDL,
    MI_USAGE_DEMAND_ZERO,
    MI_USAGE_ZERO_LOOP,
    MI_USAGE_CACHE,
    MI_USAGE_PFN_DATABASE,
    MI_USAGE_BOOT_DRIVER,
    MI_USAGE_INIT_MEMORY,
    MI_USAGE_PAGE_FILE,
    MI_USAGE_COW,
    MI_USAGE_WSLE,
    MI_USAGE_FREE_PAGE
} MI_PFN_USAGES;

//
// These two mappings are actually used by Windows itself, based on the ASSERTS
//
#define StartOfAllocation ReadInProgress
#define EndOfAllocation WriteInProgress

typedef struct _MMPFNENTRY
{
    USHORT Modified:1;
    USHORT ReadInProgress:1;                 // StartOfAllocation
    USHORT WriteInProgress:1;                // EndOfAllocation
    USHORT PrototypePte:1;
    USHORT PageColor:4;
    USHORT PageLocation:3;
    USHORT RemovalRequested:1;
    USHORT CacheAttribute:2;
    USHORT Rom:1;
    USHORT ParityError:1;
} MMPFNENTRY;

// Mm internal
typedef struct _MMPFN
{
    union
    {
        PFN_NUMBER Flink;
        ULONG WsIndex;
        PKEVENT Event;
        NTSTATUS ReadStatus;
        SINGLE_LIST_ENTRY NextStackPfn;

        // HACK for ROSPFN
        SWAPENTRY SwapEntry;
    } u1;
    PMMPTE PteAddress;
    union
    {
        PFN_NUMBER Blink;
        ULONG_PTR ShareCount;
    } u2;
    union
    {
        struct
        {
            USHORT ReferenceCount;
            MMPFNENTRY e1;
        };
        struct
        {
            USHORT ReferenceCount;
            USHORT ShortFlags;
        } e2;
    } u3;
    union
    {
        MMPTE OriginalPte;
        LONG AweReferenceCount;

        // HACK for ROSPFN
        PMM_RMAP_ENTRY RmapListHead;
    };
    union
    {
        ULONG_PTR EntireFrame;
        struct
        {
            ULONG_PTR PteFrame:25;
            ULONG_PTR InPageError:1;
            ULONG_PTR VerifierAllocation:1;
            ULONG_PTR AweAllocation:1;
            ULONG_PTR Priority:3;
            ULONG_PTR MustBeCached:1;
        };
    } u4;
#if MI_TRACE_PFNS
    MI_PFN_USAGES PfnUsage;
    CHAR ProcessName[16];
#define MI_SET_PFN_PROCESS_NAME(pfn, x) memcpy(pfn->ProcessName, x, min(sizeof(x), sizeof(pfn->ProcessName)))
    PVOID CallSite;
#endif

    // HACK until WS lists are supported
    MMWSLE Wsle;
    struct _MMPFN* NextLRU;
    struct _MMPFN* PreviousLRU;
} MMPFN, *PMMPFN;

extern PMMPFN MmPfnDatabase;

typedef struct _MMPFNLIST
{
    PFN_NUMBER Total;
    MMLISTS ListName;
    PFN_NUMBER Flink;
    PFN_NUMBER Blink;
} MMPFNLIST, *PMMPFNLIST;

extern MMPFNLIST MmZeroedPageListHead;
extern MMPFNLIST MmFreePageListHead;
extern MMPFNLIST MmStandbyPageListHead;
extern MMPFNLIST MmModifiedPageListHead;
extern MMPFNLIST MmModifiedNoWritePageListHead;

typedef struct _MM_MEMORY_CONSUMER
{
    ULONG PagesUsed;
    ULONG PagesTarget;
    NTSTATUS (*Trim)(ULONG Target, ULONG Priority, PULONG NrFreed);
} MM_MEMORY_CONSUMER, *PMM_MEMORY_CONSUMER;

typedef struct _MM_REGION
{
    ULONG Type;
    ULONG Protect;
    SIZE_T Length;
    LIST_ENTRY RegionListEntry;
} MM_REGION, *PMM_REGION;

// Mm internal
/* Entry describing free pool memory */
typedef struct _MMFREE_POOL_ENTRY
{
    LIST_ENTRY List;
    PFN_COUNT Size;
    ULONG Signature;
    struct _MMFREE_POOL_ENTRY *Owner;
} MMFREE_POOL_ENTRY, *PMMFREE_POOL_ENTRY;

/* Signature of a freed block */
#define MM_FREE_POOL_SIGNATURE 'ARM3'

/* Paged pool information */
typedef struct _MM_PAGED_POOL_INFO
{
    PRTL_BITMAP PagedPoolAllocationMap;
    PRTL_BITMAP EndOfPagedPoolBitmap;
    PMMPTE FirstPteForPagedPool;
    PMMPTE LastPteForPagedPool;
    PMMPDE NextPdeForPagedPoolExpansion;
    ULONG PagedPoolHint;
    SIZE_T PagedPoolCommit;
    SIZE_T AllocatedPagedPool;
} MM_PAGED_POOL_INFO, *PMM_PAGED_POOL_INFO;

extern MM_MEMORY_CONSUMER MiMemoryConsumers[MC_MAXIMUM];

/* Page file information */
typedef struct _MMPAGING_FILE
{
    PFN_NUMBER Size;
    PFN_NUMBER MaximumSize;
    PFN_NUMBER MinimumSize;
    PFN_NUMBER FreeSpace;
    PFN_NUMBER CurrentUsage;
    PFILE_OBJECT FileObject;
    UNICODE_STRING PageFileName;
    PRTL_BITMAP Bitmap;
    HANDLE FileHandle;
}
MMPAGING_FILE, *PMMPAGING_FILE;

extern PMMPAGING_FILE MmPagingFile[MAX_PAGING_FILES];

typedef VOID
(*PMM_ALTER_REGION_FUNC)(
    PMMSUPPORT AddressSpace,
    PVOID BaseAddress,
    SIZE_T Length,
    ULONG OldType,
    ULONG OldProtect,
    ULONG NewType,
    ULONG NewProtect
);

typedef VOID
(*PMM_FREE_PAGE_FUNC)(
    PVOID Context,
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    PFN_NUMBER Page,
    SWAPENTRY SwapEntry,
    BOOLEAN Dirty
);

//
// Mm copy support for Kd
//
NTSTATUS
NTAPI
MmDbgCopyMemory(
    IN ULONG64 Address,
    IN PVOID Buffer,
    IN ULONG Size,
    IN ULONG Flags
);

//
// Determines if a given address is a session address
//
BOOLEAN
NTAPI
MmIsSessionAddress(
    IN PVOID Address
);

ULONG
NTAPI
MmGetSessionId(
    IN PEPROCESS Process
);

ULONG
NTAPI
MmGetSessionIdEx(
    IN PEPROCESS Process
);

/* marea.c *******************************************************************/

NTSTATUS
NTAPI
MmCreateMemoryArea(
    PMMSUPPORT AddressSpace,
    ULONG Type,
    PVOID *BaseAddress,
    SIZE_T Length,
    ULONG Protection,
    PMEMORY_AREA *Result,
    ULONG AllocationFlags,
    ULONG AllocationGranularity
);

PMEMORY_AREA
NTAPI
MmLocateMemoryAreaByAddress(
    PMMSUPPORT AddressSpace,
    PVOID Address
);

NTSTATUS
NTAPI
MmFreeMemoryArea(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    PMM_FREE_PAGE_FUNC FreePage,
    PVOID FreePageContext
);

VOID
NTAPI
MiRosCleanupMemoryArea(
    PEPROCESS Process,
    PMMVAD Vad);

PMEMORY_AREA
NTAPI
MmLocateMemoryAreaByRegion(
    PMMSUPPORT AddressSpace,
    PVOID Address,
    SIZE_T Length
);

PVOID
NTAPI
MmFindGap(
    PMMSUPPORT AddressSpace,
    SIZE_T Length,
    ULONG_PTR Granularity,
    BOOLEAN TopDown
);

VOID
NTAPI
MiRosCheckMemoryAreas(
   PMMSUPPORT AddressSpace);

VOID
NTAPI
MiCheckAllProcessMemoryAreas(VOID);

/* npool.c *******************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
MiInitializeNonPagedPool(VOID);

PVOID
NTAPI
MiAllocatePoolPages(
    IN POOL_TYPE PoolType,
    IN SIZE_T SizeInBytes
);

POOL_TYPE
NTAPI
MmDeterminePoolType(
    IN PVOID VirtualAddress
);

ULONG
NTAPI
MiFreePoolPages(
    IN PVOID StartingAddress
);

/* pool.c *******************************************************************/

_Requires_lock_held_(PspQuotaLock)
BOOLEAN
NTAPI
MmRaisePoolQuota(
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T CurrentMaxQuota,
    _Out_ PSIZE_T NewMaxQuota
);

_Requires_lock_held_(PspQuotaLock)
VOID
NTAPI
MmReturnPoolQuota(
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T QuotaToReturn
);

/* mdl.c *********************************************************************/

VOID
NTAPI
MmBuildMdlFromPages(
    PMDL Mdl,
    PPFN_NUMBER Pages
);

/* mminit.c ******************************************************************/

VOID
NTAPI
MmInit1(
    VOID
);

CODE_SEG("INIT")
BOOLEAN
NTAPI
MmInitSystem(IN ULONG Phase,
             IN PLOADER_PARAMETER_BLOCK LoaderBlock);


/* pagefile.c ****************************************************************/

SWAPENTRY
NTAPI
MmAllocSwapPage(VOID);

VOID
NTAPI
MmFreeSwapPage(SWAPENTRY Entry);

CODE_SEG("INIT")
VOID
NTAPI
MmInitPagingFile(VOID);

BOOLEAN
NTAPI
MmIsFileObjectAPagingFile(PFILE_OBJECT FileObject);

NTSTATUS
NTAPI
MmReadFromSwapPage(
    SWAPENTRY SwapEntry,
    PFN_NUMBER Page
);

NTSTATUS
NTAPI
MmWriteToSwapPage(
    SWAPENTRY SwapEntry,
    PFN_NUMBER Page
);

VOID
NTAPI
MmShowOutOfSpaceMessagePagingFile(VOID);

NTSTATUS
NTAPI
MiReadPageFile(
    _In_ PFN_NUMBER Page,
    _In_ ULONG PageFileIndex,
    _In_ ULONG_PTR PageFileOffset);

/* process.c ****************************************************************/

NTSTATUS
NTAPI
MmInitializeProcessAddressSpace(
    IN PEPROCESS Process,
    IN PEPROCESS Clone OPTIONAL,
    IN PVOID Section OPTIONAL,
    IN OUT PULONG Flags,
    IN POBJECT_NAME_INFORMATION *AuditName OPTIONAL
);

NTSTATUS
NTAPI
MmCreatePeb(
    IN PEPROCESS Process,
    IN PINITIAL_PEB InitialPeb,
    OUT PPEB *BasePeb
);

NTSTATUS
NTAPI
MmCreateTeb(
    IN PEPROCESS Process,
    IN PCLIENT_ID ClientId,
    IN PINITIAL_TEB InitialTeb,
    OUT PTEB* BaseTeb
);

VOID
NTAPI
MmDeleteTeb(
    struct _EPROCESS *Process,
    PTEB Teb
);

VOID
NTAPI
MmCleanProcessAddressSpace(IN PEPROCESS Process);

VOID
NTAPI
MmDeleteProcessAddressSpace(IN PEPROCESS Process);

ULONG
NTAPI
MmGetSessionLocaleId(VOID);

NTSTATUS
NTAPI
MmSetMemoryPriorityProcess(
    IN PEPROCESS Process,
    IN UCHAR MemoryPriority
);

/* i386/pfault.c *************************************************************/

NTSTATUS
NTAPI
MmPageFault(
    ULONG Cs,
    PULONG Eip,
    PULONG Eax,
    ULONG Cr2,
    ULONG ErrorCode
);

/* special.c *****************************************************************/

VOID
NTAPI
MiInitializeSpecialPool(VOID);

BOOLEAN
NTAPI
MmUseSpecialPool(
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag);

BOOLEAN
NTAPI
MmIsSpecialPoolAddress(
    IN PVOID P);

BOOLEAN
NTAPI
MmIsSpecialPoolAddressFree(
    IN PVOID P);

PVOID
NTAPI
MmAllocateSpecialPool(
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN POOL_TYPE PoolType,
    IN ULONG SpecialType);

VOID
NTAPI
MmFreeSpecialPool(
    IN PVOID P);

/* mm.c **********************************************************************/

NTSTATUS
NTAPI
MmAccessFault(
    IN ULONG FaultCode,
    IN PVOID Address,
    IN KPROCESSOR_MODE Mode,
    IN PVOID TrapInformation
);

/* process.c *****************************************************************/

PVOID
NTAPI
MmCreateKernelStack(BOOLEAN GuiStack, UCHAR Node);

VOID
NTAPI
MmDeleteKernelStack(PVOID Stack,
                    BOOLEAN GuiStack);

/* balance.c / pagefile.c******************************************************/

FORCEINLINE VOID UpdateTotalCommittedPages(LONG Delta)
{
    /*
     * Add up all the used "Committed" memory + pagefile.
     * Not sure this is right. 8^\
     * MmTotalCommittedPages should be adjusted consistently with
     * other counters at different places.
     *
       MmTotalCommittedPages = MiMemoryConsumers[MC_SYSTEM].PagesUsed +
                               MiMemoryConsumers[MC_USER].PagesUsed +
                               MiUsedSwapPages;
     */

    /* Update Commitment */
    SIZE_T TotalCommittedPages = InterlockedExchangeAddSizeT(&MmTotalCommittedPages, Delta) + Delta;

    /* Update Peak = max(Peak, Total) in a lockless way */
    SIZE_T PeakCommitment = MmPeakCommitment;
    while (TotalCommittedPages > PeakCommitment &&
           InterlockedCompareExchangeSizeT(&MmPeakCommitment, TotalCommittedPages, PeakCommitment) != PeakCommitment)
    {
        PeakCommitment = MmPeakCommitment;
    }
}

/* balance.c *****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
MmInitializeMemoryConsumer(
    ULONG Consumer,
    NTSTATUS (*Trim)(ULONG Target, ULONG Priority, PULONG NrFreed)
);

CODE_SEG("INIT")
VOID
NTAPI
MmInitializeBalancer(
    ULONG NrAvailablePages,
    ULONG NrSystemPages
);

NTSTATUS
NTAPI
MmReleasePageMemoryConsumer(
    ULONG Consumer,
    PFN_NUMBER Page
);

NTSTATUS
NTAPI
MmRequestPageMemoryConsumer(
    ULONG Consumer,
    BOOLEAN MyWait,
    PPFN_NUMBER AllocatedPage
);

CODE_SEG("INIT")
VOID
NTAPI
MiInitBalancerThread(VOID);

VOID
NTAPI
MmRebalanceMemoryConsumers(VOID);

/* rmap.c **************************************************************/
#define RMAP_SEGMENT_MASK ~((ULONG_PTR)0xff)
#define RMAP_IS_SEGMENT(x) (((ULONG_PTR)(x) & RMAP_SEGMENT_MASK) == RMAP_SEGMENT_MASK)

VOID
NTAPI
MmSetRmapListHeadPage(
    PFN_NUMBER Page,
    struct _MM_RMAP_ENTRY* ListHead
);

struct _MM_RMAP_ENTRY*
NTAPI
MmGetRmapListHeadPage(PFN_NUMBER Page);

VOID
NTAPI
MmInsertRmap(
    PFN_NUMBER Page,
    struct _EPROCESS *Process,
    PVOID Address
);

VOID
NTAPI
MmDeleteAllRmaps(
    PFN_NUMBER Page,
    PVOID Context,
    VOID (*DeleteMapping)(PVOID Context, struct _EPROCESS *Process, PVOID Address)
);

VOID
NTAPI
MmDeleteRmap(
    PFN_NUMBER Page,
    struct _EPROCESS *Process,
    PVOID Address
);

CODE_SEG("INIT")
VOID
NTAPI
MmInitializeRmapList(VOID);

NTSTATUS
NTAPI
MmPageOutPhysicalAddress(PFN_NUMBER Page);

PMM_SECTION_SEGMENT
NTAPI
MmGetSectionAssociation(PFN_NUMBER Page,
                        PLARGE_INTEGER Offset);

/* freelist.c **********************************************************/
_IRQL_raises_(DISPATCH_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_Requires_lock_not_held_(MmPfnLock)
_Acquires_lock_(MmPfnLock)
_IRQL_saves_
FORCEINLINE
KIRQL
MiAcquirePfnLock(VOID)
{
    return KeAcquireQueuedSpinLock(LockQueuePfnLock);
}

_Requires_lock_held_(MmPfnLock)
_Releases_lock_(MmPfnLock)
_IRQL_requires_(DISPATCH_LEVEL)
FORCEINLINE
VOID
MiReleasePfnLock(
    _In_ _IRQL_restores_ KIRQL OldIrql)
{
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
}

_IRQL_requires_min_(DISPATCH_LEVEL)
_Requires_lock_not_held_(MmPfnLock)
_Acquires_lock_(MmPfnLock)
FORCEINLINE
VOID
MiAcquirePfnLockAtDpcLevel(VOID)
{
    PKSPIN_LOCK_QUEUE LockQueue;

    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);
    LockQueue = &KeGetCurrentPrcb()->LockQueue[LockQueuePfnLock];
    KeAcquireQueuedSpinLockAtDpcLevel(LockQueue);
}

_Requires_lock_held_(MmPfnLock)
_Releases_lock_(MmPfnLock)
_IRQL_requires_min_(DISPATCH_LEVEL)
FORCEINLINE
VOID
MiReleasePfnLockFromDpcLevel(VOID)
{
    PKSPIN_LOCK_QUEUE LockQueue;

    LockQueue = &KeGetCurrentPrcb()->LockQueue[LockQueuePfnLock];
    KeReleaseQueuedSpinLockFromDpcLevel(LockQueue);
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);
}

#define MI_ASSERT_PFN_LOCK_HELD() NT_ASSERT((KeGetCurrentIrql() >= DISPATCH_LEVEL) && (MmPfnLock != 0))

FORCEINLINE
PMMPFN
MiGetPfnEntry(IN PFN_NUMBER Pfn)
{
    PMMPFN Page;
    extern RTL_BITMAP MiPfnBitMap;

    /* Make sure the PFN number is valid */
    if (Pfn > MmHighestPhysicalPage) return NULL;

    /* Make sure this page actually has a PFN entry */
    if ((MiPfnBitMap.Buffer) && !(RtlTestBit(&MiPfnBitMap, (ULONG)Pfn))) return NULL;

    /* Get the entry */
    Page = &MmPfnDatabase[Pfn];

    /* Return it */
    return Page;
};

FORCEINLINE
PFN_NUMBER
MiGetPfnEntryIndex(IN PMMPFN Pfn1)
{
    //
    // This will return the Page Frame Number (PFN) from the MMPFN
    //
    return Pfn1 - MmPfnDatabase;
}

PFN_NUMBER
NTAPI
MmGetLRUNextUserPage(PFN_NUMBER PreviousPage, BOOLEAN MoveToLast);

PFN_NUMBER
NTAPI
MmGetLRUFirstUserPage(VOID);

VOID
NTAPI
MmDumpArmPfnDatabase(
   IN BOOLEAN StatusOnly
);

VOID
NTAPI
MmZeroPageThread(
    VOID
);

/* hypermap.c *****************************************************************/
PVOID
NTAPI
MiMapPageInHyperSpace(IN PEPROCESS Process,
                      IN PFN_NUMBER Page,
                      IN PKIRQL OldIrql);

VOID
NTAPI
MiUnmapPageInHyperSpace(IN PEPROCESS Process,
                        IN PVOID Address,
                        IN KIRQL OldIrql);

PVOID
NTAPI
MiMapPagesInZeroSpace(IN PMMPFN Pfn1,
                      IN PFN_NUMBER NumberOfPages);

VOID
NTAPI
MiUnmapPagesInZeroSpace(IN PVOID VirtualAddress,
                        IN PFN_NUMBER NumberOfPages);

/* i386/page.c *********************************************************/

NTSTATUS
NTAPI
MmCreateVirtualMapping(
    struct _EPROCESS* Process,
    PVOID Address,
    ULONG flProtect,
    PFN_NUMBER Page
);

NTSTATUS
NTAPI
MmCreateVirtualMappingUnsafe(
    struct _EPROCESS* Process,
    PVOID Address,
    ULONG flProtect,
    PFN_NUMBER Page
);

NTSTATUS
NTAPI
MmCreatePhysicalMapping(
    _Inout_opt_ PEPROCESS Process,
    _In_ PVOID Address,
    _In_ ULONG flProtect,
    _In_ PFN_NUMBER Page);

ULONG
NTAPI
MmGetPageProtect(
    struct _EPROCESS* Process,
    PVOID Address);

VOID
NTAPI
MmSetPageProtect(
    struct _EPROCESS* Process,
    PVOID Address,
    ULONG flProtect
);

BOOLEAN
NTAPI
MmIsPagePresent(
    struct _EPROCESS* Process,
    PVOID Address
);

BOOLEAN
NTAPI
MmIsDisabledPage(
    struct _EPROCESS* Process,
    PVOID Address
);

CODE_SEG("INIT")
VOID
NTAPI
MmInitGlobalKernelPageDirectory(VOID);

VOID
NTAPI
MmDeletePageFileMapping(
    struct _EPROCESS *Process,
    PVOID Address,
    SWAPENTRY* SwapEntry
);

NTSTATUS
NTAPI
MmCreatePageFileMapping(
    struct _EPROCESS *Process,
    PVOID Address,
    SWAPENTRY SwapEntry
);

VOID
NTAPI
MmGetPageFileMapping(
    PEPROCESS Process,
    PVOID Address,
    SWAPENTRY *SwapEntry);

BOOLEAN
NTAPI
MmIsPageSwapEntry(
    struct _EPROCESS *Process,
    PVOID Address
);

PFN_NUMBER
NTAPI
MmAllocPage(
    ULONG Consumer
);

VOID
NTAPI
MmDereferencePage(PFN_NUMBER Page);

VOID
NTAPI
MmReferencePage(PFN_NUMBER Page);

ULONG
NTAPI
MmGetReferenceCountPage(PFN_NUMBER Page);

BOOLEAN
NTAPI
MmIsPageInUse(PFN_NUMBER Page);

VOID
NTAPI
MmSetSavedSwapEntryPage(
    PFN_NUMBER Page,
    SWAPENTRY SavedSwapEntry);

SWAPENTRY
NTAPI
MmGetSavedSwapEntryPage(PFN_NUMBER Page);

VOID
NTAPI
MmSetCleanPage(
    struct _EPROCESS *Process,
    PVOID Address
);

VOID
NTAPI
MmSetDirtyBit(PEPROCESS Process, PVOID Address, BOOLEAN Bit);
#define MmSetCleanPage(__P, __A) MmSetDirtyBit(__P, __A, FALSE)
#define MmSetDirtyPage(__P, __A) MmSetDirtyBit(__P, __A, TRUE)

VOID
NTAPI
MmDeletePageTable(
    struct _EPROCESS *Process,
    PVOID Address
);

PFN_NUMBER
NTAPI
MmGetPfnForProcess(
    struct _EPROCESS *Process,
    PVOID Address
);

BOOLEAN
NTAPI
MmCreateProcessAddressSpace(
    IN ULONG MinWs,
    IN PEPROCESS Dest,
    IN PULONG_PTR DirectoryTableBase
);

CODE_SEG("INIT")
NTSTATUS
NTAPI
MmInitializeHandBuiltProcess(
    IN PEPROCESS Process,
    IN PULONG_PTR DirectoryTableBase
);

CODE_SEG("INIT")
NTSTATUS
NTAPI
MmInitializeHandBuiltProcess2(
    IN PEPROCESS Process
);

NTSTATUS
NTAPI
MmSetExecuteOptions(IN ULONG ExecuteOptions);

NTSTATUS
NTAPI
MmGetExecuteOptions(IN PULONG ExecuteOptions);

_Success_(return)
BOOLEAN
MmDeleteVirtualMapping(
    _Inout_opt_ PEPROCESS Process,
    _In_ PVOID Address,
    _Out_opt_ BOOLEAN* WasDirty,
    _Out_opt_ PPFN_NUMBER Page
);

_Success_(return)
BOOLEAN
MmDeletePhysicalMapping(
    _Inout_opt_ PEPROCESS Process,
    _In_ PVOID Address,
    _Out_opt_ BOOLEAN * WasDirty,
    _Out_opt_ PPFN_NUMBER Page
);

/* arch/procsup.c ************************************************************/

BOOLEAN
MiArchCreateProcessAddressSpace(
    _In_ PEPROCESS Process,
    _In_ PULONG_PTR DirectoryTableBase);

/* wset.c ********************************************************************/

NTSTATUS
MmTrimUserMemory(
    ULONG Target,
    ULONG Priority,
    PULONG NrFreedPages
);

/* region.c ************************************************************/

NTSTATUS
NTAPI
MmAlterRegion(
    PMMSUPPORT AddressSpace,
    PVOID BaseAddress,
    PLIST_ENTRY RegionListHead,
    PVOID StartAddress,
    SIZE_T Length,
    ULONG NewType,
    ULONG NewProtect,
    PMM_ALTER_REGION_FUNC AlterFunc
);

VOID
NTAPI
MmInitializeRegion(
    PLIST_ENTRY RegionListHead,
    SIZE_T Length,
    ULONG Type,
    ULONG Protect
);

PMM_REGION
NTAPI
MmFindRegion(
    PVOID BaseAddress,
    PLIST_ENTRY RegionListHead,
    PVOID Address,
    PVOID* RegionBaseAddress
);

/* section.c *****************************************************************/

#define PFN_FROM_SSE(E)          ((PFN_NUMBER)((E) >> PAGE_SHIFT))
#define IS_SWAP_FROM_SSE(E)      ((E) & 0x00000001)
#define MM_IS_WAIT_PTE(E)        \
    (IS_SWAP_FROM_SSE(E) && SWAPENTRY_FROM_SSE(E) == MM_WAIT_ENTRY)
#define MAKE_PFN_SSE(P)          ((ULONG_PTR)((P) << PAGE_SHIFT))
#define SWAPENTRY_FROM_SSE(E)    ((E) >> 1)
#define MAKE_SWAP_SSE(S)         (((ULONG_PTR)(S) << 1) | 0x1)
#define DIRTY_SSE(E)             ((E) | 2)
#define CLEAN_SSE(E)             ((E) & ~2)
#define IS_DIRTY_SSE(E)          ((E) & 2)
#define WRITE_SSE(E)             ((E) | 4)
#define IS_WRITE_SSE(E)          ((E) & 4)
#ifdef _WIN64
#define PAGE_FROM_SSE(E)         ((E) & 0xFFFFFFF000ULL)
#else
#define PAGE_FROM_SSE(E)         ((E) & 0xFFFFF000)
#endif
#define SHARE_COUNT_FROM_SSE(E)  (((E) & 0x00000FFC) >> 3)
#define MAX_SHARE_COUNT          0x1FF
#define MAKE_SSE(P, C)           ((ULONG_PTR)((P) | ((C) << 3)))
#define BUMPREF_SSE(E)           (PAGE_FROM_SSE(E) | ((SHARE_COUNT_FROM_SSE(E) + 1) << 3) | ((E) & 0x7))
#define DECREF_SSE(E)            (PAGE_FROM_SSE(E) | ((SHARE_COUNT_FROM_SSE(E) - 1) << 3) | ((E) & 0x7))

VOID
NTAPI
_MmLockSectionSegment(PMM_SECTION_SEGMENT Segment,
                      const char *file,
                      int line);

#define MmLockSectionSegment(x) _MmLockSectionSegment(x,__FILE__,__LINE__)

VOID
NTAPI
_MmUnlockSectionSegment(PMM_SECTION_SEGMENT Segment,
                        const char *file,
                        int line);

#define MmUnlockSectionSegment(x) _MmUnlockSectionSegment(x,__FILE__,__LINE__)

VOID
NTAPI
MmGetImageInformation(
    OUT PSECTION_IMAGE_INFORMATION ImageInformation
);

PFILE_OBJECT
NTAPI
MmGetFileObjectForSection(
    IN PVOID Section
);
NTSTATUS
NTAPI
MmGetFileNameForAddress(
    IN PVOID Address,
    OUT PUNICODE_STRING ModuleName
);

NTSTATUS
NTAPI
MmGetFileNameForSection(
    IN PVOID Section,
    OUT POBJECT_NAME_INFORMATION *ModuleName
);

NTSTATUS
NTAPI
MmQuerySectionView(
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    PMEMORY_BASIC_INFORMATION Info,
    PSIZE_T ResultLength
);

NTSTATUS
NTAPI
MmProtectSectionView(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    PVOID BaseAddress,
    SIZE_T Length,
    ULONG Protect,
    PULONG OldProtect
);

CODE_SEG("INIT")
NTSTATUS
NTAPI
MmInitSectionImplementation(VOID);

NTSTATUS
NTAPI
MmNotPresentFaultSectionView(
    PMMSUPPORT AddressSpace,
    MEMORY_AREA* MemoryArea,
    PVOID Address,
    BOOLEAN Locked
);

NTSTATUS
NTAPI
MmPageOutSectionView(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    ULONG_PTR Entry
);

CODE_SEG("INIT")
NTSTATUS
NTAPI
MmCreatePhysicalMemorySection(VOID);

NTSTATUS
NTAPI
MmAccessFaultSectionView(
    PMMSUPPORT AddressSpace,
    MEMORY_AREA* MemoryArea,
    PVOID Address,
    BOOLEAN Locked
);

VOID
NTAPI
MmFreeSectionSegments(PFILE_OBJECT FileObject);

/* Exported from NT 6.2 onward. We keep it internal. */
NTSTATUS
NTAPI
MmMapViewInSystemSpaceEx(
    _In_ PVOID Section,
    _Outptr_result_bytebuffer_ (*ViewSize) PVOID *MappedBase,
    _Inout_ PSIZE_T ViewSize,
    _Inout_ PLARGE_INTEGER SectionOffset,
    _In_ ULONG_PTR Flags);

BOOLEAN
NTAPI
MmIsDataSectionResident(
    _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
    _In_ LONGLONG Offset,
    _In_ ULONG Length);

NTSTATUS
NTAPI
MmMakeSegmentDirty(
    _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
    _In_ LONGLONG Offset,
    _In_ ULONG Length);

NTSTATUS
NTAPI
MmFlushSegment(
    _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
    _In_opt_ PLARGE_INTEGER Offset,
    _In_ ULONG Length,
    _Out_opt_ PIO_STATUS_BLOCK Iosb);

NTSTATUS
NTAPI
MmMakeDataSectionResident(
    _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
    _In_ LONGLONG Offset,
    _In_ ULONG Length,
    _In_ PLARGE_INTEGER ValidDataLength);

BOOLEAN
NTAPI
MmPurgeSegment(
    _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
    _In_opt_ PLARGE_INTEGER Offset,
    _In_ ULONG Length);

BOOLEAN
NTAPI
MmCheckDirtySegment(
    PMM_SECTION_SEGMENT Segment,
    PLARGE_INTEGER Offset,
    BOOLEAN ForceDirty,
    BOOLEAN PageOut);

BOOLEAN
NTAPI
MmUnsharePageEntrySectionSegment(PMEMORY_AREA MemoryArea,
                                 PMM_SECTION_SEGMENT Segment,
                                 PLARGE_INTEGER Offset,
                                 BOOLEAN Dirty,
                                 BOOLEAN PageOut,
                                 ULONG_PTR *InEntry);

_When_(OldIrql == MM_NOIRQL, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(OldIrql == MM_NOIRQL, _Requires_lock_not_held_(MmPfnLock))
_When_(OldIrql != MM_NOIRQL, _Requires_lock_held_(MmPfnLock))
_When_(OldIrql != MM_NOIRQL, _Releases_lock_(MmPfnLock))
_When_(OldIrql != MM_NOIRQL, _IRQL_requires_(DISPATCH_LEVEL))
VOID
NTAPI
MmDereferenceSegmentWithLock(
    _In_ PMM_SECTION_SEGMENT Segment,
    _In_ _When_(OldIrql != MM_NOIRQL, _IRQL_restores_) KIRQL OldIrql);

_IRQL_requires_max_(DISPATCH_LEVEL)
_Requires_lock_not_held_(MmPfnLock)
FORCEINLINE
VOID
MmDereferenceSegment(PMM_SECTION_SEGMENT Segment)
{
    MmDereferenceSegmentWithLock(Segment, MM_NOIRQL);
}

NTSTATUS
NTAPI
MmExtendSection(
    _In_ PVOID Section,
    _Inout_ PLARGE_INTEGER NewSize);

/* sptab.c *******************************************************************/

NTSTATUS
NTAPI
_MmSetPageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
                              PLARGE_INTEGER Offset,
                              ULONG_PTR Entry,
                              const char *file,
                              int line);

ULONG_PTR
NTAPI
_MmGetPageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
                              PLARGE_INTEGER Offset,
                              const char *file,
                              int line);

#define MmSetPageEntrySectionSegment(S,O,E) _MmSetPageEntrySectionSegment(S,O,E,__FILE__,__LINE__)

#define MmGetPageEntrySectionSegment(S,O) _MmGetPageEntrySectionSegment(S,O,__FILE__,__LINE__)

/* sysldr.c ******************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
MiReloadBootLoadedDrivers(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

CODE_SEG("INIT")
BOOLEAN
NTAPI
MiInitializeLoadedModuleList(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

BOOLEAN
NTAPI
MmChangeKernelResourceSectionProtection(IN ULONG_PTR ProtectionMask);

VOID
NTAPI
MmMakeKernelResourceSectionWritable(VOID);

NTSTATUS
NTAPI
MmLoadSystemImage(
    IN PUNICODE_STRING FileName,
    IN PUNICODE_STRING NamePrefix OPTIONAL,
    IN PUNICODE_STRING LoadedName OPTIONAL,
    IN ULONG Flags,
    OUT PVOID *ModuleObject,
    OUT PVOID *ImageBaseAddress
);

NTSTATUS
NTAPI
MmUnloadSystemImage(
    IN PVOID ImageHandle
);

NTSTATUS
NTAPI
MmCheckSystemImage(
    IN HANDLE ImageHandle,
    IN BOOLEAN PurgeSection
);

NTSTATUS
NTAPI
MmCallDllInitialize(
    _In_ PLDR_DATA_TABLE_ENTRY LdrEntry,
    _In_ PLIST_ENTRY ModuleListHead);

VOID
NTAPI
MmFreeDriverInitialization(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry);

/* ReactOS-only, used by psmgr.c PspLookupSystemDllEntryPoint() as well */
NTSTATUS
NTAPI
RtlpFindExportedRoutineByName(
    _In_ PVOID ImageBase,
    _In_ PCSTR ExportName,
    _Out_ PVOID* Function,
    _Out_opt_ PBOOLEAN IsForwarder,
    _In_ NTSTATUS NotFoundStatus);

/* Exported from NT 10.0 onward. We keep it internal. */
PVOID
NTAPI
RtlFindExportedRoutineByName(
    _In_ PVOID ImageBase,
    _In_ PCSTR ExportName);

/* procsup.c *****************************************************************/

NTSTATUS
NTAPI
MmGrowKernelStack(
    IN PVOID StackPointer
);


FORCEINLINE
VOID
MmLockAddressSpace(PMMSUPPORT AddressSpace)
{
    ASSERT(!PsGetCurrentThread()->OwnsProcessWorkingSetExclusive);
    ASSERT(!PsGetCurrentThread()->OwnsProcessWorkingSetShared);
    ASSERT(!PsGetCurrentThread()->OwnsSystemWorkingSetExclusive);
    ASSERT(!PsGetCurrentThread()->OwnsSystemWorkingSetShared);
    ASSERT(!PsGetCurrentThread()->OwnsSessionWorkingSetExclusive);
    ASSERT(!PsGetCurrentThread()->OwnsSessionWorkingSetShared);
    KeAcquireGuardedMutex(&CONTAINING_RECORD(AddressSpace, EPROCESS, Vm)->AddressCreationLock);
}

FORCEINLINE
VOID
MmUnlockAddressSpace(PMMSUPPORT AddressSpace)
{
    KeReleaseGuardedMutex(&CONTAINING_RECORD(AddressSpace, EPROCESS, Vm)->AddressCreationLock);
}

FORCEINLINE
PEPROCESS
MmGetAddressSpaceOwner(IN PMMSUPPORT AddressSpace)
{
    if (AddressSpace == MmKernelAddressSpace) return NULL;
    return CONTAINING_RECORD(AddressSpace, EPROCESS, Vm);
}

FORCEINLINE
PMMSUPPORT
MmGetCurrentAddressSpace(VOID)
{
    return &((PEPROCESS)KeGetCurrentThread()->ApcState.Process)->Vm;
}

FORCEINLINE
PMMSUPPORT
MmGetKernelAddressSpace(VOID)
{
    return MmKernelAddressSpace;
}


/* expool.c ******************************************************************/

VOID
NTAPI
ExpCheckPoolAllocation(
    PVOID P,
    POOL_TYPE PoolType,
    ULONG Tag);

VOID
NTAPI
ExReturnPoolQuota(
    IN PVOID P);


/* mmsup.c *****************************************************************/

NTSTATUS
NTAPI
MmAdjustWorkingSetSize(
    IN SIZE_T WorkingSetMinimumInBytes,
    IN SIZE_T WorkingSetMaximumInBytes,
    IN ULONG SystemCache,
    IN BOOLEAN IncreaseOkay);


/* session.c *****************************************************************/

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NTAPI
MmAttachSession(
    _Inout_ PVOID SessionEntry,
    _Out_ PKAPC_STATE ApcState);

_IRQL_requires_max_(APC_LEVEL)
VOID
NTAPI
MmDetachSession(
    _Inout_ PVOID SessionEntry,
    _Out_ PKAPC_STATE ApcState);

VOID
NTAPI
MmQuitNextSession(
    _Inout_ PVOID SessionEntry);

PVOID
NTAPI
MmGetSessionById(
    _In_ ULONG SessionId);

_IRQL_requires_max_(APC_LEVEL)
VOID
NTAPI
MmSetSessionLocaleId(
    _In_ LCID LocaleId);

/* shutdown.c *****************************************************************/

VOID
MmShutdownSystem(IN ULONG Phase);

/* virtual.c *****************************************************************/

NTSTATUS
NTAPI
MmCopyVirtualMemory(IN PEPROCESS SourceProcess,
                    IN PVOID SourceAddress,
                    IN PEPROCESS TargetProcess,
                    OUT PVOID TargetAddress,
                    IN SIZE_T BufferSize,
                    IN KPROCESSOR_MODE PreviousMode,
                    OUT PSIZE_T ReturnSize);

/* wslist.cpp ****************************************************************/
_Requires_exclusive_lock_held_(WorkingSet->WorkingSetMutex)
VOID
NTAPI
MiInitializeWorkingSetList(_Inout_ PMMSUPPORT WorkingSet);

#ifdef __cplusplus
} // extern "C"

namespace ntoskrnl
{
using MiPfnLockGuard = const KiQueuedSpinLockGuard<LockQueuePfnLock>;
} // namespace ntoskrnl

#endif
