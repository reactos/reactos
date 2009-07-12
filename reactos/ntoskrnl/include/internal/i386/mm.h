/*
 * Lowlevel memory managment definitions
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_I386_MM_H
#define __NTOSKRNL_INCLUDE_INTERNAL_I386_MM_H

struct _EPROCESS;
PULONG MmGetPageDirectory(VOID);

#define PAGE_MASK(x)		((x)&(~0xfff))
#define PAE_PAGE_MASK(x)	((x)&(~0xfffLL))

/* Base addresses of PTE and PDE */
#define PAGETABLE_MAP       (0xc0000000)
#define PAGEDIRECTORY_MAP   (0xc0000000 + (PAGETABLE_MAP / (1024)))
#define HYPER_SPACE		                    (0xC0400000)

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

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_I386_MM_H */
