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

NTSTATUS
NTAPI
DriverEntry(
  PDRIVER_OBJECT DriverObject,
  PUNICODE_STRING RegistryPath)
{
  UNIMPLEMENTED;

  return STATUS_SUCCESS;
}

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
    if (BootPhase == 0)
    {
        /* Phase 0... save bus type */
        HalpBusType = LoaderBlock->u.I386.MachineType & 0xFF;

        /* Get command-line parameters */
        HalpGetParameters(LoaderBlock);

#if DBG
        /* Checked HAL requires checked kernel */
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

        DPRINT1("HalInitSystem 1\n");

        /* Initialize the PICs */
        HalpInitPICs();

        DPRINT1("HalInitSystem 2\n");

        /* Initialize the clock */
        HalpInitializeClock();

        DPRINT1("HalInitSystem 3\n");

        /* Calibrate counters */
//        HalpCalibrateCounters();

        /* Fill out the dispatch tables */
        HalQuerySystemInformation = HaliQuerySystemInformation;
        HalSetSystemInformation = HaliSetSystemInformation;
        HalInitPnpDriver = NULL; // FIXME: TODO
        HalGetDmaAdapter = HalpGetDmaAdapter;
        HalGetInterruptTranslator = NULL;  // FIXME: TODO
        HalResetDisplay = HalpBiosDisplayReset;

        DPRINT1("HalInitSystem 4\n");

        /* Initialize the hardware lock (CMOS) */
        KeInitializeSpinLock(&HalpSystemHardwareLock);

        /* Do some HAL-specific initialization */
        HalpInitPhase0(LoaderBlock);
    }
    else if (BootPhase == 1)
    {
        /* Initialize bus handlers */
        //HalpInitBusHandler();

        /* Enable the clock interrupt */
        HalpSetInterruptGate(0x30, HalpClockInterrupt);
        HalEnableSystemInterrupt(0x30, CLOCK_LEVEL, Latched);

        /* Initialize DMA. NT does this in Phase 0 */
        HalpInitDma();

        /* Do some HAL-specific initialization */
        HalpInitPhase1();
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
