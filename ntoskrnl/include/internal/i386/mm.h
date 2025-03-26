/*
 * kernel internal memory management definitions for x86
 */
#pragma once

#ifdef _X86PAE_
#define _MI_PAGING_LEVELS 3
#define _MI_HAS_NO_EXECUTE 1
#else
#define _MI_PAGING_LEVELS 2
#define _MI_HAS_NO_EXECUTE 0
#endif

/* Memory layout base addresses */
#define MI_USER_PROBE_ADDRESS                   (PVOID)0x7FFF0000
#define MI_DEFAULT_SYSTEM_RANGE_START           (PVOID)0x80000000
#ifndef _X86PAE_
#define HYPER_SPACE                                    0xC0400000
#define HYPER_SPACE_END                                0xC07FFFFF
#else
#define HYPER_SPACE                                    0xC0800000
#define HYPER_SPACE_END                                0xC0BFFFFF
#endif
#define MI_SYSTEM_CACHE_WS_START                (PVOID)0xC0C00000
#define MI_SYSTEM_CACHE_START                   (PVOID)0xC1000000
#define MI_PAGED_POOL_START                     (PVOID)0xE1000000
#define MI_NONPAGED_POOL_END                    (PVOID)0xFFBE0000
#define MI_DEBUG_MAPPING                        (PVOID)0xFFBFF000
#define MI_HIGHEST_SYSTEM_ADDRESS               (PVOID)0xFFFFFFFF

/* Misc address definitions */
#define MM_HIGHEST_VAD_ADDRESS \
    (PVOID)((ULONG_PTR)MM_HIGHEST_USER_ADDRESS - (16 * PAGE_SIZE))
#define MI_MAPPING_RANGE_START              (ULONG)HYPER_SPACE
#define MI_MAPPING_RANGE_END                (MI_MAPPING_RANGE_START + \
                                                  MI_HYPERSPACE_PTES * PAGE_SIZE)
#define MI_DUMMY_PTE                        (PMMPTE)((ULONG_PTR)MI_MAPPING_RANGE_END + \
                                                  PAGE_SIZE)
#define MI_VAD_BITMAP                       (PMMPTE)((ULONG_PTR)MI_DUMMY_PTE + \
                                                  PAGE_SIZE)
#define MI_WORKING_SET_LIST                 (PMMPTE)((ULONG_PTR)MI_VAD_BITMAP + \
                                                  PAGE_SIZE)

/* Memory sizes */
#define MI_MIN_PAGES_FOR_NONPAGED_POOL_TUNING   ((255 * _1MB) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_TUNING          ((19 * _1MB) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_BOOST           ((32 * _1MB) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_BOOST_BOOST     ((256 * _1MB) >> PAGE_SHIFT)
#define MI_MIN_INIT_PAGED_POOLSIZE              (32 * _1MB)
#define MI_MAX_INIT_NONPAGED_POOL_SIZE          (128 * _1MB)
#define MI_MAX_NONPAGED_POOL_SIZE               (128 * _1MB)
#define MI_SYSTEM_VIEW_SIZE                     (32 * _1MB)
#define MI_SESSION_VIEW_SIZE                    (48 * _1MB)
#define MI_SESSION_POOL_SIZE                    (16 * _1MB)
#define MI_SESSION_IMAGE_SIZE                   (8 * _1MB)
#define MI_SESSION_WORKING_SET_SIZE             (4 * _1MB)
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
#define MI_MAX_FREE_PAGE_LISTS                  4
#define MI_HYPERSPACE_PTES                     (256 - 1)
#define MI_ZERO_PTES                           (32)
#define MI_MAX_ZERO_BITS                        21
#define SESSION_POOL_LOOKASIDES                 26

/* MMPTE related defines */
#define MM_EMPTY_PTE_LIST  ((ULONG)0xFFFFF)
#define MM_EMPTY_LIST  ((ULONG_PTR)-1)


/* Easy accessing PFN in PTE */
#define PFN_FROM_PTE(v) ((v)->u.Hard.PageFrameNumber)

/* Macros for portable PTE modification */
#define MI_MAKE_DIRTY_PAGE(x)      ((x)->u.Hard.Dirty = 1)
#define MI_MAKE_CLEAN_PAGE(x)      ((x)->u.Hard.Dirty = 0)
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
#ifdef _X86PAE_
#define MI_IS_PAGE_EXECUTABLE(x)   ((x)->u.Hard.NoExecute == 0)
#else
#define MI_IS_PAGE_EXECUTABLE(x)   TRUE
#endif
#define MI_IS_PAGE_DIRTY(x)        ((x)->u.Hard.Dirty == 1)
#define MI_MAKE_OWNER_PAGE(x)      ((x)->u.Hard.Owner = 1)
#if !defined(CONFIG_SMP)
#define MI_MAKE_WRITE_PAGE(x)      ((x)->u.Hard.Write = 1)
#else
#define MI_MAKE_WRITE_PAGE(x)      ((x)->u.Hard.Writable = 1)
#endif


/* Macros to identify the page fault reason from the error code */
#define MI_IS_NOT_PRESENT_FAULT(FaultCode)  !BooleanFlagOn(FaultCode, 0x00000001)
#define MI_IS_WRITE_ACCESS(FaultCode)        BooleanFlagOn(FaultCode, 0x00000002)
// 0x00000004: user-mode access.
// 0x00000008: reserved bit violation.
#define MI_IS_INSTRUCTION_FETCH(FaultCode)   BooleanFlagOn(FaultCode, 0x00000010)
// 0x00000020: protection-key violation.
// 0x00000040: shadow-stack access.
// Bits 7-14: reserved.
// 0x00008000: violation of SGX-specific access-control requirements.
// Bits 16-31: reserved.

/* On x86, these two are the same */
#define MI_WRITE_VALID_PPE MI_WRITE_VALID_PTE

/*  Translating virtual addresses to physical addresses
        (See: "Intel® 64 and IA-32 Architectures Software Developer’s Manual
              Volume 3A: System Programming Guide, Part 1, CHAPTER 4 PAGING")
    Page directory (PD) and Page table (PT) definitions
    Page directory entry (PDE) and Page table entry (PTE) definitions
*/

/* Maximum number of page directories pages */
#ifndef _X86PAE_
#define PD_COUNT 1        /* Only one page directory page */
#else
#define PD_COUNT (1 << 2) /* The two most significant bits in the VA */
#endif

/* PAE not yet implemented. */
C_ASSERT(PD_COUNT == 1);

/* The number of PTEs on one page of the PT */
#define PTE_PER_PAGE (PAGE_SIZE / sizeof(MMPTE))

/* The number of PDEs on one page of the PD */
#define PDE_PER_PAGE (PAGE_SIZE / sizeof(MMPDE))

/* Maximum number of PDEs */
#define PDE_PER_SYSTEM (PD_COUNT * PDE_PER_PAGE)

/* TODO: It seems this constant is not needed for x86 */
#define PPE_PER_PAGE 1

/* Maximum number of pages for 4 GB of virtual space */
#define MI_MAX_PAGES ((1ull << 32) / PAGE_SIZE)

/* Base addresses for page tables */
#define PTE_BASE (ULONG_PTR)0xC0000000
#define PTE_TOP  (ULONG_PTR)(PTE_BASE + (MI_MAX_PAGES * sizeof(MMPTE)) - 1)
#define PTE_MASK (PTE_TOP - PTE_BASE)

#define MI_SYSTEM_PTE_BASE (PVOID)MiAddressToPte(NULL)

/* Base addreses for page directories */
#define PDE_BASE (ULONG_PTR)MiPteToPde(PTE_BASE)
#define PDE_TOP  (ULONG_PTR)(PDE_BASE + (PDE_PER_SYSTEM * sizeof(MMPDE)) - 1)
#define PDE_MASK (PDE_TOP - PDE_BASE)

/* The size of the virtual memory area that is mapped using a single PDE */
#define PDE_MAPPED_VA (PTE_PER_PAGE * PAGE_SIZE)

/* Maps the virtual address to the corresponding PTE */
#define MiAddressToPte(Va) \
    ((PMMPTE)(PTE_BASE + ((((ULONG_PTR)(Va)) / PAGE_SIZE) * sizeof(MMPTE))))

/* Maps the virtual address to the corresponding PDE */
#define MiAddressToPde(Va) \
    ((PMMPDE)(PDE_BASE + ((MiAddressToPdeOffset(Va)) * sizeof(MMPDE))))

/* Takes the PTE index (for one PD page) from the virtual address */
#define MiAddressToPteOffset(Va) \
    ((((ULONG_PTR)(Va)) & (PDE_MAPPED_VA - 1)) / PAGE_SIZE)

/* Takes the PDE offset (within all PDs pages) from the virtual address */
#define MiAddressToPdeOffset(Va) (((ULONG_PTR)(Va)) / PDE_MAPPED_VA)

/* TODO: Free this variable (for offset from the pointer to the PDE) */
#define MiGetPdeOffset MiAddressToPdeOffset

/* Convert a PTE/PDE into a corresponding address */
#define MiPteToAddress(_Pte) ((PVOID)((ULONG)(_Pte) << 10))
#define MiPdeToAddress(_Pde) ((PVOID)((ULONG)(_Pde) << 20))

/* Translate between P*Es */
#define MiPdeToPte(_Pde) ((PMMPTE)MiPteToAddress(_Pde))
#define MiPteToPde(_Pte) ((PMMPDE)MiAddressToPte(_Pte))

/* Check P*E boundaries */
#define MiIsPteOnPdeBoundary(PointerPte) \
    ((((ULONG_PTR)PointerPte) & (PAGE_SIZE - 1)) == 0)

//
// Decodes a Prototype PTE into the underlying PTE
//
#define MiProtoPteToPte(x)                  \
    (PMMPTE)((ULONG_PTR)MmPagedPoolStart +  \
             (((x)->u.Proto.ProtoAddressHigh << 9) | (x)->u.Proto.ProtoAddressLow << 2))

//
// Decodes a Prototype PTE into the underlying PTE
//
#define MiSubsectionPteToSubsection(x)                              \
    ((x)->u.Subsect.WhichPool == PagedPool) ?                       \
        (PMMPTE)((ULONG_PTR)MmSubsectionBase +                      \
                 (((x)->u.Subsect.SubsectionAddressHigh << 7) |     \
                   (x)->u.Subsect.SubsectionAddressLow << 3)) :     \
        (PMMPTE)((ULONG_PTR)MmNonPagedPoolEnd -                     \
                (((x)->u.Subsect.SubsectionAddressHigh << 7) |      \
                  (x)->u.Subsect.SubsectionAddressLow << 3))
