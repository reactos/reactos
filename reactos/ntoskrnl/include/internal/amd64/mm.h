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
#define PAGETABLE_MAP       (0xc0000000)
#define PAGEDIRECTORY_MAP   (0xc0000000 + (PAGETABLE_MAP / (1024)))

/* Converting address to a corresponding PDE or PTE entry */
#define MiAddressToPde(x) \
    ((PMMPTE)(((((ULONG64)(x)) >> 22) << 2) + PAGEDIRECTORY_MAP))
#define MiAddressToPte(x) \
    ((PMMPTE)(((((ULONG64)(x)) >> 12) << 2) + PAGETABLE_MAP))

//#define ADDR_TO_PAGE_TABLE(v) (((ULONG)(v)) / (1024 * PAGE_SIZE))
//#define ADDR_TO_PDE_OFFSET(v) ((((ULONG)(v)) / (1024 * PAGE_SIZE)))
//#define ADDR_TO_PTE_OFFSET(v)  ((((ULONG)(v)) % (1024 * PAGE_SIZE)) / PAGE_SIZE)

#define VAtoPXI(va) ((((ULONG64)va) >> PXI_SHIFT) & 0x1FF)
#define VAtoPPI(va) ((((ULONG64)va) >> PPI_SHIFT) & 0x1FF)
#define VAtoPDI(va) ((((ULONG64)va) >> PDI_SHIFT) & 0x1FF)
#define VAtoPTI(va) ((((ULONG64)va) >> PTI_SHIFT) & 0x1FF)


/* Easy accessing PFN in PTE */
#define PFN_FROM_PTE(v) ((v)->u.Hard.PageFrameNumber)

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_AMD64_MM_H */
