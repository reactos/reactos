#pragma once

/* ARM64 Memory Management Definitions for ReactOS Kernel */

/* ARM64 Page Sizes */
#define PAGE_SIZE               0x1000      /* 4KB */
#define PAGE_SHIFT              12
#define PAGES_PER_LARGE_PAGE    512         /* 2MB / 4KB */
#define LARGE_PAGE_SIZE         0x200000    /* 2MB */
#define LARGE_PAGE_SHIFT        21

/* ARM64 Virtual Address Space Layout */
#define KERNEL_BASE             0xFFFF800000000000ULL
#define HYPERSPACE_BASE         0xFFFF900000000000ULL
#define SYSTEM_SPACE_BASE       0xFFFFA00000000000ULL
#define HAL_BASE                0xFFFFC00000000000ULL
#define USER_SPACE_END          0x00007FFFFFFFFFFFULL

/* ARM64 Page Table Levels */
#define ARM64_PT_LEVELS         4
#define ARM64_PT_INDEX_BITS     9
#define ARM64_PT_ENTRIES        512

/* ARM64 Page Table Entry Flags */
#define ARM64_PTE_VALID         0x0000000000000001ULL  /* Entry is valid */
#define ARM64_PTE_TYPE_MASK     0x0000000000000002ULL  
#define ARM64_PTE_TYPE_BLOCK    0x0000000000000000ULL  /* Block/Page descriptor */
#define ARM64_PTE_TYPE_TABLE    0x0000000000000002ULL  /* Table descriptor */

/* Memory Attributes */
#define ARM64_PTE_ATTR_MASK     0x00000000000000FCULL
#define ARM64_PTE_ATTR_NORMAL   0x0000000000000000ULL  /* Normal memory, Inner/Outer WB */
#define ARM64_PTE_ATTR_DEVICE   0x0000000000000004ULL  /* Device memory */
#define ARM64_PTE_ATTR_NC       0x0000000000000008ULL  /* Non-cacheable */

/* Access Permissions */
#define ARM64_PTE_AP_MASK       0x00000000000000C0ULL
#define ARM64_PTE_AP_KERNEL_RW  0x0000000000000000ULL  /* Kernel R/W, User no access */
#define ARM64_PTE_AP_RW         0x0000000000000040ULL  /* Kernel/User R/W */
#define ARM64_PTE_AP_KERNEL_RO  0x0000000000000080ULL  /* Kernel R/O, User no access */
#define ARM64_PTE_AP_RO         0x00000000000000C0ULL  /* Kernel/User R/O */

/* Shareability */
#define ARM64_PTE_SH_MASK       0x0000000000000300ULL
#define ARM64_PTE_SH_NONE       0x0000000000000000ULL  /* Non-shareable */
#define ARM64_PTE_SH_OUTER      0x0000000000000200ULL  /* Outer Shareable */
#define ARM64_PTE_SH_INNER      0x0000000000000300ULL  /* Inner Shareable */

/* Access Flag and Dirty Bit */
#define ARM64_PTE_AF            0x0000000000000400ULL  /* Access Flag */
#define ARM64_PTE_DIRTY         0x0000000000000800ULL  /* Dirty bit (custom) */

/* Execute permissions */
#define ARM64_PTE_PXN           0x0020000000000000ULL  /* Privileged Execute Never */
#define ARM64_PTE_UXN           0x0040000000000000ULL  /* User Execute Never */

/* Software bits (bits 55-58) */
#define ARM64_PTE_SW_MASK       0x0F00000000000000ULL
#define ARM64_PTE_SW_WIRED      0x0100000000000000ULL  /* Wired page */
#define ARM64_PTE_SW_PROTOTYPE  0x0200000000000000ULL  /* Prototype PTE */
#define ARM64_PTE_SW_TRANSITION 0x0400000000000000ULL  /* Transition PTE */

/* Physical address mask (bits 47:12 for 4KB pages) */
#define ARM64_PTE_ADDR_MASK     0x0000FFFFFFFFF000ULL

/* ARM64 Page Table Macros */
#define ARM64_PTE_GET_PFN(pte)  (((pte) & ARM64_PTE_ADDR_MASK) >> PAGE_SHIFT)
#define ARM64_PTE_SET_PFN(pfn)  (((ULONGLONG)(pfn)) << PAGE_SHIFT)

/* Virtual Address Breakdown (48-bit VA) */
#define ARM64_VA_BITS           48
#define ARM64_VA_MASK           0x0000FFFFFFFFFFFFULL

/* Page table index extraction */
#define ARM64_L0_INDEX(va)      (((va) >> 39) & 0x1FF)  /* bits 47:39 */
#define ARM64_L1_INDEX(va)      (((va) >> 30) & 0x1FF)  /* bits 38:30 */
#define ARM64_L2_INDEX(va)      (((va) >> 21) & 0x1FF)  /* bits 29:21 */
#define ARM64_L3_INDEX(va)      (((va) >> 12) & 0x1FF)  /* bits 20:12 */

/* Memory types for MAIR_EL1 */
#define ARM64_MAIR_DEVICE_nGnRnE    0x00    /* Device-nGnRnE */
#define ARM64_MAIR_NORMAL_NC        0x44    /* Normal Non-cacheable */
#define ARM64_MAIR_NORMAL_WB        0xFF    /* Normal Inner/Outer Write-Back */

#define ARM64_MAIR_VALUE  ((ARM64_MAIR_DEVICE_nGnRnE << 0) | \
                           (ARM64_MAIR_NORMAL_NC << 8) | \
                           (ARM64_MAIR_NORMAL_WB << 16))

/* TCR_EL1 Configuration */
#define ARM64_TCR_T0SZ(x)       ((x) & 0x3F)           /* Size offset for TTBR0 */
#define ARM64_TCR_EPD0          (1ULL << 7)            /* Disable TTBR0 walks */
#define ARM64_TCR_IRGN0_NC      (0ULL << 8)            /* TTBR0 Inner Non-cacheable */
#define ARM64_TCR_IRGN0_WB      (1ULL << 8)            /* TTBR0 Inner Write-Back */
#define ARM64_TCR_ORGN0_NC      (0ULL << 10)           /* TTBR0 Outer Non-cacheable */
#define ARM64_TCR_ORGN0_WB      (1ULL << 10)           /* TTBR0 Outer Write-Back */
#define ARM64_TCR_SH0_NONE      (0ULL << 12)           /* TTBR0 Non-shareable */
#define ARM64_TCR_SH0_INNER     (3ULL << 12)           /* TTBR0 Inner Shareable */
#define ARM64_TCR_TG0_4K        (0ULL << 14)           /* TTBR0 4KB granule */
#define ARM64_TCR_T1SZ(x)       (((x) & 0x3F) << 16)   /* Size offset for TTBR1 */
#define ARM64_TCR_A1            (1ULL << 22)            /* ASID in TTBR1 */
#define ARM64_TCR_EPD1          (1ULL << 23)            /* Disable TTBR1 walks */
#define ARM64_TCR_IRGN1_WB      (1ULL << 24)           /* TTBR1 Inner Write-Back */
#define ARM64_TCR_ORGN1_WB      (1ULL << 26)           /* TTBR1 Outer Write-Back */
#define ARM64_TCR_SH1_INNER     (3ULL << 28)           /* TTBR1 Inner Shareable */
#define ARM64_TCR_TG1_4K        (2ULL << 30)           /* TTBR1 4KB granule */
#define ARM64_TCR_IPS_48BIT     (5ULL << 32)           /* 48-bit physical address */

/* Standard TCR configuration for ReactOS */
#define ARM64_TCR_DEFAULT \
    (ARM64_TCR_T0SZ(16) | ARM64_TCR_IRGN0_WB | ARM64_TCR_ORGN0_WB | \
     ARM64_TCR_SH0_INNER | ARM64_TCR_TG0_4K | \
     ARM64_TCR_T1SZ(16) | ARM64_TCR_IRGN1_WB | ARM64_TCR_ORGN1_WB | \
     ARM64_TCR_SH1_INNER | ARM64_TCR_TG1_4K | ARM64_TCR_IPS_48BIT)

/* SCTLR_EL1 Configuration */
#define ARM64_SCTLR_M           (1ULL << 0)             /* MMU enable */
#define ARM64_SCTLR_A           (1ULL << 1)             /* Alignment check */
#define ARM64_SCTLR_C           (1ULL << 2)             /* Data cache enable */
#define ARM64_SCTLR_SA          (1ULL << 3)             /* Stack Alignment */
#define ARM64_SCTLR_I           (1ULL << 12)            /* Instruction cache enable */
#define ARM64_SCTLR_WXN         (1ULL << 19)            /* Write implies XN */
#define ARM64_SCTLR_EE          (1ULL << 25)            /* Exception Endianness */

/* Standard SCTLR configuration for ReactOS */
#define ARM64_SCTLR_DEFAULT \
    (ARM64_SCTLR_M | ARM64_SCTLR_C | ARM64_SCTLR_I | ARM64_SCTLR_SA)

/* ARM64 Memory Management Function Prototypes */

/* Page Table Management */
VOID
NTAPI
MiInitializeProcessAddressSpace(
    IN PEPROCESS Process,
    IN PETHREAD Thread,
    IN PFN_NUMBER DirectoryTableBase,
    IN HANDLE ProcessHandle
);

BOOLEAN
NTAPI
MmCreateProcessAddressSpace(
    IN ULONG MinWs,
    IN PEPROCESS Process,
    IN PFN_NUMBER DirectoryTableBase
);

VOID
NTAPI
MmDeleteProcessAddressSpace(
    IN PEPROCESS Process
);

/* ARM64 Specific MMU Functions */
VOID
NTAPI
MiInitializeMemoryManager(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

BOOLEAN
NTAPI
MmInitializeMemoryManager(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PFN_NUMBER MxFreeDescriptor,
    IN PFN_NUMBER HighestPhysicalPage
);

VOID
NTAPI
MiEnableVirtualMemory(
    VOID
);

VOID
NTAPI
MiSetupIdentityMapping(
    IN PULONG64 PageDirectory,
    IN PHYSICAL_ADDRESS StartAddress,
    IN PHYSICAL_ADDRESS EndAddress
);

VOID
NTAPI
MiInitializeSystemPageTables(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

/* ARM64 Cache Management */
VOID
NTAPI
MiFlushTlb(
    VOID
);

VOID
NTAPI
MiFlushTlbRange(
    IN PVOID StartVa,
    IN SIZE_T Size
);

VOID
NTAPI
MiInvalidateSystemCaches(
    IN PVOID StartVa,
    IN SIZE_T Size
);

/* ARM64 Physical Memory Management */
PFN_NUMBER
NTAPI
MmGetPhysicalAddress(
    IN PVOID BaseAddress
);

PVOID
NTAPI
MmMapIoSpace(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN SIZE_T NumberOfBytes,
    IN MEMORY_CACHING_TYPE CacheType
);

VOID
NTAPI
MmUnmapIoSpace(
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
);

/* ARM64 Page Fault Handling */
NTSTATUS
NTAPI
MmAccessFault(
    IN ULONG FaultCode,
    IN PVOID VirtualAddress,
    IN KPROCESSOR_MODE PreviousMode,
    IN PVOID TrapInformation
);

/* ARM64 Working Set Management */
VOID
NTAPI
MiInitializeWorkingSetManager(
    VOID
);

/* ARM64 Pool Management */
VOID
NTAPI
MiInitializePool(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

/* ARM64 Section Object Support */
NTSTATUS
NTAPI
MmCreateSection(
    OUT PVOID *SectionObject,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL,
    IN PFILE_OBJECT File OPTIONAL
);

/* ARM64 Memory Descriptor Lists */
VOID
NTAPI
MmInitializeMdl(
    IN PMDL MemoryDescriptorList,
    IN PVOID BaseVa,
    IN SIZE_T Length
);

/* ARM64 Hyperspace Management */
VOID
NTAPI
MiInitializeHyperspace(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

PVOID
NTAPI
MiMapPageInHyperSpace(
    IN PEPROCESS Process,
    IN PFN_NUMBER PageFrameIndex,
    IN PKIRQL OldIrql
);

VOID
NTAPI
MiUnmapPageInHyperSpace(
    IN PEPROCESS Process,
    IN PVOID Address,
    IN KIRQL OldIrql
);

/* ARM64 Constants */
#define BYTES_TO_PAGES(Size)    (((Size) >> PAGE_SHIFT) + (((Size) & (PAGE_SIZE - 1)) != 0))
#define PAGES_TO_BYTES(Pages)   ((Pages) << PAGE_SHIFT)
#define ROUND_TO_PAGES(Size)    (((Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

/* ARM64 Memory Layout Constants */
#define MM_SYSTEM_RANGE_START           (PVOID)KERNEL_BASE
#define MM_HAL_VA_START                 (PVOID)HAL_BASE
#define MM_HYPERSPACE_START             (PVOID)HYPERSPACE_BASE

#define MM_USER_PROBE_ADDRESS           ((ULONG_PTR)USER_SPACE_END)
#define MM_HIGHEST_USER_ADDRESS         (PVOID)(USER_SPACE_END)
#define MM_SYSTEM_SPACE_START           (PVOID)SYSTEM_SPACE_BASE

/* ARM64 Paging Macros */
#define MiGetPteAddress(va) \
    ((PMMPTE)(PTE_BASE + (((ULONG_PTR)(va) >> 12) << 3)))

#define MiGetPdeAddress(va) \
    ((PMMPTE)(PDE_BASE + (((ULONG_PTR)(va) >> 21) << 3)))

#define MiGetPpeAddress(va) \
    ((PMMPTE)(PPE_BASE + (((ULONG_PTR)(va) >> 30) << 3)))

#define MiGetPxeAddress(va) \
    ((PMMPTE)(PXE_BASE + (((ULONG_PTR)(va) >> 39) << 3)))

/* ARM64 PTE Base Addresses */
#define PTE_BASE    0xFFFF000000000000ULL
#define PDE_BASE    0xFFFF000080000000ULL
#define PPE_BASE    0xFFFF000080400000ULL
#define PXE_BASE    0xFFFF000080404000ULL

#endif /* _NTOSKRNL_INCLUDE_INTERNAL_ARM64_MM_H */