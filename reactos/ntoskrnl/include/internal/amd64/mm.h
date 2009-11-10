/*
 * Lowlevel memory managment definitions
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_AMD64_MM_H
#define __NTOSKRNL_INCLUDE_INTERNAL_AMD64_MM_H

struct _EPROCESS;

PULONG64
FORCEINLINE
MmGetPageDirectory(VOID)
{
    return (PULONG64)__readcr3();
}

#define PAGE_MASK(x)		((x)&(~0xfff))
#define PAE_PAGE_MASK(x)	((x)&(~0xfffLL))

#define HYPER_SPACE 0xFFFFF70000000000ULL
#define HYPER_SPACE_END   0xFFFFF77FFFFFFFFFULL

/* Base addresses of PTE and PDE */
//#define PAGETABLE_MAP       PTE_BASE
//#define PAGEDIRECTORY_MAP   (0xc0000000 + (PAGETABLE_MAP / (1024)))

/* Converting address to a corresponding PDE or PTE entry */
#define MiAddressToPxe(x) \
    ((PMMPTE)((((((ULONG64)(x)) >> PXI_SHIFT) & PXI_MASK) << 3) + PXE_BASE))
#define MiAddressToPpe(x) \
    ((PMMPTE)((((((ULONG64)(x)) >> PPI_SHIFT) & PPI_MASK) << 3) + PPE_BASE))
#define MiAddressToPde(x) \
    ((PMMPTE)((((((ULONG64)(x)) >> PDI_SHIFT) & PDI_MASK_AMD64) << 3) + PDE_BASE))
#define MiAddressToPte(x) \
    ((PMMPTE)((((((ULONG64)(x)) >> PTI_SHIFT) & PTI_MASK_AMD64) << 3) + PTE_BASE))

/* Convert a PTE into a corresponding address */
#define MiPteToAddress(PTE) ((PVOID)((ULONG64)(PTE) << 9))

//#define ADDR_TO_PAGE_TABLE(v) (((ULONG)(v)) / (1024 * PAGE_SIZE))
//#define ADDR_TO_PDE_OFFSET(v) ((((ULONG)(v)) / (1024 * PAGE_SIZE)))
//#define ADDR_TO_PTE_OFFSET(v)  ((((ULONG)(v)) % (1024 * PAGE_SIZE)) / PAGE_SIZE)

#define VAtoPXI(va) ((((ULONG64)va) >> PXI_SHIFT) & 0x1FF)
#define VAtoPPI(va) ((((ULONG64)va) >> PPI_SHIFT) & 0x1FF)
#define VAtoPDI(va) ((((ULONG64)va) >> PDI_SHIFT) & 0x1FF)
#define VAtoPTI(va) ((((ULONG64)va) >> PTI_SHIFT) & 0x1FF)

/// MIARM.H 

/* Easy accessing PFN in PTE */
#define PFN_FROM_PTE(v) ((v)->u.Hard.PageFrameNumber)

// FIXME, only copied from x86
#define MI_MAKE_LOCAL_PAGE(x)      ((x)->u.Hard.Global = 0)
#define MI_MAKE_DIRTY_PAGE(x)      ((x)->u.Hard.Dirty = 1)
#define MI_PAGE_DISABLE_CACHE(x)   ((x)->u.Hard.CacheDisable = 1)
#define MI_PAGE_WRITE_THROUGH(x)   ((x)->u.Hard.WriteThrough = 1)
#define MI_PAGE_WRITE_COMBINED(x)  ((x)->u.Hard.WriteThrough = 0)
#define MI_IS_PAGE_WRITEABLE(x)    ((x)->u.Hard.Write == 1)
#define MI_IS_PAGE_COPY_ON_WRITE(x)((x)->u.Hard.CopyOnWrite == 1)
#define MI_IS_PAGE_DIRTY(x)        ((x)->u.Hard.Dirty == 1)
#define MI_MAKE_OWNER_PAGE(x)      ((x)->u.Hard.Owner = 1)
#define MI_MAKE_WRITE_PAGE(x)      ((x)->u.Hard.Write = 1)


#define MI_MIN_PAGES_FOR_NONPAGED_POOL_TUNING ((255*1024*1024) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_TUNING         ((19*1024*1024) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_BOOST          ((32*1024*1024) >> PAGE_SHIFT)
#define MI_MAX_INIT_NONPAGED_POOL_SIZE         (128 * 1024 * 1024)
#define MI_MAX_NONPAGED_POOL_SIZE              (128 * 1024 * 1024)
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

#define MI_PAGED_POOL_START                    (PVOID)0xFFFFFA8000000000ULL
#define MI_NONPAGED_POOL_END                   (PVOID)0xFFFFFAE000000000ULL
#define MI_DEBUG_MAPPING                       (PVOID)0xFFFFFFFF80000000ULL // FIXME
 
#define MM_HIGHEST_VAD_ADDRESS \
    (PVOID)((ULONG_PTR)MM_HIGHEST_USER_ADDRESS - (16 * PAGE_SIZE))


//
// FIXFIX: These should go in ex.h after the pool merge
//
#define POOL_LISTS_PER_PAGE (PAGE_SIZE / sizeof(LIST_ENTRY))
#define BASE_POOL_TYPE_MASK 1
#define POOL_MAX_ALLOC (PAGE_SIZE - (sizeof(POOL_HEADER) + sizeof(LIST_ENTRY)))

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_AMD64_MM_H */
