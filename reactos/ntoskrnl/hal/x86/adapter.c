/* $Id: adapter.c,v 1.2 2000/07/19 14:18:18 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            hal/x86/adapter.c (from ntoskrnl/io/adapter.c)
 * PURPOSE:         DMA handling
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/* NOTE: IoAllocateAdapterChannel in NTOSKRNL.EXE */

BOOLEAN STDCALL
IoFlushAdapterBuffers (PADAPTER_OBJECT	AdapterObject,
		       PMDL		Mdl,
		       PVOID		MapRegisterBase,
		       PVOID		CurrentVa,
		       ULONG		Length,
		       BOOLEAN		WriteToDevice)
{
   UNIMPLEMENTED;
}


VOID STDCALL
IoFreeAdapterChannel (PADAPTER_OBJECT	AdapterObject)
{
   UNIMPLEMENTED;
}


VOID STDCALL
IoFreeMapRegisters (PADAPTER_OBJECT	AdapterObject,
		    PVOID		MapRegisterBase,
		    ULONG		NumberOfMapRegisters)
{
   UNIMPLEMENTED;
}


PHYSICAL_ADDRESS  STDCALL
IoMapTransfer (PADAPTER_OBJECT	AdapterObject,
	       PMDL		Mdl,
	       PVOID		MapRegisterBase,
	       PVOID		CurrentVa,
	       PULONG		Length,
	       BOOLEAN		WriteToDevice)
{
   UNIMPLEMENTED;
}


/* EOF */
