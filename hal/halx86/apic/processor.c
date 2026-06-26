/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     HAL Processor Routines
 * COPYRIGHT:   Copyright 2010 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

KAFFINITY HalpActiveProcessors;
KAFFINITY HalpDefaultInterruptAffinity;
ULONG HalpStartedProcessorCount = 1;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
HaliHaltSystem(VOID)
{
    /* Disable interrupts and halt the CPU */
    _disable();
    __halt();
}

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
HalAllProcessorsStarted(VOID)
{
    if (!(HalpBuildType & PRCB_BUILD_UNIPROCESSOR) &&
        (KeNumberProcessors != HalpStartedProcessorCount))
    {
        DPRINT1("Only %u of %lu started processors reached the kernel\n",
                KeNumberProcessors,
                HalpStartedProcessorCount);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
VOID
NTAPI
HalProcessorIdle(VOID)
{
    /* Enable interrupts and halt the processor */
    _enable();
    __halt();
}

/* EOF */
