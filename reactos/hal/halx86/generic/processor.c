/* $Id$
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

/*
 * @implemented
 */
VOID
NTAPI
HalInitializeProcessor(IN ULONG ProcessorNumber,
                       IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Set default IDR */
    KeGetPcr()->IDR = 0xFFFFFFFB;
    KeGetPcr()->StallScaleFactor = INITIAL_STALL_COUNT;
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
    _enable();
    Ki386HaltProcessor();
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
