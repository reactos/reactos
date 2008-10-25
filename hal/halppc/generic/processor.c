/* $Id: processor.c 23907 2006-09-04 05:52:23Z arty $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            hal/halx86/generic/processor.c
 * PURPOSE:         Intel MultiProcessor specification support
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:           Parts adapted from linux SMP code
 * UPDATE HISTORY:
 *     22/05/1998  DW   Created
 *     12/04/2001  CSH  Added MultiProcessor specification support
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

#define INITIAL_STALL_COUNT 0x10000

VOID STDCALL
HalInitializeProcessor(ULONG ProcessorNumber,
                       PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    DPRINT("HalInitializeProcessor(%lu %p)\n", ProcessorNumber, LoaderBlock);
    KeGetPcr()->StallScaleFactor = INITIAL_STALL_COUNT;
}

BOOLEAN STDCALL
HalAllProcessorsStarted (VOID)
{
  DPRINT("HalAllProcessorsStarted()\n");

  return TRUE;
}

NTHALAPI
BOOLEAN
NTAPI
HalStartNextProcessor(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN PKPROCESSOR_STATE ProcessorState
    )
{
  DPRINT("HalStartNextProcessor(0x%lx 0x%lx)\n", LoaderBlock, ProcessorState);

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
