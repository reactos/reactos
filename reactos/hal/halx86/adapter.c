/* $Id: adapter.c,v 1.6 2003/10/20 06:03:28 vizzini Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            hal/x86/adapter.c (from ntoskrnl/io/adapter.c)
 * PURPOSE:         DMA handling
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Vizzini (vizzini@plasmic.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  18-Oct-2003 Vizzini DMA support modifications
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/iotypes.h>
#include <internal/debug.h>
#include <hal.h>

/* FUNCTIONS *****************************************************************/

/* NOTE: IoAllocateAdapterChannel in NTOSKRNL.EXE */

NTSTATUS STDCALL
HalAllocateAdapterChannel(PADAPTER_OBJECT AdapterObject,
			  PWAIT_CONTEXT_BLOCK WaitContextBlock,
			  ULONG NumberOfMapRegisters,
			  PDRIVER_CONTROL ExecutionRoutine)
/*
 * FUNCTION: Sets up an ADAPTER_OBJECT with map registers
 * ARGUMENTS:
 *     - AdapterObject: pointer to an ADAPTER_OBJECT to set up 
 *     - WaitContextBlock: Context block to be used with ExecutionRoutine
 *     - NumberOfMapRegisters: number of map registers requested 
 *     - ExecutionRoutine: callback to call when map registers are allocated
 * RETURNS:
 *     STATUS_INSUFFICIENT_RESOURCES if map registers cannot be allocated
 *     STATUS_SUCCESS in all other cases, including if the callbacak had
 *                    to be queued for later delivery
 * NOTES:
 *     - Map registers don't exist on X86 so we can just call the callback
 *       with a map register base of 0
 *     - the ADAPTER_OBJECT struct is undocumented; please make copious
 *       notes here if anything is changed or improved since there is
 *       no other documentation for this routine or its data structures
 *     - The original implementation of this function allocated a contiguous
 *       physical buffer the size of NumberOfMapRegisters * PAGE_SIZE, which
 *       is unnecessary and very expensive (contiguous memory is rare).  It
 *       also leaked in some circumstances (drivers allocate and sometimes
 *       don't free map registers)
 * BUGS:
 *     - This routine should check whether or not map registers are needed
 *       (rather than assuming they're not) and allocate them on platforms
 *       that support them.
 */
{
#if 0
  KIRQL OldIrql;
  PVOID Buffer;
  int ret;
  LARGE_INTEGER MaxAddress;

  MaxAddress.QuadPart = 0x1000000;

  /* why 64K alignment? */
  Buffer = MmAllocateContiguousAlignedMemory( NumberOfMapRegisters * PAGE_SIZE,
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
			    WaitContextBlock->DriverContext );
    KeAcquireSpinLock( &AdapterObject->SpinLock, &OldIrql );
    if( ret == DeallocateObject )
      {
	MmFreeContiguousMemory( Buffer );
	AdapterObject->Inuse = FALSE;
      }
    else AdapterObject->Buffer = Buffer;
  }
  KeReleaseSpinLock( &AdapterObject->SpinLock, OldIrql );
#endif

  AdapterObject->MapRegisterBase = 0;
  AdapterObject->AllocatedMapRegisters = 0;

  IO_ALLOCATION_ACTION Retval = ExecutionRoutine(WaitContextBlock->DeviceObject,
      WaitContextBlock->CurrentIrp, 0, WaitContextBlock->DeviceContext);

  if(Retval == DeallocateObject)
    IoFreeAdapterChannel(AdapterObject);
  else if(Retval == DeallocateObjectKeepRegisters)
    AdapterObject->AllocatedMapRegisters = 0;

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
    memcpy( (PVOID)((DWORD)MmGetSystemAddressForMdl( Mdl ) + (DWORD)CurrentVa - (DWORD)MmGetMdlVirtualAddress( Mdl )), MapRegisterBase, Length );
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
      KEBUGCHECK(0);
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
  PHYSICAL_ADDRESS Address;
  // program up the dma controller, and return
  // if it is a write to the device, copy the caller buffer to the low buffer
  if( WriteToDevice )
    memcpy( MapRegisterBase,
	    MmGetSystemAddressForMdl( Mdl ) + ( (DWORD)CurrentVa - (DWORD)MmGetMdlVirtualAddress( Mdl ) ),
	    *Length );
  Address = MmGetPhysicalAddress( MapRegisterBase );
  // port 0xA is the dma mask register, or a 0x10 on to the channel number to mask it
  WRITE_PORT_UCHAR( (PVOID)0x0A, AdapterObject->Channel | 0x10 );
  // write zero to the reset register
  WRITE_PORT_UCHAR( (PVOID)0x0C, 0 );
  // mode register, or channel with 0x4 for write memory, 0x8 for read memory, 0x10 for non auto initialize
  WRITE_PORT_UCHAR( (PVOID)0x0B, AdapterObject->Channel | ( WriteToDevice ? 0x8 : 0x4 ) );
  // set the 64k page register for the channel
  WRITE_PORT_UCHAR( AdapterObject->PagePort, (UCHAR)(((ULONG)Address.QuadPart)>>16) );
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




