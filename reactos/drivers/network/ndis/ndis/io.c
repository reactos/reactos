/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/io.c
 * PURPOSE:     I/O related routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 *   20 Aug 2003 Vizzini - DMA support
 *   3  Oct 2003 Vizzini - Formatting and minor bugfixes
 */

#include "ndissys.h"


VOID NTAPI HandleDeferredProcessing(
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2)
/*
 * FUNCTION: Deferred interrupt processing routine
 * ARGUMENTS:
 *     Dpc             = Pointer to DPC object
 *     DeferredContext = Pointer to context information (LOGICAL_ADAPTER)
 *     SystemArgument1 = Unused
 *     SystemArgument2 = Unused
 */
{
  PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(DeferredContext);

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

  /* Call the deferred interrupt service handler for this adapter */
  (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.HandleInterruptHandler)(
      Adapter->NdisMiniportBlock.MiniportAdapterContext);

  /* re-enable the interrupt */
  NDIS_DbgPrint(MAX_TRACE, ("re-enabling the interrupt\n"));
  if(Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.EnableInterruptHandler)
    (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.EnableInterruptHandler)(
        Adapter->NdisMiniportBlock.MiniportAdapterContext);

  NDIS_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


BOOLEAN NTAPI ServiceRoutine(
    IN  PKINTERRUPT Interrupt,
    IN  PVOID       ServiceContext)
/*
 * FUNCTION: Interrupt service routine
 * ARGUMENTS:
 *     Interrupt      = Pointer to interrupt object
 *     ServiceContext = Pointer to context information (PNDIS_MINIPORT_INTERRUPT)
 * RETURNS
 *     TRUE if a miniport controlled device generated the interrupt
 */
{
  BOOLEAN InterruptRecognized = FALSE;
  BOOLEAN QueueMiniportHandleInterrupt = FALSE;
  PNDIS_MINIPORT_INTERRUPT NdisInterrupt = ServiceContext;
  PNDIS_MINIPORT_BLOCK NdisMiniportBlock = NdisInterrupt->Miniport;

  NDIS_DbgPrint(MAX_TRACE, ("Called. Interrupt (0x%X)\n", NdisInterrupt));

  if (NdisInterrupt->IsrRequested) {
      (*NdisMiniportBlock->DriverHandle->MiniportCharacteristics.ISRHandler)(
          &InterruptRecognized,
          &QueueMiniportHandleInterrupt,
          NdisMiniportBlock->MiniportAdapterContext);

  } else if (NdisMiniportBlock->DriverHandle->MiniportCharacteristics.DisableInterruptHandler) {
      (*NdisMiniportBlock->DriverHandle->MiniportCharacteristics.DisableInterruptHandler)(
          NdisMiniportBlock->MiniportAdapterContext);
       QueueMiniportHandleInterrupt = TRUE;
       InterruptRecognized = TRUE;
  }


  if (QueueMiniportHandleInterrupt)
  {
      NDIS_DbgPrint(MAX_TRACE, ("Queuing DPC.\n"));
      KeInsertQueueDpc(&NdisInterrupt->InterruptDpc, NULL, NULL);
  }

  NDIS_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return InterruptRecognized;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisImmediateReadPortUchar(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    OUT PUCHAR      Data)
{
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
  *Data = READ_PORT_UCHAR((PUCHAR)Port); // FIXME: What to do with WrapperConfigurationContext?
}


/*
 * @implemented
 */
VOID
EXPORT
NdisImmediateReadPortUlong(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    OUT PULONG      Data)
{
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
  *Data = READ_PORT_ULONG((PULONG)Port); // FIXME: What to do with WrapperConfigurationContext?
}


/*
 * @implemented
 */
VOID
EXPORT
NdisImmediateReadPortUshort(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    OUT PUSHORT     Data)
{
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
  *Data = READ_PORT_USHORT((PUSHORT)Port); // FIXME: What to do with WrapperConfigurationContext?
}


/*
 * @implemented
 */
VOID
EXPORT
NdisImmediateWritePortUchar(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    IN  UCHAR       Data)
{
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
  WRITE_PORT_UCHAR((PUCHAR)Port, Data); // FIXME: What to do with WrapperConfigurationContext?
}


/*
 * @implemented
 */
VOID
EXPORT
NdisImmediateWritePortUlong(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    IN  ULONG       Data)
{
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
  WRITE_PORT_ULONG((PULONG)Port, Data); // FIXME: What to do with WrapperConfigurationContext?
}


/*
 * @implemented
 */
VOID
EXPORT
NdisImmediateWritePortUshort(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    IN  USHORT      Data)
{
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
  WRITE_PORT_USHORT((PUSHORT)Port, Data); // FIXME: What to do with WrapperConfigurationContext?
}


IO_ALLOCATION_ACTION NTAPI NdisMapRegisterCallback (
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP            Irp,
    IN PVOID           MapRegisterBase,
    IN PVOID           Context)
/*
 * FUNCTION: Called back during reservation of map registers
 * ARGUMENTS:
 *     DeviceObject: Device object of the deivce setting up DMA
 *     Irp: Reserved; must be ignored
 *     MapRegisterBase: Map registers assigned for transfer
 *     Context: LOGICAL_ADAPTER object of the requesting miniport
 * NOTES:
 *     - Called once per BaseMapRegister (see NdisMAllocateMapRegisters)
 *     - Called at IRQL = DISPATCH_LEVEL
 */
{
  PLOGICAL_ADAPTER Adapter = Context;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  Adapter->NdisMiniportBlock.MapRegisters[Adapter->NdisMiniportBlock.CurrentMapRegister].MapRegister = MapRegisterBase;

  NDIS_DbgPrint(MAX_TRACE, ("setting event and leaving.\n"));

  KeSetEvent(Adapter->NdisMiniportBlock.AllocationEvent, 0, FALSE);

  /* this is only the thing to do for busmaster NICs */
  return DeallocateObjectKeepRegisters;
}


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMAllocateMapRegisters(
    IN  NDIS_HANDLE   MiniportAdapterHandle,
    IN  UINT          DmaChannel,
    IN  NDIS_DMA_SIZE DmaSize,
    IN  ULONG         BaseMapRegistersNeeded,
    IN  ULONG         MaximumBufferSize)
/*
 * FUNCTION: Allocate map registers for use in DMA transfers
 * ARGUMENTS:
 *     MiniportAdapterHandle: Passed in to MiniportInitialize
 *     DmaChannel: DMA channel to use
 *     DmaSize: bit width of DMA transfers
 *     BaseMapRegistersNeeded: number of base map registers requested
 *     MaximumBufferSize: largest single buffer transferred
 * RETURNS:
 *     NDIS_STATUS_SUCCESS on success
 *     NDIS_STATUS_RESOURCES on failure
 * NOTES:
 *     - the win2k ddk and the nt4 ddk have conflicting prototypes for this.
 *       I'm implementing the 2k one.
 *     - do not confuse a "base map register" with a "map register" - they
 *       are different.  Only NDIS seems to use the base concept.  The idea
 *       is that a miniport supplies the number of base map registers it will
 *       need, which is equal to the number of DMA send buffers it manages.
 *       NDIS then allocates a number of map registers to go with each base
 *       map register, so that a driver just has to send the base map register
 *       number during dma operations and NDIS can find the group of real
 *       map registers that represent the transfer.
 *     - Because of the above sillyness, you can only specify a few base map
 *       registers at most.  a 1514-byte packet is two map registers at 4k
 *       page size.
 *     - NDIS limits the total number of allocated map registers to 64,
 *       which (in the case of the above example) limits the number of base
 *       map registers to 32.
 */
{
  DEVICE_DESCRIPTION   Description;
  PDMA_ADAPTER         AdapterObject = 0;
  UINT                 MapRegistersPerBaseRegister = 0;
  ULONG                AvailableMapRegisters;
  NTSTATUS             NtStatus;
  PLOGICAL_ADAPTER     Adapter;
  PDEVICE_OBJECT       DeviceObject = 0;
  KEVENT               AllocationEvent;
  KIRQL                OldIrql;

  NDIS_DbgPrint(MAX_TRACE, ("called: Handle 0x%x, DmaChannel 0x%x, DmaSize 0x%x, BaseMapRegsNeeded: 0x%x, MaxBuffer: 0x%x.\n",
                            MiniportAdapterHandle, DmaChannel, DmaSize, BaseMapRegistersNeeded, MaximumBufferSize));

  memset(&Description,0,sizeof(Description));

  Adapter = (PLOGICAL_ADAPTER)MiniportAdapterHandle;

  ASSERT(Adapter);

  /* only bus masters may call this routine */
  if(!(Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_BUS_MASTER))
    return NDIS_STATUS_NOT_SUPPORTED;

  DeviceObject = Adapter->NdisMiniportBlock.DeviceObject;

  KeInitializeEvent(&AllocationEvent, NotificationEvent, FALSE);
  Adapter->NdisMiniportBlock.AllocationEvent = &AllocationEvent;

  /*
  * map registers correlate to physical pages.  ndis documents a
  * maximum of 64 map registers that it will return.
  * at 4k pages, a 1514-byte buffer can span not more than 2 pages.
  *
  * the number of registers required for a given physical mapping
  * is (first register + last register + one per page size),
  * given that physical mapping is > 2.
  */

  /* unhandled corner case: {1,2}-byte max buffer size */
  ASSERT(MaximumBufferSize > 2);
  MapRegistersPerBaseRegister = ((MaximumBufferSize-2) / (2*PAGE_SIZE)) + 2;

  Description.Version = DEVICE_DESCRIPTION_VERSION;
  Description.Master = TRUE;                         /* implied by calling this function */
  Description.ScatterGather = TRUE;                  /* XXX UNTRUE: All BM DMA are S/G (ms seems to do this) */
  Description.BusNumber = Adapter->NdisMiniportBlock.BusNumber;
  Description.InterfaceType = Adapter->NdisMiniportBlock.BusType;
  Description.DmaChannel = DmaChannel;
  Description.MaximumLength = MaximumBufferSize;
  
  if(DmaSize == NDIS_DMA_64BITS)
    Description.Dma64BitAddresses = TRUE;
  else
    Description.Dma32BitAddresses = TRUE;

  AdapterObject = IoGetDmaAdapter(
    Adapter->NdisMiniportBlock.PhysicalDeviceObject, &Description, &AvailableMapRegisters);

  if(!AdapterObject)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Unable to allocate an adapter object; bailing out\n"));
      return NDIS_STATUS_RESOURCES;
    }

  Adapter->NdisMiniportBlock.SystemAdapterObject = AdapterObject;

  if(AvailableMapRegisters < MapRegistersPerBaseRegister)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Didn't get enough map registers from hal - requested 0x%x, got 0x%x\n",
          MapRegistersPerBaseRegister, AvailableMapRegisters));

      AdapterObject->DmaOperations->PutDmaAdapter(AdapterObject);
      Adapter->NdisMiniportBlock.SystemAdapterObject = NULL;
      return NDIS_STATUS_RESOURCES;
    }

  /* allocate & zero space in the miniport block for the registers */
  Adapter->NdisMiniportBlock.MapRegisters = ExAllocatePool(NonPagedPool, BaseMapRegistersNeeded * sizeof(MAP_REGISTER_ENTRY));
  if(!Adapter->NdisMiniportBlock.MapRegisters)
    {
      NDIS_DbgPrint(MIN_TRACE, ("insufficient resources.\n"));
      AdapterObject->DmaOperations->PutDmaAdapter(AdapterObject);
      Adapter->NdisMiniportBlock.SystemAdapterObject = NULL;
      return NDIS_STATUS_RESOURCES;
    }

  memset(Adapter->NdisMiniportBlock.MapRegisters, 0, BaseMapRegistersNeeded * sizeof(MAP_REGISTER_ENTRY));
  Adapter->NdisMiniportBlock.BaseMapRegistersNeeded = (USHORT)BaseMapRegistersNeeded;

  while(BaseMapRegistersNeeded)
    {
      NDIS_DbgPrint(MAX_TRACE, ("iterating, basemapregistersneeded = %d\n", BaseMapRegistersNeeded));

      BaseMapRegistersNeeded--;
      Adapter->NdisMiniportBlock.CurrentMapRegister = (USHORT)BaseMapRegistersNeeded;
      KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        {
          NtStatus = AdapterObject->DmaOperations->AllocateAdapterChannel(
              AdapterObject, DeviceObject, MapRegistersPerBaseRegister,
              NdisMapRegisterCallback, Adapter);
        }
      KeLowerIrql(OldIrql);

      if(!NT_SUCCESS(NtStatus))
        {
          NDIS_DbgPrint(MIN_TRACE, ("IoAllocateAdapterChannel failed: 0x%x\n", NtStatus));
          ExFreePool(Adapter->NdisMiniportBlock.MapRegisters);
          AdapterObject->DmaOperations->PutDmaAdapter(AdapterObject);
          Adapter->NdisMiniportBlock.CurrentMapRegister = Adapter->NdisMiniportBlock.BaseMapRegistersNeeded = 0;
          Adapter->NdisMiniportBlock.SystemAdapterObject = NULL;
          return NDIS_STATUS_RESOURCES;
        }

      NDIS_DbgPrint(MAX_TRACE, ("waiting on event\n"));

      NtStatus = KeWaitForSingleObject(&AllocationEvent, Executive, KernelMode, FALSE, 0);

      if(!NT_SUCCESS(NtStatus))
        {
          NDIS_DbgPrint(MIN_TRACE, ("KeWaitForSingleObject failed: 0x%x\n", NtStatus));
          ExFreePool(Adapter->NdisMiniportBlock.MapRegisters);
          AdapterObject->DmaOperations->PutDmaAdapter(AdapterObject);
          Adapter->NdisMiniportBlock.CurrentMapRegister = Adapter->NdisMiniportBlock.BaseMapRegistersNeeded = 0;
          Adapter->NdisMiniportBlock.SystemAdapterObject = NULL;
          return NDIS_STATUS_RESOURCES;
        }

      NDIS_DbgPrint(MAX_TRACE, ("resetting event\n"));

      KeResetEvent(&AllocationEvent);
    }

  NDIS_DbgPrint(MAX_TRACE, ("returning success\n"));
  return NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMStartBufferPhysicalMapping(
    IN  NDIS_HANDLE                 MiniportAdapterHandle,
    IN  PNDIS_BUFFER                Buffer,
    IN  ULONG                       PhysicalMapRegister,
    IN  BOOLEAN                     WriteToDevice,
    OUT PNDIS_PHYSICAL_ADDRESS_UNIT	PhysicalAddressArray,
    OUT PUINT                       ArraySize)
/*
 * FUNCTION: Sets up map registers for a bus-master DMA transfer
 * ARGUMENTS:
 *     MiniportAdapterHandle: handle originally input to MiniportInitialize
 *     Buffer: data to be transferred
 *     PhysicalMapRegister: specifies the map register to set up
 *     WriteToDevice: if true, data is being written to the device; else it is being read
 *     PhysicalAddressArray: list of mapped ranges suitable for DMA with the device
 *     ArraySize: number of elements in PhysicalAddressArray
 * NOTES:
 *     - Must be called at IRQL <= DISPATCH_LEVEL
 *     - The basic idea:  call IoMapTransfer() in a loop as many times as it takes
 *       in order to map all of the virtual memory to physical memoroy readable
 *       by the device
 *     - The caller supplies storage for the physical address array.
 */
{
  PLOGICAL_ADAPTER Adapter;
  PVOID CurrentVa;
  ULONG TotalLength;
  PHYSICAL_ADDRESS ReturnedAddress;
  UINT LoopCount =  0;

  ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
  ASSERT(MiniportAdapterHandle && Buffer && PhysicalAddressArray && ArraySize);

  Adapter = (PLOGICAL_ADAPTER)MiniportAdapterHandle;
  CurrentVa = MmGetMdlVirtualAddress(Buffer);
  TotalLength = MmGetMdlByteCount(Buffer);

  while(TotalLength)
    {
      ULONG Length = TotalLength;

      ReturnedAddress = Adapter->NdisMiniportBlock.SystemAdapterObject->DmaOperations->MapTransfer(
          Adapter->NdisMiniportBlock.SystemAdapterObject, Buffer,
          Adapter->NdisMiniportBlock.MapRegisters[PhysicalMapRegister].MapRegister,
          CurrentVa, &Length, WriteToDevice);

      Adapter->NdisMiniportBlock.MapRegisters[PhysicalMapRegister].WriteToDevice = WriteToDevice;

      PhysicalAddressArray[LoopCount].PhysicalAddress = ReturnedAddress;
      PhysicalAddressArray[LoopCount].Length = Length;

      TotalLength -= Length;
      CurrentVa = (PVOID)((ULONG_PTR)CurrentVa + Length);

      LoopCount++;
    }

  *ArraySize = LoopCount;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMCompleteBufferPhysicalMapping(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_BUFFER    Buffer,
    IN  ULONG           PhysicalMapRegister)
/*
 * FUNCTION: Complete dma action started by NdisMStartBufferPhysicalMapping
 * ARGUMENTS:
 *     - MiniportAdapterHandle: handle originally input to MiniportInitialize
 *     - Buffer: NDIS_BUFFER to complete the mapping on
 *     - PhyscialMapRegister: the chosen map register
 * NOTES:
 *     - May be called at IRQL <= DISPATCH_LEVEL
 */
{
  PLOGICAL_ADAPTER Adapter;
  VOID *CurrentVa;
  ULONG Length;

  ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
  ASSERT(MiniportAdapterHandle && Buffer);

  Adapter = (PLOGICAL_ADAPTER)MiniportAdapterHandle;
  CurrentVa = MmGetMdlVirtualAddress(Buffer);
  Length = MmGetMdlByteCount(Buffer);

  Adapter->NdisMiniportBlock.SystemAdapterObject->DmaOperations->FlushAdapterBuffers(
      Adapter->NdisMiniportBlock.SystemAdapterObject, Buffer,
      Adapter->NdisMiniportBlock.MapRegisters[PhysicalMapRegister].MapRegister,
      CurrentVa, Length,
      Adapter->NdisMiniportBlock.MapRegisters[PhysicalMapRegister].WriteToDevice);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMDeregisterDmaChannel(
    IN  PNDIS_HANDLE    MiniportDmaHandle)
{
    PNDIS_MINIPORT_BLOCK NdisMiniportBlock = (PNDIS_MINIPORT_BLOCK)MiniportDmaHandle;
    PDMA_ADAPTER AdapterObject = NdisMiniportBlock->SystemAdapterObject;

    AdapterObject->DmaOperations->PutDmaAdapter(AdapterObject);

    NdisMiniportBlock->SystemAdapterObject = NULL;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMDeregisterInterrupt(
    IN  PNDIS_MINIPORT_INTERRUPT    Interrupt)
/*
 * FUNCTION: Releases an interrupt vector
 * ARGUMENTS:
 *     Interrupt = Pointer to interrupt object
 */
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
    IoDisconnectInterrupt(Interrupt->InterruptObject);
    Interrupt->Miniport->RegisteredInterrupts--;

    if (Interrupt->Miniport->Interrupt == Interrupt)
        Interrupt->Miniport->Interrupt = NULL;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMFreeMapRegisters(
    IN  NDIS_HANDLE MiniportAdapterHandle)
/*
 * FUNCTION: Free previously allocated map registers
 * ARGUMENTS:
 *     MiniportAdapterHandle:  Handle originally passed in to MiniportInitialize
 * NOTES:
 */
{
  KIRQL                OldIrql;
  PLOGICAL_ADAPTER Adapter = (PLOGICAL_ADAPTER)MiniportAdapterHandle;
  PDMA_ADAPTER         AdapterObject;
  UINT                 MapRegistersPerBaseRegister;
  UINT                 i;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  ASSERT(Adapter);

  /* only bus masters may call this routine */
  ASSERT(Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_BUS_MASTER);
  if(!(Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_BUS_MASTER) ||
     Adapter->NdisMiniportBlock.SystemAdapterObject == NULL)
    return;

  MapRegistersPerBaseRegister = ((Adapter->NdisMiniportBlock.MaximumPhysicalMapping - 2) / PAGE_SIZE) + 2;

  AdapterObject = Adapter->NdisMiniportBlock.SystemAdapterObject;

  KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    {
      for(i = 0; i < Adapter->NdisMiniportBlock.BaseMapRegistersNeeded; i++)
        {
          AdapterObject->DmaOperations->FreeMapRegisters(
              Adapter->NdisMiniportBlock.SystemAdapterObject,
              Adapter->NdisMiniportBlock.MapRegisters[i].MapRegister,
              MapRegistersPerBaseRegister);
        }
    }
 KeLowerIrql(OldIrql);

 AdapterObject->DmaOperations->PutDmaAdapter(AdapterObject);
 Adapter->NdisMiniportBlock.SystemAdapterObject = NULL;

 ExFreePool(Adapter->NdisMiniportBlock.MapRegisters);
}


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMMapIoSpace(
    OUT PVOID                   *VirtualAddress,
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress,
    IN  UINT                    Length)
/*
 * FUNCTION: Maps a bus-relative address to a system-wide virtual address
 * ARGUMENTS:
 *     VirtualAddress: receives virtual address of mapping
 *     MiniportAdapterHandle: Handle originally input to MiniportInitialize
 *     PhysicalAddress: bus-relative address to map
 *     Length: Number of bytes to map
 * RETURNS:
 *     NDIS_STATUS_SUCCESS: the operation completed successfully
 *     NDIS_STATUS_RESOURCE_CONFLICT: the physical address range is already claimed
 *     NDIS_STATUS_RESOURCES: insufficient resources to complete the mapping
 *     NDIS_STATUS_FAILURE: a general failure has occured
 * NOTES:
 *     - Must be called at IRQL = PASSIVE_LEVEL
 * BUGS:
 *     - Only supports things that MmMapIoSpace internally supports - what
 *       about considering bus type, etc?
 *     - doesn't track resources allocated...
 */
{
  PAGED_CODE();
  ASSERT(VirtualAddress && MiniportAdapterHandle);

  *VirtualAddress = MmMapIoSpace(PhysicalAddress, Length, MmNonCached);

  if(!*VirtualAddress)
    return NDIS_STATUS_RESOURCES;

  return NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
ULONG
EXPORT
NdisMReadDmaCounter(
    IN  NDIS_HANDLE MiniportDmaHandle)
{
  PNDIS_MINIPORT_BLOCK MiniportBlock = (PNDIS_MINIPORT_BLOCK)MiniportDmaHandle;
  PDMA_ADAPTER AdapterObject = MiniportBlock->SystemAdapterObject;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  if (AdapterObject == NULL)
    return 0;

  return AdapterObject->DmaOperations->ReadDmaCounter(AdapterObject);
}


/*
 * @implemented
 */
ULONG
EXPORT
NdisMGetDmaAlignment(
    IN  NDIS_HANDLE MiniportDmaHandle)
{
  PNDIS_MINIPORT_BLOCK MiniportBlock = (PNDIS_MINIPORT_BLOCK)MiniportDmaHandle;
  PDMA_ADAPTER AdapterObject = MiniportBlock->SystemAdapterObject;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  if (AdapterObject == NULL)
    return 0;

  return AdapterObject->DmaOperations->GetDmaAlignment(AdapterObject);
}


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMRegisterDmaChannel(
    OUT PNDIS_HANDLE            MiniportDmaHandle,
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  UINT                    DmaChannel,
    IN  BOOLEAN                 Dma32BitAddresses,
    IN  PNDIS_DMA_DESCRIPTION   DmaDescription,
    IN  ULONG                   MaximumLength)
{
  PLOGICAL_ADAPTER Adapter = (PLOGICAL_ADAPTER)MiniportAdapterHandle;
  DEVICE_DESCRIPTION DeviceDesc;
  ULONG MapRegisters;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  if (Adapter->NdisMiniportBlock.SystemAdapterObject)
  {
      NDIS_DbgPrint(MIN_TRACE,("Using existing DMA adapter\n"));
      *MiniportDmaHandle = &Adapter->NdisMiniportBlock;
      return NDIS_STATUS_SUCCESS;
  }

  RtlZeroMemory(&DeviceDesc, sizeof(DEVICE_DESCRIPTION));

  DeviceDesc.Version = DEVICE_DESCRIPTION_VERSION;
  DeviceDesc.Master = (Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_BUS_MASTER);
  DeviceDesc.ScatterGather = FALSE; //Is this correct?
  DeviceDesc.DemandMode = DmaDescription->DemandMode;
  DeviceDesc.AutoInitialize = DmaDescription->AutoInitialize;
  DeviceDesc.Dma32BitAddresses = Dma32BitAddresses;
  DeviceDesc.Dma64BitAddresses = !Dma32BitAddresses; //Is this correct?
  DeviceDesc.BusNumber = Adapter->NdisMiniportBlock.BusNumber;
  DeviceDesc.DmaChannel = DmaDescription->DmaChannel;
  DeviceDesc.InterfaceType = Adapter->NdisMiniportBlock.BusType;
  DeviceDesc.DmaWidth = DmaDescription->DmaWidth;
  DeviceDesc.DmaSpeed = DmaDescription->DmaSpeed;
  DeviceDesc.MaximumLength = MaximumLength;

  Adapter->NdisMiniportBlock.SystemAdapterObject = 
         IoGetDmaAdapter(Adapter->NdisMiniportBlock.PhysicalDeviceObject, &DeviceDesc, &MapRegisters);

  if (!Adapter->NdisMiniportBlock.SystemAdapterObject)
      return NDIS_STATUS_RESOURCES;

  *MiniportDmaHandle = &Adapter->NdisMiniportBlock;

  return NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMRegisterInterrupt(
    OUT PNDIS_MINIPORT_INTERRUPT    Interrupt,
    IN  NDIS_HANDLE                 MiniportAdapterHandle,
    IN  UINT                        InterruptVector,
    IN  UINT                        InterruptLevel,
    IN  BOOLEAN	                    RequestIsr,
    IN  BOOLEAN                     SharedInterrupt,
    IN  NDIS_INTERRUPT_MODE         InterruptMode)
/*
 * FUNCTION: Claims access to an interrupt vector
 * ARGUMENTS:
 *     Interrupt             = Address of interrupt object to initialize
 *     MiniportAdapterHandle = Specifies handle input to MiniportInitialize
 *     InterruptVector       = Specifies bus-relative vector to register
 *     InterruptLevel        = Specifies bus-relative DIRQL vector for interrupt
 *     RequestIsr            = TRUE if MiniportISR should always be called
 *     SharedInterrupt       = TRUE if other devices may use the same interrupt
 *     InterruptMode         = Specifies type of interrupt
 * RETURNS:
 *     Status of operation
 */
{
  NTSTATUS Status;
  ULONG MappedIRQ;
  KIRQL DIrql;
  KAFFINITY Affinity;
  PLOGICAL_ADAPTER Adapter = (PLOGICAL_ADAPTER)MiniportAdapterHandle;

  NDIS_DbgPrint(MAX_TRACE, ("Called. InterruptVector (0x%X)  InterruptLevel (0x%X)  "
      "SharedInterrupt (%d)  InterruptMode (0x%X)\n",
      InterruptVector, InterruptLevel, SharedInterrupt, InterruptMode));

  RtlZeroMemory(Interrupt, sizeof(NDIS_MINIPORT_INTERRUPT));

  KeInitializeSpinLock(&Interrupt->DpcCountLock);

  KeInitializeDpc(&Interrupt->InterruptDpc, HandleDeferredProcessing, Adapter);

  KeInitializeEvent(&Interrupt->DpcsCompletedEvent, NotificationEvent, FALSE);

  Interrupt->SharedInterrupt = SharedInterrupt;
  Interrupt->IsrRequested = RequestIsr;
  Interrupt->Miniport = &Adapter->NdisMiniportBlock;

  MappedIRQ = HalGetInterruptVector(Adapter->NdisMiniportBlock.BusType, Adapter->NdisMiniportBlock.BusNumber,
                                    InterruptLevel, InterruptVector, &DIrql,
                                    &Affinity);

  NDIS_DbgPrint(MAX_TRACE, ("Connecting to interrupt vector (0x%X)  Affinity (0x%X).\n", MappedIRQ, Affinity));

  Status = IoConnectInterrupt(&Interrupt->InterruptObject, ServiceRoutine, Interrupt, &Interrupt->DpcCountLock, MappedIRQ,
      DIrql, DIrql, InterruptMode, SharedInterrupt, Affinity, FALSE);

  NDIS_DbgPrint(MAX_TRACE, ("Leaving. Status (0x%X).\n", Status));

  if (NT_SUCCESS(Status)) {
      Adapter->NdisMiniportBlock.Interrupt = Interrupt;
      Adapter->NdisMiniportBlock.RegisteredInterrupts++;
      return NDIS_STATUS_SUCCESS;
  }

  if (Status == STATUS_INSUFFICIENT_RESOURCES)
    {
        /* FIXME: Log error */
      NDIS_DbgPrint(MIN_TRACE, ("Resource conflict!\n"));
      return NDIS_STATUS_RESOURCE_CONFLICT;
    }

  NDIS_DbgPrint(MIN_TRACE, ("Function failed. Status (0x%X).\n", Status));
  return NDIS_STATUS_FAILURE;
}


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMRegisterIoPortRange(
    OUT PVOID       *PortOffset,
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  UINT        InitialPort,
    IN  UINT        NumberOfPorts)
/*
 * FUNCTION: Sets up driver access to device I/O ports
 * ARGUMENTS:
 *     PortOffset            = Address of buffer to place mapped base port address
 *     MiniportAdapterHandle = Specifies handle input to MiniportInitialize
 *     InitialPort           = Bus-relative base port address of a range to be mapped
 *     NumberOfPorts         = Specifies number of ports to be mapped
 * RETURNS:
 *     Status of operation
 */
{
  PHYSICAL_ADDRESS     PortAddress, TranslatedAddress;
  PLOGICAL_ADAPTER Adapter  = (PLOGICAL_ADAPTER)MiniportAdapterHandle;
  ULONG                AddressSpace = 1;    /* FIXME The HAL handles this wrong atm */

  *PortOffset = 0;

  NDIS_DbgPrint(MAX_TRACE, ("Called - InitialPort 0x%x, NumberOfPorts 0x%x\n", InitialPort, NumberOfPorts));

  memset(&PortAddress, 0, sizeof(PortAddress));

  /*
   * FIXME: NDIS 5+ completely ignores the InitialPort parameter, but
   * we don't have a way to get the I/O base address yet (see
   * NDIS_MINIPORT_BLOCK->AllocatedResources and
   * NDIS_MINIPORT_BLOCK->AllocatedResourcesTranslated).
   */
  if(InitialPort)
      PortAddress = RtlConvertUlongToLargeInteger(InitialPort);
  else
      ASSERT(FALSE);

  NDIS_DbgPrint(MAX_TRACE, ("Translating address 0x%x 0x%x\n", PortAddress.u.HighPart, PortAddress.u.LowPart));

  if(!HalTranslateBusAddress(Adapter->NdisMiniportBlock.BusType, Adapter->NdisMiniportBlock.BusNumber,
                             PortAddress, &AddressSpace, &TranslatedAddress))
    {
      NDIS_DbgPrint(MIN_TRACE, ("Unable to translate address\n"));
      return NDIS_STATUS_RESOURCES;
    }

  NDIS_DbgPrint(MAX_TRACE, ("Hal returned AddressSpace=0x%x TranslatedAddress=0x%x 0x%x\n",
                            AddressSpace, TranslatedAddress.u.HighPart, TranslatedAddress.u.LowPart));

  if(AddressSpace)
    {
      ASSERT(TranslatedAddress.u.HighPart == 0);
      *PortOffset = (PVOID) TranslatedAddress.u.LowPart;
      NDIS_DbgPrint(MAX_TRACE, ("Returning 0x%x\n", *PortOffset));
      return NDIS_STATUS_SUCCESS;
    }

  NDIS_DbgPrint(MAX_TRACE, ("calling MmMapIoSpace\n"));

  *PortOffset = MmMapIoSpace(TranslatedAddress, NumberOfPorts, MmNonCached);
  NDIS_DbgPrint(MAX_TRACE, ("Returning 0x%x for port range\n", *PortOffset));

  if(!*PortOffset)
    return NDIS_STATUS_RESOURCES;

  return NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMUnmapIoSpace(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  PVOID       VirtualAddress,
    IN  UINT        Length)
/*
 * FUNCTION: Un-maps space previously mapped with NdisMMapIoSpace
 * ARGUMENTS:
 *     MiniportAdapterHandle: handle originally passed into MiniportInitialize
 *     VirtualAddress: Address to un-map
 *     Length: length of the mapped memory space
 * NOTES:
 *     - Must be called at IRQL = PASSIVE_LEVEL
 *     - Must only be called from MiniportInitialize and MiniportHalt
 *     - See also: NdisMMapIoSpace
 * BUGS:
 *     - Depends on MmUnmapIoSpace to Do The Right Thing in all cases
 */
{
  PAGED_CODE();

  ASSERT(MiniportAdapterHandle);

  MmUnmapIoSpace(VirtualAddress, Length);
}

/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMInitializeScatterGatherDma(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  BOOLEAN     Dma64BitAddresses,
    IN  ULONG       MaximumPhysicalMapping)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    PLOGICAL_ADAPTER Adapter = (PLOGICAL_ADAPTER)MiniportAdapterHandle;
    ULONG MapRegisters;
    DEVICE_DESCRIPTION DeviceDesc;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    if (!(Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_BUS_MASTER))
        return NDIS_STATUS_NOT_SUPPORTED;

    if (Adapter->NdisMiniportBlock.SystemAdapterObject)
    {
        NDIS_DbgPrint(MIN_TRACE,("Using existing DMA adapter\n"));
        return NDIS_STATUS_SUCCESS;
    }

    RtlZeroMemory(&DeviceDesc, sizeof(DEVICE_DESCRIPTION));

    DeviceDesc.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDesc.Master = TRUE;
    DeviceDesc.ScatterGather = TRUE;
    DeviceDesc.Dma32BitAddresses = !Dma64BitAddresses;
    DeviceDesc.Dma64BitAddresses = Dma64BitAddresses;
    DeviceDesc.BusNumber = Adapter->NdisMiniportBlock.BusNumber;
    DeviceDesc.InterfaceType = Adapter->NdisMiniportBlock.BusType;
    DeviceDesc.MaximumLength = MaximumPhysicalMapping;

    Adapter->NdisMiniportBlock.SystemAdapterObject = 
         IoGetDmaAdapter(Adapter->NdisMiniportBlock.PhysicalDeviceObject, &DeviceDesc, &MapRegisters);

    if (!Adapter->NdisMiniportBlock.SystemAdapterObject)
        return NDIS_STATUS_RESOURCES;

    return NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMapIoSpace(
    OUT PNDIS_STATUS            Status,
    OUT PVOID                   *VirtualAddress,
    IN  NDIS_HANDLE             NdisAdapterHandle,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress,
    IN  UINT                    Length)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    *Status = NdisMMapIoSpace(VirtualAddress,
                              NdisAdapterHandle,
                              PhysicalAddress,
                              Length);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisFreeDmaChannel(
    IN  PNDIS_HANDLE    NdisDmaHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    NdisMDeregisterDmaChannel(NdisDmaHandle);
}



/* EOF */

