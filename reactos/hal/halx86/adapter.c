/* $Id: adapter.c,v 1.12 2004/10/22 20:08:21 ekohl Exp $
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
HalAllocateAdapterChannel(
  PADAPTER_OBJECT AdapterObject,
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
  LARGE_INTEGER MinAddress;
  LARGE_INTEGER MaxAddress;
  LARGE_INTEGER BoundryAddressMultiple;
  IO_ALLOCATION_ACTION Retval;
  
  ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
  
  /*
  FIXME: return STATUS_INSUFFICIENT_RESOURCES if the NumberOfMapRegisters 
  requested is larger than the value returned by IoGetDmaAdapter. 
  */

  /* set up the wait context block in case we can't run right away */
  WaitContextBlock->DeviceRoutine = ExecutionRoutine;
  WaitContextBlock->NumberOfMapRegisters = NumberOfMapRegisters;

  /* returns true if queued, else returns false and sets the queue to busy */
  if(KeInsertDeviceQueue(&AdapterObject->DeviceQueue, &WaitContextBlock->WaitQueueEntry))
    return STATUS_SUCCESS;

  /* 24-bit max address due to 16-bit dma controllers */
  MinAddress.QuadPart = 0x0000000;
  MaxAddress.QuadPart = 0x1000000;
  BoundryAddressMultiple.QuadPart = 0;

  /* why 64K alignment? */
  /*
   * X86 lacks map registers, so for now, we allocate a contiguous
   * block of physical memory <16MB and copy all DMA buffers into
   * that.  This can be optimized.
   *
   * FIXME: We propably shouldn't allocate the memory here for common
   * buffer transfers. See a comment in IoMapTransfer about common buffer
   * support.
   */
  AdapterObject->MapRegisterBase = MmAllocateContiguousAlignedMemory( 
      NumberOfMapRegisters * PAGE_SIZE,
      MinAddress,
      MaxAddress,
      BoundryAddressMultiple,
      MmCached,
      0x10000 );

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
IoFlushAdapterBuffers (
  PADAPTER_OBJECT	AdapterObject,
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
  ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
  
  /* this can happen if the card supports scatter/gather */
  if(!MapRegisterBase)
    return TRUE;

  /* mask out (disable) the dma channel */
  if (AdapterObject->Channel < 4)
    WRITE_PORT_UCHAR( (PVOID)0x0A, (UCHAR)(AdapterObject->Channel | 0x4) );
  else
    WRITE_PORT_UCHAR( (PVOID)0xD4, (UCHAR)((AdapterObject->Channel - 4) | 0x4) );

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
  LARGE_INTEGER MinAddress;
  LARGE_INTEGER BoundryAddressMultiple;
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
      MinAddress.QuadPart = 0x0000000;
      MaxAddress.QuadPart = 0x1000000;
      BoundryAddressMultiple.QuadPart = 0;

      AdapterObject->MapRegisterBase = MmAllocateContiguousAlignedMemory( 
          WaitContextBlock->NumberOfMapRegisters * PAGE_SIZE,
          MinAddress,
          MaxAddress,
          BoundryAddressMultiple,
          MmCached,
          0x10000 );

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
IoFreeMapRegisters (
  IN PADAPTER_OBJECT	AdapterObject,
  IN PVOID		MapRegisterBase,
  IN ULONG		NumberOfMapRegisters)
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
IoMapTransfer (
  IN PADAPTER_OBJECT	AdapterObject,
  IN PMDL		Mdl,
  IN PVOID		MapRegisterBase,
  IN PVOID		CurrentVa,
  IN OUT PULONG		Length,
  IN BOOLEAN		WriteToDevice)
/*
 * FUNCTION: map a dma for transfer and do the dma if it's a slave
 * ARGUMENTS:
 *     AdapterObject: adapter object to do the dma on. busmaster may pass NULL.
 *     Mdl: locked-down user buffer to DMA in to or out of
 *     MapRegisterBase: handle to map registers to use for this dma. allways NULL
 *      when doing s/g.
 *     CurrentVa: index into Mdl to transfer into/out of
 *     Length: length of transfer in/out. Only modified on out when doing s/g.
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
  PVOID MaskReg, ClearReg, ModeReg;
  UCHAR ModeMask, LengthShift;
  KIRQL OldIrql;

#if defined(__GNUC__)
  Address.QuadPart = 0ULL;
#else
  Address.QuadPart = 0;
#endif

  /* Isa System (slave) DMA? */
  if (AdapterObject && AdapterObject->InterfaceType == Isa && !AdapterObject->Master)
  {
#if 0
    /* channel 0 is reserved for DRAM refresh */
    ASSERT(AdapterObject->Channel != 0);
    /* channel 4 is reserved for cascade */
    ASSERT(AdapterObject->Channel != 4);
#endif

    KeAcquireSpinLock(&AdapterObject->SpinLock, &OldIrql);

    /*
     * FIXME: Handle case when doing common-buffer System DMA. In this case,
     * the buffer described by MDL is already phys. contiguous and below
     * 16 mega. Driver makes a one-shot call to IoMapTransfer during init.
     * to program controller with the common-buffer.
     *
     * UPDATE: Common buffer support is in place, but it's not done in a
     * clean way. We use the buffer passed by the MDL in case that the
     * adapter object is marked as auto initialize. I'm not sure if this
     * is correct and if not, how to do it properly. Note that it's also
     * possible to allocate the common buffer with different adapter object
     * and IoMapTransfer must still work in this case. Eventually this should
     * be cleaned up somehow or at least this comment modified to reflect
     * the reality.
     * -- Filip Navara, 19/07/2004     
     */

    /* if it is a write to the device, copy the caller buffer to the low buffer */
    if( WriteToDevice && !AdapterObject->AutoInitialize )
    {
      memcpy(MapRegisterBase,
             (char*)MmGetSystemAddressForMdl(Mdl) + ((ULONG)CurrentVa - (ULONG)MmGetMdlVirtualAddress(Mdl)),
	           *Length );
    }
             
    // 16-bit DMA
    if( AdapterObject->Channel >= 4 )
    {
      MaskReg = (PVOID)0xD4; ClearReg = (PVOID)0xD8; ModeReg = (PVOID)0xD6;
      LengthShift = 1;
    }
    else
    {
      MaskReg = (PVOID)0x0A; ClearReg = (PVOID)0x0C; ModeReg = (PVOID)0x0B;
      LengthShift = 0;
    }

    // calculate the mask we will later set to the mode register
    ModeMask = (AdapterObject->Channel & 3) | ( WriteToDevice ? 0x8 : 0x4 );
    // FIXME: if not demand mode, which mode to use? 0x40 for single mode
    if (!AdapterObject->DemandMode)
      ModeMask |= 0x40;
    if (AdapterObject->AutoInitialize)
      ModeMask |= 0x10;

    // program up the dma controller, and return
    if (!AdapterObject->AutoInitialize)
      Address = MmGetPhysicalAddress( MapRegisterBase );
    else
      Address = MmGetPhysicalAddress( CurrentVa );
    // disable and select the channel number
    WRITE_PORT_UCHAR( MaskReg, (UCHAR)((AdapterObject->Channel & 3) | 0x4) );
    // write zero to the reset register
    WRITE_PORT_UCHAR( ClearReg, 0 );
    // mode register, or channel with 0x4 for write memory, 0x8 for read memory, 0x10 for auto initialize
    WRITE_PORT_UCHAR( ModeReg, ModeMask);
    // set the 64k page register for the channel
    WRITE_PORT_UCHAR( AdapterObject->PagePort, (UCHAR)(Address.u.LowPart >> 16) );
    // low, then high address byte, which is always 0 for us, because we have a 64k alligned address
    WRITE_PORT_UCHAR( AdapterObject->OffsetPort, 0 );
    WRITE_PORT_UCHAR( AdapterObject->OffsetPort, 0 );
    // count is 1 less than length, low then high
    WRITE_PORT_UCHAR( AdapterObject->CountPort, (UCHAR)((*Length >> LengthShift) - 1) );
    WRITE_PORT_UCHAR( AdapterObject->CountPort, (UCHAR)(((*Length >> LengthShift) - 1)>>8) );
    // unmask the channel to let it rip
    WRITE_PORT_UCHAR( MaskReg, AdapterObject->Channel & 3 );

    KeReleaseSpinLock(&AdapterObject->SpinLock, OldIrql);

    /* 
    NOTE: Return value should be ignored when doing System DMA.
    Maybe return some more obvious invalid address here (thou returning 
    MapRegisterBase is also wrong;-)to catch invalid use?
    */
    Address.QuadPart = (ULONG)MapRegisterBase;
    return Address;
  }
  

  /* 
  Busmaster with s/g support?   
  NOTE: old docs allowed busmasters to pass a NULL Adapter. In this case, MapRegisterBase 
  being NULL is used to detect a s/g busmaster.
  */
  if ((!AdapterObject && !MapRegisterBase) ||
      (AdapterObject && AdapterObject->Master && AdapterObject->ScatterGather))
  {
    /* 
    Just return the passed VA's corresponding phys. address. 
    Update length to the number of phys. contiguous bytes found.
    */
  
    PULONG MdlPages;
    ULONG MdlPageIndex, PhysContiguousLen;
    ULONG PhysAddress;
    
    MdlPages = (PULONG)(Mdl + 1);
    
    /* Get VA's corresponding mdl phys. page index */
    MdlPageIndex = ((ULONG)CurrentVa - (ULONG)Mdl->StartVa) / PAGE_SIZE;
   
    /* Get phys. page containing the VA */
    PhysAddress = MdlPages[MdlPageIndex];
    
    PhysContiguousLen = PAGE_SIZE - BYTE_OFFSET(CurrentVa);
    
    /* VA to map may span several contiguous phys. pages (unlikely) */
    while (PhysContiguousLen < *Length &&
           MdlPages[MdlPageIndex++] + PAGE_SIZE == MdlPages[MdlPageIndex])
    {
      /* 
      Note that allways adding PAGE_SIZE may make PhysContiguousLen greater
      than Length if buffer doesn't end on page boundary. Take this
      into consideration below. 
      */
      PhysContiguousLen += PAGE_SIZE; 
    }
    
    if (PhysContiguousLen < *Length)
    {
      *Length = PhysContiguousLen;
    }
    
    //add offset to phys. page address
    Address.QuadPart = PhysAddress + BYTE_OFFSET(CurrentVa);
    return Address;
  }
  
  
  /* 
  Busmaster without s/g support? 
  NOTE: old docs allowed busmasters to pass a NULL Adapter. In this case, MapRegisterBase 
  not being NULL is used to detect a non s/g busmaster.
  */
  if ((!AdapterObject && MapRegisterBase) ||
      (AdapterObject && AdapterObject->Master && !AdapterObject->ScatterGather))
  {
    /*
    NOTE: Busmasters doing common-buffer DMA shouldn't call IoMapTransfer, but I don't
    know if it's illegal... Maybe figure out what to do in this case...
    */

    if( WriteToDevice )
    {
      memcpy(MapRegisterBase,
             (char*)MmGetSystemAddressForMdl(Mdl) + ((ULONG)CurrentVa - (ULONG)MmGetMdlVirtualAddress(Mdl)),
             *Length );
    }

    return MmGetPhysicalAddress(MapRegisterBase);
  }

  DPRINT1("IoMapTransfer: Unsupported operation\n");
  KEBUGCHECK(0);
  return Address;
}


/* EOF */




