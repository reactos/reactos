/*
 * Lowlevel memory managment definitions
 */

#pragma once

struct _EPROCESS;
PULONG MmGetPageDirectory(VOID);

#define PAGE_MASK(x)		((x)&(~0xfff))
#define PAE_PAGE_MASK(x)	((x)&(~0xfffLL))

/* Base addresses of PTE and PDE */
#define PAGETABLE_MAP       (0xc0000000)
#define PAGEDIRECTORY_MAP   (0xc0000000 + (PAGETABLE_MAP / (1024)))

#define PTE_BASE    0xC0000000
#define PDE_BASE    0xC0300000
#define PTE_TOP     0xC03FFFFF
#define HYPER_SPACE 0xC0400000

/* Converting address to a corresponding PDE or PTE entry */
#define MiAddressToPde(x) \
    ((PMMPTE)(((((ULONG)(x)) >> 22) << 2) + PAGEDIRECTORY_MAP))
#define MiAddressToPte(x) \
    ((PMMPTE)(((((ULONG)(x)) >> 12) << 2) + PAGETABLE_MAP))
#define MiAddressToPteOffset(x) \
    ((((ULONG)(x)) << 10) >> 22)

//
// Convert a PTE into a corresponding address
//
#define MiPteToAddress(PTE) ((PVOID)((ULONG)(PTE) << 10))

#define ADDR_TO_PAGE_TABLE(v) (((ULONG)(v)) / (1024 * PAGE_SIZE))
#define ADDR_TO_PDE_OFFSET(v) ((((ULONG)(v)) / (1024 * PAGE_SIZE)))
#define ADDR_TO_PTE_OFFSET(v)  ((((ULONG)(v)) % (1024 * PAGE_SIZE)) / PAGE_SIZE)

/* Easy accessing PFN in PTE */
#define PFN_FROM_PTE(v) ((v)->u.Hard.PageFrameNumber)

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

#define PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(x) \
    ((x) / (4*1024*1024))

#define PAGE_TO_SECTION_PAGE_TABLE_OFFSET(x) \
    ((((x)) % (4*1024*1024)) / (4*1024))

#define NR_SECTION_PAGE_TABLES              1024
#define NR_SECTION_PAGE_ENTRIES             1024

#define TEB_BASE                            0x7FFDE000

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
