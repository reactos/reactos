/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/halinit.c
 * PURPOSE:         HAL Entrypoint and Initialization
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
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
    /* Make sure we have a loader block and command line */
    if (LoaderBlock && LoaderBlock->LoadOptions)
    {
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
    
    PKPRCB Prcb = KeGetCurrentPrcb();
    NTSTATUS Status;

    /* Check the boot phase */
    if (BootPhase == 0)
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
            const char msg[] = "*** HAL: Skipping bus type - global var issue on AMD64 ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Skip setting HalpBusType - global variable access issue on AMD64 */
        /* HalpBusType = MACHINE_TYPE_ISA; */

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

        /* Checked/free HAL requires checked/free kernel */
        {
            const char msg[] = "*** HAL: Skipping build type check (global var issue) ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Skip build type check - HalpBuildType global access issue on AMD64 */
        /*
        if (Prcb->BuildType != HalpBuildType)
        {
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, HalpBuildType, 0);
        }
        */

        /* Initialize ACPI */
        {
            const char msg[] = "*** HAL: Skipping ACPI (crashes on AMD64) ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Skip ACPI for now - crashes on AMD64 */
        /*
        Status = HalpSetupAcpiPhase0(LoaderBlock);
        if (!NT_SUCCESS(Status))
        {
            KeBugCheckEx(ACPI_BIOS_ERROR, Status, 0, 0, 0);
        }
        */
        Status = STATUS_SUCCESS;

        /* Initialize the PICs */
        {
            const char msg[] = "*** HAL: Skipping PIC initialization (crashes on AMD64) ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Skip PIC initialization - crashes on AMD64 */
        /* HalpInitializePICs(TRUE); */

        /* Initialize CMOS lock */
        {
            const char msg[] = "*** HAL: Skipping CMOS lock init (global var issue) ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Skip KeInitializeSpinLock - global variable access issue on AMD64 */
        /* KeInitializeSpinLock(&HalpSystemHardwareLock); */

        /* Initialize CMOS */
        {
            const char msg[] = "*** HAL: Skipping CMOS init (global var issue) ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Skip HalpInitializeCmos - global variable access issue on AMD64 */
        /* HalpInitializeCmos(); */

        /* Fill out the dispatch tables */
        {
            const char msg[] = "*** HAL: Skipping dispatch table setup (global ptr issues) ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Skip dispatch table setup - global function pointer issues on AMD64 */
        /*
        HalQuerySystemInformation = HaliQuerySystemInformation;
        HalSetSystemInformation = HaliSetSystemInformation;
        HalInitPnpDriver = HaliInitPnpDriver;
        HalGetDmaAdapter = HalpGetDmaAdapter;

        HalGetInterruptTranslator = NULL;  // FIXME: TODO
        HalResetDisplay = HalpBiosDisplayReset;
        HalHaltSystem = HaliHaltSystem;
        */

        /* Setup I/O space */
        {
            const char msg[] = "*** HAL: Skipping I/O space setup (global issues) ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Skip I/O space setup - global pointer issues */
        /*
        HalpDefaultIoSpace.Next = HalpAddressUsageList;
        HalpAddressUsageList = &HalpDefaultIoSpace;
        */

        /* Setup busy waiting */
        {
            const char msg[] = "*** HAL: Skipping stall calibration (CMOS issue) ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Skip HalpCalibrateStallExecution - uses CMOS which has issues on AMD64 */
        /* HalpCalibrateStallExecution(); */
        
        /* Set a default stall scale factor */
        {
            const char msg[] = "*** HAL: Setting default stall scale factor ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        KeGetPcr()->StallScaleFactor = INITIAL_STALL_COUNT;

        /* Initialize the clock */
        {
            const char msg[] = "*** HAL: Skipping clock init (global var issue) ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Skip HalpInitializeClock - uses global variables which have issues on AMD64 */
        /* HalpInitializeClock(); */

        /*
         * We could be rebooting with a pending profile interrupt,
         * so clear it here before interrupts are enabled
         */
        {
            const char msg[] = "*** HAL: Stopping profile interrupt ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        HalStopProfileInterrupt(ProfileTime);
        
        {
            const char msg[] = "*** HAL: Profile interrupt stopped ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }

        /* Do some HAL-specific initialization */
        {
            const char msg[] = "*** HAL: Skipping HalpInitPhase0 (global var issue) ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Skip HalpInitPhase0 - uses global variables on AMD64 */
        /* HalpInitPhase0(LoaderBlock); */

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
