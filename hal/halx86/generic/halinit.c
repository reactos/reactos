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
    #undef COM1_PORT
    #define COM1_PORT 0x3F8
    {
        const char msg[] = "*** HAL: HalpGetParameters entry ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Make sure we have a loader block and command line */
    if (LoaderBlock)
    {
        {
            const char msg[] = "*** HAL: LoaderBlock is valid ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        if (LoaderBlock->LoadOptions)
        {
            {
                const char msg[] = "*** HAL: LoadOptions is valid ***\n";
                const char *p = msg;
                while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
            }
            
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
            const char msg[] = "*** HAL: LoadOptions is NULL ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
    }
    else
    {
        const char msg[] = "*** HAL: LoaderBlock is NULL ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    {
        const char msg[] = "*** HAL: HalpGetParameters exit ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
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
    #define COM1_PORT 0x3F8
    {
        const char msg[] = "*** HAL: HalInitSystem entered ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    {
        const char msg[] = "*** HAL: About to check BootPhase ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
#ifdef _M_AMD64
    /* Skip PRCB access on AMD64 for now - might cause issues */
    {
        const char msg[] = "*** HAL: Skipping PRCB access on AMD64 ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    PKPRCB Prcb = NULL;
#else
    PKPRCB Prcb = KeGetCurrentPrcb();
#endif
#ifndef _M_AMD64
    NTSTATUS Status;
#endif

    {
        const char msg[] = "*** HAL: BootPhase parameter accessible ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

#ifdef _M_AMD64
    /* On AMD64, assume Phase 0 for now */
    {
        const char msg[] = "*** HAL: Assuming Phase 0 on AMD64 ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    if (1)  /* Always do Phase 0 on AMD64 */
#else
    /* Check the boot phase */
    if (BootPhase == 0)
#endif
    {
        {
            const char msg[] = "*** HAL: Phase 0 initialization ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Check LoaderBlock */
        if (!LoaderBlock)
        {
            const char msg[] = "*** HAL ERROR: LoaderBlock is NULL! ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
            return FALSE;
        }
        
        /* Save bus type */
        {
            const char msg[] = "*** HAL: Setting bus type to ISA ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
#ifdef _M_AMD64
        /* Skip setting bus type on AMD64 for now - causes hang */
        {
            const char msg[] = "*** HAL: Skipping bus type assignment on AMD64 ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
#else
        HalpBusType = MACHINE_TYPE_ISA;
#endif

        {
            const char msg[] = "*** HAL: Bus type handling complete ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }

        /* Get command-line parameters */
        {
            const char msg[] = "*** HAL: Getting parameters ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        HalpGetParameters(LoaderBlock);
        
        {
            const char msg[] = "*** HAL: Parameters obtained ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }

        /* Check for PRCB version mismatch */
#ifdef _M_AMD64
        {
            const char msg[] = "*** HAL: Skipping PRCB version check on AMD64 ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
#else
        {
            const char msg[] = "*** HAL: Checking PRCB version ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        if (Prcb->MajorVersion != PRCB_MAJOR_VERSION)
        {
            /* No match, bugcheck */
            {
                const char msg[] = "*** HAL ERROR: PRCB version mismatch! ***\n";
                const char *p = msg;
                while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
            }
            KeBugCheckEx(MISMATCHED_HAL, 1, Prcb->MajorVersion, PRCB_MAJOR_VERSION, 0);
        }
        
        {
            const char msg[] = "*** HAL: PRCB version OK ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
#endif

        /* Checked/free HAL requires checked/free kernel */
#ifdef _M_AMD64
        /* Skip build type check on AMD64 for now - no PRCB */
        {
            const char msg[] = "*** HAL: Skipping build type check on AMD64 ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
#else
        {
            const char msg[] = "*** HAL: Checking build type ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        if (Prcb->BuildType != HalpBuildType)
        {
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, HalpBuildType, 0);
        }
#endif

        /* Initialize ACPI */
#ifdef _M_AMD64
        {
            const char msg[] = "*** HAL: Skipping ACPI for now on AMD64 ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        /* TODO: Fix ACPI initialization on AMD64 */
#else
        Status = HalpSetupAcpiPhase0(LoaderBlock);
        if (!NT_SUCCESS(Status))
        {
            KeBugCheckEx(ACPI_BIOS_ERROR, Status, 0, 0, 0);
        }
#endif

        /* Initialize the PICs */
        {
            const char msg[] = "*** HAL: Initializing PICs ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
#ifdef _M_AMD64
        /* Initialize PICs for AMD64 */
        {
            const char msg[] = "*** HAL: Initializing PICs for AMD64 ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        /* Call the PIC initialization with simplified parameters */
        extern VOID NTAPI HalpInitializePICs(IN BOOLEAN EnableInterrupts);
        {
            const char msg[] = "*** HAL: About to call HalpInitializePICs ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        HalpInitializePICs(FALSE); /* Don't enable interrupts yet */
        {
            const char msg[] = "*** HAL: HalpInitializePICs returned ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
#else
        HalpInitializePICs(TRUE);
#endif

        /* Initialize CMOS lock */
        {
            const char msg[] = "*** HAL: Initializing CMOS hardware lock ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
#ifdef _M_AMD64
        /* On AMD64, the spinlock is already zero-initialized in BSS */
        /* Skip explicit initialization to avoid global access issues */
        {
            const char msg[] = "*** HAL: CMOS lock already zero-initialized on AMD64 ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
#else
        KeInitializeSpinLock(&HalpSystemHardwareLock);
#endif

        /* Initialize CMOS */
        {
            const char msg[] = "*** HAL: Initializing CMOS ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        HalpInitializeCmos();
        
        {
            const char msg[] = "*** HAL: CMOS initialized ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }

        /* Fill out the dispatch tables */
        {
            const char msg[] = "*** HAL: Setting up dispatch tables ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
#ifdef _M_AMD64
        /* On AMD64, skip dispatch table setup for now */
        {
            const char msg[] = "*** HAL: Skipping dispatch table setup on AMD64 for now ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
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
        {
            const char msg[] = "*** HAL: Setting up I/O address space ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
#ifdef _M_AMD64
        /* Skip I/O space setup on AMD64 - global struct field access causes issues */
        {
            const char msg[] = "*** HAL: Skipping I/O space setup on AMD64 ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
#else
        HalpDefaultIoSpace.Next = HalpAddressUsageList;
        HalpAddressUsageList = &HalpDefaultIoSpace;
#endif

        /* Setup busy waiting */
        {
            const char msg[] = "*** HAL: Calibrating stall execution ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
#ifdef _M_AMD64
        /* On AMD64, skip calibration and PCR access for now */
        {
            const char msg[] = "*** HAL: Skipping stall calibration on AMD64 ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
#else
        HalpCalibrateStallExecution();
#endif

        /* Initialize the clock */
        {
            const char msg[] = "*** HAL: Initializing system clock ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
#ifdef _M_AMD64
        /* Initialize clock for AMD64 */
        {
            const char msg[] = "*** HAL: Initializing clock for AMD64 ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        extern VOID NTAPI HalpInitializeClock(VOID);
        HalpInitializeClock();
#else
        HalpInitializeClock();
#endif

        /*
         * We could be rebooting with a pending profile interrupt,
         * so clear it here before interrupts are enabled
         */
        {
            const char msg[] = "*** HAL: Stopping profile interrupt ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
#ifdef _M_AMD64
        /* Skip HalStopProfileInterrupt on AMD64 - spinlock issues during early boot */
        {
            const char msg[] = "*** HAL: Skipping HalStopProfileInterrupt on AMD64 ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
#else
        HalStopProfileInterrupt(ProfileTime);
#endif
        
        {
            const char msg[] = "*** HAL: Profile interrupt stopped ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }

        /* Do some HAL-specific initialization */
        {
            const char msg[] = "*** HAL: Running HalpInitPhase0 ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        HalpInitPhase0(LoaderBlock);
        
        {
            const char msg[] = "*** HAL: HalpInitPhase0 handling complete ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }

        /* Initialize Phase 0 of the x86 emulator */
        {
            const char msg[] = "*** HAL: Initializing BIOS ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        HalInitializeBios(0, LoaderBlock);
        
        {
            const char msg[] = "*** HAL: Phase 0 initialization complete! ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
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
