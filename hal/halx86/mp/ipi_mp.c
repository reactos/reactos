/* $Id$
 *
 * COPYRIGHT:             See COPYING in the top level directory
 * PROJECT:               ReactOS kernel
 * FILE:                  hal/halx86/mp/ipi_mp.c
 * PURPOSE:               IPI functions for MP
 * PROGRAMMER:            Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

VOID STDCALL
HalRequestIpi(KAFFINITY TargetProcessors)
{
  /* FIXME: SMP HAL is...very broken */
  DPRINT("HalRequestIpi(TargetProcessors %d)\n", TargetProcessors);
  APICSendIPI(1 << TargetProcessors,
	      IPI_VECTOR|APIC_ICR0_LEVEL_DEASSERT|APIC_ICR0_DESTM);
}

/* EOF */
