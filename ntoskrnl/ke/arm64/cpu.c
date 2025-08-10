/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 CPU Support Functions
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* ARM64 CPU feature flags */
ULONG64 KiArm64CpuFeatures = 0;

/* ARM64 processor identification */
static ARM64_CPU_INFO KiArm64CpuInfo;

/* FUNCTIONS *****************************************************************/

/**
 * @brief Get ARM64 CPU identification information
 */
VOID
NTAPI
KiGetArm64CpuInfo(
    OUT PARM64_CPU_INFO CpuInfo
)
{
    ULONGLONG midr, revidr;
    
    /* Read Main ID Register */
    midr = __readmidr();
    revidr = ARM64_READ_SYSREG(revidr_el1);
    
    /* Extract CPU information */
    CpuInfo->Implementer = (UCHAR)((midr >> 24) & 0xFF);
    CpuInfo->Variant = (UCHAR)((midr >> 20) & 0xF);
    CpuInfo->Architecture = (UCHAR)((midr >> 16) & 0xF);
    CpuInfo->PartNumber = (USHORT)((midr >> 4) & 0xFFF);
    CpuInfo->Revision = (UCHAR)(midr & 0xF);
    
    CpuInfo->RevisionId = (ULONG)revidr;
    
    DPRINT("ARM64: CPU Implementer=0x%02X, Part=0x%03X, Variant=%u, Revision=%u\n",
           CpuInfo->Implementer, CpuInfo->PartNumber, CpuInfo->Variant, CpuInfo->Revision);
}

/**
 * @brief Detect ARM64 CPU features and extensions
 */
VOID
NTAPI
KiDetectArm64Features(VOID)
{
    ULONGLONG idar0, isar1, pfr0, pfr1, mmfr0, mmfr1;
    
    /* Read ID registers */
    idar0 = ARM64_READ_SYSREG(id_aa64isar0_el1);
    isar1 = ARM64_READ_SYSREG(id_aa64isar1_el1);
    pfr0 = ARM64_READ_SYSREG(id_aa64pfr0_el1);
    pfr1 = ARM64_READ_SYSREG(id_aa64pfr1_el1);
    mmfr0 = ARM64_READ_SYSREG(id_aa64mmfr0_el1);
    mmfr1 = ARM64_READ_SYSREG(id_aa64mmfr1_el1);
    
    KiArm64CpuFeatures = 0;
    
    /* Check Instruction Set features */
    
    /* AES encryption */
    if ((idar0 & 0xF0) != 0)
    {
        KiArm64CpuFeatures |= ARM64_FEATURE_AES;
        DPRINT("ARM64: AES encryption support\n");
    }
    
    /* SHA hash algorithms */
    if (((idar0 >> 8) & 0xF) != 0)
    {
        KiArm64CpuFeatures |= ARM64_FEATURE_SHA;
        DPRINT("ARM64: SHA hash support\n");
    }
    
    /* CRC32 instructions */
    if (((idar0 >> 16) & 0xF) != 0)
    {
        KiArm64CpuFeatures |= ARM64_FEATURE_CRC32;
        DPRINT("ARM64: CRC32 instructions\n");
    }
    
    /* Atomic instructions (LSE) */
    if (((idar0 >> 20) & 0xF) != 0)
    {
        KiArm64CpuFeatures |= ARM64_FEATURE_ATOMIC;
        DPRINT("ARM64: Large System Extensions (atomics)\n");
    }
    
    /* Pointer Authentication */
    if (((isar1 >> 4) & 0xF) != 0 || ((isar1 >> 8) & 0xF) != 0)
    {
        KiArm64CpuFeatures |= ARM64_FEATURE_PAC;
        DPRINT("ARM64: Pointer Authentication\n");
    }
    
    /* Check Processor features */
    
    /* Floating Point */
    if ((pfr0 & 0xF) != 0xF)
    {
        KiArm64CpuFeatures |= ARM64_FEATURE_FP;
        DPRINT("ARM64: Floating Point support\n");
    }
    
    /* Advanced SIMD */
    if (((pfr0 >> 4) & 0xF) != 0xF)
    {
        KiArm64CpuFeatures |= ARM64_FEATURE_ASIMD;
        DPRINT("ARM64: Advanced SIMD support\n");
    }
    
    /* Scalable Vector Extension */
    if (((pfr0 >> 32) & 0xF) != 0)
    {
        KiArm64CpuFeatures |= ARM64_FEATURE_SVE;
        DPRINT("ARM64: Scalable Vector Extension\n");
    }
    
    /* RAS Extension */
    if (((pfr0 >> 28) & 0xF) != 0)
    {
        KiArm64CpuFeatures |= ARM64_FEATURE_RAS;
        DPRINT("ARM64: RAS Extension\n");
    }
    
    /* Check Memory Model features */
    
    /* Mixed-endian support */
    if ((mmfr0 & 0xF) != 0)
    {
        KiArm64CpuFeatures |= ARM64_FEATURE_MIXED_ENDIAN;
        DPRINT("ARM64: Mixed-endian support\n");
    }
    
    /* 16KB page granule support */
    if (((mmfr0 >> 20) & 0xF) == 1)
    {
        KiArm64CpuFeatures |= ARM64_FEATURE_16KB_GRAN;
        DPRINT("ARM64: 16KB page granule support\n");
    }
    
    /* 64KB page granule support */
    if (((mmfr0 >> 24) & 0xF) == 0)
    {
        KiArm64CpuFeatures |= ARM64_FEATURE_64KB_GRAN;
        DPRINT("ARM64: 64KB page granule support\n");
    }
    
    DPRINT("ARM64: CPU features = 0x%llX\n", KiArm64CpuFeatures);
}

/**
 * @brief Initialize ARM64 CPU-specific features
 */
VOID
NTAPI
KiInitializeCpu(
    VOID
)
{
    ULONGLONG sctlr, cpacr;
    
    DPRINT("ARM64: Initializing CPU\n");
    
    /* Get CPU identification */
    KiGetArm64CpuInfo(&KiArm64CpuInfo);
    
    /* Detect CPU features */
    KiDetectArm64Features();
    
    /* Configure System Control Register */
    sctlr = __readsctlr_el1();
    
    /* Ensure required bits are set */
    sctlr |= ARM64_SCTLR_M;     /* MMU enable */
    sctlr |= ARM64_SCTLR_C;     /* Data cache enable */
    sctlr |= ARM64_SCTLR_I;     /* Instruction cache enable */
    sctlr |= ARM64_SCTLR_SA;    /* Stack Alignment check */
    
    __writesctlr_el1(sctlr);
    
    /* Configure Architectural Feature Access Control Register */
    cpacr = ARM64_READ_SYSREG(cpacr_el1);
    
    /* Enable FP/SIMD access for EL1 and EL0 */
    cpacr |= (3ULL << 20);  /* FPEN = 11 (no trapping) */
    
    /* Enable SVE if supported */
    if (KiArm64CpuFeatures & ARM64_FEATURE_SVE)
    {
        cpacr |= (3ULL << 16);  /* ZEN = 11 (no trapping) */
    }
    
    ARM64_WRITE_SYSREG(cpacr_el1, cpacr);
    ARM64_ISB();
    
    DPRINT("ARM64: CPU initialization completed\n");
}

/**
 * @brief Get ARM64 CPU cache information
 */
VOID
NTAPI
KiGetArm64CacheInfo(
    OUT PARM64_CACHE_INFO CacheInfo
)
{
    ULONGLONG ctr, clidr, ccsidr;
    ULONG level;
    
    /* Clear the structure */
    RtlZeroMemory(CacheInfo, sizeof(ARM64_CACHE_INFO));
    
    /* Read Cache Type Register */
    ctr = ARM64_READ_SYSREG(ctr_el0);
    
    /* Extract cache line sizes */
    CacheInfo->DMinLine = 4 << ((ctr >> 16) & 0xF);  /* DCache min line size */
    CacheInfo->IMinLine = 4 << (ctr & 0xF);          /* ICache min line size */
    
    /* Read Cache Level ID Register */
    clidr = ARM64_READ_SYSREG(clidr_el1);
    CacheInfo->LoC = (UCHAR)((clidr >> 24) & 0x7);   /* Level of Coherency */
    CacheInfo->LoUIS = (UCHAR)((clidr >> 21) & 0x7); /* Level of Unification Inner Shareable */
    CacheInfo->LoUU = (UCHAR)((clidr >> 27) & 0x7);  /* Level of Unification Uniprocessor */
    
    /* Iterate through cache levels */
    for (level = 0; level < 8 && level < CacheInfo->LoC; level++)
    {
        ULONG ctype = (ULONG)((clidr >> (level * 3)) & 0x7);
        
        if (ctype >= 2)  /* Data or unified cache present */
        {
            /* Select cache level */
            ARM64_WRITE_SYSREG(csselr_el1, level << 1);  /* Data/Unified cache */
            ARM64_ISB();
            
            ccsidr = ARM64_READ_SYSREG(ccsidr_el1);
            
            /* Extract cache parameters */
            ULONG line_size = 4 << ((ccsidr & 0x7) + 2);
            ULONG ways = (ULONG)(((ccsidr >> 3) & 0x3FF) + 1);
            ULONG sets = (ULONG)(((ccsidr >> 13) & 0x7FFF) + 1);
            
            CacheInfo->Levels[level].Size = ways * sets * line_size;
            CacheInfo->Levels[level].LineSize = line_size;
            CacheInfo->Levels[level].Ways = ways;
            CacheInfo->Levels[level].Sets = sets;
            CacheInfo->Levels[level].Type = (ctype == 2) ? ARM64_CACHE_DATA : 
                                           (ctype == 3) ? ARM64_CACHE_UNIFIED : ARM64_CACHE_INSTRUCTION;
            
            DPRINT("ARM64: L%u %s Cache: %u KB, %u ways, %u sets, %u byte lines\n",
                   level + 1,
                   (ctype == 2) ? "Data" : (ctype == 3) ? "Unified" : "Instruction",
                   CacheInfo->Levels[level].Size / 1024,
                   ways, sets, line_size);
        }
    }
    
    /* Reset cache selection */
    ARM64_WRITE_SYSREG(csselr_el1, 0);
    ARM64_ISB();
}

/**
 * @brief Perform ARM64 cache maintenance operations
 */
VOID
NTAPI
KiArm64CacheMaintenance(
    IN ARM64_CACHE_OP Operation,
    IN PVOID Address OPTIONAL,
    IN SIZE_T Size OPTIONAL
)
{
    ULONG_PTR addr, end;
    ULONG line_size;
    
    switch (Operation)
    {
        case ARM64_CACHE_CLEAN_ALL:
            /* Clean entire data cache hierarchy */
            __asm_dcache_all(0);
            ARM64_DSB_SY();
            break;
            
        case ARM64_CACHE_INVALIDATE_ALL:
            /* Invalidate entire data cache hierarchy */
            __asm_dcache_all(1);
            ARM64_DSB_SY();
            break;
            
        case ARM64_CACHE_CLEAN_INVALIDATE_ALL:
            /* Clean and invalidate entire data cache hierarchy */
            __asm_dcache_all(0);
            __asm_dcache_all(1);
            ARM64_DSB_SY();
            break;
            
        case ARM64_CACHE_CLEAN_RANGE:
            if (Address && Size)
            {
                line_size = KiArm64CacheInfo.DMinLine;
                addr = (ULONG_PTR)Address & ~(line_size - 1);
                end = ((ULONG_PTR)Address + Size + line_size - 1) & ~(line_size - 1);
                
                while (addr < end)
                {
                    ARM64_DC_CVAC(addr);
                    addr += line_size;
                }
                ARM64_DSB_SY();
            }
            break;
            
        case ARM64_CACHE_INVALIDATE_RANGE:
            if (Address && Size)
            {
                line_size = KiArm64CacheInfo.DMinLine;
                addr = (ULONG_PTR)Address & ~(line_size - 1);
                end = ((ULONG_PTR)Address + Size + line_size - 1) & ~(line_size - 1);
                
                while (addr < end)
                {
                    ARM64_DC_IVAC(addr);
                    addr += line_size;
                }
                ARM64_DSB_SY();
            }
            break;
            
        case ARM64_CACHE_CLEAN_INVALIDATE_RANGE:
            if (Address && Size)
            {
                line_size = KiArm64CacheInfo.DMinLine;
                addr = (ULONG_PTR)Address & ~(line_size - 1);
                end = ((ULONG_PTR)Address + Size + line_size - 1) & ~(line_size - 1);
                
                while (addr < end)
                {
                    ARM64_DC_CIVAC(addr);
                    addr += line_size;
                }
                ARM64_DSB_SY();
            }
            break;
            
        case ARM64_ICACHE_INVALIDATE_ALL:
            /* Invalidate entire instruction cache */
            __ic_iallu();
            ARM64_DSB_SY();
            ARM64_ISB();
            break;
    }
}

/**
 * @brief Get current ARM64 exception level
 */
UCHAR
NTAPI
KiGetCurrentExceptionLevel(VOID)
{
    return (UCHAR)(__readcurrentel() >> 2);
}

/**
 * @brief Check if ARM64 MMU is enabled
 */
BOOLEAN
NTAPI
KiIsArm64MmuEnabled(VOID)
{
    return (__readsctlr_el1() & ARM64_SCTLR_M) != 0;
}

/**
 * @brief ARM64 CPU idle function
 */
VOID
NTAPI
KiArm64CpuIdle(VOID)
{
    /* Wait for interrupt */
    ARM64_WFI();
}

/**
 * @brief ARM64 CPU yield to other threads
 */
VOID  
NTAPI
KiArm64CpuYield(VOID)
{
    /* Yield hint to other threads on same core */
    ARM64_YIELD();
}

/**
 * @brief ARM64 memory barrier operations
 */
VOID
NTAPI
KiArm64MemoryBarrier(
    IN ARM64_BARRIER_TYPE Type
)
{
    switch (Type)
    {
        case ARM64_BARRIER_DSB_SY:
            ARM64_DSB_SY();
            break;
            
        case ARM64_BARRIER_DMB_SY:
            ARM64_DMB_SY();
            break;
            
        case ARM64_BARRIER_ISB:
            ARM64_ISB();
            break;
            
        default:
            ARM64_DMB_SY();  /* Default to data memory barrier */
            break;
    }
}