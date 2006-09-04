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

VOID STDCALL
HalInitializeProcessor(ULONG ProcessorNumber,
                       PLOADER_PARAMETER_BLOCK LoaderBlock)
{
  DPRINT("HalInitializeProcessor(%lu %p)\n", ProcessorNumber, LoaderBlock);
    /* Set default IDR */
    KeGetPcr()->IDR = 0xFFFFFFFB;
    KeGetPcr()->StallScaleFactor = INITIAL_STALL_COUNT;
}

BOOLEAN STDCALL
HalAllProcessorsStarted (VOID)
{
  DPRINT("HalAllProcessorsStarted()\n");

  return TRUE;
}

BOOLEAN STDCALL 
HalStartNextProcessor(ULONG Unknown1,
		      ULONG ProcessorStack)
{
  DPRINT("HalStartNextProcessor(0x%lx 0x%lx)\n", Unknown1, ProcessorStack);

  return TRUE;
}

/* EOF */
