/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/hal/halx86/generic/processor.c
 * PURPOSE:         HAL Processor Routines
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
    PKPRCB Prcb = KeGetCurrentPrcb();

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

        /* Continue with HAL-specific initialization */
        HalpInitPhase0(LoaderBlock);

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

        /* Initialize the clock interrupt */
        HalpInitPhase1();

        /* Initialize DMA. NT does this in Phase 0 */
        HalpInitDma();
    }

    /* All done, return */
    return TRUE;
}

/*
 * @implemented
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
