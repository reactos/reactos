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

LONG HalpActiveProcessors;
KAFFINITY HalpDefaultInterruptAffinity;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
HalInitializeProcessor(IN ULONG ProcessorNumber,
                       IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Set default IDR and stall count */
    KeGetPcr()->IDR = 0xFFFFFFFB;
    KeGetPcr()->StallScaleFactor = INITIAL_STALL_COUNT;

    /* Update the interrupt affinity and processor mask */
    InterlockedBitTestAndSet(&HalpActiveProcessors, ProcessorNumber);
    InterlockedBitTestAndSet((PLONG)&HalpDefaultInterruptAffinity,
                             ProcessorNumber);

    /* Register routines for KDCOM */
    HalpRegisterKdSupportFunctions();
}

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
    Ke386HaltProcessor();
}

/*
 * @implemented
 */
VOID
NTAPI
HalRequestIpi(ULONG Reserved)
{
    /* Not implemented on NT */
    __debugbreak();
}

/* EOF */
