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
    PKPRCB Prcb = KeGetCurrentPrcb();
    NTSTATUS Status;

    /* Check the boot phase */
    if (BootPhase == 0)
    {
        /* Save bus type */
        HalpBusType = LoaderBlock->u.I386.MachineType & 0xFF;

        /* Get command-line parameters */
        HalpGetParameters(LoaderBlock);

        /* Check for PRCB version mismatch */
        if (Prcb->MajorVersion != PRCB_MAJOR_VERSION)
        {
            /* No match, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 1, Prcb->MajorVersion, PRCB_MAJOR_VERSION, 0);
        }

        /* Checked/free HAL requires checked/free kernel */
        if (Prcb->BuildType != HalpBuildType)
        {
            /* No match, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, HalpBuildType, 0);
        }

        /* Initialize ACPI */
        Status = HalpSetupAcpiPhase0(LoaderBlock);
        if (!NT_SUCCESS(Status))
        {
            KeBugCheckEx(ACPI_BIOS_ERROR, Status, 0, 0, 0);
        }

        /* Initialize the PICs */
        HalpInitializePICs(TRUE);

        /* Initialize CMOS lock */
        KeInitializeSpinLock(&HalpSystemHardwareLock);

        /* Initialize CMOS */
        HalpInitializeCmos();

        /* Fill out the dispatch tables */
        HalQuerySystemInformation = HaliQuerySystemInformation;
        HalSetSystemInformation = HaliSetSystemInformation;
        HalInitPnpDriver = HaliInitPnpDriver;
        HalGetDmaAdapter = HalpGetDmaAdapter;

        HalGetInterruptTranslator = NULL;  // FIXME: TODO
        HalResetDisplay = HalpBiosDisplayReset;
        HalHaltSystem = HaliHaltSystem;

        /* Setup I/O space */
        HalpDefaultIoSpace.Next = HalpAddressUsageList;
        HalpAddressUsageList = &HalpDefaultIoSpace;

        /* Setup busy waiting */
        HalpCalibrateStallExecution();

        /* Initialize the clock */
        HalpInitializeClock();

        /*
         * We could be rebooting with a pending profile interrupt,
         * so clear it here before interrupts are enabled
         */
        HalStopProfileInterrupt(ProfileTime);

        /* Do some HAL-specific initialization */
        HalpInitPhase0(LoaderBlock);

        /* Initialize Phase 0 of the x86 emulator */
        HalInitializeBios(0, LoaderBlock);
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

VOID
KiSwitchToKernelDebugRegisters(
    _Inout_ PKTRAP_FRAME TrapFrame)
{
    /* Save the user mode debug registers */
    TrapFrame->Dr0 = __readdr(0);
    TrapFrame->Dr1 = __readdr(1);
    TrapFrame->Dr2 = __readdr(2);
    TrapFrame->Dr3 = __readdr(3);
    TrapFrame->Dr6 = __readdr(6);
    TrapFrame->Dr7 = __readdr(7);

    /* Load the kernel mode debug registers */
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKSPECIAL_REGISTERS SpecialRegisters = &Prcb->ProcessorState.SpecialRegisters;
    __writedr(0, SpecialRegisters->KernelDr0);
    __writedr(1, SpecialRegisters->KernelDr1);
    __writedr(2, SpecialRegisters->KernelDr2);
    __writedr(3, SpecialRegisters->KernelDr3);
    __writedr(6, SpecialRegisters->KernelDr6);
    __writedr(7, SpecialRegisters->KernelDr7);
}

VOID
KiSwitchToUserDebugRegisters(
    _Inout_ PKTRAP_FRAME TrapFrame)
{
    /* Save the kernel mode debug registers */
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKSPECIAL_REGISTERS SpecialRegisters = &Prcb->ProcessorState.SpecialRegisters;
    SpecialRegisters->KernelDr0 = __readdr(0);
    SpecialRegisters->KernelDr1 = __readdr(1);
    SpecialRegisters->KernelDr2 = __readdr(2);
    SpecialRegisters->KernelDr3 = __readdr(3);
    SpecialRegisters->KernelDr6 = __readdr(6);
    SpecialRegisters->KernelDr7 = __readdr(7);

    /* Load the user mode debug registers */
    __writedr(0, TrapFrame->Dr0);
    __writedr(1, TrapFrame->Dr1);
    __writedr(2, TrapFrame->Dr2);
    __writedr(3, TrapFrame->Dr3);
    __writedr(6, TrapFrame->Dr6);
    __writedr(7, TrapFrame->Dr7);
}
