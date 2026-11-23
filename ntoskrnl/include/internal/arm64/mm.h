/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 memory management definitions
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#define _MI_PAGING_LEVELS 4
#define _MI_HAS_NO_EXECUTE 1

/* Same layout as amd64, the loader shares the root table between TTBR0 and TTBR1 */
#define MI_USER_PROBE_ADDRESS           (PVOID)0x000007FFFFFF0000ULL
#define MI_DEFAULT_SYSTEM_RANGE_START   (PVOID)0xFFFF080000000000ULL
#define MI_REAL_SYSTEM_RANGE_START             0xFFFF800000000000ULL
#define HYPER_SPACE                            0xFFFFF70000000000ULL
#define HYPER_SPACE_END                        0xFFFFF77FFFFFFFFFULL
#define MI_SYSTEM_CACHE_WS_START               0xFFFFF78000001000ULL
#define MM_SYSTEM_SPACE_START                  0xFFFFF88000000000ULL
#define MI_DEBUG_MAPPING                (PVOID)0xFFFFF89FFFFFF000ULL
#define MI_PAGED_POOL_START             (PVOID)0xFFFFF8A000000000ULL
#define MI_SESSION_SPACE_END                   0xFFFFF98000000000ULL
#define MI_SYSTEM_CACHE_START                  0xFFFFF98000000000ULL
#define MI_SYSTEM_CACHE_END                    0xFFFFFA7FFFFFFFFFULL
#define MI_PFN_DATABASE                        0xFFFFFA8000000000ULL
#define MI_NONPAGED_POOL_END            (PVOID)0xFFFFFFFFFFBFFFFFULL
#define MI_HIGHEST_SYSTEM_ADDRESS       (PVOID)0xFFFFFFFFFFFFFFFFULL
#define MmSystemRangeStart              ((PVOID)MI_REAL_SYSTEM_RANGE_START)

/* WOW64 address definitions */
#define MM_HIGHEST_USER_ADDRESS_WOW64   0x7FFEFFFF
#define MM_SYSTEM_RANGE_START_WOW64     0x80000000

/* The size of the virtual memory area that is mapped using a single PDE */
#define PDE_MAPPED_VA (PTE_PER_PAGE * PAGE_SIZE)

/* Misc address definitions */
#define MI_SYSTEM_PTE_BASE              (PVOID)MiAddressToPte(KSEG0_BASE)
#define MM_HIGHEST_VAD_ADDRESS          (PVOID)((ULONG_PTR)MM_HIGHEST_USER_ADDRESS - (16 * PAGE_SIZE))
#define MI_MAPPING_RANGE_START          HYPER_SPACE
#define MI_MAPPING_RANGE_END            (MI_MAPPING_RANGE_START + MI_HYPERSPACE_PTES * PAGE_SIZE)
#define MI_DUMMY_PTE                    (MI_MAPPING_RANGE_END + PAGE_SIZE)
#define MI_VAD_BITMAP                   (MI_DUMMY_PTE + PAGE_SIZE)
#define MI_WORKING_SET_LIST             (MI_VAD_BITMAP + PAGE_SIZE)

/* Memory sizes */
#define MI_MIN_PAGES_FOR_NONPAGED_POOL_TUNING   ((255 * _1MB) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_TUNING          ((19 * _1MB) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_BOOST           ((32 * _1MB) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_BOOST_BOOST     ((256 * _1MB) >> PAGE_SHIFT)
#define MI_MIN_INIT_PAGED_POOLSIZE              (32 * _1MB)
#define MI_MAX_INIT_NONPAGED_POOL_SIZE          (128ULL * 1024 * 1024 * 1024)
#define MI_MAX_NONPAGED_POOL_SIZE               (128ULL * 1024 * 1024 * 1024)
#define MI_SYSTEM_VIEW_SIZE                     (512 * _1MB)
#define MI_SESSION_VIEW_SIZE                    (512 * _1MB)
#define MI_SESSION_POOL_SIZE                    (64 * _1MB)
#define MI_SESSION_IMAGE_SIZE                   (16 * _1MB)
#define MI_SESSION_WORKING_SET_SIZE             (16 * _1MB)
#define MI_SESSION_SIZE                         (MI_SESSION_VIEW_SIZE + \
                                                 MI_SESSION_POOL_SIZE + \
                                                 MI_SESSION_IMAGE_SIZE + \
                                                 MI_SESSION_WORKING_SET_SIZE)
#define MI_MIN_ALLOCATION_FRAGMENT              (4 * _1KB)
#define MI_ALLOCATION_FRAGMENT                  (64 * _1KB)
#define MI_MAX_ALLOCATION_FRAGMENT              (2  * _1MB)

/* Misc constants */
#define MM_PTE_SOFTWARE_PROTECTION_BITS         5
#define MI_MIN_SECONDARY_COLORS                 8
#define MI_SECONDARY_COLORS                     64
#define MI_MAX_SECONDARY_COLORS                 1024
#define MI_NUMBER_SYSTEM_PTES                   (22000 * 22)
#define MI_MAX_FREE_PAGE_LISTS                  4
#define MI_HYPERSPACE_PTES                      (256 - 1)
#define MI_ZERO_PTES                            (32)
#define MI_MAX_ZERO_BITS                        53
#define SESSION_POOL_LOOKASIDES                 21

/* MMPTE related defines */
#define MM_EMPTY_PTE_LIST  ((ULONG64)0xFFFFFFFF)
#define MM_EMPTY_LIST  ((ULONG_PTR)-1)

/* MAIR_EL1 slots the loader programs */
#define MI_CACHE_TYPE_CACHED            0
#define MI_CACHE_TYPE_WRITE_COMBINED    1
#define MI_CACHE_TYPE_DEVICE            3

/* Easy accessing PFN in PTE */
#define PFN_FROM_PTE(v) ((v)->u.Hard.PageFrameNumber)
#define PFN_FROM_PDE(v) ((v)->u.Hard.PageFrameNumber)
#define PFN_FROM_PPE(v) ((v)->u.Hard.PageFrameNumber)
#define PFN_FROM_PXE(v) ((v)->u.Hard.PageFrameNumber)

/*
 * Writable is a software bit, the hardware only sees AP[2] (NotDirty).
 * A writable page stays read only until the first write marks it dirty.
 */
#define MI_MAKE_DIRTY_PAGE(x)      ((x)->u.Hard.NotDirty = ((x)->u.Hard.Writable == 0))
#define MI_MAKE_CLEAN_PAGE(x)      ((x)->u.Hard.NotDirty = 1)
#define MI_MAKE_ACCESSED_PAGE(x)   ((x)->u.Hard.Accessed = 1)
#define MI_IS_PAGE_LARGE(x)        ((x)->u.Hard.NotLargePage == 0)
#define MI_IS_PAGE_WRITEABLE(x)    ((x)->u.Hard.Writable == 1)
#define MI_IS_PAGE_COPY_ON_WRITE(x)((x)->u.Hard.CopyOnWrite == 1)
#define MI_IS_PAGE_EXECUTABLE(x)   \
    (((x)->u.Hard.Owner ? (x)->u.Hard.UserNoExecute : (x)->u.Hard.PrivilegedNoExecute) == 0)
#define MI_IS_PAGE_DIRTY(x)        ((x)->u.Hard.NotDirty == 0)
#define MI_MAKE_WRITE_PAGE(x)      ((x)->u.Hard.Writable = 1)
#define MI_MAKE_READ_ONLY_PAGE(x)  ((x)->u.Hard.Writable = 0, (x)->u.Hard.NotDirty = 1)
#define MI_MAKE_EXECUTABLE_PAGE(x) ((x)->u.Hard.PrivilegedNoExecute = 0, (x)->u.Hard.UserNoExecute = 0)
#define MI_MAKE_NO_EXECUTE_PAGE(x) ((x)->u.Hard.PrivilegedNoExecute = 1, (x)->u.Hard.UserNoExecute = 1)
#define MI_IS_PAGE_GLOBAL(x)       ((x)->u.Hard.NonGlobal == 0)

/* User pages are tagged with the ASID and never run at EL1 */
#define MI_MAKE_OWNER_PAGE(x)              \
    ((x)->u.Hard.Owner = 1,                \
     (x)->u.Hard.NonGlobal = 1,            \
     (x)->u.Hard.PrivilegedNoExecute = 1)

#define MI_MAKE_PTE_CACHED(Pte)         ((Pte)->u.Hard.CacheType = MI_CACHE_TYPE_CACHED)
#define MI_MAKE_PTE_NON_CACHED(Pte)     ((Pte)->u.Hard.CacheType = MI_CACHE_TYPE_DEVICE)
#define MI_MAKE_PTE_WRITE_COMBINE(Pte)  ((Pte)->u.Hard.CacheType = MI_CACHE_TYPE_WRITE_COMBINED)

/* Fault status from ESR_EL1, passed to MmAccessFault as the fault code */
#define MI_IS_NOT_PRESENT_FAULT(FaultCode)  !BooleanFlagOn(FaultCode, 0x00000001)
#define MI_IS_WRITE_ACCESS(FaultCode)        BooleanFlagOn(FaultCode, 0x00000002)
#define MI_IS_INSTRUCTION_FETCH(FaultCode)   BooleanFlagOn(FaultCode, 0x00000010)

/* On ARM64, these are the same */
#define MI_WRITE_VALID_PPE MI_WRITE_VALID_PTE
#define ValidKernelPpe ValidKernelPde

/* Each level indexes 9 bits of the address, the self map repeats the walk */
#define MI_VA_LEVEL_SHIFT(Level)    (PTI_SHIFT + 9 * (Level))
#define MI_VA_SIGNIFICANT_BITS      48

FORCEINLINE
PMMPTE
MiArm64AddressToEntry(
    _In_ PVOID Address,
    _In_ ULONG Level,
    _In_ ULONG_PTR TableBase)
{
    ULONG64 Index;

    Index = ((ULONG64)Address & ((1ULL << MI_VA_SIGNIFICANT_BITS) - 1)) >> MI_VA_LEVEL_SHIFT(Level);
    return (PMMPTE)TableBase + Index;
}

#define MiAddressToPte(x) MiArm64AddressToEntry((PVOID)(x), 0, PTE_BASE)
#define MiAddressToPde(x) MiArm64AddressToEntry((PVOID)(x), 1, PDE_BASE)
#define MiAddressToPpe(x) MiArm64AddressToEntry((PVOID)(x), 2, PPE_BASE)
#define MiAddressToPxe(x) MiArm64AddressToEntry((PVOID)(x), 3, PXE_BASE)

/* Convert an address to the index of its entry inside the table */
FORCEINLINE
ULONG
MiAddressToPti(
    _In_ PVOID Address)
{
    return (ULONG)(((ULONG64)Address >> PTI_SHIFT) & PTI_MASK_ARM64);
}
#define MiAddressToPteOffset(x) MiAddressToPti(x)

FORCEINLINE
ULONG
MiAddressToPdi(
    _In_ PVOID Address)
{
    return (ULONG)(((ULONG64)Address >> PDI_SHIFT) & PDI_MASK_ARM64);
}
#define MiAddressToPdeOffset(x) MiAddressToPdi(x)
#define MiGetPdeOffset(x) MiAddressToPdi(x)

FORCEINLINE
ULONG
MiAddressToPxi(
    _In_ PVOID Address)
{
    return (ULONG)(((ULONG64)Address >> PXI_SHIFT) & PXI_MASK);
}

/* An entry address maps back by dropping one level and sign extending bit 47 */
FORCEINLINE
PVOID
MiArm64EntryToAddress(
    _In_ PVOID Entry,
    _In_ ULONG Level)
{
    LONG64 Value;

    Value = (LONG64)((ULONG64)Entry << (MI_VA_LEVEL_SHIFT(Level) - 3));
    Value <<= (64 - MI_VA_SIGNIFICANT_BITS);
    return (PVOID)(Value >> (64 - MI_VA_SIGNIFICANT_BITS));
}

#define MiPteToAddress(x) MiArm64EntryToAddress((x), 0)
#define MiPdeToAddress(x) MiArm64EntryToAddress((x), 1)
#define MiPpeToAddress(x) MiArm64EntryToAddress((x), 2)
#define MiPxeToAddress(x) MiArm64EntryToAddress((x), 3)

/* Walking up one level is the same as translating the entry address again */
#define MiPdeToPte(x) ((PMMPTE)MiPteToAddress(x))
#define MiPpeToPte(x) ((PMMPTE)MiPdeToAddress(x))
#define MiPxeToPte(x) ((PMMPTE)MiPpeToAddress(x))
#define MiPteToPde(x) ((PMMPDE)MiAddressToPte(x))
#define MiPteToPpe(x) ((PMMPPE)MiAddressToPde(x))
#define MiPteToPxe(x) ((PMMPXE)MiAddressToPpe(x))
#define MiPdeToPpe(x) ((PMMPPE)MiAddressToPte(x))
#define MiPdeToPxe(x) ((PMMPXE)MiAddressToPde(x))

/* Check P*E boundaries */
#define MiIsPteOnPdeBoundary(PointerPte) \
    ((((ULONG_PTR)(PointerPte)) & (PAGE_SIZE - 1)) == 0)
#define MiIsPteOnPpeBoundary(PointerPte) \
    ((((ULONG_PTR)(PointerPte)) & (PDE_PER_PAGE * PAGE_SIZE - 1)) == 0)
#define MiIsPteOnPxeBoundary(PointerPte) \
    ((((ULONG_PTR)(PointerPte)) & (PPE_PER_PAGE * PDE_PER_PAGE * PAGE_SIZE - 1)) == 0)

/* Prototype and subsection PTEs keep a sign extended 48 bit address in bits 16 to 63 */
#define MiProtoPteToPte(x) \
    (PMMPTE)(((LONG64)(x)->u.Long) >> 16)

#define MiSubsectionPteToSubsection(x) \
    (PMMPTE)((LONG64)(x)->u.Subsect.SubsectionAddress)

FORCEINLINE
VOID
MI_MAKE_SUBSECTION_PTE(
    _Out_ PMMPTE NewPte,
    _In_ PVOID Segment)
{
    NewPte->u.Long = 0;
    NewPte->u.Subsect.Prototype = 1;
    NewPte->u.Subsect.SubsectionAddress = ((ULONG_PTR)Segment & 0x0000FFFFFFFFFFFFULL);
}

FORCEINLINE
VOID
MI_MAKE_PROTOTYPE_PTE(
    _Out_ PMMPTE NewPte,
    _In_ PMMPTE PointerPte)
{
    NewPte->u.Long = (ULONG64)PointerPte << 16;
    NewPte->u.Proto.Prototype = 1;
    ASSERT(MiProtoPteToPte(NewPte) == PointerPte);
}

FORCEINLINE
BOOLEAN
MI_IS_MAPPED_PTE(
    _In_ PMMPTE PointerPte)
{
    return ((PointerPte->u.Hard.Valid != 0) ||
            (PointerPte->u.Proto.Prototype != 0) ||
            (PointerPte->u.Trans.Transition != 0) ||
            (PointerPte->u.Hard.PageFrameNumber != 0));
}

FORCEINLINE
BOOLEAN
MiIsPdeForAddressValid(
    _In_ PVOID Address)
{
    return ((MiAddressToPxe(Address)->u.Hard.Valid) &&
            (MiAddressToPpe(Address)->u.Hard.Valid) &&
            (MiAddressToPde(Address)->u.Hard.Valid));
}
