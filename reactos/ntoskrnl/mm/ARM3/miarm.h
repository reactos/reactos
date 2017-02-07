/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/miarm.h
 * PURPOSE:         ARM Memory Manager Header
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#ifndef _M_AMD64

#define MI_MIN_PAGES_FOR_NONPAGED_POOL_TUNING   ((255 * _1MB) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_TUNING          ((19 * _1MB) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_BOOST           ((32 * _1MB) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_BOOST_BOOST     ((256 * _1MB) >> PAGE_SHIFT)
#define MI_MAX_INIT_NONPAGED_POOL_SIZE          (128 * _1MB)
#define MI_MAX_NONPAGED_POOL_SIZE               (128 * _1MB)
#define MI_MAX_FREE_PAGE_LISTS                  4

#define MI_MIN_INIT_PAGED_POOLSIZE              (32 * _1MB)

#define MI_SESSION_VIEW_SIZE                    (20 * _1MB)
#define MI_SESSION_POOL_SIZE                    (16 * _1MB)
#define MI_SESSION_IMAGE_SIZE                   (8 * _1MB)
#define MI_SESSION_WORKING_SET_SIZE             (4 * _1MB)
#define MI_SESSION_SIZE                         (MI_SESSION_VIEW_SIZE + \
                                                 MI_SESSION_POOL_SIZE + \
                                                 MI_SESSION_IMAGE_SIZE + \
                                                 MI_SESSION_WORKING_SET_SIZE)

#define MI_SYSTEM_VIEW_SIZE                     (32 * _1MB)

#define MI_HIGHEST_USER_ADDRESS                 (PVOID)0x7FFEFFFF
#define MI_USER_PROBE_ADDRESS                   (PVOID)0x7FFF0000
#define MI_DEFAULT_SYSTEM_RANGE_START           (PVOID)0x80000000
#define MI_SYSTEM_CACHE_WS_START                (PVOID)0xC0C00000
#define MI_PAGED_POOL_START                     (PVOID)0xE1000000
#define MI_NONPAGED_POOL_END                    (PVOID)0xFFBE0000
#define MI_DEBUG_MAPPING                        (PVOID)0xFFBFF000

#define MI_SYSTEM_PTE_BASE                      (PVOID)MiAddressToPte(NULL)

#define MI_MIN_SECONDARY_COLORS                 8
#define MI_SECONDARY_COLORS                     64
#define MI_MAX_SECONDARY_COLORS                 1024

#define MI_MIN_ALLOCATION_FRAGMENT              (4 * _1KB)
#define MI_ALLOCATION_FRAGMENT                  (64 * _1KB)
#define MI_MAX_ALLOCATION_FRAGMENT              (2  * _1MB)

#define MM_HIGHEST_VAD_ADDRESS \
    (PVOID)((ULONG_PTR)MM_HIGHEST_USER_ADDRESS - (16 * PAGE_SIZE))
#define MI_LOWEST_VAD_ADDRESS                   (PVOID)MM_LOWEST_USER_ADDRESS

#define MI_DEFAULT_SYSTEM_PTE_COUNT             50000

#endif /* !_M_AMD64 */

/* Make the code cleaner with some definitions for size multiples */
#define _1KB (1024u)
#define _1MB (1024 * _1KB)
#define _1GB (1024 * _1MB)

/* Everyone loves 64K */
#define _64K (64 * _1KB)

/* Area mapped by a PDE */
#define PDE_MAPPED_VA  (PTE_COUNT * PAGE_SIZE)

/* Size of a page table */
#define PT_SIZE  (PTE_COUNT * sizeof(MMPTE))

/* Size of a page directory */
#define PD_SIZE  (PDE_COUNT * sizeof(MMPDE))

/* Size of all page directories for a process */
#define SYSTEM_PD_SIZE (PD_COUNT * PD_SIZE)

/* Architecture specific count of PDEs in a directory, and count of PTEs in a PT */
#ifdef _M_IX86
#define PD_COUNT  1
#define PDE_COUNT 1024
#define PTE_COUNT 1024
C_ASSERT(SYSTEM_PD_SIZE == PAGE_SIZE);
#define MiIsPteOnPdeBoundary(PointerPte) \
    ((((ULONG_PTR)PointerPte) & (PAGE_SIZE - 1)) == 0)
#elif _M_ARM
#define PD_COUNT  1
#define PDE_COUNT 4096
#define PTE_COUNT 256
#else
#define PD_COUNT  PPE_PER_PAGE
#define PDE_COUNT PDE_PER_PAGE
#define PTE_COUNT PTE_PER_PAGE
#endif

//
// Protection Bits part of the internal memory manager Protection Mask
// Taken from http://www.reactos.org/wiki/Techwiki:Memory_management_in_the_Windows_XP_kernel
// and public assertions.
//
#define MM_ZERO_ACCESS         0
#define MM_READONLY            1
#define MM_EXECUTE             2
#define MM_EXECUTE_READ        3
#define MM_READWRITE           4
#define MM_WRITECOPY           5
#define MM_EXECUTE_READWRITE   6
#define MM_EXECUTE_WRITECOPY   7
#define MM_NOCACHE             8
#define MM_DECOMMIT            0x10
#define MM_NOACCESS            (MM_DECOMMIT | MM_NOCACHE)
#define MM_INVALID_PROTECTION  0xFFFFFFFF

//
// Specific PTE Definitions that map to the Memory Manager's Protection Mask Bits
// The Memory Manager's definition define the attributes that must be preserved
// and these PTE definitions describe the attributes in the hardware sense. This
// helps deal with hardware differences between the actual boolean expression of
// the argument.
//
// For example, in the logical attributes, we want to express read-only as a flag
// but on x86, it is writability that must be set. On the other hand, on x86, just
// like in the kernel, it is disabling the caches that requires a special flag,
// while on certain architectures such as ARM, it is enabling the cache which
// requires a flag.
//
#if defined(_M_IX86) || defined(_M_AMD64)
//
// Access Flags
//
#define PTE_READONLY            0 // Doesn't exist on x86
#define PTE_EXECUTE             0 // Not worrying about NX yet
#define PTE_EXECUTE_READ        0 // Not worrying about NX yet
#define PTE_READWRITE           0x2
#define PTE_WRITECOPY           0x200
#define PTE_EXECUTE_READWRITE   0x2 // Not worrying about NX yet
#define PTE_EXECUTE_WRITECOPY   0x200
#define PTE_PROTOTYPE           0x400

//
// State Flags
//
#define PTE_VALID               0x1
#define PTE_ACCESSED            0x20
#define PTE_DIRTY               0x40

//
// Cache flags
//
#define PTE_ENABLE_CACHE        0
#define PTE_DISABLE_CACHE       0x10
#define PTE_WRITECOMBINED_CACHE 0x10
#elif defined(_M_ARM)
#define PTE_READONLY            0x200
#define PTE_EXECUTE             0 // Not worrying about NX yet
#define PTE_EXECUTE_READ        0 // Not worrying about NX yet
#define PTE_READWRITE           0 // Doesn't exist on ARM
#define PTE_WRITECOPY           0 // Doesn't exist on ARM
#define PTE_EXECUTE_READWRITE   0 // Not worrying about NX yet
#define PTE_EXECUTE_WRITECOPY   0 // Not worrying about NX yet
#define PTE_PROTOTYPE           0x400 // Using the Shared bit
//
// Cache flags
//
#define PTE_ENABLE_CACHE        0
#define PTE_DISABLE_CACHE       0x10
#define PTE_WRITECOMBINED_CACHE 0x10
#else
#error Define these please!
#endif

extern const ULONG_PTR MmProtectToPteMask[32];
extern const ULONG MmProtectToValue[32];

//
// Assertions for session images, addresses, and PTEs
//
#define MI_IS_SESSION_IMAGE_ADDRESS(Address) \
    (((Address) >= MiSessionImageStart) && ((Address) < MiSessionImageEnd))

#define MI_IS_SESSION_ADDRESS(Address) \
    (((Address) >= MmSessionBase) && ((Address) < MiSessionSpaceEnd))

#define MI_IS_SESSION_PTE(Pte) \
    ((((PMMPTE)Pte) >= MiSessionBasePte) && (((PMMPTE)Pte) < MiSessionLastPte))

#define MI_IS_PAGE_TABLE_ADDRESS(Address) \
    (((PVOID)(Address) >= (PVOID)PTE_BASE) && ((PVOID)(Address) <= (PVOID)PTE_TOP))

#define MI_IS_SYSTEM_PAGE_TABLE_ADDRESS(Address) \
    (((Address) >= (PVOID)MiAddressToPte(MmSystemRangeStart)) && ((Address) <= (PVOID)PTE_TOP))

#define MI_IS_PAGE_TABLE_OR_HYPER_ADDRESS(Address) \
    (((PVOID)(Address) >= (PVOID)PTE_BASE) && ((PVOID)(Address) <= (PVOID)MmHyperSpaceEnd))

//
// Corresponds to MMPTE_SOFTWARE.Protection
//
#ifdef _M_IX86
#define MM_PTE_SOFTWARE_PROTECTION_BITS   5
#elif _M_ARM
#define MM_PTE_SOFTWARE_PROTECTION_BITS   6
#elif _M_AMD64
#define MM_PTE_SOFTWARE_PROTECTION_BITS   5
#else
#error Define these please!
#endif

//
// Creates a software PTE with the given protection
//
#define MI_MAKE_SOFTWARE_PTE(p, x)          ((p)->u.Long = (x << MM_PTE_SOFTWARE_PROTECTION_BITS))

//
// Marks a PTE as deleted
//
#define MI_SET_PFN_DELETED(x)               ((x)->PteAddress = (PMMPTE)((ULONG_PTR)(x)->PteAddress | 1))
#define MI_IS_PFN_DELETED(x)                ((ULONG_PTR)((x)->PteAddress) & 1)

//
// Special values for LoadedImports
//
#define MM_SYSLDR_NO_IMPORTS   (PVOID)0xFFFFFFFE
#define MM_SYSLDR_BOOT_LOADED  (PVOID)0xFFFFFFFF
#define MM_SYSLDR_SINGLE_ENTRY 0x1

//
// Number of initial session IDs
//
#define MI_INITIAL_SESSION_IDS  64

#if defined(_M_IX86) || defined(_M_ARM)
//
// PFN List Sentinel
//
#define LIST_HEAD 0xFFFFFFFF

//
// Because GCC cannot automatically downcast 0xFFFFFFFF to lesser-width bits,
// we need a manual definition suited to the number of bits in the PteFrame.
// This is used as a LIST_HEAD for the colored list
//
#define COLORED_LIST_HEAD ((1 << 25) - 1) // 0x1FFFFFF
#elif defined(_M_AMD64)
#define LIST_HEAD 0xFFFFFFFFFFFFFFFFLL
#define COLORED_LIST_HEAD ((1ULL << 57) - 1) // 0x1FFFFFFFFFFFFFFLL
#else
#error Define these please!
#endif

//
// Special IRQL value (found in assertions)
//
#define MM_NOIRQL (KIRQL)0xFFFFFFFF

//
// Returns the color of a page
//
#define MI_GET_PAGE_COLOR(x)                ((x) & MmSecondaryColorMask)
#define MI_GET_NEXT_COLOR()                 (MI_GET_PAGE_COLOR(++MmSystemPageColor))
#define MI_GET_NEXT_PROCESS_COLOR(x)        (MI_GET_PAGE_COLOR(++(x)->NextPageColor))

#ifndef _M_AMD64
//
// Decodes a Prototype PTE into the underlying PTE
//
#define MiProtoPteToPte(x)                  \
    (PMMPTE)((ULONG_PTR)MmPagedPoolStart +  \
             (((x)->u.Proto.ProtoAddressHigh << 7) | (x)->u.Proto.ProtoAddressLow))
#endif

//
// Prototype PTEs that don't yet have a pagefile association
//
#ifdef _M_AMD64
#define MI_PTE_LOOKUP_NEEDED 0xffffffffULL
#else
#define MI_PTE_LOOKUP_NEEDED 0xFFFFF
#endif

//
// Number of session lists in the MM_SESSIONS_SPACE structure
//
#if defined(_M_AMD64)
#define SESSION_POOL_LOOKASIDES 21
#elif defined(_M_IX86)
#define SESSION_POOL_LOOKASIDES 26
#else
#error Not Defined!
#endif

//
// Number of session data and tag pages
//
#define MI_SESSION_DATA_PAGES_MAXIMUM (MM_ALLOCATION_GRANULARITY / PAGE_SIZE)
#define MI_SESSION_TAG_PAGES_MAXIMUM  (MM_ALLOCATION_GRANULARITY / PAGE_SIZE)


//
// System views are binned into 64K chunks
//
#define MI_SYSTEM_VIEW_BUCKET_SIZE  _64K

//
// FIXFIX: These should go in ex.h after the pool merge
//
#ifdef _M_AMD64
#define POOL_BLOCK_SIZE 16
#else
#define POOL_BLOCK_SIZE  8
#endif
#define POOL_LISTS_PER_PAGE (PAGE_SIZE / POOL_BLOCK_SIZE)
#define BASE_POOL_TYPE_MASK 1
#define POOL_MAX_ALLOC (PAGE_SIZE - (sizeof(POOL_HEADER) + POOL_BLOCK_SIZE))

//
// Pool debugging/analysis/tracing flags
//
#define POOL_FLAG_CHECK_TIMERS 0x1
#define POOL_FLAG_CHECK_WORKERS 0x2
#define POOL_FLAG_CHECK_RESOURCES 0x4
#define POOL_FLAG_VERIFIER 0x8
#define POOL_FLAG_CHECK_DEADLOCK 0x10
#define POOL_FLAG_SPECIAL_POOL 0x20
#define POOL_FLAG_DBGPRINT_ON_FAILURE 0x40
#define POOL_FLAG_CRASH_ON_FAILURE 0x80

//
// BAD_POOL_HEADER codes during pool bugcheck
//
#define POOL_CORRUPTED_LIST 3
#define POOL_SIZE_OR_INDEX_MISMATCH 5
#define POOL_ENTRIES_NOT_ALIGNED_PREVIOUS 6
#define POOL_HEADER_NOT_ALIGNED 7
#define POOL_HEADER_IS_ZERO 8
#define POOL_ENTRIES_NOT_ALIGNED_NEXT 9
#define POOL_ENTRY_NOT_FOUND 10

//
// BAD_POOL_CALLER codes during pool bugcheck
//
#define POOL_ENTRY_CORRUPTED 1
#define POOL_ENTRY_ALREADY_FREE 6
#define POOL_ENTRY_NOT_ALLOCATED 7
#define POOL_ALLOC_IRQL_INVALID 8
#define POOL_FREE_IRQL_INVALID 9
#define POOL_BILLED_PROCESS_INVALID 13
#define POOL_HEADER_SIZE_INVALID 32

typedef struct _POOL_DESCRIPTOR
{
    POOL_TYPE PoolType;
    ULONG PoolIndex;
    ULONG RunningAllocs;
    ULONG RunningDeAllocs;
    ULONG TotalPages;
    ULONG TotalBigPages;
    ULONG Threshold;
    PVOID LockAddress;
    PVOID PendingFrees;
    LONG PendingFreeDepth;
    SIZE_T TotalBytes;
    SIZE_T Spare0;
    LIST_ENTRY ListHeads[POOL_LISTS_PER_PAGE];
} POOL_DESCRIPTOR, *PPOOL_DESCRIPTOR;

typedef struct _POOL_HEADER
{
    union
    {
        struct
        {
#ifdef _M_AMD64
            USHORT PreviousSize:8;
            USHORT PoolIndex:8;
            USHORT BlockSize:8;
            USHORT PoolType:8;
#else
            USHORT PreviousSize:9;
            USHORT PoolIndex:7;
            USHORT BlockSize:9;
            USHORT PoolType:7;
#endif
        };
        ULONG Ulong1;
    };
#ifdef _M_AMD64
    ULONG PoolTag;
#endif
    union
    {
#ifdef _M_AMD64
        PEPROCESS ProcessBilled;
#else
        ULONG PoolTag;
#endif
        struct
        {
            USHORT AllocatorBackTraceIndex;
            USHORT PoolTagHash;
        };
    };
} POOL_HEADER, *PPOOL_HEADER;

C_ASSERT(sizeof(POOL_HEADER) == POOL_BLOCK_SIZE);
C_ASSERT(POOL_BLOCK_SIZE == sizeof(LIST_ENTRY));

typedef struct _POOL_TRACKER_TABLE
{
    ULONG Key;
    LONG NonPagedAllocs;
    LONG NonPagedFrees;
    SIZE_T NonPagedBytes;
    LONG PagedAllocs;
    LONG PagedFrees;
    SIZE_T PagedBytes;
} POOL_TRACKER_TABLE, *PPOOL_TRACKER_TABLE;

typedef struct _POOL_TRACKER_BIG_PAGES
{
    PVOID Va;
    ULONG Key;
    ULONG NumberOfPages;
    PVOID QuotaObject;
} POOL_TRACKER_BIG_PAGES, *PPOOL_TRACKER_BIG_PAGES;

extern ULONG ExpNumberOfPagedPools;
extern POOL_DESCRIPTOR NonPagedPoolDescriptor;
extern PPOOL_DESCRIPTOR ExpPagedPoolDescriptor[16 + 1];
extern PPOOL_TRACKER_TABLE PoolTrackTable;

//
// END FIXFIX
//

typedef struct _MI_LARGE_PAGE_DRIVER_ENTRY
{
    LIST_ENTRY Links;
    UNICODE_STRING BaseName;
} MI_LARGE_PAGE_DRIVER_ENTRY, *PMI_LARGE_PAGE_DRIVER_ENTRY;

typedef enum _MMSYSTEM_PTE_POOL_TYPE
{
    SystemPteSpace,
    NonPagedPoolExpansion,
    MaximumPtePoolTypes
} MMSYSTEM_PTE_POOL_TYPE;

typedef enum _MI_PFN_CACHE_ATTRIBUTE
{
    MiNonCached,
    MiCached,
    MiWriteCombined,
    MiNotMapped
} MI_PFN_CACHE_ATTRIBUTE, *PMI_PFN_CACHE_ATTRIBUTE;

typedef struct _PHYSICAL_MEMORY_RUN
{
    PFN_NUMBER BasePage;
    PFN_NUMBER PageCount;
} PHYSICAL_MEMORY_RUN, *PPHYSICAL_MEMORY_RUN;

typedef struct _PHYSICAL_MEMORY_DESCRIPTOR
{
    ULONG NumberOfRuns;
    PFN_NUMBER NumberOfPages;
    PHYSICAL_MEMORY_RUN Run[1];
} PHYSICAL_MEMORY_DESCRIPTOR, *PPHYSICAL_MEMORY_DESCRIPTOR;

typedef struct _MMCOLOR_TABLES
{
    PFN_NUMBER Flink;
    PVOID Blink;
    PFN_NUMBER Count;
} MMCOLOR_TABLES, *PMMCOLOR_TABLES;

typedef struct _MI_LARGE_PAGE_RANGES
{
    PFN_NUMBER StartFrame;
    PFN_NUMBER LastFrame;
} MI_LARGE_PAGE_RANGES, *PMI_LARGE_PAGE_RANGES;

typedef struct _MMVIEW
{
    ULONG_PTR Entry;
    PCONTROL_AREA ControlArea;
} MMVIEW, *PMMVIEW;

typedef struct _MMSESSION
{
    KGUARDED_MUTEX SystemSpaceViewLock;
    PKGUARDED_MUTEX SystemSpaceViewLockPointer;
    PCHAR SystemSpaceViewStart;
    PMMVIEW SystemSpaceViewTable;
    ULONG SystemSpaceHashSize;
    ULONG SystemSpaceHashEntries;
    ULONG SystemSpaceHashKey;
    ULONG BitmapFailures;
    PRTL_BITMAP SystemSpaceBitMap;
} MMSESSION, *PMMSESSION;

typedef struct _MM_SESSION_SPACE_FLAGS
{
    ULONG Initialized:1;
    ULONG DeletePending:1;
    ULONG Filler:30;
} MM_SESSION_SPACE_FLAGS;

typedef struct _MM_SESSION_SPACE
{
    struct _MM_SESSION_SPACE *GlobalVirtualAddress;
    LONG ReferenceCount;
    union
    {
        ULONG LongFlags;
        MM_SESSION_SPACE_FLAGS Flags;
    } u;
    ULONG SessionId;
    LIST_ENTRY ProcessList;
    LARGE_INTEGER LastProcessSwappedOutTime;
    PFN_NUMBER SessionPageDirectoryIndex;
    SIZE_T NonPageablePages;
    SIZE_T CommittedPages;
    PVOID PagedPoolStart;
    PVOID PagedPoolEnd;
    PMMPTE PagedPoolBasePde;
    ULONG Color;
    LONG ResidentProcessCount;
    ULONG SessionPoolAllocationFailures[4];
    LIST_ENTRY ImageList;
    LCID LocaleId;
    ULONG AttachCount;
    KEVENT AttachEvent;
    PEPROCESS LastProcess;
    LONG ProcessReferenceToSession;
    LIST_ENTRY WsListEntry;
    GENERAL_LOOKASIDE Lookaside[SESSION_POOL_LOOKASIDES];
    MMSESSION Session;
    KGUARDED_MUTEX PagedPoolMutex;
    MM_PAGED_POOL_INFO PagedPoolInfo;
    MMSUPPORT Vm;
    PMMWSLE Wsle;
    PDRIVER_UNLOAD Win32KDriverUnload;
    POOL_DESCRIPTOR PagedPool;
#if defined (_M_AMD64)
    MMPTE PageDirectory;
#else
    PMMPTE PageTables;
#endif
#if defined (_M_AMD64)
    PMMPTE SpecialPoolFirstPte;
    PMMPTE SpecialPoolLastPte;
    PMMPTE NextPdeForSpecialPoolExpansion;
    PMMPTE LastPdeForSpecialPoolExpansion;
    PFN_NUMBER SpecialPagesInUse;
#endif
    LONG ImageLoadingCount;
} MM_SESSION_SPACE, *PMM_SESSION_SPACE;

extern PMM_SESSION_SPACE MmSessionSpace;
extern MMPTE HyperTemplatePte;
extern MMPDE ValidKernelPde;
extern MMPTE ValidKernelPte;
extern MMPDE ValidKernelPdeLocal;
extern MMPTE ValidKernelPteLocal;
extern MMPDE DemandZeroPde;
extern MMPTE DemandZeroPte;
extern MMPTE PrototypePte;
extern MMPTE MmDecommittedPte;
extern BOOLEAN MmLargeSystemCache;
extern BOOLEAN MmZeroPageFile;
extern BOOLEAN MmProtectFreedNonPagedPool;
extern BOOLEAN MmTrackLockedPages;
extern BOOLEAN MmTrackPtes;
extern BOOLEAN MmDynamicPfn;
extern BOOLEAN MmMirroring;
extern BOOLEAN MmMakeLowMemory;
extern BOOLEAN MmEnforceWriteProtection;
extern SIZE_T MmAllocationFragment;
extern ULONG MmConsumedPoolPercentage;
extern ULONG MmVerifyDriverBufferType;
extern ULONG MmVerifyDriverLevel;
extern WCHAR MmVerifyDriverBuffer[512];
extern WCHAR MmLargePageDriverBuffer[512];
extern LIST_ENTRY MiLargePageDriverList;
extern BOOLEAN MiLargePageAllDrivers;
extern ULONG MmVerifyDriverBufferLength;
extern ULONG MmLargePageDriverBufferLength;
extern SIZE_T MmSizeOfNonPagedPoolInBytes;
extern SIZE_T MmMaximumNonPagedPoolInBytes;
extern PFN_NUMBER MmMaximumNonPagedPoolInPages;
extern PFN_NUMBER MmSizeOfPagedPoolInPages;
extern PVOID MmNonPagedSystemStart;
extern SIZE_T MiNonPagedSystemSize;
extern PVOID MmNonPagedPoolStart;
extern PVOID MmNonPagedPoolExpansionStart;
extern PVOID MmNonPagedPoolEnd;
extern SIZE_T MmSizeOfPagedPoolInBytes;
extern PVOID MmPagedPoolStart;
extern PVOID MmPagedPoolEnd;
extern PVOID MmSessionBase;
extern SIZE_T MmSessionSize;
extern PMMPTE MmFirstReservedMappingPte, MmLastReservedMappingPte;
extern PMMPTE MiFirstReservedZeroingPte;
extern MI_PFN_CACHE_ATTRIBUTE MiPlatformCacheAttributes[2][MmMaximumCacheType];
extern PPHYSICAL_MEMORY_DESCRIPTOR MmPhysicalMemoryBlock;
extern SIZE_T MmBootImageSize;
extern PMMPTE MmSystemPtesStart[MaximumPtePoolTypes];
extern PMMPTE MmSystemPtesEnd[MaximumPtePoolTypes];
extern PMEMORY_ALLOCATION_DESCRIPTOR MxFreeDescriptor;
extern MEMORY_ALLOCATION_DESCRIPTOR MxOldFreeDescriptor;
extern ULONG_PTR MxPfnAllocation;
extern MM_PAGED_POOL_INFO MmPagedPoolInfo;
extern RTL_BITMAP MiPfnBitMap;
extern KGUARDED_MUTEX MmPagedPoolMutex;
extern KGUARDED_MUTEX MmSectionCommitMutex;
extern PVOID MmPagedPoolStart;
extern PVOID MmPagedPoolEnd;
extern PVOID MmNonPagedSystemStart;
extern PVOID MiSystemViewStart;
extern SIZE_T MmSystemViewSize;
extern PVOID MmSessionBase;
extern PVOID MiSessionSpaceEnd;
extern PMMPTE MiSessionImagePteStart;
extern PMMPTE MiSessionImagePteEnd;
extern PMMPTE MiSessionBasePte;
extern PMMPTE MiSessionLastPte;
extern SIZE_T MmSizeOfPagedPoolInBytes;
extern PMMPDE MmSystemPagePtes;
extern PVOID MmSystemCacheStart;
extern PVOID MmSystemCacheEnd;
extern MMSUPPORT MmSystemCacheWs;
extern SIZE_T MmAllocatedNonPagedPool;
extern ULONG_PTR MmSubsectionBase;
extern ULONG MmSpecialPoolTag;
extern PVOID MmHyperSpaceEnd;
extern PMMWSL MmSystemCacheWorkingSetList;
extern SIZE_T MmMinimumNonPagedPoolSize;
extern ULONG MmMinAdditionNonPagedPoolPerMb;
extern SIZE_T MmDefaultMaximumNonPagedPool;
extern ULONG MmMaxAdditionNonPagedPoolPerMb;
extern ULONG MmSecondaryColors;
extern ULONG MmSecondaryColorMask;
extern ULONG MmNumberOfSystemPtes;
extern ULONG MmMaximumNonPagedPoolPercent;
extern ULONG MmLargeStackSize;
extern PMMCOLOR_TABLES MmFreePagesByColor[FreePageList + 1];
extern MMPFNLIST MmStandbyPageListByPriority[8];
extern ULONG MmProductType;
extern MM_SYSTEMSIZE MmSystemSize;
extern PKEVENT MiLowMemoryEvent;
extern PKEVENT MiHighMemoryEvent;
extern PKEVENT MiLowPagedPoolEvent;
extern PKEVENT MiHighPagedPoolEvent;
extern PKEVENT MiLowNonPagedPoolEvent;
extern PKEVENT MiHighNonPagedPoolEvent;
extern PFN_NUMBER MmLowMemoryThreshold;
extern PFN_NUMBER MmHighMemoryThreshold;
extern PFN_NUMBER MiLowPagedPoolThreshold;
extern PFN_NUMBER MiHighPagedPoolThreshold;
extern PFN_NUMBER MiLowNonPagedPoolThreshold;
extern PFN_NUMBER MiHighNonPagedPoolThreshold;
extern PFN_NUMBER MmMinimumFreePages;
extern PFN_NUMBER MmPlentyFreePages;
extern PFN_COUNT MiExpansionPoolPagesInitialCharge;
extern PFN_NUMBER MmResidentAvailablePages;
extern PFN_NUMBER MmResidentAvailableAtInit;
extern ULONG MmTotalFreeSystemPtes[MaximumPtePoolTypes];
extern PFN_NUMBER MmTotalSystemDriverPages;
extern PVOID MiSessionImageStart;
extern PVOID MiSessionImageEnd;
extern PMMPTE MiHighestUserPte;
extern PMMPDE MiHighestUserPde;
extern PFN_NUMBER MmSystemPageDirectory[PD_COUNT];
extern PMMPTE MmSharedUserDataPte;
extern LIST_ENTRY MmProcessList;
extern BOOLEAN MmZeroingPageThreadActive;
extern KEVENT MmZeroingPageEvent;
extern ULONG MmSystemPageColor;
extern ULONG MmProcessColorSeed;
extern PMMWSL MmWorkingSetList;
extern PFN_NUMBER MiNumberOfFreePages;
extern SIZE_T MmSessionViewSize;
extern SIZE_T MmSessionPoolSize;
extern SIZE_T MmSessionImageSize;
extern PVOID MiSystemViewStart;
extern PVOID MiSessionPoolEnd;     // 0xBE000000
extern PVOID MiSessionPoolStart;   // 0xBD000000
extern PVOID MiSessionViewStart;   // 0xBE000000
extern ULONG MmMaximumDeadKernelStacks;
extern SLIST_HEADER MmDeadStackSListHead;
extern MM_AVL_TABLE MmSectionBasedRoot;
extern KGUARDED_MUTEX MmSectionBasedMutex;
extern PVOID MmHighSectionBase;

BOOLEAN
FORCEINLINE
MiIsMemoryTypeFree(TYPE_OF_MEMORY MemoryType)
{
    return ((MemoryType == LoaderFree) ||
            (MemoryType == LoaderLoadedProgram) ||
            (MemoryType == LoaderFirmwareTemporary) ||
            (MemoryType == LoaderOsloaderStack));
}

BOOLEAN
FORCEINLINE
MiIsMemoryTypeInvisible(TYPE_OF_MEMORY MemoryType)
{
    return ((MemoryType == LoaderFirmwarePermanent) ||
            (MemoryType == LoaderSpecialMemory) ||
            (MemoryType == LoaderHALCachedMemory) ||
            (MemoryType == LoaderBBTMemory));
}

#ifdef _M_AMD64
BOOLEAN
FORCEINLINE
MiIsUserPxe(PVOID Address)
{
    return ((ULONG_PTR)Address >> 7) == 0x1FFFFEDF6FB7DA0ULL;
}

BOOLEAN
FORCEINLINE
MiIsUserPpe(PVOID Address)
{
    return ((ULONG_PTR)Address >> 16) == 0xFFFFF6FB7DA0ULL;
}

BOOLEAN
FORCEINLINE
MiIsUserPde(PVOID Address)
{
    return ((ULONG_PTR)Address >> 25) == 0x7FFFFB7DA0ULL;
}

BOOLEAN
FORCEINLINE
MiIsUserPte(PVOID Address)
{
    return ((ULONG_PTR)Address >> 34) == 0x3FFFFDA0ULL;
}
#else
BOOLEAN
FORCEINLINE
MiIsUserPde(PVOID Address)
{
    return ((Address >= (PVOID)MiAddressToPde(NULL)) &&
            (Address <= (PVOID)MiHighestUserPde));
}

BOOLEAN
FORCEINLINE
MiIsUserPte(PVOID Address)
{
    return (Address <= (PVOID)MiHighestUserPte);
}
#endif

//
// Figures out the hardware bits for a PTE
//
ULONG_PTR
FORCEINLINE
MiDetermineUserGlobalPteMask(IN PVOID PointerPte)
{
    MMPTE TempPte;

    /* Start fresh */
    TempPte.u.Long = 0;

    /* Make it valid and accessed */
    TempPte.u.Hard.Valid = TRUE;
    MI_MAKE_ACCESSED_PAGE(&TempPte);

    /* Is this for user-mode? */
    if (
#if (_MI_PAGING_LEVELS == 4)
        MiIsUserPxe(PointerPte) ||
#endif
#if (_MI_PAGING_LEVELS >= 3)
        MiIsUserPpe(PointerPte) ||
#endif
        MiIsUserPde(PointerPte) ||
        MiIsUserPte(PointerPte))
    {
        /* Set the owner bit */
        MI_MAKE_OWNER_PAGE(&TempPte);
    }

    /* FIXME: We should also set the global bit */

    /* Return the protection */
    return TempPte.u.Long;
}

//
// Creates a valid kernel PTE with the given protection
//
FORCEINLINE
VOID
MI_MAKE_HARDWARE_PTE_KERNEL(IN PMMPTE NewPte,
                            IN PMMPTE MappingPte,
                            IN ULONG_PTR ProtectionMask,
                            IN PFN_NUMBER PageFrameNumber)
{
    /* Only valid for kernel, non-session PTEs */
    ASSERT(MappingPte > MiHighestUserPte);
    ASSERT(!MI_IS_SESSION_PTE(MappingPte));
    ASSERT((MappingPte < (PMMPTE)PDE_BASE) || (MappingPte > (PMMPTE)PDE_TOP));

    /* Start fresh */
    *NewPte = ValidKernelPte;

    /* Set the protection and page */
    NewPte->u.Hard.PageFrameNumber = PageFrameNumber;
    NewPte->u.Long |= MmProtectToPteMask[ProtectionMask];
}

//
// Creates a valid PTE with the given protection
//
FORCEINLINE
VOID
MI_MAKE_HARDWARE_PTE(IN PMMPTE NewPte,
                     IN PMMPTE MappingPte,
                     IN ULONG_PTR ProtectionMask,
                     IN PFN_NUMBER PageFrameNumber)
{
    /* Set the protection and page */
    NewPte->u.Long = MiDetermineUserGlobalPteMask(MappingPte);
    NewPte->u.Long |= MmProtectToPteMask[ProtectionMask];
    NewPte->u.Hard.PageFrameNumber = PageFrameNumber;
}

//
// Creates a valid user PTE with the given protection
//
FORCEINLINE
VOID
MI_MAKE_HARDWARE_PTE_USER(IN PMMPTE NewPte,
                          IN PMMPTE MappingPte,
                          IN ULONG_PTR ProtectionMask,
                          IN PFN_NUMBER PageFrameNumber)
{
    /* Only valid for kernel, non-session PTEs */
    ASSERT(MappingPte <= MiHighestUserPte);

    /* Start fresh */
    *NewPte = ValidKernelPte;

    /* Set the protection and page */
    NewPte->u.Hard.Owner = TRUE;
    NewPte->u.Hard.PageFrameNumber = PageFrameNumber;
    NewPte->u.Long |= MmProtectToPteMask[ProtectionMask];
}

#ifndef _M_AMD64
//
// Builds a Prototype PTE for the address of the PTE
//
FORCEINLINE
VOID
MI_MAKE_PROTOTYPE_PTE(IN PMMPTE NewPte,
                      IN PMMPTE PointerPte)
{
    ULONG_PTR Offset;

    /* Mark this as a prototype */
    NewPte->u.Long = 0;
    NewPte->u.Proto.Prototype = 1;

    /*
     * Prototype PTEs are only valid in paged pool by design, this little trick
     * lets us only use 28 bits for the adress of the PTE
     */
    Offset = (ULONG_PTR)PointerPte - (ULONG_PTR)MmPagedPoolStart;

    /* 7 bits go in the "low", and the other 21 bits go in the "high" */
    NewPte->u.Proto.ProtoAddressLow = Offset & 0x7F;
    NewPte->u.Proto.ProtoAddressHigh = (Offset & 0xFFFFFF80) >> 7;
    ASSERT(MiProtoPteToPte(NewPte) == PointerPte);
}
#endif

//
// Returns if the page is physically resident (ie: a large page)
// FIXFIX: CISC/x86 only?
//
FORCEINLINE
BOOLEAN
MI_IS_PHYSICAL_ADDRESS(IN PVOID Address)
{
    PMMPDE PointerPde;

    /* Large pages are never paged out, always physically resident */
    PointerPde = MiAddressToPde(Address);
    return ((PointerPde->u.Hard.LargePage) && (PointerPde->u.Hard.Valid));
}

//
// Writes a valid PTE
//
VOID
FORCEINLINE
MI_WRITE_VALID_PTE(IN PMMPTE PointerPte,
                   IN MMPTE TempPte)
{
    /* Write the valid PTE */
    ASSERT(PointerPte->u.Hard.Valid == 0);
    ASSERT(TempPte.u.Hard.Valid == 1);
    *PointerPte = TempPte;
}

//
// Writes an invalid PTE
//
VOID
FORCEINLINE
MI_WRITE_INVALID_PTE(IN PMMPTE PointerPte,
                     IN MMPTE InvalidPte)
{
    /* Write the invalid PTE */
    ASSERT(InvalidPte.u.Hard.Valid == 0);
    *PointerPte = InvalidPte;
}

//
// Writes a valid PDE
//
VOID
FORCEINLINE
MI_WRITE_VALID_PDE(IN PMMPDE PointerPde,
                   IN MMPDE TempPde)
{
    /* Write the valid PDE */
    ASSERT(PointerPde->u.Hard.Valid == 0);
    ASSERT(TempPde.u.Hard.Valid == 1);
    *PointerPde = TempPde;
}

//
// Writes an invalid PDE
//
VOID
FORCEINLINE
MI_WRITE_INVALID_PDE(IN PMMPDE PointerPde,
                     IN MMPDE InvalidPde)
{
    /* Write the invalid PDE */
    ASSERT(InvalidPde.u.Hard.Valid == 0);
    *PointerPde = InvalidPde;
}

//
// Checks if the thread already owns a working set
//
FORCEINLINE
BOOLEAN
MM_ANY_WS_LOCK_HELD(IN PETHREAD Thread)
{
    /* If any of these are held, return TRUE */
    return ((Thread->OwnsProcessWorkingSetExclusive) ||
            (Thread->OwnsProcessWorkingSetShared) ||
            (Thread->OwnsSystemWorkingSetExclusive) ||
            (Thread->OwnsSystemWorkingSetShared) ||
            (Thread->OwnsSessionWorkingSetExclusive) ||
            (Thread->OwnsSessionWorkingSetShared));
}

//
// Checks if the process owns the working set lock
//
FORCEINLINE
BOOLEAN
MI_WS_OWNER(IN PEPROCESS Process)
{
    /* Check if this process is the owner, and that the thread owns the WS */
    return ((KeGetCurrentThread()->ApcState.Process == &Process->Pcb) &&
            ((PsGetCurrentThread()->OwnsProcessWorkingSetExclusive) ||
             (PsGetCurrentThread()->OwnsProcessWorkingSetShared)));
}

//
// Locks the working set for the given process
//
FORCEINLINE
VOID
MiLockProcessWorkingSet(IN PEPROCESS Process,
                        IN PETHREAD Thread)
{
    /* Shouldn't already be owning the process working set */
    ASSERT(Thread->OwnsProcessWorkingSetShared == FALSE);
    ASSERT(Thread->OwnsProcessWorkingSetExclusive == FALSE);

    /* Block APCs, make sure that still nothing is already held */
    KeEnterGuardedRegion();
    ASSERT(!MM_ANY_WS_LOCK_HELD(Thread));

    /* FIXME: Actually lock it (we can't because Vm is used by MAREAs) */

    /* FIXME: This also can't be checked because Vm is used by MAREAs) */
    //ASSERT(Process->Vm.Flags.AcquiredUnsafe == 0);

    /* Okay, now we can own it exclusively */
    Thread->OwnsProcessWorkingSetExclusive = TRUE;
}

//
// Unlocks the working set for the given process
//
FORCEINLINE
VOID
MiUnlockProcessWorkingSet(IN PEPROCESS Process,
                          IN PETHREAD Thread)
{
    /* Make sure this process really is owner, and it was a safe acquisition */
    ASSERT(MI_WS_OWNER(Process));
    /* This can't be checked because Vm is used by MAREAs) */
    //ASSERT(Process->Vm.Flags.AcquiredUnsafe == 0);

    /* The thread doesn't own it anymore */
    ASSERT(Thread->OwnsProcessWorkingSetExclusive == TRUE);
    Thread->OwnsProcessWorkingSetExclusive = FALSE;

    /* FIXME: Actually release it (we can't because Vm is used by MAREAs) */

    /* Unblock APCs */
    KeLeaveGuardedRegion();
}

//
// Locks the working set
//
FORCEINLINE
VOID
MiLockWorkingSet(IN PETHREAD Thread,
                 IN PMMSUPPORT WorkingSet)
{
    /* Block APCs */
    KeEnterGuardedRegion();

    /* Working set should be in global memory */
    ASSERT(MI_IS_SESSION_ADDRESS((PVOID)WorkingSet) == FALSE);

    /* Thread shouldn't already be owning something */
    ASSERT(!MM_ANY_WS_LOCK_HELD(Thread));

    /* FIXME: Actually lock it (we can't because Vm is used by MAREAs) */

    /* Which working set is this? */
    if (WorkingSet == &MmSystemCacheWs)
    {
        /* Own the system working set */
        ASSERT((Thread->OwnsSystemWorkingSetExclusive == FALSE) &&
               (Thread->OwnsSystemWorkingSetShared == FALSE));
        Thread->OwnsSystemWorkingSetExclusive = TRUE;
    }
    else if (WorkingSet->Flags.SessionSpace)
    {
        /* We don't implement this yet */
        UNIMPLEMENTED;
        while (TRUE);
    }
    else
    {
        /* Own the process working set */
        ASSERT((Thread->OwnsProcessWorkingSetExclusive == FALSE) &&
               (Thread->OwnsProcessWorkingSetShared == FALSE));
        Thread->OwnsProcessWorkingSetExclusive = TRUE;
    }
}

//
// Unlocks the working set
//
FORCEINLINE
VOID
MiUnlockWorkingSet(IN PETHREAD Thread,
                   IN PMMSUPPORT WorkingSet)
{
    /* Working set should be in global memory */
    ASSERT(MI_IS_SESSION_ADDRESS((PVOID)WorkingSet) == FALSE);

    /* Which working set is this? */
    if (WorkingSet == &MmSystemCacheWs)
    {
        /* Release the system working set */
        ASSERT((Thread->OwnsSystemWorkingSetExclusive == TRUE) ||
               (Thread->OwnsSystemWorkingSetShared == TRUE));
        Thread->OwnsSystemWorkingSetExclusive = FALSE;
    }
    else if (WorkingSet->Flags.SessionSpace)
    {
        /* We don't implement this yet */
        UNIMPLEMENTED;
        while (TRUE);
    }
    else
    {
        /* Release the process working set */
        ASSERT((Thread->OwnsProcessWorkingSetExclusive) ||
               (Thread->OwnsProcessWorkingSetShared));
        Thread->OwnsProcessWorkingSetExclusive = FALSE;
    }

    /* FIXME: Actually release it (we can't because Vm is used by MAREAs) */

    /* Unblock APCs */
    KeLeaveGuardedRegion();
}

//
// Returns the ProtoPTE inside a VAD for the given VPN
//
FORCEINLINE
PMMPTE
MI_GET_PROTOTYPE_PTE_FOR_VPN(IN PMMVAD Vad,
                             IN ULONG_PTR Vpn)
{
    PMMPTE ProtoPte;

    /* Find the offset within the VAD's prototype PTEs */
    ProtoPte = Vad->FirstPrototypePte + (Vpn - Vad->StartingVpn);
    ASSERT(ProtoPte <= Vad->LastContiguousPte);
    return ProtoPte;
}

//
// Returns the PFN Database entry for the given page number
// Warning: This is not necessarily a valid PFN database entry!
//
FORCEINLINE
PMMPFN
MI_PFN_ELEMENT(IN PFN_NUMBER Pfn)
{
    /* Get the entry */
    return &MmPfnDatabase[Pfn];
};

BOOLEAN
NTAPI
MmArmInitSystem(
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
NTAPI
MiInitializeSessionSpaceLayout();

NTSTATUS
NTAPI
MiInitMachineDependent(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
NTAPI
MiComputeColorInformation(
    VOID
);

VOID
NTAPI
MiMapPfnDatabase(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
NTAPI
MiInitializeColorTables(
    VOID
);

VOID
NTAPI
MiInitializePfnDatabase(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
NTAPI
MiInitializeSessionIds(
    VOID
);

BOOLEAN
NTAPI
MiInitializeMemoryEvents(
    VOID
);

PFN_NUMBER
NTAPI
MxGetNextPage(
    IN PFN_NUMBER PageCount
);

PPHYSICAL_MEMORY_DESCRIPTOR
NTAPI
MmInitializeMemoryLimits(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PBOOLEAN IncludeType
);

PFN_NUMBER
NTAPI
MiPagesInLoaderBlock(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PBOOLEAN IncludeType
);

VOID
FASTCALL
MiSyncARM3WithROS(
    IN PVOID AddressStart,
    IN PVOID AddressEnd
);

NTSTATUS
NTAPI
MiRosProtectVirtualMemory(
    IN PEPROCESS Process,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T NumberOfBytesToProtect,
    IN ULONG NewAccessProtection,
    OUT PULONG OldAccessProtection OPTIONAL
);

NTSTATUS
NTAPI
MmArmAccessFault(
    IN BOOLEAN StoreInstruction,
    IN PVOID Address,
    IN KPROCESSOR_MODE Mode,
    IN PVOID TrapInformation
);

NTSTATUS
FASTCALL
MiCheckPdeForPagedPool(
    IN PVOID Address
);

VOID
NTAPI
MiInitializeNonPagedPool(
    VOID
);

VOID
NTAPI
MiInitializeNonPagedPoolThresholds(
    VOID
);

VOID
NTAPI
MiInitializePoolEvents(
    VOID
);

VOID                      //
NTAPI                     //
InitializePool(           //
    IN POOL_TYPE PoolType,// FIXFIX: This should go in ex.h after the pool merge
    IN ULONG Threshold    //
);                        //

// FIXFIX: THIS ONE TOO
VOID
NTAPI
INIT_FUNCTION
ExInitializePoolDescriptor(
    IN PPOOL_DESCRIPTOR PoolDescriptor,
    IN POOL_TYPE PoolType,
    IN ULONG PoolIndex,
    IN ULONG Threshold,
    IN PVOID PoolLock
);

NTSTATUS
NTAPI
MiInitializeSessionPool(
    VOID
);

VOID
NTAPI
MiInitializeSystemPtes(
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE PoolType
);

PMMPTE
NTAPI
MiReserveSystemPtes(
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
);

VOID
NTAPI
MiReleaseSystemPtes(
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
);


PFN_NUMBER
NTAPI
MiFindContiguousPages(
    IN PFN_NUMBER LowestPfn,
    IN PFN_NUMBER HighestPfn,
    IN PFN_NUMBER BoundaryPfn,
    IN PFN_NUMBER SizeInPages,
    IN MEMORY_CACHING_TYPE CacheType
);

PVOID
NTAPI
MiCheckForContiguousMemory(
    IN PVOID BaseAddress,
    IN PFN_NUMBER BaseAddressPages,
    IN PFN_NUMBER SizeInPages,
    IN PFN_NUMBER LowestPfn,
    IN PFN_NUMBER HighestPfn,
    IN PFN_NUMBER BoundaryPfn,
    IN MI_PFN_CACHE_ATTRIBUTE CacheAttribute
);

PMDL
NTAPI
MiAllocatePagesForMdl(
    IN PHYSICAL_ADDRESS LowAddress,
    IN PHYSICAL_ADDRESS HighAddress,
    IN PHYSICAL_ADDRESS SkipBytes,
    IN SIZE_T TotalBytes,
    IN MI_PFN_CACHE_ATTRIBUTE CacheAttribute,
    IN ULONG Flags
);

PVOID
NTAPI
MiMapLockedPagesInUserSpace(
    IN PMDL Mdl,
    IN PVOID BaseVa,
    IN MEMORY_CACHING_TYPE CacheType,
    IN PVOID BaseAddress
);

VOID
NTAPI
MiUnmapLockedPagesInUserSpace(
    IN PVOID BaseAddress,
    IN PMDL Mdl
);

VOID
NTAPI
MiInsertPageInList(
    IN PMMPFNLIST ListHead,
    IN PFN_NUMBER PageFrameIndex
);

VOID
NTAPI
MiUnlinkFreeOrZeroedPage(
    IN PMMPFN Entry
);

VOID
NTAPI
MiUnlinkPageFromList(
    IN PMMPFN Pfn
);

PFN_NUMBER
NTAPI
MiAllocatePfn(
    IN PMMPTE PointerPte,
    IN ULONG Protection
);

VOID
NTAPI
MiInitializePfn(
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPte,
    IN BOOLEAN Modified
);

NTSTATUS
NTAPI
MiInitializeAndChargePfn(
    OUT PPFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPde,
    IN PFN_NUMBER ContainingPageFrame,
    IN BOOLEAN SessionAllocation
);

VOID
NTAPI
MiInitializePfnAndMakePteValid(
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPte,
    IN MMPTE TempPte
);

VOID
NTAPI
MiInitializePfnForOtherProcess(
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPte,
    IN PFN_NUMBER PteFrame
);

VOID
NTAPI
MiDecrementShareCount(
    IN PMMPFN Pfn1,
    IN PFN_NUMBER PageFrameIndex
);

VOID
NTAPI
MiDecrementReferenceCount(
    IN PMMPFN Pfn1,
    IN PFN_NUMBER PageFrameIndex
);

PFN_NUMBER
NTAPI
MiRemoveAnyPage(
    IN ULONG Color
);

PFN_NUMBER
NTAPI
MiRemoveZeroPage(
    IN ULONG Color
);

VOID
NTAPI
MiZeroPhysicalPage(
    IN PFN_NUMBER PageFrameIndex
);

VOID
NTAPI
MiInsertPageInFreeList(
    IN PFN_NUMBER PageFrameIndex
);

PFN_COUNT
NTAPI
MiDeleteSystemPageableVm(
    IN PMMPTE PointerPte,
    IN PFN_NUMBER PageCount,
    IN ULONG Flags,
    OUT PPFN_NUMBER ValidPages
);

ULONG
NTAPI
MiGetPageProtection(
    IN PMMPTE PointerPte
);

PLDR_DATA_TABLE_ENTRY
NTAPI
MiLookupDataTableEntry(
    IN PVOID Address
);

VOID
NTAPI
MiInitializeDriverLargePageList(
    VOID
);

VOID
NTAPI
MiInitializeLargePageSupport(
    VOID
);

VOID
NTAPI
MiSyncCachedRanges(
    VOID
);

BOOLEAN
NTAPI
MiIsPfnInUse(
    IN PMMPFN Pfn1
);

PMMVAD
NTAPI
MiLocateAddress(
    IN PVOID VirtualAddress
);

PMMADDRESS_NODE
NTAPI
MiCheckForConflictingNode(
    IN ULONG_PTR StartVpn,
    IN ULONG_PTR EndVpn,
    IN PMM_AVL_TABLE Table
);

TABLE_SEARCH_RESULT
NTAPI
MiFindEmptyAddressRangeDownTree(
    IN SIZE_T Length,
    IN ULONG_PTR BoundaryAddress,
    IN ULONG_PTR Alignment,
    IN PMM_AVL_TABLE Table,
    OUT PULONG_PTR Base,
    OUT PMMADDRESS_NODE *Parent
);

NTSTATUS
NTAPI
MiFindEmptyAddressRangeDownBasedTree(
    IN SIZE_T Length,
    IN ULONG_PTR BoundaryAddress,
    IN ULONG_PTR Alignment,
    IN PMM_AVL_TABLE Table,
    OUT PULONG_PTR Base
);

NTSTATUS
NTAPI
MiFindEmptyAddressRangeInTree(
    IN SIZE_T Length,
    IN ULONG_PTR Alignment,
    IN PMM_AVL_TABLE Table,
    OUT PMMADDRESS_NODE *PreviousVad,
    OUT PULONG_PTR Base
);

VOID
NTAPI
MiInsertVad(
    IN PMMVAD Vad,
    IN PEPROCESS Process
);

VOID
NTAPI
MiInsertBasedSection(
    IN PSECTION Section
);

NTSTATUS
NTAPI
MiUnmapViewOfSection(
    IN PEPROCESS Process,
    IN PVOID BaseAddress,
    IN ULONG Flags
);

NTSTATUS
NTAPI
MiRosUnmapViewOfSection(
    IN PEPROCESS Process,
    IN PVOID BaseAddress,
    IN ULONG Flags
);

VOID
NTAPI
MiInsertNode(
    IN PMM_AVL_TABLE Table,
    IN PMMADDRESS_NODE NewNode,
    PMMADDRESS_NODE Parent,
    TABLE_SEARCH_RESULT Result
);

VOID
NTAPI
MiRemoveNode(
    IN PMMADDRESS_NODE Node,
    IN PMM_AVL_TABLE Table
);

PMMADDRESS_NODE
NTAPI
MiGetPreviousNode(
    IN PMMADDRESS_NODE Node
);

PMMADDRESS_NODE
NTAPI
MiGetNextNode(
    IN PMMADDRESS_NODE Node
);

BOOLEAN
NTAPI
MiInitializeSystemSpaceMap(
    IN PMMSESSION InputSession OPTIONAL
);

NTSTATUS
NTAPI
MiSessionCommitPageTables(
    IN PVOID StartVa,
    IN PVOID EndVa
);

ULONG
NTAPI
MiMakeProtectionMask(
    IN ULONG Protect
);

VOID
NTAPI
MiDeleteVirtualAddresses(
    IN ULONG_PTR Va,
    IN ULONG_PTR EndingAddress,
    IN PMMVAD Vad
);

ULONG
NTAPI
MiMakeSystemAddressValid(
    IN PVOID PageTableVirtualAddress,
    IN PEPROCESS CurrentProcess
);

ULONG
NTAPI
MiMakeSystemAddressValidPfn(
    IN PVOID VirtualAddress,
    IN KIRQL OldIrql
);

VOID
NTAPI
MiRemoveMappedView(
    IN PEPROCESS CurrentProcess,
    IN PMMVAD Vad
);

PSUBSECTION
NTAPI
MiLocateSubsection(
    IN PMMVAD Vad,
    IN ULONG_PTR Vpn
);

NTSTATUS
NTAPI
MiQueryMemorySectionName(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    OUT PVOID MemoryInformation,
    IN SIZE_T MemoryInformationLength,
    OUT PSIZE_T ReturnLength
);

NTSTATUS
NTAPI
MiRosAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PEPROCESS Process,
    IN PMEMORY_AREA MemoryArea,
    IN PMMSUPPORT AddressSpace,
    IN OUT PVOID* UBaseAddress,
    IN BOOLEAN Attached,
    IN OUT PSIZE_T URegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
);

NTSTATUS
NTAPI
MiRosUnmapViewInSystemSpace(
    IN PVOID MappedBase
);

POOL_TYPE
NTAPI
MmDeterminePoolType(
    IN PVOID PoolAddress
);

VOID
NTAPI
MiMakePdeExistAndMakeValid(
    IN PMMPTE PointerPde,
    IN PEPROCESS TargetProcess,
    IN KIRQL OldIrql
);

//
// MiRemoveZeroPage will use inline code to zero out the page manually if only
// free pages are available. In some scenarios, we don't/can't run that piece of
// code and would rather only have a real zero page. If we can't have a zero page,
// then we'd like to have our own code to grab a free page and zero it out, by
// using MiRemoveAnyPage. This macro implements this.
//
PFN_NUMBER
FORCEINLINE
MiRemoveZeroPageSafe(IN ULONG Color)
{
    if (MmFreePagesByColor[ZeroedPageList][Color].Flink != LIST_HEAD) return MiRemoveZeroPage(Color);
    return 0;
}

//
// New ARM3<->RosMM PAGE Architecture
//
BOOLEAN
FORCEINLINE
MiIsRosSectionObject(IN PVOID Section)
{
    PROS_SECTION_OBJECT RosSection = Section;
    if ((RosSection->Type == 'SC') && (RosSection->Size == 'TN')) return TRUE;
    return FALSE;
}

#ifdef _WIN64
// HACK ON TOP OF HACK ALERT!!!
#define MI_GET_ROS_DATA(x) \
    (((x)->RosMmData == 0) ? NULL : ((PMMROSPFN)((ULONG64)(ULONG)((x)->RosMmData) | \
                                    ((ULONG64)MmNonPagedPoolStart & 0xffffffff00000000ULL))))
#else
#define MI_GET_ROS_DATA(x)   ((PMMROSPFN)(x->RosMmData))
#endif
#define MI_IS_ROS_PFN(x)     (((x)->u4.AweAllocation == TRUE) && (MI_GET_ROS_DATA(x) != NULL))
#define ASSERT_IS_ROS_PFN(x) ASSERT(MI_IS_ROS_PFN(x) == TRUE);
typedef struct _MMROSPFN
{
    PMM_RMAP_ENTRY RmapListHead;
    SWAPENTRY SwapEntry;
} MMROSPFN, *PMMROSPFN;

#define RosMmData            AweReferenceCount

/* EOF */
