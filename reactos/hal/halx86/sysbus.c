/* $Id: sysbus.c,v 1.5 2003/04/06 10:45:15 chorns Exp $
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

#include <roscfg.h>
#include <ddk/ntddk.h>
#include <bus.h>
#ifdef MP
#include <mps.h>
#endif


/* FUNCTIONS ****************************************************************/

ULONG STDCALL
HalpGetSystemInterruptVector(PVOID BusHandler,
			     ULONG BusNumber,
			     ULONG BusInterruptLevel,
			     ULONG BusInterruptVector,
			     PKIRQL Irql,
			     PKAFFINITY Affinity)
{
#ifdef MP
  *Irql = PROFILE_LEVEL - BusInterruptVector;
  *Affinity = 0xFFFFFFFF;
  return IRQ2VECTOR(BusInterruptVector);
#else
  *Irql = PROFILE_LEVEL - BusInterruptVector;
  *Affinity = 0xFFFFFFFF;
  return BusInterruptVector;
#endif
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
