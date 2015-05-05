/*
 * kernel internal memory management definitions for amd64
 */
#pragma once

#define _MI_PAGING_LEVELS                   4

/* Memory layout base addresses */
#define MI_LOWEST_VAD_ADDRESS           (PVOID)0x0000000000010000ULL
#define MI_USER_PROBE_ADDRESS           (PVOID)0x000007FFFFFF0000ULL
#define MI_DEFAULT_SYSTEM_RANGE_START   (PVOID)0xFFFF080000000000ULL
#define MI_REAL_SYSTEM_RANGE_START             0xFFFF800000000000ULL
#define HYPER_SPACE                            0xFFFFF70000000000ULL
#define HYPER_SPACE_END                        0xFFFFF77FFFFFFFFFULL
#define MI_SYSTEM_CACHE_WS_START               0xFFFFF78000001000ULL
#define MI_PAGED_POOL_START             (PVOID)0xFFFFF8A000000000ULL
//#define MI_PAGED_POOL_END                      0xFFFFF8BFFFFFFFFFULL
//#define MI_SESSION_SPACE_START                 0xFFFFF90000000000ULL
#define MI_SESSION_VIEW_END                    0xFFFFF97FFF000000ULL
#define MI_SESSION_SPACE_END                   0xFFFFF97FFFFFFFFFULL
#define MM_SYSTEM_SPACE_START                  0xFFFFF98000000000ULL
#define MI_PFN_DATABASE                        0xFFFFFA8000000000ULL
#define MI_NONPAGED_POOL_END            (PVOID)0xFFFFFFFFFFBFFFFFULL
#define MI_HIGHEST_SYSTEM_ADDRESS       (PVOID)0xFFFFFFFFFFFFFFFFULL

/* WOW64 address definitions */
#define MM_HIGHEST_USER_ADDRESS_WOW64   0x7FFEFFFF
#define MM_SYSTEM_RANGE_START_WOW64     0x80000000

#define MI_DEBUG_MAPPING                (PVOID)0xFFFFFFFF80000000ULL // FIXME
#define MI_NON_PAGED_SYSTEM_START_MIN   MM_SYSTEM_SPACE_START // FIXME
#define MI_SYSTEM_PTE_START             MM_SYSTEM_SPACE_START
#define MI_SYSTEM_PTE_END               (MI_SYSTEM_PTE_START + MI_NUMBER_SYSTEM_PTES * PAGE_SIZE - 1)
#define MI_SYSTEM_PTE_BASE              (PVOID)MiAddressToPte(KSEG0_BASE)
#define MM_HIGHEST_VAD_ADDRESS          (PVOID)((ULONG_PTR)MM_HIGHEST_USER_ADDRESS - (16 * PAGE_SIZE))
#define MI_MAPPING_RANGE_START          HYPER_SPACE
#define MI_MAPPING_RANGE_END            (MI_MAPPING_RANGE_START + MI_HYPERSPACE_PTES * PAGE_SIZE)
#define MI_DUMMY_PTE                        (MI_MAPPING_RANGE_END + PAGE_SIZE)
#define MI_VAD_BITMAP                       (MI_DUMMY_PTE + PAGE_SIZE)
#define MI_WORKING_SET_LIST                 (MI_VAD_BITMAP + PAGE_SIZE)

/* Memory sizes */
#define MI_MIN_PAGES_FOR_NONPAGED_POOL_TUNING ((255*1024*1024) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_TUNING        ((19*1024*1024) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_BOOST         ((32*1024*1024) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_BOOST_BOOST   ((256*1024*1024) >> PAGE_SHIFT)
#define MI_MIN_INIT_PAGED_POOLSIZE            (32 * 1024 * 1024)
#define MI_MAX_INIT_NONPAGED_POOL_SIZE        (128ULL * 1024 * 1024 * 1024)
#define MI_MAX_NONPAGED_POOL_SIZE             (128ULL * 1024 * 1024 * 1024)
#define MI_SYSTEM_VIEW_SIZE                   (16 * 1024 * 1024)
#define MI_MIN_SECONDARY_COLORS               8
#define MI_SECONDARY_COLORS                   64
#define MI_MAX_SECONDARY_COLORS               1024
#define MI_MIN_ALLOCATION_FRAGMENT            (4 * _1KB)
#define MI_ALLOCATION_FRAGMENT                (64 * _1KB)
#define MI_MAX_ALLOCATION_FRAGMENT            (2  * _1MB)
#define MI_SESSION_WORKING_SET_SIZE           (4 * 1024 * 1024)
#define MI_SESSION_VIEW_SIZE                  (20 * 1024 * 1024)
#define MI_SESSION_POOL_SIZE                  (16 * 1024 * 1024)
#define MI_SESSION_IMAGE_SIZE                 (8 * 1024 * 1024)
#define MI_SESSION_SIZE                       (MI_SESSION_VIEW_SIZE + \
                                               MI_SESSION_POOL_SIZE + \
                                               MI_SESSION_IMAGE_SIZE + \
                                               MI_SESSION_WORKING_SET_SIZE)

#define MmSystemRangeStart ((PVOID)MI_REAL_SYSTEM_RANGE_START)

/* Misc constants */
#define MI_NUMBER_SYSTEM_PTES               22000
#define MI_MAX_FREE_PAGE_LISTS              4
#define MI_HYPERSPACE_PTES                  (256 - 1)
#define MI_ZERO_PTES                        (32)
/* FIXME - different architectures have different cache line sizes... */
#define MI_MAX_ZERO_BITS                    53

/* Helper macros */
#define IS_ALIGNED(addr, align) (((ULONG64)(addr) & (align - 1)) == 0)
#define IS_PAGE_ALIGNED(addr) IS_ALIGNED(addr, PAGE_SIZE)

#define MiIsPteOnPdeBoundary(PointerPte) \
    ((((ULONG_PTR)PointerPte) & (PAGE_SIZE - 1)) == 0)
#define MiIsPteOnPpeBoundary(PointerPte) \
    ((((ULONG_PTR)PointerPte) & (PDE_PER_PAGE * PAGE_SIZE - 1)) == 0)
#define MiIsPteOnPxeBoundary(PointerPte) \
    ((((ULONG_PTR)PointerPte) & (PPE_PER_PAGE * PDE_PER_PAGE * PAGE_SIZE - 1)) == 0)

/* MMPTE related defines */
#define MM_EMPTY_PTE_LIST  ((ULONG64)0xFFFFFFFF)
#define MM_EMPTY_LIST  ((ULONG_PTR)-1)

#define ADDR_TO_PAGE_TABLE(v) ((ULONG)(((ULONG_PTR)(v)) / (512 * PAGE_SIZE)))
#define ADDR_TO_PDE_OFFSET(v) ((ULONG)((((ULONG_PTR)(v)) / (512 * PAGE_SIZE))))
#define ADDR_TO_PTE_OFFSET(v)  ((ULONG)((((ULONG_PTR)(v)) % (512 * PAGE_SIZE)) / PAGE_SIZE))

#define MiGetPdeOffset ADDR_TO_PDE_OFFSET

#define VAtoPXI(va) ((((ULONG64)va) >> PXI_SHIFT) & 0x1FF)
#define VAtoPPI(va) ((((ULONG64)va) >> PPI_SHIFT) & 0x1FF)
#define VAtoPDI(va) ((((ULONG64)va) >> PDI_SHIFT) & 0x1FF)
#define VAtoPTI(va) ((((ULONG64)va) >> PTI_SHIFT) & 0x1FF)

/* Easy accessing PFN in PTE */
#define PFN_FROM_PTE(v) ((v)->u.Hard.PageFrameNumber)
#define PFN_FROM_PDE(v) ((v)->u.Hard.PageFrameNumber)
#define PFN_FROM_PPE(v) ((v)->u.Hard.PageFrameNumber)
#define PFN_FROM_PXE(v) ((v)->u.Hard.PageFrameNumber)

// FIXME, only copied from x86
#define MI_MAKE_LOCAL_PAGE(x)      ((x)->u.Hard.Global = 0)
#define MI_MAKE_DIRTY_PAGE(x)      ((x)->u.Hard.Dirty = 1)
#define MI_MAKE_ACCESSED_PAGE(x)   ((x)->u.Hard.Accessed = 1)
#define MI_PAGE_DISABLE_CACHE(x)   ((x)->u.Hard.CacheDisable = 1)
#define MI_PAGE_WRITE_THROUGH(x)   ((x)->u.Hard.WriteThrough = 1)
#define MI_PAGE_WRITE_COMBINED(x)  ((x)->u.Hard.WriteThrough = 0)
#define MI_IS_PAGE_LARGE(x)        ((x)->u.Hard.LargePage == 1)
#if !defined(CONFIG_SMP)
#define MI_IS_PAGE_WRITEABLE(x)    ((x)->u.Hard.Write == 1)
#else
#define MI_IS_PAGE_WRITEABLE(x)    ((x)->u.Hard.Writable == 1)
#endif
#define MI_IS_PAGE_COPY_ON_WRITE(x)((x)->u.Hard.CopyOnWrite == 1)
#define MI_IS_PAGE_DIRTY(x)        ((x)->u.Hard.Dirty == 1)
#define MI_MAKE_OWNER_PAGE(x)      ((x)->u.Hard.Owner = 1)
#if !defined(CONFIG_SMP)
#define MI_MAKE_WRITE_PAGE(x)      ((x)->u.Hard.Write = 1)
#else
#define MI_MAKE_WRITE_PAGE(x)      ((x)->u.Hard.Writable = 1)
#endif

/* On x64, these are the same */
#define MMPDE MMPTE
#define PMMPDE PMMPTE
#define MMPPE MMPTE
#define PMMPPE PMMPTE
#define MMPXE MMPTE
#define PMMPXE PMMPTE
#define MI_WRITE_VALID_PPE MI_WRITE_VALID_PTE

#define ValidKernelPpe ValidKernelPde

PMMPTE
FORCEINLINE
MiAddressToPxe(PVOID Address)
{
    ULONG64 Offset = (ULONG64)Address >> (PXI_SHIFT - 3);
    Offset &= PXI_MASK << 3;
    return (PMMPTE)(PXE_BASE + Offset);
}

PMMPTE
FORCEINLINE
MiAddressToPpe(PVOID Address)
{
    ULONG64 Offset = (ULONG64)Address >> (PPI_SHIFT - 3);
    Offset &= 0x3FFFF << 3;
    return (PMMPTE)(PPE_BASE + Offset);
}

PMMPTE
FORCEINLINE
_MiAddressToPde(PVOID Address)
{
    ULONG64 Offset = (ULONG64)Address >> (PDI_SHIFT - 3);
    Offset &= 0x7FFFFFF << 3;
    return (PMMPTE)(PDE_BASE + Offset);
}
#define MiAddressToPde(x) _MiAddressToPde((PVOID)(x))

PMMPTE
FORCEINLINE
_MiAddressToPte(PVOID Address)
{
    ULONG64 Offset = (ULONG64)Address >> (PTI_SHIFT - 3);
    Offset &= 0xFFFFFFFFFULL << 3;
    return (PMMPTE)(PTE_BASE + Offset);
}
#define MiAddressToPte(x) _MiAddressToPte((PVOID)(x))

ULONG
FORCEINLINE
MiAddressToPti(PVOID Address)
{
    return ((((ULONG64)Address) >> PTI_SHIFT) & 0x1FF);
}
#define MiAddressToPteOffset(x) MiAddressToPti(x) // FIXME: bad name

ULONG
FORCEINLINE
MiAddressToPxi(PVOID Address)
{
    return ((((ULONG64)Address) >> PXI_SHIFT) & 0x1FF);
}


/* Convert a PTE into a corresponding address */
PVOID
FORCEINLINE
MiPteToAddress(PMMPTE PointerPte)
{
    /* Use signed math */
    return (PVOID)(((LONG64)PointerPte << 25) >> 16);
}

PVOID
FORCEINLINE
MiPdeToAddress(PMMPTE PointerPde)
{
    /* Use signed math */
    return (PVOID)(((LONG64)PointerPde << 34) >> 16);
}

PVOID
FORCEINLINE
MiPpeToAddress(PMMPTE PointerPpe)
{
    /* Use signed math */
    return (PVOID)(((LONG64)PointerPpe << 43) >> 16);
}

PVOID
FORCEINLINE
MiPxeToAddress(PMMPTE PointerPxe)
{
    /* Use signed math */
    return (PVOID)(((LONG64)PointerPxe << 52) >> 16);
}

BOOLEAN
FORCEINLINE
MiIsPdeForAddressValid(PVOID Address)
{
    return ((MiAddressToPxe(Address)->u.Hard.Valid) &&
            (MiAddressToPpe(Address)->u.Hard.Valid) &&
            (MiAddressToPde(Address)->u.Hard.Valid));
}

#define MiPdeToPte(PDE) ((PMMPTE)MiPteToAddress(PDE))
#define MiPteToPde(PTE) ((PMMPDE)MiAddressToPte(PTE))
#define MiPdeToPpe(Pde) ((PMMPPE)MiAddressToPte(Pde))

/* Sign extend 48 bits */
#define MiProtoPteToPte(x) (PMMPTE)(((LONG64)(x)->u.Long) >> 16)

//
// Decodes a Prototype PTE into the underlying PTE
//
#define MiSubsectionPteToSubsection(x)                              \
        (PMMPTE)((x)->u.Subsect.SubsectionAddress >> 16)

FORCEINLINE
VOID
MI_MAKE_SUBSECTION_PTE(IN PMMPTE NewPte,
                       IN PVOID Segment)
{
    ASSERT(FALSE);
}

FORCEINLINE
VOID
MI_MAKE_PROTOTYPE_PTE(IN PMMPTE NewPte,
                      IN PMMPTE PointerPte)
{
    /* Store the Address */
    NewPte->u.Long = (ULONG64)PointerPte << 16;

    /* Mark this as a prototype PTE */
    NewPte->u.Proto.Prototype = 1;

    ASSERT(MiProtoPteToPte(NewPte) == PointerPte);
}

FORCEINLINE
BOOLEAN
MI_IS_MAPPED_PTE(PMMPTE PointerPte)
{
    /// FIXME
    __debugbreak();
    return ((PointerPte->u.Long & 0xFFFFFC01) != 0);
}

VOID
FORCEINLINE
MmInitGlobalKernelPageDirectory(VOID)
{
    /* Nothing to do */
}

