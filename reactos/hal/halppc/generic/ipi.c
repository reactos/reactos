/* $Id: ipi.c 23907 2006-09-04 05:52:23Z arty $
 *
 * COPYRIGHT:             See COPYING in the top level directory
 * PROJECT:               ReactOS kernel
 * FILE:                  hal/halx86/generic/ipi.c
 * PURPOSE:               Miscellaneous hardware functions
 * PROGRAMMER:            Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

VOID STDCALL
HalRequestIpi(ULONG ProcessorNo)
{
  DPRINT("HalRequestIpi(ProcessorNo %lu)\n", ProcessorNo);
}

/* EOF */
