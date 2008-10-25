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

HALP_HOOKS HalpHooks;
BOOLEAN HalpPciLockSettings;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
HalpGetParameters(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PCHAR CommandLine;

    /* Make sure we have a loader block and command line */
    if ((LoaderBlock) && (LoaderBlock->LoadOptions))
    {
        /* Read the command line */
        CommandLine = LoaderBlock->LoadOptions;

        /* Check if PCI is locked */
        if (strstr(CommandLine, "PCILOCK")) HalpPciLockSettings = TRUE;

        /* Check for initial breakpoint */
        if (strstr(CommandLine, "BREAK")) DbgBreakPoint();
    }
}

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
HalInitSystem(IN ULONG BootPhase,
              IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    KIRQL CurIrql;
    PKPRCB Prcb = KeGetCurrentPrcb();

    DbgPrint("Prcb: %x BuildType %x\n", Prcb, Prcb->BuildType);

    /* Check the boot phase */
    if (!BootPhase)
    {
        /* Phase 0... save bus type */
        HalpBusType = LoaderBlock->u.I386.MachineType & 0xFF;

        /* Get command-line parameters */
        HalpGetParameters(LoaderBlock);

        /* Checked HAL requires checked kernel */
#if DBG
        if (!(Prcb->BuildType & PRCB_BUILD_DEBUG))
        {
            /* No match, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, 1, 0);
        }
#else
        /* Release build requires release HAL */
        if (Prcb->BuildType & PRCB_BUILD_DEBUG)
        {
            /* No match, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, 0, 0);
        }
#endif

#ifdef CONFIG_SMP
        /* SMP HAL requires SMP kernel */
        if (Prcb->BuildType & PRCB_BUILD_UNIPROCESSOR)
        {
            /* No match, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, 0, 0);
        }
#endif

        /* Validate the PRCB */
        if (Prcb->MajorVersion != PRCB_MAJOR_VERSION)
        {
            /* Validation failed, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 1, Prcb->MajorVersion, 1, 0);
        }

        /* Initialize the PICs */
        HalpInitPICs();

        /* Force initial PIC state */
        KeRaiseIrql(KeGetCurrentIrql(), &CurIrql);

        /* Initialize the clock */
        HalpInitializeClock();

        /* Setup busy waiting */
        //HalpCalibrateStallExecution();

        /* Fill out the dispatch tables */
        HalQuerySystemInformation = HaliQuerySystemInformation;
        HalSetSystemInformation = HaliSetSystemInformation;
        HalInitPnpDriver = NULL; // FIXME: TODO
        HalGetDmaAdapter = HalpGetDmaAdapter;
        HalGetInterruptTranslator = NULL;  // FIXME: TODO

        /* Initialize the hardware lock (CMOS) */
        KeInitializeSpinLock(&HalpSystemHardwareLock);
    }
    else if (BootPhase == 1)
    {
        /* Initialize the default HAL stubs for bus handling functions */
        HalpInitNonBusHandler();

#if 0
        /* Enable the clock interrupt */
        ((PKIPCR)KeGetPcr())->IDT[0x30].ExtendedOffset =
            (USHORT)(((ULONG_PTR)HalpClockInterrupt >> 16) & 0xFFFF);
        ((PKIPCR)KeGetPcr())->IDT[0x30].Offset =
            (USHORT)((ULONG_PTR)HalpClockInterrupt);
#endif
        HalEnableSystemInterrupt(0x30, CLOCK2_LEVEL, Latched);

        /* Initialize DMA. NT does this in Phase 0 */
        HalpInitDma();
    }

    /* All done, return */
    return TRUE;
}

/*
 * @unimplemented
 */
VOID
NTAPI
HalReportResourceUsage(VOID)
{
    /* Initialize PCI bus. */
    HalpInitializePciBus();

    /* FIXME: This is done in ReactOS MP HAL only*/
    //HaliReconfigurePciInterrupts();

    /* FIXME: Report HAL Usage to kernel */
}

/* EOF */
