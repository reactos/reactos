/* $Id: misc.c,v 1.8 2004/11/28 01:30:01 hbirr Exp $
 *
 * COPYRIGHT:             See COPYING in the top level directory
 * PROJECT:               ReactOS kernel
 * FILE:                  ntoskrnl/hal/x86/misc.c
 * PURPOSE:               Miscellaneous hardware functions
 * PROGRAMMER:            Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <roscfg.h>
#include <ddk/ntddk.h>
#include <hal.h>

#ifdef MP
#include <apic.h>
#endif

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

#ifdef MP

VOID
HaliReconfigurePciInterrupts(VOID);

#endif

VOID STDCALL
HalHandleNMI(ULONG Unused)
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
HalProcessorIdle(VOID)
{
#if 1
  Ki386EnableInterrupts();
  Ki386HaltProcessor();
#else

#endif
}

VOID STDCALL
HalRequestIpi(ULONG ProcessorNo)
{
  DPRINT("HalRequestIpi(ProcessorNo %d)\n", ProcessorNo);
#ifdef MP 
  APICSendIPI(1 << ProcessorNo,
	      IPI_VECTOR|APIC_ICR0_LEVEL_DEASSERT|APIC_ICR0_DESTM);
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
KeFlushWriteBuffer(VOID)
{
  return;
}


VOID STDCALL
HalReportResourceUsage(VOID)
{
  /*
   * FIXME: Report all resources used by hal.
   *        Calls IoReportHalResourceUsage()
   */

  /* Initialize PCI bus. */
  HalpInitPciBus ();
#ifdef MP

  HaliReconfigurePciInterrupts();
#endif

}

/* EOF */
