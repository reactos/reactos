#ifndef __INCLUDE_INTERNAL_MM_H
#define __INCLUDE_INTERNAL_MM_H

#include <internal/arch/mm.h>

/* TYPES *********************************************************************/

struct _EPROCESS;

extern ULONG MiFreeSwapPages;
extern ULONG MiUsedSwapPages;
extern ULONG MmPagedPoolSize;
extern ULONG MmTotalPagedPoolQuota;
extern ULONG MmTotalNonPagedPoolQuota;
extern PHYSICAL_ADDRESS MmSharedDataPagePhysicalAddress;
extern ULONG MmNumberOfPhysicalPages;
extern UCHAR MmDisablePagingExecutive;
extern ULONG MmLowestPhysicalPage;
extern ULONG MmHighestPhysicalPage;
extern ULONG MmAvailablePages;
extern ULONG MmResidentAvailablePages;

extern PVOID MmPagedPoolBase;
extern ULONG MmPagedPoolSize;

extern PMEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptor;
extern MEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptorOrg;

extern LIST_ENTRY MmLoadedUserImageList;

extern KMUTANT MmSystemLoadLock;

extern ULONG MmNumberOfPagingFiles;

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

extern BOOLEAN MiDbgReadyForPhysical;

struct _KTRAP_FRAME;
struct _EPROCESS;
struct _MM_RMAP_ENTRY;
struct _MM_PAGEOP;
typedef ULONG SWAPENTRY;
typedef ULONG PFN_TYPE, *PPFN_TYPE;

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

#if defined(_X86_)
#define MI_STATIC_MEMORY_AREAS              (14)
#else
#define MI_STATIC_MEMORY_AREAS              (13)
#endif

#define MEMORY_AREA_INVALID                 (0)
#define MEMORY_AREA_SECTION_VIEW            (1)
#define MEMORY_AREA_CONTINUOUS_MEMORY       (2)
#define MEMORY_AREA_NO_CACHE                (3)
#define MEMORY_AREA_IO_MAPPING              (4)
#define MEMORY_AREA_SYSTEM                  (5)
#define MEMORY_AREA_MDL_MAPPING             (7)
#define MEMORY_AREA_VIRTUAL_MEMORY          (8)
#define MEMORY_AREA_CACHE_SEGMENT           (9)
#define MEMORY_AREA_SHARED_DATA             (10)
#define MEMORY_AREA_KERNEL_STACK            (11)
#define MEMORY_AREA_PAGED_POOL              (12)
#define MEMORY_AREA_NO_ACCESS               (13)
#define MEMORY_AREA_PEB_OR_TEB              (14)
#define MEMORY_AREA_OWNED_BY_ARM3           (15)
#define MEMORY_AREA_PHYSICAL_MEMORY_SECTION (0x00010000)
#define MEMORY_AREA_PAGE_FILE_SECTION       (0x00010001)
#define MEMORY_AREA_IMAGE_SECTION           (0x00010002)
#define MEMORY_AREA_STATIC                  (0x80000000)

#define MM_PHYSICAL_PAGE_MPW_PENDING        (0x8)

#define MM_CORE_DUMP_TYPE_NONE              (0x0)
#define MM_CORE_DUMP_TYPE_MINIMAL           (0x1)
#define MM_CORE_DUMP_TYPE_FULL              (0x2)

#define MM_PAGEOP_PAGEIN                    (1)
#define MM_PAGEOP_PAGEOUT                   (2)
#define MM_PAGEOP_PAGESYNCH                 (3)
#define MM_PAGEOP_ACCESSFAULT               (4)

/* Number of list heads to use */
#define MI_FREE_POOL_LISTS 4

#define MI_HYPERSPACE_PTES                  (256 - 1)
#define MI_ZERO_PTES                        (32)
#define MI_MAPPING_RANGE_START              (ULONG)HYPER_SPACE
#define MI_MAPPING_RANGE_END                (MI_MAPPING_RANGE_START + \
                                             MI_HYPERSPACE_PTES * PAGE_SIZE)
#define MI_ZERO_PTE                         (PMMPTE)(MI_MAPPING_RANGE_END + \
                                             PAGE_SIZE)

#define MM_WAIT_ENTRY            0x7fffffff
#define PFN_FROM_SSE(E)          ((E) >> PAGE_SHIFT)
#define IS_SWAP_FROM_SSE(E)      ((E) & 0x00000001)
#define MM_IS_WAIT_PTE(E)        \
	(IS_SWAP_FROM_SSE(E) && SWAPENTRY_FROM_SSE(E) == MM_WAIT_ENTRY)
#define MAKE_PFN_SSE(P)          ((P) << PAGE_SHIFT)
#define SWAPENTRY_FROM_SSE(E)    ((E) >> 1)
#define MAKE_SWAP_SSE(S)         (((S) << 1) | 0x1)
#define DIRTY_SSE(E)             ((E) | 2)
#define CLEAN_SSE(E)             ((E) & ~2)
#define IS_DIRTY_SSE(E)          ((E) & 2)

#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))

/* Determine what's needed to make paged pool fit in this category.
 * it seems that something more is required to satisfy arm3. */
#define BALANCER_CAN_EVICT(Consumer) \
	(((Consumer) == MC_USER) || \
	 ((Consumer) == MC_CACHE)) 

/* Signature of free pool blocks */
#define MM_FREE_POOL_TAG    'lprF'

#define PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(x) \
    ((x) / (4*1024*1024))

#define PAGE_TO_SECTION_PAGE_TABLE_OFFSET(x) \
    ((((x)) % (4*1024*1024)) / (4*1024))

#define NR_SECTION_PAGE_TABLES              1024
#define NR_SECTION_PAGE_ENTRIES             1024

#define TEB_BASE                            0x7FFDE000

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
#define SEC_CACHE                           (0x40000000)

/* These shouldn't be bit flags but are */
#define MM_PAGEFILE_SEGMENT                 (0x1)
#define MM_DATAFILE_SEGMENT                 (0x2)
#define MM_PHYSIMEM_SEGMENT                 (0x4)
#define MM_IMAGE_SEGMENT                    (0x8)

#define MC_CACHE                            (0)
#define MC_USER                             (1)
#define MC_PPOOL                            (2)
#define MC_NPPOOL                           (3)
#define MC_SYSTEM                           (4)
#define MC_MAXIMUM                          (5)

#define PAGED_POOL_MASK                     1
#define MUST_SUCCEED_POOL_MASK              2
#define CACHE_ALIGNED_POOL_MASK             4
#define QUOTA_POOL_MASK                     8
#define SESSION_POOL_MASK                   32
#define VERIFIER_POOL_MASK                  64

#define MM_PAGED_POOL_SIZE                  (100*1024*1024)
#define MM_NONPAGED_POOL_SIZE               (100*1024*1024)

/*
 * Paged and non-paged pools are 8-byte aligned
 */
#define MM_POOL_ALIGNMENT                   8

/*
 * Maximum size of the kmalloc area (this is totally arbitary)
 */
#define MM_KERNEL_MAP_SIZE                  (16*1024*1024)
#define MM_KERNEL_MAP_BASE                  (0xf0c00000)

/*
 * FIXME - different architectures have different cache line sizes...
 */
#define MM_CACHE_LINE_SIZE                  32

#define MM_ROUND_UP(x,s)                    \
    ((PVOID)(((ULONG_PTR)(x)+(s)-1) & ~((ULONG_PTR)(s)-1)))

#define MM_ROUND_DOWN(x,s)                  \
    ((PVOID)(((ULONG_PTR)(x)) & ~((ULONG_PTR)(s)-1)))

#define PAGE_FLAGS_VALID_FROM_USER_MODE     \
    (PAGE_READONLY | \
    PAGE_READWRITE | \
    PAGE_WRITECOPY | \
    PAGE_EXECUTE | \
    PAGE_EXECUTE_READ | \
    PAGE_EXECUTE_READWRITE | \
    PAGE_EXECUTE_WRITECOPY | \
    PAGE_GUARD | \
    PAGE_NOACCESS | \
    PAGE_NOCACHE)

#define PAGE_FLAGS_VALID_FOR_SECTION \
    (PAGE_READONLY | \
     PAGE_READWRITE | \
     PAGE_WRITECOPY | \
     PAGE_EXECUTE | \
     PAGE_EXECUTE_READ | \
     PAGE_EXECUTE_READWRITE | \
     PAGE_EXECUTE_WRITECOPY | \
     PAGE_NOACCESS)

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


#define InterlockedCompareExchangePte(PointerPte, Exchange, Comperand) \
    InterlockedCompareExchange((PLONG)(PointerPte), Exchange, Comperand)

#define InterlockedExchangePte(PointerPte, Value) \
    InterlockedExchange((PLONG)(PointerPte), Value)

typedef struct _MM_SECTION_SEGMENT
{
    FAST_MUTEX Lock;		/* lock which protects the page directory */
	PFILE_OBJECT FileObject;
    ULARGE_INTEGER RawLength;		/* length of the segment which is part of the mapped file */
    ULARGE_INTEGER Length;			/* absolute length of the segment */
    ULONG ReferenceCount;
    ULONG Protection;
    ULONG Flags;
    BOOLEAN WriteCopy;

	struct 
	{
		LONG FileOffset;		/* start offset into the file for image sections */
		ULONG_PTR VirtualAddress;	/* dtart offset into the address range for image sections */
		ULONG Characteristics;
	} Image;

	RTL_GENERIC_TABLE PageTable;
} MM_SECTION_SEGMENT, *PMM_SECTION_SEGMENT;

typedef struct _MM_IMAGE_SECTION_OBJECT
{
    ULONG_PTR ImageBase;
    ULONG_PTR StackReserve;
    ULONG_PTR StackCommit;
    ULONG_PTR EntryPoint;
	ULONG RefCount;
    USHORT Subsystem;
    USHORT ImageCharacteristics;
    USHORT MinorSubsystemVersion;
    USHORT MajorSubsystemVersion;
    USHORT Machine;
    BOOLEAN Executable;
    ULONG NrSegments;
    ULONG ImageSize;
    PMM_SECTION_SEGMENT Segments;
} MM_IMAGE_SECTION_OBJECT, *PMM_IMAGE_SECTION_OBJECT;

typedef struct _ROS_SECTION_OBJECT
{
    CSHORT Type;
    CSHORT Size;
    LARGE_INTEGER MaximumSize;
    ULONG SectionPageProtection;
    ULONG AllocationAttributes;
    PFILE_OBJECT FileObject;
    union
    {
        PMM_IMAGE_SECTION_OBJECT ImageSection;
        PMM_SECTION_SEGMENT Segment;
    };
} ROS_SECTION_OBJECT, *PROS_SECTION_OBJECT;

#define MM_REQUIRE_PAGE_1 1
#define MM_REQUIRE_PAGE_2 2
#define MM_REQUIRE_BUFFER_1 0x10
#define MM_REQUIRE_BUFFER_2 0x20
#define MM_REQUIRE_SWAP_ENTRY 0x800
#define MM_REQUIRE_FLAG_MASK 0x833

#define MM_BUFFER_SIZE(N,X) \
	(4 * ((N) ? ((X) >> 12) : ((X) >> 23)))
#define MM_MAKE_REQUIREREMENT(F,B1,B2) \
	(((((B2) + 3) >> 2) << 23) | ((((B1) + 3) >> 2) << 12) | (F))

struct _MEMORY_AREA;
struct _MM_REQUIRED_RESOURCES;

typedef NTSTATUS (NTAPI * AcquireResource)
	(PMMSUPPORT AddressSpace,
	 struct _MEMORY_AREA *MemoryArea,
	 struct _MM_REQUIRED_RESOURCES *Required);
typedef NTSTATUS (NTAPI * NotPresentFaultHandler)
	(PMMSUPPORT AddressSpace, 
	 struct _MEMORY_AREA *MemoryArea, 
	 PVOID Address,
	 BOOLEAN Locked,
	 struct _MM_REQUIRED_RESOURCES *Required);
typedef NTSTATUS (NTAPI * FaultHandler)
	(PMMSUPPORT AddressSpace, 
	 struct _MEMORY_AREA *MemoryArea, 
	 PVOID Address,
	 struct _MM_REQUIRED_RESOURCES *Required);

typedef struct _MM_REQUIRED_RESOURCES
{
	ULONG Consumer;
	ULONG Amount;
	ULONG Offset;
	ULONG State;
	PVOID Context;
	LARGE_INTEGER FileOffset;
	AcquireResource DoAcquisition;
	PFN_TYPE Page[2];
	PVOID Buffer[2];
	SWAPENTRY SwapEntry;
} MM_REQUIRED_RESOURCES, *PMM_REQUIRED_RESOURCES;

typedef struct _MEMORY_AREA
{
    PVOID StartingAddress;
    PVOID EndingAddress;
    struct _MEMORY_AREA *Parent;
    struct _MEMORY_AREA *LeftChild;
    struct _MEMORY_AREA *RightChild;
    ULONG Type;
    ULONG Protect;
    ULONG Flags;
    BOOLEAN DeleteInProgress;
    ULONG PageOpCount;
	NotPresentFaultHandler NotPresent;
	NotPresentFaultHandler AccessFault;
	FaultHandler PageOut;
    union
    {
        struct
        {
            ROS_SECTION_OBJECT* Section;
            PMM_SECTION_SEGMENT Segment;
            LARGE_INTEGER ViewOffset;
            BOOLEAN WriteCopyView;
            LIST_ENTRY RegionListHead;
        } SectionData;
        struct
        {
            LIST_ENTRY RegionListHead;
        } VirtualMemoryData;
    } Data;

	
} MEMORY_AREA, *PMEMORY_AREA;

typedef struct
{
    ULONG NrTotalPages;
    ULONG NrSystemPages;
    ULONG NrUserPages;
    ULONG NrFreePages;
    ULONG NrDirtyPages;
    ULONG NrLockedPages;
    ULONG PagingRequestsInLastMinute;
    ULONG PagingRequestsInLastFiveMinutes;
    ULONG PagingRequestsInLastFifteenMinutes;
} MM_STATS;

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
    USHORT PrototypePte:1;                   // Zero
    USHORT PageColor:4;                      // LockCount
    USHORT PageLocation:3;                   // Consumer
    USHORT RemovalRequested:1;
    USHORT CacheAttribute:2;                 // Type
    USHORT Rom:1;
    USHORT ParityError:1;
} MMPFNENTRY;

typedef struct _MMPFN
{
    union
    {
        PFN_NUMBER Flink;                    // ListEntry.Flink
        ULONG WsIndex;
        PKEVENT Event;
        NTSTATUS ReadStatus;
        SINGLE_LIST_ENTRY NextStackPfn;
    } u1;
    PMMPTE PteAddress;                       // ListEntry.Blink
    union
    {
        PFN_NUMBER Blink;
        ULONG_PTR ShareCount;                // MapCount
		PVOID SegmentPart;
    } u2;
    union
    {
        struct
        {
            USHORT ReferenceCount;           // ReferenceCount
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
        LONG AweReferenceCount;              // RmapListHead
    };
    union
    {
        ULONG_PTR EntireFrame;               // SavedSwapEntry
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

typedef struct _MM_PAGEOP
{
  /* Type of operation. */
  ULONG OpType;
  /* Number of threads interested in this operation. */
  ULONG ReferenceCount;
  /* Event that will be set when the operation is completed. */
  KEVENT CompletionEvent;
  /* Status of the operation once it is completed. */
  NTSTATUS Status;
  /* TRUE if the operation was abandoned. */
  BOOLEAN Abandoned;
  /* The memory area to be affected by the operation. */
  PMEMORY_AREA MArea;
  ULONG Hash;
  struct _MM_PAGEOP* Next;
  struct _ETHREAD* Thread;
  /*
   * These fields are used to identify the operation if it is against a
   * virtual memory area.
   */
  HANDLE Pid;
  PVOID Address;
  /*
   * These fields are used to identify the operation if it is against a
   * section mapping.
   */
  PMM_SECTION_SEGMENT Segment;
  ULONG Offset;
} MM_PAGEOP, *PMM_PAGEOP;

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
    ULONG Length;
    LIST_ENTRY RegionListEntry;
} MM_REGION, *PMM_REGION;

/* Entry describing free pool memory */
typedef struct _MMFREE_POOL_ENTRY
{
    LIST_ENTRY List;
    PFN_NUMBER Size;
    ULONG Signature;
    struct _MMFREE_POOL_ENTRY *Owner;
} MMFREE_POOL_ENTRY, *PMMFREE_POOL_ENTRY;

/* Paged pool information */
typedef struct _MM_PAGED_POOL_INFO
{
    PRTL_BITMAP PagedPoolAllocationMap;
    PRTL_BITMAP EndOfPagedPoolBitmap;
    PMMPTE FirstPteForPagedPool;
    PMMPTE LastPteForPagedPool;
    PMMPTE NextPdeForPagedPoolExpansion;
    ULONG PagedPoolHint;
    SIZE_T PagedPoolCommit;
    SIZE_T AllocatedPagedPool;
} MM_PAGED_POOL_INFO, *PMM_PAGED_POOL_INFO;

typedef struct
{
   PROS_SECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   LARGE_INTEGER Offset;
   BOOLEAN WasDirty;
   BOOLEAN Private;
}
MM_SECTION_PAGEOUT_CONTEXT;

extern MM_MEMORY_CONSUMER MiMemoryConsumers[MC_MAXIMUM];

typedef VOID
(*PMM_ALTER_REGION_FUNC)(
    PMMSUPPORT AddressSpace,
    PVOID BaseAddress,
    ULONG Length,
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
    PFN_TYPE Page,
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

/* marea.c *******************************************************************/

NTSTATUS
NTAPI
MmCreateMemoryArea(
    PMMSUPPORT AddressSpace,
    ULONG Type,
    PVOID *BaseAddress,
    ULONG_PTR Length,
    ULONG Protection,
    PMEMORY_AREA *Result,
    BOOLEAN FixedAddress,
    ULONG AllocationFlags,
    PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL
);

PMEMORY_AREA
NTAPI
MmLocateMemoryAreaByAddress(
    PMMSUPPORT AddressSpace,
    PVOID Address
);

ULONG_PTR
NTAPI
MmFindGapAtAddress(
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

NTSTATUS
NTAPI
MmFreeMemoryAreaByPtr(
    PMMSUPPORT AddressSpace,
    PVOID BaseAddress,
    PMM_FREE_PAGE_FUNC FreePage,
    PVOID FreePageContext
);

VOID
NTAPI
MmDumpMemoryAreas(PMMSUPPORT AddressSpace);

PMEMORY_AREA
NTAPI
MmLocateMemoryAreaByRegion(
    PMMSUPPORT AddressSpace,
    PVOID Address,
    ULONG_PTR Length
);

PVOID
NTAPI
MmFindGap(
    PMMSUPPORT AddressSpace,
    ULONG_PTR Length,
    ULONG_PTR Granularity,
    BOOLEAN TopDown
);

VOID
NTAPI
MmReleaseMemoryAreaIfDecommitted(
    struct _EPROCESS *Process,
    PMMSUPPORT AddressSpace,
    PVOID BaseAddress
);

VOID
NTAPI
MmMapMemoryArea(PVOID BaseAddress,
                ULONG Length,
                ULONG Consumer,
                ULONG Protection);

/* npool.c *******************************************************************/

VOID
NTAPI
MiDebugDumpNonPagedPool(BOOLEAN NewOnly);

VOID
NTAPI
MiDebugDumpNonPagedPoolStats(BOOLEAN NewOnly);

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

PVOID
NTAPI
MmGetMdlPageAddress(
    PMDL Mdl,
    PVOID Offset
);

/* pool.c *******************************************************************/

PVOID
NTAPI
ExAllocateNonPagedPoolWithTag(
    POOL_TYPE type,
    ULONG size,
    ULONG Tag,
    PVOID Caller
);

PVOID
NTAPI
ExAllocatePagedPoolWithTag(
    POOL_TYPE Type,
    ULONG size,
    ULONG Tag
);

VOID
NTAPI
ExFreeNonPagedPool(PVOID block);

VOID
NTAPI
ExFreePagedPool(IN PVOID Block);

BOOLEAN
NTAPI
ExpIsPoolTagDebuggable(ULONG Tag);

PVOID
NTAPI
ExpAllocateDebugPool(
    POOL_TYPE Type,
    ULONG Size,
    ULONG Tag,
    PVOID Caller,
    BOOLEAN EndOfPage
);

VOID
NTAPI
ExpFreeDebugPool(PVOID Block, BOOLEAN PagedPool);

VOID
NTAPI
MmInitializePagedPool(VOID);

PVOID
NTAPI
MiAllocateSpecialPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN ULONG Underrun
);

BOOLEAN
NTAPI
MiRaisePoolQuota(
    IN POOL_TYPE PoolType,
    IN ULONG CurrentMaxQuota,
    OUT PULONG NewMaxQuota
);

/* mdl.c *********************************************************************/

VOID
NTAPI
MmBuildMdlFromPages(
    PMDL Mdl,
    PULONG Pages
);

/* mminit.c ******************************************************************/

VOID
NTAPI
MiShutdownMemoryManager(VOID);

VOID
NTAPI
MmInit1(
    VOID
);

BOOLEAN
NTAPI
MmInitSystem(IN ULONG Phase,
             IN PLOADER_PARAMETER_BLOCK LoaderBlock);

VOID
NTAPI
MiFreeInitMemory(VOID);

VOID
NTAPI
MmInitializeMdlImplementation(VOID);

/* pagefile.c ****************************************************************/

BOOLEAN
NTAPI
MmIsFileAPagingFile(PFILE_OBJECT FileObject);

SWAPENTRY
NTAPI
MmAllocSwapPage(VOID);

VOID
NTAPI
MmDereserveSwapPages(ULONG Nr);

VOID
NTAPI
MmFreeSwapPage(SWAPENTRY Entry);

VOID
NTAPI
MmInitPagingFile(VOID);

NTSTATUS
NTAPI
MmReadFromSwapPage(
    SWAPENTRY SwapEntry,
    PFN_TYPE Page
);

BOOLEAN
NTAPI
MmReserveSwapPages(ULONG Nr);

NTSTATUS
NTAPI
MmWriteToSwapPage(
    SWAPENTRY SwapEntry,
    PFN_TYPE Page
);

NTSTATUS
NTAPI
MmDumpToPagingFile(
    ULONG BugCode,
    ULONG BugCodeParameter1,
    ULONG BugCodeParameter2,
    ULONG BugCodeParameter3,
    ULONG BugCodeParameter4,
    struct _KTRAP_FRAME* TrapFrame
);

BOOLEAN
NTAPI
MmIsAvailableSwapPage(VOID);

VOID
NTAPI
MmShowOutOfSpaceMessagePagingFile(VOID);

PFN_TYPE
NTAPI
MmWithdrawSectionPage
(PMM_SECTION_SEGMENT Segment, PLARGE_INTEGER FileOffset, BOOLEAN *Dirty);

NTSTATUS
NTAPI
MmFinalizeSectionPageOut
(PMM_SECTION_SEGMENT Segment, PLARGE_INTEGER FileOffset, PFN_TYPE Page,
 BOOLEAN Dirty);

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

NTSTATUS
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

/* mm.c **********************************************************************/

NTSTATUS
NTAPI
MmAccessFault(
    IN BOOLEAN StoreInstruction,
    IN PVOID Address,
    IN KPROCESSOR_MODE Mode,
    IN PVOID TrapInformation
);

/* anonmem.c *****************************************************************/

NTSTATUS
NTAPI
MmNotPresentFaultVirtualMemory(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    BOOLEAN Locked,
	PMM_REQUIRED_RESOURCES Required
);

NTSTATUS
NTAPI
MmPageOutVirtualMemory(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    PVOID Address,
	PMM_REQUIRED_RESOURCES Required
);

NTSTATUS
NTAPI
MmQueryAnonMem(
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    PMEMORY_BASIC_INFORMATION Info,
    PULONG ResultLength
);

VOID
NTAPI
MmFreeVirtualMemory(
    struct _EPROCESS* Process,
    PMEMORY_AREA MemoryArea
);

NTSTATUS
NTAPI
MmProtectAnonMem(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    PVOID BaseAddress,
    ULONG Length,
    ULONG Protect,
    PULONG OldProtect
);

NTSTATUS
NTAPI
MmWritePageVirtualMemory(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MArea,
    PVOID Address
);

/* kmap.c ********************************************************************/

PVOID
NTAPI
ExAllocatePage(VOID);

VOID
NTAPI
ExUnmapPage(PVOID Addr);

PVOID
NTAPI
ExAllocatePageWithPhysPage(PFN_TYPE Page);

NTSTATUS
NTAPI
MiCopyFromUserPage(
    PFN_TYPE Page,
    PVOID SourceAddress
);

NTSTATUS
NTAPI
MiZeroPage(PFN_TYPE Page);

/* memsafe.s *****************************************************************/

PVOID
FASTCALL
MmSafeReadPtr(PVOID Source);

/* pageop.c ******************************************************************/

VOID
NTAPI
MmReleasePageOp(PMM_PAGEOP PageOp);

PMM_PAGEOP
NTAPI
MmGetPageOp(
    PMEMORY_AREA MArea,
    HANDLE Pid,
    PVOID Address,
    PMM_SECTION_SEGMENT Segment,
    ULONG Offset,
    ULONG OpType,
    BOOLEAN First
);

PMM_PAGEOP
NTAPI
MmCheckForPageOp(
    PMEMORY_AREA MArea,
    HANDLE Pid,
    PVOID Address,
    PMM_SECTION_SEGMENT Segment,
    ULONG Offset
);

VOID
NTAPI
MmInitializePageOp(VOID);

/* process.c *****************************************************************/

PVOID
NTAPI
MmCreateKernelStack(BOOLEAN GuiStack, UCHAR Node);

VOID
NTAPI
MmDeleteKernelStack(PVOID Stack,
                    BOOLEAN GuiStack);

/* sptab.c *******************************************************************/

VOID
NTAPI
MiInitializeSectionPageTable(PMM_SECTION_SEGMENT Segment);

NTSTATUS
NTAPI
MiSetPageEntrySectionSegment
(PMM_SECTION_SEGMENT Segment,
 PLARGE_INTEGER Offset,
 ULONG Entry);

ULONG
NTAPI
MiGetPageEntrySectionSegment
(PMM_SECTION_SEGMENT Segment,
 PLARGE_INTEGER Offset);

typedef VOID (NTAPI *FREE_SECTION_PAGE_FUN)
	(PMM_SECTION_SEGMENT Segment,
	 PLARGE_INTEGER Offset);

VOID
NTAPI
MiFreePageTablesSectionSegment(PMM_SECTION_SEGMENT Segment, FREE_SECTION_PAGE_FUN FreePage);

/* Yields a lock */
PMM_SECTION_SEGMENT
NTAPI
MmGetSectionAssociation(PFN_TYPE Page, PLARGE_INTEGER Offset);

NTSTATUS
NTAPI
MmSetSectionAssociation(PFN_TYPE Page, PMM_SECTION_SEGMENT Segment, PLARGE_INTEGER Offset);

VOID
NTAPI
MmDeleteSectionAssociation(PFN_TYPE Page);

/* balance.c ******************************************************************/

VOID
NTAPI
MmInitializeMemoryConsumer(
    ULONG Consumer,
    NTSTATUS (*Trim)(ULONG Target, ULONG Priority, PULONG NrFreed)
);

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
    PFN_TYPE Page
);

NTSTATUS
NTAPI
MmRequestPageMemoryConsumer(
    ULONG Consumer,
    BOOLEAN MyWait,
    PPFN_TYPE AllocatedPage
);

VOID
NTAPI
MiInitBalancerThread(VOID);

VOID
NTAPI
MmRebalanceMemoryConsumers(VOID);

/* rmap.c **************************************************************/

VOID
NTAPI
MmSetRmapListHeadPage(
    PFN_TYPE Page,
    struct _MM_RMAP_ENTRY* ListHead
);

struct _MM_RMAP_ENTRY*
NTAPI
MmGetRmapListHeadPage(PFN_TYPE Page);

VOID
NTAPI
MmInsertRmap(
    PFN_TYPE Page,
    struct _EPROCESS *Process,
    PVOID Address
);

VOID
NTAPI
MmDeleteAllRmaps(
    PFN_TYPE Page,
    PVOID Context,
    VOID (*DeleteMapping)(PVOID Context, struct _EPROCESS *Process, PVOID Address)
);

VOID
NTAPI
MmDeleteRmap(
    PFN_TYPE Page,
    struct _EPROCESS *Process,
    PVOID Address
);

VOID
NTAPI
MmInitializeRmapList(VOID);

VOID
NTAPI
MmSetCleanAllRmaps(PFN_TYPE Page);

VOID
NTAPI
MmSetDirtyAllRmaps(PFN_TYPE Page);

BOOLEAN
NTAPI
MmIsDirtyPageRmap(PFN_TYPE Page);

NTSTATUS
NTAPI
MmWritePagePhysicalAddress(PFN_TYPE Page);

NTSTATUS
NTAPI
MmPageOutPhysicalAddress(PFN_TYPE Page);

/* freelist.c **********************************************************/

#define ASSERT_PFN(x) ASSERT((x)->u3.e1.CacheAttribute != 0)

FORCEINLINE
PMMPFN
MiGetPfnEntry(IN PFN_TYPE Pfn)
{
    PMMPFN Page;
    extern RTL_BITMAP MiPfnBitMap;

    /* Make sure the PFN number is valid */
    if (Pfn > MmHighestPhysicalPage) return NULL;
    
    /* Make sure this page actually has a PFN entry */
    if ((MiPfnBitMap.Buffer) && !(RtlTestBit(&MiPfnBitMap, Pfn))) return NULL;

    /* Get the entry */
    Page = &MmPfnDatabase[Pfn];

    /* Make sure it's valid */
    ASSERT_PFN(Page);

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

ULONG
NTAPI
MmGetPageConsumer(PFN_TYPE Page);

PFN_TYPE
NTAPI
MmGetLRUNextUserPage(PFN_TYPE PreviousPage);

PFN_TYPE
NTAPI
MmGetLRUFirstUserPage(VOID);

VOID
NTAPI
MmInsertLRULastUserPage(PFN_TYPE Page);

VOID
NTAPI
MmRemoveLRUUserPage(PFN_TYPE Page);

VOID
NTAPI
MmLockPage(PFN_TYPE Page);

VOID
NTAPI
MmLockPageUnsafe(PFN_TYPE Page);

VOID
NTAPI
MmUnlockPage(PFN_TYPE Page);

ULONG
NTAPI
MmGetLockCountPage(PFN_TYPE Page);

static
__inline
KIRQL
NTAPI
MmAcquirePageListLock()
{
	return KeAcquireQueuedSpinLock(LockQueuePfnLock);
}

FORCEINLINE
VOID
NTAPI
MmReleasePageListLock(KIRQL oldIrql)
{
	KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
}

VOID
NTAPI
MmInitializePageList(
    VOID
);

VOID
NTAPI
MmDumpPfnDatabase(
   VOID
);

PFN_TYPE
NTAPI
MmGetContinuousPages(
    ULONG NumberOfBytes,
    PHYSICAL_ADDRESS LowestAcceptableAddress,
    PHYSICAL_ADDRESS HighestAcceptableAddress,
    PHYSICAL_ADDRESS BoundaryAddressMultiple,
    BOOLEAN ZeroPages
);

NTSTATUS
NTAPI
MmZeroPageThreadMain(
    PVOID Context
);

/* hypermap.c *****************************************************************/

extern PEPROCESS HyperProcess;
extern KIRQL HyperIrql;

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
MiMapPagesToZeroInHyperSpace(IN PMMPFN *Pages,
                             IN PFN_NUMBER NumberOfPages);

VOID
NTAPI
MiUnmapPagesInZeroSpace(IN PVOID VirtualAddress,
                        IN PFN_NUMBER NumberOfPages);

//
// ReactOS Compatibility Layer
//
FORCEINLINE
PVOID
MmCreateHyperspaceMapping(IN PFN_NUMBER Page)
{
    HyperProcess = (PEPROCESS)KeGetCurrentThread()->ApcState.Process;
    return MiMapPageInHyperSpace(HyperProcess, Page, &HyperIrql);
}

FORCEINLINE
PVOID
MiMapPageToZeroInHyperSpace(IN PFN_NUMBER Page)
{
    PMMPFN Pfn1 = MiGetPfnEntry(Page);
    return MiMapPagesToZeroInHyperSpace(&Pfn1, 1);
}

#define MmDeleteHyperspaceMapping(x) MiUnmapPageInHyperSpace(HyperProcess, x, HyperIrql);

/* virtual.c ***********************************************************/

NTSTATUS NTAPI
MiProtectVirtualMemory(IN PEPROCESS Process,
                       IN OUT PVOID *BaseAddress,
                       IN OUT PSIZE_T NumberOfBytesToProtect,
                       IN ULONG NewAccessProtection,
                       OUT PULONG OldAccessProtection  OPTIONAL);

/* i386/page.c *********************************************************/

NTSTATUS
NTAPI
MmCreateVirtualMappingForKernel(
    PVOID Address,
    ULONG flProtect,
    PPFN_TYPE Pages,
    ULONG PageCount
);

NTSTATUS
NTAPI
MmCommitPagedPoolAddress(
	PMMSUPPORT AddressSpace,
	PMEMORY_AREA MemoryArea,
    PVOID Address,
	BOOLEAN Locked,
	PMM_REQUIRED_RESOURCES Required);

NTSTATUS
NTAPI
MmCreateVirtualMapping(
    struct _EPROCESS* Process,
    PVOID Address,
    ULONG flProtect,
    PPFN_TYPE Pages,
    ULONG PageCount
);

NTSTATUS
NTAPI
MmCreateVirtualMappingUnsafe(
    struct _EPROCESS* Process,
    PVOID Address,
    ULONG flProtect,
    PPFN_TYPE Pages,
    ULONG PageCount
);

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

VOID
NTAPI
MmInitGlobalKernelPageDirectory(VOID);

VOID
NTAPI
MmDisableVirtualMapping(
    struct _EPROCESS *Process,
    PVOID Address,
    BOOLEAN* WasDirty,
    PPFN_TYPE Page
);

VOID
NTAPI
MmEnableVirtualMapping(
    struct _EPROCESS *Process,
    PVOID Address
);

VOID
NTAPI
MmRawDeleteVirtualMapping(PVOID Address);

VOID
NTAPI
MmDeletePageFileMapping(
    struct _EPROCESS *Process,
    PVOID Address,
    SWAPENTRY* SwapEntry
);

VOID
NTAPI
MmGetPageFileMapping(
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

BOOLEAN
NTAPI
MmIsPageSwapEntry(
    struct _EPROCESS *Process,
    PVOID Address
);

VOID
NTAPI
MmTransferOwnershipPage(
    PFN_TYPE Page,
    ULONG NewConsumer
);

VOID
NTAPI
MmSetDirtyPage(
    struct _EPROCESS *Process,
    PVOID Address
);

PFN_TYPE
NTAPI
MmAllocPage(
    ULONG Consumer,
    SWAPENTRY SavedSwapEntry
);

LONG
NTAPI
MmAllocPagesSpecifyRange(
    ULONG Consumer,
    PHYSICAL_ADDRESS LowestAddress,
    PHYSICAL_ADDRESS HighestAddress,
    ULONG NumberOfPages,
    PPFN_TYPE Pages
);

VOID
NTAPI
MmDereferencePage(PFN_TYPE Page);

#define MmDereferencePage(Page) _MmDereferencePage(Page,__FILE__,__LINE__)
#define MmReferencePage(Page) _MmReferencePage(Page,__FILE__,__LINE__)

VOID
NTAPI
_MmDereferencePage(PFN_TYPE Page, const char *file, int line);

VOID
NTAPI
_MmReferencePage(PFN_TYPE Page, const char *file, int line);

ULONG
NTAPI
MmGetReferenceCountPage(PFN_TYPE Page);

BOOLEAN
NTAPI
MmIsPageInUse(PFN_TYPE Page);

VOID
NTAPI
MmSetSavedSwapEntryPage(
    PFN_TYPE Page,
    SWAPENTRY SavedSwapEntry);

SWAPENTRY
NTAPI
MmGetSavedSwapEntryPage(PFN_TYPE Page);

VOID
NTAPI
MmSetCleanPage(
    struct _EPROCESS *Process,
    PVOID Address
);

NTSTATUS
NTAPI
MmCreatePageTable(PVOID PAddress);

VOID
NTAPI
MmDeletePageTable(
    struct _EPROCESS *Process,
    PVOID Address
);

PFN_TYPE
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
    IN PULONG DirectoryTableBase
);

NTSTATUS
NTAPI
MmInitializeHandBuiltProcess(
    IN PEPROCESS Process,
    IN PULONG DirectoryTableBase
);


NTSTATUS
NTAPI
MmInitializeHandBuiltProcess2(
    IN PEPROCESS Process
);

NTSTATUS
NTAPI
MmReleaseMmInfo(struct _EPROCESS *Process);

NTSTATUS
NTAPI
Mmi386ReleaseMmInfo(struct _EPROCESS *Process);

VOID
NTAPI
MmDeleteVirtualMapping(
    struct _EPROCESS *Process,
    PVOID Address,
    BOOLEAN FreePage,
    BOOLEAN* WasDirty,
    PPFN_TYPE Page
);

BOOLEAN
NTAPI
MmIsDirtyPage(
    struct _EPROCESS *Process,
    PVOID Address
);

VOID
NTAPI
MmMarkPageMapped(PFN_TYPE Page);

VOID
NTAPI
MmMarkPageUnmapped(PFN_TYPE Page);

VOID
NTAPI
MmUpdatePageDir(
    struct _EPROCESS *Process,
    PVOID Address,
    ULONG Size
);

VOID
NTAPI
MiInitPageDirectoryMap(VOID);

ULONG
NTAPI
MiGetUserPageDirectoryCount(VOID);

/* io.c **********************************************************************/

NTSTATUS
MmspWaitForFileLock(PFILE_OBJECT File);

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
 PIO_STATUS_BLOCK ReadStatus);

NTSTATUS
NTAPI
MiWriteBackPage
(PFILE_OBJECT FileObject,
 PLARGE_INTEGER Offset,
 ULONG Length,
 PFN_TYPE Page);

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
    ULONG Length,
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

NTSTATUS
NTAPI
MiReadFilePage
(PMMSUPPORT AddressSpace, 
 PMEMORY_AREA MemoryArea, 
 PMM_REQUIRED_RESOURCES RequiredResources);

NTSTATUS
NTAPI
MiGetOnePage
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PMM_REQUIRED_RESOURCES RequiredResources);

NTSTATUS
NTAPI
MiSwapInPage
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PMM_REQUIRED_RESOURCES RequiredResources);

NTSTATUS
NTAPI
MiWriteSwapPage
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PMM_REQUIRED_RESOURCES Resources);

NTSTATUS
NTAPI
MiWriteFilePage
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PMM_REQUIRED_RESOURCES Resources);

VOID
NTAPI
MiFreeSegmentPage
(PMM_SECTION_SEGMENT Segment,
 PLARGE_INTEGER FileOffset);

VOID
NTAPI
MiFreeDataSectionSegment(PMM_SECTION_SEGMENT Segment);

NTSTATUS
NTAPI
MiCowSectionPage
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PVOID Address,
 BOOLEAN Locked,
 PMM_REQUIRED_RESOURCES Required);

NTSTATUS
NTAPI
MiZeroFillSection(PVOID Address, PLARGE_INTEGER FileOffsetPtr, ULONG Length);

VOID
MmPageOutDeleteMapping(PVOID Context, PEPROCESS Process, PVOID Address);

VOID
NTAPI
MmSharePageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
                               PLARGE_INTEGER Offset);

BOOLEAN
NTAPI
MmUnsharePageEntrySectionSegment(PROS_SECTION_OBJECT Section,
                                 PMM_SECTION_SEGMENT Segment,
								 PLARGE_INTEGER Offset);

VOID
NTAPI
MmSetPageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
                             ULONG Offset,
                             ULONG Entry);

NTSTATUS
MmspWaitForPageOpCompletionEvent(PMM_PAGEOP PageOp);

VOID
NTAPI
_MmLockSectionSegment(PMM_SECTION_SEGMENT Segment, const char *file, int line);

#define MmLockSectionSegment(x) _MmLockSectionSegment(x,__FILE__,__LINE__)

VOID
NTAPI
_MmUnlockSectionSegment(PMM_SECTION_SEGMENT Segment, const char *file, int line);

#define MmUnlockSectionSegment(x) _MmUnlockSectionSegment(x,__FILE__,__LINE__)

VOID
MmFreeSectionPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address,
                  PFN_TYPE Page, SWAPENTRY SwapEntry, BOOLEAN Dirty);

PFILE_OBJECT
NTAPI
MmGetFileObjectForSection(
    IN PROS_SECTION_OBJECT Section
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
    IN PROS_SECTION_OBJECT Section,
    OUT POBJECT_NAME_INFORMATION *ModuleName
);

NTSTATUS
NTAPI
MiFlushMappedSection(PVOID BaseAddress, PLARGE_INTEGER FileSize);

NTSTATUS
NTAPI
MmExtendSection(PROS_SECTION_OBJECT Section, PLARGE_INTEGER NewSize, BOOLEAN ExtendFile);

PVOID
NTAPI
MmAllocateSection(
    IN ULONG Length,
    PVOID BaseAddress
);

NTSTATUS
NTAPI
MmQuerySectionView(
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    PMEMORY_BASIC_INFORMATION Info,
    PULONG ResultLength
);

NTSTATUS
NTAPI
MmProtectSectionView(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    PVOID BaseAddress,
    ULONG Length,
    ULONG Protect,
    PULONG OldProtect
);

NTSTATUS
NTAPI
MmWritePageSectionView(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MArea,
    PVOID Address
);

NTSTATUS
NTAPI
MmInitSectionImplementation(VOID);

VOID
NTAPI
MmFreeSectionSegments(PFILE_OBJECT FileObject);

NTSTATUS NTAPI
MmMapViewInSystemSpaceAtOffset
(IN PVOID SectionObject,
 OUT PVOID * MappedBase,
 IN PLARGE_INTEGER ViewOffset,
 IN OUT PULONG ViewSize);

NTSTATUS
NTAPI
MiMapViewOfSegment(PMMSUPPORT AddressSpace,
                   PROS_SECTION_OBJECT Section,
                   PMM_SECTION_SEGMENT Segment,
                   PVOID* BaseAddress,
                   SIZE_T ViewSize,
                   ULONG Protect,
                   PLARGE_INTEGER ViewOffset,
                   ULONG AllocationType);

/* section/image.c ***********************************************************/

NTSTATUS
NTAPI
MiMapImageFileSection
(PMMSUPPORT AddressSpace,
 PROS_SECTION_OBJECT Section,
 PVOID *BaseAddress);

NTSTATUS
MmCreateImageSection
(PROS_SECTION_OBJECT *SectionObject,
 ACCESS_MASK DesiredAccess,
 POBJECT_ATTRIBUTES ObjectAttributes,
 PLARGE_INTEGER UMaximumSize,
 ULONG SectionPageProtection,
 ULONG AllocationAttributes,
 PFILE_OBJECT FileObject);

NTSTATUS
NTAPI
MmUnmapViewOfSegment(PMMSUPPORT AddressSpace,
                     PVOID BaseAddress);

NTSTATUS
NTAPI
MiUnmapImageSection
(PMMSUPPORT AddressSpace, PMEMORY_AREA MemoryArea, PVOID BaseAddress);

VOID
NTAPI
MiDeleteImageSection(PROS_SECTION_OBJECT Section);

/* section/physical.c ********************************************************/

NTSTATUS
NTAPI
MmNotPresentFaultPhysicalMemory
(PMMSUPPORT AddressSpace,
 MEMORY_AREA* MemoryArea,
 PVOID Address,
 BOOLEAN Locked,
 PMM_REQUIRED_RESOURCES Required);

NTSTATUS
NTAPI
MmAccessFaultPhysicalMemory
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PVOID Address,
 BOOLEAN Locked,
 PMM_REQUIRED_RESOURCES Required);

NTSTATUS
NTAPI
MmCreatePhysicalMemorySection();

VOID
NTAPI
MmUnmapPhysicalMemorySegment
(PMMSUPPORT AddressSpace, 
 PMEMORY_AREA MemoryArea,
 PROS_SECTION_OBJECT Section,
 PMM_SECTION_SEGMENT Segment);

/* section/pagefile.c ********************************************************/

NTSTATUS
NTAPI
MmNotPresentFaultImageFile(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    PVOID Address,
    BOOLEAN Locked,
	PMM_REQUIRED_RESOURCES Required
);

NTSTATUS
NTAPI
MmNotPresentFaultPageFile
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PVOID Address,
 BOOLEAN Locked,
 PMM_REQUIRED_RESOURCES Required);

NTSTATUS
NTAPI
MmCreatePageFileSection
(PROS_SECTION_OBJECT *SectionObject,
 ACCESS_MASK DesiredAccess,
 POBJECT_ATTRIBUTES ObjectAttributes,
 PLARGE_INTEGER UMaximumSize,
 ULONG SectionPageProtection,
 ULONG AllocationAttributes);

NTSTATUS
NTAPI
MmPageOutPageFileView
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PVOID Address,
 PMM_REQUIRED_RESOURCES Required);

VOID
NTAPI
MmUnmapPageFileSegment
(PMMSUPPORT AddressSpace, 
 PMEMORY_AREA MemoryArea,
 PROS_SECTION_OBJECT Section,
 PMM_SECTION_SEGMENT Segment);

/* mpw.c *********************************************************************/

NTSTATUS
NTAPI
MmInitMpwThread(VOID);

NTSTATUS
NTAPI
MmInitBsmThread(VOID);

/* pager.c *******************************************************************/

BOOLEAN
NTAPI
MiIsPagerThread(VOID);

VOID
NTAPI
MiStartPagerThread(VOID);

VOID
NTAPI
MiStopPagerThread(VOID);

NTSTATUS
FASTCALL
MiQueryVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID Address,
    IN MEMORY_INFORMATION_CLASS VirtualMemoryInformationClass,
    OUT PVOID VirtualMemoryInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

/* sysldr.c ******************************************************************/

VOID
NTAPI
MiReloadBootLoadedDrivers(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

BOOLEAN
NTAPI
MiInitializeLoadedModuleList(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

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
    IN PLDR_DATA_TABLE_ENTRY LdrEntry,
    IN PLIST_ENTRY ListHead
);

/* ReactOS Mm Hacks */
VOID
FASTCALL
MiSyncForProcessAttach(
    IN PKTHREAD NextThread,
    IN PEPROCESS Process
);

VOID
FASTCALL
MiSyncForContextSwitch(
    IN PKTHREAD Thread
);

extern PMMSUPPORT MmKernelAddressSpace;

FORCEINLINE
VOID
_MmLockAddressSpace(PMMSUPPORT AddressSpace, const char *file, int line)
{
	//DbgPrint("(%s:%d) Lock Address Space %x\n", file, line, AddressSpace);
    KeAcquireGuardedMutex(&CONTAINING_RECORD(AddressSpace, EPROCESS, Vm)->AddressCreationLock);
}

#define MmLockAddressSpace(x) _MmLockAddressSpace(x,__FILE__,__LINE__)

FORCEINLINE
VOID
_MmUnlockAddressSpace(PMMSUPPORT AddressSpace, const char *file, int line)
{
	//DbgPrint("(%s:%d) Unlock Address Space %x\n", file, line, AddressSpace);
    KeReleaseGuardedMutex(&CONTAINING_RECORD(AddressSpace, EPROCESS, Vm)->AddressCreationLock);
}

#define MmUnlockAddressSpace(x) _MmUnlockAddressSpace(x,__FILE__,__LINE__)

FORCEINLINE
BOOLEAN
_MmTryToLockAddressSpace(IN PMMSUPPORT AddressSpace, const char *file, int line)
{
	BOOLEAN Result = KeTryToAcquireGuardedMutex(&CONTAINING_RECORD(AddressSpace, EPROCESS, Vm)->AddressCreationLock);
	DbgPrint("(%s:%d) Try Lock Address Space %x -> %s\n", file, line, AddressSpace, Result ? "true" : "false");
	return Result;
}

#define MmTryToLockAddressSpace(x) _MmTryToLockAddressSpace(x,__FILE__,__LINE__)

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

NTSTATUS
NTAPI
MiWidenSegment
(PMMSUPPORT AddressSpace, 
 PMEMORY_AREA MemoryArea, 
 PMM_REQUIRED_RESOURCES RequiredResources);

NTSTATUS
NTAPI
MiSwapInSectionPage
(PMMSUPPORT AddressSpace, 
 PMEMORY_AREA MemoryArea, 
 PMM_REQUIRED_RESOURCES RequiredResources);

#endif
