/* $Id: sysbus.c,v 1.6 2003/12/28 22:38:09 fireball Exp $
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
  *Irql = (KIRQL)(PROFILE_LEVEL - BusInterruptVector);
  *Affinity = 0xFFFFFFFF;
  return IRQ2VECTOR(BusInterruptVector);
#else
  *Irql = (KIRQL)(PROFILE_LEVEL - BusInterruptVector);
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
