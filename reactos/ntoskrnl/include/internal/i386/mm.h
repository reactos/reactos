/*
 * kernel internal memory management definitions for x86
 */
#pragma once

#ifdef _PAE_
#define _MI_PAGING_LEVELS 3
#else
#define _MI_PAGING_LEVELS 2
#endif

/* MMPTE related defines */
#define MM_EMPTY_PTE_LIST  ((ULONG)0xFFFFF)
#define MM_EMPTY_LIST  ((ULONG_PTR)-1)
/* FIXME: These are different for PAE */
#define PTE_BASE    0xC0000000
#define PDE_BASE    0xC0300000
#define PDE_TOP     0xC0300FFF
#define PTE_TOP     0xC03FFFFF
#define HYPER_SPACE 0xC0400000
#define HYPER_SPACE_END 0xC07FFFFF

#define PTE_PER_PAGE 0x400
#define PDE_PER_PAGE 0x400

/* Converting address to a corresponding PDE or PTE entry */
#define MiAddressToPde(x) \
    ((PMMPDE)(((((ULONG)(x)) >> 22) << 2) + PDE_BASE))
#define MiAddressToPte(x) \
    ((PMMPTE)(((((ULONG)(x)) >> 12) << 2) + PTE_BASE))
#define MiAddressToPteOffset(x) \
    ((((ULONG)(x)) << 10) >> 22)

/* Convert a PTE into a corresponding address */
#define MiPteToAddress(PTE) ((PVOID)((ULONG)(PTE) << 10))
#define MiPdeToAddress(PDE) ((PVOID)((ULONG)(PDE) << 20))
#define MiPdeToPte(PDE) ((PMMPTE)MiPteToAddress(PDE))
#define MiPteToPde(PTE) ((PMMPDE)MiAddressToPte(PTE))

#define ADDR_TO_PAGE_TABLE(v)  (((ULONG)(v)) / (1024 * PAGE_SIZE))
#define ADDR_TO_PDE_OFFSET(v)  (((ULONG)(v)) / (1024 * PAGE_SIZE))
#define ADDR_TO_PTE_OFFSET(v) ((((ULONG)(v)) % (1024 * PAGE_SIZE)) / PAGE_SIZE)

#define MiGetPdeOffset ADDR_TO_PDE_OFFSET

/* Easy accessing PFN in PTE */
#define PFN_FROM_PTE(v) ((v)->u.Hard.PageFrameNumber)

/* Macros for portable PTE modification */
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

#define MI_HYPERSPACE_PTES                  (256 - 1)
#define MI_ZERO_PTES                        (32)
#define MI_MAPPING_RANGE_START              (ULONG)HYPER_SPACE
#define MI_MAPPING_RANGE_END                (MI_MAPPING_RANGE_START + \
	                                              MI_HYPERSPACE_PTES * PAGE_SIZE)
#define MI_DUMMY_PTE                        (PMMPTE)((ULONG_PTR)MI_MAPPING_RANGE_END + \
	                                              PAGE_SIZE)
#define MI_VAD_BITMAP                       (PMMPTE)((ULONG_PTR)MI_DUMMY_PTE + \
	                                              PAGE_SIZE)
#define MI_WORKING_SET_LIST                 (PMMPTE)((ULONG_PTR)MI_VAD_BITMAP + \
	                                              PAGE_SIZE)

/* On x86, these two are the same */
#define MMPDE MMPTE
#define PMMPDE PMMPTE

