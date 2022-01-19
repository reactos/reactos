/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/stream/pnp.c
 * PURPOSE:         pnp handling
 * PROGRAMMER:      Johannes Anderwald
 */

#include "stream.h"

VOID
CompleteIrp(
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG_PTR Information)
{
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

VOID
NTAPI
StreamClassReleaseResources(
    IN  PDEVICE_OBJECT DeviceObject)
{
    PSTREAM_DEVICE_EXTENSION DeviceExtension;
    PLIST_ENTRY Entry;
    PMEMORY_RESOURCE_LIST Mem;

    /* Get device extension */
    DeviceExtension = (PSTREAM_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* Disconnect interrupt */
    if (DeviceExtension->Interrupt)
    {
        IoDisconnectInterrupt(DeviceExtension->Interrupt);
        DeviceExtension->Interrupt = NULL;
    }

    /* Release DmaAdapter */
    if (DeviceExtension->DmaAdapter)
    {
        DeviceExtension->DmaAdapter->DmaOperations->PutDmaAdapter(DeviceExtension->DmaAdapter);
        DeviceExtension->DmaAdapter = NULL;
    }

    /* Release mem mapped I/O */
    while(!IsListEmpty(&DeviceExtension->MemoryResourceList))
    {
        Entry = RemoveHeadList(&DeviceExtension->MemoryResourceList);
        Mem = (PMEMORY_RESOURCE_LIST)CONTAINING_RECORD(Entry, MEMORY_RESOURCE_LIST, Entry);

        MmUnmapIoSpace(Mem->Start, Mem->Length);
        ExFreePool(Entry);
    }
}

BOOLEAN
NTAPI
StreamClassSynchronize(
    IN PKINTERRUPT  Interrupt,
    IN PKSYNCHRONIZE_ROUTINE  SynchronizeRoutine,
    IN PVOID  SynchronizeContext)
{
    /* This function is used when the driver either implements synchronization on its own
     * or if there is no interrupt assigned
     */
    return SynchronizeRoutine(SynchronizeContext);
}

VOID
NTAPI
StreamClassInterruptDpc(
    IN PKDPC Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2)
{
    //TODO
    //read/write data
}


BOOLEAN
NTAPI
StreamClassInterruptRoutine(
    IN PKINTERRUPT  Interrupt,
    IN PVOID  ServiceContext)
{
    BOOLEAN Ret = FALSE;
    PSTREAM_DEVICE_EXTENSION DeviceExtension = (PSTREAM_DEVICE_EXTENSION)ServiceContext;

    /* Does the driver implement HwInterrupt routine */
    if (DeviceExtension->DriverExtension->Data.HwInterrupt)
    {
        /* Check if the interrupt was coming from this device */
        Ret = DeviceExtension->DriverExtension->Data.HwInterrupt(DeviceExtension->DeviceExtension);
        if (Ret)
        {
            /* Interrupt has from this device, schedule a Dpc for us */
            KeInsertQueueDpc(&DeviceExtension->InterruptDpc, NULL, NULL);
        }
    }
    /* Return result */
    return Ret;
}



NTSTATUS
NTAPI
StreamClassStartDevice(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PHW_STREAM_REQUEST_BLOCK_EXT RequestBlock;
    PPORT_CONFIGURATION_INFORMATION Config;
    PSTREAM_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IoStack;
    PCM_RESOURCE_LIST List;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    PSTREAM_CLASS_DRIVER_EXTENSION DriverObjectExtension;
    PDMA_ADAPTER Adapter;
    DEVICE_DESCRIPTION DeviceDesc;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ResultLength, Index;
    BOOLEAN bUseDMA, bUseInterrupt;
    ULONG MapRegisters;
    KAFFINITY Affinity = 0;
    PHW_STREAM_DESCRIPTOR StreamDescriptor;
    PACCESS_RANGE Range;
    PVOID MappedAddr;
    PMEMORY_RESOURCE_LIST Mem;

    /* Get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* Get resource list */
    List = IoStack->Parameters.StartDevice.AllocatedResourcesTranslated;
    /* Calculate request length */
    ResultLength = sizeof(HW_STREAM_REQUEST_BLOCK_EXT) + sizeof(PPORT_CONFIGURATION_INFORMATION) + List->List[0].PartialResourceList.Count * sizeof(ACCESS_RANGE);

    /* Allocate Request Block */
    RequestBlock = ExAllocatePool(NonPagedPool, ResultLength);

    if (!RequestBlock)
    {
        /* Not enough memory */
        CompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES, 0);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Get device extension */
    DeviceExtension = (PSTREAM_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* Get driver object extension */
    DriverObjectExtension = IoGetDriverObjectExtension(DeviceObject->DriverObject, (PVOID)StreamClassAddDevice);

    /* sanity checks */
    ASSERT(DeviceExtension);
    ASSERT(DriverObjectExtension);

    /* Zero request block */
    RtlZeroMemory(RequestBlock, ResultLength);

    /* Locate Config struct */
    Config = (PPORT_CONFIGURATION_INFORMATION) (RequestBlock + 1);
    Range = (PACCESS_RANGE) (Config + 1);

    /* Initialize Request */
    RequestBlock->Block.SizeOfThisPacket = sizeof(HW_STREAM_REQUEST_BLOCK);
    RequestBlock->Block.Command = SRB_INITIALIZE_DEVICE;
    RequestBlock->Block.CommandData.ConfigInfo = Config;
    KeInitializeEvent(&RequestBlock->Event, SynchronizationEvent, FALSE);

    Config->SizeOfThisPacket =  sizeof(PPORT_CONFIGURATION_INFORMATION);
    Config->HwDeviceExtension  = (PVOID) (DeviceExtension + 1);
    Config->ClassDeviceObject = DeviceObject;
    Config->PhysicalDeviceObject = DeviceExtension->LowerDeviceObject;
    Config->RealPhysicalDeviceObject = DeviceExtension->PhysicalDeviceObject;
    Config->AccessRanges = Range;

    IoGetDeviceProperty(DeviceObject, DevicePropertyBusNumber, sizeof(ULONG), (PVOID)&Config->SystemIoBusNumber, &ResultLength);
    IoGetDeviceProperty(DeviceObject, DevicePropertyLegacyBusType, sizeof(INTERFACE_TYPE), (PVOID)&Config->AdapterInterfaceType, &ResultLength);

    /* Get resource list */
    List = IoStack->Parameters.StartDevice.AllocatedResourcesTranslated;

    /* Scan the translated resources */
    bUseDMA = FALSE;
    bUseInterrupt = FALSE;

    Range = (PACCESS_RANGE) (Config + 1);

    for(Index = 0; Index < List->List[0].PartialResourceList.Count; Index++)
    {
        /* Locate partial descriptor */
        Descriptor = &List->List[0].PartialResourceList.PartialDescriptors[Index];

        switch(Descriptor->Type)
        {
            case CmResourceTypePort:
            {
                /* Store resource information in AccessRange struct */
                Range[Config->NumberOfAccessRanges].RangeLength = Descriptor->u.Port.Length;
                Range[Config->NumberOfAccessRanges].RangeStart.QuadPart = Descriptor->u.Port.Start.QuadPart;
                Range[Config->NumberOfAccessRanges].RangeInMemory = FALSE;
                Config->NumberOfAccessRanges++;
                break;
            }
            case CmResourceTypeInterrupt:
            {
                /* Store resource information */
                Config->BusInterruptLevel = Descriptor->u.Interrupt.Level;
                Config->BusInterruptVector = Descriptor->u.Interrupt.Vector;
                Config->InterruptMode = Descriptor->Flags;
                Affinity = Descriptor->u.Interrupt.Affinity;
                bUseInterrupt = TRUE;
                break;
            }
            case CmResourceTypeMemory:
            {
                Mem = ExAllocatePool(NonPagedPool, sizeof(MEMORY_RESOURCE_LIST));
                MappedAddr = MmMapIoSpace(Descriptor->u.Memory.Start, Descriptor->u.Memory.Length, MmNonCached);
                if (!MappedAddr || !Mem)
                {
                    if (Mem)
                    {
                        /* Release Memory resource descriptor */
                        ExFreePool(Mem);
                    }

                    if (MappedAddr)
                    {
                        /* Release mem mapped I/O */
                        MmUnmapIoSpace(MappedAddr, Descriptor->u.Memory.Length);
                    }

                    /* Release resources */
                    StreamClassReleaseResources(DeviceObject);
                    /* Complete irp */
                    CompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES, 0);
                    ExFreePool(RequestBlock);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                /* Store range for driver */
                Range[Config->NumberOfAccessRanges].RangeLength = Descriptor->u.Memory.Length;
                Range[Config->NumberOfAccessRanges].RangeStart.QuadPart = Descriptor->u.Memory.Start.QuadPart;
                Range[Config->NumberOfAccessRanges].RangeInMemory = TRUE;
                Config->NumberOfAccessRanges++;
                /* Initialize Memory resource descriptor */
                Mem->Length = Descriptor->u.Memory.Length;
                Mem->Start = MappedAddr;
                InsertTailList(&DeviceExtension->MemoryResourceList, &Mem->Entry);
                break;
            }
            case CmResourceTypeDma:
            {
                bUseDMA = TRUE;
                Config->DmaChannel = Descriptor->u.Dma.Channel;
                break;
            }
        }
    }

    if (!bUseInterrupt || DriverObjectExtension->Data.HwInterrupt == NULL || Config->BusInterruptLevel == 0 || Config->BusInterruptVector == 0)
    {
        /* requirements not satisfied */
        DeviceExtension->SynchronizeFunction = StreamClassSynchronize;
    }
    else
    {
        /* use real sync routine */
        DeviceExtension->SynchronizeFunction = KeSynchronizeExecution;

        /* connect interrupt */
        Status = IoConnectInterrupt(&DeviceExtension->Interrupt,
                                    StreamClassInterruptRoutine,
                                    (PVOID)DeviceExtension,
                                    NULL,
                                    Config->BusInterruptVector,
                                    Config->BusInterruptLevel,
                                    Config->BusInterruptLevel,
                                    Config->InterruptMode,
                                    TRUE,
                                    Affinity,
                                    FALSE);
        if (!NT_SUCCESS(Status))
        {
            /* Release resources */
            StreamClassReleaseResources(DeviceObject);
            /* Failed to connect interrupt */
            CompleteIrp(Irp, Status, 0);
            /* Release request block */
            ExFreePool(RequestBlock);
            return Status;
        }

        /* store interrupt object */
        Config->InterruptObject = DeviceExtension->Interrupt;
    }

    /* does the device use DMA */
    if (bUseDMA && DriverObjectExtension->Data.BusMasterDMA)
    {
        /* Zero device description */
        RtlZeroMemory(&DeviceDesc, sizeof(DEVICE_DESCRIPTION));

        DeviceDesc.Version = DEVICE_DESCRIPTION_VERSION;
        DeviceDesc.Master = TRUE;
        DeviceDesc.ScatterGather = TRUE;
        DeviceDesc.AutoInitialize = FALSE;
        DeviceDesc.DmaChannel = Config->DmaChannel;
        DeviceDesc.InterfaceType = Config->AdapterInterfaceType;
        DeviceDesc.DmaWidth = Width32Bits;
        DeviceDesc.DmaSpeed = Compatible;
        DeviceDesc.MaximumLength = MAXULONG;
        DeviceDesc.Dma32BitAddresses = DriverObjectExtension->Data.Dma24BitAddresses;

        Adapter = IoGetDmaAdapter(DeviceExtension->PhysicalDeviceObject, &DeviceDesc, &MapRegisters);
        if (!Adapter)
        {
            /* Failed to claim DMA Adapter */
            CompleteIrp(Irp, Status, 0);
            /* Release resources */
            StreamClassReleaseResources(DeviceObject);
            /* Release request block */
            ExFreePool(RequestBlock);
            return Status;
        }

        if (DeviceExtension->DriverExtension->Data.DmaBufferSize)
        {
            DeviceExtension->DmaCommonBuffer = Adapter->DmaOperations->AllocateCommonBuffer(Adapter, DeviceExtension->DriverExtension->Data.DmaBufferSize, &DeviceExtension->DmaPhysicalAddress, FALSE);
            if (!DeviceExtension->DmaCommonBuffer)
            {
                /* Failed to allocate a common buffer */
                CompleteIrp(Irp, Status, 0);
                /* Release resources */
                StreamClassReleaseResources(DeviceObject);
                /* Release request block */
                ExFreePool(RequestBlock);
                return Status;
            }
        }


        DeviceExtension->MapRegisters = MapRegisters;
        DeviceExtension->DmaAdapter = Adapter;
        Config->DmaAdapterObject = (PADAPTER_OBJECT)Adapter;
    }


    /* First forward the request to lower attached device object */
    if (IoForwardIrpSynchronously(DeviceExtension->LowerDeviceObject, Irp))
    {
        Status = Irp->IoStatus.Status;
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(Status))
    {
        /* Failed to start lower devices */
        CompleteIrp(Irp, Status, 0);
        /* Release resources */
        StreamClassReleaseResources(DeviceObject);
        /* Release request block */
        ExFreePool(RequestBlock);
        return Status;
    }

    Config->Irp = Irp;

    /* FIXME SYNCHRONIZATION */

    /* Send the request */
    DriverObjectExtension->Data.HwReceivePacket((PHW_STREAM_REQUEST_BLOCK)RequestBlock);
    if (RequestBlock->Block.Status == STATUS_PENDING)
    {
        /* Request is pending, wait for result */
        KeWaitForSingleObject(&RequestBlock->Event, Executive, KernelMode, FALSE, NULL);
        /* Get final status code */
        Status = RequestBlock->Block.Status;
    }

    /* Copy stream descriptor size */
    DeviceExtension->StreamDescriptorSize = Config->StreamDescriptorSize;

    /* check if the request has succeeded or if stream size is valid*/
    if (!NT_SUCCESS(Status)|| !Config->StreamDescriptorSize)
    {
        /* Failed to start device */
        CompleteIrp(Irp, Status, 0);
        /* Release resources */
        StreamClassReleaseResources(DeviceObject);
        /* Release request block */
        ExFreePool(RequestBlock);
        return Status;
    }

    /* Allocate a stream Descriptor */
    StreamDescriptor = ExAllocatePool(NonPagedPool, DeviceExtension->StreamDescriptorSize);
    if (!StreamDescriptor)
    {
        /* Not enough memory */
        CompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES, 0);
        /* Release resources */
        StreamClassReleaseResources(DeviceObject);
        /* Release request block */
        ExFreePool(RequestBlock);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Zero stream descriptor */
    RtlZeroMemory(StreamDescriptor, DeviceExtension->StreamDescriptorSize);

    /* Setup get stream info struct */
    RequestBlock->Block.Command = SRB_GET_STREAM_INFO;
    RequestBlock->Block.CommandData.StreamBuffer = StreamDescriptor;
    KeClearEvent(&RequestBlock->Event);

    /* send the request */
    DriverObjectExtension->Data.HwReceivePacket((PHW_STREAM_REQUEST_BLOCK)RequestBlock);
    if (RequestBlock->Block.Status == STATUS_PENDING)
    {
        /* Request is pending, wait for result */
        KeWaitForSingleObject(&RequestBlock->Event, Executive, KernelMode, FALSE, NULL);
        /* Get final status code */
        Status = RequestBlock->Block.Status;
    }

    if (NT_SUCCESS(Status))
    {
        /* store stream descriptor */
        DeviceExtension->StreamDescriptor = StreamDescriptor;
    }
    else
    {
        /* cleanup resources */
        ExFreePool(StreamDescriptor);
    }

    ExFreePool(RequestBlock);
    /* Complete Irp */
    CompleteIrp(Irp, Status, 0);
    /* Return result */
    return Status;
}

NTSTATUS
NTAPI
StreamClassPnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    /* Get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IoStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            return StreamClassStartDevice(DeviceObject, Irp);
        }
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_SUPPORTED;
}


