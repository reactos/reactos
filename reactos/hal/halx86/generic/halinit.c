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

PVOID HalpZeroPageMapping = NULL;
HALP_HOOKS HalpHooks;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
HalInitSystem(IN ULONG BootPhase,
              IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PHYSICAL_ADDRESS Null = {{0}};

    if (BootPhase == 0)
    {
        RtlZeroMemory(&HalpHooks, sizeof(HALP_HOOKS));
        HalpInitPhase0(LoaderBlock);
    }
    else if (BootPhase == 1)
    {
        /* Initialize the clock interrupt */
        //HalpInitPhase1();

        /* Initialize BUS handlers and DMA */
        HalpInitBusHandlers();
        HalpInitDma();
    }
    else if (BootPhase == 2)
    {
        HalpZeroPageMapping = MmMapIoSpace(Null, PAGE_SIZE, MmNonCached);
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
    HalpInitPciBus();

    /* FIXME: Report HAL Usage to kernel */
}


/* EOF */
