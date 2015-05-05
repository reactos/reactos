#pragma once

#define _MI_PAGING_LEVELS 2

#define PDE_SHIFT 20

//
// Number of bits corresponding to the area that a coarse page table entry represents (4KB)
//
#define PTE_SHIFT 12
#define PTE_SIZE (1 << PTE_SHIFT)

//
// Number of bits corresponding to the area that a coarse page table occupies (1KB)
//
#define CPT_SHIFT 10
#define CPT_SIZE  (1 << CPT_SHIFT)

/* MMPTE related defines */
#define MM_EMPTY_PTE_LIST  ((ULONG)0xFFFFF)
#define MM_EMPTY_LIST  ((ULONG_PTR)-1)

//
// Base Addresses
//
#define PTE_BASE    0xC0000000
#define PTE_TOP     0xC03FFFFF
#define PDE_BASE    0xC0400000
#define PDE_TOP     0xC04FFFFF
#define HYPER_SPACE 0xC0500000

#if 0
typedef struct _HARDWARE_PDE_ARMV6
{
    ULONG Valid:1;     // Only for small pages
    ULONG LargePage:1; // Note, if large then Valid = 0
    ULONG Buffered:1;
    ULONG Cached:1;
    ULONG NoExecute:1;
    ULONG Domain:4;
    ULONG Ecc:1;
    ULONG PageFrameNumber:22;
} HARDWARE_PDE_ARMV6, *PHARDWARE_PDE_ARMV6;

typedef struct _HARDWARE_LARGE_PTE_ARMV6
{
    ULONG Valid:1;     // Only for small pages
    ULONG LargePage:1; // Note, if large then Valid = 0
    ULONG Buffered:1;
    ULONG Cached:1;
    ULONG NoExecute:1;
    ULONG Domain:4;
    ULONG Ecc:1;
    ULONG Accessed:1;
    ULONG Owner:1;
    ULONG CacheAttributes:3;
    ULONG ReadOnly:1;
    ULONG Shared:1;
    ULONG NonGlobal:1;
    ULONG SuperLagePage:1;
    ULONG Reserved:1;
    ULONG PageFrameNumber:12;
} HARDWARE_LARGE_PTE_ARMV6, *PHARDWARE_LARGE_PTE_ARMV6;

typedef struct _HARDWARE_PTE_ARMV6
{
    ULONG NoExecute:1;
    ULONG Valid:1;
    ULONG Buffered:1;
    ULONG Cached:1;
    ULONG Accessed:1;
    ULONG Owner:1;
    ULONG CacheAttributes:3;
    ULONG ReadOnly:1;
    ULONG Shared:1;
    ULONG NonGlobal:1;
    ULONG PageFrameNumber:20;
} HARDWARE_PTE_ARMV6, *PHARDWARE_PTE_ARMV6;

C_ASSERT(sizeof(HARDWARE_PDE_ARMV6) == sizeof(ULONG));
C_ASSERT(sizeof(HARDWARE_LARGE_PTE_ARMV6) == sizeof(ULONG));
C_ASSERT(sizeof(HARDWARE_PTE_ARMV6) == sizeof(ULONG));
#endif

/* For FreeLDR */
typedef struct _PAGE_TABLE_ARM
{
    HARDWARE_PTE_ARMV6 Pte[1024];
} PAGE_TABLE_ARM, *PPAGE_TABLE_ARM;

typedef struct _PAGE_DIRECTORY_ARM
{
    union
    {
        HARDWARE_PDE_ARMV6 Pde[4096];
        HARDWARE_LARGE_PTE_ARMV6 Pte[4096];
    };
} PAGE_DIRECTORY_ARM, *PPAGE_DIRECTORY_ARM;

C_ASSERT(sizeof(PAGE_TABLE_ARM) == PAGE_SIZE);
C_ASSERT(sizeof(PAGE_DIRECTORY_ARM) == (4 * PAGE_SIZE));

typedef enum _ARM_DOMAIN
{
    FaultDomain,
    ClientDomain,
    InvalidDomain,
    ManagerDomain
} ARM_DOMAIN;

#define MI_MAKE_LOCAL_PAGE(x)      ((x)->u.Hard.NonGlobal = 1)
#define MI_MAKE_DIRTY_PAGE(x)
#define MI_MAKE_ACCESSED_PAGE(x)
#define MI_MAKE_OWNER_PAGE(x)      ((x)->u.Hard.Owner = 1)
#define MI_MAKE_WRITE_PAGE(x)      ((x)->u.Hard.ReadOnly = 0)
#define MI_PAGE_DISABLE_CACHE(x)   ((x)->u.Hard.Cached = 0)
#define MI_PAGE_WRITE_THROUGH(x)   ((x)->u.Hard.Buffered = 0)
#define MI_PAGE_WRITE_COMBINED(x)  ((x)->u.Hard.Buffered = 1)
#define MI_IS_PAGE_WRITEABLE(x)    ((x)->u.Hard.ReadOnly == 0)
#define MI_IS_PAGE_COPY_ON_WRITE(x)FALSE
#define MI_IS_PAGE_DIRTY(x)        TRUE
#define MI_IS_PAGE_LARGE(x)        FALSE

/* Easy accessing PFN in PTE */
#define PFN_FROM_PTE(v) ((v)->u.Hard.PageFrameNumber)

#define NR_SECTION_PAGE_TABLES              1024
#define NR_SECTION_PAGE_ENTRIES             256

/* See PDR definition */
#define MI_HYPERSPACE_PTES                  (256 - 1)
#define MI_ZERO_PTES                        (32)
#define MI_MAPPING_RANGE_START              ((ULONG)HYPER_SPACE)
#define MI_MAPPING_RANGE_END                (MI_MAPPING_RANGE_START + \
                                             MI_HYPERSPACE_PTES * PAGE_SIZE)
#define MI_ZERO_PTE                         (PMMPTE)(MI_MAPPING_RANGE_END + \
                                             PAGE_SIZE)
#define MI_DUMMY_PTE                        (PMMPTE)(MI_MAPPING_RANGE_END + \
                                             PAGE_SIZE)
#define MI_VAD_BITMAP                       (PMMPTE)(MI_DUMMY_PTE + \
                                             PAGE_SIZE)
#define MI_WORKING_SET_LIST                 (PMMPTE)(MI_VAD_BITMAP + \
                                             PAGE_SIZE)

/* Retrives the PDE entry for the given VA */
#define MiGetPdeAddress(x) ((PMMPDE)(PDE_BASE + (((ULONG)(x) >> 20) << 2)))
#define MiAddressToPde(x)  MiGetPdeAddress(x)

/* Retrieves the PTE entry for the given VA */
#define MiGetPteAddress(x) ((PMMPTE)(PTE_BASE + (((ULONG)(x) >> 12) << 2)))
#define MiAddressToPte(x)  MiGetPteAddress(x)

/* Retrives the PDE offset for the given VA */
#define MiGetPdeOffset(x)       (((ULONG)(x)) >> 20)
#define MiGetPteOffset(x)       ((((ULONG)(x)) << 12) >> 24)
#define MiAddressToPteOffset(x) MiGetPteOffset(x)

/* Convert a PTE into a corresponding address */
#define MiPteToAddress(x) ((PVOID)((ULONG)(x) << 10))
#define MiPdeToAddress(x) ((PVOID)((ULONG)(x) << 18))

#define PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(x) \
    ((x) / (4*1024*1024))

#define PAGE_TO_SECTION_PAGE_TABLE_OFFSET(x) \
    ((((x)) % (4*1024*1024)) / (4*1024))

#define MM_CACHE_LINE_SIZE 64
