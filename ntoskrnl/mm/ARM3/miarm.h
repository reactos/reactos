/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/miarm.h
 * PURPOSE:         ARM Memory Manager Header
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define MI_LOWEST_VAD_ADDRESS                   (PVOID)MM_LOWEST_USER_ADDRESS

/* Make the code cleaner with some definitions for size multiples */
#define _1KB (1024u)
#define _1MB (1024 * _1KB)
#define _1GB (1024 * _1MB)

/* Everyone loves 64K */
#define _64K (64 * _1KB)

/* Size of a page table */
#define PT_SIZE  (PTE_PER_PAGE * sizeof(MMPTE))

/* Size of a page directory */
#define PD_SIZE  (PDE_PER_PAGE * sizeof(MMPDE))

/* Size of all page directories for a process */
#define SYSTEM_PD_SIZE (PPE_PER_PAGE * PD_SIZE)
#ifdef _M_IX86
C_ASSERT(SYSTEM_PD_SIZE == PAGE_SIZE);
#endif

//
// Protection Bits part of the internal memory manager Protection Mask, from:
// http://reactos.org/wiki/Techwiki:Memory_management_in_the_Windows_XP_kernel
// https://www.reactos.org/wiki/Techwiki:Memory_Protection_constants
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
#define MM_PROTECT_ACCESS      7

//
// These are flags on top of the actual protection mask
//
#define MM_NOCACHE            0x08
#define MM_GUARDPAGE          0x10
#define MM_WRITECOMBINE       0x18
#define MM_PROTECT_SPECIAL    0x18

//
// These are special cases
//
#define MM_DECOMMIT           (MM_ZERO_ACCESS | MM_GUARDPAGE)
#define MM_NOACCESS           (MM_ZERO_ACCESS | MM_WRITECOMBINE)
#define MM_OUTSWAPPED_KSTACK  (MM_EXECUTE_WRITECOPY | MM_WRITECOMBINE)
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
#if defined(_M_IX86)
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
#define PTE_PROTECT_MASK        0x612
#elif defined(_M_AMD64)
//
// Access Flags
//
#define PTE_READONLY            0x8000000000000000ULL
#define PTE_EXECUTE             0x0000000000000000ULL
#define PTE_EXECUTE_READ        PTE_EXECUTE /* EXECUTE implies READ on x64 */
#define PTE_READWRITE           0x8000000000000002ULL
#define PTE_WRITECOPY           0x8000000000000200ULL
#define PTE_EXECUTE_READWRITE   0x0000000000000002ULL
#define PTE_EXECUTE_WRITECOPY   0x0000000000000200ULL
#define PTE_PROTOTYPE           0x0000000000000400ULL

//
// State Flags
//
#define PTE_VALID               0x0000000000000001ULL
#define PTE_ACCESSED            0x0000000000000020ULL
#define PTE_DIRTY               0x0000000000000040ULL

//
// Cache flags
//
#define PTE_ENABLE_CACHE        0x0000000000000000ULL
#define PTE_DISABLE_CACHE       0x0000000000000010ULL
#define PTE_WRITECOMBINED_CACHE 0x0000000000000010ULL
#define PTE_PROTECT_MASK        0x8000000000000612ULL
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
#define PTE_PROTECT_MASK        0x610
#else
#error Define these please!
#endif

//
// Some internal SYSTEM_PTE_MISUSE bugcheck subcodes
// These names were created by Oleg Dubinskiy and Doug Lyons for ReactOS. For reference, see
// https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/bug-check-0xda--system-pte-misuse
//
#define PTE_MAPPING_NONE                0x100
#define PTE_MAPPING_NOT_OWNED           0x101
#define PTE_MAPPING_EMPTY               0x102
#define PTE_MAPPING_RESERVED            0x103
#define PTE_MAPPING_ADDRESS_NOT_OWNED   0x104
#define PTE_MAPPING_ADDRESS_INVALID     0x105
#define PTE_UNMAPPING_ADDRESS_NOT_OWNED 0x108
#define PTE_MAPPING_ADDRESS_EMPTY       0x109

//
// Mask for image section page protection
//
#define IMAGE_SCN_PROTECTION_MASK (IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE)

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
#define MM_SYSLDR_NO_IMPORTS   ((PVOID)(ULONG_PTR)-2)
#define MM_SYSLDR_BOOT_LOADED  ((PVOID)(ULONG_PTR)-1)
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
// Returns the color of a page
//
#define MI_GET_PAGE_COLOR(x)                ((x) & MmSecondaryColorMask)
#define MI_GET_NEXT_COLOR()                 (MI_GET_PAGE_COLOR(++MmSystemPageColor))
#define MI_GET_NEXT_PROCESS_COLOR(x)        (MI_GET_PAGE_COLOR(++(x)->NextPageColor))

//
// Prototype PTEs that don't yet have a pagefile association
//
#ifdef _WIN64
#define MI_PTE_LOOKUP_NEEDED 0xffffffffULL
#else
#define MI_PTE_LOOKUP_NEEDED 0xFFFFF
#endif

//
// Number of session data and tag pages
//
#define MI_SESSION_DATA_PAGES_MAXIMUM (MM_ALLOCATION_GRANULARITY / PAGE_SIZE)
#define MI_SESSION_TAG_PAGES_MAXIMUM  (MM_ALLOCATION_GRANULARITY / PAGE_SIZE)

//
// Used by MiCheckSecuredVad
//
#define MM_READ_WRITE_ALLOWED   11
#define MM_READ_ONLY_ALLOWED    10
#define MM_NO_ACCESS_ALLOWED    01
#define MM_DELETE_CHECK         85

//
// System views are binned into 64K chunks
//
#define MI_SYSTEM_VIEW_BUCKET_SIZE  _64K

//
// FIXFIX: These should go in ex.h after the pool merge
//
#ifdef _WIN64
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
#ifdef _WIN64
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
#ifdef _WIN64
    ULONG PoolTag;
#endif
    union
    {
#ifdef _WIN64
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
    PMMPDE PagedPoolBasePde;
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
    MMPDE PageDirectory;
#else
    PMMPDE PageTables;
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
extern SIZE_T MmMinimumStackCommitInBytes;
extern PFN_COUNT MiExpansionPoolPagesInitialCharge;
extern PFN_NUMBER MmResidentAvailableAtInit;
extern ULONG MmTotalFreeSystemPtes[MaximumPtePoolTypes];
extern PFN_NUMBER MmTotalSystemDriverPages;
extern ULONG MmCritsectTimeoutSeconds;
extern PVOID MiSessionImageStart;
extern PVOID MiSessionImageEnd;
extern PMMPTE MiHighestUserPte;
extern PMMPDE MiHighestUserPde;
extern PFN_NUMBER MmSystemPageDirectory[PPE_PER_PAGE];
extern PMMPTE MmSharedUserDataPte;
extern LIST_ENTRY MmProcessList;
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
extern PVOID MiSessionSpaceWs;
extern ULONG MmMaximumDeadKernelStacks;
extern SLIST_HEADER MmDeadStackSListHead;
extern MM_AVL_TABLE MmSectionBasedRoot;
extern KGUARDED_MUTEX MmSectionBasedMutex;
extern PVOID MmHighSectionBase;
extern SIZE_T MmSystemLockPagesCount;
extern ULONG_PTR MmSubsectionBase;
extern LARGE_INTEGER MmCriticalSectionTimeout;
extern LIST_ENTRY MmWorkingSetExpansionHead;
extern KSPIN_LOCK MmExpansionLock;
extern PETHREAD MiExpansionLockOwner;

FORCEINLINE
BOOLEAN
MI_IS_PROCESS_WORKING_SET(PMMSUPPORT WorkingSet)
{
    return (WorkingSet != &MmSystemCacheWs) && !WorkingSet->Flags.SessionSpace;
}

FORCEINLINE
BOOLEAN
MiIsMemoryTypeFree(TYPE_OF_MEMORY MemoryType)
{
    return ((MemoryType == LoaderFree) ||
            (MemoryType == LoaderLoadedProgram) ||
            (MemoryType == LoaderFirmwareTemporary) ||
            (MemoryType == LoaderOsloaderStack));
}

FORCEINLINE
BOOLEAN
MiIsMemoryTypeInvisible(TYPE_OF_MEMORY MemoryType)
{
    return ((MemoryType == LoaderFirmwarePermanent) ||
            (MemoryType == LoaderSpecialMemory) ||
            (MemoryType == LoaderHALCachedMemory) ||
            (MemoryType == LoaderBBTMemory));
}

#ifdef _M_AMD64
FORCEINLINE
BOOLEAN
MiIsUserPxe(PVOID Address)
{
    return ((ULONG_PTR)Address >> 7) == 0x1FFFFEDF6FB7DA0ULL;
}

FORCEINLINE
BOOLEAN
MiIsUserPpe(PVOID Address)
{
    return ((ULONG_PTR)Address >> 16) == 0xFFFFF6FB7DA0ULL;
}

FORCEINLINE
BOOLEAN
MiIsUserPde(PVOID Address)
{
    return ((ULONG_PTR)Address >> 25) == 0x7FFFFB7DA0ULL;
}

FORCEINLINE
BOOLEAN
MiIsUserPte(PVOID Address)
{
    return ((ULONG_PTR)Address >> 34) == 0x3FFFFDA0ULL;
}
#else
FORCEINLINE
BOOLEAN
MiIsUserPde(PVOID Address)
{
    return ((Address >= (PVOID)MiAddressToPde(NULL)) &&
            (Address <= (PVOID)MiHighestUserPde));
}

FORCEINLINE
BOOLEAN
MiIsUserPte(PVOID Address)
{
    return (Address >= (PVOID)PTE_BASE) && (Address <= (PVOID)MiHighestUserPte);
}
#endif

//
// Figures out the hardware bits for a PTE
//
FORCEINLINE
ULONG_PTR
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

    /* Check that we are not setting valid a page that should not be */
    ASSERT(ProtectionMask & MM_PROTECT_ACCESS);
    ASSERT((ProtectionMask & MM_GUARDPAGE) == 0);

    /* Start fresh */
    NewPte->u.Long = 0;

    /* Set the protection and page */
    NewPte->u.Hard.PageFrameNumber = PageFrameNumber;
    NewPte->u.Long |= MmProtectToPteMask[ProtectionMask];

    /* Make this valid & global */
#ifdef _GLOBAL_PAGES_ARE_AWESOME_
    if (KeFeatureBits & KF_GLOBAL_PAGE)
        NewPte->u.Hard.Global = 1;
#endif
    NewPte->u.Hard.Valid = 1;
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
    /* Check that we are not setting valid a page that should not be */
    ASSERT(ProtectionMask & MM_PROTECT_ACCESS);
    ASSERT((ProtectionMask & MM_GUARDPAGE) == 0);

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
    NewPte->u.Long = 0;

    /* Check that we are not setting valid a page that should not be */
    ASSERT(ProtectionMask & MM_PROTECT_ACCESS);
    ASSERT((ProtectionMask & MM_GUARDPAGE) == 0);

    NewPte->u.Hard.Valid = TRUE;
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
     * lets us only use 30 bits for the address of the PTE, as long as the area
     * stays 1024MB At most.
     */
    Offset = (ULONG_PTR)PointerPte - (ULONG_PTR)MmPagedPoolStart;

    /*
     * 7 bits go in the "low" (but we assume the bottom 2 are zero)
     * and the other 21 bits go in the "high"
     */
    NewPte->u.Proto.ProtoAddressLow = (Offset & 0x1FC) >> 2;
    NewPte->u.Proto.ProtoAddressHigh = (Offset & 0x3FFFFE00) >> 9;
}

//
// Builds a Subsection PTE for the address of the Segment
//
FORCEINLINE
VOID
MI_MAKE_SUBSECTION_PTE(IN PMMPTE NewPte,
                       IN PVOID Segment)
{
    ULONG_PTR Offset;

    /* Mark this as a prototype */
    NewPte->u.Long = 0;
    NewPte->u.Subsect.Prototype = 1;

    /*
     * Segments are only valid either in nonpaged pool. We store the 20 bit
     * difference either from the top or bottom of nonpaged pool, giving a
     * maximum of 128MB to each delta, meaning nonpaged pool cannot exceed
     * 256MB.
     */
    if ((ULONG_PTR)Segment < ((ULONG_PTR)MmSubsectionBase + (128 * _1MB)))
    {
        Offset = (ULONG_PTR)Segment - (ULONG_PTR)MmSubsectionBase;
        NewPte->u.Subsect.WhichPool = PagedPool;
    }
    else
    {
        Offset = (ULONG_PTR)MmNonPagedPoolEnd - (ULONG_PTR)Segment;
        NewPte->u.Subsect.WhichPool = NonPagedPool;
    }

    /*
     * 4 bits go in the "low" (but we assume the bottom 3 are zero)
     * and the other 20 bits go in the "high"
     */
    NewPte->u.Subsect.SubsectionAddressLow = (Offset & 0x78) >> 3;
    NewPte->u.Subsect.SubsectionAddressHigh = (Offset & 0xFFFFF80) >> 7;
}

FORCEINLINE
BOOLEAN
MI_IS_MAPPED_PTE(PMMPTE PointerPte)
{
    /// \todo Make this reasonable code, this is UGLY!
    return ((PointerPte->u.Long & 0xFFFFFC01) != 0);
}

#endif

FORCEINLINE
VOID
MI_MAKE_TRANSITION_PTE(_Out_ PMMPTE NewPte,
                       _In_ PFN_NUMBER Page,
                       _In_ ULONG Protection)
{
    NewPte->u.Long = 0;
    NewPte->u.Trans.Transition = 1;
    NewPte->u.Trans.Protection = Protection;
    NewPte->u.Trans.PageFrameNumber = Page;
}

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
FORCEINLINE
VOID
MI_WRITE_VALID_PTE(IN PMMPTE PointerPte,
                   IN MMPTE TempPte)
{
    /* Write the valid PTE */
    ASSERT(PointerPte->u.Hard.Valid == 0);
    ASSERT(TempPte.u.Hard.Valid == 1);
#if _M_AMD64
    ASSERT(!MI_IS_PAGE_TABLE_ADDRESS(MiPteToAddress(PointerPte)) ||
           (TempPte.u.Hard.NoExecute == 0));
#endif
    *PointerPte = TempPte;
}

//
// Updates a valid PTE
//
FORCEINLINE
VOID
MI_UPDATE_VALID_PTE(IN PMMPTE PointerPte,
                   IN MMPTE TempPte)
{
    /* Write the valid PTE */
    ASSERT(PointerPte->u.Hard.Valid == 1);
    ASSERT(TempPte.u.Hard.Valid == 1);
    ASSERT(PointerPte->u.Hard.PageFrameNumber == TempPte.u.Hard.PageFrameNumber);
    *PointerPte = TempPte;
}

//
// Writes an invalid PTE
//
FORCEINLINE
VOID
MI_WRITE_INVALID_PTE(IN PMMPTE PointerPte,
                     IN MMPTE InvalidPte)
{
    /* Write the invalid PTE */
    ASSERT(InvalidPte.u.Hard.Valid == 0);
    *PointerPte = InvalidPte;
}

//
// Erase the PTE completely
//
FORCEINLINE
VOID
MI_ERASE_PTE(IN PMMPTE PointerPte)
{
    /* Zero out the PTE */
    ASSERT(PointerPte->u.Long != 0);
    PointerPte->u.Long = 0;
}

//
// Writes a valid PDE
//
FORCEINLINE
VOID
MI_WRITE_VALID_PDE(IN PMMPDE PointerPde,
                   IN MMPDE TempPde)
{
    /* Write the valid PDE */
    ASSERT(PointerPde->u.Hard.Valid == 0);
#ifdef _M_AMD64
    ASSERT(PointerPde->u.Hard.NoExecute == 0);
#endif
    ASSERT(TempPde.u.Hard.Valid == 1);
    *PointerPde = TempPde;
}

//
// Writes an invalid PDE
//
FORCEINLINE
VOID
MI_WRITE_INVALID_PDE(IN PMMPDE PointerPde,
                     IN MMPDE InvalidPde)
{
    /* Write the invalid PDE */
    ASSERT(InvalidPde.u.Hard.Valid == 0);
    ASSERT(InvalidPde.u.Long != 0);
#ifdef _M_AMD64
    ASSERT(InvalidPde.u.Soft.Protection == MM_EXECUTE_READWRITE);
#endif
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

FORCEINLINE
BOOLEAN
MM_ANY_WS_LOCK_HELD_EXCLUSIVE(_In_ PETHREAD Thread)
{
    return ((Thread->OwnsProcessWorkingSetExclusive) ||
            (Thread->OwnsSystemWorkingSetExclusive) ||
            (Thread->OwnsSessionWorkingSetExclusive));
}

//
// Checks if the process owns the working set lock
//
FORCEINLINE
BOOLEAN
MI_WS_OWNER(IN PEPROCESS Process)
{
    /* Check if this process is the owner, and that the thread owns the WS */
    if (PsGetCurrentThread()->OwnsProcessWorkingSetExclusive == 0)
    {
        DPRINT("Thread: %p is not an owner\n", PsGetCurrentThread());
    }
    if (KeGetCurrentThread()->ApcState.Process != &Process->Pcb)
    {
        DPRINT("Current thread %p is attached to another process %p\n", PsGetCurrentThread(), Process);
    }
    return ((KeGetCurrentThread()->ApcState.Process == &Process->Pcb) &&
            ((PsGetCurrentThread()->OwnsProcessWorkingSetExclusive) ||
             (PsGetCurrentThread()->OwnsProcessWorkingSetShared)));
}

//
// New ARM3<->RosMM PAGE Architecture
//
FORCEINLINE
BOOLEAN
MiIsRosSectionObject(IN PSECTION Section)
{
    return Section->u.Flags.filler;
}

#define MI_IS_ROS_PFN(x)     ((x)->u4.AweAllocation == TRUE)

VOID
NTAPI
MiDecrementReferenceCount(
    IN PMMPFN Pfn1,
    IN PFN_NUMBER PageFrameIndex
);

FORCEINLINE
BOOLEAN
MI_IS_WS_UNSAFE(IN PEPROCESS Process)
{
    return (Process->Vm.Flags.AcquiredUnsafe == TRUE);
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

    /* Lock the working set */
    ExAcquirePushLockExclusive(&Process->Vm.WorkingSetMutex);

    /* Now claim that we own the lock */
    ASSERT(!MI_IS_WS_UNSAFE(Process));
    ASSERT(Thread->OwnsProcessWorkingSetExclusive == FALSE);
    Thread->OwnsProcessWorkingSetExclusive = TRUE;
}

FORCEINLINE
VOID
MiLockProcessWorkingSetShared(IN PEPROCESS Process,
                              IN PETHREAD Thread)
{
    /* Shouldn't already be owning the process working set */
    ASSERT(Thread->OwnsProcessWorkingSetShared == FALSE);
    ASSERT(Thread->OwnsProcessWorkingSetExclusive == FALSE);

    /* Block APCs, make sure that still nothing is already held */
    KeEnterGuardedRegion();
    ASSERT(!MM_ANY_WS_LOCK_HELD(Thread));

    /* Lock the working set */
    ExAcquirePushLockShared(&Process->Vm.WorkingSetMutex);

    /* Now claim that we own the lock */
    ASSERT(!MI_IS_WS_UNSAFE(Process));
    ASSERT(Thread->OwnsProcessWorkingSetShared == FALSE);
    ASSERT(Thread->OwnsProcessWorkingSetExclusive == FALSE);
    Thread->OwnsProcessWorkingSetShared = TRUE;
}

FORCEINLINE
VOID
MiLockProcessWorkingSetUnsafe(IN PEPROCESS Process,
                              IN PETHREAD Thread)
{
    /* Shouldn't already be owning the process working set */
    ASSERT(Thread->OwnsProcessWorkingSetExclusive == FALSE);

    /* APCs must be blocked, make sure that still nothing is already held */
    ASSERT(KeAreAllApcsDisabled() == TRUE);
    ASSERT(!MM_ANY_WS_LOCK_HELD(Thread));

    /* Lock the working set */
    ExAcquirePushLockExclusive(&Process->Vm.WorkingSetMutex);

    /* Now claim that we own the lock */
    ASSERT(!MI_IS_WS_UNSAFE(Process));
    Process->Vm.Flags.AcquiredUnsafe = 1;
    ASSERT(Thread->OwnsProcessWorkingSetExclusive == FALSE);
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
    /* Make sure we are the owner of a safe acquisition */
    ASSERT(MI_WS_OWNER(Process));
    ASSERT(!MI_IS_WS_UNSAFE(Process));

    /* The thread doesn't own it anymore */
    ASSERT(Thread->OwnsProcessWorkingSetExclusive == TRUE);
    Thread->OwnsProcessWorkingSetExclusive = FALSE;

    /* Release the lock and re-enable APCs */
    ExReleasePushLockExclusive(&Process->Vm.WorkingSetMutex);
    KeLeaveGuardedRegion();
}

//
// Unlocks the working set for the given process
//
FORCEINLINE
VOID
MiUnlockProcessWorkingSetShared(IN PEPROCESS Process,
                                IN PETHREAD Thread)
{
    /* Make sure we are the owner of a safe acquisition (because shared) */
    ASSERT(MI_WS_OWNER(Process));
    ASSERT(!MI_IS_WS_UNSAFE(Process));

    /* Ensure we are in a shared acquisition */
    ASSERT(Thread->OwnsProcessWorkingSetShared == TRUE);
    ASSERT(Thread->OwnsProcessWorkingSetExclusive == FALSE);

    /* Don't claim the lock anylonger */
    Thread->OwnsProcessWorkingSetShared = FALSE;

    /* Release the lock and re-enable APCs */
    ExReleasePushLockShared(&Process->Vm.WorkingSetMutex);
    KeLeaveGuardedRegion();
}

//
// Unlocks the working set for the given process
//
FORCEINLINE
VOID
MiUnlockProcessWorkingSetUnsafe(IN PEPROCESS Process,
                                IN PETHREAD Thread)
{
    /* Make sure we are the owner of an unsafe acquisition */
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    ASSERT(KeAreAllApcsDisabled() == TRUE);
    ASSERT(MI_WS_OWNER(Process));
    ASSERT(MI_IS_WS_UNSAFE(Process));

    /* No longer unsafe */
    Process->Vm.Flags.AcquiredUnsafe = 0;

    /* The thread doesn't own it anymore */
    ASSERT(Thread->OwnsProcessWorkingSetExclusive == TRUE);
    Thread->OwnsProcessWorkingSetExclusive = FALSE;

    /* Release the lock but don't touch APC state */
    ExReleasePushLockExclusive(&Process->Vm.WorkingSetMutex);
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
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

    /* Lock this working set */
    ExAcquirePushLockExclusive(&WorkingSet->WorkingSetMutex);

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
        /* Own the session working set */
        ASSERT((Thread->OwnsSessionWorkingSetExclusive == FALSE) &&
               (Thread->OwnsSessionWorkingSetShared == FALSE));
        Thread->OwnsSessionWorkingSetExclusive = TRUE;
    }
    else
    {
        /* Own the process working set */
        ASSERT((Thread->OwnsProcessWorkingSetExclusive == FALSE) &&
               (Thread->OwnsProcessWorkingSetShared == FALSE));
        Thread->OwnsProcessWorkingSetExclusive = TRUE;
    }
}

FORCEINLINE
VOID
MiLockWorkingSetShared(
    _In_ PETHREAD Thread,
    _In_ PMMSUPPORT WorkingSet)
{
    /* Block APCs */
    KeEnterGuardedRegion();

    /* Working set should be in global memory */
    ASSERT(MI_IS_SESSION_ADDRESS((PVOID)WorkingSet) == FALSE);

    /* Thread shouldn't already be owning something */
    ASSERT(!MM_ANY_WS_LOCK_HELD(Thread));

    /* Lock this working set */
    ExAcquirePushLockShared(&WorkingSet->WorkingSetMutex);

    /* Which working set is this? */
    if (WorkingSet == &MmSystemCacheWs)
    {
        /* Own the system working set */
        ASSERT((Thread->OwnsSystemWorkingSetExclusive == FALSE) &&
               (Thread->OwnsSystemWorkingSetShared == FALSE));
        Thread->OwnsSystemWorkingSetShared = TRUE;
    }
    else if (WorkingSet->Flags.SessionSpace)
    {
        /* Own the session working set */
        ASSERT((Thread->OwnsSessionWorkingSetExclusive == FALSE) &&
               (Thread->OwnsSessionWorkingSetShared == FALSE));
        Thread->OwnsSessionWorkingSetShared = TRUE;
    }
    else
    {
        /* Own the process working set */
        ASSERT((Thread->OwnsProcessWorkingSetExclusive == FALSE) &&
               (Thread->OwnsProcessWorkingSetShared == FALSE));
        Thread->OwnsProcessWorkingSetShared = TRUE;
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
        ASSERT((Thread->OwnsSystemWorkingSetExclusive == TRUE) &&
               (Thread->OwnsSystemWorkingSetShared == FALSE));
        Thread->OwnsSystemWorkingSetExclusive = FALSE;
    }
    else if (WorkingSet->Flags.SessionSpace)
    {
        /* Release the session working set */
        ASSERT((Thread->OwnsSessionWorkingSetExclusive == TRUE) &&
               (Thread->OwnsSessionWorkingSetShared == FALSE));
        Thread->OwnsSessionWorkingSetExclusive = FALSE;
    }
    else
    {
        /* Release the process working set */
        ASSERT((Thread->OwnsProcessWorkingSetExclusive == TRUE) &&
               (Thread->OwnsProcessWorkingSetShared == FALSE));
        Thread->OwnsProcessWorkingSetExclusive = FALSE;
    }

    /* Release the working set lock */
    ExReleasePushLockExclusive(&WorkingSet->WorkingSetMutex);

    /* Unblock APCs */
    KeLeaveGuardedRegion();
}

FORCEINLINE
VOID
MiUnlockWorkingSetShared(
    _In_ PETHREAD Thread,
    _In_ PMMSUPPORT WorkingSet)
{
    /* Working set should be in global memory */
    ASSERT(MI_IS_SESSION_ADDRESS((PVOID)WorkingSet) == FALSE);

    /* Which working set is this? */
    if (WorkingSet == &MmSystemCacheWs)
    {
        /* Release the system working set */
        ASSERT((Thread->OwnsSystemWorkingSetExclusive == FALSE) &&
               (Thread->OwnsSystemWorkingSetShared == TRUE));
        Thread->OwnsSystemWorkingSetShared = FALSE;
    }
    else if (WorkingSet->Flags.SessionSpace)
    {
        /* Release the session working set */
        ASSERT((Thread->OwnsSessionWorkingSetExclusive == FALSE) &&
               (Thread->OwnsSessionWorkingSetShared == TRUE));
        Thread->OwnsSessionWorkingSetShared = FALSE;
    }
    else
    {
        /* Release the process working set */
        ASSERT((Thread->OwnsProcessWorkingSetExclusive == FALSE) &&
               (Thread->OwnsProcessWorkingSetShared == TRUE));
        Thread->OwnsProcessWorkingSetShared = FALSE;
    }

    /* Release the working set lock */
    ExReleasePushLockShared(&WorkingSet->WorkingSetMutex);

    /* Unblock APCs */
    KeLeaveGuardedRegion();
}

FORCEINLINE
BOOLEAN
MiConvertSharedWorkingSetLockToExclusive(
    _In_ PETHREAD Thread,
    _In_ PMMSUPPORT Vm)
{
    /* Sanity check: No exclusive lock. */
    ASSERT(!Thread->OwnsProcessWorkingSetExclusive);
    ASSERT(!Thread->OwnsSessionWorkingSetExclusive);
    ASSERT(!Thread->OwnsSystemWorkingSetExclusive);

    /* And it should have one and only one shared lock */
    ASSERT((Thread->OwnsProcessWorkingSetShared + Thread->OwnsSessionWorkingSetShared + Thread->OwnsSystemWorkingSetShared) == 1);

    /* Try. */
    if (!ExConvertPushLockSharedToExclusive(&Vm->WorkingSetMutex))
        return FALSE;

    if (Vm == &MmSystemCacheWs)
    {
        ASSERT(Thread->OwnsSystemWorkingSetShared);
        Thread->OwnsSystemWorkingSetShared = FALSE;
        Thread->OwnsSystemWorkingSetExclusive = TRUE;
    }
    else if (Vm->Flags.SessionSpace)
    {
        ASSERT(Thread->OwnsSessionWorkingSetShared);
        Thread->OwnsSessionWorkingSetShared = FALSE;
        Thread->OwnsSessionWorkingSetExclusive = TRUE;
    }
    else
    {
        ASSERT(Thread->OwnsProcessWorkingSetShared);
        Thread->OwnsProcessWorkingSetShared = FALSE;
        Thread->OwnsProcessWorkingSetExclusive = TRUE;
    }

    return TRUE;
}

FORCEINLINE
VOID
MiUnlockProcessWorkingSetForFault(IN PEPROCESS Process,
                                  IN PETHREAD Thread,
                                  OUT PBOOLEAN Safe,
                                  OUT PBOOLEAN Shared)
{
    ASSERT(MI_WS_OWNER(Process));

    /* Check if the current owner is unsafe */
    if (MI_IS_WS_UNSAFE(Process))
    {
        /* Release unsafely */
        MiUnlockProcessWorkingSetUnsafe(Process, Thread);
        *Safe = FALSE;
        *Shared = FALSE;
    }
    else if (Thread->OwnsProcessWorkingSetExclusive == 1)
    {
        /* Owner is safe and exclusive, release normally */
        MiUnlockProcessWorkingSet(Process, Thread);
        *Safe = TRUE;
        *Shared = FALSE;
    }
    else
    {
        /* Owner is shared (implies safe), release normally */
        MiUnlockProcessWorkingSetShared(Process, Thread);
        *Safe = TRUE;
        *Shared = TRUE;
    }
}

FORCEINLINE
VOID
MiLockProcessWorkingSetForFault(IN PEPROCESS Process,
                                IN PETHREAD Thread,
                                IN BOOLEAN Safe,
                                IN BOOLEAN Shared)
{
    /* Check if this was a safe lock or not */
    if (Safe)
    {
        if (Shared)
        {
            /* Reacquire safely & shared */
            MiLockProcessWorkingSetShared(Process, Thread);
        }
        else
        {
            /* Reacquire safely */
            MiLockProcessWorkingSet(Process, Thread);
        }
    }
    else
    {
        /* Unsafe lock cannot be shared */
        ASSERT(Shared == FALSE);
        /* Reacquire unsafely */
        MiLockProcessWorkingSetUnsafe(Process, Thread);
    }
}

FORCEINLINE
KIRQL
MiAcquireExpansionLock(VOID)
{
    KIRQL OldIrql;

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    KeAcquireSpinLock(&MmExpansionLock, &OldIrql);
    ASSERT(MiExpansionLockOwner == NULL);
    MiExpansionLockOwner = PsGetCurrentThread();
    return OldIrql;
}

FORCEINLINE
VOID
MiReleaseExpansionLock(KIRQL OldIrql)
{
    ASSERT(MiExpansionLockOwner == PsGetCurrentThread());
    MiExpansionLockOwner = NULL;
    KeReleaseSpinLock(&MmExpansionLock, OldIrql);
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
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

//
// Drops a locked page without dereferencing it
//
FORCEINLINE
VOID
MiDropLockCount(IN PMMPFN Pfn1)
{
    /* This page shouldn't be locked, but it should be valid */
    ASSERT(Pfn1->u3.e2.ReferenceCount != 0);
    ASSERT(Pfn1->u2.ShareCount == 0);

    /* Is this the last reference to the page */
    if (Pfn1->u3.e2.ReferenceCount == 1)
    {
        /* It better not be valid */
        ASSERT(Pfn1->u3.e1.PageLocation != ActiveAndValid);

        /* Is it a prototype PTE? */
        if ((Pfn1->u3.e1.PrototypePte == 1) &&
            (Pfn1->OriginalPte.u.Soft.Prototype == 1))
        {
            /* FIXME: We should return commit */
            DPRINT1("Not returning commit for prototype PTE\n");
        }

        /* Update the counter */
        InterlockedDecrementSizeT(&MmSystemLockPagesCount);
    }
}

//
// Drops a locked page and dereferences it
//
FORCEINLINE
VOID
MiDereferencePfnAndDropLockCount(IN PMMPFN Pfn1)
{
    USHORT RefCount, OldRefCount;
    PFN_NUMBER PageFrameIndex;

    /* Loop while we decrement the page successfully */
    do
    {
        /* There should be at least one reference */
        OldRefCount = Pfn1->u3.e2.ReferenceCount;
        ASSERT(OldRefCount != 0);

        /* Are we the last one */
        if (OldRefCount == 1)
        {
            /* The page shoudln't be shared not active at this point */
            ASSERT(Pfn1->u3.e2.ReferenceCount == 1);
            ASSERT(Pfn1->u3.e1.PageLocation != ActiveAndValid);
            ASSERT(Pfn1->u2.ShareCount == 0);

            /* Is it a prototype PTE? */
            if ((Pfn1->u3.e1.PrototypePte == 1) &&
                (Pfn1->OriginalPte.u.Soft.Prototype == 1))
            {
                /* FIXME: We should return commit */
                DPRINT1("Not returning commit for prototype PTE\n");
            }

            /* Update the counter, and drop a reference the long way */
            InterlockedDecrementSizeT(&MmSystemLockPagesCount);
            PageFrameIndex = MiGetPfnEntryIndex(Pfn1);
            MiDecrementReferenceCount(Pfn1, PageFrameIndex);
            return;
        }

        /* Drop a reference the short way, and that's it */
        RefCount = InterlockedCompareExchange16((PSHORT)&Pfn1->u3.e2.ReferenceCount,
                                                OldRefCount - 1,
                                                OldRefCount);
        ASSERT(RefCount != 0);
    } while (OldRefCount != RefCount);

    /* If we got here, there should be more than one reference */
    ASSERT(RefCount > 1);
    if (RefCount == 2)
    {
        /* Is it still being shared? */
        if (Pfn1->u2.ShareCount >= 1)
        {
            /* Then it should be valid */
            ASSERT(Pfn1->u3.e1.PageLocation == ActiveAndValid);

            /* Is it a prototype PTE? */
            if ((Pfn1->u3.e1.PrototypePte == 1) &&
                (Pfn1->OriginalPte.u.Soft.Prototype == 1))
            {
                /* We don't handle ethis */
                ASSERT(FALSE);
            }

            /* Update the counter */
            InterlockedDecrementSizeT(&MmSystemLockPagesCount);
        }
    }
}

//
// References a locked page and updates the counter
// Used in MmProbeAndLockPages to handle different edge cases
//
FORCEINLINE
VOID
MiReferenceProbedPageAndBumpLockCount(IN PMMPFN Pfn1)
{
    USHORT RefCount, OldRefCount;

    /* Sanity check */
    ASSERT(Pfn1->u3.e2.ReferenceCount != 0);

    /* Does ARM3 own the page? */
    if (MI_IS_ROS_PFN(Pfn1))
    {
        /* ReactOS Mm doesn't track share count */
        ASSERT(Pfn1->u3.e1.PageLocation == ActiveAndValid);
    }
    else
    {
        /* On ARM3 pages, we should see a valid share count */
        ASSERT((Pfn1->u2.ShareCount != 0) && (Pfn1->u3.e1.PageLocation == ActiveAndValid));

        /* Is it a prototype PTE? */
        if ((Pfn1->u3.e1.PrototypePte == 1) &&
            (Pfn1->OriginalPte.u.Soft.Prototype == 1))
        {
            /* FIXME: We should charge commit */
            DPRINT1("Not charging commit for prototype PTE\n");
        }
    }

    /* More locked pages! */
    InterlockedIncrementSizeT(&MmSystemLockPagesCount);

    /* Loop trying to update the reference count */
    do
    {
        /* Get the current reference count, make sure it's valid */
        OldRefCount = Pfn1->u3.e2.ReferenceCount;
        ASSERT(OldRefCount != 0);
        ASSERT(OldRefCount < 2500);

        /* Bump it up by one */
        RefCount = InterlockedCompareExchange16((PSHORT)&Pfn1->u3.e2.ReferenceCount,
                                                OldRefCount + 1,
                                                OldRefCount);
        ASSERT(RefCount != 0);
    } while (OldRefCount != RefCount);

    /* Was this the first lock attempt? If not, undo our bump */
    if (OldRefCount != 1) InterlockedDecrementSizeT(&MmSystemLockPagesCount);
}

//
// References a locked page and updates the counter
// Used in all other cases except MmProbeAndLockPages
//
FORCEINLINE
VOID
MiReferenceUsedPageAndBumpLockCount(IN PMMPFN Pfn1)
{
    USHORT NewRefCount;

    /* Is it a prototype PTE? */
    if ((Pfn1->u3.e1.PrototypePte == 1) &&
        (Pfn1->OriginalPte.u.Soft.Prototype == 1))
    {
        /* FIXME: We should charge commit */
        DPRINT1("Not charging commit for prototype PTE\n");
    }

    /* More locked pages! */
    InterlockedIncrementSizeT(&MmSystemLockPagesCount);

    /* Update the reference count */
    NewRefCount = InterlockedIncrement16((PSHORT)&Pfn1->u3.e2.ReferenceCount);
    if (NewRefCount == 2)
    {
        /* Is it locked or shared? */
        if (Pfn1->u2.ShareCount)
        {
            /* It's shared, so make sure it's active */
            ASSERT(Pfn1->u3.e1.PageLocation == ActiveAndValid);
        }
        else
        {
            /* It's locked, so we shouldn't lock again */
            InterlockedDecrementSizeT(&MmSystemLockPagesCount);
        }
    }
    else
    {
        /* Someone had already locked the page, so undo our bump */
        ASSERT(NewRefCount < 2500);
        InterlockedDecrementSizeT(&MmSystemLockPagesCount);
    }
}

//
// References a locked page and updates the counter
// Used in all other cases except MmProbeAndLockPages
//
FORCEINLINE
VOID
MiReferenceUnusedPageAndBumpLockCount(IN PMMPFN Pfn1)
{
    USHORT NewRefCount;

    /* Make sure the page isn't used yet */
    ASSERT(Pfn1->u2.ShareCount == 0);
    ASSERT(Pfn1->u3.e1.PageLocation != ActiveAndValid);

    /* Is it a prototype PTE? */
    if ((Pfn1->u3.e1.PrototypePte == 1) &&
        (Pfn1->OriginalPte.u.Soft.Prototype == 1))
    {
        /* FIXME: We should charge commit */
        DPRINT1("Not charging commit for prototype PTE\n");
    }

    /* More locked pages! */
    InterlockedIncrementSizeT(&MmSystemLockPagesCount);

    /* Update the reference count */
    NewRefCount = InterlockedIncrement16((PSHORT)&Pfn1->u3.e2.ReferenceCount);
    if (NewRefCount != 1)
    {
        /* Someone had already locked the page, so undo our bump */
        ASSERT(NewRefCount < 2500);
        InterlockedDecrementSizeT(&MmSystemLockPagesCount);
    }
}



CODE_SEG("INIT")
BOOLEAN
NTAPI
MmArmInitSystem(
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

CODE_SEG("INIT")
VOID
NTAPI
MiInitializeSessionSpaceLayout(VOID);

CODE_SEG("INIT")
NTSTATUS
NTAPI
MiInitMachineDependent(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

CODE_SEG("INIT")
VOID
NTAPI
MiComputeColorInformation(
    VOID
);

CODE_SEG("INIT")
VOID
NTAPI
MiMapPfnDatabase(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

CODE_SEG("INIT")
VOID
NTAPI
MiInitializeColorTables(
    VOID
);

CODE_SEG("INIT")
VOID
NTAPI
MiInitializePfnDatabase(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
NTAPI
MiInitializeSessionWsSupport(
    VOID
);

VOID
NTAPI
MiInitializeSessionIds(
    VOID
);

CODE_SEG("INIT")
BOOLEAN
NTAPI
MiInitializeMemoryEvents(
    VOID
);

CODE_SEG("INIT")
PFN_NUMBER
NTAPI
MxGetNextPage(
    IN PFN_NUMBER PageCount
);

CODE_SEG("INIT")
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
    IN ULONG FaultCode,
    IN PVOID Address,
    IN KPROCESSOR_MODE Mode,
    IN PVOID TrapInformation
);

NTSTATUS
FASTCALL
MiCheckPdeForPagedPool(
    IN PVOID Address
);

CODE_SEG("INIT")
VOID
NTAPI
MiInitializeNonPagedPoolThresholds(
    VOID
);

CODE_SEG("INIT")
VOID
NTAPI
MiInitializePoolEvents(
    VOID
);

CODE_SEG("INIT")
VOID                      //
NTAPI                     //
InitializePool(           //
    IN POOL_TYPE PoolType,// FIXFIX: This should go in ex.h after the pool merge
    IN ULONG Threshold    //
);                        //

// FIXFIX: THIS ONE TOO
CODE_SEG("INIT")
VOID
NTAPI
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

CODE_SEG("INIT")
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
    IN PMMPDE PointerPde,
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
    IN PVOID PteAddress,
    IN PFN_NUMBER PteFrame
);

VOID
NTAPI
MiDecrementShareCount(
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

CODE_SEG("INIT")
VOID
NTAPI
MiInitializeDriverLargePageList(
    VOID
);

CODE_SEG("INIT")
VOID
NTAPI
MiInitializeLargePageSupport(
    VOID
);

CODE_SEG("INIT")
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

TABLE_SEARCH_RESULT
NTAPI
MiCheckForConflictingNode(
    IN ULONG_PTR StartVpn,
    IN ULONG_PTR EndVpn,
    IN PMM_AVL_TABLE Table,
    OUT PMMADDRESS_NODE *NodeOrParent
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

TABLE_SEARCH_RESULT
NTAPI
MiFindEmptyAddressRangeInTree(
    IN SIZE_T Length,
    IN ULONG_PTR Alignment,
    IN PMM_AVL_TABLE Table,
    OUT PMMADDRESS_NODE *PreviousVad,
    OUT PULONG_PTR Base
);

NTSTATUS
NTAPI
MiCheckSecuredVad(
    IN PMMVAD Vad,
    IN PVOID Base,
    IN SIZE_T Size,
    IN ULONG ProtectionMask
);

VOID
NTAPI
MiInsertVad(
    _Inout_ PMMVAD Vad,
    _Inout_ PMM_AVL_TABLE VadRoot);

NTSTATUS
NTAPI
MiInsertVadEx(
    _Inout_ PMMVAD Vad,
    _In_ ULONG_PTR *BaseAddress,
    _In_ SIZE_T ViewSize,
    _In_ ULONG_PTR HighestAddress,
    _In_ ULONG_PTR Alignment,
    _In_ ULONG AllocationType);

VOID
NTAPI
MiInsertBasedSection(
    IN PSECTION Section
);

NTSTATUS
NTAPI
MiRosUnmapViewOfSection(
    IN PEPROCESS Process,
    IN PVOID BaseAddress,
    IN BOOLEAN SkipDebuggerNotify
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

VOID
NTAPI
MiSessionRemoveProcess(
    VOID
);

VOID
NTAPI
MiReleaseProcessReferenceToSessionDataPage(
    IN PMM_SESSION_SPACE SessionGlobal
);

VOID
NTAPI
MiSessionAddProcess(
    IN PEPROCESS NewProcess
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

VOID
NTAPI
MiDeletePte(
    IN PMMPTE PointerPte,
    IN PVOID VirtualAddress,
    IN PEPROCESS CurrentProcess,
    IN PMMPTE PrototypePte
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

VOID
NTAPI
MiDeleteARM3Section(
    PVOID ObjectBody
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
MiRosUnmapViewInSystemSpace(
    IN PVOID MappedBase
);

VOID
NTAPI
MiMakePdeExistAndMakeValid(
    IN PMMPDE PointerPde,
    IN PEPROCESS TargetProcess,
    IN KIRQL OldIrql
);

VOID
NTAPI
MiWriteProtectSystemImage(
    _In_ PVOID ImageBase);

//
// MiRemoveZeroPage will use inline code to zero out the page manually if only
// free pages are available. In some scenarios, we don't/can't run that piece of
// code and would rather only have a real zero page. If we can't have a zero page,
// then we'd like to have our own code to grab a free page and zero it out, by
// using MiRemoveAnyPage. This macro implements this.
//
FORCEINLINE
PFN_NUMBER
MiRemoveZeroPageSafe(IN ULONG Color)
{
    if (MmFreePagesByColor[ZeroedPageList][Color].Flink != LIST_HEAD) return MiRemoveZeroPage(Color);
    return 0;
}

#if (_MI_PAGING_LEVELS == 2)
FORCEINLINE
BOOLEAN
MiSynchronizeSystemPde(PMMPDE PointerPde)
{
    ULONG Index;

    /* Get the Index from the PDE */
    Index = ((ULONG_PTR)PointerPde & (SYSTEM_PD_SIZE - 1)) / sizeof(MMPTE);
    if (PointerPde->u.Hard.Valid != 0)
    {
        NT_ASSERT(PointerPde->u.Long == MmSystemPagePtes[Index].u.Long);
        return TRUE;
    }

    if (MmSystemPagePtes[Index].u.Hard.Valid == 0)
    {
        return FALSE;
    }

    /* Copy the PDE from the double-mapped system page directory */
    MI_WRITE_VALID_PDE(PointerPde, MmSystemPagePtes[Index]);

    /* Make sure we re-read the PDE and PTE */
    KeMemoryBarrierWithoutFence();

    /* Return success */
    return TRUE;
}
#endif

#if _MI_PAGING_LEVELS == 2
FORCEINLINE
USHORT
MiIncrementPageTableReferences(IN PVOID Address)
{
    PUSHORT RefCount;

    RefCount = &MmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(Address)];

    *RefCount += 1;
    ASSERT(*RefCount <= PTE_PER_PAGE);
    return *RefCount;
}

FORCEINLINE
USHORT
MiDecrementPageTableReferences(IN PVOID Address)
{
    PUSHORT RefCount;

    RefCount = &MmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(Address)];

    *RefCount -= 1;
    ASSERT(*RefCount < PTE_PER_PAGE);
    return *RefCount;
}
#else
FORCEINLINE
USHORT
MiIncrementPageTableReferences(IN PVOID Address)
{
    PMMPDE PointerPde = MiAddressToPde(Address);
    PMMPFN Pfn;

    /* We should not tinker with this one. */
    ASSERT(PointerPde != (PMMPDE)PXE_SELFMAP);
    DPRINT("Incrementing %p from %p\n", Address, _ReturnAddress());

    /* Make sure we're locked */
    ASSERT(PsGetCurrentThread()->OwnsProcessWorkingSetExclusive);

    /* If we're bumping refcount, then it must be valid! */
    ASSERT(PointerPde->u.Hard.Valid == 1);

    /* This lies on the PFN */
    Pfn = MiGetPfnEntry(PFN_FROM_PDE(PointerPde));
    Pfn->OriginalPte.u.Soft.UsedPageTableEntries++;

    ASSERT(Pfn->OriginalPte.u.Soft.UsedPageTableEntries <= PTE_PER_PAGE);

    return Pfn->OriginalPte.u.Soft.UsedPageTableEntries;
}

FORCEINLINE
USHORT
MiDecrementPageTableReferences(IN PVOID Address)
{
    PMMPDE PointerPde = MiAddressToPde(Address);
    PMMPFN Pfn;

    /* We should not tinker with this one. */
    ASSERT(PointerPde != (PMMPDE)PXE_SELFMAP);

    DPRINT("Decrementing %p from %p\n", PointerPde, _ReturnAddress());

    /* Make sure we're locked */
    ASSERT(PsGetCurrentThread()->OwnsProcessWorkingSetExclusive);

    /* If we're decreasing refcount, then it must be valid! */
    ASSERT(PointerPde->u.Hard.Valid == 1);

    /* This lies on the PFN */
    Pfn = MiGetPfnEntry(PFN_FROM_PDE(PointerPde));

    ASSERT(Pfn->OriginalPte.u.Soft.UsedPageTableEntries != 0);
    Pfn->OriginalPte.u.Soft.UsedPageTableEntries--;

    ASSERT(Pfn->OriginalPte.u.Soft.UsedPageTableEntries < PTE_PER_PAGE);

    return Pfn->OriginalPte.u.Soft.UsedPageTableEntries;
}
#endif

#ifdef __cplusplus
} // extern "C"
#endif

FORCEINLINE
VOID
MiDeletePde(
    _In_ PMMPDE PointerPde,
    _In_ PEPROCESS CurrentProcess)
{
    /* Only for user-mode ones */
    ASSERT(MiIsUserPde(PointerPde));

    /* Kill this one as a PTE */
    MiDeletePte((PMMPTE)PointerPde, MiPdeToPte(PointerPde), CurrentProcess, NULL);
#if _MI_PAGING_LEVELS >= 3
    /* Cascade down */
    if (MiDecrementPageTableReferences(MiPdeToPte(PointerPde)) == 0)
    {
        MiDeletePte(MiPdeToPpe(PointerPde), PointerPde, CurrentProcess, NULL);
#if _MI_PAGING_LEVELS == 4
        if (MiDecrementPageTableReferences(PointerPde) == 0)
        {
            MiDeletePte(MiPdeToPxe(PointerPde), MiPdeToPpe(PointerPde), CurrentProcess, NULL);
        }
#endif
    }
#endif
}

/* EOF */
