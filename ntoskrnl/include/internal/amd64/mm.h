/*
 * Lowlevel memory managment definitions
 */

#pragma once

/* Helper macros */
#define PAGE_MASK(x)		((x)&(~0xfff))
#define PAE_PAGE_MASK(x)	((x)&(~0xfffLL))

/* Memory layout base addresses */
#define HYPER_SPACE                            0xFFFFF70000000000ULL
#define HYPER_SPACE_END                        0xFFFFF77FFFFFFFFFULL
#define MI_SESSION_SPACE_MINIMUM        (PVOID)0xFFFFF90000000000ULL
#define MI_SESSION_VIEW_END             (PVOID)0xFFFFF97FFF000000ULL
#define MI_SESSION_SPACE_END            (PVOID)0xFFFFF98000000000ULL
#define MI_SYSTEM_PTE_START             (PVOID)0xFFFFFAA000000000ULL
#define MI_PAGED_POOL_START             (PVOID)0xFFFFFA8000000000ULL
#define MI_NON_PAGED_SYSTEM_START_MIN          0xFFFFFAA000000000ULL
#define MI_PFN_DATABASE                 (PVOID)0xFFFFFAC000000000ULL
#define MI_NONPAGED_POOL_END            (PVOID)0xFFFFFAE000000000ULL
#define MI_DEBUG_MAPPING                (PVOID)0xFFFFFFFF80000000ULL // FIXME
#define MI_HIGHEST_SYSTEM_ADDRESS       (PVOID)0xFFFFFFFFFFFFFFFFULL
#define MI_SYSTEM_CACHE_WS_START        (PVOID)0xFFFFF78000001000ULL // CHECKME

/* Memory sizes */
#define MI_MIN_PAGES_FOR_NONPAGED_POOL_TUNING ((255*1024*1024) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_TUNING         ((19*1024*1024) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_BOOST          ((32*1024*1024) >> PAGE_SHIFT)
#define MI_MAX_INIT_NONPAGED_POOL_SIZE         (128ULL * 1024 * 1024 * 1024)
#define MI_MAX_NONPAGED_POOL_SIZE              (128ULL * 1024 * 1024 * 1024)
#define MI_MAX_FREE_PAGE_LISTS                 4
#define MI_MIN_INIT_PAGED_POOLSIZE             (32 * 1024 * 1024)
#define MI_SESSION_VIEW_SIZE                   (20 * 1024 * 1024)
#define MI_SESSION_POOL_SIZE                   (16 * 1024 * 1024)
#define MI_SESSION_IMAGE_SIZE                  (8 * 1024 * 1024)
#define MI_SESSION_WORKING_SET_SIZE            (4 * 1024 * 1024)
#define MI_SESSION_SIZE                        (MI_SESSION_VIEW_SIZE + \
                                                MI_SESSION_POOL_SIZE + \
                                                MI_SESSION_IMAGE_SIZE + \
                                                MI_SESSION_WORKING_SET_SIZE)
#define MI_SYSTEM_VIEW_SIZE                    (16 * 1024 * 1024)
#define MI_NUMBER_SYSTEM_PTES 22000

PULONG64
FORCEINLINE
MmGetPageDirectory(VOID)
{
    return (PULONG64)__readcr3();
}

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

/* Convert a PTE into a corresponding address */
PVOID
FORCEINLINE
MiPteToAddress(PMMPTE Pte)
{
    /* Use signed math */
    LONG64 Temp = (LONG64)Pte;
    Temp <<= 25;
    Temp >>= 16;
    return (PVOID)Temp;
}

BOOLEAN
FORCEINLINE
MiIsPdeForAddressValid(PVOID Address)
{
    return ((MiAddressToPxe(Address)->u.Hard.Valid) &&
            (MiAddressToPpe(Address)->u.Hard.Valid) &&
            (MiAddressToPde(Address)->u.Hard.Valid));
}

#define ADDR_TO_PAGE_TABLE(v) (((ULONG_PTR)(v)) / (512 * PAGE_SIZE))
#define ADDR_TO_PDE_OFFSET(v) ((((ULONG_PTR)(v)) / (512 * PAGE_SIZE)))
#define ADDR_TO_PTE_OFFSET(v)  ((((ULONG_PTR)(v)) % (512 * PAGE_SIZE)) / PAGE_SIZE)

#define VAtoPXI(va) ((((ULONG64)va) >> PXI_SHIFT) & 0x1FF)
#define VAtoPPI(va) ((((ULONG64)va) >> PPI_SHIFT) & 0x1FF)
#define VAtoPDI(va) ((((ULONG64)va) >> PDI_SHIFT) & 0x1FF)
#define VAtoPTI(va) ((((ULONG64)va) >> PTI_SHIFT) & 0x1FF)

/* We don't use these hacks */
VOID
FORCEINLINE
MmUpdatePageDir(PEPROCESS Process, PVOID Address, ULONG Size)
{
    /* Nothing to do */
}

VOID
FORCEINLINE
MmInitGlobalKernelPageDirectory(VOID)
{
    /* Nothing to do */
}

#define IS_ALIGNED(addr, align) (((ULONG64)(addr) & (align - 1)) == 0)
#define IS_PAGE_ALIGNED(addr) IS_ALIGNED(addr, PAGE_SIZE)

/* Easy accessing PFN in PTE */
#define PFN_FROM_PTE(v) ((v)->u.Hard.PageFrameNumber)

// FIXME, only copied from x86
#define MI_MAKE_LOCAL_PAGE(x)      ((x)->u.Hard.Global = 0)
#define MI_MAKE_DIRTY_PAGE(x)      ((x)->u.Hard.Dirty = 1)
#define MI_PAGE_DISABLE_CACHE(x)   ((x)->u.Hard.CacheDisable = 1)
#define MI_PAGE_WRITE_THROUGH(x)   ((x)->u.Hard.WriteThrough = 1)
#define MI_PAGE_WRITE_COMBINED(x)  ((x)->u.Hard.WriteThrough = 0)
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

// FIXME!!!
#define PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(x) \
    ((x) / (4*1024*1024))

#define PAGE_TO_SECTION_PAGE_TABLE_OFFSET(x) \
    ((((x)) % (4*1024*1024)) / (4*1024))

#define NR_SECTION_PAGE_TABLES              1024
#define NR_SECTION_PAGE_ENTRIES             1024

//#define TEB_BASE                            0x7FFDE000

#define MI_HYPERSPACE_PTES                  (256 - 1)
#define MI_ZERO_PTES                        (32)
#define MI_MAPPING_RANGE_START              (ULONG)HYPER_SPACE
#define MI_MAPPING_RANGE_END                (MI_MAPPING_RANGE_START + \
                                             MI_HYPERSPACE_PTES * PAGE_SIZE)
#define MI_ZERO_PTE                         (PMMPTE)(MI_MAPPING_RANGE_END + \
                                             PAGE_SIZE)

/* On x86, these two are the same */
#define MMPDE MMPTE
#define PMMPDE PMMPTE

/*
* FIXME - different architectures have different cache line sizes...
*/
#define MM_CACHE_LINE_SIZE                  32

