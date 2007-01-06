/* $Id: sysbus.c 23907 2006-09-04 05:52:23Z arty $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/sysbus.c
 * PURPOSE:         System bus handler functions
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  09/04/2000 Created
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>


/* FUNCTIONS ****************************************************************/

ULONG STDCALL
HalpGetSystemInterruptVector(PVOID BusHandler,
			     ULONG BusNumber,
			     ULONG BusInterruptLevel,
			     ULONG BusInterruptVector,
			     PKIRQL Irql,
			     PKAFFINITY Affinity)
{
  ULONG Vector = IRQ2VECTOR(BusInterruptVector);
  *Irql = VECTOR2IRQL(Vector);
  *Affinity = 0xFFFFFFFF;
  return Vector;
}


BOOLEAN STDCALL
HalpTranslateSystemBusAddress(PBUS_HANDLER BusHandler,
			      ULONG BusNumber,
			      PHYSICAL_ADDRESS BusAddress,
			      PULONG AddressSpace,
			      PPHYSICAL_ADDRESS TranslatedAddress)
{
   ULONG BaseAddress = 0;

   if (*AddressSpace == 0)
     {
	/* memory space */

     }
   else if (*AddressSpace == 1)
     {
	/* io space */

     }
   else
     {
	/* other */
	return FALSE;
     }

   TranslatedAddress->QuadPart = BusAddress.QuadPart + BaseAddress;

   return TRUE;
}

/* EOF */
