/* $Id: adapter.c,v 1.5 2001/03/31 16:46:59 jfilby Exp $
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
#include <ddk/iotypes.h>
#include <internal/debug.h>
#include <internal/hal/hal.h>

/* FUNCTIONS *****************************************************************/

/* NOTE: IoAllocateAdapterChannel in NTOSKRNL.EXE */

NTSTATUS STDCALL
HalAllocateAdapterChannel(PADAPTER_OBJECT AdapterObject,
			  PDEVICE_OBJECT DeviceObject,
			  ULONG NumberOfMapRegisters,
			  PDRIVER_CONTROL ExecutionRoutine,
			  PVOID Context )
{
  KIRQL OldIrql;
  PVOID Buffer;
  int ret;
  LARGE_INTEGER MaxAddress;

  MaxAddress.QuadPart = 0x1000000;
  Buffer = MmAllocateContiguousAlignedMemory( NumberOfMapRegisters * PAGESIZE,
					      MaxAddress,
					      0x10000 );
  if( !Buffer )
    return STATUS_INSUFFICIENT_RESOURCES;
  KeAcquireSpinLock( &AdapterObject->SpinLock, &OldIrql );
  if( AdapterObject->Inuse )
    {
      // someone is already using it, we need to wait
      // create a wait block, and add it to the chain
      UNIMPLEMENTED;
    }
  else {
    AdapterObject->Inuse = TRUE;
    KeReleaseSpinLock( &AdapterObject->SpinLock, OldIrql );
    ret = ExecutionRoutine( DeviceObject,
			    NULL,
			    Buffer,
			    Context );
    KeAcquireSpinLock( &AdapterObject->SpinLock, &OldIrql );
    if( ret == DeallocateObject )
      {
	MmFreeContiguousMemory( Buffer );
	AdapterObject->Inuse = FALSE;
      }
    else AdapterObject->Buffer = Buffer;
  }
  KeReleaseSpinLock( &AdapterObject->SpinLock, OldIrql );
  return STATUS_SUCCESS;
}


BOOLEAN STDCALL
IoFlushAdapterBuffers (PADAPTER_OBJECT	AdapterObject,
		       PMDL		Mdl,
		       PVOID		MapRegisterBase,
		       PVOID		CurrentVa,
		       ULONG		Length,
		       BOOLEAN		WriteToDevice)
{
  // if this was a read from device, copy data back to caller buffer, otherwise, do nothing
  if( !WriteToDevice )
    memcpy( MmGetSystemAddressForMdl( Mdl ), MapRegisterBase, Length );
  return TRUE;
}


VOID STDCALL
IoFreeAdapterChannel (PADAPTER_OBJECT	AdapterObject)
{
  KIRQL OldIrql;
  
  KeAcquireSpinLock( &AdapterObject->SpinLock, &OldIrql );
  if( AdapterObject->Inuse == FALSE )
    {
      DbgPrint( "Attempting to IoFreeAdapterChannel on a channel not in use\n" );
      KeBugCheck(0);
    }
  AdapterObject->Inuse = FALSE;
  if( AdapterObject->Buffer )
    {
      MmFreeContiguousMemory( AdapterObject->Buffer );
      AdapterObject->Buffer = 0;
    }
  KeReleaseSpinLock( &AdapterObject->SpinLock, OldIrql );
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
  LARGE_INTEGER Address;
  // program up the dma controller, and return
  // if it is a write to the device, copy the caller buffer to the low buffer
  if( WriteToDevice )
    memcpy( MapRegisterBase,
	    MmGetSystemAddressForMdl( Mdl ) + ( (DWORD)CurrentVa - (DWORD)MmGetMdlVirtualAddress( Mdl ) ),
	    *Length );
  // port 0xA is the dma mask register, or a 0x10 on to the channel number to mask it
  WRITE_PORT_UCHAR( (PVOID)0x0A, AdapterObject->Channel | 0x10 );
  // write zero to the reset register
  WRITE_PORT_UCHAR( (PVOID)0x0C, 0 );
  // mode register, or channel with 0x4 for write memory, 0x8 for read memory, 0x10 for non auto initialize
  WRITE_PORT_UCHAR( (PVOID)0x0B, AdapterObject->Channel | ( WriteToDevice ? 0x8 : 0x4 ) );
  // set the 64k page register for the channel
  WRITE_PORT_UCHAR( AdapterObject->PagePort, (UCHAR)(((ULONG)MapRegisterBase)>>16) );
  // low, then high address byte, which is always 0 for us, because we have a 64k alligned address
  WRITE_PORT_UCHAR( AdapterObject->OffsetPort, 0 );
  WRITE_PORT_UCHAR( AdapterObject->OffsetPort, 0 );
  // count is 1 less than length, low then high
  WRITE_PORT_UCHAR( AdapterObject->CountPort, (UCHAR)(*Length - 1) );
  WRITE_PORT_UCHAR( AdapterObject->CountPort, (UCHAR)((*Length - 1)>>8) );
  // unmask the channel to let it rip
  WRITE_PORT_UCHAR( (PVOID)0x0A, AdapterObject->Channel );
  Address.QuadPart = (DWORD)MapRegisterBase;
  return Address;
}


/* EOF */




