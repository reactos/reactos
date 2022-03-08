/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/generic/halinit.c
 * PURPOSE:         HAL Entrypoint and Initialization
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* PRIVATE FUNCTIONS **********************************************************/

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

        /* Check for initial breakpoint */
        if (strstr(CommandLine, "BREAK")) DbgBreakPoint();
    }
}

/* FUNCTIONS ******************************************************************/

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
        /* Get command-line parameters */
        HalpGetParameters(LoaderBlock);

        /* Checked HAL requires checked kernel */
#if DBG
        if (!(Prcb->BuildType & PRCB_BUILD_DEBUG))
        {
            /* No match, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, PRCB_BUILD_DEBUG, 0);
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
            KeBugCheckEx(MISMATCHED_HAL, 1, Prcb->MajorVersion, PRCB_MAJOR_VERSION, 0);
        }

        /* Initialize interrupts */
        HalpInitializeInterrupts();

        /* Force initial PIC state */
        KfRaiseIrql(KeGetCurrentIrql());

        /* Fill out the dispatch tables */
        //HalQuerySystemInformation = NULL; // FIXME: TODO;
        //HalSetSystemInformation = NULL; // FIXME: TODO;
        //HalInitPnpDriver = NULL; // FIXME: TODO
        //HalGetDmaAdapter = NULL; // FIXME: TODO;
        //HalGetInterruptTranslator = NULL;  // FIXME: TODO
        //HalResetDisplay = NULL; // FIXME: TODO;
        //HalHaltSystem = NULL; // FIXME: TODO;

        /* Setup I/O space */
        //HalpDefaultIoSpace.Next = HalpAddressUsageList;
        //HalpAddressUsageList = &HalpDefaultIoSpace;

        /* Setup busy waiting */
        //HalpCalibrateStallExecution();

        /* Initialize the clock */
        HalpInitializeClock();

        /* Setup time increments to 10ms and 1ms */
        HalpCurrentTimeIncrement = 100000;
        HalpNextTimeIncrement = 100000;
        HalpNextIntervalCount = 0;
        KeSetTimeIncrement(100000, 10000);

        /*
         * We could be rebooting with a pending profile interrupt,
         * so clear it here before interrupts are enabled
         */
        HalStopProfileInterrupt(ProfileTime);

        /* Do some HAL-specific initialization */
        HalpInitPhase0(LoaderBlock);
    }
    else if (BootPhase == 1)
    {
        /* Enable timer interrupt */
        HalpEnableInterruptHandler(IDT_DEVICE,
                                   0,
                                   PRIMARY_VECTOR_BASE,
                                   CLOCK2_LEVEL,
                                   HalpClockInterrupt,
                                   Latched);
#if 0
        /* Enable IRQ 8 */
        HalpEnableInterruptHandler(IDT_DEVICE,
                                   0,
                                   PRIMARY_VECTOR_BASE + 8,
                                   PROFILE_LEVEL,
                                   HalpProfileInterrupt,
                                   Latched);
#endif
        /* Initialize DMA. NT does this in Phase 0 */
        //HalpInitDma();

        /* Do some HAL-specific initialization */
        HalpInitPhase1();
    }

    /* All done, return */
    return TRUE;
}

#include <internal/kd.h>
ULONG
DbgPrintEarly(const char *fmt, ...)
{
    va_list args;
    unsigned int i;
    char Buffer[1024];
    PCHAR String = Buffer;

    va_start(args, fmt);
    i = vsprintf(Buffer, fmt, args);
    va_end(args);

    /* Output the message */
    while (*String != 0)
    {
        if (*String == '\n')
        {
            KdPortPutByteEx(NULL, '\r');
        }
        KdPortPutByteEx(NULL, *String);
        String++;
    }

    return STATUS_SUCCESS;
}

/* EOF */
