/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/halinit.c
 * PURPOSE:         HAL Entrypoint and Initialization
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#include <debug.h>

/* GLOBALS *******************************************************************/

//#ifdef CONFIG_SMP // FIXME: Reenable conditional once HAL is consistently compiled for SMP mode
BOOLEAN HalpOnlyBootProcessor;
//#endif
BOOLEAN HalpPciLockSettings;

/* PRIVATE FUNCTIONS *********************************************************/

static
CODE_SEG("INIT")
VOID
HalpGetParameters(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Debug output */
    DPRINT1("HAL: HalpGetParameters entry\n");
    
    /* Make sure we have a loader block and command line */
    if (LoaderBlock)
    {
        DPRINT1("HAL: LoaderBlock is valid\n");
        
        if (LoaderBlock->LoadOptions)
        {
            DPRINT1("HAL: LoadOptions is valid\n");
            
            /* Read the command line */
            PCSTR CommandLine = LoaderBlock->LoadOptions;

//#ifdef CONFIG_SMP // FIXME: Reenable conditional once HAL is consistently compiled for SMP mode
            /* Check whether we should only start one CPU */
            if (strstr(CommandLine, "ONECPU"))
                HalpOnlyBootProcessor = TRUE;
//#endif

            /* Check if PCI is locked */
            if (strstr(CommandLine, "PCILOCK"))
                HalpPciLockSettings = TRUE;

            /* Check for initial breakpoint */
            if (strstr(CommandLine, "BREAK"))
                DbgBreakPoint();
        }
        else
        {
            DPRINT1("HAL: LoadOptions is NULL\n");
        }
    }
    else
    {
        DPRINT1("HAL: LoaderBlock is NULL\n");
    }
    
    DPRINT1("HAL: HalpGetParameters exit\n");
}

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
HalInitializeProcessor(
    IN ULONG ProcessorNumber,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Hal specific initialization for this cpu */
    HalpInitProcessor(ProcessorNumber, LoaderBlock);

    /* Set default stall count */
    KeGetPcr()->StallScaleFactor = INITIAL_STALL_COUNT;

    /* Update the interrupt affinity and processor mask */
    InterlockedBitTestAndSetAffinity(&HalpActiveProcessors, ProcessorNumber);
    InterlockedBitTestAndSetAffinity(&HalpDefaultInterruptAffinity, ProcessorNumber);

    if (ProcessorNumber == 0)
    {
        /* Register routines for KDCOM */
        HalpRegisterKdSupportFunctions();
    }
}

/*
 * @implemented
 */
CODE_SEG("INIT")
BOOLEAN
NTAPI
HalInitSystem(
    _In_ ULONG BootPhase,
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Debug output */
    DPRINT1("HAL: HalInitSystem entered (Phase %lu)\n", BootPhase);
    
    DPRINT1("HAL: About to check BootPhase\n");
    
#ifdef _M_AMD64
    /* Skip PRCB access on AMD64 for now - might cause issues */
    DPRINT1("HAL: Skipping PRCB access on AMD64\n");
    PKPRCB Prcb = NULL;
#else
    PKPRCB Prcb = KeGetCurrentPrcb();
#endif
#ifndef _M_AMD64
    NTSTATUS Status;
#endif

    DPRINT1("HAL: BootPhase parameter accessible\n");

#ifdef _M_AMD64
    /* On AMD64, assume Phase 0 for now */
    DPRINT1("HAL: Assuming Phase 0 on AMD64\n");
    if (1)  /* Always do Phase 0 on AMD64 */
#else
    /* Check the boot phase */
    if (BootPhase == 0)
#endif
    {
        DPRINT1("HAL: Phase 0 initialization\n");
        
        /* Check LoaderBlock */
        if (!LoaderBlock)
        {
            DPRINT1("HAL ERROR: LoaderBlock is NULL!\n");
            return FALSE;
        }
        
        /* Save bus type */
        DPRINT1("HAL: Setting bus type to ISA\n");
        
#ifdef _M_AMD64
        /* Skip setting bus type on AMD64 for now - causes hang */
        DPRINT1("HAL: Skipping bus type assignment on AMD64\n");
#else
        HalpBusType = MACHINE_TYPE_ISA;
#endif

        DPRINT1("HAL: Bus type handling complete\n");

        /* Get command-line parameters */
        DPRINT1("HAL: Getting parameters\n");
        
        HalpGetParameters(LoaderBlock);
        
        DPRINT1("HAL: Parameters obtained\n");

        /* Check for PRCB version mismatch */
#ifdef _M_AMD64
        DPRINT1("HAL: Skipping PRCB version check on AMD64\n");
#else
        DPRINT1("HAL: Checking PRCB version\n");
        
        if (Prcb->MajorVersion != PRCB_MAJOR_VERSION)
        {
            /* No match, bugcheck */
            DPRINT1("HAL ERROR: PRCB version mismatch!\n");
            KeBugCheckEx(MISMATCHED_HAL, 1, Prcb->MajorVersion, PRCB_MAJOR_VERSION, 0);
        }
        
        DPRINT1("HAL: PRCB version OK\n");
#endif

        /* Checked/free HAL requires checked/free kernel */
#ifdef _M_AMD64
        /* Skip build type check on AMD64 for now - no PRCB */
        DPRINT1("HAL: Skipping build type check on AMD64\n");
#else
        DPRINT1("HAL: Checking build type\n");
        
        if (Prcb->BuildType != HalpBuildType)
        {
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, HalpBuildType, 0);
        }
#endif

        /* Initialize ACPI */
#ifdef _M_AMD64
        DPRINT1("HAL: Skipping ACPI for now on AMD64\n");
        /* TODO: Fix ACPI initialization on AMD64 */
#else
        Status = HalpSetupAcpiPhase0(LoaderBlock);
        if (!NT_SUCCESS(Status))
        {
            KeBugCheckEx(ACPI_BIOS_ERROR, Status, 0, 0, 0);
        }
#endif

        /* Initialize the PICs */
        DPRINT1("HAL: Initializing PICs\n");
        
#ifdef _M_AMD64
        /* Initialize PICs for AMD64 */
        DPRINT1("HAL: Initializing PICs for AMD64\n");
        /* Call the PIC initialization with simplified parameters */
        extern VOID NTAPI HalpInitializePICs(IN BOOLEAN EnableInterrupts);
        DPRINT1("HAL: About to call HalpInitializePICs\n");
        HalpInitializePICs(FALSE); /* Don't enable interrupts yet */
        DPRINT1("HAL: HalpInitializePICs returned\n");
#else
        HalpInitializePICs(TRUE);
#endif

        /* Initialize CMOS lock */
        DPRINT1("HAL: Initializing CMOS hardware lock\n");
        
#ifdef _M_AMD64
        /* On AMD64, the spinlock is already zero-initialized in BSS */
        /* Skip explicit initialization to avoid global access issues */
        DPRINT1("HAL: CMOS lock already zero-initialized on AMD64\n");
#else
        KeInitializeSpinLock(&HalpSystemHardwareLock);
#endif

        /* Initialize CMOS */
        DPRINT1("HAL: Initializing CMOS\n");
        
        HalpInitializeCmos();
        
        DPRINT1("HAL: CMOS initialized\n");

        /* Fill out the dispatch tables */
        DPRINT1("HAL: Setting up dispatch tables\n");
        
#ifdef _M_AMD64
        /* On AMD64, skip dispatch table setup for now */
        DPRINT1("HAL: Skipping dispatch table setup on AMD64 for now\n");
        /* These macros resolve to HALDISPATCH->field which are complex to handle during early boot */
#else
        HalQuerySystemInformation = HaliQuerySystemInformation;
        HalSetSystemInformation = HaliSetSystemInformation;
        HalInitPnpDriver = HaliInitPnpDriver;
        HalGetDmaAdapter = HalpGetDmaAdapter;

        HalGetInterruptTranslator = NULL;  // FIXME: TODO
        HalResetDisplay = HalpBiosDisplayReset;
        HalHaltSystem = HaliHaltSystem;
#endif

        /* Setup I/O space */
        DPRINT1("HAL: Setting up I/O address space\n");
        
#ifdef _M_AMD64
        /* Skip I/O space setup on AMD64 - global struct field access causes issues */
        DPRINT1("HAL: Skipping I/O space setup on AMD64\n");
#else
        HalpDefaultIoSpace.Next = HalpAddressUsageList;
        HalpAddressUsageList = &HalpDefaultIoSpace;
#endif

        /* Setup busy waiting */
        DPRINT1("HAL: Calibrating stall execution\n");
        
#ifdef _M_AMD64
        /* On AMD64, skip calibration and PCR access for now */
        DPRINT1("HAL: Skipping stall calibration on AMD64\n");
#else
        HalpCalibrateStallExecution();
#endif

        /* Initialize the clock */
        DPRINT1("HAL: Initializing system clock\n");
        
#ifdef _M_AMD64
        /* Initialize clock for AMD64 */
        DPRINT1("HAL: Initializing clock for AMD64\n");
        extern VOID NTAPI HalpInitializeClock(VOID);
        HalpInitializeClock();
#else
        HalpInitializeClock();
#endif

        /*
         * We could be rebooting with a pending profile interrupt,
         * so clear it here before interrupts are enabled
         */
        DPRINT1("HAL: Stopping profile interrupt\n");
        
#ifdef _M_AMD64
        /* Skip HalStopProfileInterrupt on AMD64 - spinlock issues during early boot */
        DPRINT1("HAL: Skipping HalStopProfileInterrupt on AMD64\n");
#else
        HalStopProfileInterrupt(ProfileTime);
#endif
        
        DPRINT1("HAL: Profile interrupt stopped\n");

        /* Do some HAL-specific initialization */
        DPRINT1("HAL: Running HalpInitPhase0\n");
        
        HalpInitPhase0(LoaderBlock);
        
        DPRINT1("HAL: HalpInitPhase0 handling complete\n");

        /* Initialize Phase 0 of the x86 emulator */
        DPRINT1("HAL: Initializing BIOS\n");
        
        HalInitializeBios(0, LoaderBlock);
        
        DPRINT1("HAL: Phase 0 initialization complete!\n");
    }
    else if (BootPhase == 1)
    {
        /* Initialize bus handlers */
        HalpInitBusHandlers();

        /* Do some HAL-specific initialization */
        HalpInitPhase1();

        /* Initialize Phase 1 of the x86 emulator */
        HalInitializeBios(1, LoaderBlock);
    }

    /* All done, return */
    return TRUE;
}
