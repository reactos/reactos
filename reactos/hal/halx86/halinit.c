/* $Id: halinit.c,v 1.7 2004/03/18 19:58:35 dwelch Exp $
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

#include <ddk/ntddk.h>
#include <roscfg.h>
#include <hal.h>

#ifdef MP
#include <mps.h>
#endif /* MP */

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

PVOID HalpZeroPageMapping = NULL;

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
      /* Initialize display and make the screen black */
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
      HalpInitBusHandlers();
      HalpCalibrateStallExecution();

      /* Enumerate the devices on the motherboard */
      HalpStartEnumerator();
   }
  else if (BootPhase == 2)
    {
      /* Go to blue screen */
      HalClearDisplay (0x17); /* grey on blue */
      
      HalpZeroPageMapping = MmMapIoSpace((LARGE_INTEGER)0LL, PAGE_SIZE, FALSE);
    }

  return TRUE;
}

/* EOF */
