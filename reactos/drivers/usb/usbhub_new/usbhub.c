#include "usbhub.h"

//#define NDEBUG
#include <debug.h>

PVOID GenericUSBDeviceString = NULL;

NTSTATUS
NTAPI
USBH_Wait(IN ULONG Milliseconds)
{
    LARGE_INTEGER Interval = {{0, 0}};

    DPRINT("USBH_Wait: Milliseconds - %x\n", Milliseconds);
    Interval.QuadPart -= 10000 * Milliseconds + (KeQueryTimeIncrement() - 1);
    Interval.HighPart = -1;
    return KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}

NTSTATUS
NTAPI
USBH_GetConfigValue(IN PWSTR ValueName,
                    IN ULONG ValueType,
                    IN PVOID ValueData,
                    IN ULONG ValueLength,
                    IN PVOID Context,
                    IN PVOID EntryContext)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("USBPORT_GetConfigValue: ... \n");

    if (ValueType == REG_BINARY)
    {
        *(PUCHAR)EntryContext = *(PUCHAR)ValueData;
    }
    else if (ValueType == REG_DWORD)
    {
        *(PULONG)EntryContext = *(PULONG)ValueData;
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}

VOID
NTAPI
USBH_CompleteIrp(IN PIRP Irp,
                 IN NTSTATUS CompleteStatus)
{
    if (CompleteStatus != STATUS_SUCCESS)
    {
        //DbgBreakPoint();
        DPRINT1("USBH_CompleteIrp: Irp - %p, CompleteStatus - %x\n",
                Irp,
                CompleteStatus);
    }

    Irp->IoStatus.Status = CompleteStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
NTAPI
USBH_PassIrp(IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp)
{
    //DPRINT("USBH_PassIrp: DeviceObject - %p, Irp - %p\n",
    //       DeviceObject,
    //       Irp);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DeviceObject, Irp);
}

NTSTATUS
NTAPI
USBH_SyncIrpComplete(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp,
                     IN PVOID Context)
{
    PUSBHUB_URB_TIMEOUT_CONTEXT HubTimeoutContext;
    KIRQL OldIrql;
    BOOLEAN TimerCancelled;

    DPRINT("USBH_SyncIrpComplete: ... \n");

    HubTimeoutContext = (PUSBHUB_URB_TIMEOUT_CONTEXT)Context;

    KeAcquireSpinLock(&HubTimeoutContext->UrbTimeoutSpinLock, &OldIrql);
    HubTimeoutContext->IsNormalCompleted = TRUE;
    TimerCancelled = KeCancelTimer(&HubTimeoutContext->UrbTimeoutTimer);
    KeReleaseSpinLock(&HubTimeoutContext->UrbTimeoutSpinLock, OldIrql);

    if (TimerCancelled)
    {
        KeSetEvent(&HubTimeoutContext->UrbTimeoutEvent,
                   EVENT_INCREMENT,
                   FALSE);
    }

    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
IsBitSet(IN ULONG_PTR BitMapAddress,
         IN ULONG Bit)
{
    BOOLEAN IsSet;

    IsSet = (*(PUCHAR)(BitMapAddress + (Bit >> 3)) & (1 << (Bit & 7))) != 0;
    DPRINT("IsBitSet: Bit - %p, IsSet - %x\n", Bit, IsSet);
    return IsSet;
}

PUSBHUB_PORT_PDO_EXTENSION
NTAPI
PdoExt(IN PDEVICE_OBJECT DeviceObject)
{
    PVOID PdoExtension;

    DPRINT("PdoExt: DeviceObject - %p\n", DeviceObject);

    if (DeviceObject)
    {
        PdoExtension = DeviceObject->DeviceExtension;
    }
    else
    {
        PdoExtension = (PVOID)-1;
    }

    return (PUSBHUB_PORT_PDO_EXTENSION)PdoExtension;
}

NTSTATUS
NTAPI
USBH_WriteFailReasonID(IN PDEVICE_OBJECT DeviceObject,
                       IN ULONG Data)
{
    NTSTATUS Status;
    WCHAR SourceString[64];
    HANDLE KeyHandle;
    UNICODE_STRING DestinationString;

    DPRINT("USBH_WriteFailReason: ID - %x\n", Data);

    swprintf(SourceString, L"FailReasonID");

    Status = IoOpenDeviceRegistryKey(DeviceObject,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_ALL,
                                     &KeyHandle);

    if (NT_SUCCESS(Status))
    {
        RtlInitUnicodeString(&DestinationString, SourceString);

        return ZwSetValueKey(KeyHandle,
                             &DestinationString,
                             0,
                             REG_DWORD,
                             &Data,
                             sizeof(Data));

        ZwClose(KeyHandle);
    }

    return Status;
}

VOID
NTAPI
USBH_UrbTimeoutDPC(IN PKDPC Dpc,
                   IN PVOID DeferredContext,
                   IN PVOID SystemArgument1,
                   IN PVOID SystemArgument2)
{
    PUSBHUB_URB_TIMEOUT_CONTEXT HubTimeoutContext;
    KIRQL OldIrql;
    BOOL IsCompleted;

    DPRINT("USBH_TimeoutDPC ... \n");

    HubTimeoutContext = (PUSBHUB_URB_TIMEOUT_CONTEXT)DeferredContext;

    KeAcquireSpinLock(&HubTimeoutContext->UrbTimeoutSpinLock, &OldIrql);
    IsCompleted = HubTimeoutContext->IsNormalCompleted;
    KeReleaseSpinLock(&HubTimeoutContext->UrbTimeoutSpinLock, OldIrql);

    if (!IsCompleted)
    {
        IoCancelIrp(HubTimeoutContext->Irp);
    }

    KeSetEvent(&HubTimeoutContext->UrbTimeoutEvent,
               EVENT_INCREMENT,
               FALSE);
}

NTSTATUS
NTAPI
USBH_SetPdoRegistryParameter(IN PDEVICE_OBJECT DeviceObject,
                             IN PCWSTR SourceString,
                             IN PVOID Data,
                             IN ULONG DataSize,
                             IN ULONG Type,
                             IN ULONG DevInstKeyType)
{
    NTSTATUS Status;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;

    DPRINT("USBH_SetPdoRegistryParameter ... \n");

    RtlInitUnicodeString(&ValueName, SourceString);

    Status = IoOpenDeviceRegistryKey(DeviceObject,
                                     DevInstKeyType,
                                     STANDARD_RIGHTS_ALL,
                                     &KeyHandle);

    if (NT_SUCCESS(Status))
    {
         ZwSetValueKey(KeyHandle,
                       &ValueName,
                       0,
                       Type,
                       Data,
                       DataSize);

        ZwClose(KeyHandle);
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_SyncSubmitUrb(IN PDEVICE_OBJECT DeviceObject,
                   IN PURB Urb)
{
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    PUSBHUB_URB_TIMEOUT_CONTEXT HubTimeoutContext;
    BOOLEAN IsWaitTimeout = FALSE;
    LARGE_INTEGER DueTime = {{0, 0}};
    NTSTATUS Status;

    DPRINT("USBH_SyncSubmitUrb: ... \n");

    Urb->UrbHeader.UsbdDeviceHandle = NULL;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &Event,
                                        &IoStatusBlock);

    if (!Irp)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->Parameters.Others.Argument1 = Urb;

    HubTimeoutContext = ExAllocatePoolWithTag(NonPagedPool,
                                              sizeof(USBHUB_URB_TIMEOUT_CONTEXT),
                                              USB_HUB_TAG);

    if (HubTimeoutContext)
    {
        RtlZeroMemory(HubTimeoutContext, sizeof(USBHUB_URB_TIMEOUT_CONTEXT));

        HubTimeoutContext->Irp = Irp;
        HubTimeoutContext->IsNormalCompleted = FALSE;

        KeInitializeEvent(&HubTimeoutContext->UrbTimeoutEvent,
                          NotificationEvent,
                          FALSE);

        KeInitializeSpinLock(&HubTimeoutContext->UrbTimeoutSpinLock);
        KeInitializeTimer(&HubTimeoutContext->UrbTimeoutTimer);

        KeInitializeDpc(&HubTimeoutContext->UrbTimeoutDPC,
                        USBH_UrbTimeoutDPC,
                        HubTimeoutContext);

        DueTime.QuadPart -= 5000 * 10000; // Timeout 5 sec.

        KeSetTimer(&HubTimeoutContext->UrbTimeoutTimer,
                   DueTime,
                   &HubTimeoutContext->UrbTimeoutDPC);

        IoSetCompletionRoutine(Irp,
                               USBH_SyncIrpComplete,
                               HubTimeoutContext,
                               TRUE,
                               TRUE,
                               TRUE);

        IsWaitTimeout = TRUE;
    }

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }
    else
    {
        IoStatusBlock.Status = Status;
    }

    if (IsWaitTimeout)
    {
        KeWaitForSingleObject(&HubTimeoutContext->UrbTimeoutEvent,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);

       ExFreePool(HubTimeoutContext);
    }

    Status = IoStatusBlock.Status;

    return Status;
}

NTSTATUS
NTAPI
USBH_FdoSyncSubmitUrb(IN PDEVICE_OBJECT FdoDevice,
                      IN PURB Urb)
{
    PUSBHUB_FDO_EXTENSION HubExtension;

    DPRINT("USBH_FdoSyncSubmitUrb: FdoDevice - %p, Urb - %p\n",
           FdoDevice,
           Urb);

    HubExtension = (PUSBHUB_FDO_EXTENSION)FdoDevice->DeviceExtension;
    return USBH_SyncSubmitUrb(HubExtension->LowerDevice, Urb);
}

NTSTATUS
NTAPI
USBH_Transact(IN PUSBHUB_FDO_EXTENSION HubExtension,
              IN PVOID TransferBuffer,
              IN ULONG BufferLen,
              IN BOOLEAN Direction,
              IN USHORT Function,
              IN BM_REQUEST_TYPE RequestType,
              IN UCHAR Request,
              IN USHORT RequestValue,
              IN USHORT RequestIndex)
{
    struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST * Urb;
    ULONG TransferFlags;
    PVOID Buffer = NULL;
    NTSTATUS Status;

    DPRINT("USBH_Transact: ... \n");

    if (BufferLen)
    {
        Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                       (BufferLen + sizeof(ULONG)) & ~((sizeof(ULONG) - 1)),
                                       USB_HUB_TAG);

        if (!Buffer)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(Buffer, (BufferLen + sizeof(ULONG)) & ~((sizeof(ULONG) - 1)));
    }

    Urb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
                                USB_HUB_TAG);

    if (!Urb)
    {
        if (Buffer)
        {
            ExFreePool(Buffer);
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Urb, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));

    if (Direction)
    {
        if (BufferLen)
        {
            RtlCopyMemory(Buffer, TransferBuffer, BufferLen);
        }

        TransferFlags = USBD_TRANSFER_DIRECTION_OUT;
    }
    else
    {
        if (BufferLen)
        {
            RtlZeroMemory(TransferBuffer, BufferLen);
        }

        TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
    }

    Urb->Hdr.Length = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
    Urb->Hdr.Function = Function;
    Urb->Hdr.UsbdDeviceHandle = NULL;

    Urb->TransferFlags = TransferFlags;
    Urb->TransferBuffer = BufferLen != 0 ? Buffer : NULL;
    Urb->TransferBufferLength = BufferLen;
    Urb->TransferBufferMDL = NULL;
    Urb->UrbLink = NULL;

    Urb->RequestTypeReservedBits = RequestType.B;
    Urb->Request = Request;
    Urb->Value = RequestValue;
    Urb->Index = RequestIndex;

    Status = USBH_FdoSyncSubmitUrb(HubExtension->Common.SelfDevice, (PURB)Urb);

    if (!Direction && BufferLen)
    {
        RtlCopyMemory(TransferBuffer, Buffer, BufferLen);
    }

    if (Buffer)
    {
        ExFreePool(Buffer);
    }

    ExFreePool(Urb);

    return Status;
}

NTSTATUS
NTAPI
USBH_SyncResetPort(IN PUSBHUB_FDO_EXTENSION HubExtension,
                   IN USHORT Port)
{
    USBHUB_PORT_STATUS PortStatus;
    KEVENT Event;
    LARGE_INTEGER Timeout = {{0, 0}};
    ULONG ix;
    NTSTATUS Status;

    DPRINT("USBH_SyncResetPort: Port - %x\n", Port);

    InterlockedIncrement(&HubExtension->PendingRequestCount);

    KeWaitForSingleObject(&HubExtension->HubPortSemaphore,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    Status = USBH_SyncGetPortStatus(HubExtension,
                                    Port,
                                    &PortStatus,
                                    4);

    if (NT_SUCCESS(Status) && !PortStatus.UsbPortStatus.ConnectStatus)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    HubExtension->HubFlags |= USBHUB_FDO_FLAG_RESET_PORT_LOCK;
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    ix = 0;

    while (TRUE)
    {
        BM_REQUEST_TYPE RequestType;

        InterlockedExchange((PLONG)&HubExtension->pResetPortEvent,
                            (LONG)&Event);

        RequestType.B = 0;//0x23
        RequestType._BM.Recipient = BMREQUEST_TO_DEVICE;
        RequestType._BM.Type = BMREQUEST_CLASS;
        RequestType._BM.Dir = BMREQUEST_HOST_TO_DEVICE;

        Status = USBH_Transact(HubExtension,
                               NULL,
                               0,
                               1, // to device
                               URB_FUNCTION_CLASS_OTHER,
                               RequestType,
                               USB_REQUEST_SET_FEATURE,
                               USBHUB_FEATURE_PORT_RESET,
                               Port);

        Timeout.QuadPart -= 5000 * 10000;

        if (!NT_SUCCESS(Status))
        {
            InterlockedExchange((PLONG)&HubExtension->pResetPortEvent, 0);

            USBH_Wait(10);
            HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_RESET_PORT_LOCK;

            goto Exit;
        }

        Status = KeWaitForSingleObject(&Event,
                                       Suspended,
                                       KernelMode,
                                       FALSE,
                                       &Timeout);

        if (Status != STATUS_TIMEOUT)
        {
            break;
        }

        Status = USBH_SyncGetPortStatus(HubExtension,
                                        Port,
                                        &PortStatus,
                                        4);

        if (!NT_SUCCESS(Status) ||
            !PortStatus.UsbPortStatus.ConnectStatus ||
            ix >= 3)
        {
            InterlockedExchange((PLONG)&HubExtension->pResetPortEvent, 0);

            USBH_Wait(10);
            HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_RESET_PORT_LOCK;

            Status = STATUS_DEVICE_DATA_ERROR;
            goto Exit;
        }

        ++ix;

        KeInitializeEvent(&Event, NotificationEvent, FALSE);
    }

    Status = USBH_SyncGetPortStatus(HubExtension,
                                    Port,
                                    &PortStatus,
                                    4);

    if (!PortStatus.UsbPortStatus.ConnectStatus &&
        NT_SUCCESS(Status) &&
        HubExtension->HubFlags & USBHUB_FDO_FLAG_USB20_HUB)
    {
        Status = STATUS_DEVICE_DATA_ERROR;
    }

    USBH_Wait(10);
    HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_RESET_PORT_LOCK;

Exit:

    KeReleaseSemaphore(&HubExtension->HubPortSemaphore,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
    {
        KeSetEvent(&HubExtension->PendingRequestEvent,
                   EVENT_INCREMENT,
                   FALSE);
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_GetDeviceType(IN PUSBHUB_FDO_EXTENSION HubExtension,
                   IN PUSB_DEVICE_HANDLE DeviceHandle,
                   OUT USB_DEVICE_TYPE * OutDeviceType)
{
    PUSB_BUSIFFN_GET_DEVICE_INFORMATION QueryDeviceInformation;
    PUSB_DEVICE_INFORMATION_0 DeviceInfo;
    SIZE_T DeviceInformationBufferLength;
    USB_DEVICE_TYPE DeviceType = Usb11Device;
    ULONG dummy;
    NTSTATUS Status;

    DPRINT("USBH_GetDeviceType: ... \n");

    QueryDeviceInformation = HubExtension->BusInterface.QueryDeviceInformation;

    if (!QueryDeviceInformation)
    {
        DPRINT1("USBH_GetDeviceType: no QueryDeviceInformation()\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    DeviceInformationBufferLength = sizeof(USB_DEVICE_INFORMATION_0);

    while (TRUE)
    {
        DeviceInfo = ExAllocatePoolWithTag(PagedPool,
                                           DeviceInformationBufferLength,
                                           USB_HUB_TAG);

        if (!DeviceInfo)
        {
            DPRINT1("USBH_GetDeviceType: ExAllocatePoolWithTag() failed\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlZeroMemory(DeviceInfo, DeviceInformationBufferLength);

        DeviceInfo->InformationLevel = 0;

        Status = QueryDeviceInformation(HubExtension->BusInterface.BusContext,
                                        DeviceHandle,
                                        DeviceInfo,
                                        DeviceInformationBufferLength,
                                        &dummy);

        if (Status != STATUS_BUFFER_TOO_SMALL)
        {
            if (NT_SUCCESS(Status))
            {
                DeviceType = DeviceInfo->DeviceType;
            }

            ExFreePool(DeviceInfo);
            break;
        }

        DeviceInformationBufferLength = DeviceInfo->ActualLength;
        ExFreePool(DeviceInfo);
    }

    if (OutDeviceType)
    {
        *OutDeviceType = DeviceType;
        DPRINT("USBH_GetDeviceType: DeviceType - %x\n", DeviceType);
    }

    return Status;
}

NTSTATUS
NTAPI
USBHUB_GetExtendedHubInfo(IN PUSBHUB_FDO_EXTENSION HubExtension,
                          IN PUSB_EXTHUB_INFORMATION_0 HubInfoBuffer)
{
    PUSB_BUSIFFN_GET_EXTENDED_HUB_INFO GetExtendedHubInformation;
    ULONG dummy = 0;

    DPRINT("USBHUB_GetExtendedHubInfo: ... \n");

    GetExtendedHubInformation = HubExtension->BusInterface.GetExtendedHubInformation;

    return GetExtendedHubInformation(HubExtension->BusInterface.BusContext,
                                     HubExtension->LowerPDO,
                                     HubInfoBuffer,
                                     sizeof(USB_EXTHUB_INFORMATION_0),
                                     &dummy);
}

PUSBHUB_FDO_EXTENSION
NTAPI
USBH_GetRootHubExtension(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PDEVICE_OBJECT RootHubPdo;
    PDEVICE_OBJECT RootHubFdo;
    PUSBHUB_FDO_EXTENSION RootHubExtension;

    DPRINT("USBH_GetRootHubExtension: HubExtension - %p\n", HubExtension);

    RootHubExtension = HubExtension;

    if (HubExtension->LowerPDO != HubExtension->RootHubPdo)
    {
        RootHubPdo = HubExtension->RootHubPdo;

        do
        {
            RootHubFdo = RootHubPdo->AttachedDevice;
        }
        while (RootHubFdo->DriverObject != HubExtension->Common.SelfDevice->DriverObject);

        RootHubExtension = (PUSBHUB_FDO_EXTENSION)RootHubFdo->DeviceExtension;
    }

    DPRINT("USBH_GetRootHubExtension: RootHubExtension - %p\n", RootHubExtension);

    return RootHubExtension;
}

NTSTATUS
NTAPI
USBH_SyncGetRootHubPdo(IN PDEVICE_OBJECT DeviceObject,
                       IN OUT PDEVICE_OBJECT * OutPdo1,
                       IN OUT PDEVICE_OBJECT * OutPdo2)
{
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    NTSTATUS result;

    DPRINT("USBH_SyncGetRootHubPdo: ... \n");

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &Event,
                                        &IoStatusBlock);

    if (!Irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->Parameters.Others.Argument1 = OutPdo1;
    IoStack->Parameters.Others.Argument2 = OutPdo2;

    result = IoCallDriver(DeviceObject, Irp);

    if (result == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }
    else
    {
        IoStatusBlock.Status = result;
    }

    Status = IoStatusBlock.Status;

    return Status;
}

NTSTATUS
NTAPI
USBH_SyncGetHubCount(IN PDEVICE_OBJECT DeviceObject,
                     IN OUT PULONG OutHubCount)
{
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    NTSTATUS result;

    DPRINT("USBH_SyncGetHubCount: *OutHubCount - %x\n", *OutHubCount);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_GET_HUB_COUNT,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &Event,
                                        &IoStatusBlock);

    if (!Irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->Parameters.Others.Argument1 = OutHubCount;

    result = IoCallDriver(DeviceObject, Irp);

    if (result == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }
    else
    {
        IoStatusBlock.Status = result;
    }

    Status = IoStatusBlock.Status;

    return Status;
}

PUSB_DEVICE_HANDLE
NTAPI
USBH_SyncGetDeviceHandle(IN PDEVICE_OBJECT DeviceObject)
{
    PIRP Irp;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PUSB_DEVICE_HANDLE DeviceHandle = NULL;
    PIO_STACK_LOCATION IoStack;

    DPRINT("USBH_SyncGetDeviceHandle: ... \n");

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &Event,
                                        &IoStatusBlock);

    if (!Irp)
    {
        DPRINT1("USBH_SyncGetDeviceHandle: Irp - NULL!\n");
        return NULL;
    }

    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->Parameters.Others.Argument1 = &DeviceHandle;

    if (IoCallDriver(DeviceObject, Irp) == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    return DeviceHandle;
}

NTSTATUS
NTAPI
USBH_GetDeviceDescriptor(IN PDEVICE_OBJECT DeviceObject,
                         IN PUSB_DEVICE_DESCRIPTOR HubDeviceDescriptor)
{
    struct _URB_CONTROL_DESCRIPTOR_REQUEST * Urb;
    NTSTATUS Status;

    DPRINT("USBH_GetDeviceDescriptor: ... \n");

    Urb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                USB_HUB_TAG);

    if (!Urb)
    {
        DPRINT1("USBH_SyncGetDeviceHandle: Urb - NULL!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Urb, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    Urb->Hdr.Function = URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE;
    Urb->Hdr.Length = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);

    Urb->TransferBufferLength = sizeof(USB_DEVICE_DESCRIPTOR);
    Urb->TransferBuffer = HubDeviceDescriptor;
    Urb->DescriptorType = USB_DEVICE_DESCRIPTOR_TYPE;

    Status = USBH_FdoSyncSubmitUrb(DeviceObject, (PURB)Urb);

    ExFreePool(Urb);

    return Status;
}

NTSTATUS
NTAPI
USBH_SyncGetDeviceConfigurationDescriptor(IN PDEVICE_OBJECT DeviceObject,
                                          IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor,
                                          IN ULONG NumberOfBytes,
                                          IN PULONG OutLength)
{
    PCOMMON_DEVICE_EXTENSION DeviceExtension;
    struct _URB_CONTROL_DESCRIPTOR_REQUEST * Urb;
    NTSTATUS Status;

    DPRINT("USBH_SyncGetDeviceConfigurationDescriptor: ... \n");

    DeviceExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (OutLength)
    {
        *OutLength = 0;
    }

    Urb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                USB_HUB_TAG);

    if (!Urb)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Urb, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    Urb->Hdr.Function = URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE;
    Urb->Hdr.Length = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);

    Urb->TransferBufferLength = NumberOfBytes;
    Urb->TransferBuffer = ConfigDescriptor;
    Urb->DescriptorType = USB_CONFIGURATION_DESCRIPTOR_TYPE;

    if (DeviceExtension->ExtensionType == USBH_EXTENSION_TYPE_HUB ||
        DeviceExtension->ExtensionType == USBH_EXTENSION_TYPE_PARENT)
    {
        Status = USBH_FdoSyncSubmitUrb(DeviceObject, (PURB)Urb);
    }
    else
    {
        Status = USBH_SyncSubmitUrb(DeviceObject, (PURB)Urb);
    }

    if (OutLength)
    {
        *OutLength = Urb->TransferBufferLength;
    }

    if (Urb)
    {
        ExFreePool(Urb);
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_GetConfigurationDescriptor(IN PDEVICE_OBJECT DeviceObject,
                                IN PUSB_CONFIGURATION_DESCRIPTOR * pConfigurationDescriptor)
{
    PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor;
    NTSTATUS Status;
    ULONG Length;

    DPRINT("USBH_GetConfigurationDescriptor: ... \n");

    ConfigDescriptor = ExAllocatePoolWithTag(NonPagedPool, 0xFF, USB_HUB_TAG);

    if (!ConfigDescriptor)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    RtlZeroMemory(ConfigDescriptor, 0xFF);

    Status = USBH_SyncGetDeviceConfigurationDescriptor(DeviceObject,
                                                       ConfigDescriptor,
                                                       0xFF,
                                                       &Length);

    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit;
    }

    if (Length < sizeof(USB_CONFIGURATION_DESCRIPTOR))
    {
        Status = STATUS_DEVICE_DATA_ERROR;
        goto ErrorExit;
    }

    if (Length < ConfigDescriptor->wTotalLength)
    {
        Status = STATUS_DEVICE_DATA_ERROR;
        goto ErrorExit;
    }

    if (ConfigDescriptor->wTotalLength > 0xFF)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    *pConfigurationDescriptor = ConfigDescriptor;

     return Status;

ErrorExit:

    if (ConfigDescriptor)
    {
        ExFreePool(ConfigDescriptor);
    }

    *pConfigurationDescriptor = NULL;

    return Status;
}

NTSTATUS
NTAPI
USBH_SyncGetHubDescriptor(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PUSB_EXTHUB_INFORMATION_0 ExtendedHubInfo;
    ULONG NumberPorts;
    PUSBHUB_PORT_DATA PortData;
    USHORT RequestValue;
    ULONG NumberOfBytes;
    NTSTATUS Status;
    PUSB_HUB_DESCRIPTOR HubDescriptor;
    ULONG ix;

    DPRINT("USBH_SyncGetHubDescriptor: ... \n");

    ExtendedHubInfo = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(USB_EXTHUB_INFORMATION_0),
                                            USB_HUB_TAG);

    if (!ExtendedHubInfo)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    RtlZeroMemory(ExtendedHubInfo, sizeof(USB_EXTHUB_INFORMATION_0));

    Status = USBHUB_GetExtendedHubInfo(HubExtension, ExtendedHubInfo);

    if (!NT_SUCCESS(Status))
    {
        ExFreePool(ExtendedHubInfo);
        ExtendedHubInfo = NULL;
    }

    NumberOfBytes = sizeof(USB_HUB_DESCRIPTOR);

    HubDescriptor = ExAllocatePoolWithTag(NonPagedPool,
                                          NumberOfBytes,
                                          USB_HUB_TAG);

    if (!HubDescriptor)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    RtlZeroMemory(HubDescriptor, NumberOfBytes);

    RequestValue = 0;

    while (TRUE)
    {
        while (TRUE)
        {
            BM_REQUEST_TYPE RequestType;

            RequestType.B = 0;//0xA0
            RequestType._BM.Recipient = 0;
            RequestType._BM.Type = 0;
            RequestType._BM.Dir = 0;

            Status = USBH_Transact(HubExtension,
                                   HubDescriptor,
                                   NumberOfBytes,
                                   0,
                                   URB_FUNCTION_CLASS_DEVICE,
                                   RequestType,
                                   USB_REQUEST_GET_DESCRIPTOR,
                                   RequestValue,
                                   0);

            if (NT_SUCCESS(Status))
            {
                break;
            }

            RequestValue = 0x2900; // Hub DescriptorType - 0x29
        }

        if (HubDescriptor->bDescriptorLength <= NumberOfBytes)
        {
            break;
        }

        NumberOfBytes = HubDescriptor->bDescriptorLength;
        ExFreePool(HubDescriptor);

        HubDescriptor = ExAllocatePoolWithTag(NonPagedPool,
                                              NumberOfBytes,
                                              USB_HUB_TAG);

        if (!HubDescriptor)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit;
        }

        RtlZeroMemory(HubDescriptor, NumberOfBytes);
    }

    NumberPorts = HubDescriptor->bNumberOfPorts;

    if (HubExtension->PortData)
    {
        PortData = HubExtension->PortData;

        if (HubDescriptor->bNumberOfPorts)
        {
            for (ix = 0; ix < NumberPorts; ix++)
            {
                PortData[ix].PortStatus.AsULONG = 0;

                if (ExtendedHubInfo)
                {
                    PortData[ix].PortAttributes = ExtendedHubInfo->Port[ix].PortAttributes;
                }
                else
                {
                    PortData[ix].PortAttributes = 0;
                }

                PortData[ix].ConnectionStatus = NoDeviceConnected;

                if (PortData[ix].DeviceObject != NULL)
                {
                    PortData[ix].ConnectionStatus = DeviceConnected;
                }
            }
        }
    }
    else
    {
        PortData = NULL;

        if (HubDescriptor->bNumberOfPorts)
        {
            PortData = ExAllocatePoolWithTag(NonPagedPool,
                                             NumberPorts * sizeof(USBHUB_PORT_DATA),
                                             USB_HUB_TAG);
        }

        if (!PortData)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit;
        }

        RtlZeroMemory(PortData, NumberPorts * sizeof(USBHUB_PORT_DATA));

        if (NumberPorts)
        {
            for (ix = 0; ix < NumberPorts; ix++)
            {
                PortData[ix].ConnectionStatus = NoDeviceConnected;

                if (ExtendedHubInfo)
                {
                    PortData[ix].PortAttributes = ExtendedHubInfo->Port[ix].PortAttributes;
                }
            }
       }
    }

    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit;
    }

    HubExtension->HubDescriptor = HubDescriptor;

    HubExtension->PortData = PortData;

    if (ExtendedHubInfo)
    {
        ExFreePool(ExtendedHubInfo);
    }

    return Status;

ErrorExit:

    if (HubDescriptor)
    {
        ExFreePool(HubDescriptor);
    }

    if (ExtendedHubInfo)
    {
        ExFreePool(ExtendedHubInfo);
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_SyncGetStringDescriptor(IN PDEVICE_OBJECT DeviceObject,
                             IN UCHAR Index,
                             IN USHORT LanguageId,
                             IN PUSB_STRING_DESCRIPTOR Descriptor,
                             IN ULONG NumberOfBytes,
                             IN PULONG OutLength,
                             IN BOOLEAN IsValidateLength)
{
    struct _URB_CONTROL_DESCRIPTOR_REQUEST * Urb;
    ULONG TransferedLength;
    NTSTATUS Status;

    DPRINT("USBH_SyncGetStringDescriptor: Index - %x, LanguageId - %x\n",
           Index,
           LanguageId);

    Urb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                USB_HUB_TAG);

    if (!Urb)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Urb, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    Urb->Hdr.Function = URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE;
    Urb->Hdr.Length = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);

    Urb->TransferBuffer = Descriptor;
    Urb->TransferBufferLength = NumberOfBytes;

    Urb->Index = Index;
    Urb->DescriptorType = USB_STRING_DESCRIPTOR_TYPE;
    Urb->LanguageId = LanguageId;

    Status = USBH_SyncSubmitUrb(DeviceObject, (PURB)Urb);

    if (!NT_SUCCESS(Status))
    {
        ExFreePool(Urb);
        return Status;
    }

    TransferedLength = Urb->TransferBufferLength;

    if (TransferedLength > NumberOfBytes)
    {
        Status = STATUS_DEVICE_DATA_ERROR;
    }

    if (!NT_SUCCESS(Status))
    {
        ExFreePool(Urb);
        return Status;
    }

    if (OutLength)
    {
        *OutLength = TransferedLength;
    }

    if (IsValidateLength && TransferedLength != Descriptor->bLength)
    {
        Status = STATUS_DEVICE_DATA_ERROR;
    }

    ExFreePool(Urb);

    return Status;
}

NTSTATUS
NTAPI
USBH_SyncGetStatus(IN PDEVICE_OBJECT DeviceObject,
                   IN PUSHORT OutStatus,
                   IN USHORT Function,
                   IN USHORT RequestIndex)
{
    struct _URB_CONTROL_GET_STATUS_REQUEST * Urb;
    NTSTATUS NtStatus;
    USHORT UsbStatus;

    DPRINT("USBH_SyncGetStatus: ... \n");

    Urb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST),
                                USB_HUB_TAG);

    if (!Urb)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Urb, sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST));

    Urb->Hdr.Length = sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST);
    Urb->Hdr.Function = Function;

    Urb->TransferBuffer = &UsbStatus;
    Urb->TransferBufferLength = sizeof(UsbStatus);
    Urb->Index = RequestIndex;

    NtStatus = USBH_FdoSyncSubmitUrb(DeviceObject, (PURB)Urb);

    *OutStatus = UsbStatus;

    ExFreePool(Urb);

    return NtStatus;
}

NTSTATUS
NTAPI
USBH_SyncGetPortStatus(IN PUSBHUB_FDO_EXTENSION HubExtension,
                       IN USHORT Port,
                       IN PUSBHUB_PORT_STATUS PortStatus,
                       IN ULONG Length)
{
    BM_REQUEST_TYPE RequestType;

    DPRINT("USBH_SyncGetPortStatus: Port - %x\n", Port);

    RequestType.B = 0;//0xA3
    RequestType._BM.Recipient = BMREQUEST_TO_OTHER;
    RequestType._BM.Type = BMREQUEST_CLASS;
    RequestType._BM.Dir = BMREQUEST_DEVICE_TO_HOST;

    return USBH_Transact(HubExtension,
                         PortStatus,
                         Length,
                         0, // to host
                         URB_FUNCTION_CLASS_OTHER,
                         RequestType,
                         USB_REQUEST_GET_STATUS,
                         0,
                         Port);
}


NTSTATUS
NTAPI
USBH_SyncClearPortStatus(IN PUSBHUB_FDO_EXTENSION HubExtension,
                         IN USHORT Port,
                         IN USHORT RequestValue)
{
    BM_REQUEST_TYPE RequestType;

    DPRINT("USBH_SyncClearPortStatus: Port - %x, RequestValue - %x\n",
           Port,
           RequestValue);

    RequestType.B = 0;//0x23
    RequestType._BM.Recipient = BMREQUEST_TO_DEVICE;
    RequestType._BM.Type = BMREQUEST_CLASS;
    RequestType._BM.Dir = BMREQUEST_HOST_TO_DEVICE;

    return USBH_Transact(HubExtension,
                         NULL,
                         0,
                         1, // to device
                         URB_FUNCTION_CLASS_OTHER,
                         RequestType,
                         USB_REQUEST_CLEAR_FEATURE,
                         RequestValue,
                         Port);
}

NTSTATUS
NTAPI
USBH_SyncPowerOnPort(IN PUSBHUB_FDO_EXTENSION HubExtension,
                     IN USHORT Port,
                     IN BOOLEAN IsWait)
{
    PUSBHUB_PORT_DATA PortData;
    PUSB_HUB_DESCRIPTOR HubDescriptor;
    NTSTATUS Status;
    BM_REQUEST_TYPE RequestType;

    DPRINT("USBH_SyncPowerOnPort: Port - %x, IsWait - %x\n", Port, IsWait);

    PortData = &HubExtension->PortData[Port - 1];

    HubDescriptor = HubExtension->HubDescriptor;

    if (PortData->PortStatus.UsbPortStatus.ConnectStatus)
    {
        return STATUS_SUCCESS;
    }

    RequestType.B = 0;
    RequestType._BM.Recipient = BMREQUEST_TO_DEVICE;
    RequestType._BM.Type = BMREQUEST_CLASS;
    RequestType._BM.Dir = BMREQUEST_HOST_TO_DEVICE;

    Status = USBH_Transact(HubExtension,
                           NULL,
                           0,
                           1,
                           URB_FUNCTION_CLASS_OTHER,
                           RequestType,
                           USB_REQUEST_SET_FEATURE,
                           USBHUB_FEATURE_PORT_POWER,
                           Port);

    if (NT_SUCCESS(Status))
    {
        if (IsWait)
        {
            USBH_Wait(2 * HubDescriptor->bPowerOnToPowerGood);
        }

        PortData->PortStatus.UsbPortStatus.ConnectStatus = 1;
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_SyncPowerOnPorts(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PUSB_HUB_DESCRIPTOR HubDescriptor;
    ULONG NumberOfPorts;
    ULONG Port;
    NTSTATUS Status;

    DPRINT("USBH_SyncPowerOnPorts: ... \n");

    HubDescriptor = HubExtension->HubDescriptor;
    NumberOfPorts = HubDescriptor->bNumberOfPorts;

    Port = 1;

    if (HubDescriptor->bNumberOfPorts)
    {
        do
        {
            Status = USBH_SyncPowerOnPort(HubExtension, Port, 0);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("USBH_SyncPowerOnPorts: USBH_SyncPowerOnPort() failed - %p\n",
                        Status);
                break;
            }

            ++Port;
        }
        while (Port <= NumberOfPorts);
    }

    USBH_Wait(2 * HubDescriptor->bPowerOnToPowerGood);

    return Status;
}

NTSTATUS
NTAPI
USBH_SyncDisablePort(IN PUSBHUB_FDO_EXTENSION HubExtension,
                     IN USHORT Port)
{
    PUSBHUB_PORT_DATA PortData;
    NTSTATUS Status;
    BM_REQUEST_TYPE RequestType;

    DPRINT("USBH_SyncDisablePort ... \n");

    PortData = &HubExtension->PortData[Port - 1];

    RequestType.B = 0;//0x23
    RequestType._BM.Recipient = BMREQUEST_TO_DEVICE;
    RequestType._BM.Type = BMREQUEST_CLASS;
    RequestType._BM.Dir = BMREQUEST_HOST_TO_DEVICE;

    Status = USBH_Transact(HubExtension,
                           NULL,
                           0,
                           1, // to device
                           URB_FUNCTION_CLASS_OTHER,
                           RequestType,
                           USB_REQUEST_CLEAR_FEATURE,
                           USBHUB_FEATURE_PORT_ENABLE,
                           Port);

    if (NT_SUCCESS(Status))
    {
        PortData->PortStatus.UsbPortStatus.EnableStatus = 0;
    }

    return Status;
}

BOOLEAN
NTAPI
USBH_HubIsBusPowered(IN PDEVICE_OBJECT DeviceObject,
                     IN PUSB_CONFIGURATION_DESCRIPTOR HubConfigDescriptor)
{
    BOOLEAN Result;
    USHORT UsbStatus;
    NTSTATUS Status;

    DPRINT("USBH_HubIsBusPowered: ... \n");

    Status = USBH_SyncGetStatus(DeviceObject,
                                &UsbStatus,
                                URB_FUNCTION_GET_STATUS_FROM_DEVICE,
                                0);

    if (!NT_SUCCESS(Status))
    {
        Result = (HubConfigDescriptor->bmAttributes & 0xC0) == 0x80;
    }
    else
    {
        Result = ~UsbStatus & 1; //SelfPowered bit from status word
    }

    return Result;
}

NTSTATUS
NTAPI
USBH_ChangeIndicationAckChangeComplete(IN PDEVICE_OBJECT DeviceObject,
                                       IN PIRP Irp,
                                       IN PVOID Context)
{
    PUSBHUB_FDO_EXTENSION HubExtension;
    LONG Event;
    USHORT Port;

    HubExtension = (PUSBHUB_FDO_EXTENSION)Context;

    DPRINT("USBH_ChangeIndicationAckChangeComplete: ... \n");

    Port = HubExtension->Port - 1;
    HubExtension->PortData[Port].PortStatus = HubExtension->PortStatus;

    Event = InterlockedExchange((PLONG)&HubExtension->pResetPortEvent, 0);

    if (Event)
    {
        KeSetEvent((PRKEVENT)Event, EVENT_INCREMENT, FALSE);
    }

    USBH_SubmitStatusChangeTransfer(HubExtension);

    if (!InterlockedDecrement(&HubExtension->ResetRequestCount))
    {
        KeSetEvent(&HubExtension->ResetEvent,
                   EVENT_INCREMENT,
                   FALSE);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
USBH_ChangeIndicationAckChange(IN PUSBHUB_FDO_EXTENSION HubExtension,
                               IN PIRP Irp,
                               IN struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST * Urb,
                               IN USHORT Port,
                               IN USHORT RequestValue)
{
    PIO_STACK_LOCATION IoStack;
    BM_REQUEST_TYPE RequestType;

    DPRINT("USBH_ChangeIndicationAckChange: ... \n");

    Urb->Hdr.Length = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
    Urb->Hdr.Function = URB_FUNCTION_CLASS_OTHER;
    Urb->Hdr.UsbdDeviceHandle = NULL;

    Urb->TransferFlags = USBD_SHORT_TRANSFER_OK;
    Urb->TransferBufferLength = 0;
    Urb->TransferBuffer = NULL;
    Urb->TransferBufferMDL = NULL;
    Urb->UrbLink = NULL;

    RequestType.B = 0;
    RequestType._BM.Recipient = BMREQUEST_TO_OTHER;
    RequestType._BM.Type = BMREQUEST_CLASS;
    RequestType._BM.Dir = BMREQUEST_HOST_TO_DEVICE;

    Urb->RequestTypeReservedBits = RequestType.B;
    Urb->Request = USB_REQUEST_CLEAR_FEATURE;
    Urb->Index = Port;
    Urb->Value = RequestValue;

    IoInitializeIrp(Irp,
                    IoSizeOfIrp(HubExtension->LowerDevice->StackSize),
                    HubExtension->LowerDevice->StackSize);

    IoStack = IoGetNextIrpStackLocation(Irp);

    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.Others.Argument1 = Urb;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine(Irp,
                          USBH_ChangeIndicationAckChangeComplete,
                          HubExtension,
                          TRUE,
                          TRUE,
                          TRUE);

    return IoCallDriver(HubExtension->LowerDevice, Irp);
}

NTSTATUS
NTAPI
USBH_ChangeIndicationProcessChange(IN PDEVICE_OBJECT DeviceObject,
                                   IN PIRP Irp,
                                   IN PVOID Context)
{
    PUSBHUB_FDO_EXTENSION HubExtension;
    PUSBHUB_IO_WORK_ITEM WorkItem;
    USHORT RequestValue;

    HubExtension = (PUSBHUB_FDO_EXTENSION)Context;

    DPRINT("USBH_ChangeIndicationProcessChange: PortStatus - %p\n",
           HubExtension->PortStatus.AsULONG);

    if ((NT_SUCCESS(Irp->IoStatus.Status) ||
        USBD_SUCCESS(HubExtension->SCEWorkerUrb.Hdr.Status)) &&
        (HubExtension->PortStatus.UsbPortStatusChange.ResetStatusChange ||
         HubExtension->PortStatus.UsbPortStatusChange.EnableStatusChange))
    {
        if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
        {
            KeSetEvent(&HubExtension->PendingRequestEvent,
                       EVENT_INCREMENT,
                       FALSE);
        }

        USBH_FreeWorkItem(HubExtension->WorkItemToQueue);

        HubExtension->WorkItemToQueue = NULL;

        if (HubExtension->PortStatus.UsbPortStatusChange.ResetStatusChange)
        {
           RequestValue = USBHUB_FEATURE_C_PORT_RESET;
        }
        else
        {
            RequestValue = USBHUB_FEATURE_C_PORT_ENABLE;
        }

        USBH_ChangeIndicationAckChange(HubExtension,
                                       HubExtension->ResetPortIrp,
                                       &HubExtension->SCEWorkerUrb,
                                       HubExtension->Port,
                                       RequestValue);
    }
    else
    {
        WorkItem = HubExtension->WorkItemToQueue;

        if (!HubExtension->WorkItemToQueue)
        {
            DPRINT1("USBH_ChangeIndicationProcessChange: WorkItem == NULL \n");
            KeBugCheckEx(0xFE, 0xC1, 0, 0, 0);
            //DbgBreakPoint();
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        HubExtension->WorkItemToQueue = NULL;

        USBH_QueueWorkItem(HubExtension, WorkItem);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
USBH_ChangeIndicationQueryChange(IN PUSBHUB_FDO_EXTENSION HubExtension,
                                 IN PIRP Irp,
                                 IN struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST * Urb,
                                 IN USHORT Port)
{
    PUSBHUB_IO_WORK_ITEM WorkItem;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    BM_REQUEST_TYPE RequestType;

    DPRINT("USBH_ChangeIndicationQueryChange: Port - %x\n", Port);

    InterlockedIncrement(&HubExtension->PendingRequestCount);

    if (!Port)
    {
        WorkItem = HubExtension->WorkItemToQueue;
        if (!HubExtension->WorkItemToQueue)
        {
            DPRINT1("USBH_ChangeIndicationProcessChange: WorkItem == NULL \n");
            KeBugCheckEx(0xFE, 0xC2, 0, 0, 0);
            //DbgBreakPoint();
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            HubExtension->WorkItemToQueue = NULL;

            USBH_QueueWorkItem(HubExtension, WorkItem);

            Status = STATUS_SUCCESS;
        }
    }

    Urb->Hdr.Length = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
    Urb->Hdr.UsbdDeviceHandle = NULL;
    Urb->Hdr.Function = URB_FUNCTION_CLASS_OTHER;

    Urb->TransferFlags = USBD_SHORT_TRANSFER_OK | USBD_TRANSFER_DIRECTION_IN;
    Urb->TransferBuffer = &HubExtension->PortStatus;
    Urb->TransferBufferLength = sizeof(HubExtension->PortStatus);
    Urb->TransferBufferMDL = NULL;
    Urb->UrbLink = NULL;

    RequestType.B = 0;
    RequestType._BM.Recipient = BMREQUEST_TO_OTHER;
    RequestType._BM.Type = BMREQUEST_CLASS;
    RequestType._BM.Dir = BMREQUEST_DEVICE_TO_HOST;

    Urb->RequestTypeReservedBits = RequestType.B;
    Urb->Request = USB_REQUEST_GET_STATUS;
    Urb->Value = 0;
    Urb->Index = Port;

    HubExtension->Port = Port;

    IoInitializeIrp(Irp,
                    IoSizeOfIrp(HubExtension->LowerDevice->StackSize),
                    HubExtension->LowerDevice->StackSize);

    IoStack = IoGetNextIrpStackLocation(Irp);

    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.Others.Argument1 = Urb;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine(Irp,
                           USBH_ChangeIndicationProcessChange,
                           HubExtension,
                           TRUE,
                           TRUE,
                           TRUE);

    Status = IoCallDriver(HubExtension->LowerDevice, Irp);

    return Status;
}

VOID
NTAPI
USBH_ProcessPortStateChange(IN PUSBHUB_FDO_EXTENSION HubExtension,
                            IN USHORT Port,
                            IN PUSBHUB_PORT_STATUS PortStatus)
{
    PUSBHUB_PORT_DATA PortData;
    USB_PORT_STATUS_CHANGE PortStatusChange;
    PDEVICE_OBJECT PortDevice;
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    LONG SerialNumber;
    LONG DeviceHandle;
    USHORT RequestValue;
    KIRQL Irql;

    DPRINT("USBH_ProcessPortStateChange ... \n");

    PortData = &HubExtension->PortData[Port - 1];

    PortStatusChange = PortStatus->UsbPortStatusChange;

    if (PortStatusChange.ConnectStatusChange)
    {
        PortData->PortStatus.AsULONG = *(PULONG)PortStatus;

        USBH_SyncClearPortStatus(HubExtension,
                                 Port,
                                 USBHUB_FEATURE_C_PORT_CONNECTION);

        PortData = &HubExtension->PortData[Port - 1];

        PortDevice = PortData->DeviceObject;

        if (!PortDevice)
        {
            IoInvalidateDeviceRelations(HubExtension->LowerPDO, BusRelations);
            return;
        }

        PortExtension = (PUSBHUB_PORT_PDO_EXTENSION)PortDevice->DeviceExtension;

        if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_OVERCURRENT_PORT)
        {
            return;
        }

        KeAcquireSpinLock(&HubExtension->RelationsWorkerSpinLock, &Irql);

        if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_POWER_D3)
        {
            KeReleaseSpinLock(&HubExtension->RelationsWorkerSpinLock, Irql);
            IoInvalidateDeviceRelations(HubExtension->LowerPDO, BusRelations);
            return;
        }

        PortData->DeviceObject = NULL;
        PortData->ConnectionStatus = NoDeviceConnected;

        HubExtension->HubFlags |= USBHUB_FDO_FLAG_STATE_CHANGING;

        InsertTailList(&HubExtension->PdoList, &PortExtension->PortLink);

        KeReleaseSpinLock(&HubExtension->RelationsWorkerSpinLock, Irql);

        SerialNumber = InterlockedExchange((PLONG)&PortExtension->SerialNumber, 0);

        if (SerialNumber)
        {
            ExFreePool((PVOID)SerialNumber);
        }

        DeviceHandle = InterlockedExchange((PLONG)&PortExtension->DeviceHandle, 0);

        if (DeviceHandle)
        {
            USBD_RemoveDeviceEx(HubExtension, (PUSB_DEVICE_HANDLE)DeviceHandle, 0);
            USBH_SyncDisablePort(HubExtension, Port);
        }

        IoInvalidateDeviceRelations(HubExtension->LowerPDO, BusRelations);
    }
    else if (PortStatusChange.EnableStatusChange)
    {
        RequestValue = USBHUB_FEATURE_C_PORT_ENABLE;
        PortData->PortStatus = *PortStatus;
        USBH_SyncClearPortStatus(HubExtension, Port, RequestValue);
        return;
    }
    else if (PortStatusChange.SuspendStatusChange)
    {
        DPRINT1("USBH_ProcessPortStateChange: SuspendStatusChange UNIMPLEMENTED. FIXME. \n");
        DbgBreakPoint();
    }
    else if (PortStatusChange.OverCurrentChange)
    {
        DPRINT1("USBH_ProcessPortStateChange: OverCurrentChange UNIMPLEMENTED. FIXME. \n");
        DbgBreakPoint();
    }
    else if (PortStatusChange.ResetStatusChange)
    {
        RequestValue = USBHUB_FEATURE_C_PORT_RESET;
        PortData->PortStatus = *PortStatus;
        USBH_SyncClearPortStatus(HubExtension, Port, RequestValue);
    }
}

NTSTATUS
NTAPI
USBH_GetPortStatus(IN PUSBHUB_FDO_EXTENSION HubExtension,
                   IN PULONG PortStatus)
{
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;

    DPRINT("USBH_GetPortStatus ... \n");

    *PortStatus = 0;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_GET_PORT_STATUS,
                                        HubExtension->LowerDevice,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &Event,
                                        &IoStatusBlock);

    if (Irp)
    {
        IoStack = IoGetNextIrpStackLocation(Irp);
        IoStack->Parameters.Others.Argument1 = PortStatus;

        Status = IoCallDriver(HubExtension->LowerDevice, Irp);

        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event,
                                  Suspended,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }
        else
        {
            IoStatusBlock.Status = Status;
        }

        Status = IoStatusBlock.Status;
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_EnableParentPort(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PIRP Irp;
    NTSTATUS Status;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;

    DPRINT("USBH_EnableParentPort ... \n");

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_ENABLE_PORT,
                                        HubExtension->LowerDevice,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &Event,
                                        &IoStatusBlock);

    if (Irp)
    {
        Status = IoCallDriver(HubExtension->LowerDevice, Irp);

        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event,
                                  Suspended,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }
        else
        {
            IoStatusBlock.Status = Status;
        }

        Status = IoStatusBlock.Status;
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_ResetInterruptPipe(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    struct _URB_PIPE_REQUEST * Urb;
    NTSTATUS Status;

    DPRINT("USBH_ResetInterruptPipe ... \n");

    Urb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(struct _URB_PIPE_REQUEST),
                                USB_HUB_TAG);

    if (Urb)
    {
        RtlZeroMemory(Urb, sizeof(struct _URB_PIPE_REQUEST));

        Urb->Hdr.Length = sizeof(struct _URB_PIPE_REQUEST);
        Urb->Hdr.Function = URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL;
        Urb->PipeHandle = HubExtension->PipeInfo.PipeHandle;

        Status = USBH_FdoSyncSubmitUrb(HubExtension->Common.SelfDevice,
                                       (PURB)Urb);

        ExFreePool(Urb);
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(Status))
    {
        HubExtension->RequestErrors = 0;
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_ResetHub(IN PUSBHUB_FDO_EXTENSION HubExtension,
              IN ULONG PortStatus)
{
    NTSTATUS Status;

    DPRINT("USBH_ResetHub: ... \n");

    Status = USBH_GetPortStatus(HubExtension, &PortStatus);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (!(PortStatus & USBD_PORT_ENABLED))
    {
        if (PortStatus & USBD_PORT_CONNECTED)
        {
            USBH_EnableParentPort(HubExtension);
        }
    }

    Status = USBH_ResetInterruptPipe(HubExtension);

    return Status;
}

VOID
NTAPI
USBH_ChangeIndicationWorker(IN PUSBHUB_FDO_EXTENSION HubExtension,
                            IN PVOID Context)
{
    PUSBHUB_FDO_EXTENSION LowerHubExtension;
    PUSBHUB_PORT_PDO_EXTENSION LowerPortExtension;
    PUSBHUB_STATUS_CHANGE_CONTEXT WorkItem;
    USBHUB_PORT_STATUS PortStatus;
    ULONG Port = 0;
    NTSTATUS Status;

    DPRINT("USBH_ChangeIndicationWorker ... \n");

    WorkItem = (PUSBHUB_STATUS_CHANGE_CONTEXT)Context;

    KeWaitForSingleObject(&HubExtension->HubSemaphore,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STOPPING)
    {
        KeSetEvent(&HubExtension->StatusChangeEvent,
                   EVENT_INCREMENT,
                   FALSE);

        goto Exit;
    }

    if (!HubExtension->RequestErrors)
    {
        goto Enum;
    }

    DPRINT("USBH_ChangeIndicationWorker: RequestErrors - %x\n",
           HubExtension->RequestErrors);

    if (HubExtension->LowerPDO == HubExtension->RootHubPdo)
    {
        goto Enum;
    }

    LowerPortExtension = (PUSBHUB_PORT_PDO_EXTENSION)HubExtension->LowerPDO->DeviceExtension;

    if (LowerPortExtension->PortPdoFlags & USBHUB_PDO_FLAG_POWER_D1_OR_D2)
    {
        goto Enum;
    }

    LowerHubExtension = LowerPortExtension->HubExtension;

    if (!LowerHubExtension)
    {
        goto Enum;
    }

    Status = USBH_SyncGetPortStatus(LowerHubExtension,
                                    LowerPortExtension->PortNumber,
                                    &PortStatus,
                                    sizeof(USBHUB_PORT_STATUS));

    if (!NT_SUCCESS(Status) || !PortStatus.UsbPortStatus.ConnectStatus)
    {
        HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_REMOVED;

        KeSetEvent(&HubExtension->StatusChangeEvent,
                   EVENT_INCREMENT,
                   FALSE);

        goto Exit;
    }

    if (!(HubExtension->HubFlags & USBHUB_FDO_FLAG_ESD_RECOVERING))
    {
        HubExtension->HubFlags |= USBHUB_FDO_FLAG_ESD_RECOVERING;

        DPRINT1("USBH_ChangeIndicationWorker: USBHUB_FDO_FLAG_ESD_RECOVERING FIXME. \n");
        DbgBreakPoint();

        goto Exit;
    }

Enum:

    if (WorkItem->RequestErrors & 1)
    {
        ULONG PortStatusFlags = 0x200;
        USBH_ResetHub(HubExtension, PortStatusFlags);
    }
    else
    {
        do
        {
            if (IsBitSet(((ULONG)WorkItem + sizeof(USBHUB_STATUS_CHANGE_CONTEXT)), Port))
            {
                break;
            }

            ++Port;
        }
        while (Port <= HubExtension->HubDescriptor->bNumberOfPorts);

        if (Port <= HubExtension->HubDescriptor->bNumberOfPorts)
        {
            if (Port)
            {
                Status = USBH_SyncGetPortStatus(HubExtension,
                                                Port,
                                                &PortStatus,
                                                sizeof(USBHUB_PORT_STATUS));
            }
            else
            {
                DPRINT1("USBH_ChangeIndicationWorker: USBH_SyncGetHubStatus() UNIMPLEMENTED. FIXME. \n");
                DbgBreakPoint();
            }

            if (NT_SUCCESS(Status))
            {
               if (Port)
               {
                   USBH_ProcessPortStateChange(HubExtension,
                                               Port,
                                               &PortStatus);
               }
               else
               {
                    DPRINT1("USBH_ChangeIndicationWorker: USBH_ProcessHubStateChange() UNIMPLEMENTED. FIXME. \n");
                    DbgBreakPoint();
               }
            }
            else
            {
               ++HubExtension->RequestErrors;

               if (HubExtension->RequestErrors > 3)
               {
                   HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_FAILED;
                   goto Exit;
               }
            }
        }
    }

    USBH_SubmitStatusChangeTransfer(HubExtension);

Exit:

    KeReleaseSemaphore(&HubExtension->HubSemaphore,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
    {
        KeSetEvent(&HubExtension->PendingRequestEvent,
                   EVENT_INCREMENT,
                   FALSE);
    }

    if (!InterlockedDecrement((PLONG)&HubExtension->ResetRequestCount))
    {
        KeSetEvent(&HubExtension->ResetEvent,
                   EVENT_INCREMENT,
                   FALSE);

        if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEFER_CHECK_IDLE)
        {
            USBH_CheckHubIdle(HubExtension);
        }
    }
}

NTSTATUS
NTAPI
USBH_ChangeIndication(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp,
                      IN PVOID Context)
{
    PUSBHUB_FDO_EXTENSION HubExtension;
    USBD_STATUS UrbStatus;
    BOOLEAN IsErrors = FALSE;
    PUSBHUB_IO_WORK_ITEM HubWorkItem;
    PUSBHUB_STATUS_CHANGE_CONTEXT HubWorkItemBuffer;
    USHORT NumPorts;
    USHORT Port;
    NTSTATUS Status;
    ULONG_PTR Bitmap;
    ULONG BufferLength;

    HubExtension = (PUSBHUB_FDO_EXTENSION)Context;
    UrbStatus = HubExtension->SCEWorkerUrb.Hdr.Status;

    DPRINT("USBH_ChangeIndication: IrpStatus - %p, UrbStatus - %p, HubFlags - %p\n",
           Irp->IoStatus.Status,
           UrbStatus,
           HubExtension->HubFlags);

    if (NT_ERROR(Irp->IoStatus.Status) || USBD_ERROR(UrbStatus) ||
       (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_FAILED) ||
       (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STOPPING))
    {
        ++HubExtension->RequestErrors;

        IsErrors = TRUE;

        KeSetEvent(&HubExtension->StatusChangeEvent,
                   EVENT_INCREMENT,
                   FALSE);

        if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STOPPING ||
            HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_FAILED ||
            HubExtension->RequestErrors > 3 ||
            Irp->IoStatus.Status == STATUS_DELETE_PENDING)
        {
            DPRINT("USBH_ChangeIndication: HubExtension->RequestErrors - %x\n",
                   HubExtension->RequestErrors);

            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        DPRINT("USBH_ChangeIndication: HubExtension->RequestErrors - %x\n",
               HubExtension->RequestErrors);
    }
    else
    {
        HubExtension->RequestErrors = 0;
    }

    BufferLength = sizeof(USBHUB_STATUS_CHANGE_CONTEXT) +
                   HubExtension->SCEBitmapLength;

    Status = USBH_AllocateWorkItem(HubExtension,
                                   &HubWorkItem,
                                   USBH_ChangeIndicationWorker,
                                   BufferLength,
                                   (PVOID *)&HubWorkItemBuffer,
                                   DelayedWorkQueue);

    if (!NT_SUCCESS(Status))
    {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    RtlZeroMemory(HubWorkItemBuffer, BufferLength);

    HubWorkItemBuffer->RequestErrors = 0;

    if (IsErrors == TRUE)
    {
        HubWorkItemBuffer->RequestErrors = 1;
    }

    if (InterlockedIncrement(&HubExtension->ResetRequestCount) == 1)
    {
        KeResetEvent(&HubExtension->ResetEvent);
    }

    HubWorkItemBuffer->HubExtension = HubExtension;

    Port = 0;

    HubExtension->WorkItemToQueue = HubWorkItem;

    RtlCopyMemory((PVOID)((ULONG)HubWorkItemBuffer + sizeof(USBHUB_STATUS_CHANGE_CONTEXT)),
                  HubExtension->SCEBitmap,
                  HubExtension->SCEBitmapLength);

    NumPorts = HubExtension->HubDescriptor->bNumberOfPorts;

    Bitmap = (ULONG)HubWorkItemBuffer + sizeof(USBHUB_STATUS_CHANGE_CONTEXT);

    do
    {
        if (IsBitSet(Bitmap, Port))
        {
            break;
        }

        ++Port;
    }
    while (Port <= NumPorts);

    if (Port > NumPorts)
    {
       Port = 0;
    }

    Status = USBH_ChangeIndicationQueryChange(HubExtension,
                                              HubExtension->ResetPortIrp,
                                              &HubExtension->SCEWorkerUrb,
                                              Port);

    if (NT_ERROR(Status))
    {
        HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_FAILED;
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
USBH_SubmitStatusChangeTransfer(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PIRP Irp;
    NTSTATUS Status;
    struct _URB_BULK_OR_INTERRUPT_TRANSFER * Urb;
    PIO_STACK_LOCATION IoStack;

    DPRINT("USBH_SubmitStatusChangeTransfer: HubExtension - %p, SCEIrp - %p\n",
           HubExtension,
           HubExtension->SCEIrp);

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_NOT_D0_STATE)
    {
        DPRINT("USBH_SubmitStatusChangeTransfer: USBHUB_FDO_FLAG_NOT_D0_STATE - FALSE\n");
        DPRINT("USBH_SubmitStatusChangeTransfer: HubFlags - %p\n", HubExtension->HubFlags);
        return STATUS_INVALID_DEVICE_STATE;
    }

    Irp = HubExtension->SCEIrp;

    if (!Irp)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    Urb = (struct _URB_BULK_OR_INTERRUPT_TRANSFER *)&HubExtension->SCEWorkerUrb;

    Urb->Hdr.Length = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
    Urb->Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    Urb->Hdr.UsbdDeviceHandle = NULL;

    Urb->PipeHandle = HubExtension->PipeInfo.PipeHandle;
    Urb->TransferFlags = USBD_SHORT_TRANSFER_OK;
    Urb->TransferBuffer = HubExtension->SCEBitmap;
    Urb->TransferBufferLength = HubExtension->SCEBitmapLength;
    Urb->TransferBufferMDL = NULL;
    Urb->UrbLink = NULL;

    IoInitializeIrp(Irp,
                    IoSizeOfIrp(HubExtension->LowerDevice->StackSize),
                    HubExtension->LowerDevice->StackSize);

    IoStack = IoGetNextIrpStackLocation(Irp);

    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.Others.Argument1 = &HubExtension->SCEWorkerUrb;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine(Irp,
                          USBH_ChangeIndication,
                          HubExtension,
                          TRUE,
                          TRUE,
                          TRUE);

    KeResetEvent(&HubExtension->StatusChangeEvent);

    Status = IoCallDriver(HubExtension->LowerDevice, Irp);

    return Status;
}

NTSTATUS
NTAPI
USBD_CreateDeviceEx(IN PUSBHUB_FDO_EXTENSION HubExtension,
                    IN PUSB_DEVICE_HANDLE * OutDeviceHandle,
                    IN USB_PORT_STATUS UsbPortStatus,
                    IN USHORT Port)
{
    PUSB_DEVICE_HANDLE HubDeviceHandle;
    PUSB_BUSIFFN_CREATE_USB_DEVICE CreateUsbDevice;

    DPRINT("USBD_CreateDeviceEx: Port - %x, UsbPortStatus - 0x%04X\n",
           Port,
           UsbPortStatus.AsUSHORT);

    CreateUsbDevice = HubExtension->BusInterface.CreateUsbDevice;

    if (!CreateUsbDevice)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    HubDeviceHandle = USBH_SyncGetDeviceHandle(HubExtension->LowerDevice);

    return CreateUsbDevice(HubExtension->BusInterface.BusContext,
                           OutDeviceHandle,
                           HubDeviceHandle,
                           UsbPortStatus.AsUSHORT,
                           Port);
}

NTSTATUS
NTAPI
USBD_RemoveDeviceEx(IN PUSBHUB_FDO_EXTENSION HubExtension,
                    IN PUSB_DEVICE_HANDLE DeviceHandle,
                    IN ULONG Flags)
{
    PUSB_BUSIFFN_REMOVE_USB_DEVICE RemoveUsbDevice;

    DPRINT("USBD_RemoveDeviceEx: DeviceHandle - %p, Flags - %x\n",
           DeviceHandle,
           Flags);

    RemoveUsbDevice = HubExtension->BusInterface.RemoveUsbDevice;

    if (!RemoveUsbDevice)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    return RemoveUsbDevice(HubExtension->BusInterface.BusContext,
                           DeviceHandle,
                           Flags);
}

NTSTATUS
NTAPI
USBD_InitializeDeviceEx(IN PUSBHUB_FDO_EXTENSION HubExtension,
                        IN PUSB_DEVICE_HANDLE DeviceHandle,
                        IN PUCHAR DeviceDescriptorBuffer,
                        IN ULONG DeviceDescriptorBufferLength,
                        IN PUCHAR ConfigDescriptorBuffer,
                        IN ULONG ConfigDescriptorBufferLength)
{
    NTSTATUS Status;
    PUSB_BUSIFFN_INITIALIZE_USB_DEVICE InitializeUsbDevice;
    PUSB_BUSIFFN_GET_USB_DESCRIPTORS GetUsbDescriptors;

    DPRINT("USBD_InitializeDeviceEx: ... \n");

    InitializeUsbDevice = HubExtension->BusInterface.InitializeUsbDevice;
    GetUsbDescriptors = HubExtension->BusInterface.GetUsbDescriptors;

    if (!InitializeUsbDevice || !GetUsbDescriptors)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    Status = InitializeUsbDevice(HubExtension->BusInterface.BusContext,
                                 DeviceHandle);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    return GetUsbDescriptors(HubExtension->BusInterface.BusContext,
                             DeviceHandle,
                             DeviceDescriptorBuffer,
                             &DeviceDescriptorBufferLength,
                             ConfigDescriptorBuffer,
                             &ConfigDescriptorBufferLength);
}

VOID
NTAPI
USBHUB_SetDeviceHandleData(IN PUSBHUB_FDO_EXTENSION HubExtension,
                           IN PDEVICE_OBJECT UsbDevicePdo,
                           IN PVOID DeviceHandle)
{
    PUSB_BUSIFFN_SET_DEVHANDLE_DATA SetDeviceHandleData;

    DPRINT("USBHUB_SetDeviceHandleData ... \n");

    SetDeviceHandleData = HubExtension->BusInterface.SetDeviceHandleData;

    if (!SetDeviceHandleData)
    {
        return;
    }

    SetDeviceHandleData(HubExtension->BusInterface.BusContext,
                        DeviceHandle,
                        UsbDevicePdo);
}

VOID
NTAPI
USBHUB_FlushAllTransfers(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PUSB_BUSIFFN_FLUSH_TRANSFERS FlushTransfers;

    DPRINT("USBHUB_FlushAllTransfers ... \n");

    FlushTransfers = HubExtension->BusInterface.FlushTransfers;

    if (FlushTransfers)
    {
        FlushTransfers(HubExtension->BusInterface.BusContext, NULL);
    }
}

NTSTATUS
NTAPI
USBD_GetDeviceInformationEx(IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
                            IN PUSBHUB_FDO_EXTENSION HubExtension,
                            IN PUSB_NODE_CONNECTION_INFORMATION_EX Info,
                            IN ULONG Length,
                            IN PUSB_DEVICE_HANDLE DeviceHandle)
{
    PUSB_BUSIFFN_GET_DEVICE_INFORMATION QueryDeviceInformation;
    PUSB_DEVICE_INFORMATION_0 DeviceInfo;
    SIZE_T DeviceInfoLength;
    PUSB_NODE_CONNECTION_INFORMATION_EX NodeInfo;
    SIZE_T NodeInfoLength;
    ULONG PipeNumber;
    ULONG dummy;
    NTSTATUS Status;

    DPRINT("USBD_GetDeviceInformationEx ... \n");

    QueryDeviceInformation = HubExtension->BusInterface.QueryDeviceInformation;

    if (!QueryDeviceInformation)
    {
        Status = STATUS_NOT_IMPLEMENTED;
        return Status;
    }

    DeviceInfoLength = sizeof(USB_DEVICE_INFORMATION_0);

    while (TRUE)
    {
        DeviceInfo = ExAllocatePoolWithTag(PagedPool,
                                           DeviceInfoLength,
                                           USB_HUB_TAG);

        if (!DeviceInfo)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(DeviceInfo, DeviceInfoLength);

        DeviceInfo->InformationLevel = 0;

        Status = QueryDeviceInformation(HubExtension->BusInterface.BusContext,
                                        DeviceHandle,
                                        DeviceInfo,
                                        DeviceInfoLength,
                                        &dummy);

        if (Status != STATUS_BUFFER_TOO_SMALL)
        {
            break;
        }

        DeviceInfoLength = DeviceInfo->ActualLength;

        ExFreePool(DeviceInfo);
    }

    NodeInfo = NULL;
    NodeInfoLength = 0;

    if (NT_SUCCESS(Status))
    {
        NodeInfoLength = (sizeof(USB_NODE_CONNECTION_INFORMATION_EX) - sizeof(USB_PIPE_INFO)) +
                         DeviceInfo->NumberOfOpenPipes * sizeof(USB_PIPE_INFO);

        NodeInfo = ExAllocatePoolWithTag(PagedPool, NodeInfoLength, USB_HUB_TAG);

        if (!NodeInfo)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(NodeInfo, NodeInfoLength);

        if (NT_SUCCESS(Status))
        {
            NodeInfo->ConnectionIndex = Info->ConnectionIndex;

            RtlCopyMemory(&NodeInfo->DeviceDescriptor,
                          &DeviceInfo->DeviceDescriptor,
                          sizeof(USB_DEVICE_DESCRIPTOR));

            NodeInfo->CurrentConfigurationValue = DeviceInfo->CurrentConfigurationValue;
            NodeInfo->Speed = DeviceInfo->DeviceSpeed;
            NodeInfo->DeviceIsHub = PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_HUB_DEVICE;
            NodeInfo->DeviceAddress = DeviceInfo->DeviceAddress;
            NodeInfo->NumberOfOpenPipes = DeviceInfo->NumberOfOpenPipes;
            NodeInfo->ConnectionStatus = Info->ConnectionStatus;

            PipeNumber = 0;

            if (DeviceInfo->NumberOfOpenPipes)
            {
                do
                {
                    RtlCopyMemory(&NodeInfo->PipeList[PipeNumber],
                                  &DeviceInfo->PipeList[PipeNumber],
                                  sizeof(USB_PIPE_INFO));

                    ++PipeNumber;
                }
                while (PipeNumber < DeviceInfo->NumberOfOpenPipes);
            }
        }
    }

    ExFreePool(DeviceInfo);

    if (NodeInfo)
    {
        if (NodeInfoLength <= Length)
        {
            Length = NodeInfoLength;
        }
        else
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }

        RtlCopyMemory(Info, NodeInfo, Length);

        ExFreePool(NodeInfo);
    }

    return Status;
}

NTSTATUS
NTAPI
USBD_RestoreDeviceEx(IN PUSBHUB_FDO_EXTENSION HubExtension,
                     IN OUT PUSB_DEVICE_HANDLE OldDeviceHandle,
                     IN OUT PUSB_DEVICE_HANDLE NewDeviceHandle)
{
    PUSB_BUSIFFN_RESTORE_DEVICE RestoreUsbDevice;
    NTSTATUS Status;

    DPRINT("USBD_RestoreDeviceEx: HubExtension - %p, OldDeviceHandle - %p, NewDeviceHandle - %p\n",
           HubExtension,
           OldDeviceHandle,
           NewDeviceHandle);

    RestoreUsbDevice = HubExtension->BusInterface.RestoreUsbDevice;

    if (RestoreUsbDevice)
    {
        Status = RestoreUsbDevice(HubExtension->BusInterface.BusContext,
                                  OldDeviceHandle,
                                  NewDeviceHandle);
    }
    else
    {
        Status = STATUS_NOT_IMPLEMENTED;
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_AllocateWorkItem(PUSBHUB_FDO_EXTENSION HubExtension,
                      PUSBHUB_IO_WORK_ITEM * OutHubIoWorkItem,
                      PVOID WorkerRoutine,
                      SIZE_T BufferLength,
                      PVOID * OutHubWorkItemBuffer,
                      WORK_QUEUE_TYPE Type)
{
    PUSBHUB_IO_WORK_ITEM HubIoWorkItem;
    PIO_WORKITEM WorkItem;
    PVOID WorkItemBuffer;

    DPRINT("USBH_AllocateWorkItem: ... \n");

    if (!(HubExtension->HubFlags & USBHUB_FDO_FLAG_WITEM_INIT))
    {
        return STATUS_INVALID_PARAMETER;
    }

    HubIoWorkItem = ExAllocatePoolWithTag(NonPagedPool,
                                          sizeof(USBHUB_IO_WORK_ITEM),
                                          USB_HUB_TAG);

    if (!HubIoWorkItem)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(HubIoWorkItem, sizeof(USBHUB_IO_WORK_ITEM));

    WorkItem = IoAllocateWorkItem(HubExtension->Common.SelfDevice);

    HubIoWorkItem->HubWorkItem = WorkItem;

    if (!WorkItem)
    {
        ExFreePool(HubIoWorkItem);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (BufferLength && OutHubWorkItemBuffer)
    {
        WorkItemBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                               BufferLength,
                                               USB_HUB_TAG);

        HubIoWorkItem->HubWorkItemBuffer = WorkItemBuffer;

        if (!WorkItemBuffer)
        {
            IoFreeWorkItem(HubIoWorkItem->HubWorkItem);
            ExFreePool(HubIoWorkItem);

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(WorkItemBuffer, BufferLength);
    }
    else
    {
        HubIoWorkItem->HubWorkItemBuffer = NULL;
    }

    HubIoWorkItem->HubWorkItemType = Type;
    HubIoWorkItem->HubExtension = HubExtension;
    HubIoWorkItem->HubWorkerRoutine = WorkerRoutine;

    if (OutHubIoWorkItem)
    {
        *OutHubIoWorkItem = HubIoWorkItem;
    }

    if (OutHubWorkItemBuffer)
    {
        *OutHubWorkItemBuffer = HubIoWorkItem->HubWorkItemBuffer;
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
USBH_Worker(IN PDEVICE_OBJECT DeviceObject,
            IN PVOID Context)
{
    PUSBHUB_IO_WORK_ITEM HubIoWorkItem;
    PUSBHUB_FDO_EXTENSION HubExtension;
    KIRQL OldIrql;
    PIO_WORKITEM WorkItem;

    DPRINT("USBH_Worker: HubIoWorkItem - %p\n", Context);

    HubIoWorkItem = (PUSBHUB_IO_WORK_ITEM)Context;

    InterlockedDecrement(&HubIoWorkItem->HubWorkerQueued);

    HubExtension = HubIoWorkItem->HubExtension;
    WorkItem = HubIoWorkItem->HubWorkItem;

    HubIoWorkItem->HubWorkerRoutine(HubIoWorkItem->HubExtension,
                                    HubIoWorkItem->HubWorkItemBuffer);

    KeAcquireSpinLock(&HubExtension->WorkItemSpinLock, &OldIrql);
    RemoveEntryList(&HubIoWorkItem->HubWorkItemLink);
    KeReleaseSpinLock(&HubExtension->WorkItemSpinLock, OldIrql);

    if (HubIoWorkItem->HubWorkItemBuffer)
    {
        ExFreePool(HubIoWorkItem->HubWorkItemBuffer);
    }

    ExFreePool(HubIoWorkItem);

    if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
    {
        KeSetEvent(&HubExtension->PendingRequestEvent,
                   EVENT_INCREMENT,
                   FALSE);
    }

    IoFreeWorkItem(WorkItem);

    DPRINT("USBH_Worker: HubIoWorkItem %p complete\n", Context);
}

VOID
NTAPI
USBH_QueueWorkItem(IN PUSBHUB_FDO_EXTENSION HubExtension,
                   IN PUSBHUB_IO_WORK_ITEM HubIoWorkItem)
{
    DPRINT("USBH_QueueWorkItem: ... \n");

    InterlockedIncrement(&HubExtension->PendingRequestCount);
    InterlockedIncrement(&HubIoWorkItem->HubWorkerQueued);

    ExInterlockedInsertTailList(&HubExtension->WorkItemList,
                                &HubIoWorkItem->HubWorkItemLink,
                                &HubExtension->WorkItemSpinLock);

    IoQueueWorkItem(HubIoWorkItem->HubWorkItem,
                    USBH_Worker,
                    HubIoWorkItem->HubWorkItemType,
                    HubIoWorkItem);
}

VOID
NTAPI
USBH_FreeWorkItem(IN PUSBHUB_IO_WORK_ITEM HubIoWorkItem)
{
    PIO_WORKITEM WorkItem;

    DPRINT("USBH_FreeWorkItem: ... \n");

    WorkItem = HubIoWorkItem->HubWorkItem;

    if (HubIoWorkItem->HubWorkItemBuffer)
    {
        ExFreePool(HubIoWorkItem->HubWorkItemBuffer);
    }

    ExFreePool(HubIoWorkItem);

    IoFreeWorkItem(WorkItem);
}

VOID
NTAPI
USBHUB_RootHubCallBack(IN PVOID Context)
{
    PUSBHUB_FDO_EXTENSION HubExtension;

    DPRINT("USBHUB_RootHubCallBack: ... \n");

    HubExtension = (PUSBHUB_FDO_EXTENSION)Context;

    if (HubExtension->SCEIrp)
    {
        HubExtension->HubFlags |= (USBHUB_FDO_FLAG_DO_ENUMERATION |
                                   USBHUB_FDO_FLAG_NOT_ENUMERATED);

        USBH_SubmitStatusChangeTransfer(HubExtension);

        IoInvalidateDeviceRelations(HubExtension->LowerPDO, BusRelations);
    }
    else
    {
        HubExtension->HubFlags |= USBHUB_FDO_FLAG_DO_ENUMERATION;
    }

    KeSetEvent(&HubExtension->RootHubNotificationEvent,
               EVENT_INCREMENT,
               FALSE);
}

NTSTATUS
NTAPI
USBD_RegisterRootHubCallBack(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PUSB_BUSIFFN_ROOTHUB_INIT_NOTIFY RootHubInitNotification;

    DPRINT("USBD_RegisterRootHubCallBack: ... \n");

    RootHubInitNotification = HubExtension->BusInterface.RootHubInitNotification;

    if (!RootHubInitNotification)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    KeResetEvent(&HubExtension->RootHubNotificationEvent);

    return RootHubInitNotification(HubExtension->BusInterface.BusContext,
                                   HubExtension,
                                   USBHUB_RootHubCallBack);
}

NTSTATUS
NTAPI
USBD_UnRegisterRootHubCallBack(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PUSB_BUSIFFN_ROOTHUB_INIT_NOTIFY RootHubInitNotification;
    NTSTATUS Status;

    DPRINT("USBD_UnRegisterRootHubCallBack ... \n");

    RootHubInitNotification = HubExtension->BusInterface.RootHubInitNotification;

    if (!RootHubInitNotification)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    Status = RootHubInitNotification(HubExtension->BusInterface.BusContext,
                                     NULL,
                                     NULL);

    if (!NT_SUCCESS(Status))
    {
        KeWaitForSingleObject(&HubExtension->RootHubNotificationEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    return Status;
}

VOID
NTAPI
USBH_HubSetDWakeCompletion(IN PDEVICE_OBJECT DeviceObject,
                           IN UCHAR MinorFunction,
                           IN POWER_STATE PowerState,
                           IN PVOID Context,
                           IN PIO_STATUS_BLOCK IoStatus)
{
    DPRINT("USBH_HubSetDWakeCompletion: ... \n");
    KeSetEvent((PRKEVENT)Context, IO_NO_INCREMENT, FALSE);
}

VOID
NTAPI
USBH_HubQueuePortIdleIrps(IN PUSBHUB_FDO_EXTENSION HubExtension,
                          IN PLIST_ENTRY IdleList)
{
    PDEVICE_OBJECT PortDevice;
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    PIRP IdleIrp;
    PIRP HubIdleIrp;
    ULONG NumPorts;
    ULONG Port;
    KIRQL Irql;

    DPRINT("USBH_HubQueuePortIdleIrps ... \n");

    InitializeListHead(IdleList);

    IoAcquireCancelSpinLock(&Irql);

    NumPorts = HubExtension->HubDescriptor->bNumberOfPorts;

    if (NumPorts)
    {
        Port = 0;

        do
        {
            PortDevice = HubExtension->PortData[Port].DeviceObject;

            if (PortDevice)
            {
                PortExtension = (PUSBHUB_PORT_PDO_EXTENSION)PortDevice->DeviceExtension;

                IdleIrp = PortExtension->IdleNotificationIrp;
                PortExtension->IdleNotificationIrp = NULL;

                if (IdleIrp && IoSetCancelRoutine(IdleIrp, NULL))
                {
                    DPRINT1("USBH_HubQueuePortIdleIrps: IdleIrp != NULL. FIXME. \n");
                    DbgBreakPoint();
                }
            }

            ++Port;
        }
        while (Port < NumPorts);
    }

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_WAIT_IDLE_REQUEST)
    {
        HubIdleIrp = HubExtension->PendingIdleIrp;
        HubExtension->PendingIdleIrp = NULL;
    }
    else
    {
        HubIdleIrp = NULL;
    }

    IoReleaseCancelSpinLock(Irql);

    if (HubIdleIrp)
    {
        USBH_HubCancelIdleIrp(HubExtension, HubIdleIrp);
    }
}

VOID
NTAPI
USBH_HubCompleteQueuedPortIdleIrps(IN PUSBHUB_FDO_EXTENSION HubExtension,
                                   IN PLIST_ENTRY IdleList,
                                   IN NTSTATUS NtStatus)
{
    DPRINT("USBH_HubCompleteQueuedPortIdleIrps ... \n");

    while (!IsListEmpty(IdleList))
    {
        DPRINT1("USBH_HubCompleteQueuedPortIdleIrps: IdleList not Empty. FIXME. \n");
        DbgBreakPoint();
    }
}

VOID
NTAPI
USBH_FlushPortPwrList(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PDEVICE_OBJECT PortDevice;
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    PLIST_ENTRY Entry;
    ULONG Port;

    DPRINT("USBH_FlushPortPwrList ... \n");

    InterlockedIncrement((PLONG)&HubExtension->PendingRequestCount);

    KeWaitForSingleObject(&HubExtension->ResetDeviceSemaphore,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    if (!HubExtension->HubDescriptor->bNumberOfPorts)
    {
        goto Exit;
    }

    Port = 0;

    do
    {
        PortDevice = HubExtension->PortData[Port].DeviceObject;

        if (!PortDevice)
        {
            goto NextPort;
        }

        PortExtension = (PUSBHUB_PORT_PDO_EXTENSION)PortDevice->DeviceExtension;

        InterlockedExchange((PLONG)&PortExtension->StateBehindD2, 0);

        while (TRUE)
        {
            Entry = ExInterlockedRemoveHeadList(&PortExtension->PortPowerList,
                                                &PortExtension->PortPowerListSpinLock);

            if (!Entry)
            {
                break;
            }

            DPRINT1("USBH_FlushPortPwrList: PortPowerList FIXME. \n");
            DbgBreakPoint();
        }

  NextPort:
        ++Port;
    }
    while (Port < HubExtension->HubDescriptor->bNumberOfPorts);

Exit:

    KeReleaseSemaphore(&HubExtension->ResetDeviceSemaphore,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    if (!InterlockedDecrement((PLONG)&HubExtension->PendingRequestCount))
    {
        KeSetEvent(&HubExtension->PendingRequestEvent,
                            EVENT_INCREMENT,
                            FALSE);
    }
}

VOID
NTAPI
USBH_HubCompletePortIdleIrps(IN PUSBHUB_FDO_EXTENSION HubExtension,
                             IN NTSTATUS NtStatus)
{
    LIST_ENTRY IdleList;

    DPRINT("USBH_HubCompletePortIdleIrps ... \n");

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STARTED)
    {
        USBH_HubQueuePortIdleIrps(HubExtension, &IdleList);

        USBH_HubCompleteQueuedPortIdleIrps(HubExtension,
                                           &IdleList,
                                           NtStatus);

        USBH_FlushPortPwrList(HubExtension);
    }
}

VOID
NTAPI
USBH_HubCancelIdleIrp(IN PUSBHUB_FDO_EXTENSION HubExtension, 
                      IN PIRP IdleIrp)
{
    DPRINT("USBH_HubCancelIdleIrp ... \n");

    IoCancelIrp(IdleIrp);

    if (InterlockedExchange(&HubExtension->IdleRequestLock, 1))
    {
        IoFreeIrp(IdleIrp);
    }
}

BOOLEAN
NTAPI
USBH_CheckIdleAbort(IN PUSBHUB_FDO_EXTENSION HubExtension,
                    IN BOOLEAN IsWait,
                    IN BOOLEAN IsExtCheck)
{
    PDEVICE_OBJECT PdoDevice;
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    PUSBHUB_PORT_DATA PortData;
    ULONG Port;
    BOOLEAN Result;

    DPRINT("USBH_CheckIdleAbort: ... \n");

    Result = FALSE;

    InterlockedIncrement(&HubExtension->PendingRequestCount);

    if (IsWait == TRUE)
    {
        KeWaitForSingleObject(&HubExtension->ResetDeviceSemaphore,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    Port = 0;

    if (HubExtension->HubDescriptor->bNumberOfPorts)
    {
        PortData = HubExtension->PortData;

        while (TRUE)
        {
            PdoDevice = PortData[Port].DeviceObject;

            if (PdoDevice)
            {
                PortExtension = (PUSBHUB_PORT_PDO_EXTENSION)PdoDevice->DeviceExtension;

                if (PortExtension->PoRequestCounter)
                {
                    break;
                }
            }

            ++Port;

            if (Port >= HubExtension->HubDescriptor->bNumberOfPorts)
            {
                goto ExtCheck;
            }
        }

        Result = TRUE;
    }
    else
    {

ExtCheck:

        if (IsExtCheck == TRUE &&
            HubExtension->HubDescriptor->bNumberOfPorts)
        {
            PortData = HubExtension->PortData;

            Port = 0;

            do
            {
                PdoDevice = PortData[Port].DeviceObject;

                if (PdoDevice)
                {
                    PortExtension = (PUSBHUB_PORT_PDO_EXTENSION)PdoDevice->DeviceExtension;
                    InterlockedExchange(&PortExtension->StateBehindD2, 0);
                }

                ++Port;
            }
            while (Port < HubExtension->HubDescriptor->bNumberOfPorts);
        }
    }

    if (IsWait == TRUE)
    {
        KeReleaseSemaphore(&HubExtension->ResetDeviceSemaphore,
                           LOW_REALTIME_PRIORITY,
                           1,
                           FALSE);
    }

    if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
    {
        KeSetEvent(&HubExtension->PendingRequestEvent,
                   EVENT_INCREMENT,
                   FALSE);
    }

    return Result;
}

VOID
NTAPI
USBH_FdoWaitWakeIrpCompletion(IN PDEVICE_OBJECT DeviceObject, 
                              IN UCHAR MinorFunction,
                              IN POWER_STATE PowerState,
                              IN PVOID Context,
                              IN PIO_STATUS_BLOCK IoStatus)
{
    DPRINT("USBH_FdoWaitWakeIrpCompletion ... \n");
}

NTSTATUS
NTAPI
USBH_FdoSubmitWaitWakeIrp(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    POWER_STATE PowerState;
    NTSTATUS Status;
    PIRP Irp = NULL;
    KIRQL Irql;

    DPRINT("USBH_FdoSubmitWaitWakeIrp: ... \n");

    PowerState.SystemState = HubExtension->SystemWake;
    HubExtension->HubFlags |= USBHUB_FDO_FLAG_PENDING_WAKE_IRP;

    InterlockedIncrement(&HubExtension->PendingRequestCount);
    InterlockedExchange(&HubExtension->FdoWaitWakeLock, 0);

    Status = PoRequestPowerIrp(HubExtension->LowerPDO,
                               IRP_MN_WAIT_WAKE,
                               PowerState,
                               USBH_FdoWaitWakeIrpCompletion,
                               HubExtension,
                               &Irp);

    IoAcquireCancelSpinLock(&Irql);

    if (Status == STATUS_PENDING)
    {
        if (HubExtension->HubFlags & USBHUB_FDO_FLAG_PENDING_WAKE_IRP)
        {
            HubExtension->PendingWakeIrp = Irp;
            DPRINT("USBH_FdoSubmitWaitWakeIrp: PendingWakeIrp - %p\n",
                   HubExtension->PendingWakeIrp);
        }
    }
    else
    {
        HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_PENDING_WAKE_IRP;

        if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
        {
            KeSetEvent(&HubExtension->PendingRequestEvent,
                       EVENT_INCREMENT,
                       FALSE);
        }
    }

    IoReleaseCancelSpinLock(Irql);

    return Status;
}

VOID
NTAPI
USBH_FdoIdleNotificationCallback(IN PVOID Context)
{
    PUSBHUB_FDO_EXTENSION HubExtension;
    PUSBHUB_PORT_DATA PortData;
    PDEVICE_OBJECT PortDevice;
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    PIRP Irp = NULL;
    PIRP IdleIrp;
    POWER_STATE PowerState;
    KEVENT Event;
    ULONG Port;
    PIO_STACK_LOCATION IoStack;
    PUSB_IDLE_CALLBACK_INFO CallbackInfo;
    BOOLEAN IsReady;
    KIRQL OldIrql;
    NTSTATUS Status;

    HubExtension = (PUSBHUB_FDO_EXTENSION)Context;

    DPRINT("USBH_FdoIdleNotificationCallback: HubExtension - %p, HubFlags - %p\n",
           HubExtension,
           HubExtension->HubFlags);

    IsReady = TRUE;

    if (HubExtension->HubFlags & (USBHUB_FDO_FLAG_ENUM_POST_RECOVER |
                                  USBHUB_FDO_FLAG_WAKEUP_START |
                                  USBHUB_FDO_FLAG_DEVICE_REMOVED |
                                  USBHUB_FDO_FLAG_STATE_CHANGING |
                                  USBHUB_FDO_FLAG_ESD_RECOVERING |
                                  USBHUB_FDO_FLAG_DEVICE_FAILED |
                                  USBHUB_FDO_FLAG_DEVICE_STOPPING))
    {
        DbgBreakPoint();
        return;
    }

    HubExtension->HubFlags |= USBHUB_FDO_FLAG_GOING_IDLE;

    if (!(HubExtension->HubFlags & USBHUB_FDO_FLAG_PENDING_WAKE_IRP))
    {
        Status = USBH_FdoSubmitWaitWakeIrp(HubExtension);

        if (Status != STATUS_PENDING)
        {
            DPRINT("Status != STATUS_PENDING. DbgBreakPoint()\n");
            DbgBreakPoint();
            HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_GOING_IDLE;
            return;
        }
    }

    InterlockedIncrement(&HubExtension->PendingRequestCount);

    KeWaitForSingleObject(&HubExtension->ResetDeviceSemaphore,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    Port = 0;

    if (HubExtension->HubDescriptor->bNumberOfPorts == 0)
    {
        if ((HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STOPPING) ||
            (USBH_CheckIdleAbort(HubExtension, FALSE, FALSE) != TRUE))
        {
            goto IdleHub;
        }
    }
    else
    {
        PortData = HubExtension->PortData;

        while (TRUE)
        {
            PortDevice = PortData[Port].DeviceObject;

            if (PortDevice)
            {
                PortExtension = (PUSBHUB_PORT_PDO_EXTENSION)PortDevice->DeviceExtension;

                IdleIrp = PortExtension->IdleNotificationIrp;

                if (!IdleIrp)
                {
                    break;
                }

                IoStack = IoGetCurrentIrpStackLocation(IdleIrp);

                CallbackInfo = (PUSB_IDLE_CALLBACK_INFO)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

                if (!CallbackInfo)
                {
                    break;
                }

                if (!CallbackInfo->IdleCallback)
                {
                    break;
                }

                if (PortExtension->PendingSystemPoRequest)
                {
                    break;
                }

                if (InterlockedCompareExchange(&PortExtension->StateBehindD2,
                                               1,
                                               0))
                {
                    break;
                }

                DPRINT("USBH_FdoIdleNotificationCallback: IdleContext - %p\n",
                       CallbackInfo->IdleContext);

                CallbackInfo->IdleCallback(CallbackInfo->IdleContext);

                if (PortExtension->CurrentPowerState.DeviceState == PowerDeviceD0)
                {
                    break;
                }
            }

            ++Port;

            if (Port >= HubExtension->HubDescriptor->bNumberOfPorts)
            {
                if ((HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STOPPING) ||
                    (USBH_CheckIdleAbort(HubExtension, FALSE, FALSE) != TRUE))
                {
                    goto IdleHub;
                }
            }
        }
    }

    IsReady = FALSE;

IdleHub:

    KeReleaseSemaphore(&HubExtension->ResetDeviceSemaphore,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
    {
        KeSetEvent(&HubExtension->PendingRequestEvent,
                   EVENT_INCREMENT,
                   FALSE);
    }

    if (!IsReady ||
        (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_SUSPENDED))
    {
        DPRINT1("USBH_FdoIdleNotificationCallback: HubFlags - %p\n",
                HubExtension->HubFlags);

        HubExtension->HubFlags &= ~(USBHUB_FDO_FLAG_DEVICE_SUSPENDED |
                                    USBHUB_FDO_FLAG_GOING_IDLE);

        //Aborting Idle for Hub
        IoAcquireCancelSpinLock(&OldIrql);

        if (HubExtension->PendingIdleIrp)
        {
            Irp = HubExtension->PendingIdleIrp;
            HubExtension->PendingIdleIrp = NULL;
        }

        IoReleaseCancelSpinLock(OldIrql);

        if (Irp)
        {
            USBH_HubCancelIdleIrp(HubExtension, Irp);
        }

        DbgBreakPoint();
        USBH_HubCompletePortIdleIrps(HubExtension, STATUS_CANCELLED);
    }
    else
    {
        PowerState.DeviceState = HubExtension->DeviceWake;

        KeWaitForSingleObject(&HubExtension->IdleSemaphore,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_GOING_IDLE;
        HubExtension->HubFlags |= USBHUB_FDO_FLAG_DO_SUSPENSE;

        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        DPRINT("USBH_FdoIdleNotificationCallback: LowerPdo - %p\n",
                HubExtension->LowerPDO);

        DPRINT("USBH_FdoIdleNotificationCallback: PowerState.DeviceState - %x\n",
               PowerState.DeviceState);

        Status = PoRequestPowerIrp(HubExtension->LowerPDO,
                                   IRP_MN_SET_POWER,
                                   PowerState,
                                   USBH_HubSetDWakeCompletion,
                                   &Event,
                                   0);

        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }
    }
}

VOID
NTAPI
USBH_CompletePortIdleIrpsWorker(IN PUSBHUB_FDO_EXTENSION HubExtension,
                                IN PVOID Context)
{
    PUSBHUB_IDLE_PORT_CONTEXT IdlePortContext;
    NTSTATUS NtStatus;
    NTSTATUS Status;
    BOOLEAN IsFlush = FALSE;

    DPRINT("USBH_CompletePortIdleIrpsWorker ... \n");

    IdlePortContext = (PUSBHUB_IDLE_PORT_CONTEXT)Context;
    NtStatus = IdlePortContext->Status;

    USBH_HubCompleteQueuedPortIdleIrps(HubExtension,
                                       &IdlePortContext->PwrList,
                                       NtStatus);

    DPRINT1("USBH_CompletePortIdleIrpsWorker: USBH_RegQueryFlushPortPowerIrpsFlag() UNIMPLEMENTED. FIXME. \n");
    Status = 0xC0000000;// USBH_RegQueryFlushPortPowerIrpsFlag(&IsFlush);

    if (NT_SUCCESS(Status))
    {
        if (IsFlush)
        {
            USBH_FlushPortPwrList(HubExtension);
        }
    }
}

VOID
NTAPI
USBH_IdleCompletePowerHubWorker(IN PUSBHUB_FDO_EXTENSION HubExtension,
                                IN PVOID Context)
{
    PUSBHUB_IDLE_HUB_CONTEXT HubWorkItemBuffer;

    DPRINT("USBH_IdleCompletePowerHubWorker ... \n");

    if (HubExtension &&
        HubExtension->CurrentPowerState.DeviceState != PowerDeviceD0 &&
        HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STARTED)
    {
        USBH_HubSetD0(HubExtension);
    }

    HubWorkItemBuffer = (PUSBHUB_IDLE_HUB_CONTEXT)Context;

    USBH_HubCompletePortIdleIrps(HubExtension, HubWorkItemBuffer->Status);

}

NTSTATUS
NTAPI
USBH_FdoIdleNotificationRequestComplete(IN PDEVICE_OBJECT DeviceObject,
                                        IN PIRP Irp,
                                        IN PVOID Context)
{
    PUSBHUB_FDO_EXTENSION HubExtension;
    NTSTATUS NtStatus;
    LONG IdleIrp;
    KIRQL Irql;
    NTSTATUS Status;
    PUSBHUB_IO_WORK_ITEM HubIoWorkItem;

    IoAcquireCancelSpinLock(&Irql);

    HubExtension = (PUSBHUB_FDO_EXTENSION)Context;
    HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_WAIT_IDLE_REQUEST;

    IdleIrp = InterlockedExchange((PLONG)&HubExtension->PendingIdleIrp, 0);
    DPRINT("USBH_FdoIdleNotificationRequestComplete: IdleIrp - %p\n", IdleIrp);

    if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
    {
        KeSetEvent(&HubExtension->PendingRequestEvent, EVENT_INCREMENT, FALSE);
    }

    IoReleaseCancelSpinLock(Irql);

    NtStatus = Irp->IoStatus.Status;
    DPRINT("USBH_FdoIdleNotificationRequestComplete: NtStatus - %p\n", NtStatus);

    if (!NT_SUCCESS(NtStatus) &&
        NtStatus != STATUS_POWER_STATE_INVALID &&
        !(HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_REMOVED) &&
        !(HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STOPPED))
    {
        DPRINT("USBH_FdoIdleNotificationRequestComplete: DeviceState - %x\n",
               HubExtension->CurrentPowerState.DeviceState);

        if (HubExtension->CurrentPowerState.DeviceState == PowerDeviceD0)
        {
            PUSBHUB_IDLE_PORT_CONTEXT HubWorkItemBuffer;

            Status = USBH_AllocateWorkItem(HubExtension,
                                           &HubIoWorkItem,
                                           USBH_CompletePortIdleIrpsWorker,
                                           sizeof(USBHUB_IDLE_PORT_CONTEXT),
                                           (PVOID *)&HubWorkItemBuffer,
                                           DelayedWorkQueue);

            if (NT_SUCCESS(Status))
            {
                HubWorkItemBuffer->Status = NtStatus;

                USBH_HubQueuePortIdleIrps(HubExtension,
                                          &HubWorkItemBuffer->PwrList);

                USBH_QueueWorkItem(HubExtension, HubIoWorkItem);
            }
        }
        else
        {
            PUSBHUB_IDLE_HUB_CONTEXT HubWorkItemBuffer;

            Status = USBH_AllocateWorkItem(HubExtension,
                                           &HubIoWorkItem,
                                           USBH_IdleCompletePowerHubWorker,
                                           sizeof(USBHUB_IDLE_HUB_CONTEXT),
                                           (PVOID *)&HubWorkItemBuffer,
                                           DelayedWorkQueue);

            if (NT_SUCCESS(Status))
            {
                HubWorkItemBuffer->Status = NtStatus;
                USBH_QueueWorkItem(HubExtension, HubIoWorkItem);
            }
        }
    }

    if (IdleIrp ||
        InterlockedExchange((PLONG)&HubExtension->IdleRequestLock, 1))
    {
        DPRINT("USBH_FdoIdleNotificationRequestComplete: Irp - %p\n", Irp);
        IoFreeIrp(Irp);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
USBH_FdoSubmitIdleRequestIrp(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    NTSTATUS Status;
    ULONG HubFlags;
    PDEVICE_OBJECT LowerPDO;
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    KIRQL Irql;

    DPRINT("USBH_FdoSubmitIdleRequestIrp: HubExtension - %p, PendingIdleIrp - %p\n",
           HubExtension,
           HubExtension->PendingIdleIrp);

    if (HubExtension->PendingIdleIrp)
    {
        Status = STATUS_DEVICE_BUSY;
        KeSetEvent(&HubExtension->IdleEvent, EVENT_INCREMENT, FALSE);
        return Status;
    }

    HubFlags = HubExtension->HubFlags;

    if (HubFlags & USBHUB_FDO_FLAG_DEVICE_STOPPING ||
        HubFlags & USBHUB_FDO_FLAG_DEVICE_REMOVED)
    {
        HubExtension->HubFlags = HubFlags & ~USBHUB_FDO_FLAG_WAIT_IDLE_REQUEST;
        KeSetEvent(&HubExtension->IdleEvent, EVENT_INCREMENT, FALSE);
        return STATUS_DEVICE_REMOVED;
    }

    LowerPDO = HubExtension->LowerPDO;

    HubExtension->IdleCallbackInfo.IdleCallback = USBH_FdoIdleNotificationCallback;
    HubExtension->IdleCallbackInfo.IdleContext = HubExtension;

    Irp = IoAllocateIrp(LowerPDO->StackSize, 0);

    if (!Irp)
    {
        HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_WAIT_IDLE_REQUEST;
        Status = STATUS_INSUFFICIENT_RESOURCES;

        KeSetEvent(&HubExtension->IdleEvent, EVENT_INCREMENT, FALSE);
        return Status;
    }
 
    IoStack = IoGetNextIrpStackLocation(Irp);

    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    IoStack->Parameters.DeviceIoControl.InputBufferLength = sizeof(USB_IDLE_CALLBACK_INFO);
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION;
    IoStack->Parameters.DeviceIoControl.Type3InputBuffer = &HubExtension->IdleCallbackInfo;

    IoSetCompletionRoutine(Irp,
                           USBH_FdoIdleNotificationRequestComplete,
                           HubExtension,
                           TRUE,
                           TRUE,
                           TRUE);

    InterlockedIncrement(&HubExtension->PendingRequestCount);
    InterlockedExchange(&HubExtension->IdleRequestLock, 0);

    HubExtension->HubFlags &= ~(USBHUB_FDO_FLAG_DEVICE_SUSPENDED |
                                USBHUB_FDO_FLAG_GOING_IDLE);

    Status = IoCallDriver(HubExtension->LowerPDO, Irp);

    IoAcquireCancelSpinLock(&Irql);

    if (Status == STATUS_PENDING &&
        HubExtension->HubFlags & USBHUB_FDO_FLAG_WAIT_IDLE_REQUEST)
    {
        HubExtension->PendingIdleIrp = Irp;
    }

    IoReleaseCancelSpinLock(Irql);

    KeSetEvent(&HubExtension->IdleEvent, EVENT_INCREMENT, FALSE);

    return Status;
}

VOID
NTAPI
USBH_CheckHubIdle(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PDEVICE_OBJECT PdoDevice;
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    PUSBHUB_PORT_DATA PortData;
    ULONG HubFlags;
    ULONG Port;
    KIRQL Irql;
    BOOLEAN IsHubIdle = FALSE;
    BOOLEAN IsAllPortsIdle;

    DPRINT("USBH_CheckHubIdle: FIXME !!! HubExtension - %p\n", HubExtension);

return; //HACK: delete it line after fixing Power Manager!!!

    KeAcquireSpinLock(&HubExtension->CheckIdleSpinLock, &Irql);

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_CHECK_IDLE_LOCK)
    {
        KeReleaseSpinLock(&HubExtension->CheckIdleSpinLock, Irql);
        return;
    }

    HubExtension->HubFlags |= USBHUB_FDO_FLAG_CHECK_IDLE_LOCK;
    KeReleaseSpinLock(&HubExtension->CheckIdleSpinLock, Irql);

    if (USBH_GetRootHubExtension(HubExtension)->SystemPowerState.SystemState != PowerSystemWorking)
    {
        KeAcquireSpinLock(&HubExtension->CheckIdleSpinLock, &Irql);
        HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_CHECK_IDLE_LOCK;
        KeReleaseSpinLock(&HubExtension->CheckIdleSpinLock, Irql);
        return;
    }

    HubFlags = HubExtension->HubFlags;
    DPRINT("USBH_CheckHubIdle: HubFlags - %p\n", HubFlags);

    if (HubFlags & USBHUB_FDO_FLAG_DEVICE_STARTED &&
        HubFlags & USBHUB_FDO_FLAG_DO_ENUMERATION)
    {
        if (!(HubFlags & USBHUB_FDO_FLAG_NOT_ENUMERATED) &&
            !(HubFlags & USBHUB_FDO_FLAG_ENUM_POST_RECOVER) &&
            !(HubFlags & USBHUB_FDO_FLAG_DEVICE_FAILED) &&
            !(HubFlags & USBHUB_FDO_FLAG_DEVICE_STOPPING) &&
            !(HubFlags & USBHUB_FDO_FLAG_DEVICE_REMOVED) &&
            !(HubFlags & USBHUB_FDO_FLAG_STATE_CHANGING) &&
            !(HubFlags & USBHUB_FDO_FLAG_WAKEUP_START) &&
            !(HubFlags & USBHUB_FDO_FLAG_ESD_RECOVERING))
        {
            if (HubExtension->ResetRequestCount)
            {
                HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEFER_CHECK_IDLE;
            }
            else
            {
                HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_DEFER_CHECK_IDLE;

                InterlockedIncrement(&HubExtension->PendingRequestCount);

                KeWaitForSingleObject(&HubExtension->ResetDeviceSemaphore,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);

                IoAcquireCancelSpinLock(&Irql);

                IsAllPortsIdle = TRUE;

                Port = 0;

                if (HubExtension->HubDescriptor->bNumberOfPorts)
                {
                    PortData = HubExtension->PortData;

                    while (TRUE)
                    {
                        PdoDevice = PortData[Port].DeviceObject;

                        if (PdoDevice)
                        {
                            PortExtension = (PUSBHUB_PORT_PDO_EXTENSION)PdoDevice->DeviceExtension;

                            if (!PortExtension->IdleNotificationIrp)
                            {
                                DPRINT("USBH_CheckHubIdle: PortExtension - %p\n", PortExtension);
                                break;
                            }
                        }

                        ++Port;

                        if (Port >= HubExtension->HubDescriptor->bNumberOfPorts)
                        {
                            goto HubIdleCheck;
                        }
                    }

                    IsAllPortsIdle = FALSE;
                }
                else
                {
      HubIdleCheck:
                    if (!(HubExtension->HubFlags & USBHUB_FDO_FLAG_WAIT_IDLE_REQUEST))
                    {
                        KeResetEvent(&HubExtension->IdleEvent);
                        HubExtension->HubFlags |= USBHUB_FDO_FLAG_WAIT_IDLE_REQUEST;
                        IsHubIdle = TRUE;
                    }
                }

                IoReleaseCancelSpinLock(Irql);

                KeReleaseSemaphore(&HubExtension->ResetDeviceSemaphore,
                                   LOW_REALTIME_PRIORITY,
                                   1,
                                   FALSE);

                if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
                {
                    KeSetEvent(&HubExtension->PendingRequestEvent,
                               EVENT_INCREMENT,
                               FALSE);
                }

                DPRINT("USBH_CheckHubIdle: IsAllPortsIdle - %x, IsHubIdle - %x\n",
                       IsAllPortsIdle,
                       IsHubIdle);

                if (IsAllPortsIdle && IsHubIdle)
                {
                    USBH_FdoSubmitIdleRequestIrp(HubExtension);
                }
            }
        }
    }

    KeAcquireSpinLock(&HubExtension->CheckIdleSpinLock, &Irql);
    HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_CHECK_IDLE_LOCK;
    KeReleaseSpinLock(&HubExtension->CheckIdleSpinLock, Irql);
}

VOID
NTAPI
USBH_CheckIdleWorker(IN PUSBHUB_FDO_EXTENSION HubExtension,
                     IN PVOID Context)
{
    DPRINT("USBH_CheckIdleWorker: ... \n");
    USBH_CheckHubIdle(HubExtension);
}

VOID
NTAPI
USBH_CheckIdleDeferred(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PUSBHUB_IO_WORK_ITEM HubIoWorkItem;
    NTSTATUS Status;

    DPRINT("USBH_CheckIdleDeferred: HubExtension - %p\n", HubExtension);

    Status = USBH_AllocateWorkItem(HubExtension,
                                   &HubIoWorkItem,
                                   USBH_CheckIdleWorker,
                                   0,
                                   NULL,
                                   DelayedWorkQueue);

    DPRINT("USBH_CheckIdleDeferred: HubIoWorkItem - %p\n", HubIoWorkItem);

    if (NT_SUCCESS(Status))
    {
        USBH_QueueWorkItem(HubExtension, HubIoWorkItem);
    }
}

VOID
NTAPI
USBH_PdoSetCapabilities(IN PUSBHUB_PORT_PDO_EXTENSION PortExtension)
{
    PUSBHUB_FDO_EXTENSION HubExtension;
    ULONG State;
    SYSTEM_POWER_STATE SystemPowerState;
    PDEVICE_POWER_STATE pDeviceState;

    DPRINT("USBH_PdoSetCapabilities ... \n");

    HubExtension = PortExtension->HubExtension;

    PortExtension->Capabilities.Size = 64;
    PortExtension->Capabilities.Version = 1;

    PortExtension->Capabilities.Removable = 1;
    PortExtension->Capabilities.Address = PortExtension->PortNumber;

    if (PortExtension->SerialNumber)
    {
        PortExtension->Capabilities.UniqueID = 1;
    }
    else
    {
        PortExtension->Capabilities.UniqueID = 0;
    }

    PortExtension->Capabilities.RawDeviceOK = 0;

    RtlCopyMemory(PortExtension->Capabilities.DeviceState,
                  HubExtension->DeviceState,
                  (PowerSystemMaximum + 2) * sizeof(POWER_STATE));

    PortExtension->Capabilities.DeviceState[1] = PowerDeviceD0;

    if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_REMOTE_WAKEUP)
    {
        PortExtension->Capabilities.DeviceWake = PowerDeviceD2;

        PortExtension->Capabilities.DeviceD1 = 1;
        PortExtension->Capabilities.DeviceD2 = 1;

        PortExtension->Capabilities.WakeFromD0 = 1;
        PortExtension->Capabilities.WakeFromD1 = 1;
        PortExtension->Capabilities.WakeFromD2 = 1;

        pDeviceState = &PortExtension->Capabilities.DeviceState[2];

        State = 2;

        do
        {
            SystemPowerState = State++;

            if (PortExtension->Capabilities.SystemWake < SystemPowerState)
            {
                *pDeviceState = PowerDeviceD3;
            }
            else
            {
                *pDeviceState = PowerDeviceD2;
            }

            ++pDeviceState;
        }
        while (State <= 5);
    }
    else
    {
        PortExtension->Capabilities.DeviceWake = PowerDeviceD0;
        PortExtension->Capabilities.DeviceState[2] = PowerDeviceD3;
        PortExtension->Capabilities.DeviceState[3] = PowerDeviceD3;
        PortExtension->Capabilities.DeviceState[4] = PowerDeviceD3;
        PortExtension->Capabilities.DeviceState[5] = PowerDeviceD3;
    }
}

NTSTATUS
NTAPI
USBH_ProcessDeviceInformation(IN PUSBHUB_PORT_PDO_EXTENSION PortExtension)
{
    PUSB_INTERFACE_DESCRIPTOR Pid;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor;
    NTSTATUS Status;

    DPRINT("USBH_ProcessDeviceInformation ... \n");

    ConfigDescriptor = NULL;

    RtlZeroMemory(&PortExtension->InterfaceDescriptor,
                  sizeof(PortExtension->InterfaceDescriptor));

    PortExtension->PortPdoFlags &= ~USBHUB_PDO_FLAG_HUB_DEVICE;

    Status = USBH_GetConfigurationDescriptor(PortExtension->Common.SelfDevice,
                                             &ConfigDescriptor);

    if (!NT_SUCCESS(Status))
    {
        if (ConfigDescriptor)
        {
            ExFreePool(ConfigDescriptor);
        }

        return Status;
    }

    PortExtension->PortPdoFlags &= ~USBHUB_PDO_FLAG_REMOTE_WAKEUP;

    if (ConfigDescriptor->bmAttributes & 0x20)
    {
        /* device configuration supports remote wakeup */
        PortExtension->PortPdoFlags |= USBHUB_PDO_FLAG_REMOTE_WAKEUP;
    }

    USBPORT_DumpingDeviceDescriptor(&PortExtension->DeviceDescriptor);
    USBPORT_DumpingConfiguration(ConfigDescriptor);

    //DPRINT("USBH_ProcessDeviceInformation: Class - %x, SubClass - %x, Protocol - %x\n",
    //       PortExtension->DeviceDescriptor.bDeviceClass,
    //       PortExtension->DeviceDescriptor.bDeviceSubClass,
    //       PortExtension->DeviceDescriptor.bDeviceProtocol);

    //DPRINT("USBH_ProcessDeviceInformation: bNumConfigurations - %x, bNumInterfaces - %x\n",
    //       PortExtension->DeviceDescriptor.bNumConfigurations,
    //       ConfigDescriptor->bNumInterfaces);


    /* Enumeration of USB Composite Devices (msdn):
       1) The device class field of the device descriptor (bDeviceClass) must contain a value of zero,
       or the class (bDeviceClass), subclass (bDeviceSubClass), and protocol (bDeviceProtocol)
       fields of the device descriptor must have the values 0xEF, 0x02 and 0x01 respectively,
       as explained in USB Interface Association Descriptor.
       2) The device must have multiple interfaces
       3) The device must have a single configuration.
    */

    if (((PortExtension->DeviceDescriptor.bDeviceClass == USB_DEVICE_CLASS_RESERVED) ||
        (PortExtension->DeviceDescriptor.bDeviceClass == USBC_DEVICE_CLASS_MISCELLANEOUS &&
         PortExtension->DeviceDescriptor.bDeviceSubClass == 0x02 &&
         PortExtension->DeviceDescriptor.bDeviceProtocol == 0x01)) &&
         (ConfigDescriptor->bNumInterfaces > 1) &&
         (PortExtension->DeviceDescriptor.bNumConfigurations < 2))
    {
        DPRINT("USBH_ProcessDeviceInformation: Multi-Interface configuration\n");
        //DbgBreakPoint();

        PortExtension->PortPdoFlags |= USBHUB_PDO_FLAG_MULTI_INTERFACE;

        if (ConfigDescriptor)
        {
            ExFreePool(ConfigDescriptor);
        }

        return Status;
    }

    Pid = USBD_ParseConfigurationDescriptorEx(ConfigDescriptor,
                                              ConfigDescriptor,
                                              -1,
                                              -1,
                                              -1,
                                              -1,
                                              -1);
    if (Pid)
    {
        RtlCopyMemory(&PortExtension->InterfaceDescriptor,
                      Pid,
                      sizeof(PortExtension->InterfaceDescriptor));

        if (Pid->bInterfaceClass == USB_DEVICE_CLASS_HUB)
        {
            PortExtension->PortPdoFlags |= (USBHUB_PDO_FLAG_HUB_DEVICE |
                                            USBHUB_PDO_FLAG_REMOTE_WAKEUP);
        }
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    if (ConfigDescriptor)
    {
        ExFreePool(ConfigDescriptor);
    }

    return Status;
}

BOOLEAN
NTAPI
USBH_CheckDeviceIDUnique(IN PUSBHUB_FDO_EXTENSION HubExtension,
                         IN USHORT idVendor,
                         IN USHORT idProduct,
                         IN PVOID SerialNumber,
                         IN USHORT SN_DescriptorLength)
{
    PDEVICE_OBJECT PortDevice;
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    ULONG Port;
    SIZE_T NumberBytes;

    DPRINT("USBH_CheckDeviceIDUnique: idVendor - 0x%04X, idProduct - 0x%04X\n",
           idVendor,
           idProduct);

    Port = 0;

    if (!HubExtension->HubDescriptor->bNumberOfPorts)
    {
        return TRUE;
    }

    while (TRUE)
    {
        PortDevice = HubExtension->PortData[Port].DeviceObject;

        if (PortDevice)
        {
            PortExtension = (PUSBHUB_PORT_PDO_EXTENSION)PortDevice->DeviceExtension;

            if (PortExtension->DeviceDescriptor.idVendor == idVendor &&
                PortExtension->DeviceDescriptor.idProduct == idProduct &&
                PortExtension->SN_DescriptorLength == SN_DescriptorLength)
            {
                if (PortExtension->SerialNumber)
                {
                    NumberBytes = RtlCompareMemory(PortExtension->SerialNumber,
                                                   SerialNumber,
                                                   SN_DescriptorLength);

                    if (NumberBytes == SN_DescriptorLength)
                    {
                        break;
                    }
                }
            }
        }

        ++Port;

        if (Port >= HubExtension->HubDescriptor->bNumberOfPorts)
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
NTAPI
USBH_ValidateSerialNumberString(IN PUSHORT SerialNumberString)
{
    USHORT Symbol;

    DPRINT("USBH_ValidateSerialNumberString: ... \n");

    for (Symbol = *SerialNumberString; ; Symbol = *SerialNumberString)
    {
        if (!Symbol)
        {
            return TRUE;
        }

        if (Symbol < 0x0020 || Symbol > 0x007F || Symbol == 0x002C) // ','
        {
            break;
        }

        ++SerialNumberString;
    }

    return 0;
}

NTSTATUS
NTAPI
USBH_CheckDeviceLanguage(IN PDEVICE_OBJECT DeviceObject,
                         IN USHORT LanguageId)
{
    PUSB_STRING_DESCRIPTOR Descriptor;
    NTSTATUS Status;
    ULONG NumSymbols;
    ULONG ix = 0;
    PWCHAR pSymbol;
    ULONG Length;

    DPRINT("USBH_CheckDeviceLanguage: LanguageId - 0x%04X\n", LanguageId);

    Descriptor = ExAllocatePoolWithTag(NonPagedPool, 0xFF, USB_HUB_TAG);

    if (!Descriptor)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Descriptor, 0xFF);

    Status = USBH_SyncGetStringDescriptor(DeviceObject,
                                          0,
                                          0,
                                          Descriptor,
                                          0xFF,
                                          &Length,
                                          TRUE);

    if (NT_SUCCESS(Status))
    {
        if (Length >= 2)
        {
            NumSymbols = (Length - 2) >> 1;
        }
        else
        {
            NumSymbols = 0;
        }

        pSymbol = Descriptor->bString;

        if (NumSymbols > 0)
        {
            while (*pSymbol != (WCHAR)LanguageId)
            {
                ++pSymbol;
                ++ix;

                if (ix >= NumSymbols)
                {
                    ExFreePool(Descriptor);
                    return STATUS_NOT_SUPPORTED;
                }
            }

            Status = STATUS_SUCCESS;
        }
    }

    ExFreePool(Descriptor);

    return Status;
}

NTSTATUS
NTAPI
USBH_GetSerialNumberString(IN PDEVICE_OBJECT DeviceObject,
                           IN LPWSTR * OutSerialNumber,
                           IN PUSHORT OutDescriptorLength,
                           IN USHORT LanguageId,
                           IN UCHAR Index)
{
    PUSB_STRING_DESCRIPTOR Descriptor;
    NTSTATUS Status;
    LPWSTR SerialNumberBuffer = NULL;

    DPRINT("USBH_GetSerialNumberString: ... \n");

    *OutSerialNumber = NULL;
    *OutDescriptorLength = 0;

    Descriptor = ExAllocatePoolWithTag(NonPagedPool, 0xFF, USB_HUB_TAG);

    if (!Descriptor)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Descriptor, 0xFF);

    Status = USBH_CheckDeviceLanguage(DeviceObject, LanguageId);

    if (!NT_SUCCESS(Status))
    {
        ExFreePool(Descriptor);
        return Status;
    }

    Status = USBH_SyncGetStringDescriptor(DeviceObject,
                                          Index,
                                          LanguageId,
                                          Descriptor,
                                          0xFF,
                                          0,
                                          TRUE);

    if (NT_SUCCESS(Status) && Descriptor->bLength > 2)
    {
        SerialNumberBuffer = ExAllocatePoolWithTag(PagedPool,
                                                   Descriptor->bLength,
                                                   USB_HUB_TAG);

        if (SerialNumberBuffer)
        {
            RtlZeroMemory(SerialNumberBuffer, Descriptor->bLength);

            RtlCopyMemory(SerialNumberBuffer,
                          Descriptor->bString,
                          Descriptor->bLength - 2);

            *OutSerialNumber = SerialNumberBuffer;
            *OutDescriptorLength = Descriptor->bLength;
        }
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    ExFreePool(Descriptor);

    return Status;
}

NTSTATUS
NTAPI
USBH_CreateDevice(IN PUSBHUB_FDO_EXTENSION HubExtension,
                  IN USHORT Port,
                  IN USB_PORT_STATUS UsbPortStatus,
                  IN ULONG IsWait)
{
    ULONG PdoNumber = 0;
    WCHAR CharDeviceName[64];
    UNICODE_STRING DeviceName;
    PDEVICE_OBJECT DeviceObject = NULL;
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    PUSB_DEVICE_HANDLE DeviceHandle;
    LPWSTR SerialNumberBuffer;
    BOOLEAN IsHsDevice;
    BOOLEAN IsLsDevice;
    BOOLEAN IgnoringHwSerial = FALSE;
    NTSTATUS Status;
    UNICODE_STRING DestinationString;

    DPRINT("USBH_CreateDevice: Port - %x, UsbPortStatus - %p\n",
           Port,
           UsbPortStatus.AsUSHORT);

    do
    {
        swprintf(CharDeviceName, L"\\Device\\USBPDO-%d", PdoNumber);
        RtlInitUnicodeString(&DeviceName, CharDeviceName);

        Status = IoCreateDevice(HubExtension->Common.SelfDevice->DriverObject,
                                sizeof(USBHUB_PORT_PDO_EXTENSION),
                                &DeviceName,
                                FILE_DEVICE_USB,
                                0,
                                FALSE,
                                &DeviceObject);

        ++PdoNumber;
    }
    while (Status == STATUS_OBJECT_NAME_COLLISION);

    if (!NT_SUCCESS(Status))
    {
        HubExtension->PortData[Port-1].DeviceObject = DeviceObject;
        return Status;
    }

    DeviceObject->StackSize = HubExtension->RootHubPdo2->StackSize;

    PortExtension = (PUSBHUB_PORT_PDO_EXTENSION)DeviceObject->DeviceExtension;

    DPRINT("USBH_CreateDevice: PortDevice - %p, <%wZ>\n", DeviceObject, &DeviceName);
    DPRINT("USBH_CreateDevice: PortExtension - %p\n", PortExtension);

    RtlZeroMemory(PortExtension, sizeof(USBHUB_PORT_PDO_EXTENSION));

    PortExtension->Common.ExtensionType = USBH_EXTENSION_TYPE_PORT;
    PortExtension->Common.SelfDevice = DeviceObject;

    PortExtension->HubExtension = HubExtension;
    PortExtension->RootHubExtension = HubExtension;

    PortExtension->PortNumber = Port;
    PortExtension->CurrentPowerState.DeviceState = PowerDeviceD0;
    PortExtension->IgnoringHwSerial = FALSE;

    KeInitializeSpinLock(&PortExtension->PortTimeoutSpinLock);

    InitializeListHead(&PortExtension->PortPowerList);
    KeInitializeSpinLock(&PortExtension->PortPowerListSpinLock);

    PortExtension->PoRequestCounter = 0;
    PortExtension->PendingSystemPoRequest = 0;
    PortExtension->PendingDevicePoRequest = 0;
    PortExtension->StateBehindD2 = 0;

    SerialNumberBuffer = NULL;

    IsHsDevice = UsbPortStatus.HsDeviceAttached; // High-speed Device Attached
    IsLsDevice = UsbPortStatus.LsDeviceAttached; // Low-Speed Device Attached

    if (IsLsDevice == 0)
    {
        if (IsHsDevice)
        {
            PortExtension->PortPdoFlags = USBHUB_PDO_FLAG_PORT_HIGH_SPEED;
        }
    }
    else
    {
        PortExtension->PortPdoFlags = USBHUB_PDO_FLAG_PORT_LOW_SPEED;
    }

    /* Initialize PortExtension->InstanceID */
    RtlInitUnicodeString(&DestinationString, (PCWSTR)&PortExtension->InstanceID);
    DestinationString.MaximumLength = 4 * sizeof(WCHAR);
    Status = RtlIntegerToUnicodeString(Port, 10, &DestinationString);

    DeviceObject->Flags |= DO_POWER_PAGABLE;
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_CreateDevice: IoCreateDevice() failed - %p\n", Status);
        goto ErrorExit;
    }

    Status = USBD_CreateDeviceEx(HubExtension,
                                 &PortExtension->DeviceHandle,
                                 UsbPortStatus,
                                 Port);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_CreateDevice: USBD_CreateDeviceEx() failed - %p\n", Status);
        goto ErrorExit;
    }

    Status = USBH_SyncResetPort(HubExtension, Port);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_CreateDevice: USBH_SyncResetPort() failed - %p\n", Status);
        goto ErrorExit;
    }

    if (IsWait)
    {
        USBH_Wait(50);
    }

    Status = USBD_InitializeDeviceEx(HubExtension,
                                     PortExtension->DeviceHandle,
                                     (PUCHAR)&PortExtension->DeviceDescriptor,
                                     sizeof(USB_DEVICE_DESCRIPTOR),
                                     (PUCHAR)&PortExtension->ConfigDescriptor,
                                     sizeof(USB_CONFIGURATION_DESCRIPTOR));

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_CreateDevice: USBD_InitializeDeviceEx() failed - %p\n", Status);
        PortExtension->DeviceHandle = NULL;
        goto ErrorExit;
    }

    DPRINT1("USBH_RegQueryDeviceIgnoreHWSerNumFlag UNIMPLEMENTED. FIXME. \n");
    //Status = USBH_RegQueryDeviceIgnoreHWSerNumFlag(PortExtension->DeviceDescriptor.idVendor,
    //                                               PortExtension->DeviceDescriptor.idProduct,
    //                                               &IgnoringHwSerial);

    if (TRUE)//Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        IgnoringHwSerial = FALSE;
    }

    if (IgnoringHwSerial)
    {
        PortExtension->IgnoringHwSerial = TRUE;
    }

    if (PortExtension->DeviceDescriptor.iSerialNumber &&
       !PortExtension->IgnoringHwSerial)
    {
        InterlockedExchange((PLONG)&PortExtension->SerialNumber, 0);

        USBH_GetSerialNumberString(PortExtension->Common.SelfDevice,
                                   &SerialNumberBuffer,
                                   &PortExtension->SN_DescriptorLength,
                                   0x0409,
                                   PortExtension->DeviceDescriptor.iSerialNumber);

        if (SerialNumberBuffer)
        {
            if (!USBH_ValidateSerialNumberString((PUSHORT)SerialNumberBuffer))
            {
                ExFreePool(SerialNumberBuffer);
                SerialNumberBuffer = NULL;
            }

            if (SerialNumberBuffer &&
                !USBH_CheckDeviceIDUnique(HubExtension,
                                          PortExtension->DeviceDescriptor.idVendor,
                                          PortExtension->DeviceDescriptor.idProduct,
                                          SerialNumberBuffer,
                                          PortExtension->SN_DescriptorLength))
            {
                ExFreePool(SerialNumberBuffer);
                SerialNumberBuffer = NULL;
            }
        }

        InterlockedExchange((PLONG)&PortExtension->SerialNumber,
                            (LONG)SerialNumberBuffer);
    }

    Status = USBH_ProcessDeviceInformation(PortExtension);

    USBH_PdoSetCapabilities(PortExtension);

    if (NT_SUCCESS(Status))
    {
        goto Exit;
    }

ErrorExit:

    PortExtension->PortPdoFlags |= USBHUB_PDO_FLAG_INIT_PORT_FAILED;

    DeviceHandle = (PUSB_DEVICE_HANDLE)InterlockedExchange((PLONG)&PortExtension->DeviceHandle,
                                                           0);

    if (DeviceHandle)
    {
        USBD_RemoveDeviceEx(HubExtension, DeviceHandle, 0);
    }

    SerialNumberBuffer = (LPWSTR)InterlockedExchange((PLONG)&PortExtension->SerialNumber,
                                                     0);

    if (SerialNumberBuffer)
    {
        ExFreePool(SerialNumberBuffer);
    }

Exit:

    HubExtension->PortData[Port-1].DeviceObject = DeviceObject;
    return Status;
}

NTSTATUS
NTAPI
USBH_ResetDevice(IN PUSBHUB_FDO_EXTENSION HubExtension,
                 IN USHORT Port,
                 IN BOOLEAN IsKeepDeviceData,
                 IN BOOLEAN IsWait)
{
    NTSTATUS Status;
    PUSBHUB_PORT_DATA PortData;
    PDEVICE_OBJECT PortDevice;
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    LONG NewDeviceHandle;
    LONG Handle;
    LONG OldDeviceHandle;
    PUSB_DEVICE_HANDLE * DeviceHandle;
    USBHUB_PORT_STATUS PortStatus;

    DPRINT("USBH_ResetDevice: HubExtension - %p, Port - %x, IsKeepDeviceData - %x, IsWait - %x\n",
           HubExtension,
           Port,
           IsKeepDeviceData,
           IsWait);

    Status = USBH_SyncGetPortStatus(HubExtension,
                                    Port,
                                    &PortStatus,
                                    sizeof(USBHUB_PORT_STATUS));

    if (!NT_SUCCESS(Status) ||
        !(PortStatus.UsbPortStatus.ConnectStatus))
    {
        return STATUS_UNSUCCESSFUL;
    }

    InterlockedIncrement(&HubExtension->PendingRequestCount);

    KeWaitForSingleObject(&HubExtension->ResetDeviceSemaphore,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    PortData = &HubExtension->PortData[Port-1];

    PortDevice = PortData->DeviceObject;

    if (!PortDevice)
    {
        Status = STATUS_INVALID_PARAMETER;

        KeReleaseSemaphore(&HubExtension->ResetDeviceSemaphore,
                           LOW_REALTIME_PRIORITY,
                           1,
                           FALSE);

        if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
        {
            KeSetEvent(&HubExtension->PendingRequestEvent,
                       EVENT_INCREMENT,
                       FALSE);
        }

        return Status;
    }

    PortExtension = (PUSBHUB_PORT_PDO_EXTENSION)PortDevice->DeviceExtension;

    DeviceHandle = &PortExtension->DeviceHandle;
    OldDeviceHandle = InterlockedExchange((PLONG)&PortExtension->DeviceHandle, 0);

    if (OldDeviceHandle)
    {
        if (!(PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_REMOVING_PORT_PDO))
        {
            Status = USBD_RemoveDeviceEx(HubExtension,
                                         (PUSB_DEVICE_HANDLE)OldDeviceHandle,
                                         IsKeepDeviceData);

            PortExtension->PortPdoFlags |= USBHUB_PDO_FLAG_REMOVING_PORT_PDO;
        }
    }
    else
    {
      OldDeviceHandle = 0;
    }

    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit;
    }

    Status = USBH_SyncResetPort(HubExtension, Port);

    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit;
    }

    Status = USBH_SyncGetPortStatus(HubExtension,
                                    Port,
                                    &PortStatus,
                                    sizeof(USBHUB_PORT_STATUS));

    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit;
    }

    Status = USBD_CreateDeviceEx(HubExtension,
                                 DeviceHandle,
                                 PortStatus.UsbPortStatus,
                                 Port);

    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit;
    }

    Status = USBH_SyncResetPort(HubExtension, Port);

    if (IsWait)
    {
        USBH_Wait(50);
    }

    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit;
    }

    Status = USBD_InitializeDeviceEx(HubExtension,
                                     *DeviceHandle,
                                     &PortExtension->DeviceDescriptor.bLength,
                                     sizeof(PortExtension->DeviceDescriptor),
                                     &PortExtension->ConfigDescriptor.bLength,
                                     sizeof(PortExtension->ConfigDescriptor));

    if (NT_SUCCESS(Status))
    {
        if (IsKeepDeviceData)
        {
            Status = USBD_RestoreDeviceEx(HubExtension,
                                          (PUSB_DEVICE_HANDLE)OldDeviceHandle,
                                          *DeviceHandle);

            if (!NT_SUCCESS(Status))
            {
                Handle = InterlockedExchange((PLONG)DeviceHandle, 0);

                USBD_RemoveDeviceEx(HubExtension,
                                    (PUSB_DEVICE_HANDLE)Handle,
                                    0);

                USBH_SyncDisablePort(HubExtension, Port);

                Status = STATUS_NO_SUCH_DEVICE;
            }
        }
        else
        {
            PortExtension->PortPdoFlags &= ~USBHUB_PDO_FLAG_REMOVING_PORT_PDO;
        }

        goto Exit;
    }

    *DeviceHandle = NULL;

ErrorExit:

    NewDeviceHandle = InterlockedExchange((PLONG)DeviceHandle, OldDeviceHandle);

    if (NewDeviceHandle)
    {
        Status = USBD_RemoveDeviceEx(HubExtension,
                                     (PUSB_DEVICE_HANDLE)NewDeviceHandle,
                                     0);
    }

Exit:

    KeReleaseSemaphore(&HubExtension->ResetDeviceSemaphore,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
    {
        KeSetEvent(&HubExtension->PendingRequestEvent,
                   EVENT_INCREMENT,
                   FALSE);
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_PdoDispatch(IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
                 IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    UCHAR MajorFunction;
    BOOLEAN IsCompleteIrp;
    ULONG ControlCode;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    MajorFunction = IoStack->MajorFunction;

    switch (MajorFunction)
    {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
            DPRINT("USBH_PdoDispatch: IRP_MJ_CREATE / IRP_MJ_CLOSE (%d)\n",
                   MajorFunction);
            Status = STATUS_SUCCESS;
            USBH_CompleteIrp(Irp, Status);
            break;

        case IRP_MJ_DEVICE_CONTROL:
            ControlCode = IoStack->Parameters.DeviceIoControl.IoControlCode;
            DPRINT("USBH_PdoDispatch: IRP_MJ_DEVICE_CONTROL ControlCode - %x\n",
                   ControlCode);

            if (ControlCode == 0X2D0C10)//IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER
            {
                Status = STATUS_NOT_SUPPORTED;
                USBH_CompleteIrp(Irp, Status);
                break;
            }

            if (ControlCode == 0x2F0003)//IOCTL_KS_PROPERTY
            {
                DPRINT1("USBH_PdoDispatch: IOCTL_KS_PROPERTY FIXME. \n");
                DbgBreakPoint();
                Status = STATUS_NOT_SUPPORTED;
                USBH_CompleteIrp(Irp, Status);
                break;
            }

            Status = Irp->IoStatus.Status;
            USBH_CompleteIrp(Irp, Status);
            break;

        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            Status = USBH_PdoInternalControl(PortExtension, Irp);
            break;

        case IRP_MJ_PNP:
            Status = USBH_PdoPnP(PortExtension,
                                 Irp,
                                 IoStack->MinorFunction,
                                 &IsCompleteIrp);

            if (IsCompleteIrp)
            {
                USBH_CompleteIrp(Irp, Status);
            }

            break;

        case IRP_MJ_POWER:
            Status = USBH_PdoPower(PortExtension, Irp, IoStack->MinorFunction);
            break;

        case IRP_MJ_SYSTEM_CONTROL:
            DPRINT1("USBH_PdoDispatch: USBH_SystemControl() UNIMPLEMENTED. FIXME\n");
            Status = STATUS_NOT_SUPPORTED;//USBH_PortSystemControl(PortExtension, Irp);
            break;

        default:
            DPRINT("USBH_PdoDispatch: Unhandled MajorFunction - %d\n", MajorFunction);
            Status = Irp->IoStatus.Status;
            USBH_CompleteIrp(Irp, Status);
            break;
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_FdoDispatch(IN PUSBHUB_FDO_EXTENSION HubExtension,
                 IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    UCHAR MajorFunction;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("USBH_FdoDispatch: HubExtension - %p, Irp - %p, MajorFunction - %d\n",
           HubExtension,
           Irp,
           IoStack->MajorFunction);

    MajorFunction = IoStack->MajorFunction;

    switch (MajorFunction)
    {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
            USBH_CompleteIrp(Irp, STATUS_SUCCESS);
            break;

        case IRP_MJ_DEVICE_CONTROL:
            Status = USBH_DeviceControl(HubExtension, Irp);
            break;

        case IRP_MJ_PNP:
            Status = USBH_FdoPnP(HubExtension, Irp, IoStack->MinorFunction);
            break;

        case IRP_MJ_POWER:
            Status = USBH_FdoPower(HubExtension, Irp, IoStack->MinorFunction);
            break;

        case IRP_MJ_SYSTEM_CONTROL:
            DPRINT1("USBH_FdoDispatch: USBH_SystemControl() UNIMPLEMENTED. FIXME\n");
            //Status USBH_SystemControl(HubExtension, Irp);
            break;

        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        default:
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_AddDevice(IN PDRIVER_OBJECT DriverObject,
               IN PDEVICE_OBJECT LowerPDO)
{
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    PUSBHUB_FDO_EXTENSION HubExtension;
    PDEVICE_OBJECT LowerDevice;

    DPRINT("USBH_AddDevice: DriverObject - %p, LowerPDO - %p\n",
           DriverObject,
           LowerPDO);

    DeviceObject = NULL;

    Status = IoCreateDevice(DriverObject,
                            sizeof(USBHUB_FDO_EXTENSION),
                            NULL,
                            0x8600,
                            FILE_AUTOGENERATED_DEVICE_NAME,
                            FALSE,
                            &DeviceObject);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_AddDevice: IoCreateDevice() fail\n");

        if (DeviceObject)
        {
            IoDeleteDevice(DeviceObject);
        }

        return Status;
    }

    DPRINT("USBH_AddDevice: DeviceObject - %p\n", DeviceObject);

    HubExtension = (PUSBHUB_FDO_EXTENSION)DeviceObject->DeviceExtension;
    RtlZeroMemory(HubExtension, sizeof(USBHUB_FDO_EXTENSION));

    HubExtension->Common.ExtensionType = USBH_EXTENSION_TYPE_HUB;

    LowerDevice = IoAttachDeviceToDeviceStack(DeviceObject, LowerPDO);

    if (!LowerDevice)
    {
        DPRINT1("USBH_AddDevice: IoAttachDeviceToDeviceStack() fail\n");

        if (DeviceObject)
        {
            IoDeleteDevice(DeviceObject);
        }

        return STATUS_UNSUCCESSFUL;
    }

    DPRINT("USBH_AddDevice: LowerDevice  - %p\n", LowerDevice);

    HubExtension->Common.SelfDevice = DeviceObject;

    HubExtension->LowerPDO = LowerPDO;
    HubExtension->LowerDevice = LowerDevice;

    KeInitializeSemaphore(&HubExtension->IdleSemaphore, 1, 1);

    DeviceObject->Flags |= DO_POWER_PAGABLE;
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    DPRINT("USBH_AddDevice: call IoWMIRegistrationControl() UNIMPLEMENTED. FIXME. \n");

    return Status;
}

VOID
NTAPI
USBH_DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
    DPRINT("USBH_DriverUnload: UNIMPLEMENTED\n");

    if (GenericUSBDeviceString)
    {
        ExFreePool(GenericUSBDeviceString);
        GenericUSBDeviceString = NULL;
    }
}

NTSTATUS
NTAPI
USBH_HubDispatch(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION DeviceExtension;
    ULONG ExtensionType;
    NTSTATUS Status;


    DeviceExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ExtensionType = DeviceExtension->ExtensionType;

    if (ExtensionType == USBH_EXTENSION_TYPE_HUB)
    {
        DPRINT("USBH_HubDispatch: DeviceObject - %p, Irp - %p\n",
               DeviceObject,
               Irp);

        Status = USBH_FdoDispatch((PUSBHUB_FDO_EXTENSION)DeviceExtension, Irp);
    }
    else if (ExtensionType == USBH_EXTENSION_TYPE_PORT)
    {
        PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
        UCHAR MajorFunction = IoStack->MajorFunction;
        BOOLEAN IsDprint = TRUE;

        if (MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL)
        {
            ULONG ControlCode = IoStack->Parameters.DeviceIoControl.IoControlCode;

            if (ControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB)
            {
                IsDprint = FALSE;
            }
        }

        if (IsDprint)
        {
            DPRINT("USBH_HubDispatch: DeviceObject - %p, Irp - %p\n",
                   DeviceObject,
                   Irp);
        }

        Status = USBH_PdoDispatch((PUSBHUB_PORT_PDO_EXTENSION)DeviceExtension, Irp);
    }
    else
    {
        DPRINT1("USBH_HubDispatch: Unknown ExtensionType - %x\n", ExtensionType);
        DbgBreakPoint();
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_RegQueryGenericUSBDeviceString(PVOID USBDeviceString)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];

    DPRINT("USBH_RegQueryGenericUSBDeviceString ... \n");

    QueryTable[0].QueryRoutine = USBH_GetConfigValue;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = L"GenericUSBDeviceString";
    QueryTable[0].EntryContext = USBDeviceString;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = 0;
    QueryTable[0].DefaultLength = 0;

    return RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                  L"usbflags",
                                  QueryTable,
                                  NULL,
                                  NULL);
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    DPRINT("USBHUB: DriverEntry - %wZ\n", RegistryPath);

    DriverObject->DriverExtension->AddDevice = USBH_AddDevice;
    DriverObject->DriverUnload = USBH_DriverUnload;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = USBH_HubDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = USBH_HubDispatch;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = USBH_HubDispatch;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = USBH_HubDispatch;

    DriverObject->MajorFunction[IRP_MJ_PNP] = USBH_HubDispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER] = USBH_HubDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = USBH_HubDispatch;

    USBH_RegQueryGenericUSBDeviceString(&GenericUSBDeviceString);

    return STATUS_SUCCESS;
}

