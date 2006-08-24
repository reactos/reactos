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
VOID NTAPI HalpClockInterrupt(VOID);

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
      HalpInitPhase0((PROS_LOADER_PARAMETER_BLOCK)LoaderBlock);      
    }
  else if (BootPhase == 1)
    {
        /* Enable the clock interrupt */
        ((PKIPCR)KeGetPcr())->IDT[IRQ2VECTOR(0)].ExtendedOffset =
            (USHORT)(((ULONG_PTR)HalpClockInterrupt >> 16) & 0xFFFF);
        ((PKIPCR)KeGetPcr())->IDT[IRQ2VECTOR(0)].Offset =
            (USHORT)HalpClockInterrupt;
        HalEnableSystemInterrupt(IRQ2VECTOR(0), CLOCK2_LEVEL, Latched);

      /* Initialize display and make the screen black */
      HalInitializeDisplay ((PROS_LOADER_PARAMETER_BLOCK)LoaderBlock);
      HalpInitBusHandlers();
      HalpInitDma();

      /* Enumerate the devices on the motherboard */
      HalpStartEnumerator();
   }
  else if (BootPhase == 2)
    {
      PHYSICAL_ADDRESS Null = {{0}};

      /* Go to blue screen */
      HalClearDisplay (0x17); /* grey on blue */
      
      HalpZeroPageMapping = MmMapIoSpace(Null, PAGE_SIZE, MmNonCached);
    }

  return TRUE;
}

/* EOF */
