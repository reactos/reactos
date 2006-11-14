/* $Id$
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
#include <debug.h>

/* GLOBALS *****************************************************************/

PVOID HalpZeroPageMapping = NULL;
HALP_HOOKS HalpHooks;

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
      RtlZeroMemory(&HalpHooks, sizeof(HALP_HOOKS));
      HalpInitPhase0(LoaderBlock);      
    }
  else if (BootPhase == 1)
    {
      /* Initialize the clock interrupt */
      //HalpInitPhase1();

      /* Initialize BUS handlers and DMA */
      HalpInitBusHandlers();
      HalpInitDma();
   }
  else if (BootPhase == 2)
    {
      HalpZeroPageMapping = MmMapIoSpace((LARGE_INTEGER)0LL, PAGE_SIZE, MmNonCached);
    }

  return TRUE;
}

/* EOF */
