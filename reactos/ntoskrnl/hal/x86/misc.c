/* $Id: misc.c,v 1.5 2000/08/12 19:33:20 dwelch Exp $
 *
 * COPYRIGHT:             See COPYING in the top level directory
 * PROJECT:               ReactOS kernel
 * FILE:                  ntoskrnl/hal/x86/misc.c
 * PURPOSE:               Miscellaneous hardware functions
 * PROGRAMMER:            Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>


/* FUNCTIONS ****************************************************************/

VOID STDCALL
HalHandleNMI (ULONG Unused)
{
	UCHAR ucStatus;
	
	ucStatus = READ_PORT_UCHAR((PUCHAR) 0x61);
	
	HalDisplayString ("\n*** Hardware Malfunction\n\n");
	HalDisplayString ("Call your hardware vendor for support\n\n");
	
	if (ucStatus & 0x80)
		HalDisplayString ("NMI: Parity Check / Memory Parity Error\n");
		
	if (ucStatus & 0x40)
		HalDisplayString ("NMI: Channel Check / IOCHK\n");

	HalDisplayString ("\n*** The system has halted ***\n");
	KeEnterKernelDebugger ();
}

VOID STDCALL
HalProcessorIdle (VOID)
{
#if 1
	__asm__("sti\n\t" \
	        "hlt\n\t");
#else
   
#endif
}

ULONG FASTCALL
HalSystemVectorDispatchEntry (
	ULONG	Unknown1,
	ULONG	Unknown2,
	ULONG	Unknown3
	)
{
	return 0;
}

VOID STDCALL
KeFlushWriteBuffer (
	VOID
	)
{
	return;
}

VOID STDCALL
HalReportResourceUsage (
	VOID
	)
{
	/*
	 * FIXME: Report all resources used by hal.
	 *        Calls IoReportHalResourceUsage()
	 */

	/*
	 * Initialize PCI, IsaPnP and other busses.
	 */

#if 0
      /*
       * Probe for a BIOS32 extension
       */
      Hal_bios32_probe();
   
      /*
       * Probe for buses attached to the computer
       */

      HalPciProbe();
#endif

	return;
}

/* EOF */
