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
		       PVOID /*PLOADER_PARAMETER_BLOCK*/ LoaderBlock)
{
  DPRINT("HalInitializeProcessor(%x %x)\n", ProcessorNumber, LoaderBlock);
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
  DPRINT("HalStartNextProcessor(%x %x)\n", Unknown1, ProcessorStack);

  return TRUE;
}

/* EOF */
