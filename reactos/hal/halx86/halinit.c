/* $Id: halinit.c,v 1.2 2002/09/07 15:12:10 chorns Exp $
 *
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          ntoskrnl/hal/x86/halinit.c
 * PURPOSE:       Initalize the x86 hal
 * PROGRAMMER:    David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              11/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include <hal.h>

#define NDEBUG
#include <internal/debug.h>


#ifdef MP
#include <mps.h>
#endif /* MP */

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

NTSTATUS
STDCALL
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath)
{
	return STATUS_SUCCESS;
}

BOOLEAN STDCALL
HalInitSystem (ULONG BootPhase,
               PLOADER_PARAMETER_BLOCK LoaderBlock)
{
  if (BootPhase == 0)
    {
      HalInitializeDisplay (LoaderBlock);
      
#ifdef MP
      
      HalpInitMPS();
      
#else

      HalpInitPICs();

      /* Setup busy waiting */
      HalpCalibrateStallExecution();

#endif /* MP */

    }
  else if (BootPhase == 1)
    {
      HalpInitBusHandlers ();
  	  HalpCalibrateStallExecution ();

      /* Enumerate the devices on the motherboard */
      HalpStartEnumerator();
   }

  return TRUE;
}

/* EOF */
