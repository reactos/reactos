/*
 * PROJECT:         ReactOS Videoport
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/drivers/videoprt/dma.c
 * PURPOSE:         Videoport Direct Memory Access Support
 * PROGRAMMERS:     ...
 */

/* INCLUDES ******************************************************************/

#include <videoprt.h>

#define NDEBUG
#include <debug.h>

typedef struct
{
    LIST_ENTRY Entry;
    PDMA_ADAPTER Adapter;
    ULONG MapRegisters;
    PVOID HwDeviceExtension;

}VIP_DMA_ADAPTER, *PVIP_DMA_ADAPTER;

typedef struct
{
    PVOID HwDeviceExtension;
    PSCATTER_GATHER_LIST  ScatterGatherList;
    PEXECUTE_DMA ExecuteDmaRoutine;
    PVOID Context;
    PVP_DMA_ADAPTER VpDmaAdapter;

}DMA_START_CONTEXT, *PDMA_START_CONTEXT;


/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
PVOID
NTAPI
VideoPortAllocateCommonBuffer(IN PVOID HwDeviceExtension,
                              IN PVP_DMA_ADAPTER VpDmaAdapter,
                              IN ULONG DesiredLength,
                              OUT PPHYSICAL_ADDRESS LogicalAddress,
                              IN BOOLEAN CacheEnabled,
                              PVOID Reserved)
{
    PVIP_DMA_ADAPTER Adapter = (PVIP_DMA_ADAPTER)VpDmaAdapter;

    /* check for valid arguments */
    if (!Adapter || !Adapter->Adapter)
    {
        /* invalid parameter */
        return NULL;
    }

    /* allocate common buffer */
    return Adapter->Adapter->DmaOperations->AllocateCommonBuffer(Adapter->Adapter, DesiredLength, LogicalAddress, CacheEnabled);
}

/*
 * @implemented
 */
VOID
NTAPI
VideoPortReleaseCommonBuffer(IN PVOID HwDeviceExtension,
                             IN PVP_DMA_ADAPTER VpDmaAdapter,
                             IN ULONG Length,
                             IN PHYSICAL_ADDRESS LogicalAddress,
                             IN PVOID VirtualAddress,
                             IN BOOLEAN CacheEnabled)
{
    PVIP_DMA_ADAPTER Adapter = (PVIP_DMA_ADAPTER)VpDmaAdapter;

    /* check for valid arguments */
    if (!Adapter || !Adapter->Adapter)
    {
        /* invalid parameter */
        return;
    }

    /* release common buffer */
    Adapter->Adapter->DmaOperations->FreeCommonBuffer(Adapter->Adapter, Length, LogicalAddress, VirtualAddress, CacheEnabled);
}

/*
 * @implemented
 */
VOID
NTAPI
VideoPortPutDmaAdapter(IN PVOID HwDeviceExtension,
                       IN PVP_DMA_ADAPTER VpDmaAdapter)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    PVIP_DMA_ADAPTER Adapter = (PVIP_DMA_ADAPTER)VpDmaAdapter;

    /* get hw device extension */
    DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

    /* sanity check */
    ASSERT(!IsListEmpty(&DeviceExtension->DmaAdapterList));

    /* remove dma adapter from list */
    RemoveEntryList(&Adapter->Entry);

    /* release dma adapter */
    Adapter->Adapter->DmaOperations->PutDmaAdapter(Adapter->Adapter);

    /* free memory */
    ExFreePool(Adapter);
}

/*
 * @implemented
 */
PVP_DMA_ADAPTER
NTAPI
VideoPortGetDmaAdapter(IN PVOID HwDeviceExtension,
                       IN PVP_DEVICE_DESCRIPTION VpDeviceExtension)
{
    DEVICE_DESCRIPTION DeviceDescription;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
    ULONG NumberOfMapRegisters;
    PVIP_DMA_ADAPTER Adapter;
    PDMA_ADAPTER DmaAdapter;

    /* allocate private adapter structure */
    Adapter = ExAllocatePool(NonPagedPool, sizeof(VIP_DMA_ADAPTER));
    if (!Adapter)
    {
        /* failed to allocate adapter structure */
        return NULL;
    }

    /* Zero the structure */
    RtlZeroMemory(&DeviceDescription,
                  sizeof(DEVICE_DESCRIPTION));

    /* Initialize the structure */
    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.Master = TRUE;
    DeviceDescription.DmaWidth = Width8Bits;
    DeviceDescription.DmaSpeed = Compatible;

    /* Copy data from caller's device extension */
    DeviceDescription.ScatterGather = VpDeviceExtension->ScatterGather;
    DeviceDescription.Dma32BitAddresses = VpDeviceExtension->Dma32BitAddresses;
    DeviceDescription.Dma64BitAddresses = VpDeviceExtension->Dma64BitAddresses;
    DeviceDescription.MaximumLength = VpDeviceExtension->MaximumLength;

    /* Copy data from the internal device extension */
    DeviceDescription.BusNumber = DeviceExtension->SystemIoBusNumber;
    DeviceDescription.InterfaceType = DeviceExtension->AdapterInterfaceType;

    /* acquire dma adapter */
    DmaAdapter = IoGetDmaAdapter(DeviceExtension->PhysicalDeviceObject, &DeviceDescription, &NumberOfMapRegisters);
    if (!DmaAdapter)
    {
        /* failed to acquire dma */
        ExFreePool(Adapter);
        return NULL;
    }

    /* store dma adapter */
    Adapter->Adapter = DmaAdapter;

    /* store map register count */
    Adapter->MapRegisters = NumberOfMapRegisters;

    /* store hw device extension */
    Adapter->HwDeviceExtension = HwDeviceExtension;

    /* store in dma adapter list */
    InsertTailList(&DeviceExtension->DmaAdapterList, &Adapter->Entry);

    /* return result */
    return (PVP_DMA_ADAPTER)Adapter;
}

/*
 * @implemented
 */
VOID
NTAPI
VideoPortFreeCommonBuffer(IN PVOID HwDeviceExtension,
                          IN ULONG Length,
                          IN PVOID VirtualAddress,
                          IN PHYSICAL_ADDRESS LogicalAddress,
                          IN BOOLEAN CacheEnabled)
{
    PVIP_DMA_ADAPTER VpDmaAdapter;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

    /* sanity check */
    ASSERT(!IsListEmpty(&DeviceExtension->DmaAdapterList));

    /* grab first dma adapter */
    VpDmaAdapter = (PVIP_DMA_ADAPTER)CONTAINING_RECORD(DeviceExtension->DmaAdapterList.Flink, VIP_DMA_ADAPTER, Entry);

    /* sanity checks */
    ASSERT(VpDmaAdapter->HwDeviceExtension == HwDeviceExtension);
    ASSERT(VpDmaAdapter->Adapter != NULL);
    ASSERT(VpDmaAdapter->MapRegisters != 0);

    VideoPortReleaseCommonBuffer(HwDeviceExtension, (PVP_DMA_ADAPTER)VpDmaAdapter, Length, LogicalAddress, VirtualAddress, CacheEnabled);
}

/*
 * @implemented
 */
PVOID
NTAPI
VideoPortGetCommonBuffer(IN PVOID HwDeviceExtension,
                         IN ULONG DesiredLength,
                         IN ULONG Alignment,
                         OUT PPHYSICAL_ADDRESS LogicalAddress,
                         OUT PULONG pActualLength,
                         IN BOOLEAN CacheEnabled)
{
    PVOID Result;
    PVIP_DMA_ADAPTER VpDmaAdapter;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

    /* maximum palette size */
    if (DesiredLength > 262144)
    {
        /* size exceeded */
        return NULL;
    }

    if (IsListEmpty(&DeviceExtension->DmaAdapterList))
    {
        /* no adapter available */
        return NULL;
    }

    /* grab first dma adapter */
    VpDmaAdapter = (PVIP_DMA_ADAPTER)CONTAINING_RECORD(DeviceExtension->DmaAdapterList.Flink, VIP_DMA_ADAPTER, Entry);

    /* sanity checks */
    ASSERT(VpDmaAdapter->HwDeviceExtension == HwDeviceExtension);
    ASSERT(VpDmaAdapter->Adapter != NULL);
    ASSERT(VpDmaAdapter->MapRegisters != 0);

    /* allocate common buffer */
    Result = VideoPortAllocateCommonBuffer(HwDeviceExtension, (PVP_DMA_ADAPTER)VpDmaAdapter, DesiredLength, LogicalAddress, CacheEnabled, NULL);

    if (Result)
    {
        /* store length */
        *pActualLength = DesiredLength;
    }
    else
    {
        /* failed to allocate common buffer */
        *pActualLength = 0;
    }

    return Result;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
VideoPortUnmapDmaMemory(
    PVOID  HwDeviceExtension,
    PVOID  VirtualAddress,
    HANDLE  ProcessHandle,
    PDMA  BoardMemoryHandle)
{
    /* Deprecated */
    return FALSE;
}

/*
 * @implemented
 */
PDMA
NTAPI
VideoPortMapDmaMemory(IN PVOID HwDeviceExtension,
                      IN PVIDEO_REQUEST_PACKET pVrp,
                      IN PHYSICAL_ADDRESS BoardAddress,
                      IN PULONG Length,
                      IN PULONG InIoSpace,
                      IN PVOID MappedUserEvent,
                      IN PVOID DisplayDriverEvent,
                      IN OUT PVOID *VirtualAddress)
{
    /* Deprecated */
    return NULL;
}

/*
 * @implemented
 */
VOID
NTAPI
VideoPortSetDmaContext(IN PVOID HwDeviceExtension,
                       OUT PDMA pDma,
                       IN PVOID InstanceContext)
{
    /* Deprecated */
    return;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
VideoPortSignalDmaComplete(IN PVOID HwDeviceExtension,
                           IN PDMA pDmaHandle)
{
    /* Deprecated */
    return FALSE;
}


BOOLEAN
NTAPI
SyncScatterRoutine(
    IN PVOID  Context)
{
    PDMA_START_CONTEXT StartContext = (PDMA_START_CONTEXT)Context;

    StartContext->ExecuteDmaRoutine(StartContext->HwDeviceExtension, StartContext->VpDmaAdapter, (PVP_SCATTER_GATHER_LIST)StartContext->ScatterGatherList, StartContext->Context);
    return TRUE;
}

VOID
NTAPI
ScatterAdapterControl(
    IN PDEVICE_OBJECT  *DeviceObject,
    IN PIRP  *Irp,
    IN PSCATTER_GATHER_LIST  ScatterGather,
    IN PVOID  Context)
{
    PDMA_START_CONTEXT StartContext = (PDMA_START_CONTEXT)Context;

    StartContext->ScatterGatherList = ScatterGather;

    VideoPortSynchronizeExecution(StartContext->HwDeviceExtension, VpMediumPriority, SyncScatterRoutine, StartContext);
    ExFreePool(StartContext);
}

/*
 * @implemented
 */
VP_STATUS
NTAPI
VideoPortStartDma(IN PVOID HwDeviceExtension,
                  IN PVP_DMA_ADAPTER VpDmaAdapter,
                  IN PVOID Mdl,
                  IN ULONG Offset,
                  IN OUT PULONG pLength,
                  IN PEXECUTE_DMA ExecuteDmaRoutine,
                  IN PVOID Context,
                  IN BOOLEAN WriteToDevice)
{
    NTSTATUS Status;
    KIRQL OldIrql;
    PDMA_START_CONTEXT StartContext;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
    PVIP_DMA_ADAPTER Adapter = (PVIP_DMA_ADAPTER)VpDmaAdapter;

    StartContext = ExAllocatePool(NonPagedPool, sizeof(DMA_START_CONTEXT));
    if (!StartContext)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    StartContext->Context = Context;
    StartContext->ExecuteDmaRoutine = ExecuteDmaRoutine;
    StartContext->HwDeviceExtension = HwDeviceExtension;
    StartContext->VpDmaAdapter = VpDmaAdapter;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    Status = Adapter->Adapter->DmaOperations->GetScatterGatherList(Adapter->Adapter,
                                                                   DeviceExtension->PhysicalDeviceObject,
                                                                   Mdl,
                                                                   MmGetSystemAddressForMdl((PMDL)Mdl),
                                                                   MmGetMdlByteCount((PMDL)Mdl),
                                                                   (PDRIVER_LIST_CONTROL)ScatterAdapterControl,
                                                                   StartContext,
                                                                   WriteToDevice);

    KeLowerIrql(OldIrql);

    if (!NT_SUCCESS(Status))
    {
        *pLength = 0;
        ExFreePool(StartContext);
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        Status = NO_ERROR;
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
PVOID
NTAPI
VideoPortGetDmaContext(IN PVOID HwDeviceExtension,
                       IN PDMA pDma)
{
    /* Deprecated */
    return NULL;
}

/*
 * @implemented
 */
PDMA
NTAPI
VideoPortDoDma(IN PVOID HwDeviceExtension,
               IN PDMA pDma,
               IN DMA_FLAGS DmaFlags)
{
    /* Deprecated */
    return NULL;
}

/*
 * @implemented
 */
PDMA
NTAPI
VideoPortAssociateEventsWithDmaHandle(IN PVOID HwDeviceExtension,
                                      IN OUT PVIDEO_REQUEST_PACKET pVrp,
                                      IN PVOID MappedUserEvent,
                                      IN PVOID DisplayDriverEvent)
{
    /* Deprecated */
    return NULL;
}

/*
 * @implemented
 */
VP_STATUS
NTAPI
VideoPortCompleteDma(IN PVOID HwDeviceExtension,
                     IN PVP_DMA_ADAPTER VpDmaAdapter,
                     IN PVP_SCATTER_GATHER_LIST VpScatterGather,
                     IN BOOLEAN WriteToDevice)
{
    KIRQL OldIrql;
    PVIP_DMA_ADAPTER Adapter = (PVIP_DMA_ADAPTER)VpDmaAdapter;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    Adapter->Adapter->DmaOperations->PutScatterGatherList(Adapter->Adapter, (PSCATTER_GATHER_LIST)VpScatterGather, WriteToDevice);
    KeLowerIrql(OldIrql);

    return NO_ERROR;
}
