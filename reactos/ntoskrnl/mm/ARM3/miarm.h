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

#define MI_SYSTEM_VIEW_SIZE                     (16 * _1MB)

#define MI_SYSTEM_CACHE_WS_START                (PVOID)0xC0C00000
#define MI_PAGED_POOL_START                     (PVOID)0xE1000000
#define MI_NONPAGED_POOL_END                    (PVOID)0xFFBE0000
#define MI_DEBUG_MAPPING                        (PVOID)0xFFBFF000

// on AMD64 this would be MiAddressToPte(MM_KSEG0_BASE)
#define MI_SYSTEM_PTE_BASE                      (PVOID)MiAddressToPte(NULL)

#define MI_MIN_SECONDARY_COLORS                 8
#define MI_SECONDARY_COLORS                     64
#define MI_MAX_SECONDARY_COLORS                 1024

#define MI_MIN_ALLOCATION_FRAGMENT              (4 * _1KB)
#define MI_ALLOCATION_FRAGMENT                  (64 * _1KB)
#define MI_MAX_ALLOCATION_FRAGMENT              (2  * _1MB)

#define MM_HIGHEST_VAD_ADDRESS \
    (PVOID)((ULONG_PTR)MM_HIGHEST_USER_ADDRESS - (16 * PAGE_SIZE))

/* The range 0x10000->0x7FEFFFFF is reserved for the ROSMM MAREA Allocator */
#define MI_LOWEST_VAD_ADDRESS                   (PVOID)0x7FF00000

#endif /* !_M_AMD64 */

/* Make the code cleaner with some definitions for size multiples */
#define _1KB (1024u)
#define _1MB (1024 * _1KB)
#define _1GB (1024 * _1MB)

/* Area mapped by a PDE */
#define PDE_MAPPED_VA  (PTE_COUNT * PAGE_SIZE)

/* Size of a page table */
#define PT_SIZE  (PTE_COUNT * sizeof(MMPTE))

/* Architecture specific count of PDEs in a directory, and count of PTEs in a PT */
#ifdef _M_IX86
#define PD_COUNT  1
#define PDE_COUNT 1024
#define PTE_COUNT 1024
#elif _M_ARM
#define PD_COUNT  1
#define PDE_COUNT 4096
#define PTE_COUNT 256
#else
#define PD_COUNT  PPE_PER_PAGE
#define PDE_COUNT PDE_PER_PAGE
#define PTE_COUNT PTE_PER_PAGE
#endif

#ifdef _M_IX86
#define IMAGE_FILE_MACHINE_NATIVE   IMAGE_FILE_MACHINE_I386
#elif _M_ARM
#define IMAGE_FILE_MACHINE_NATIVE   IMAGE_FILE_MACHINE_ARM
#elif _M_AMD64
#define IMAGE_FILE_MACHINE_NATIVE   IMAGE_FILE_MACHINE_AMD64
#else
#error Define these please!
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
#define PTE_READONLY            0
#define PTE_EXECUTE             0 // Not worrying about NX yet
#define PTE_EXECUTE_READ        0 // Not worrying about NX yet
#define PTE_READWRITE           0x2
#define PTE_WRITECOPY           0x200
#define PTE_EXECUTE_READWRITE   0x0
#define PTE_EXECUTE_WRITECOPY   0x200
//
// Cache flags
//
#define PTE_ENABLE_CACHE        0
#define PTE_DISABLE_CACHE       0x10
#define PTE_WRITECOMBINED_CACHE 0x10
#elif defined(_M_ARM)
#else
#error Define these please!
#endif
static const
ULONG
MmProtectToPteMask[32] =
{
    //
    // These are the base MM_ protection flags
    //
    0,
    PTE_READONLY            | PTE_ENABLE_CACHE,
    PTE_EXECUTE             | PTE_ENABLE_CACHE,
    PTE_EXECUTE_READ        | PTE_ENABLE_CACHE,
    PTE_READWRITE           | PTE_ENABLE_CACHE,
    PTE_WRITECOPY           | PTE_ENABLE_CACHE,
    PTE_EXECUTE_READWRITE   | PTE_ENABLE_CACHE,
    PTE_EXECUTE_WRITECOPY   | PTE_ENABLE_CACHE,
    //
    // These OR in the MM_NOCACHE flag
    //
    0,
    PTE_READONLY            | PTE_DISABLE_CACHE,
    PTE_EXECUTE             | PTE_DISABLE_CACHE,
    PTE_EXECUTE_READ        | PTE_DISABLE_CACHE,
    PTE_READWRITE           | PTE_DISABLE_CACHE,
    PTE_WRITECOPY           | PTE_DISABLE_CACHE,
    PTE_EXECUTE_READWRITE   | PTE_DISABLE_CACHE,
    PTE_EXECUTE_WRITECOPY   | PTE_DISABLE_CACHE,
    //
    // These OR in the MM_DECOMMIT flag, which doesn't seem supported on x86/64/ARM
    //
    0,
    PTE_READONLY            | PTE_ENABLE_CACHE,
    PTE_EXECUTE             | PTE_ENABLE_CACHE,
    PTE_EXECUTE_READ        | PTE_ENABLE_CACHE,
    PTE_READWRITE           | PTE_ENABLE_CACHE,
    PTE_WRITECOPY           | PTE_ENABLE_CACHE,
    PTE_EXECUTE_READWRITE   | PTE_ENABLE_CACHE,
    PTE_EXECUTE_WRITECOPY   | PTE_ENABLE_CACHE,
    //
    // These OR in the MM_NOACCESS flag, which seems to enable WriteCombining?
    //
    0,
    PTE_READONLY            | PTE_WRITECOMBINED_CACHE,
    PTE_EXECUTE             | PTE_WRITECOMBINED_CACHE,
    PTE_EXECUTE_READ        | PTE_WRITECOMBINED_CACHE,
    PTE_READWRITE           | PTE_WRITECOMBINED_CACHE,
    PTE_WRITECOPY           | PTE_WRITECOMBINED_CACHE,
    PTE_EXECUTE_READWRITE   | PTE_WRITECOMBINED_CACHE,
    PTE_EXECUTE_WRITECOPY   | PTE_WRITECOMBINED_CACHE,
};
 
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
#define MM_PTE_SOFTWARE_PROTECTION_BITS   5
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
// PFN List Sentinel
//
#define LIST_HEAD 0xFFFFFFFF

//
// Special IRQL value (found in assertions)
//
#define MM_NOIRQL (KIRQL)0xFFFFFFFF

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
            ULONG PreviousSize:8;
            ULONG PoolIndex:8;
            ULONG BlockSize:8;
            ULONG PoolType:8;
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

extern ULONG ExpNumberOfPagedPools;
extern POOL_DESCRIPTOR NonPagedPoolDescriptor;
extern PPOOL_DESCRIPTOR ExpPagedPoolDescriptor[16 + 1];
extern PVOID PoolTrackTable;

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
    ULONG BasePage;
    ULONG PageCount;
} PHYSICAL_MEMORY_RUN, *PPHYSICAL_MEMORY_RUN;

typedef struct _PHYSICAL_MEMORY_DESCRIPTOR
{
    ULONG NumberOfRuns;
    ULONG NumberOfPages;
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

extern MMPTE HyperTemplatePte;
extern MMPDE ValidKernelPde;
extern MMPTE ValidKernelPte;
extern MMPDE DemandZeroPde;
extern BOOLEAN MmLargeSystemCache;
extern BOOLEAN MmZeroPageFile;
extern BOOLEAN MmProtectFreedNonPagedPool;
extern BOOLEAN MmTrackLockedPages;
extern BOOLEAN MmTrackPtes;
extern BOOLEAN MmDynamicPfn;
extern BOOLEAN MmMirroring;
extern BOOLEAN MmMakeLowMemory;
extern BOOLEAN MmEnforceWriteProtection;
extern ULONG MmAllocationFragment;
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
extern RTL_BITMAP MiPfnBitMap;
extern KGUARDED_MUTEX MmPagedPoolMutex;
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
extern PMMPTE MmSystemPagePtes;
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
extern ULONG_PTR MmNumberOfSystemPtes;
extern ULONG MmMaximumNonPagedPoolPercent;
extern ULONG MmLargeStackSize;
extern PMMCOLOR_TABLES MmFreePagesByColor[FreePageList + 1];
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
extern PFN_NUMBER MiExpansionPoolPagesInitialCharge;
extern PFN_NUMBER MmResidentAvailablePages;
extern PFN_NUMBER MmResidentAvailableAtInit;
extern ULONG MmTotalFreeSystemPtes[MaximumPtePoolTypes];
extern PFN_NUMBER MmTotalSystemDriverPages;
extern PVOID MiSessionImageStart;
extern PVOID MiSessionImageEnd;
extern PMMPTE MiHighestUserPte;
extern PMMPDE MiHighestUserPde;
extern PFN_NUMBER MmSystemPageDirectory[PD_COUNT];

#define MI_PFN_TO_PFNENTRY(x)     (&MmPfnDatabase[1][x])
#define MI_PFNENTRY_TO_PFN(x)     (x - MmPfnDatabase[1])

//
// Figures out the hardware bits for a PTE
//
ULONG
FORCEINLINE
MiDetermineUserGlobalPteMask(IN PMMPTE PointerPte)
{
    MMPTE TempPte;
    
    /* Start fresh */
    TempPte.u.Long = 0;
    
    /* Make it valid and accessed */
    TempPte.u.Hard.Valid = TRUE;
    TempPte.u.Hard.Accessed = TRUE;
    
    /* Is this for user-mode? */
    if ((PointerPte <= MiHighestUserPte) ||
        ((PointerPte >= MiAddressToPde(NULL)) && (PointerPte <= MiHighestUserPde)))
    {
        /* Set the owner bit */
        TempPte.u.Hard.Owner = TRUE;
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
                            IN ULONG ProtectionMask,
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
                     IN ULONG ProtectionMask,
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
                          IN ULONG ProtectionMask,
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

NTSTATUS
NTAPI
MmArmInitSystem(
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

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
MiInsertInListTail(
    IN PMMPFNLIST ListHead,
    IN PMMPFN Entry
);

VOID
NTAPI
MiInsertZeroListAtBack(
    IN PFN_NUMBER PageIndex
);

VOID
NTAPI
MiUnlinkFreeOrZeroedPage(
    IN PMMPFN Entry
);

PMMPFN
NTAPI
MiRemoveHeadList(
    IN PMMPFNLIST ListHead
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
MiInsertPageInFreeList(
    IN PFN_NUMBER PageFrameIndex
);

PFN_NUMBER
NTAPI
MiDeleteSystemPageableVm(
    IN PMMPTE PointerPte,
    IN PFN_NUMBER PageCount,
    IN ULONG Flags,
    OUT PPFN_NUMBER ValidPages
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

NTSTATUS
NTAPI
MiFindEmptyAddressRangeDownTree(
    IN SIZE_T Length,
    IN ULONG_PTR BoundaryAddress,
    IN ULONG_PTR Alignment,
    IN PMM_AVL_TABLE Table,
    OUT PULONG_PTR Base
);

VOID
NTAPI
MiInsertNode(
    IN PMMADDRESS_NODE NewNode,
    IN PMM_AVL_TABLE Table
);

/* EOF */
