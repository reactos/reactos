#pragma once


#define _MI_PAGING_LEVELS 4
#define _MI_HAS_NO_EXECUTE 1

/* Memory layout base addresses (This is based on Vista!) */
#define MI_USER_PROBE_ADDRESS           (PVOID)0x000007FFFFFF0000ULL
#define MI_DEFAULT_SYSTEM_RANGE_START   (PVOID)0xFFFF080000000000ULL
#define MI_REAL_SYSTEM_RANGE_START             0xFFFF800000000000ULL
//#define MI_PAGE_TABLE_BASE                   0xFFFFF68000000000ULL // 512 GB page tables
#define HYPER_SPACE                            0xFFFFF70000000000ULL // 512 GB hyper space [MiVaProcessSpace]
#define HYPER_SPACE_END                        0xFFFFF77FFFFFFFFFULL
//#define MI_SHARED_SYSTEM_PAGE                0xFFFFF78000000000ULL
#define MI_SYSTEM_CACHE_WS_START               0xFFFFF78000001000ULL // 512 GB - 4 KB system cache working set
//#define MI_LOADER_MAPPINGS                   0xFFFFF80000000000ULL // 512 GB loader mappings aka KSEG0_BASE (NDK) [MiVaBootLoaded]
#define MM_SYSTEM_SPACE_START                  0xFFFFF88000000000ULL // 128 GB system PTEs [MiVaSystemPtes]
#define MI_DEBUG_MAPPING                (PVOID)0xFFFFF89FFFFFF000ULL // FIXME should be allocated from System PTEs
#define MI_PAGED_POOL_START             (PVOID)0xFFFFF8A000000000ULL // 128 GB paged pool [MiVaPagedPool]
//#define MI_PAGED_POOL_END                    0xFFFFF8BFFFFFFFFFULL
//#define MI_SESSION_SPACE_START               0xFFFFF90000000000ULL // 512 GB session space [MiVaSessionSpace]
//#define MI_SESSION_VIEW_END                    0xFFFFF97FFF000000ULL
#define MI_SESSION_SPACE_END                   0xFFFFF98000000000ULL
#define MI_SYSTEM_CACHE_START                  0xFFFFF98000000000ULL // 1 TB system cache (on Vista+ this is dynamic VA space) [MiVaSystemCache,MiVaSpecialPoolPaged,MiVaSpecialPoolNonPaged]
#define MI_SYSTEM_CACHE_END                    0xFFFFFA7FFFFFFFFFULL
#define MI_PFN_DATABASE                        0xFFFFFA8000000000ULL // up to 5.5 TB PFN database followed by non paged pool [MiVaPfnDatabase/MiVaNonPagedPool]
#define MI_NONPAGED_POOL_END            (PVOID)0xFFFFFFFFFFBFFFFFULL
//#define MM_HAL_VA_START                      0xFFFFFFFFFFC00000ULL // 4 MB HAL mappings, defined in NDK [MiVaHal]
#define MI_HIGHEST_SYSTEM_ADDRESS       (PVOID)0xFFFFFFFFFFFFFFFFULL
#define MmSystemRangeStart              ((PVOID)MI_REAL_SYSTEM_RANGE_START)

/* WOW64 address definitions */
#define MM_HIGHEST_USER_ADDRESS_WOW64   0x7FFEFFFF
#define MM_SYSTEM_RANGE_START_WOW64     0x80000000

/* The size of the virtual memory area that is mapped using a single PDE */
#define PDE_MAPPED_VA (PTE_PER_PAGE * PAGE_SIZE)

/* Misc address definitions */
//#define MI_NON_PAGED_SYSTEM_START_MIN   MM_SYSTEM_SPACE_START // FIXME
//#define MI_SYSTEM_PTE_START             MM_SYSTEM_SPACE_START
//#define MI_SYSTEM_PTE_END               (MI_SYSTEM_PTE_START + MI_NUMBER_SYSTEM_PTES * PAGE_SIZE - 1)
#define MI_SYSTEM_PTE_BASE              (PVOID)MiAddressToPte(KSEG0_BASE)
#define MM_HIGHEST_VAD_ADDRESS          (PVOID)((ULONG_PTR)MM_HIGHEST_USER_ADDRESS - (16 * PAGE_SIZE))
#define MI_MAPPING_RANGE_START          HYPER_SPACE
#define MI_MAPPING_RANGE_END            (MI_MAPPING_RANGE_START + MI_HYPERSPACE_PTES * PAGE_SIZE)
#define MI_DUMMY_PTE                        (MI_MAPPING_RANGE_END + PAGE_SIZE)
#define MI_VAD_BITMAP                       (MI_DUMMY_PTE + PAGE_SIZE)
#define MI_WORKING_SET_LIST                 (MI_VAD_BITMAP + PAGE_SIZE)

/* Memory sizes */
#define MI_MIN_PAGES_FOR_NONPAGED_POOL_TUNING   ((255 * _1MB) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_TUNING          ((19 * _1MB) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_BOOST           ((32 * _1MB) >> PAGE_SHIFT)
#define MI_MIN_PAGES_FOR_SYSPTE_BOOST_BOOST     ((256 * _1MB) >> PAGE_SHIFT)
#define MI_MIN_INIT_PAGED_POOLSIZE              (32 * _1MB)
#define MI_MAX_INIT_NONPAGED_POOL_SIZE          (128ULL * 1024 * 1024 * 1024)
#define MI_MAX_NONPAGED_POOL_SIZE               (128ULL * 1024 * 1024 * 1024)
#define MI_SYSTEM_VIEW_SIZE                     (104 * _1MB)
#define MI_SESSION_VIEW_SIZE                    (104 * _1MB)
#define MI_SESSION_POOL_SIZE                    (64 * _1MB)
#define MI_SESSION_IMAGE_SIZE                   (16 * _1MB)
#define MI_SESSION_WORKING_SET_SIZE             (16 * _1MB)
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
#define MI_NUMBER_SYSTEM_PTES                   22000
#define MI_MAX_FREE_PAGE_LISTS                  4
#define MI_HYPERSPACE_PTES                     (256 - 1)
#define MI_ZERO_PTES                           (32)
#define MI_MAX_ZERO_BITS                        53
#define SESSION_POOL_LOOKASIDES                 21

/* MMPTE related defines */
#define MM_EMPTY_PTE_LIST  ((ULONG64)0xFFFFFFFF)
#define MM_EMPTY_LIST  ((ULONG_PTR)-1)

