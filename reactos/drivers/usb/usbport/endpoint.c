#include "usbport.h"

#define NDEBUG
#include <debug.h>

#define NDEBUG_USBPORT_CORE
#include "usbdebug.h"

ULONG
NTAPI
USBPORT_CalculateUsbBandwidth(IN PDEVICE_OBJECT FdoDevice,
                              IN PUSBPORT_ENDPOINT Endpoint)
{
    PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties;
    ULONG Bandwidth;
    ULONG Additional;

    DPRINT("USBPORT_CalculateUsbBandwidth ... \n");

    EndpointProperties = &Endpoint->EndpointProperties;

    switch (EndpointProperties->TransferType)
    {
        case USBPORT_TRANSFER_TYPE_ISOCHRONOUS:
            Additional = 9;
            break;

        case USBPORT_TRANSFER_TYPE_INTERRUPT:
            Additional = 13;
            break;

        default: //USBPORT_TRANSFER_TYPE_CONTROL or USBPORT_TRANSFER_TYPE_BULK
            Additional = 0;
            break;
    }

    if (Additional == 0)
    {
        Bandwidth = 0;
    }
    else
    {
        Bandwidth = (EndpointProperties->TotalMaxPacketSize + Additional) * 8 * 7 / 6;
    }

    if (EndpointProperties->DeviceSpeed == UsbLowSpeed)
    {
        Bandwidth *= 8;
    }

    return Bandwidth;
}

BOOLEAN
NTAPI
USBPORT_AllocateBandwidth(IN PDEVICE_OBJECT FdoDevice,
                          IN PUSBPORT_ENDPOINT Endpoint)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties;
    ULONG TransferType;
    ULONG TotalBusBandwidth;
    ULONG EndpointBandwidth;
    ULONG Period;

    DPRINT("USBPORT_AllocateBandwidth: ... \n");

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;
    EndpointProperties = &Endpoint->EndpointProperties;
    TransferType = EndpointProperties->TransferType;

    if (TransferType == USBPORT_TRANSFER_TYPE_BULK || 
        TransferType == USBPORT_TRANSFER_TYPE_CONTROL ||
        Endpoint->Flags & ENDPOINT_FLAG_ROOTHUB_EP0)
    {
        EndpointProperties->ScheduleOffset = 0;
        return TRUE;
    }

    TotalBusBandwidth = FdoExtension->TotalBusBandwidth;
    EndpointBandwidth = EndpointProperties->UsbBandwidth;
    Period = EndpointProperties->Period;

    DPRINT1("USBPORT_AllocateBandwidth: FIXME. \n");
    DPRINT1("USBPORT_AllocateBandwidth: Endpoint - %p, Type - %x, TotalBandwidth - %x, EpBandwidth - %x, Period - %x\n",
           Endpoint,
           TransferType,
           TotalBusBandwidth,
           EndpointBandwidth,
           Period);

    return TRUE;
}

VOID
NTAPI
USBPORT_FreeBandwidth(IN PDEVICE_OBJECT FdoDevice,
                      IN PUSBPORT_ENDPOINT Endpoint)
{
    DPRINT1("USBPORT_FreeBandwidth: UNIMPLEMENTED. FIXME. \n");
}

UCHAR
NTAPI
USBPORT_NormalizeHsInterval(UCHAR Interval)
{
    UCHAR interval;

    DPRINT("USBPORT_NormalizeHsInterval: Interval - %x\n", Interval);

    interval = Interval;

    if (Interval)
       interval = Interval - 1;

    if (interval > 5)
       interval = 5;

    return 1 << interval;
}

BOOLEAN
NTAPI
USBPORT_EndpointHasQueuedTransfers(IN PDEVICE_OBJECT FdoDevice,
                                   IN PUSBPORT_ENDPOINT Endpoint,
                                   IN PULONG TransferCount)
{
    PLIST_ENTRY Entry;
    PUSBPORT_TRANSFER Transfer;
    BOOLEAN Result = FALSE;

    DPRINT_CORE("USBPORT_EndpointHasQueuedTransfers: ... \n");

    KeAcquireSpinLock(&Endpoint->EndpointSpinLock, &Endpoint->EndpointOldIrql);

    if (!IsListEmpty(&Endpoint->PendingTransferList))
        Result = TRUE;

    if (!IsListEmpty(&Endpoint->TransferList))
    {
        Result = TRUE;

        if (TransferCount)
        {
            *TransferCount = 0;

            for (Entry = Endpoint->TransferList.Flink;
                 Entry && Entry != &Endpoint->TransferList;
                 Entry = Transfer->TransferLink.Flink)
            {
                Transfer = CONTAINING_RECORD(Entry,
                                             USBPORT_TRANSFER,
                                             TransferLink);

                if (Transfer->Flags & TRANSFER_FLAG_SUBMITED)
                {
                    ++*TransferCount;
                }
            }
        }
    }

    KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);

    return Result;
}

VOID
NTAPI
USBPORT_NukeAllEndpoints(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION  FdoExtension;
    PLIST_ENTRY EndpointList;
    PUSBPORT_ENDPOINT Endpoint;
    KIRQL OldIrql;

    DPRINT("USBPORT_NukeAllEndpoints \n");

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

    KeAcquireSpinLock(&FdoExtension->EndpointListSpinLock, &OldIrql);

    EndpointList = FdoExtension->EndpointList.Flink;

    if (!IsListEmpty(&FdoExtension->EndpointList))
    {
        while (EndpointList && (EndpointList != &FdoExtension->EndpointList))
        {
            Endpoint = CONTAINING_RECORD(EndpointList,
                                         USBPORT_ENDPOINT,
                                         EndpointLink);

            if (!(Endpoint->Flags & ENDPOINT_FLAG_ROOTHUB_EP0))
                Endpoint->Flags |= ENDPOINT_FLAG_NUKE;

            EndpointList = Endpoint->EndpointLink.Flink;
        }
    }

    KeReleaseSpinLock(&FdoExtension->EndpointListSpinLock, OldIrql);
}

ULONG 
NTAPI
USBPORT_GetEndpointState(IN PUSBPORT_ENDPOINT Endpoint)
{
    ULONG State;

    //DPRINT("USBPORT_GetEndpointState \n");

    KeAcquireSpinLock(&Endpoint->StateChangeSpinLock, &Endpoint->EndpointStateOldIrql);

    if (Endpoint->StateLast != Endpoint->StateNext)
    {
        State = USBPORT_ENDPOINT_UNKNOWN;
    }
    else
    {
        State = Endpoint->StateLast;
    }

    KeReleaseSpinLock(&Endpoint->StateChangeSpinLock, Endpoint->EndpointStateOldIrql);

    if (State != USBPORT_ENDPOINT_ACTIVE)
    {
        DPRINT("USBPORT_GetEndpointState: Endpoint - %p, State - %x\n",
               Endpoint,
               State);
    }

    return State;
}

VOID
NTAPI
USBPORT_SetEndpointState(IN PUSBPORT_ENDPOINT Endpoint,
                         IN ULONG State)
{
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    KIRQL OldIrql;

    DPRINT("USBPORT_SetEndpointState: Endpoint - %p, State - %x\n",
           Endpoint,
           State);

    FdoDevice = Endpoint->FdoDevice;
    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    KeAcquireSpinLock(&Endpoint->StateChangeSpinLock, &Endpoint->EndpointStateOldIrql);

    if (!(Endpoint->Flags & ENDPOINT_FLAG_ROOTHUB_EP0))
    {
        if (Endpoint->Flags & ENDPOINT_FLAG_NUKE)
        {
            Endpoint->StateLast = State;
            Endpoint->StateNext = State;

            KeReleaseSpinLock(&Endpoint->StateChangeSpinLock, Endpoint->EndpointStateOldIrql);

            USBPORT_InvalidateEndpointHandler(FdoDevice,
                                              Endpoint,
                                              INVALIDATE_ENDPOINT_WORKER_THREAD);
            return;
        }

        KeReleaseSpinLock(&Endpoint->StateChangeSpinLock, Endpoint->EndpointStateOldIrql);

        KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);
        Packet->SetEndpointState(FdoExtension->MiniPortExt,
                                 (PVOID)((ULONG_PTR)Endpoint + sizeof(USBPORT_ENDPOINT)),
                                 State);
        KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);

        Endpoint->StateNext = State;

        KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);
        Endpoint->FrameNumber = Packet->Get32BitFrameNumber(FdoExtension->MiniPortExt);
        KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);

        ExInterlockedInsertTailList(&FdoExtension->EpStateChangeList,
                                    &Endpoint->StateChangeLink,
                                    &FdoExtension->EpStateChangeSpinLock);

        KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);
        Packet->InterruptNextSOF(FdoExtension->MiniPortExt);
        KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);
    }
    else
    {
        Endpoint->StateLast = State;
        Endpoint->StateNext = State;

        if (State == USBPORT_ENDPOINT_CLOSED)
        {
            KeReleaseSpinLock(&Endpoint->StateChangeSpinLock, Endpoint->EndpointStateOldIrql);

            USBPORT_InvalidateEndpointHandler(FdoDevice,
                                              Endpoint,
                                              INVALIDATE_ENDPOINT_WORKER_THREAD);
            return;
        }

        KeReleaseSpinLock(&Endpoint->StateChangeSpinLock, Endpoint->EndpointStateOldIrql);
    }
}

VOID
NTAPI
USBPORT_AddPipeHandle(IN PUSBPORT_DEVICE_HANDLE DeviceHandle,
                      IN PUSBPORT_PIPE_HANDLE PipeHandle)
{
    DPRINT("USBPORT_AddPipeHandle: DeviceHandle - %p, PipeHandle - %p\n",
           DeviceHandle,
           PipeHandle);

    InsertTailList(&DeviceHandle->PipeHandleList, &PipeHandle->PipeLink);
}

VOID
NTAPI
USBPORT_RemovePipeHandle(IN PUSBPORT_DEVICE_HANDLE DeviceHandle,
                         IN PUSBPORT_PIPE_HANDLE PipeHandle)
{
    DPRINT("USBPORT_RemovePipeHandle: PipeHandle - %p\n", PipeHandle);

    RemoveEntryList(&PipeHandle->PipeLink);

    PipeHandle->PipeLink.Flink = NULL;
    PipeHandle->PipeLink.Blink = NULL;
}

BOOLEAN
NTAPI
USBPORT_ValidatePipeHandle(IN PUSBPORT_DEVICE_HANDLE DeviceHandle,
                           IN PUSBPORT_PIPE_HANDLE PipeHandle)
{
    PLIST_ENTRY HandleList;
    BOOLEAN Result;
    PUSBPORT_PIPE_HANDLE CurrentHandle;

    //DPRINT("USBPORT_ValidatePipeHandle: DeviceHandle - %p, PipeHandle - %p\n",
    //       DeviceHandle,
    //       PipeHandle);

    HandleList = &DeviceHandle->PipeHandleList;

    Result = FALSE;

    if (!IsListEmpty(HandleList))
        HandleList = HandleList->Flink;

    if (HandleList != &DeviceHandle->PipeHandleList)
    {
        while (TRUE)
        {
            CurrentHandle = CONTAINING_RECORD(HandleList,
                                              USBPORT_PIPE_HANDLE,
                                              PipeLink);
      
            HandleList = HandleList->Flink;
      
            if (CurrentHandle == PipeHandle)
                break;
      
            if (HandleList == &DeviceHandle->PipeHandleList)
                return Result;
        }
    
        Result = TRUE;
    }

    return Result;
}

BOOLEAN
NTAPI
USBPORT_DeleteEndpoint(IN PDEVICE_OBJECT FdoDevice,
                       IN PUSBPORT_ENDPOINT Endpoint)
{
    PUSBPORT_DEVICE_EXTENSION  FdoExtension;
    BOOLEAN Result;
    KIRQL OldIrql;

    DPRINT("USBPORT_DeleteEndpoint: Endpoint - %p\n", Endpoint);

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

    if ((Endpoint->WorkerLink.Flink && Endpoint->WorkerLink.Blink) ||
        Endpoint->LockCounter != -1)
    {
        KeAcquireSpinLock(&FdoExtension->EndpointListSpinLock, &OldIrql);

        ExInterlockedInsertTailList(&FdoExtension->EndpointClosedList,
                                    &Endpoint->CloseLink,
                                    &FdoExtension->EndpointClosedSpinLock);

        KeReleaseSpinLock(&FdoExtension->EndpointListSpinLock, OldIrql);

        Result = FALSE;
    }
    else
    {
        KeAcquireSpinLock(&FdoExtension->EndpointListSpinLock, &OldIrql);

        RemoveEntryList(&Endpoint->EndpointLink);
        Endpoint->EndpointLink.Flink = NULL;
        Endpoint->EndpointLink.Blink = NULL;

        KeReleaseSpinLock(&FdoExtension->EndpointListSpinLock, OldIrql);

        MiniportCloseEndpoint(FdoDevice, Endpoint);

        if (Endpoint->HeaderBuffer)
        {
            USBPORT_FreeCommonBuffer(FdoDevice, Endpoint->HeaderBuffer);
        }

        ExFreePool(Endpoint);

        Result = TRUE;
    }

    return Result;
}

VOID
NTAPI
MiniportCloseEndpoint(IN PDEVICE_OBJECT FdoDevice,
                      IN PUSBPORT_ENDPOINT Endpoint)
{
    PUSBPORT_DEVICE_EXTENSION  FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    BOOLEAN IsDoDisablePeriodic;
    ULONG TransferType;
    KIRQL OldIrql;

    DPRINT("MiniportCloseEndpoint: Endpoint - %p\n", Endpoint);

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);

    if (Endpoint->Flags & ENDPOINT_FLAG_OPENED)
    {
        TransferType = Endpoint->EndpointProperties.TransferType;

        if (TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT ||
            TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
        {
            --FdoExtension->PeriodicEndpoints;
        }

        IsDoDisablePeriodic = FdoExtension->PeriodicEndpoints == 0;

        Packet->CloseEndpoint(FdoExtension->MiniPortExt,
                              (PVOID)((ULONG_PTR)Endpoint + sizeof(USBPORT_ENDPOINT)),
                              IsDoDisablePeriodic);

        Endpoint->Flags &= ~ENDPOINT_FLAG_OPENED;
        Endpoint->Flags |= ENDPOINT_FLAG_CLOSED;
    }

    KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);
}

VOID
NTAPI
USBPORT_ClosePipe(IN PUSBPORT_DEVICE_HANDLE DeviceHandle,
                  IN PDEVICE_OBJECT FdoDevice,
                  IN PUSBPORT_PIPE_HANDLE PipeHandle)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PUSBPORT_ENDPOINT Endpoint;
    BOOLEAN IsReady;
    KIRQL OldIrql;

    DPRINT("USBPORT_ClosePipe \n");

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

    if (PipeHandle->Flags & PIPE_HANDLE_FLAG_CLOSED)
        return;

    USBPORT_RemovePipeHandle(DeviceHandle, PipeHandle);

    PipeHandle->Flags |= PIPE_HANDLE_FLAG_CLOSED;

    if (PipeHandle->Flags & PIPE_HANDLE_FLAG_NULL_PACKET_SIZE)
    {
        PipeHandle->Flags &= ~PIPE_HANDLE_FLAG_NULL_PACKET_SIZE;
        return;
    }

    Endpoint = PipeHandle->Endpoint;
    DPRINT("USBPORT_ClosePipe: Endpoint - %p\n", Endpoint);

    KeAcquireSpinLock(&FdoExtension->EndpointListSpinLock, &OldIrql);

    if ((Endpoint->Flags & ENDPOINT_FLAG_ROOTHUB_EP0) &&
        (Endpoint->EndpointProperties.TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT))
    {
        PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)FdoExtension->RootHubPdo->DeviceExtension;
        PdoExtension->Endpoint = NULL;
    }

    KeReleaseSpinLock(&FdoExtension->EndpointListSpinLock, OldIrql);

    while (TRUE)
    {
        IsReady = TRUE;

        KeAcquireSpinLock(&Endpoint->EndpointSpinLock, &Endpoint->EndpointOldIrql);

        if (!IsListEmpty(&Endpoint->PendingTransferList))
            IsReady = FALSE;

        if (!IsListEmpty(&Endpoint->TransferList))
            IsReady = FALSE;

        if (!IsListEmpty(&Endpoint->CancelList))
            IsReady = FALSE;

        if (!IsListEmpty(&Endpoint->AbortList))
            IsReady = FALSE;

        KeAcquireSpinLock(&Endpoint->StateChangeSpinLock, &Endpoint->EndpointStateOldIrql);
        if (Endpoint->StateLast != Endpoint->StateNext)
            IsReady = FALSE;
        KeReleaseSpinLock(&Endpoint->StateChangeSpinLock, Endpoint->EndpointStateOldIrql);

        KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);

        if (InterlockedIncrement(&Endpoint->LockCounter))
            IsReady = FALSE;
        InterlockedDecrement(&Endpoint->LockCounter);

        if (IsReady == TRUE)
            break;

        USBPORT_Wait(FdoDevice, 1);
    }

    Endpoint->DeviceHandle = NULL;

    if (FdoExtension->MiniPortInterface->Packet.MiniPortFlags & USB_MINIPORT_FLAGS_USB2)
    {
        DPRINT("USBPORT_ClosePipe: FIXME USBPORT_FreeBandwidthUSB20\n");
        //USBPORT_FreeBandwidthUSB20();
    }
    else
    {
        DPRINT("USBPORT_ClosePipe: FIXME USBPORT_FreeBandwidthUSB11\n");
        //USBPORT_FreeBandwidthUSB11();
    }

    KeAcquireSpinLock(&Endpoint->EndpointSpinLock, &Endpoint->EndpointOldIrql);
    USBPORT_SetEndpointState(Endpoint, USBPORT_ENDPOINT_CLOSED);
    KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);

    USBPORT_SignalWorkerThread(FdoDevice);
}

MPSTATUS
NTAPI
MiniportOpenEndpoint(IN PDEVICE_OBJECT FdoDevice,
                     IN PUSBPORT_ENDPOINT Endpoint)
{
    PUSBPORT_DEVICE_EXTENSION  FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    KIRQL OldIrql;
    ULONG TransferType;
    MPSTATUS MpStatus;

    DPRINT("MiniportOpenEndpoint: Endpoint - %p\n", Endpoint);

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);

    Endpoint->Flags &= ~ENDPOINT_FLAG_CLOSED;

    MpStatus = Packet->OpenEndpoint(FdoExtension->MiniPortExt,
                                    &Endpoint->EndpointProperties,
                                    (PVOID)((ULONG_PTR)Endpoint + sizeof(USBPORT_ENDPOINT)));

    if (!MpStatus)
    {
        TransferType = Endpoint->EndpointProperties.TransferType;

        if (TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT ||
            TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
        {
            ++FdoExtension->PeriodicEndpoints;
        }

        Endpoint->Flags |= ENDPOINT_FLAG_OPENED;
    }

    KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);
    return MpStatus;
}

NTSTATUS
NTAPI
USBPORT_OpenPipe(IN PDEVICE_OBJECT FdoDevice,
                 IN PUSBPORT_DEVICE_HANDLE DeviceHandle,
                 IN PUSBPORT_PIPE_HANDLE PipeHandle,
                 IN OUT PUSBD_STATUS UsbdStatus)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    ULONG EndpointSize;
    PUSBPORT_ENDPOINT Endpoint;
    PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor;
    UCHAR Direction;
    UCHAR Interval;
    UCHAR Period;
    ULONG TransferParams[2] = {0};
    PUSBPORT_COMMON_BUFFER_HEADER HeaderBuffer;
    MPSTATUS MpStatus;
    USBD_STATUS USBDStatus;
    NTSTATUS Status;
    KIRQL OldIrql;
    USHORT MaxPacketSize;
    USHORT AdditionalTransaction;
    BOOLEAN IsAllocatedBandwidth;

    DPRINT("USBPORT_OpenPipe: DeviceHandle - %p, FdoDevice - %p, PipeHandle - %p\n",
           DeviceHandle,
           FdoDevice,
           PipeHandle);

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    EndpointSize = sizeof(USBPORT_ENDPOINT) + Packet->MiniPortEndpointSize;

    if (Packet->MiniPortFlags & USB_MINIPORT_FLAGS_USB2)
    {
        DPRINT1("USBPORT_OpenPipe: FIXME USB2 EndpointSize\n");
    }

    if (PipeHandle->EndpointDescriptor.wMaxPacketSize == 0)
    {
        USBPORT_AddPipeHandle(DeviceHandle, PipeHandle);
  
        PipeHandle->Flags = (PipeHandle->Flags & ~PIPE_HANDLE_FLAG_CLOSED) |
                             PIPE_HANDLE_FLAG_NULL_PACKET_SIZE;

        PipeHandle->Endpoint = (PUSBPORT_ENDPOINT)-1;

        return STATUS_SUCCESS;
    }

    Endpoint = ExAllocatePoolWithTag(NonPagedPool, EndpointSize, USB_PORT_TAG);

    if (!Endpoint)
    {
        DPRINT1("USBPORT_OpenPipe: Not allocated Endpoint!\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        return Status;
    }

    RtlZeroMemory(Endpoint, EndpointSize);

    Endpoint->FdoDevice = FdoDevice;
    Endpoint->DeviceHandle = (PUSBPORT_DEVICE_HANDLE)DeviceHandle;
    Endpoint->LockCounter = -1;

    KeInitializeSpinLock(&Endpoint->EndpointSpinLock);
    KeInitializeSpinLock(&Endpoint->StateChangeSpinLock);

    InitializeListHead(&Endpoint->PendingTransferList);
    InitializeListHead(&Endpoint->TransferList);
    InitializeListHead(&Endpoint->CancelList);
    InitializeListHead(&Endpoint->AbortList);

    EndpointProperties = &Endpoint->EndpointProperties;
    EndpointDescriptor = &PipeHandle->EndpointDescriptor;

    MaxPacketSize = EndpointDescriptor->wMaxPacketSize & 0x7FF;
    AdditionalTransaction = (EndpointDescriptor->wMaxPacketSize >> 11) & 3;

    EndpointProperties->DeviceAddress = DeviceHandle->DeviceAddress;
    EndpointProperties->DeviceSpeed = DeviceHandle->DeviceSpeed;
    EndpointProperties->Period = 0; // HighSpeedInterval
    EndpointProperties->EndpointAddress = EndpointDescriptor->bEndpointAddress;
    EndpointProperties->TransactionPerMicroframe = AdditionalTransaction + 1;
    EndpointProperties->MaxPacketSize = MaxPacketSize;
    EndpointProperties->TotalMaxPacketSize = MaxPacketSize * (AdditionalTransaction + 1);

    switch (EndpointDescriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK)
    {
        case USB_ENDPOINT_TYPE_CONTROL: // 0x00
            EndpointProperties->TransferType = USBPORT_TRANSFER_TYPE_CONTROL;

            if (EndpointProperties->EndpointAddress == 0)
            {
                EndpointProperties->MaxTransferSize = 0x1000; // OUT Ep0
            }
            else
            {
                EndpointProperties->MaxTransferSize = 0x10000;
            }

            break;

        case USB_ENDPOINT_TYPE_ISOCHRONOUS: // 0x01
            DPRINT1("USBPORT_OpenPipe: USB_ENDPOINT_TYPE_ISOCHRONOUS UNIMPLEMENTED. FIXME. \n");
            EndpointProperties->TransferType = USBPORT_TRANSFER_TYPE_ISOCHRONOUS;
            EndpointProperties->MaxTransferSize = 0x1000000;
            break;

        case USB_ENDPOINT_TYPE_BULK: // 0x02
            EndpointProperties->TransferType = USBPORT_TRANSFER_TYPE_BULK;
            EndpointProperties->MaxTransferSize = 0x10000;
            break;

        case USB_ENDPOINT_TYPE_INTERRUPT: // 0x03
            EndpointProperties->TransferType = USBPORT_TRANSFER_TYPE_INTERRUPT;
            EndpointProperties->MaxTransferSize = 0x400;
            break;
    }

    if (EndpointProperties->TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
    {
        if (EndpointProperties->DeviceSpeed == UsbHighSpeed)
        {
            Interval = USBPORT_NormalizeHsInterval(EndpointDescriptor->bInterval);
        }
        else
        {
            Interval = EndpointDescriptor->bInterval;
        }

        EndpointProperties->Period = 32;

        if (Interval && (Interval < 32))
        {
            if ((EndpointProperties->DeviceSpeed != UsbLowSpeed) ||
                (Interval >= 8))
            {
                if (!(Interval & 0x20))
                {
                    Period = EndpointProperties->Period;

                    do
                    {
                        Period >>= 1;
                    }
                    while (!(Period & Interval));

                    EndpointProperties->Period = Period;
                }
            }
            else
            {
                EndpointProperties->Period = 8;
            }
        }
    }

    if (EndpointProperties->TransferType == USB_ENDPOINT_TYPE_ISOCHRONOUS)
    {
        if (EndpointProperties->DeviceSpeed == UsbHighSpeed)
        {
            EndpointProperties->Period = 
                USBPORT_NormalizeHsInterval(EndpointDescriptor->bInterval);
        }
        else
        {
            EndpointProperties->Period = 1;
        }
    }

    if (Packet->MiniPortFlags & USB_MINIPORT_FLAGS_USB2)
    {
        IsAllocatedBandwidth = USBPORT_AllocateBandwidthUSB2(FdoDevice, Endpoint);
    }
    else
    {
        EndpointProperties->UsbBandwidth = USBPORT_CalculateUsbBandwidth(FdoDevice,
                                                                         Endpoint);

        IsAllocatedBandwidth = USBPORT_AllocateBandwidth(FdoDevice, Endpoint);
    }

    if (!IsAllocatedBandwidth)
    {
        Status = USBPORT_USBDStatusToNtStatus(NULL, USBD_STATUS_NO_BANDWIDTH);

        if (UsbdStatus)
        {
            *UsbdStatus = USBD_STATUS_NO_BANDWIDTH;
        }

        goto ExitWithError;
    }

    //EndpointProperties->Direction = ~EndpointDescriptor->bEndpointAddress >> 7;
    Direction = ~EndpointDescriptor->bEndpointAddress;
    Direction >>= 7;
    EndpointProperties->Direction = Direction;

    if (DeviceHandle->IsRootHub)
    {
        Endpoint->EndpointWorker = 0; // USBPORT_RootHubEndpointWorker;

        Endpoint->Flags |= ENDPOINT_FLAG_ROOTHUB_EP0;

        Endpoint->StateLast = USBPORT_ENDPOINT_ACTIVE;
        Endpoint->StateNext = USBPORT_ENDPOINT_ACTIVE;

        PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)FdoExtension->RootHubPdo->DeviceExtension;

        if (EndpointProperties->TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
        {
            PdoExtension->Endpoint = Endpoint;
        }

        USBDStatus = USBD_STATUS_SUCCESS;
    }
    else
    {
        Endpoint->EndpointWorker = 1; // USBPORT_DmaEndpointWorker;

        KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);

        Packet->QueryEndpointRequirements(FdoExtension->MiniPortExt,
                                          &Endpoint->EndpointProperties,
                                          (PULONG)&TransferParams);

        KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);

        if ((EndpointProperties->TransferType == USBPORT_TRANSFER_TYPE_BULK) ||
            (EndpointProperties->TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT))
        {
            EndpointProperties->MaxTransferSize = TransferParams[1];
        }

        if (TransferParams[0])
        {
            HeaderBuffer = USBPORT_AllocateCommonBuffer(FdoDevice,
                                                        TransferParams[0]);
        }
        else
        {
            HeaderBuffer = NULL;
        }

        if (HeaderBuffer || (TransferParams[0] == 0))
        {
            Endpoint->HeaderBuffer = HeaderBuffer;

            if (HeaderBuffer)
            {
                EndpointProperties->BufferVA = HeaderBuffer->VirtualAddress;
                EndpointProperties->BufferPA = HeaderBuffer->PhysicalAddress;
                EndpointProperties->BufferLength = HeaderBuffer->BufferLength; // BufferLength + LengthPadded;
            }

            MpStatus = MiniportOpenEndpoint(FdoDevice, Endpoint);

            Endpoint->Flags |= ENDPOINT_FLAG_DMA_TYPE;
            Endpoint->Flags |= ENDPOINT_FLAG_QUEUENE_EMPTY;

            if (MpStatus == 0)
            {
                ULONG State;

                KeAcquireSpinLock(&Endpoint->EndpointSpinLock, &Endpoint->EndpointOldIrql);

                Endpoint->StateLast = USBPORT_ENDPOINT_PAUSED;
                Endpoint->StateNext = USBPORT_ENDPOINT_PAUSED;

                USBPORT_SetEndpointState(Endpoint, USBPORT_ENDPOINT_ACTIVE);

                KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);


                while (TRUE)
                {
                    KeAcquireSpinLock(&Endpoint->EndpointSpinLock, &Endpoint->EndpointOldIrql);
                    State = USBPORT_GetEndpointState(Endpoint);
                    KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);

                    if (State == USBPORT_ENDPOINT_ACTIVE)
                    {
                        break;
                    }

                    USBPORT_Wait(FdoDevice, 1); // 1 msec.
                }
            }
        }
        else
        {
            MpStatus = 2;
            Endpoint->HeaderBuffer = NULL;
        }

        if (MpStatus)
        {
            USBDStatus = USBD_STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            USBDStatus = USBD_STATUS_SUCCESS;
        }
    }

    if (UsbdStatus)
    {
        *UsbdStatus = USBDStatus;
    }

    Status = USBPORT_USBDStatusToNtStatus(NULL, USBDStatus);

    if (NT_SUCCESS(Status))
    {
        USBPORT_AddPipeHandle(DeviceHandle, PipeHandle);

        ExInterlockedInsertTailList(&FdoExtension->EndpointList,
                                    &Endpoint->EndpointLink,
                                    &FdoExtension->EndpointListSpinLock);

        PipeHandle->Endpoint = Endpoint;
        PipeHandle->Flags &= ~PIPE_HANDLE_FLAG_CLOSED;

        return Status;
    }

ExitWithError:

    if (Endpoint)
    {
        if (IsAllocatedBandwidth)
        {
            if (Packet->MiniPortFlags & USB_MINIPORT_FLAGS_USB2)
            {
                USBPORT_FreeBandwidthUSB2(FdoDevice, Endpoint);
            }
            else
            {
                USBPORT_FreeBandwidth(FdoDevice, Endpoint);
            }
        }

        ExFreePool(Endpoint);
    }

    DPRINT1("USBPORT_OpenPipe: Status - %p\n", Status);
    return Status;
}

NTSTATUS
NTAPI
USBPORT_ReopenPipe(IN PDEVICE_OBJECT FdoDevice,
                   IN PUSBPORT_ENDPOINT Endpoint)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_COMMON_BUFFER_HEADER HeaderBuffer;
    ULONG EndpointRequirements[2] = {0};
    PUSBPORT_REGISTRATION_PACKET Packet;
    KIRQL MiniportOldIrql;
    KIRQL OldIrql;
    NTSTATUS Status;

    DPRINT("USBPORT_ReopenPipe ... \n");

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    while (TRUE)
    {
        if (!InterlockedIncrement(&Endpoint->LockCounter))
            break;
    }

    KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &MiniportOldIrql);

    Packet->SetEndpointState(FdoExtension->MiniPortExt,
                             (PVOID)((ULONG_PTR)Endpoint + sizeof(USBPORT_ENDPOINT)),
                             USBPORT_ENDPOINT_CLOSED);

    KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, MiniportOldIrql);

    USBPORT_Wait(FdoDevice, 2);

    MiniportCloseEndpoint(FdoDevice, Endpoint);

    RtlZeroMemory((PVOID)((ULONG_PTR)Endpoint + sizeof(USBPORT_ENDPOINT)),
                  Packet->MiniPortEndpointSize);

    if (Endpoint->HeaderBuffer)
    {
        USBPORT_FreeCommonBuffer(FdoDevice, Endpoint->HeaderBuffer);
        Endpoint->HeaderBuffer = 0;
    }

    KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &MiniportOldIrql);

    Packet->QueryEndpointRequirements(FdoExtension->MiniPortExt,
                                      &Endpoint->EndpointProperties,
                                      (PULONG)&EndpointRequirements);

    KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, MiniportOldIrql);

    if (EndpointRequirements[0])
    {
        HeaderBuffer = USBPORT_AllocateCommonBuffer(FdoDevice,
                                                    EndpointRequirements[0]);
    }
    else
    {
        HeaderBuffer = NULL;
    }

    if (HeaderBuffer || EndpointRequirements[0] == 0)
    {
        Endpoint->HeaderBuffer = HeaderBuffer;
        Status = STATUS_SUCCESS;
    }
    else
    {
        Endpoint->HeaderBuffer = 0;
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Endpoint->HeaderBuffer && HeaderBuffer)
    {
        Endpoint->EndpointProperties.BufferVA = HeaderBuffer->VirtualAddress;
        Endpoint->EndpointProperties.BufferPA = HeaderBuffer->PhysicalAddress;
        Endpoint->EndpointProperties.BufferLength = HeaderBuffer->BufferLength;
    }

    if (NT_SUCCESS(Status))
    {
        MiniportOpenEndpoint(FdoDevice, Endpoint);

        KeAcquireSpinLock(&Endpoint->EndpointSpinLock, &Endpoint->EndpointOldIrql);
        KeAcquireSpinLock(&Endpoint->StateChangeSpinLock, &OldIrql);

        if (Endpoint->StateLast == USBPORT_ENDPOINT_ACTIVE)
        {
            KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &MiniportOldIrql);

            Packet->SetEndpointState(FdoExtension->MiniPortExt,
                                     (PVOID)((ULONG_PTR)Endpoint + sizeof(USBPORT_ENDPOINT)),
                                     USBPORT_ENDPOINT_ACTIVE);

            KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, MiniportOldIrql);
        }

        KeReleaseSpinLock(&Endpoint->StateChangeSpinLock, OldIrql);
        KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);
    }

    InterlockedDecrement(&Endpoint->LockCounter);

    return Status;
}

VOID
NTAPI
USBPORT_FlushClosedEndpointList(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION  FdoExtension;
    KIRQL OldIrql;
    PLIST_ENTRY ClosedList;
    PUSBPORT_ENDPOINT Endpoint;

    DPRINT("USBPORT_FlushClosedEndpointList: ... \n");

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

    KeAcquireSpinLock(&FdoExtension->EndpointClosedSpinLock, &OldIrql);
    ClosedList = &FdoExtension->EndpointClosedList;

    while (TRUE)
    {
        if (IsListEmpty(ClosedList))
            break;

        Endpoint = CONTAINING_RECORD(ClosedList->Flink,
                                     USBPORT_ENDPOINT,
                                     CloseLink);

        RemoveHeadList(ClosedList);
        Endpoint->CloseLink.Flink = NULL;
        Endpoint->CloseLink.Blink = NULL;

        KeReleaseSpinLock(&FdoExtension->EndpointClosedSpinLock, OldIrql);

        USBPORT_DeleteEndpoint(FdoDevice, Endpoint);

        KeAcquireSpinLock(&FdoExtension->EndpointClosedSpinLock, &OldIrql);
    }

    KeReleaseSpinLock(&FdoExtension->EndpointClosedSpinLock, OldIrql);
}

VOID
NTAPI
USBPORT_InvalidateEndpointHandler(IN PDEVICE_OBJECT FdoDevice,
                                  IN PUSBPORT_ENDPOINT Endpoint,
                                  IN ULONG Type)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    PLIST_ENTRY Entry;
    PLIST_ENTRY WorkerLink;
    PUSBPORT_ENDPOINT endpoint;
    KIRQL OldIrql;
    BOOLEAN IsAddEntry = FALSE;

    DPRINT_CORE("USBPORT_InvalidateEndpointHandler: Endpoint - %p, Type - %x\n",
                Endpoint,
                Type);

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    if (Endpoint)
    {
        WorkerLink = &Endpoint->WorkerLink;
        KeAcquireSpinLock(&FdoExtension->EndpointListSpinLock, &OldIrql);
        DPRINT_CORE("USBPORT_InvalidateEndpointHandler: KeAcquireSpinLock \n");

        if ((!WorkerLink->Flink || !WorkerLink->Blink) &&
            !(Endpoint->Flags & ENDPOINT_FLAG_IDLE) &&
            USBPORT_GetEndpointState(Endpoint) != USBPORT_ENDPOINT_NOT_HANDLED)
        {
            DPRINT_CORE("USBPORT_InvalidateEndpointHandler: InsertTailList \n");
            InsertTailList(&FdoExtension->WorkerList, WorkerLink);
            IsAddEntry = TRUE;
        }

        KeReleaseSpinLock(&FdoExtension->EndpointListSpinLock, OldIrql);

        if (Endpoint->Flags & ENDPOINT_FLAG_ROOTHUB_EP0)
            Type = INVALIDATE_ENDPOINT_WORKER_THREAD;
    }
    else
    {
        KeAcquireSpinLock(&FdoExtension->EndpointListSpinLock, &OldIrql);

        Entry = &FdoExtension->EndpointList;

        if (Entry != &FdoExtension->EndpointList)
        {
            while (Entry && Entry != &FdoExtension->EndpointList)
            {
                endpoint = CONTAINING_RECORD(Entry,
                                             USBPORT_ENDPOINT,
                                             EndpointLink);

                if (!endpoint->WorkerLink.Flink || !endpoint->WorkerLink.Blink)
                {
                    if (!(endpoint->Flags & ENDPOINT_FLAG_IDLE) &&
                        !(endpoint->Flags & ENDPOINT_FLAG_ROOTHUB_EP0) &&
                        USBPORT_GetEndpointState(endpoint) != USBPORT_ENDPOINT_NOT_HANDLED)
                    {
                        DPRINT_CORE("USBPORT_InvalidateEndpointHandler: InsertTailList \n");
                        InsertTailList(&FdoExtension->WorkerList, &endpoint->WorkerLink);
                        IsAddEntry = TRUE;
                    }
                }

                Entry = endpoint->EndpointLink.Flink;
            }
        }

        KeReleaseSpinLock(&FdoExtension->EndpointListSpinLock, OldIrql);
    }

    if (FdoExtension->Flags & USBPORT_FLAG_HC_SUSPEND)
    {
        Type = INVALIDATE_ENDPOINT_WORKER_THREAD;
    }
    else if (IsAddEntry == FALSE && Type == INVALIDATE_ENDPOINT_INT_NEXT_SOF)
    {
        Type = INVALIDATE_ENDPOINT_ONLY;
    }

    switch (Type)
    {
        case INVALIDATE_ENDPOINT_WORKER_THREAD:
            USBPORT_SignalWorkerThread(FdoDevice);
            break;

        case INVALIDATE_ENDPOINT_WORKER_DPC:
            KeInsertQueueDpc(&FdoExtension->WorkerRequestDpc, NULL, NULL);
            break;

        case INVALIDATE_ENDPOINT_INT_NEXT_SOF:
            KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);
            Packet->InterruptNextSOF(FdoExtension->MiniPortExt);
            KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);
            break;
    }
}

ULONG
NTAPI
USBPORT_DmaEndpointPaused(IN PDEVICE_OBJECT FdoDevice,
                          IN PUSBPORT_ENDPOINT Endpoint)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    PLIST_ENTRY Entry;
    PUSBPORT_TRANSFER Transfer;
    PURB Urb;
    ULONG Frame;
    ULONG CurrentFrame;
    ULONG CompletedLen = 0;
    KIRQL OldIrql;

    DPRINT_CORE("USBPORT_DmaEndpointPaused \n");

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    Entry = Endpoint->TransferList.Flink;

    if (Entry == &Endpoint->TransferList)
        return USBPORT_ENDPOINT_ACTIVE;

    while (Entry && Entry != &Endpoint->TransferList)
    {
        Transfer = CONTAINING_RECORD(Entry,
                                     USBPORT_TRANSFER,
                                     TransferLink);

        if (Transfer->Flags & (TRANSFER_FLAG_CANCELED | TRANSFER_FLAG_ABORTED))
        {
            if (Transfer->Flags & TRANSFER_FLAG_ISO &&
                Transfer->Flags & TRANSFER_FLAG_SUBMITED &&
                !(Endpoint->Flags & ENDPOINT_FLAG_NUKE))
            {
                Urb = Transfer->Urb;

                Frame = Urb->UrbIsochronousTransfer.StartFrame +
                        Urb->UrbIsochronousTransfer.NumberOfPackets;

                KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);
                CurrentFrame = Packet->Get32BitFrameNumber(FdoExtension->MiniPortExt);
                KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);

                if (Frame + 1 > CurrentFrame)
                {
                    return USBPORT_GetEndpointState(Endpoint);
                }
            }

            if ((Transfer->Flags & TRANSFER_FLAG_SUBMITED) &&
                 !(Endpoint->Flags & ENDPOINT_FLAG_NUKE))
            {
                KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);

                Packet->AbortTransfer(FdoExtension->MiniPortExt,
                                      (PVOID)((ULONG_PTR)Endpoint + sizeof(USBPORT_ENDPOINT)),
                                      Transfer->MiniportTransfer,
                                      &CompletedLen);

                KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);

                if (Transfer->Flags & TRANSFER_FLAG_ISO)
                {
                    DPRINT1("USBPORT_DmaEndpointActive: FIXME call USBPORT_FlushIsoTransfer\n");
                    ASSERT(FALSE); //USBPORT_FlushIsoTransfer();
                }
                else
                {
                    Transfer->CompletedTransferLen = CompletedLen;
                }
            }

            RemoveEntryList(&Transfer->TransferLink);
            Entry = Transfer->TransferLink.Flink;

            if (Transfer->Flags & TRANSFER_FLAG_SPLITED)
            {
                DPRINT1("USBPORT_DmaEndpointActive: FIXME call USBPORT_CancelSplitTransfer\n");
                ASSERT(FALSE); //USBPORT_CancelSplitTransfer();
            }
            else
            {
                InsertTailList(&Endpoint->CancelList, &Transfer->TransferLink);
            }
        }
        else
        {
            Entry = Transfer->TransferLink.Flink;
        }
    }

    return USBPORT_ENDPOINT_ACTIVE;
}

ULONG
NTAPI
USBPORT_DmaEndpointActive(IN PDEVICE_OBJECT FdoDevice,
                          IN PUSBPORT_ENDPOINT Endpoint)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    PLIST_ENTRY Entry;
    PUSBPORT_TRANSFER Transfer;
    LARGE_INTEGER TimeOut = {{0, 0}};
    UCHAR CF;
    MPSTATUS MpStatus;
    KIRQL OldIrql;

    DPRINT_CORE("USBPORT_DmaEndpointActive \n");

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

    Entry = Endpoint->TransferList.Flink;

    if (Entry != &Endpoint->TransferList)
    {
        while (Entry && Entry != &Endpoint->TransferList)
        {
            Transfer = CONTAINING_RECORD(Entry,
                                         USBPORT_TRANSFER,
                                         TransferLink);

            if (Transfer->Flags & 0x800)
            {
                USBPORT_QueueDoneTransfer(Transfer, USBD_STATUS_SUCCESS);
                return USBPORT_ENDPOINT_ACTIVE;
            }

            if (!(Transfer->Flags & TRANSFER_FLAG_SUBMITED) &&
                 !(Endpoint->Flags & ENDPOINT_FLAG_NUKE))
            {
                KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);

                Packet = &FdoExtension->MiniPortInterface->Packet;

                if (Transfer->Flags & TRANSFER_FLAG_ISO)
                {
                    DPRINT1("USBPORT_DmaEndpointActive: FIXME call SubmitIsoTransfer\n");

                    MpStatus = Packet->SubmitIsoTransfer(FdoExtension->MiniPortExt,
                                                         (PVOID)((ULONG_PTR)Endpoint + sizeof(USBPORT_ENDPOINT)),
                                                         &Transfer->TransferParameters,
                                                         Transfer->MiniportTransfer,
                                                         NULL);//&Transfer->IsoTransferParameters);
                }
                else
                {
                    MpStatus = Packet->SubmitTransfer(FdoExtension->MiniPortExt,
                                                      (PVOID)((ULONG_PTR)Endpoint + sizeof(USBPORT_ENDPOINT)),
                                                      &Transfer->TransferParameters,
                                                      Transfer->MiniportTransfer,
                                                      &Transfer->SgList);
                }

                KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);

                if (MpStatus)
                {
                    if ((MpStatus != 1) && Transfer->Flags & TRANSFER_FLAG_ISO)
                    {
                        DPRINT1("USBPORT_DmaEndpointActive: FIXME call USBPORT_ErrorCompleteIsoTransfer\n");
                        ASSERT(FALSE); //USBPORT_ErrorCompleteIsoTransfer();
                    }

                    return USBPORT_ENDPOINT_ACTIVE;
                }

                Transfer->Flags |= TRANSFER_FLAG_SUBMITED;
                KeQuerySystemTime(&Transfer->Time);

                CF = 0;
                TimeOut.QuadPart = 10000 * Transfer->TimeOut;

                if (TimeOut.LowPart > (0xFFFFFFFF - Transfer->Time.LowPart))
                {
                    CF = 1;
                }

                Transfer->Time.LowPart += TimeOut.LowPart;
                Transfer->Time.HighPart += (TimeOut.HighPart + CF);
            }

            if (Transfer->Flags & (TRANSFER_FLAG_CANCELED | TRANSFER_FLAG_ABORTED))
            {
                return USBPORT_ENDPOINT_PAUSED;
            }

            Entry = Transfer->TransferLink.Flink;
        }
    }

    return USBPORT_ENDPOINT_ACTIVE;
}

VOID
NTAPI
USBPORT_DmaEndpointWorker(IN PUSBPORT_ENDPOINT Endpoint)
{
    PDEVICE_OBJECT FdoDevice;
    ULONG PipeState;
    ULONG EndpointState;
    BOOLEAN IsPaused = FALSE;

    DPRINT_CORE("USBPORT_DmaEndpointWorker ... \n");

    FdoDevice = Endpoint->FdoDevice;

    KeAcquireSpinLock(&Endpoint->EndpointSpinLock, &Endpoint->EndpointOldIrql);

    PipeState = USBPORT_GetEndpointState(Endpoint);

    if (PipeState == USBPORT_ENDPOINT_PAUSED)
    {
        EndpointState = USBPORT_DmaEndpointPaused(FdoDevice, Endpoint);
    }
    else if (PipeState == USBPORT_ENDPOINT_ACTIVE)
    {
        EndpointState = USBPORT_DmaEndpointActive(FdoDevice, Endpoint);
    }

    KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);

    USBPORT_FlushCancelList(Endpoint);

    KeAcquireSpinLock(&Endpoint->EndpointSpinLock, &Endpoint->EndpointOldIrql);

    if (EndpointState == PipeState)
    {
        if (EndpointState == USBPORT_ENDPOINT_PAUSED)
        {
            IsPaused = TRUE;
        }
    }
    else
    {
        USBPORT_SetEndpointState(Endpoint, EndpointState);
    }

    KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);

    if (IsPaused)
    {
       USBPORT_InvalidateEndpointHandler(FdoDevice,
                                         Endpoint,
                                         INVALIDATE_ENDPOINT_WORKER_THREAD);
    }

    DPRINT_CORE("USBPORT_DmaEndpointWorker exit \n");
}

BOOLEAN
NTAPI
USBPORT_EndpointWorker(IN PUSBPORT_ENDPOINT Endpoint,
                       IN BOOLEAN Flag)
{
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    KIRQL OldIrql;
    ULONG EndpointState;

    DPRINT_CORE("USBPORT_EndpointWorker: Endpoint - %p, Flag - %x\n",
           Endpoint,
           Flag);

    FdoDevice = Endpoint->FdoDevice;
    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    if (Flag == FALSE)
    {
        if (InterlockedIncrement(&Endpoint->LockCounter))
        {
            InterlockedDecrement(&Endpoint->LockCounter);
            DPRINT_CORE("USBPORT_EndpointWorker: LockCounter > 0\n");
            return TRUE;
        }
    }

    KeAcquireSpinLock(&Endpoint->EndpointSpinLock, &Endpoint->EndpointOldIrql);

    if (USBPORT_GetEndpointState(Endpoint) == USBPORT_ENDPOINT_NOT_HANDLED)
    {
        KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);
        InterlockedDecrement(&Endpoint->LockCounter);
        DPRINT_CORE("USBPORT_EndpointWorker: State == USBPORT_ENDPOINT_NOT_HANDLED. return FALSE\n");
        return FALSE;
    }

    if ((Endpoint->Flags & (ENDPOINT_FLAG_ROOTHUB_EP0 | ENDPOINT_FLAG_NUKE)) == 0)
    {
        KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);

        Packet->PollEndpoint(FdoExtension->MiniPortExt,
                             (PVOID)((ULONG_PTR)Endpoint + sizeof(USBPORT_ENDPOINT)));

        KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);
    }

    EndpointState = USBPORT_GetEndpointState(Endpoint);

    if (EndpointState == USBPORT_ENDPOINT_CLOSED)
    {
        KeAcquireSpinLock(&Endpoint->StateChangeSpinLock, &Endpoint->EndpointStateOldIrql);
        Endpoint->StateLast = USBPORT_ENDPOINT_NOT_HANDLED;
        Endpoint->StateNext = USBPORT_ENDPOINT_NOT_HANDLED;
        KeReleaseSpinLock(&Endpoint->StateChangeSpinLock, Endpoint->EndpointStateOldIrql);

        KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);

        KeAcquireSpinLockAtDpcLevel(&FdoExtension->EndpointListSpinLock);

        ExInterlockedInsertTailList(&FdoExtension->EndpointClosedList,
                                    &Endpoint->CloseLink,
                                    &FdoExtension->EndpointClosedSpinLock);

        KeReleaseSpinLockFromDpcLevel(&FdoExtension->EndpointListSpinLock);

        InterlockedDecrement(&Endpoint->LockCounter);
        DPRINT_CORE("USBPORT_EndpointWorker: State == USBPORT_ENDPOINT_CLOSED. return FALSE\n");
        return FALSE;
    }

    if (!IsListEmpty(&Endpoint->PendingTransferList) ||
        !IsListEmpty(&Endpoint->TransferList) ||
        !IsListEmpty(&Endpoint->CancelList))
    {
        KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);

        EndpointState = USBPORT_GetEndpointState(Endpoint);

        KeAcquireSpinLock(&Endpoint->StateChangeSpinLock, &Endpoint->EndpointStateOldIrql);
        if (EndpointState == Endpoint->StateNext)
        {
            KeReleaseSpinLock(&Endpoint->StateChangeSpinLock, Endpoint->EndpointStateOldIrql);

            if (Endpoint->EndpointWorker)
            {
                USBPORT_DmaEndpointWorker(Endpoint);
            }
            else
            {
                USBPORT_RootHubEndpointWorker(Endpoint);
            }

            USBPORT_FlushAbortList(Endpoint);

            InterlockedDecrement(&Endpoint->LockCounter);
            DPRINT_CORE("USBPORT_EndpointWorker: return FALSE\n");
            return FALSE;
        }

        KeReleaseSpinLock(&Endpoint->StateChangeSpinLock, Endpoint->EndpointStateOldIrql);
        InterlockedDecrement(&Endpoint->LockCounter);

        DPRINT_CORE("USBPORT_EndpointWorker: return TRUE\n");
        return TRUE;
    }

    KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);

    USBPORT_FlushAbortList(Endpoint);

    InterlockedDecrement(&Endpoint->LockCounter);
    DPRINT_CORE("USBPORT_EndpointWorker: return FALSE\n");
    return FALSE;
}
