/* $Id: adapter.c,v 1.14 2004/11/24 17:54:38 navaraf Exp $
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
#define NDEBUG
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
  if(KeInsertDeviceQueue(&AdapterObject->ChannelWaitQueue, &WaitContextBlock->WaitQueueEntry))
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

  AdapterObject->CommittedMapRegisters = NumberOfMapRegisters;

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
      AdapterObject->CommittedMapRegisters = 0;
      IoFreeAdapterChannel(AdapterObject);
    }

  /*
   * if we don't call IoFreeAdapterChannel, the next device won't get de-queued,
   * which is what we want.
   */

  return STATUS_SUCCESS;
}


BOOLEAN
HalpGrowMapBuffers(
  IN PADAPTER_OBJECT	AdapterObject,
  IN ULONG		SizeOfMapBuffers)
/*
 * FUNCTION: Allocate initial, or additional, map buffers for IO adapters.
 * ARGUMENTS:
 *     AdapterObject: DMA adapter to allocate buffers for.
 *     SizeOfMapBuffers: Size of the map buffers to allocate
 * NOTES:
 *     - Needs to be tested...
 */
{
	//ULONG PagesToAllocate = BYTES_TO_PAGES(SizeOfMapBuffers);
	
	/* TODO: Allocation */

	return TRUE;
}

PADAPTER_OBJECT STDCALL
HalpAllocateAdapterEx(
  ULONG NumberOfMapRegisters,
  BOOLEAN IsMaster,
  BOOLEAN Dma32BitAddresses)
/*
 * FUNCTION: Allocates an ADAPTER_OBJECT, optionally creates the Master Adapter.
 * ARGUMENTS:
 *     - NumberOfMapRegisters: Number of map registers to allocate
 *     - IsMaster: Wether this is a Master Device or not
 *     - Dma32BitAddresses: Wether 32-bit Addresses are supported
 * RETURNS:
 *     - Pointer to Adapter Object, or NULL if failure.
 * BUGS:
 *     - Some stuff is unhandled/incomplete
 */
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	ULONG ObjectSize;
	ULONG BitmapSize;
	NTSTATUS Status;
	ULONG AllowedMapRegisters = 64;
	PADAPTER_OBJECT AdapterObject;
	HANDLE Handle;
	
	/* Allocate the Master Adapter if we haven't already 
	   but make sure we're not asked to do it now, and also check if we need it */
	if ((MasterAdapter == NULL) && (!IsMaster) && (NumberOfMapRegisters)) {
		
		/* Allocate and Save */
		DPRINT("Allocating the Master Adapter Object\n");
		MasterAdapter = HalpAllocateAdapterEx(NumberOfMapRegisters,
						     TRUE,
						     Dma32BitAddresses);
		
		/* Cancel on Failure */
		DPRINT("Checking if Master Adapter was allocated properly\n");
		if (!MasterAdapter) return NULL;
	}
	
	/* Initialize the Object Attributes for the Adapter Object */
	InitializeObjectAttributes(&ObjectAttributes,
				   NULL,
				   OBJ_PERMANENT,
				   NULL,
				   NULL);
	
	/* Check if this is the Master Adapter, in which case we need to allocate the bitmap */
	if (IsMaster) {
		/* Size due to the Bitmap + Bytes in the Bitmap Buffer (8 bytes, 64 bits)*/
		BitmapSize = sizeof(RTL_BITMAP) + AllowedMapRegisters / 8;
		
		/* We will put the Bitmap Buffer after the Adapter Object for simplicity */
		ObjectSize = sizeof(ADAPTER_OBJECT) + BitmapSize;
	} else {
		ObjectSize = sizeof(ADAPTER_OBJECT);
	}
	
	/* Create and Allocate the Object */
	DPRINT("Creating the Object\n");
	Status = ObCreateObject(KernelMode,
				IoAdapterObjectType,
				&ObjectAttributes,
				KernelMode,
				NULL,
				ObjectSize,
				0,
				0,
				(PVOID)&AdapterObject);
	
	if (!NT_SUCCESS(Status)) return NULL;
	
	/* Add a Reference */
	DPRINT("Referencing the Object\n");
	Status = ObReferenceObjectByPointer(AdapterObject,
					    FILE_READ_DATA | FILE_WRITE_DATA,
					    IoAdapterObjectType,
					    KernelMode);
	
	if (!NT_SUCCESS(Status)) return NULL;
	
	/* It's a Valid Object, so now we can play with the memory */
	RtlZeroMemory(AdapterObject, sizeof(ADAPTER_OBJECT));
	
	/* Insert it into the Object Table */
	DPRINT("Inserting the Object\n");
	Status = ObInsertObject(AdapterObject,
				NULL,
				FILE_READ_DATA | FILE_WRITE_DATA,
				0,
				NULL,
				&Handle);
	
	if (!NT_SUCCESS(Status)) return NULL;
	
	/* We don't want the handle */
	NtClose(Handle);	
	
	/* Set up the Adapter Object fields */
	AdapterObject->MapRegistersPerChannel = 1;
	
	/* Set the Master if needed (master only needed if we use Map Registers) */
	if (NumberOfMapRegisters) AdapterObject->MasterAdapter = MasterAdapter;
	
	/* Initalize the Channel Wait queue, which every adapter has */
	DPRINT("Initializing the Device Queue of the Object\n");
	KeInitializeDeviceQueue(&AdapterObject->ChannelWaitQueue);
	
	/* Initialize the SpinLock, Queue and Bitmap, which are kept in the Master Adapter only */
	if (IsMaster) {
		
		DPRINT("Initializing the Master Adapter Stuff\n");
		KeInitializeSpinLock(&AdapterObject->SpinLock);
		InitializeListHead(&AdapterObject->AdapterQueue);
		
		/* As said previously, we put them here for simplicity */
		AdapterObject->MapRegisters = (PVOID)(AdapterObject + 1);
		
		/* Set up Bitmap */
		RtlInitializeBitMap(AdapterObject->MapRegisters,
				    (PULONG)(AdapterObject->MapRegisters + 1),
				    AllowedMapRegisters);
				    
		/* Reset the Bitmap */	
		RtlSetAllBits(AdapterObject->MapRegisters);
		AdapterObject->NumberOfMapRegisters = 0;
		AdapterObject->CommittedMapRegisters = 0;
		
		/* Allocate Memory for the Map Registers */
		AdapterObject->MapRegisterBase = ExAllocatePool(NonPagedPool,
								AllowedMapRegisters * sizeof(DWORD));
		
		/* Clear them */
		RtlZeroMemory(AdapterObject->MapRegisterBase, AllowedMapRegisters * sizeof(DWORD));
		
		/* Allocate the contigous memory */
		DPRINT("Allocating Buffers\n");
		HalpGrowMapBuffers(AdapterObject, 0x1000000);
	}
	
	DPRINT("Adapter Object allocated\n");
	return AdapterObject;	
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
  if (AdapterObject->AdapterNumber == 1) {
  
  		/* Set this for Ease */
  		PDMA1_CONTROL DmaControl1 = AdapterObject->AdapterBaseVa;
	
		/* Set Channel */
		WRITE_PORT_UCHAR(&DmaControl1->SingleMask, AdapterObject->ChannelNumber | DMA_SETMASK);
  } else {
  		/* Set this for Ease */
  		PDMA2_CONTROL DmaControl2 = AdapterObject->AdapterBaseVa;
	
		/* Set Channel */
		WRITE_PORT_UCHAR(&DmaControl2->SingleMask, (AdapterObject->ChannelNumber - 4) | DMA_SETMASK);
  }
  
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
      if(AdapterObject->CommittedMapRegisters)
        IoFreeMapRegisters(AdapterObject, AdapterObject->MapRegisterBase, AdapterObject->CommittedMapRegisters);

      if(!(WaitContextBlock = (PWAIT_CONTEXT_BLOCK)KeRemoveDeviceQueue(&AdapterObject->ChannelWaitQueue)))
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
          AdapterObject->CommittedMapRegisters = 0;
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
  if( AdapterObject->CommittedMapRegisters )
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
  KIRQL OldIrql;
  UCHAR Mode;

#if defined(__GNUC__)
  Address.QuadPart = 0ULL;
#else
  Address.QuadPart = 0;
#endif

  /* Isa System (slave) DMA? */
  if (MapRegisterBase && !AdapterObject->MasterDevice)
  {

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

     /* Get the mode for easier coding */
     Mode = AdapterObject->AdapterMode;
     
    /* if it is a write to the device, copy the caller buffer to the low buffer */
    if ((WriteToDevice) && !((PDMA_MODE)&Mode)->AutoInitialize)
    {
      memcpy(MapRegisterBase,
             (char*)MmGetSystemAddressForMdl(Mdl) + ((ULONG)CurrentVa - (ULONG)MmGetMdlVirtualAddress(Mdl)),
	           *Length );
    }

    /* Writer Adapter Mode, transfer type */
    ((PDMA_MODE)&Mode)->TransferType = (WriteToDevice ? WRITE_TRANSFER : READ_TRANSFER);

    // program up the dma controller, and return
    if (!((PDMA_MODE)&Mode)->AutoInitialize) {
      Address = MmGetPhysicalAddress( MapRegisterBase );
    } else {
      Address = MmGetPhysicalAddress( CurrentVa );
    }
    
    /* 16-bit DMA has a shifted length */
    if (AdapterObject->Width16Bits) *Length = (*Length >> 1);
     
    /* Make the Transfer */
    if (AdapterObject->AdapterNumber == 1) {
    
    	PDMA1_CONTROL DmaControl1 = AdapterObject->AdapterBaseVa; /* For Writing Less Code */
    	
	/* Reset Register */
	WRITE_PORT_UCHAR(&DmaControl1->ClearBytePointer, 0);
	
	/* Set the Mode */
	WRITE_PORT_UCHAR(&DmaControl1->Mode, (UCHAR)(Mode));
	
	/* Set the Page Register (apparently always 0 for us if I trust the previous comment) */
	WRITE_PORT_UCHAR(&DmaControl1->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseAddress, 0);
	WRITE_PORT_UCHAR(&DmaControl1->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseAddress, 0);
	
	/* Set the Length */
 	WRITE_PORT_UCHAR(&DmaControl1->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseCount,
			(UCHAR)((*Length) - 1));
	WRITE_PORT_UCHAR(&DmaControl1->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseCount,
			(UCHAR)((*Length) - 1) >> 8);
			
	/* Unmask the Channel */
	WRITE_PORT_UCHAR(&DmaControl1->SingleMask, AdapterObject->ChannelNumber | DMA_CLEARMASK);
    } else {
        PDMA2_CONTROL DmaControl2 = AdapterObject->AdapterBaseVa; /* For Writing Less Code */
    	
	/* Reset Register */
	WRITE_PORT_UCHAR(&DmaControl2->ClearBytePointer, 0);
	
	/* Set the Mode */
	WRITE_PORT_UCHAR(&DmaControl2->Mode, (UCHAR)(Mode));
	
	/* Set the Page Register (apparently always 0 for us if I trust the previous comment) */
	WRITE_PORT_UCHAR(&DmaControl2->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseAddress, 0);
	WRITE_PORT_UCHAR(&DmaControl2->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseAddress, 0);
	
	/* Set the Length */
 	WRITE_PORT_UCHAR(&DmaControl2->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseCount,
			(UCHAR)((*Length) - 1));
	WRITE_PORT_UCHAR(&DmaControl2->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseCount,
			(UCHAR)((*Length) - 1) >> 8);
			
	/* Unmask the Channel */
	WRITE_PORT_UCHAR(&DmaControl2->SingleMask, AdapterObject->ChannelNumber | DMA_CLEARMASK);
    }
	
    /* Release Spinlock */
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
      (AdapterObject && AdapterObject->MasterDevice && AdapterObject->ScatterGather))
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
      (AdapterObject && AdapterObject->MasterDevice && !AdapterObject->ScatterGather))
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

  DPRINT("IoMapTransfer: Unsupported operation\n");
  KEBUGCHECK(0);
  return Address;
}


/* EOF */




