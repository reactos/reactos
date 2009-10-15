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

/* Base addresses of PTE and PDE */
//#define PAGETABLE_MAP       PTE_BASE
//#define PAGEDIRECTORY_MAP   (0xc0000000 + (PAGETABLE_MAP / (1024)))

/* Converting address to a corresponding PDE or PTE entry */
#define MiAddressToPxe(x) \
    ((PMMPTE)(((((ULONG64)(x)) >> PXI_SHIFT) << 3) + PXE_BASE))
#define MiAddressToPpe(x) \
    ((PMMPTE)(((((ULONG64)(x)) >> PPI_SHIFT) << 3) + PPE_BASE))
#define MiAddressToPde(x) \
    ((PMMPTE)(((((ULONG64)(x)) >> PDI_SHIFT) << 3) + PDE_BASE))
#define MiAddressToPte(x) \
    ((PMMPTE)(((((ULONG64)(x)) >> PTI_SHIFT) << 3) + PTE_BASE))

/* Convert a PTE into a corresponding address */
#define MiPteToAddress(PTE) ((PVOID)((ULONG64)(PTE) << 9))

//#define ADDR_TO_PAGE_TABLE(v) (((ULONG)(v)) / (1024 * PAGE_SIZE))
//#define ADDR_TO_PDE_OFFSET(v) ((((ULONG)(v)) / (1024 * PAGE_SIZE)))
//#define ADDR_TO_PTE_OFFSET(v)  ((((ULONG)(v)) % (1024 * PAGE_SIZE)) / PAGE_SIZE)

#define VAtoPXI(va) ((((ULONG64)va) >> PXI_SHIFT) & 0x1FF)
#define VAtoPPI(va) ((((ULONG64)va) >> PPI_SHIFT) & 0x1FF)
#define VAtoPDI(va) ((((ULONG64)va) >> PDI_SHIFT) & 0x1FF)
#define VAtoPTI(va) ((((ULONG64)va) >> PTI_SHIFT) & 0x1FF)


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

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_AMD64_MM_H */
