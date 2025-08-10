/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 MMU and memory management
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

#include <freeldr.h>
#include <arch/arm64/arm64.h>
#include <debug.h>

/* ARM64 page table attributes */
#define ARM64_PTE_VALID         (1ULL << 0)
#define ARM64_PTE_TABLE         (1ULL << 1)  /* For non-leaf entries */
#define ARM64_PTE_PAGE          (1ULL << 1)  /* For leaf entries at level 3 */
#define ARM64_PTE_BLOCK         (0ULL << 1)  /* For leaf entries at level 1,2 */

/* Access permissions */
#define ARM64_PTE_AP_RW_EL1     (0ULL << 6)  /* Read-write, EL1 only */
#define ARM64_PTE_AP_RW_ALL     (1ULL << 6)  /* Read-write, all ELs */
#define ARM64_PTE_AP_RO_EL1     (2ULL << 6)  /* Read-only, EL1 only */
#define ARM64_PTE_AP_RO_ALL     (3ULL << 6)  /* Read-only, all ELs */

/* Execute permissions */
#define ARM64_PTE_UXN           (1ULL << 54) /* User execute never */
#define ARM64_PTE_PXN           (1ULL << 53) /* Privileged execute never */

/* Shareability */
#define ARM64_PTE_SH_NON        (0ULL << 8)  /* Non-shareable */
#define ARM64_PTE_SH_OUTER      (2ULL << 8)  /* Outer shareable */
#define ARM64_PTE_SH_INNER      (3ULL << 8)  /* Inner shareable */

/* Memory attributes index */
#define ARM64_PTE_ATTR_INDEX(x) ((ULONGLONG)(x) << 2)

/* Memory attribute indices in MAIR_EL1 */
#define MT_DEVICE_nGnRnE        0  /* Device non-Gathering, non-Reordering, no Early write acknowledgement */
#define MT_DEVICE_nGnRE         1  /* Device non-Gathering, non-Reordering, Early write acknowledgement */
#define MT_DEVICE_GRE           2  /* Device Gathering, Reordering, Early write acknowledgement */
#define MT_NORMAL_NC            3  /* Normal memory, Non-cacheable */
#define MT_NORMAL               4  /* Normal memory, Cacheable */
#define MT_NORMAL_WT            5  /* Normal memory, Write-through */

/* MAIR_EL1 values for different memory types */
#define MAIR_DEVICE_nGnRnE      0x00  /* Device, strongly ordered */
#define MAIR_DEVICE_nGnRE       0x04  /* Device */
#define MAIR_DEVICE_GRE         0x0C  /* Device */
#define MAIR_NORMAL_NC          0x44  /* Normal, non-cacheable */
#define MAIR_NORMAL             0xFF  /* Normal, cacheable, RW-allocate */
#define MAIR_NORMAL_WT          0xBB  /* Normal, write-through */

/* Page sizes and shifts */
#define ARM64_PAGE_SHIFT        12
#define ARM64_PAGE_SIZE         (1UL << ARM64_PAGE_SHIFT)
#define ARM64_PAGE_MASK         (ARM64_PAGE_SIZE - 1)

/* Translation granule - 4KB pages */
#define ARM64_GRANULE_SHIFT     12
#define ARM64_GRANULE_SIZE      (1UL << ARM64_GRANULE_SHIFT)

/* Address space configuration */
#define ARM64_VA_BITS           48    /* 48-bit virtual addresses */
#define ARM64_PA_BITS           48    /* 48-bit physical addresses */

/* TCR_EL1 configuration */
#define TCR_T0SZ(x)             ((64 - (x)) & 0x3F)
#define TCR_T1SZ(x)             (((64 - (x)) & 0x3F) << 16)
#define TCR_TG0_4K              (0ULL << 14)
#define TCR_TG0_16K             (2ULL << 14)
#define TCR_TG0_64K             (1ULL << 14)
#define TCR_TG1_4K              (2ULL << 30)
#define TCR_TG1_16K             (1ULL << 30)
#define TCR_TG1_64K             (3ULL << 30)
#define TCR_SH0_NON             (0ULL << 12)
#define TCR_SH0_OUTER           (2ULL << 12)
#define TCR_SH0_INNER           (3ULL << 12)
#define TCR_SH1_NON             (0ULL << 28)
#define TCR_SH1_OUTER           (2ULL << 28)
#define TCR_SH1_INNER           (3ULL << 28)
#define TCR_ORGN0_NC            (0ULL << 10)
#define TCR_ORGN0_WBWA          (1ULL << 10)
#define TCR_ORGN0_WT            (2ULL << 10)
#define TCR_ORGN0_WB            (3ULL << 10)
#define TCR_ORGN1_NC            (0ULL << 26)
#define TCR_ORGN1_WBWA          (1ULL << 26)
#define TCR_ORGN1_WT            (2ULL << 26)
#define TCR_ORGN1_WB            (3ULL << 26)
#define TCR_IRGN0_NC            (0ULL << 8)
#define TCR_IRGN0_WBWA          (1ULL << 8)
#define TCR_IRGN0_WT            (2ULL << 8)
#define TCR_IRGN0_WB            (3ULL << 8)
#define TCR_IRGN1_NC            (0ULL << 24)
#define TCR_IRGN1_WBWA          (1ULL << 24)
#define TCR_IRGN1_WT            (2ULL << 24)
#define TCR_IRGN1_WB            (3ULL << 24)
#define TCR_IPS_32BITS          (0ULL << 32)
#define TCR_IPS_36BITS          (1ULL << 32)
#define TCR_IPS_40BITS          (2ULL << 32)
#define TCR_IPS_42BITS          (3ULL << 32)
#define TCR_IPS_44BITS          (4ULL << 32)
#define TCR_IPS_48BITS          (5ULL << 32)
#define TCR_IPS_52BITS          (6ULL << 32)

/* SCTLR_EL1 bits */
#define SCTLR_EL1_M             (1ULL << 0)   /* MMU enable */
#define SCTLR_EL1_A             (1ULL << 1)   /* Alignment check enable */
#define SCTLR_EL1_C             (1ULL << 2)   /* Data cache enable */
#define SCTLR_EL1_SA            (1ULL << 3)   /* Stack alignment check enable */
#define SCTLR_EL1_SA0           (1ULL << 4)   /* Stack alignment check enable for EL0 */
#define SCTLR_EL1_CP15BEN       (1ULL << 5)   /* CP15 barrier enable */
#define SCTLR_EL1_ITD           (1ULL << 7)   /* IT disable */
#define SCTLR_EL1_SED           (1ULL << 8)   /* SETEND disable */
#define SCTLR_EL1_UMA           (1ULL << 9)   /* User mask access */
#define SCTLR_EL1_I             (1ULL << 12)  /* Instruction cache enable */
#define SCTLR_EL1_DZE           (1ULL << 14)  /* Data zero enable */
#define SCTLR_EL1_UCT           (1ULL << 15)  /* User cache type */
#define SCTLR_EL1_nTWI          (1ULL << 16)  /* Not trap WFI */
#define SCTLR_EL1_nTWE          (1ULL << 18)  /* Not trap WFE */
#define SCTLR_EL1_WXN           (1ULL << 19)  /* Write execute never */
#define SCTLR_EL1_E0E           (1ULL << 24)  /* Exception endianness for EL0 */
#define SCTLR_EL1_EE            (1ULL << 25)  /* Exception endianness */
#define SCTLR_EL1_UCI           (1ULL << 26)  /* User cache instructions */
#define SCTLR_EL1_EnDB          (1ULL << 13)  /* Enable pointer authentication */
#define SCTLR_EL1_EnDA          (1ULL << 27)  /* Enable pointer authentication */
#define SCTLR_EL1_EnIB          (1ULL << 30)  /* Enable pointer authentication */
#define SCTLR_EL1_EnIA          (1ULL << 31)  /* Enable pointer authentication */

/* Simple identity mapping for boot loader */
static UINT64 arm64_page_tables[4][512] __attribute__((aligned(4096)));
static BOOLEAN mmu_enabled = FALSE;

/* Initialize ARM64 MMU with identity mapping */
VOID Arm64InitializeMMU(VOID)
{
    ULONGLONG mair, tcr, sctlr;
    ULONG i;
    
    TRACE("ARM64: Initializing MMU\n");
    
    /* Set up MAIR_EL1 (Memory Attribute Indirection Register) */
    mair = ((ULONGLONG)MAIR_DEVICE_nGnRnE << (8 * MT_DEVICE_nGnRnE)) |
           ((ULONGLONG)MAIR_DEVICE_nGnRE << (8 * MT_DEVICE_nGnRE)) |
           ((ULONGLONG)MAIR_DEVICE_GRE << (8 * MT_DEVICE_GRE)) |
           ((ULONGLONG)MAIR_NORMAL_NC << (8 * MT_NORMAL_NC)) |
           ((ULONGLONG)MAIR_NORMAL << (8 * MT_NORMAL)) |
           ((ULONGLONG)MAIR_NORMAL_WT << (8 * MT_NORMAL_WT));\n    
    ARM64_WRITE_SYSREG(mair_el1, mair);
    
    /* Clear page tables */
    RtlZeroMemory(arm64_page_tables, sizeof(arm64_page_tables));
    
    /* Set up L0 page table (512GB entries) */\n    /* Map first 512GB with a single L0 entry pointing to L1 table */
    arm64_page_tables[0][0] = (UINT64)&arm64_page_tables[1][0] | 
                              ARM64_PTE_VALID | ARM64_PTE_TABLE;
    
    /* Set up L1 page table (1GB entries) */
    /* Map first 512GB with 1GB blocks */
    for (i = 0; i < 512; i++)
    {
        arm64_page_tables[1][i] = ((UINT64)i << 30) | 
                                  ARM64_PTE_VALID | ARM64_PTE_BLOCK |
                                  ARM64_PTE_AP_RW_EL1 |
                                  ARM64_PTE_SH_INNER |
                                  ARM64_PTE_ATTR_INDEX(MT_NORMAL);
    }
    
    /* Map device memory regions (if needed) */\n    /* This would map specific device regions with device attributes */\n    /* For now, using normal memory for everything */
    
    /* Set up TCR_EL1 (Translation Control Register) */
    tcr = TCR_T0SZ(ARM64_VA_BITS) |
          TCR_T1SZ(ARM64_VA_BITS) |
          TCR_TG0_4K |              /* 4KB granule for TTBR0_EL1 */
          TCR_TG1_4K |              /* 4KB granule for TTBR1_EL1 */
          TCR_SH0_INNER |           /* Inner shareable for TTBR0_EL1 */
          TCR_SH1_INNER |           /* Inner shareable for TTBR1_EL1 */
          TCR_ORGN0_WBWA |          /* Normal memory, write-back write-allocate */
          TCR_ORGN1_WBWA |
          TCR_IRGN0_WBWA |          /* Normal memory, write-back write-allocate */
          TCR_IRGN1_WBWA |
          TCR_IPS_48BITS;           /* 48-bit physical address size */
    
    ARM64_WRITE_SYSREG(tcr_el1, tcr);
    
    /* Set up TTBR0_EL1 (Translation Table Base Register) */
    ARM64_WRITE_SYSREG(ttbr0_el1, (UINT64)&arm64_page_tables[0][0]);
    
    /* Ensure all system register writes are complete */
    ARM64_ISB();
    
    /* Enable MMU and caches */
    sctlr = ARM64_READ_SYSREG(sctlr_el1);
    sctlr |= SCTLR_EL1_M |   /* Enable MMU */
             SCTLR_EL1_C |   /* Enable data cache */
             SCTLR_EL1_I |   /* Enable instruction cache */
             SCTLR_EL1_SA |  /* Enable stack alignment check */
             SCTLR_EL1_A;    /* Enable alignment check */
    
    ARM64_WRITE_SYSREG(sctlr_el1, sctlr);
    ARM64_ISB();
    
    mmu_enabled = TRUE;
    
    TRACE("ARM64: MMU enabled with identity mapping\n");
}

/* Map a virtual address range to physical address */
BOOLEAN Arm64MapVirtualMemory(ULONGLONG VirtualAddress,
                             ULONGLONG PhysicalAddress, 
                             ULONGLONG Size,
                             ULONG Attributes)
{
    /* This would implement page table manipulation for mapping */
    /* For the boot loader, we primarily use identity mapping */
    
    TRACE("ARM64: Map VA=0x%016llx -> PA=0x%016llx, Size=0x%016llx, Attr=0x%lx\n",
          VirtualAddress, PhysicalAddress, Size, Attributes);
    
    /* For now, assume identity mapping is sufficient */
    return TRUE;
}

/* Unmap a virtual address range */
BOOLEAN Arm64UnmapVirtualMemory(ULONGLONG VirtualAddress, ULONGLONG Size)
{
    TRACE("ARM64: Unmap VA=0x%016llx, Size=0x%016llx\n", VirtualAddress, Size);
    
    /* Implementation would clear page table entries */
    return TRUE;
}

/* Get physical address from virtual address */
ULONGLONG Arm64GetPhysicalAddress(ULONGLONG VirtualAddress)
{
    /* For identity mapping, virtual == physical */
    if (!mmu_enabled)
        return VirtualAddress;
    
    /* In a full implementation, this would walk page tables */
    /* or use the hardware address translation registers */
    
    /* Simple identity mapping assumption */
    return VirtualAddress;
}

/* Check if MMU is enabled */
BOOLEAN Arm64IsMMUEnabled(VOID)
{
    return mmu_enabled;
}

/* Disable MMU (for kernel handoff) */
VOID Arm64DisableMMU(VOID)
{
    ULONGLONG sctlr;
    
    if (!mmu_enabled)
        return;
    
    TRACE("ARM64: Disabling MMU\n");
    
    /* Clean and invalidate all caches */
    ARM64_DC_CIVAC(0);  /* This should be a full cache clean */
    ARM64_IC_IALLU();
    ARM64_DSB_SY();
    ARM64_ISB();
    
    /* Disable MMU and caches */
    sctlr = ARM64_READ_SYSREG(sctlr_el1);
    sctlr &= ~(SCTLR_EL1_M |  /* Disable MMU */
               SCTLR_EL1_C |  /* Disable data cache */
               SCTLR_EL1_I);  /* Disable instruction cache */
    
    ARM64_WRITE_SYSREG(sctlr_el1, sctlr);
    ARM64_ISB();
    
    mmu_enabled = FALSE;
    
    TRACE("ARM64: MMU disabled\n");
}

/* Get memory attributes for an address */
ULONG Arm64GetMemoryAttributes(ULONGLONG Address)
{
    /* Return attributes based on address range */
    /* This is a simplified implementation */
    
    if (Address < 0x40000000)  /* First 1GB - normal memory */
        return MT_NORMAL;
    else if (Address < 0x80000000)  /* Device memory range */
        return MT_DEVICE_nGnRnE;
    else
        return MT_NORMAL;
}

/* Flush TLB for specific address range */
VOID Arm64FlushTlbRange(ULONGLONG VirtualAddress, ULONGLONG Size)
{
    ULONGLONG end = VirtualAddress + Size;
    ULONGLONG addr;
    
    /* Flush TLB entries for each page in the range */
    for (addr = VirtualAddress & ~ARM64_PAGE_MASK; 
         addr < end; 
         addr += ARM64_PAGE_SIZE)
    {
        __asm__ volatile ("tlbi vaae1, %0" :: "r" (addr >> 12));
    }
    
    ARM64_DSB_SY();
    ARM64_ISB();
}