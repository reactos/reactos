/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 Kernel Initialization
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* ARM64 processor information */
ULONG KiProcessorArchitecture = PROCESSOR_ARCHITECTURE_ARM64;
ULONG KiProcessorLevel = 8;  /* ARMv8 */
ULONG KiProcessorRevision = 0;

/* ARM64 CPU features */
ULONG64 KiArm64Features = 0;

/* ARM64 Cache information */
ULONG KiDcacheLineSize = 64;
ULONG KiIcacheLineSize = 64;

/* ARM64 Timer frequency */
ULONG64 KiTimerFrequency = 0;

/* PCR and PRCB */
KIPCR KiInitialPcr;
KPRCB KiInitialPrcb;

/* FUNCTIONS *****************************************************************/

/**
 * @brief Initialize ARM64 processor features and capabilities
 */
VOID
NTAPI
KiInitializeArm64Features(VOID)
{
    ULONGLONG midr, idr0, pfr0;
    
    /* Read processor identification */
    midr = __readmidr();
    idr0 = ARM64_READ_SYSREG(id_aa64isar0_el1);
    pfr0 = ARM64_READ_SYSREG(id_aa64pfr0_el1);
    
    /* Extract processor information */
    KiProcessorRevision = (ULONG)(midr & 0xF);
    
    /* Detect CPU features */
    KiArm64Features = 0;
    
    /* Check for AES support */
    if ((idr0 & 0xF0) != 0)
    {
        KiArm64Features |= ARM64_FEATURE_AES;
        DPRINT("ARM64: AES encryption support detected\n");
    }
    
    /* Check for SHA support */
    if (((idr0 >> 8) & 0xF) != 0)
    {
        KiArm64Features |= ARM64_FEATURE_SHA;
        DPRINT("ARM64: SHA hash support detected\n");
    }
    
    /* Check for floating point support */
    if ((pfr0 & 0xF) != 0xF)
    {
        KiArm64Features |= ARM64_FEATURE_FP;
        DPRINT("ARM64: Floating point support detected\n");
    }
    
    /* Check for Advanced SIMD support */
    if (((pfr0 >> 4) & 0xF) != 0xF)
    {
        KiArm64Features |= ARM64_FEATURE_ASIMD;
        DPRINT("ARM64: Advanced SIMD support detected\n");
    }
    
    /* Check for CRC32 support */
    if (((idr0 >> 16) & 0xF) != 0)
    {
        KiArm64Features |= ARM64_FEATURE_CRC32;
        DPRINT("ARM64: CRC32 support detected\n");
    }
    
    /* Check for Atomic instructions support */
    if (((idr0 >> 20) & 0xF) != 0)
    {
        KiArm64Features |= ARM64_FEATURE_ATOMIC;
        DPRINT("ARM64: Atomic instructions support detected\n");
    }
    
    DPRINT("ARM64: Features=0x%llx, Revision=%u\n", KiArm64Features, KiProcessorRevision);
}

/**
 * @brief Initialize ARM64 cache information
 */
VOID
NTAPI
KiInitializeArm64Cache(VOID)
{
    ULONGLONG ctr, ccsidr;
    
    /* Read cache type register */
    ctr = ARM64_READ_SYSREG(ctr_el0);
    
    /* Extract cache line sizes */
    KiDcacheLineSize = 4 << ((ctr & 0xF0000) >> 16);
    KiIcacheLineSize = 4 << (ctr & 0xF);
    
    /* Select L1 data cache */
    ARM64_WRITE_SYSREG(csselr_el1, 0);
    ARM64_ISB();
    
    ccsidr = ARM64_READ_SYSREG(ccsidr_el1);
    
    DPRINT("ARM64: DCache line=%u bytes, ICache line=%u bytes\n", 
           KiDcacheLineSize, KiIcacheLineSize);
    DPRINT("ARM64: L1 DCache CCSIDR=0x%llx\n", ccsidr);
}

/**
 * @brief Initialize ARM64 Generic Timer
 */
VOID
NTAPI
KiInitializeArm64Timer(VOID)
{
    /* Read timer frequency */
    KiTimerFrequency = __readcntfrq();
    
    if (KiTimerFrequency == 0)
    {
        /* Use default frequency if not set by firmware */
        KiTimerFrequency = ARM64_TIMER_FREQ_DEFAULT;
        DPRINT("ARM64: Using default timer frequency %llu Hz\n", KiTimerFrequency);
    }
    else
    {
        DPRINT("ARM64: Timer frequency %llu Hz\n", KiTimerFrequency);
    }
    
    /* Disable timer interrupt initially */
    __writecntp_ctl_el0(0);
    ARM64_ISB();
}

/**
 * @brief Initialize ARM64 Memory Management Unit
 */
VOID
NTAPI
KiInitializeArm64Mmu(VOID)
{
    ULONGLONG tcr, mair, sctlr;
    
    /* Configure Memory Attribute Indirection Register */
    mair = ARM64_MAIR_VALUE;
    __writemair_el1(mair);
    
    /* Configure Translation Control Register */
    tcr = ARM64_TCR_DEFAULT;
    __writetcr_el1(tcr);
    
    /* Read current SCTLR */
    sctlr = __readsctlr_el1();
    
    /* Enable MMU if not already enabled */
    if (!(sctlr & ARM64_SCTLR_M))
    {
        DPRINT("ARM64: Enabling MMU\n");
        sctlr |= ARM64_SCTLR_DEFAULT;
        __writesctlr_el1(sctlr);
    }
    else
    {
        DPRINT("ARM64: MMU already enabled\n");
    }
    
    DPRINT("ARM64: TCR=0x%llx, MAIR=0x%llx, SCTLR=0x%llx\n", tcr, mair, sctlr);
}

/**
 * @brief Initialize ARM64 Exception Handling
 */
VOID
NTAPI
KiInitializeArm64Exceptions(VOID)
{
    extern VOID KiExceptionVectors(VOID);
    ULONGLONG vbar;
    
    /* Set Vector Base Address Register */
    vbar = (ULONGLONG)&KiExceptionVectors;
    __writevbar_el1(vbar);
    
    DPRINT("ARM64: Exception vectors at 0x%llx\n", vbar);
}

/**
 * @brief Initialize ARM64 Processor Control Region (PCR)
 */
VOID
NTAPI
KiInitializeArm64Pcr(
    IN PKIPCR Pcr,
    IN ULONG ProcessorNumber,
    IN PKTHREAD IdleThread,
    IN PVOID DpcStack
)
{
    /* Clear the PCR */
    RtlZeroMemory(Pcr, sizeof(KIPCR));
    
    /* Set up basic PCR fields */
    Pcr->MajorVersion = PCR_MAJOR_VERSION;
    Pcr->MinorVersion = PCR_MINOR_VERSION;
    Pcr->PrcbData.MajorVersion = PRCB_MAJOR_VERSION;
    Pcr->PrcbData.MinorVersion = PRCB_MINOR_VERSION;
    Pcr->PrcbData.BuildType = 0;
    
    /* Set processor number */
    Pcr->PrcbData.Number = (UCHAR)ProcessorNumber;
    Pcr->PrcbData.SetMember = 1ULL << ProcessorNumber;
    
    /* Initialize PRCB */
    Pcr->PrcbData.CurrentThread = IdleThread;
    Pcr->PrcbData.NextThread = NULL;
    Pcr->PrcbData.IdleThread = IdleThread;
    
    /* Set DPC stack */
    Pcr->PrcbData.DpcStack = DpcStack;
    
    /* Initialize processor features */
    Pcr->PrcbData.FeatureBits = (ULONG)KiArm64Features;
    
    /* Initialize cache information */
    Pcr->PrcbData.CacheLineSize = KiDcacheLineSize;
    
    DPRINT("ARM64: PCR initialized for processor %u\n", ProcessorNumber);
}

/**
 * @brief Early ARM64 processor initialization
 */
VOID
NTAPI
KiInitializeProcessor(VOID)
{
    ULONGLONG el;
    
    /* Check current Exception Level */
    el = __readcurrentel() >> 2;
    DPRINT("ARM64: Running at Exception Level %llu\n", el);
    
    if (el != 1)
    {
        DPRINT1("ARM64: Warning - Not running at EL1!\n");
    }
    
    /* Initialize ARM64 features */
    KiInitializeArm64Features();
    
    /* Initialize cache information */
    KiInitializeArm64Cache();
    
    /* Initialize Generic Timer */
    KiInitializeArm64Timer();
    
    /* Initialize MMU */
    KiInitializeArm64Mmu();
    
    /* Initialize exception handling */
    KiInitializeArm64Exceptions();
    
    DPRINT("ARM64: Processor initialization completed\n");
}

/**
 * @brief Initialize ARM64 kernel
 */
NTSTATUS
NTAPI
KiInitializeKernel(
    IN PKPROCESS InitProcess,
    IN PKTHREAD InitThread,
    IN PVOID IdleStack,
    IN PKPRCB Prcb,
    IN CCHAR Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
)
{
    /* Early processor initialization */
    KiInitializeProcessor();
    
    /* Initialize PCR */
    KiInitializeArm64Pcr((PKIPCR)&KiInitialPcr, Number, InitThread, IdleStack);
    
    /* Initialize PRCB */
    RtlCopyMemory(&KiInitialPrcb, &KiInitialPcr.PrcbData, sizeof(KPRCB));
    
    /* Set up initial thread */
    InitThread->ApcState.Process = InitProcess;
    InitProcess->Pcb.DirectoryTableBase = __readttbr1_el1();
    
    DPRINT("ARM64: Kernel initialization completed\n");
    
    return STATUS_SUCCESS;
}

/**
 * @brief System startup routine called from boot code
 */
VOID
NTAPI
KiSystemStartupBootStack(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
)
{
    DPRINT("ARM64: System startup - LoaderBlock at 0x%p\n", LoaderBlock);
    
    /* Disable interrupts during initialization */
    ARM64_DISABLE_INTERRUPTS();
    
    /* Early kernel initialization */
    KiInitializeKernel(NULL, NULL, NULL, NULL, 0, LoaderBlock);
    
    /* Call generic kernel startup */
    KiSystemStartup(LoaderBlock);
    
    /* Should never reach here */
    KeBugCheck(KERNEL_INITIALIZATION_FAILURE);
}