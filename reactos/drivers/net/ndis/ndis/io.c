/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/io.c
 * PURPOSE:     I/O related routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 *   8-20-2003 Vizzini - DMA support
 */
#include <ndissys.h>
#include <miniport.h>

VOID STDCALL HandleDeferredProcessing(
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
    BOOLEAN WasBusy;
    PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(DeferredContext);

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    KeAcquireSpinLockAtDpcLevel(&Adapter->NdisMiniportBlock.Lock);
    WasBusy = Adapter->MiniportBusy;
    Adapter->MiniportBusy = TRUE;
    KeReleaseSpinLockFromDpcLevel(&Adapter->NdisMiniportBlock.Lock);

    /* Call the deferred interrupt service handler for this adapter */
    (*Adapter->Miniport->Chars.HandleInterruptHandler)(
        Adapter->NdisMiniportBlock.MiniportAdapterContext);

    KeAcquireSpinLockAtDpcLevel(&Adapter->NdisMiniportBlock.Lock);
    if ((!WasBusy) && (Adapter->WorkQueueHead)) {
        KeInsertQueueDpc(&Adapter->MiniportDpc, NULL, NULL);
    } else {
        Adapter->MiniportBusy = WasBusy;
    }
    KeReleaseSpinLockFromDpcLevel(&Adapter->NdisMiniportBlock.Lock);

    NDIS_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


BOOLEAN STDCALL ServiceRoutine(
    IN  PKINTERRUPT Interrupt,
    IN  PVOID       ServiceContext)
/*
 * FUNCTION: Interrupt service routine
 * ARGUMENTS:
 *     Interrupt      = Pointer to interrupt object
 *     ServiceContext = Pointer to context information (LOGICAL_ADAPTER)
 * RETURNS
 *     TRUE if a miniport controlled device generated the interrupt
 */
{
    BOOLEAN InterruptRecognized;
    BOOLEAN QueueMiniportHandleInterrupt;
    PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(ServiceContext);

    NDIS_DbgPrint(MAX_TRACE, ("Called. Adapter (0x%X)\n", Adapter));

    (*Adapter->Miniport->Chars.ISRHandler)(&InterruptRecognized,
                                           &QueueMiniportHandleInterrupt,
                                           Adapter->NdisMiniportBlock.MiniportAdapterContext);

    if (QueueMiniportHandleInterrupt) {
        NDIS_DbgPrint(MAX_TRACE, ("Queueing DPC.\n"));
        KeInsertQueueDpc(&Adapter->NdisMiniportBlock.Interrupt->InterruptDpc, NULL, NULL);
    }

    NDIS_DbgPrint(MAX_TRACE, ("Leaving.\n"));

    return InterruptRecognized;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCompleteDmaTransfer(
    OUT PNDIS_STATUS    Status,
    IN  PNDIS_HANDLE    NdisDmaHandle,
    IN  PNDIS_BUFFER    Buffer,
    IN  ULONG           Offset,
    IN  ULONG           Length,
    IN  BOOLEAN         WriteToDevice)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisFlushBuffer(
    IN  PNDIS_BUFFER    Buffer,
    IN  BOOLEAN         WriteToDevice)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
ULONG
EXPORT
NdisGetCacheFillSize(
    VOID)
{
    UNIMPLEMENTED

	return 0;
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
 * @unimplemented
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


IO_ALLOCATION_ACTION NdisMapRegisterCallback (
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP            Irp,
    IN PVOID           MapRegisterBase,
    IN PVOID           Context)
/*
 * FUNCTION: Called back during reservation of map registers
 */
{
  PLOGICAL_ADAPTER Adapter = (PLOGICAL_ADAPTER)Context;
  PADAPTER_MAP_REGISTER_LIST Register = ExAllocatePool(NonPagedPool, sizeof(ADAPTER_MAP_REGISTER_LIST));

   NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  if(!Register)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources\n"));
      KeSetEvent(&Adapter->DmaEvent, 0, FALSE);
      return DeallocateObject;
    }

  Register->MapRegister = MapRegisterBase;
  Register->NumRegisters = Adapter->MapRegistersRequested;

  ExInterlockedInsertTailList(&Adapter->MapRegisterList.ListEntry, &Register->ListEntry, &Adapter->DmaLock);

  KeSetEvent(&Adapter->DmaEvent, 0, FALSE);

  /* XXX this is only the thing to do for busmaster NICs */
  return DeallocateObjectKeepRegisters;
}

/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMAllocateMapRegisters(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  UINT        DmaChannel,
    IN  BOOLEAN     DmaSize,
    IN  ULONG       BaseMapRegistersNeeded,
    IN  ULONG       MaximumBufferSize)
/*
 * FUNCTION: Allocate map registers for use in DMA transfers
 * ARGUMENTS:
 *     MiniportAdapterHandle: Passed in to MiniportInitialize
 *     DmaChannel: DMA channel to use
 *     DmaSize: bit width of DMA transfers
 *     BaseMapRegistersNeeded: number of map registers requested
 *     MaximumBufferSize: largest single buffer transferred
 * RETURNS:
 *     NDIS_STATUS_SUCCESS on success
 *     NDIS_STATUS_RESOURCES on failure
 * NOTES: 
 *     - the win2k ddk and the nt4 ddk have conflicting prototypes for this.
 *       I'm implementing the 2k one.
 */
{
  DEVICE_DESCRIPTION Description;
  PADAPTER_OBJECT    AdapterObject = 0;
  UINT               MapRegistersRequired = 0;
  UINT               MapRegistersPerBaseRegister = 0;
  ULONG              AvailableMapRegisters;
  NTSTATUS           NtStatus;
  PLOGICAL_ADAPTER   Adapter = 0;
  PDEVICE_OBJECT     DeviceObject = 0;
  KIRQL              OldIrql;

  NDIS_DbgPrint(MAX_TRACE, ("called: Handle 0x%x, DmaChannel 0x%x, DmaSize 0x%x, BaseMapRegsNeeded: 0x%x, MaxBuffer: 0x%x.\n",
                            MiniportAdapterHandle, DmaChannel, DmaSize, BaseMapRegistersNeeded, MaximumBufferSize));

  memset(&Description,0,sizeof(Description));

  Adapter = (PLOGICAL_ADAPTER)MiniportAdapterHandle;
  DeviceObject = Adapter->NdisMiniportBlock.DeviceObject;

  InitializeListHead(&Adapter->MapRegisterList.ListEntry);
  KeInitializeEvent(&Adapter->DmaEvent, NotificationEvent, FALSE);
  KeInitializeSpinLock(&Adapter->DmaLock);

  /*
  * map registers correlate to physical pages.  ndis documents a
  * maximum of 64 map registers that it will return.  
  * at 4k pages, a 1514-byte buffer can span not more than 2 pages.
  *
  * the number of registers required for a given physical mapping
  * is (first register + last register + one per page size), 
  * given that physical mapping is > 2.
  */

  /* unhandled corner case: 1-byte max buffer size */
  MapRegistersPerBaseRegister = 2 + MaximumBufferSize / PAGE_SIZE;
  MapRegistersRequired = BaseMapRegistersNeeded * MapRegistersPerBaseRegister;

  if(MapRegistersRequired > 64)
    {
      NDIS_DbgPrint(MID_TRACE, ("Request for too many map registers: %d\n", MapRegistersRequired));
      return NDIS_STATUS_RESOURCES;
    }

  Description.Version = DEVICE_DESCRIPTION_VERSION;
  Description.Master = TRUE;                         /* implied by calling this function */
  Description.ScatterGather = FALSE;                 /* implied by calling this function */
  Description.DemandMode = 0;                        /* unused due to bus master */
  Description.AutoInitialize = 0;                    /* unused due to bus master */
  Description.Dma32BitAddresses = DmaSize;          
  Description.IgnoreCount = 0;                       /* unused due to bus master */
  Description.Reserved1 = 0;
  Description.Reserved2 = 0;
  Description.BusNumber = Adapter->BusNumber;
  Description.DmaChannel = 0;                        /* unused due to bus master */
  Description.InterfaceType = Adapter->BusType;
  Description.DmaChannel = 0;                        /* unused due to bus master */
  Description.DmaWidth = 0;                          /* unused (i think) due to bus master */
  Description.DmaSpeed = 0;                          /* unused (i think) due to bus master */
  Description.MaximumLength = 0;                     /* unused (i think) due to bus master */
  Description.DmaPort = 0;                           /* unused due to bus type */

  AvailableMapRegisters = MapRegistersRequired;
  AdapterObject = HalGetAdapter(&Description, &AvailableMapRegisters);

  if(!AdapterObject)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Unable to allocate an adapter object; bailing out\n"));
      return NDIS_STATUS_RESOURCES;
    }

  Adapter->AdapterObject = AdapterObject;

  if(AvailableMapRegisters < MapRegistersRequired)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Didn't get enough map registers from hal - requested 0x%x, got 0x%x\n", 
                                MapRegistersRequired, AvailableMapRegisters));
      return NDIS_STATUS_RESOURCES;
    }

  KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

  Adapter->MapRegistersRequested = MapRegistersRequired;

  NtStatus = IoAllocateAdapterChannel(AdapterObject, DeviceObject, 
      MapRegistersRequired, NdisMapRegisterCallback, Adapter);

  KeLowerIrql(OldIrql);

  if(!NT_SUCCESS(NtStatus))
    {
      NDIS_DbgPrint(MIN_TRACE, ("IoAllocateAdapterChannel failed: 0x%x\n", NtStatus));
      return NDIS_STATUS_RESOURCES;
    }

  NtStatus = KeWaitForSingleObject(&Adapter->DmaEvent, Executive, KernelMode, FALSE, 0);

  if(!NT_SUCCESS(NtStatus))
    {
      NDIS_DbgPrint(MIN_TRACE, ("KeWaitForSingleObject failed: 0x%x\n", NtStatus));
      return NDIS_STATUS_RESOURCES;
    }

  NDIS_DbgPrint(MAX_TRACE, ("returning success\n"));
  return NDIS_STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMCompleteDmaTransfer(
    OUT PNDIS_STATUS    Status,
    IN  PNDIS_HANDLE    MiniportDmaHandle,
    IN  PNDIS_BUFFER    Buffer,
    IN  ULONG           Offset,
    IN  ULONG           Length,
    IN  BOOLEAN         WriteToDevice)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMDeregisterDmaChannel(
    IN  PNDIS_HANDLE    MiniportDmaHandle)
{
    UNIMPLEMENTED
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
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMDeregisterIoPortRange(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  UINT        InitialPort,
    IN  UINT        NumberOfPorts,
    IN  PVOID       PortOffset)
/*
 * FUNCTION: Releases a register mapping to I/O ports
 * ARGUMENTS:
 *     MiniportAdapterHandle = Specifies handle input to MiniportInitialize
 *     InitialPort           = Bus-relative base port address of a range to be mapped
 *     NumberOfPorts         = Specifies number of ports to be mapped
 *     PortOffset            = Pointer to mapped base port address
 */
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
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
  KIRQL            OldIrql;
  PLOGICAL_ADAPTER Adapter = (PLOGICAL_ADAPTER)MiniportAdapterHandle;
  PADAPTER_OBJECT  AdapterObject = Adapter->AdapterObject;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

  while(!IsListEmpty(&Adapter->MapRegisterList.ListEntry))
    {
      PADAPTER_MAP_REGISTER_LIST Register = (PADAPTER_MAP_REGISTER_LIST)RemoveTailList(&Adapter->MapRegisterList.ListEntry);
      if(Register)
        {
          IoFreeMapRegisters(AdapterObject, Register->MapRegister, Register->NumRegisters);
          ExFreePool(Register);
        }
      else
        NDIS_DbgPrint(MIN_TRACE,("Internal NDIS error - Register is 0\n"));
    }

 KeLowerIrql(OldIrql);
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMMapIoSpace(
    OUT PVOID                   *VirtualAddress,
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress,
    IN  UINT                    Length)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
ULONG
EXPORT
NdisMReadDmaCounter(
    IN  NDIS_HANDLE MiniportDmaHandle)
{
    UNIMPLEMENTED

	return 0;
}


/*
 * @unimplemented
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
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
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
    PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(MiniportAdapterHandle);

    NDIS_DbgPrint(MAX_TRACE, ("Called. InterruptVector (0x%X)  InterruptLevel (0x%X)  "
        "SharedInterrupt (%d)  InterruptMode (0x%X)\n",
        InterruptVector, InterruptLevel, SharedInterrupt, InterruptMode));

    RtlZeroMemory(Interrupt, sizeof(NDIS_MINIPORT_INTERRUPT));

    KeInitializeSpinLock(&Interrupt->DpcCountLock);

    KeInitializeDpc(&Interrupt->InterruptDpc,
                    HandleDeferredProcessing,
                    Adapter);

    KeInitializeEvent(&Interrupt->DpcsCompletedEvent,
                      NotificationEvent,
                      FALSE);

    Interrupt->SharedInterrupt = SharedInterrupt;

    Adapter->NdisMiniportBlock.Interrupt = Interrupt;

    MappedIRQ = HalGetInterruptVector(Adapter->BusType,
                                      Adapter->BusNumber,
                                      InterruptLevel,
                                      InterruptVector,
                                      &DIrql,
                                      &Affinity);

    NDIS_DbgPrint(MAX_TRACE, ("Connecting to interrupt vector (0x%X)  Affinity (0x%X).\n", MappedIRQ, Affinity));

    Status = IoConnectInterrupt(&Interrupt->InterruptObject,
                                ServiceRoutine,
                                Adapter,
                                &Interrupt->DpcCountLock,
                                MappedIRQ,
                                DIrql,
                                DIrql,
                                InterruptMode,
                                SharedInterrupt,
                                Affinity,
                                FALSE);

    NDIS_DbgPrint(MAX_TRACE, ("Leaving. Status (0x%X).\n", Status));

    if (NT_SUCCESS(Status))
        return NDIS_STATUS_SUCCESS;

    if (Status == STATUS_INSUFFICIENT_RESOURCES) {
        /* FIXME: Log error */
        return NDIS_STATUS_RESOURCE_CONFLICT;
    }

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
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
  PHYSICAL_ADDRESS PortAddress, TranslatedAddress;
  PLOGICAL_ADAPTER Adapter  = GET_LOGICAL_ADAPTER(MiniportAdapterHandle);
  ULONG            AddressSpace = 1;    /* FIXME The HAL handles this wrong atm */

  NDIS_DbgPrint(MAX_TRACE, ("Called - InitialPort 0x%x, NumberOfPorts 0x%x\n", InitialPort, NumberOfPorts));

  memset(&PortAddress, 0, sizeof(PortAddress));

  /* this might be a hack - ndis5 miniports seem to specify 0 */
  if(InitialPort)
      PortAddress = RtlConvertUlongToLargeInteger(InitialPort);
  else
      PortAddress = Adapter->BaseIoAddress;

  NDIS_DbgPrint(MAX_TRACE, ("Translating address 0x%x 0x%x\n", PortAddress.u.HighPart, PortAddress.u.LowPart));

  /* FIXME: hard-coded bus number */
  if(!HalTranslateBusAddress(Adapter->BusType, 0, PortAddress, &AddressSpace, &TranslatedAddress))
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

  *PortOffset = 0;
  *PortOffset = MmMapIoSpace(TranslatedAddress, NumberOfPorts, 0);
  NDIS_DbgPrint(MAX_TRACE, ("Returning 0x%x for port range\n", *PortOffset));

  if(!*PortOffset)
    return NDIS_STATUS_RESOURCES;

  return NDIS_STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
VOID
EXPORT
NdisMSetupDmaTransfer(
    OUT	PNDIS_STATUS    Status,
    IN	PNDIS_HANDLE    MiniportDmaHandle,
    IN	PNDIS_BUFFER    Buffer,
    IN	ULONG           Offset,
    IN	ULONG           Length,
    IN	BOOLEAN         WriteToDevice)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMUnmapIoSpace(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  PVOID       VirtualAddress,
    IN  UINT        Length)
{
    UNIMPLEMENTED
}

/* EOF */
