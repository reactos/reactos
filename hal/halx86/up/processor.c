/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/up/processor.c
 * PURPOSE:         HAL Processor Routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

KAFFINITY HalpActiveProcessors;
KAFFINITY HalpDefaultInterruptAffinity;

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
    /* Do nothing */
    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
HalStartNextProcessor(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                      IN PKPROCESSOR_STATE ProcessorState)
{
    /* Ready to start */
    return FALSE;
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

/*
 * @implemented
 */
VOID
NTAPI
HalRequestIpi(KAFFINITY TargetProcessors)
{
    /* Not implemented on UP */
    __debugbreak();
}

/* EOF */
