/* $Id: adapter.c,v 1.7 2003/10/23 09:03:51 vizzini Exp $
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
 *     - the ADAPTER_OBJECT struct is undocumented; please make copious
 *       notes in hal.h if anything is changed or improved since there is
 *       no other documentation for this data structure
 * BUGS:
 *     - it's possible that some of this code is in the wrong place
 *     - there are many unhandled cases
 */
{
  LARGE_INTEGER MaxAddress;
  IO_ALLOCATION_ACTION Retval;

  /* set up the wait context block in case we can't run right away */
  WaitContextBlock->DeviceRoutine = ExecutionRoutine;
  WaitContextBlock->NumberOfMapRegisters = NumberOfMapRegisters;

  /* returns true if queued, else returns false and sets the queue to busy */
  if(KeInsertDeviceQueue(&AdapterObject->DeviceQueue, (PKDEVICE_QUEUE_ENTRY)WaitContextBlock))
    return STATUS_SUCCESS;

  /* 24-bit max address due to 16-bit dma controllers */
  MaxAddress.QuadPart = 0x1000000;

  /* why 64K alignment? */
  /*
   * X86 lacks map registers, so for now, we allocate a contiguous
   * block of physical memory <16MB and copy all DMA buffers into
   * that.  This can be optimized.
   */
  AdapterObject->MapRegisterBase = MmAllocateContiguousAlignedMemory( 
      NumberOfMapRegisters * PAGE_SIZE, MaxAddress, 0x10000 );

  if(!AdapterObject->MapRegisterBase)
    return STATUS_INSUFFICIENT_RESOURCES;

  AdapterObject->AllocatedMapRegisters = NumberOfMapRegisters;

  /* call the client's AdapterControl callback with its map registers and context */
  Retval = ExecutionRoutine(WaitContextBlock->DeviceObject, WaitContextBlock->CurrentIrp, 
      AdapterObject->MapRegisterBase, WaitContextBlock->DeviceContext);

  /* 
   * KeepObject: don't free any resources; the ADAPTER_OBJECT is still in use
   *             and the caller will call IoFreeAdapterChannel later
   *
   * DeallocateObject: Deallocate the map registers and release the ADAPTER_OBJECT
   *             so someone else can use it
   *
   * DeallocateObjectKeepRegisters: release the ADAPTER_OBJECT but hang on to
   *             the map registers.  The client will later call IoFreeMapRegisters.
   *
   * NOTE - IoFreeAdapterChannel runs the queue, so it must be called
   *        unless the adapter object is not to be freed.
   */
  if( Retval == DeallocateObject )
      IoFreeAdapterChannel(AdapterObject);
  else if(Retval == DeallocateObjectKeepRegisters)
    {
      /* don't free the allocated map registers - this is what IoFreeAdapterChannel checks */
      AdapterObject->AllocatedMapRegisters = 0;
      IoFreeAdapterChannel(AdapterObject);
    }

  /*
   * if we don't call IoFreeAdapterChannel, the next device won't get de-queued,
   * which is what we want.
   */

  return STATUS_SUCCESS;
}


BOOLEAN STDCALL
IoFlushAdapterBuffers (PADAPTER_OBJECT	AdapterObject,
		       PMDL		Mdl,
		       PVOID		MapRegisterBase,
		       PVOID		CurrentVa,
		       ULONG		Length,
		       BOOLEAN		WriteToDevice)
/*
 * FUNCTION: flush any data remaining in the dma controller's memory into the host memory
 * ARGUMENTS:
 *     AdapterObject: the adapter object to flush
 *     Mdl: original MDL to flush data into
 *     MapRegisterBase: map register base that was just used by IoMapTransfer, etc
 *     CurrentVa: offset into Mdl to be flushed into, same as was passed to IoMapTransfer
 *     Length: length of the buffer to be flushed into
 *     WriteToDevice: True if it's a write, False if it's a read
 * RETURNS:
 *     TRUE in all cases
 * NOTES:
 *     - This copies data from the map register-backed buffer to the user's target buffer.
 *       Data is not in the user buffer until this is called.
 *     - This is only meaningful on a read operation.  Return immediately for a write.
 */
{
  /* FIXME we don't have ASSERT */
  //ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
  
  /* this can happen if the card supports scatter/gather */
  if(!MapRegisterBase)
    return TRUE;

  if(WriteToDevice)
    return TRUE;

  memcpy( 
        (PVOID)((DWORD)MmGetSystemAddressForMdl( Mdl ) + (DWORD)CurrentVa - (DWORD)MmGetMdlVirtualAddress( Mdl )), 
      MapRegisterBase, Length );

  return TRUE;
}


VOID STDCALL
IoFreeAdapterChannel (PADAPTER_OBJECT	AdapterObject)
/*
 * FUNCTION: frees DMA resources allocated by IoAllocateAdapterChannel
 * ARGUMENTS:
 *     AdapterObject: Adapter object with resources to free
 * NOTES:
 *     - This function releases the DMA adapter and optionally the map registers
 *     - After releasing the adapter, it checks the adapter's queue and runs
 *       each queued device object in series until the queue is empty
 *     - This is the only way the device queue is emptied.
 */
{
  LARGE_INTEGER MaxAddress;
  PWAIT_CONTEXT_BLOCK WaitContextBlock;
  IO_ALLOCATION_ACTION Retval;

  while(1)
    {
      /* To keep map registers, call here with the following set to 0 */
      if(AdapterObject->AllocatedMapRegisters)
        IoFreeMapRegisters(AdapterObject, AdapterObject->MapRegisterBase, AdapterObject->AllocatedMapRegisters);

      if(!(WaitContextBlock = (PWAIT_CONTEXT_BLOCK)KeRemoveDeviceQueue(&AdapterObject->DeviceQueue)))
        break;

      /*
       * the following should really be done elsewhere since this
       * function really can't return an error code.  FIXME.
       */

      /* 24-bit max address due to 16-bit dma controllers */
      MaxAddress.QuadPart = 0x1000000;

      AdapterObject->MapRegisterBase = MmAllocateContiguousAlignedMemory( 
          WaitContextBlock->NumberOfMapRegisters * PAGE_SIZE, MaxAddress, 0x10000 );

      if(!AdapterObject->MapRegisterBase)
        return;

      /* call the adapter control routine */
      Retval = ((PDRIVER_CONTROL)WaitContextBlock->DeviceRoutine)(WaitContextBlock->DeviceObject, WaitContextBlock->CurrentIrp,
          AdapterObject->MapRegisterBase, WaitContextBlock->DeviceContext);

      if(Retval == KeepObject)
        {
          /* we're done until the caller manually calls IoFreeAdapterChannel */
          break;
        }
      else if(Retval == DeallocateObjectKeepRegisters)
        {
          /* hide the map registers so they aren't deallocated next time around */
          AdapterObject->AllocatedMapRegisters = 0;
        }
    }
}


VOID STDCALL
IoFreeMapRegisters (PADAPTER_OBJECT	AdapterObject,
		    PVOID		MapRegisterBase,
		    ULONG		NumberOfMapRegisters)
/*
 * FUNCTION: free map registers reserved by the system for a DMA
 * ARGUMENTS:
 *     AdapterObject: dma adapter to free map registers on
 *     MapRegisterBase: hadle to map registers to free
 *     NumberOfRegisters: number of map registers to be freed
 * NOTES:
 *     - XXX real windows has a funky interdependence between IoFreeMapRegisters 
 *       and IoFreeAdapterChannel 
 * BUGS:
 *     - needs to be improved to use a real map register implementation
 */
{
  if( AdapterObject->AllocatedMapRegisters )
    {
      MmFreeContiguousMemory(AdapterObject->MapRegisterBase);
      AdapterObject->MapRegisterBase = 0;
    }
}


PHYSICAL_ADDRESS  STDCALL
IoMapTransfer (PADAPTER_OBJECT	AdapterObject,
	       PMDL		Mdl,
	       PVOID		MapRegisterBase,
	       PVOID		CurrentVa,
	       PULONG		Length,
	       BOOLEAN		WriteToDevice)
/*
 * FUNCTION: map a dma for transfer and do the dma if it's a slave
 * ARGUMENTS:
 *     AdapterObject: adapter object to do the dma on
 *     Mdl: locked-down user buffer to DMA in to or out of
 *     MapRegisterBase: handle to map registers to use for this dma
 *     CurrentVa: index into Mdl to transfer into/out of
 *     Length: length of transfer in/out
 *     WriteToDevice: TRUE if it's an output dma, FALSE otherwise
 * RETURNS: 
 *     If a busmaster: A logical address that can be used to program a dma controller
 *     Otherwise: nothing meaningful
 * NOTES:
 *     - This function does a copyover to contiguous memory <16MB
 *     - If it's a slave transfer, this function actually performs it.
 * BUGS:
 *     - If the controller supports scatter/gather, the copyover should not happen
 */
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




