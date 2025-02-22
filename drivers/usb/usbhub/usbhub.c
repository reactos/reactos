/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBHub main driver functions
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbhub.h"

#define NDEBUG
#include <debug.h>

#define NDEBUG_USBHUB_SCE
#define NDEBUG_USBHUB_PNP
#include "dbg_uhub.h"

#include <ntddstor.h>

PWSTR GenericUSBDeviceString = NULL;

NTSTATUS
NTAPI
USBH_Wait(IN ULONG Milliseconds)
{
    LARGE_INTEGER Interval;

    DPRINT("USBH_Wait: Milliseconds - %x\n", Milliseconds);
    Interval.QuadPart = -10000LL * Milliseconds - ((ULONGLONG)KeQueryTimeIncrement() - 1);
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

    DPRINT("USBHUB_GetConfigValue: ... \n");

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
        DPRINT1("USBH_CompleteIrp: Irp - %p, CompleteStatus - %X\n",
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
    DPRINT_PNP("USBH_PassIrp: DeviceObject - %p, Irp - %p\n",
               DeviceObject,
               Irp);

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

    HubTimeoutContext = Context;

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
IsBitSet(IN PUCHAR BitMapAddress,
         IN USHORT Bit)
{
    BOOLEAN IsSet;

    IsSet = (BitMapAddress[Bit / 8] & (1 << (Bit & 7))) != 0;
    DPRINT("IsBitSet: Bit - %lX, IsSet - %x\n", Bit, IsSet);
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
        PdoExtension = NULL;
    }

    return (PUSBHUB_PORT_PDO_EXTENSION)PdoExtension;
}

NTSTATUS
NTAPI
USBH_WriteFailReasonID(IN PDEVICE_OBJECT DeviceObject,
                       IN ULONG FailReason)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"FailReasonID");

    DPRINT("USBH_WriteFailReason: ID - %x\n", FailReason);

    Status = IoOpenDeviceRegistryKey(DeviceObject,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_ALL,
                                     &KeyHandle);

    if (NT_SUCCESS(Status))
    {
        ZwSetValueKey(KeyHandle,
                      &ValueName,
                      0,
                      REG_DWORD,
                      &FailReason,
                      sizeof(FailReason));

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

    HubTimeoutContext = DeferredContext;

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
                             IN PCWSTR ValueName,
                             IN PVOID Data,
                             IN ULONG DataSize,
                             IN ULONG Type,
                             IN ULONG DevInstKeyType)
{
    NTSTATUS Status;
    UNICODE_STRING ValueNameString;
    HANDLE KeyHandle;

    DPRINT("USBH_SetPdoRegistryParameter ... \n");

    RtlInitUnicodeString(&ValueNameString, ValueName);

    Status = IoOpenDeviceRegistryKey(DeviceObject,
                                     DevInstKeyType,
                                     STANDARD_RIGHTS_ALL,
                                     &KeyHandle);

    if (NT_SUCCESS(Status))
    {
         ZwSetValueKey(KeyHandle,
                       &ValueNameString,
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
    LARGE_INTEGER DueTime;
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
        return STATUS_INSUFFICIENT_RESOURCES;
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

        DueTime.QuadPart = -5000 * 10000; // Timeout 5 sec.

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

        ExFreePoolWithTag(HubTimeoutContext, USB_HUB_TAG);
    }

    return IoStatusBlock.Status;
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

    HubExtension = FdoDevice->DeviceExtension;
    return USBH_SyncSubmitUrb(HubExtension->LowerDevice, Urb);
}

NTSTATUS
NTAPI
USBH_Transact(IN PUSBHUB_FDO_EXTENSION HubExtension,
              IN PVOID TransferBuffer,
              IN ULONG BufferLen,
              IN BOOLEAN IsDeviceToHost,
              IN USHORT Function,
              IN BM_REQUEST_TYPE RequestType,
              IN UCHAR Request,
              IN USHORT RequestValue,
              IN USHORT RequestIndex)
{
    struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST * Urb;
    ULONG TransferFlags;
    PVOID Buffer = NULL;
    ULONG Length;
    NTSTATUS Status;

    DPRINT("USBH_Transact: ... \n");

    if (BufferLen)
    {
        Length = ALIGN_DOWN_BY(BufferLen + sizeof(ULONG), sizeof(ULONG));

        Buffer = ExAllocatePoolWithTag(NonPagedPool, Length, USB_HUB_TAG);

        if (!Buffer)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(Buffer, Length);
    }

    Urb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
                                USB_HUB_TAG);

    if (!Urb)
    {
        if (Buffer)
        {
            ExFreePoolWithTag(Buffer, USB_HUB_TAG);
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Urb, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));

    if (IsDeviceToHost)
    {
        if (BufferLen)
        {
            RtlZeroMemory(TransferBuffer, BufferLen);
        }

        TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
    }
    else
    {
        if (BufferLen)
        {
            RtlCopyMemory(Buffer, TransferBuffer, BufferLen);
        }

        TransferFlags = USBD_TRANSFER_DIRECTION_OUT;
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

    if (IsDeviceToHost && BufferLen)
    {
        RtlCopyMemory(TransferBuffer, Buffer, BufferLen);
    }

    if (Buffer)
    {
        ExFreePoolWithTag(Buffer, USB_HUB_TAG);
    }

    ExFreePoolWithTag(Urb, USB_HUB_TAG);

    return Status;
}

NTSTATUS
NTAPI
USBH_SyncResetPort(IN PUSBHUB_FDO_EXTENSION HubExtension,
                   IN USHORT Port)
{
    USB_PORT_STATUS_AND_CHANGE PortStatus;
    KEVENT Event;
    LARGE_INTEGER Timeout;
    ULONG ResetRetry = 0;
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
                                    sizeof(USB_PORT_STATUS_AND_CHANGE));

    if (NT_SUCCESS(Status) &&
        (PortStatus.PortStatus.Usb20PortStatus.CurrentConnectStatus == 0))
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    HubExtension->HubFlags |= USBHUB_FDO_FLAG_RESET_PORT_LOCK;

    while (TRUE)
    {
        BM_REQUEST_TYPE RequestType;

        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        InterlockedExchangePointer((PVOID)&HubExtension->pResetPortEvent,
                                   &Event);

        RequestType.B = 0;
        RequestType.Recipient = BMREQUEST_TO_DEVICE;
        RequestType.Type = BMREQUEST_CLASS;
        RequestType.Dir = BMREQUEST_HOST_TO_DEVICE;

        Status = USBH_Transact(HubExtension,
                               NULL,
                               0,
                               BMREQUEST_HOST_TO_DEVICE,
                               URB_FUNCTION_CLASS_OTHER,
                               RequestType,
                               USB_REQUEST_SET_FEATURE,
                               USBHUB_FEATURE_PORT_RESET,
                               Port);

        Timeout.QuadPart = -5000 * 10000;

        if (!NT_SUCCESS(Status))
        {
            InterlockedExchangePointer((PVOID)&HubExtension->pResetPortEvent,
                                       NULL);

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
                                        sizeof(USB_PORT_STATUS_AND_CHANGE));

        if (!NT_SUCCESS(Status) ||
            (PortStatus.PortStatus.Usb20PortStatus.CurrentConnectStatus == 0) ||
            ResetRetry >= USBHUB_RESET_PORT_MAX_RETRY)
        {
            InterlockedExchangePointer((PVOID)&HubExtension->pResetPortEvent,
                                       NULL);

            USBH_Wait(10);
            HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_RESET_PORT_LOCK;

            Status = STATUS_DEVICE_DATA_ERROR;
            goto Exit;
        }

        ResetRetry++;
    }

    Status = USBH_SyncGetPortStatus(HubExtension,
                                    Port,
                                    &PortStatus,
                                    sizeof(USB_PORT_STATUS_AND_CHANGE));

    if ((PortStatus.PortStatus.Usb20PortStatus.CurrentConnectStatus == 0) &&
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

            ExFreePoolWithTag(DeviceInfo, USB_HUB_TAG);
            break;
        }

        DeviceInformationBufferLength = DeviceInfo->ActualLength;
        ExFreePoolWithTag(DeviceInfo, USB_HUB_TAG);
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
    PDEVICE_OBJECT Device;
    PUSBHUB_FDO_EXTENSION RootHubExtension;

    DPRINT("USBH_GetRootHubExtension: HubExtension - %p\n", HubExtension);

    RootHubExtension = HubExtension;

    if (HubExtension->LowerPDO != HubExtension->RootHubPdo)
    {
        Device = HubExtension->RootHubPdo;

        do
        {
            Device = Device->AttachedDevice;
        }
        while (Device->DriverObject != HubExtension->Common.SelfDevice->DriverObject);

        RootHubExtension = Device->DeviceExtension;
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

    return IoStatusBlock.Status;
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

    return IoStatusBlock.Status;
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

    ExFreePoolWithTag(Urb, USB_HUB_TAG);

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

    DeviceExtension = DeviceObject->DeviceExtension;

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
        ExFreePoolWithTag(Urb, USB_HUB_TAG);
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_GetConfigurationDescriptor(IN PDEVICE_OBJECT DeviceObject,
                                IN PUSB_CONFIGURATION_DESCRIPTOR * OutDescriptor)
{
    PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor;
    ULONG ReturnedLen;
    SIZE_T DescriptorLen;
    NTSTATUS Status;

    DPRINT("USBH_GetConfigurationDescriptor: ... \n");

    DescriptorLen = MAXUCHAR;

    while (TRUE)
    {
        ConfigDescriptor = ExAllocatePoolWithTag(NonPagedPool,
                                                 DescriptorLen,
                                                 USB_HUB_TAG);

        if (!ConfigDescriptor)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        Status = USBH_SyncGetDeviceConfigurationDescriptor(DeviceObject,
                                                           ConfigDescriptor,
                                                           DescriptorLen,
                                                           &ReturnedLen);

        if (ReturnedLen < sizeof(USB_CONFIGURATION_DESCRIPTOR))
        {
            Status = STATUS_DEVICE_DATA_ERROR;
        }

        if (!NT_SUCCESS(Status))
        {
            break;
        }

        *OutDescriptor = ConfigDescriptor;

        if (ConfigDescriptor->wTotalLength <= DescriptorLen)
        {
            break;
        }

        DescriptorLen = ConfigDescriptor->wTotalLength;

        ExFreePool(ConfigDescriptor);
        *OutDescriptor = NULL;
    }

    if (NT_SUCCESS(Status))
    {
        if (ReturnedLen < ConfigDescriptor->wTotalLength)
        {
            Status = STATUS_DEVICE_DATA_ERROR;
        }
    }
    else
    {
        if (ConfigDescriptor)
        {
            ExFreePool(ConfigDescriptor);
        }

        *OutDescriptor = NULL;
    }

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
    PUSB_HUB_DESCRIPTOR HubDescriptor = NULL;
    ULONG ix;
    ULONG Retry;

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
        ExFreePoolWithTag(ExtendedHubInfo, USB_HUB_TAG);
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
    Retry = 0;

    while (TRUE)
    {
        while (Retry <= 5)
        {
            BM_REQUEST_TYPE RequestType;

            RequestType.B = 0;
            RequestType.Recipient = BMREQUEST_TO_DEVICE;
            RequestType.Type = BMREQUEST_STANDARD;
            RequestType.Dir = BMREQUEST_DEVICE_TO_HOST;

            Status = USBH_Transact(HubExtension,
                                   HubDescriptor,
                                   NumberOfBytes,
                                   BMREQUEST_DEVICE_TO_HOST,
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

            Retry++;
        }

        if (HubDescriptor->bDescriptorLength <= NumberOfBytes)
        {
            break;
        }

        NumberOfBytes = HubDescriptor->bDescriptorLength;
        ExFreePoolWithTag(HubDescriptor, USB_HUB_TAG);

        if (Retry >= 5)
        {
            Status = STATUS_DEVICE_DATA_ERROR;
            HubDescriptor = NULL;
            goto ErrorExit;
        }

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

        for (ix = 0; ix < NumberPorts; ix++)
        {
            PortData[ix].PortStatus.AsUlong32 = 0;

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

        for (ix = 0; ix < NumberPorts; ix++)
        {
            PortData[ix].ConnectionStatus = NoDeviceConnected;

            if (ExtendedHubInfo)
            {
                PortData[ix].PortAttributes = ExtendedHubInfo->Port[ix].PortAttributes;
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
        ExFreePoolWithTag(ExtendedHubInfo, USB_HUB_TAG);
    }

    return Status;

ErrorExit:

    if (HubDescriptor)
    {
        ExFreePoolWithTag(HubDescriptor, USB_HUB_TAG);
    }

    if (ExtendedHubInfo)
    {
        ExFreePoolWithTag(ExtendedHubInfo, USB_HUB_TAG);
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
        ExFreePoolWithTag(Urb, USB_HUB_TAG);
        return Status;
    }

    TransferedLength = Urb->TransferBufferLength;

    if (TransferedLength > NumberOfBytes)
    {
        Status = STATUS_DEVICE_DATA_ERROR;
    }

    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Urb, USB_HUB_TAG);
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

    ExFreePoolWithTag(Urb, USB_HUB_TAG);

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

    ExFreePoolWithTag(Urb, USB_HUB_TAG);

    return NtStatus;
}

NTSTATUS
NTAPI
USBH_SyncGetHubStatus(IN PUSBHUB_FDO_EXTENSION HubExtension,
                      IN PUSB_HUB_STATUS_AND_CHANGE HubStatus,
                      IN ULONG Length)
{
    BM_REQUEST_TYPE RequestType;

    DPRINT("USBH_SyncGetHubStatus\n");

    RequestType.B = 0;
    RequestType.Recipient = BMREQUEST_TO_DEVICE;
    RequestType.Type = BMREQUEST_CLASS;
    RequestType.Dir = BMREQUEST_DEVICE_TO_HOST;

    return USBH_Transact(HubExtension,
                         HubStatus,
                         Length,
                         BMREQUEST_DEVICE_TO_HOST,
                         URB_FUNCTION_CLASS_DEVICE,
                         RequestType,
                         USB_REQUEST_GET_STATUS,
                         0,
                         0);
}

NTSTATUS
NTAPI
USBH_SyncClearHubStatus(IN PUSBHUB_FDO_EXTENSION HubExtension,
                        IN USHORT RequestValue)
{
    BM_REQUEST_TYPE RequestType;

    DPRINT("USBH_SyncClearHubStatus: RequestValue - %x\n", RequestValue);

    RequestType.B = 0;
    RequestType.Recipient = BMREQUEST_TO_DEVICE;
    RequestType.Type = BMREQUEST_CLASS;
    RequestType.Dir = BMREQUEST_HOST_TO_DEVICE;

    return USBH_Transact(HubExtension,
                         NULL,
                         0,
                         BMREQUEST_HOST_TO_DEVICE,
                         URB_FUNCTION_CLASS_DEVICE,
                         RequestType,
                         USB_REQUEST_CLEAR_FEATURE,
                         RequestValue,
                         0);
}

NTSTATUS
NTAPI
USBH_SyncGetPortStatus(IN PUSBHUB_FDO_EXTENSION HubExtension,
                       IN USHORT Port,
                       IN PUSB_PORT_STATUS_AND_CHANGE PortStatus,
                       IN ULONG Length)
{
    BM_REQUEST_TYPE RequestType;

    DPRINT("USBH_SyncGetPortStatus: Port - %x\n", Port);

    RequestType.B = 0;
    RequestType.Recipient = BMREQUEST_TO_OTHER;
    RequestType.Type = BMREQUEST_CLASS;
    RequestType.Dir = BMREQUEST_DEVICE_TO_HOST;

    return USBH_Transact(HubExtension,
                         PortStatus,
                         Length,
                         BMREQUEST_DEVICE_TO_HOST,
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

    RequestType.B = 0;
    RequestType.Recipient = BMREQUEST_TO_OTHER;
    RequestType.Type = BMREQUEST_CLASS;
    RequestType.Dir = BMREQUEST_HOST_TO_DEVICE;

    return USBH_Transact(HubExtension,
                         NULL,
                         0,
                         BMREQUEST_HOST_TO_DEVICE,
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
    NTSTATUS Status = STATUS_SUCCESS;
    BM_REQUEST_TYPE RequestType;
    PUSB_PORT_STATUS_AND_CHANGE PortStatus;

    DPRINT("USBH_SyncPowerOnPort: Port - %x, IsWait - %x\n", Port, IsWait);

    ASSERT(Port > 0);
    PortData = &HubExtension->PortData[Port - 1];
    PortStatus = &PortData->PortStatus;

    if (PortStatus->PortStatus.Usb20PortStatus.CurrentConnectStatus == 1)
    {
        return Status;
    }

    RequestType.B = 0;
    RequestType.Recipient = BMREQUEST_TO_DEVICE;
    RequestType.Type = BMREQUEST_CLASS;
    RequestType.Dir = BMREQUEST_HOST_TO_DEVICE;

    Status = USBH_Transact(HubExtension,
                           NULL,
                           0,
                           BMREQUEST_HOST_TO_DEVICE,
                           URB_FUNCTION_CLASS_OTHER,
                           RequestType,
                           USB_REQUEST_SET_FEATURE,
                           USBHUB_FEATURE_PORT_POWER,
                           Port);

    if (NT_SUCCESS(Status))
    {
        if (IsWait)
        {
            HubDescriptor = HubExtension->HubDescriptor;
            USBH_Wait(2 * HubDescriptor->bPowerOnToPowerGood);
        }

        PortStatus->PortStatus.Usb20PortStatus.CurrentConnectStatus = 1;
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_SyncPowerOnPorts(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PUSB_HUB_DESCRIPTOR HubDescriptor;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    USHORT Port;
    UCHAR NumberOfPorts;

    DPRINT("USBH_SyncPowerOnPorts: ... \n");

    HubDescriptor = HubExtension->HubDescriptor;
    NumberOfPorts = HubDescriptor->bNumberOfPorts;

    for (Port = 1; Port <= NumberOfPorts; ++Port)
    {
        Status = USBH_SyncPowerOnPort(HubExtension, Port, 0);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("USBH_SyncPowerOnPorts: USBH_SyncPowerOnPort() failed - %lX\n",
                    Status);
            break;
        }
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

    RequestType.B = 0;
    RequestType.Recipient = BMREQUEST_TO_DEVICE;
    RequestType.Type = BMREQUEST_CLASS;
    RequestType.Dir = BMREQUEST_HOST_TO_DEVICE;

    Status = USBH_Transact(HubExtension,
                           NULL,
                           0,
                           BMREQUEST_HOST_TO_DEVICE,
                           URB_FUNCTION_CLASS_OTHER,
                           RequestType,
                           USB_REQUEST_CLEAR_FEATURE,
                           USBHUB_FEATURE_PORT_ENABLE,
                           Port);

    if (NT_SUCCESS(Status))
    {
        PortData->PortStatus.PortStatus.Usb20PortStatus.PortEnabledDisabled = 0;
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
        Result = (HubConfigDescriptor->bmAttributes & USB_CONFIG_POWERED_MASK)
                                                   == USB_CONFIG_BUS_POWERED;
    }
    else
    {
        Result = (UsbStatus & USB_GETSTATUS_SELF_POWERED) == 0;
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
    PVOID Event;
    USHORT Port;

    HubExtension = Context;

    DPRINT_SCE("USBH_ChangeIndicationAckChangeComplete: ... \n");

    ASSERT(HubExtension->Port > 0);
    Port = HubExtension->Port - 1;

    HubExtension->PortData[Port].PortStatus = HubExtension->PortStatus;

    Event = InterlockedExchangePointer((PVOID)&HubExtension->pResetPortEvent,
                                       NULL);

    if (Event)
    {
        KeSetEvent(Event, EVENT_INCREMENT, FALSE);
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

    DPRINT_SCE("USBH_ChangeIndicationAckChange: ... \n");

    Urb->Hdr.Length = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
    Urb->Hdr.Function = URB_FUNCTION_CLASS_OTHER;
    Urb->Hdr.UsbdDeviceHandle = NULL;

    Urb->TransferFlags = USBD_SHORT_TRANSFER_OK;
    Urb->TransferBufferLength = 0;
    Urb->TransferBuffer = NULL;
    Urb->TransferBufferMDL = NULL;
    Urb->UrbLink = NULL;

    RequestType.B = 0;
    RequestType.Recipient = BMREQUEST_TO_OTHER;
    RequestType.Type = BMREQUEST_CLASS;
    RequestType.Dir = BMREQUEST_HOST_TO_DEVICE;

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

    HubExtension = Context;

    DPRINT_SCE("USBH_ChangeIndicationProcessChange: PortStatus - %lX\n",
               HubExtension->PortStatus.AsUlong32);

    if ((NT_SUCCESS(Irp->IoStatus.Status) ||
        USBD_SUCCESS(HubExtension->SCEWorkerUrb.Hdr.Status)) &&
        (HubExtension->PortStatus.PortChange.Usb20PortChange.ResetChange ||
         HubExtension->PortStatus.PortChange.Usb20PortChange.PortEnableDisableChange))
    {
        if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
        {
            KeSetEvent(&HubExtension->PendingRequestEvent,
                       EVENT_INCREMENT,
                       FALSE);
        }

        USBH_FreeWorkItem(HubExtension->WorkItemToQueue);

        HubExtension->WorkItemToQueue = NULL;

        if (HubExtension->PortStatus.PortChange.Usb20PortChange.ResetChange)
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
        ASSERT(HubExtension->WorkItemToQueue != NULL);

        WorkItem = HubExtension->WorkItemToQueue;
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

    DPRINT_SCE("USBH_ChangeIndicationQueryChange: Port - %x\n", Port);

    InterlockedIncrement(&HubExtension->PendingRequestCount);

    if (!Port)
    {
        ASSERT(HubExtension->WorkItemToQueue != NULL);

        WorkItem = HubExtension->WorkItemToQueue;
        HubExtension->WorkItemToQueue = NULL;

        USBH_QueueWorkItem(HubExtension, WorkItem);

        return STATUS_SUCCESS;
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
    RequestType.Recipient = BMREQUEST_TO_OTHER;
    RequestType.Type = BMREQUEST_CLASS;
    RequestType.Dir = BMREQUEST_DEVICE_TO_HOST;

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
USBH_ProcessHubStateChange(IN PUSBHUB_FDO_EXTENSION HubExtension,
                           IN PUSB_HUB_STATUS_AND_CHANGE HubStatus)
{
    USB_HUB_CHANGE HubStatusChange;

    DPRINT_SCE("USBH_ProcessHubStateChange: HubStatus - %lx\n", HubStatus->AsUlong32);

    HubStatusChange = HubStatus->HubChange;

    if (HubStatusChange.LocalPowerChange)
    {
        DPRINT1("USBH_ProcessHubStateChange: LocalPowerChange\n");
        USBH_SyncClearHubStatus(HubExtension,
                                USBHUB_FEATURE_C_HUB_LOCAL_POWER);
    }
    else if (HubStatusChange.OverCurrentChange)
    {
        USBH_SyncClearHubStatus(HubExtension,
                                USBHUB_FEATURE_C_HUB_OVER_CURRENT);
        if (HubStatus->HubStatus.OverCurrent)
        {
            DPRINT1("USBH_ProcessHubStateChange: OverCurrent UNIMPLEMENTED. FIXME\n");
            DbgBreakPoint();
        }
    }
}

VOID
NTAPI
USBH_ProcessPortStateChange(IN PUSBHUB_FDO_EXTENSION HubExtension,
                            IN USHORT Port,
                            IN PUSB_PORT_STATUS_AND_CHANGE PortStatus)
{
    PUSBHUB_PORT_DATA PortData;
    USB_20_PORT_CHANGE PortStatusChange;
    PDEVICE_OBJECT PortDevice;
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    PVOID SerialNumber;
    PVOID DeviceHandle;
    USHORT RequestValue;
    KIRQL Irql;

    DPRINT_SCE("USBH_ProcessPortStateChange ... \n");

    ASSERT(Port > 0);
    PortData = &HubExtension->PortData[Port - 1];

    PortStatusChange = PortStatus->PortChange.Usb20PortChange;

    if (PortStatusChange.ConnectStatusChange)
    {
        PortData->PortStatus = *PortStatus;

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

        PortExtension = PortDevice->DeviceExtension;

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

        SerialNumber = InterlockedExchangePointer((PVOID)&PortExtension->SerialNumber,
                                                  NULL);

        if (SerialNumber)
        {
            ExFreePoolWithTag(SerialNumber, USB_HUB_TAG);
        }

        DeviceHandle = InterlockedExchangePointer(&PortExtension->DeviceHandle,
                                                  NULL);

        if (DeviceHandle)
        {
            USBD_RemoveDeviceEx(HubExtension, DeviceHandle, 0);
            USBH_SyncDisablePort(HubExtension, Port);
        }

        IoInvalidateDeviceRelations(HubExtension->LowerPDO, BusRelations);
    }
    else if (PortStatusChange.PortEnableDisableChange)
    {
        RequestValue = USBHUB_FEATURE_C_PORT_ENABLE;
        PortData->PortStatus = *PortStatus;
        USBH_SyncClearPortStatus(HubExtension, Port, RequestValue);
        return;
    }
    else if (PortStatusChange.SuspendChange)
    {
        DPRINT1("USBH_ProcessPortStateChange: SuspendChange UNIMPLEMENTED. FIXME\n");
        DbgBreakPoint();
    }
    else if (PortStatusChange.OverCurrentIndicatorChange)
    {
        DPRINT1("USBH_ProcessPortStateChange: OverCurrentIndicatorChange UNIMPLEMENTED. FIXME\n");
        DbgBreakPoint();
    }
    else if (PortStatusChange.ResetChange)
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

    if (!Irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

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

    return IoStatusBlock.Status;
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

    if (!Irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

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

    return IoStatusBlock.Status;
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

        ExFreePoolWithTag(Urb, USB_HUB_TAG);
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
USBH_ResetHub(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    NTSTATUS Status;
    ULONG PortStatusFlags = 0;

    DPRINT("USBH_ResetHub: ... \n");

    Status = USBH_GetPortStatus(HubExtension, &PortStatusFlags);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (!(PortStatusFlags & USBD_PORT_ENABLED))
    {
        if (PortStatusFlags & USBD_PORT_CONNECTED)
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
    USB_PORT_STATUS_AND_CHANGE PortStatus;
    USB_HUB_STATUS_AND_CHANGE HubStatus;
    NTSTATUS Status;
    USHORT Port = 0;

    DPRINT_SCE("USBH_ChangeIndicationWorker ... \n");

    WorkItem = Context;

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

    DPRINT_SCE("USBH_ChangeIndicationWorker: RequestErrors - %x\n",
               HubExtension->RequestErrors);

    if (HubExtension->LowerPDO == HubExtension->RootHubPdo)
    {
        goto Enum;
    }

    LowerPortExtension = HubExtension->LowerPDO->DeviceExtension;

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
                                    sizeof(USB_PORT_STATUS_AND_CHANGE));

    if (!NT_SUCCESS(Status) ||
        !PortStatus.PortStatus.Usb20PortStatus.CurrentConnectStatus)
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

        DPRINT1("USBH_ChangeIndicationWorker: USBHUB_FDO_FLAG_ESD_RECOVERING FIXME\n");
        DbgBreakPoint();

        goto Exit;
    }

Enum:

    if (WorkItem->IsRequestErrors)
    {
        USBH_ResetHub(HubExtension);
    }
    else
    {
        for (Port = 0;
             Port < HubExtension->HubDescriptor->bNumberOfPorts;
             Port++)
        {
            if (IsBitSet((PUCHAR)(WorkItem + 1), Port))
            {
                break;
            }
        }

        if (Port)
        {
            Status = USBH_SyncGetPortStatus(HubExtension,
                                            Port,
                                            &PortStatus,
                                            sizeof(PortStatus));
        }
        else
        {
            Status = USBH_SyncGetHubStatus(HubExtension,
                                           &HubStatus,
                                           sizeof(HubStatus));
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
                USBH_ProcessHubStateChange(HubExtension,
                                           &HubStatus);
            }
        }
        else
        {
            HubExtension->RequestErrors++;

            if (HubExtension->RequestErrors > USBHUB_MAX_REQUEST_ERRORS)
            {
                HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_FAILED;
                goto Exit;
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
    PVOID Bitmap;
    ULONG BufferLength;

    HubExtension = Context;
    UrbStatus = HubExtension->SCEWorkerUrb.Hdr.Status;

    DPRINT_SCE("USBH_ChangeIndication: IrpStatus - %x, UrbStatus - %x, HubFlags - %lX\n",
               Irp->IoStatus.Status,
               UrbStatus,
               HubExtension->HubFlags);

    if (NT_ERROR(Irp->IoStatus.Status) || USBD_ERROR(UrbStatus) ||
       (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_FAILED) ||
       (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STOPPING))
    {
        HubExtension->RequestErrors++;

        IsErrors = TRUE;

        KeSetEvent(&HubExtension->StatusChangeEvent,
                   EVENT_INCREMENT,
                   FALSE);

        if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STOPPING ||
            HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_FAILED ||
            HubExtension->RequestErrors > USBHUB_MAX_REQUEST_ERRORS ||
            Irp->IoStatus.Status == STATUS_DELETE_PENDING)
        {
            DPRINT_SCE("USBH_ChangeIndication: HubExtension->RequestErrors - %x\n",
                       HubExtension->RequestErrors);

            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        DPRINT_SCE("USBH_ChangeIndication: HubExtension->RequestErrors - %x\n",
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

    HubWorkItemBuffer->IsRequestErrors = FALSE;

    if (IsErrors)
    {
        HubWorkItemBuffer->IsRequestErrors = TRUE;
    }

    if (InterlockedIncrement(&HubExtension->ResetRequestCount) == 1)
    {
        KeClearEvent(&HubExtension->ResetEvent);
    }

    HubWorkItemBuffer->HubExtension = HubExtension;

    HubExtension->WorkItemToQueue = HubWorkItem;

    Bitmap = HubWorkItemBuffer + 1;

    RtlCopyMemory(Bitmap,
                  HubExtension->SCEBitmap,
                  HubExtension->SCEBitmapLength);

    NumPorts = HubExtension->HubDescriptor->bNumberOfPorts;

    for (Port = 0; Port <= NumPorts; ++Port)
    {
        if (IsBitSet(Bitmap, Port))
        {
            break;
        }
    }

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

    DPRINT_SCE("USBH_SubmitStatusChangeTransfer: HubExtension - %p, SCEIrp - %p\n",
               HubExtension,
               HubExtension->SCEIrp);

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_NOT_D0_STATE)
    {
        DPRINT_SCE("USBH_SubmitStatusChangeTransfer: USBHUB_FDO_FLAG_NOT_D0_STATE\n");
        DPRINT_SCE("USBH_SubmitStatusChangeTransfer: HubFlags - %lX\n",
                   HubExtension->HubFlags);

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

    KeClearEvent(&HubExtension->StatusChangeEvent);

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
           UsbPortStatus.AsUshort16);

    CreateUsbDevice = HubExtension->BusInterface.CreateUsbDevice;

    if (!CreateUsbDevice)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    HubDeviceHandle = USBH_SyncGetDeviceHandle(HubExtension->LowerDevice);

    return CreateUsbDevice(HubExtension->BusInterface.BusContext,
                           OutDeviceHandle,
                           HubDeviceHandle,
                           UsbPortStatus.AsUshort16,
                           Port);
}

NTSTATUS
NTAPI
USBD_RemoveDeviceEx(IN PUSBHUB_FDO_EXTENSION HubExtension,
                    IN PUSB_DEVICE_HANDLE DeviceHandle,
                    IN ULONG Flags)
{
    PUSB_BUSIFFN_REMOVE_USB_DEVICE RemoveUsbDevice;

    DPRINT("USBD_RemoveDeviceEx: DeviceHandle - %p, Flags - %X\n",
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

        ExFreePoolWithTag(DeviceInfo, USB_HUB_TAG);
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
            ExFreePoolWithTag(DeviceInfo, USB_HUB_TAG);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(NodeInfo, NodeInfoLength);

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

        for (PipeNumber = 0;
             PipeNumber < DeviceInfo->NumberOfOpenPipes;
             PipeNumber++)
        {
            RtlCopyMemory(&NodeInfo->PipeList[PipeNumber],
                          &DeviceInfo->PipeList[PipeNumber],
                          sizeof(USB_PIPE_INFO));
        }
    }

    ExFreePoolWithTag(DeviceInfo, USB_HUB_TAG);

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

        ExFreePoolWithTag(NodeInfo, USB_HUB_TAG);
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
                      PUSBHUB_WORKER_ROUTINE WorkerRoutine,
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
        ExFreePoolWithTag(HubIoWorkItem, USB_HUB_TAG);
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
            ExFreePoolWithTag(HubIoWorkItem, USB_HUB_TAG);

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

    HubIoWorkItem = Context;

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
        ExFreePoolWithTag(HubIoWorkItem->HubWorkItemBuffer, USB_HUB_TAG);
    }

    ExFreePoolWithTag(HubIoWorkItem, USB_HUB_TAG);

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
        ExFreePoolWithTag(HubIoWorkItem->HubWorkItemBuffer, USB_HUB_TAG);
    }

    ExFreePoolWithTag(HubIoWorkItem, USB_HUB_TAG);

    IoFreeWorkItem(WorkItem);
}

VOID
NTAPI
USBHUB_RootHubCallBack(IN PVOID Context)
{
    PUSBHUB_FDO_EXTENSION HubExtension;

    DPRINT("USBHUB_RootHubCallBack: ... \n");

    HubExtension = Context;

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

    KeClearEvent(&HubExtension->RootHubNotificationEvent);

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

    for (Port = 0; Port < NumPorts; ++Port)
    {
        PortDevice = HubExtension->PortData[Port].DeviceObject;

        if (PortDevice)
        {
            PortExtension = PortDevice->DeviceExtension;

            IdleIrp = PortExtension->IdleNotificationIrp;
            PortExtension->IdleNotificationIrp = NULL;

            if (IdleIrp && IoSetCancelRoutine(IdleIrp, NULL))
            {
                DPRINT1("USBH_HubQueuePortIdleIrps: IdleIrp != NULL. FIXME\n");
                DbgBreakPoint();
            }
        }
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
        DPRINT1("USBH_HubCompleteQueuedPortIdleIrps: IdleList not Empty. FIXME\n");
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

    for (Port = 0; Port < HubExtension->HubDescriptor->bNumberOfPorts; ++Port)
    {
        PortDevice = HubExtension->PortData[Port].DeviceObject;

        if (!PortDevice)
        {
            continue;
        }

        PortExtension = PortDevice->DeviceExtension;

        InterlockedExchange((PLONG)&PortExtension->StateBehindD2, 0);

        while (TRUE)
        {
            Entry = ExInterlockedRemoveHeadList(&PortExtension->PortPowerList,
                                                &PortExtension->PortPowerListSpinLock);

            if (!Entry)
            {
                break;
            }

            DPRINT1("USBH_FlushPortPwrList: PortPowerList FIXME\n");
            DbgBreakPoint();
        }
    }

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
    BOOLEAN Result = FALSE;

    DPRINT("USBH_CheckIdleAbort: ... \n");

    InterlockedIncrement(&HubExtension->PendingRequestCount);

    if (IsWait == TRUE)
    {
        KeWaitForSingleObject(&HubExtension->ResetDeviceSemaphore,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    PortData = HubExtension->PortData;

    for (Port = 0; Port < HubExtension->HubDescriptor->bNumberOfPorts; Port++)
    {
        PdoDevice = PortData[Port].DeviceObject;

        if (PdoDevice)
        {
            PortExtension = PdoDevice->DeviceExtension;

            if (PortExtension->PoRequestCounter)
            {
                Result = TRUE;
                goto Wait;
            }
        }
    }

    if (IsExtCheck == TRUE)
    {
        PortData = HubExtension->PortData;

        for (Port = 0;
             Port < HubExtension->HubDescriptor->bNumberOfPorts;
             Port++)
        {
            PdoDevice = PortData[Port].DeviceObject;

            if (PdoDevice)
            {
                PortExtension = PdoDevice->DeviceExtension;
                InterlockedExchange(&PortExtension->StateBehindD2, 0);
            }
        }
    }

Wait:

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

    HubExtension = Context;

    DPRINT("USBH_FdoIdleNotificationCallback: HubExtension - %p, HubFlags - %lX\n",
           HubExtension,
           HubExtension->HubFlags);

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

    PortData = HubExtension->PortData;
    IsReady = TRUE;

    for (Port = 0;
         Port < HubExtension->HubDescriptor->bNumberOfPorts;
         Port++)
    {
        PortDevice = PortData[Port].DeviceObject;

        if (PortDevice)
        {
            PortExtension = PortDevice->DeviceExtension;

            IdleIrp = PortExtension->IdleNotificationIrp;

            if (!IdleIrp)
            {
                IsReady = FALSE;
                goto IdleHub;
            }

            IoStack = IoGetCurrentIrpStackLocation(IdleIrp);

            CallbackInfo = IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

            if (!CallbackInfo)
            {
                IsReady = FALSE;
                goto IdleHub;
            }

            if (!CallbackInfo->IdleCallback)
            {
                IsReady = FALSE;
                goto IdleHub;
            }

            if (PortExtension->PendingSystemPoRequest)
            {
                IsReady = FALSE;
                goto IdleHub;
            }

            if (InterlockedCompareExchange(&PortExtension->StateBehindD2,
                                           1,
                                           0))
            {
                IsReady = FALSE;
                goto IdleHub;
            }

            DPRINT("USBH_FdoIdleNotificationCallback: IdleContext - %p\n",
                   CallbackInfo->IdleContext);

            CallbackInfo->IdleCallback(CallbackInfo->IdleContext);

            if (PortExtension->CurrentPowerState.DeviceState == PowerDeviceD0)
            {
                IsReady = FALSE;
                goto IdleHub;
            }
        }
    }

    if (!(HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STOPPING) &&
        (USBH_CheckIdleAbort(HubExtension, FALSE, FALSE) == TRUE))
    {
        IsReady = FALSE;
    }

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
        DPRINT1("USBH_FdoIdleNotificationCallback: HubFlags - %lX\n",
                HubExtension->HubFlags);

        HubExtension->HubFlags &= ~(USBHUB_FDO_FLAG_DEVICE_SUSPENDED |
                                    USBHUB_FDO_FLAG_GOING_IDLE);

        /* Aborting Idle for Hub */
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
                                   NULL);

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

    IdlePortContext = Context;
    NtStatus = IdlePortContext->Status;

    USBH_HubCompleteQueuedPortIdleIrps(HubExtension,
                                       &IdlePortContext->PwrList,
                                       NtStatus);

    DPRINT1("USBH_CompletePortIdleIrpsWorker: USBH_RegQueryFlushPortPowerIrpsFlag() UNIMPLEMENTED. FIXME\n");
    Status = STATUS_NOT_IMPLEMENTED;// USBH_RegQueryFlushPortPowerIrpsFlag(&IsFlush);

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

    HubWorkItemBuffer = Context;

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
    PVOID IdleIrp;
    KIRQL Irql;
    NTSTATUS Status;
    PUSBHUB_IO_WORK_ITEM HubIoWorkItem;

    IoAcquireCancelSpinLock(&Irql);

    HubExtension = Context;
    HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_WAIT_IDLE_REQUEST;

    IdleIrp = InterlockedExchangePointer((PVOID)&HubExtension->PendingIdleIrp,
                                         NULL);

    DPRINT("USBH_FdoIdleNotificationRequestComplete: IdleIrp - %p\n", IdleIrp);

    if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
    {
        KeSetEvent(&HubExtension->PendingRequestEvent, EVENT_INCREMENT, FALSE);
    }

    IoReleaseCancelSpinLock(Irql);

    NtStatus = Irp->IoStatus.Status;

    DPRINT("USBH_FdoIdleNotificationRequestComplete: NtStatus - %lX\n",
           NtStatus);

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

    Irp = IoAllocateIrp(LowerPDO->StackSize, FALSE);

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
    BOOLEAN IsHubCheck = TRUE;

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
    DPRINT("USBH_CheckHubIdle: HubFlags - %lX\n", HubFlags);

    if (!(HubFlags & USBHUB_FDO_FLAG_DEVICE_STARTED) ||
        !(HubFlags & USBHUB_FDO_FLAG_DO_ENUMERATION))
    {
        goto Exit;
    }

    if (HubFlags & USBHUB_FDO_FLAG_NOT_ENUMERATED ||
        HubFlags & USBHUB_FDO_FLAG_ENUM_POST_RECOVER ||
        HubFlags & USBHUB_FDO_FLAG_DEVICE_FAILED ||
        HubFlags & USBHUB_FDO_FLAG_DEVICE_STOPPING ||
        HubFlags & USBHUB_FDO_FLAG_DEVICE_REMOVED ||
        HubFlags & USBHUB_FDO_FLAG_STATE_CHANGING ||
        HubFlags & USBHUB_FDO_FLAG_WAKEUP_START ||
        HubFlags & USBHUB_FDO_FLAG_ESD_RECOVERING)
    {
        goto Exit;
    }

    if (HubExtension->ResetRequestCount)
    {
        HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEFER_CHECK_IDLE;
        goto Exit;
    }

    HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_DEFER_CHECK_IDLE;

    InterlockedIncrement(&HubExtension->PendingRequestCount);

    KeWaitForSingleObject(&HubExtension->ResetDeviceSemaphore,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    IoAcquireCancelSpinLock(&Irql);

    IsAllPortsIdle = TRUE;

    PortData = HubExtension->PortData;

    for (Port = 0;
         Port < HubExtension->HubDescriptor->bNumberOfPorts;
         Port++)
    {
        PdoDevice = PortData[Port].DeviceObject;

        if (PdoDevice)
        {
            PortExtension = PdoDevice->DeviceExtension;

            if (!PortExtension->IdleNotificationIrp)
            {
                DPRINT("USBH_CheckHubIdle: PortExtension - %p\n",
                       PortExtension);

                IsAllPortsIdle = FALSE;
                IsHubCheck = FALSE;

                break;
            }
        }
    }

    if (IsHubCheck &&
        !(HubExtension->HubFlags & USBHUB_FDO_FLAG_WAIT_IDLE_REQUEST))
    {
        KeClearEvent(&HubExtension->IdleEvent);
        HubExtension->HubFlags |= USBHUB_FDO_FLAG_WAIT_IDLE_REQUEST;
        IsHubIdle = TRUE;
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

Exit:
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

        for (State = 2; State <= 5; State++)
        {
            SystemPowerState = State;

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

    USBHUB_DumpingDeviceDescriptor(&PortExtension->DeviceDescriptor);
    USBHUB_DumpingConfiguration(ConfigDescriptor);

    DPRINT_PNP("USBH_ProcessDeviceInformation: Class - %x, SubClass - %x, Protocol - %x\n",
               PortExtension->DeviceDescriptor.bDeviceClass,
               PortExtension->DeviceDescriptor.bDeviceSubClass,
               PortExtension->DeviceDescriptor.bDeviceProtocol);

    DPRINT_PNP("USBH_ProcessDeviceInformation: bNumConfigurations - %x, bNumInterfaces - %x\n",
               PortExtension->DeviceDescriptor.bNumConfigurations,
               ConfigDescriptor->bNumInterfaces);


    /* Enumeration of USB Composite Devices (msdn):
       1) The device class field of the device descriptor (bDeviceClass) must contain a value of zero,
       or the class (bDeviceClass), subclass (bDeviceSubClass), and protocol (bDeviceProtocol)
       fields of the device descriptor must have the values 0xEF, 0x02 and 0x01 respectively,
       as explained in USB Interface Association Descriptor.
       2) The device must have multiple interfaces
       3) The device must have a single configuration.
    */

    if (((PortExtension->DeviceDescriptor.bDeviceClass == USB_DEVICE_CLASS_RESERVED) ||
        (PortExtension->DeviceDescriptor.bDeviceClass == USB_DEVICE_CLASS_MISCELLANEOUS &&
         PortExtension->DeviceDescriptor.bDeviceSubClass == 0x02 &&
         PortExtension->DeviceDescriptor.bDeviceProtocol == 0x01)) &&
         (ConfigDescriptor->bNumInterfaces > 1) &&
         (PortExtension->DeviceDescriptor.bNumConfigurations < 2))
    {
        DPRINT("USBH_ProcessDeviceInformation: Multi-Interface configuration\n");

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

    if (!HubExtension->HubDescriptor->bNumberOfPorts)
    {
        return TRUE;
    }

    for (Port = 0; Port < HubExtension->HubDescriptor->bNumberOfPorts; Port++)
    {
        PortDevice = HubExtension->PortData[Port].DeviceObject;

        if (PortDevice)
        {
            PortExtension = PortDevice->DeviceExtension;

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
                        return FALSE;
                    }
                }
            }
        }
    }

    return TRUE;
}

BOOLEAN
NTAPI
USBH_ValidateSerialNumberString(IN PUSHORT SerialNumberString)
{
    USHORT ix;
    USHORT Symbol;

    DPRINT("USBH_ValidateSerialNumberString: ... \n");

    for (ix = 0; SerialNumberString[ix] != UNICODE_NULL; ix++)
    {
        Symbol = SerialNumberString[ix];

        if (Symbol < 0x20 || Symbol > 0x7F || Symbol == 0x2C) // ','
        {
            return FALSE;
        }
    }

    return TRUE;
}


NTSTATUS
NTAPI
USBH_CheckDeviceLanguage(IN PDEVICE_OBJECT DeviceObject,
                         IN USHORT LanguageId)
{
    PUSB_STRING_DESCRIPTOR Descriptor;
    NTSTATUS Status;
    ULONG NumSymbols;
    ULONG ix;
    PWCHAR pSymbol;
    ULONG Length;

    DPRINT("USBH_CheckDeviceLanguage: LanguageId - 0x%04X\n", LanguageId);

    Descriptor = ExAllocatePoolWithTag(NonPagedPool,
                                       MAXIMUM_USB_STRING_LENGTH,
                                       USB_HUB_TAG);

    if (!Descriptor)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Descriptor, MAXIMUM_USB_STRING_LENGTH);

    Status = USBH_SyncGetStringDescriptor(DeviceObject,
                                          0,
                                          0,
                                          Descriptor,
                                          MAXIMUM_USB_STRING_LENGTH,
                                          &Length,
                                          TRUE);

    if (!NT_SUCCESS(Status) ||
        Length < sizeof(USB_COMMON_DESCRIPTOR))
    {
        goto Exit;
    }

    NumSymbols = (Length -
                  FIELD_OFFSET(USB_STRING_DESCRIPTOR, bString)) / sizeof(WCHAR);

    pSymbol = Descriptor->bString;

    for (ix = 1; ix < NumSymbols; ix++)
    {
        if (*pSymbol == (WCHAR)LanguageId)
        {
            Status = STATUS_SUCCESS;
            goto Exit;
        }

        pSymbol++;
    }

    Status = STATUS_NOT_SUPPORTED;

Exit:
    ExFreePoolWithTag(Descriptor, USB_HUB_TAG);
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
    UCHAR StringLength;
    UCHAR Length;

    DPRINT("USBH_GetSerialNumberString: ... \n");

    *OutSerialNumber = NULL;
    *OutDescriptorLength = 0;

    Descriptor = ExAllocatePoolWithTag(NonPagedPool,
                                       MAXIMUM_USB_STRING_LENGTH,
                                       USB_HUB_TAG);

    if (!Descriptor)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Descriptor, MAXIMUM_USB_STRING_LENGTH);

    Status = USBH_CheckDeviceLanguage(DeviceObject, LanguageId);

    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

    Status = USBH_SyncGetStringDescriptor(DeviceObject,
                                          Index,
                                          LanguageId,
                                          Descriptor,
                                          MAXIMUM_USB_STRING_LENGTH,
                                          NULL,
                                          TRUE);

    if (!NT_SUCCESS(Status) ||
        Descriptor->bLength <= sizeof(USB_COMMON_DESCRIPTOR))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    StringLength = Descriptor->bLength -
                   FIELD_OFFSET(USB_STRING_DESCRIPTOR, bString);

    Length = StringLength + sizeof(UNICODE_NULL);

    SerialNumberBuffer = ExAllocatePoolWithTag(PagedPool, Length, USB_HUB_TAG);

    if (!SerialNumberBuffer)
    {
        goto Exit;
    }

    RtlZeroMemory(SerialNumberBuffer, Length);
    RtlCopyMemory(SerialNumberBuffer, Descriptor->bString, StringLength);

    *OutSerialNumber = SerialNumberBuffer;
    *OutDescriptorLength = Length;

Exit:
    ExFreePoolWithTag(Descriptor, USB_HUB_TAG);
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

    DPRINT("USBH_CreateDevice: Port - %x, UsbPortStatus - %lX\n",
           Port,
           UsbPortStatus.AsUshort16);

    do
    {
        RtlStringCbPrintfW(CharDeviceName,
                           sizeof(CharDeviceName),
                           L"\\Device\\USBPDO-%d",
                           PdoNumber);

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
        ASSERT(Port > 0);
        HubExtension->PortData[Port-1].DeviceObject = DeviceObject;
        return Status;
    }

    DeviceObject->StackSize = HubExtension->RootHubPdo2->StackSize;

    PortExtension = DeviceObject->DeviceExtension;

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

    IsHsDevice = UsbPortStatus.Usb20PortStatus.HighSpeedDeviceAttached;
    IsLsDevice = UsbPortStatus.Usb20PortStatus.LowSpeedDeviceAttached;

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
        DPRINT1("USBH_CreateDevice: IoCreateDevice() failed - %lX\n", Status);
        goto ErrorExit;
    }

    Status = USBD_CreateDeviceEx(HubExtension,
                                 &PortExtension->DeviceHandle,
                                 UsbPortStatus,
                                 Port);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_CreateDevice: USBD_CreateDeviceEx() failed - %lX\n", Status);
        goto ErrorExit;
    }

    Status = USBH_SyncResetPort(HubExtension, Port);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_CreateDevice: USBH_SyncResetPort() failed - %lX\n", Status);
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
        DPRINT1("USBH_CreateDevice: USBD_InitializeDeviceEx() failed - %lX\n", Status);
        PortExtension->DeviceHandle = NULL;
        goto ErrorExit;
    }

    DPRINT1("USBH_RegQueryDeviceIgnoreHWSerNumFlag UNIMPLEMENTED. FIXME\n");
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
        InterlockedExchangePointer((PVOID)&PortExtension->SerialNumber, NULL);

        USBH_GetSerialNumberString(PortExtension->Common.SelfDevice,
                                   &SerialNumberBuffer,
                                   &PortExtension->SN_DescriptorLength,
                                   MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                                   PortExtension->DeviceDescriptor.iSerialNumber);

        if (SerialNumberBuffer)
        {
            if (!USBH_ValidateSerialNumberString((PUSHORT)SerialNumberBuffer))
            {
                ExFreePoolWithTag(SerialNumberBuffer, USB_HUB_TAG);
                SerialNumberBuffer = NULL;
            }

            if (SerialNumberBuffer &&
                !USBH_CheckDeviceIDUnique(HubExtension,
                                          PortExtension->DeviceDescriptor.idVendor,
                                          PortExtension->DeviceDescriptor.idProduct,
                                          SerialNumberBuffer,
                                          PortExtension->SN_DescriptorLength))
            {
                ExFreePoolWithTag(SerialNumberBuffer, USB_HUB_TAG);
                SerialNumberBuffer = NULL;
            }
        }

        InterlockedExchangePointer((PVOID)&PortExtension->SerialNumber,
                                   SerialNumberBuffer);
    }

    Status = USBH_ProcessDeviceInformation(PortExtension);

    USBH_PdoSetCapabilities(PortExtension);

    if (NT_SUCCESS(Status))
    {
        goto Exit;
    }

ErrorExit:

    PortExtension->PortPdoFlags |= USBHUB_PDO_FLAG_INIT_PORT_FAILED;

    DeviceHandle = InterlockedExchangePointer(&PortExtension->DeviceHandle,
                                              NULL);

    if (DeviceHandle)
    {
        USBD_RemoveDeviceEx(HubExtension, DeviceHandle, 0);
    }

    SerialNumberBuffer = InterlockedExchangePointer((PVOID)&PortExtension->SerialNumber,
                                                     NULL);

    if (SerialNumberBuffer)
    {
        ExFreePoolWithTag(SerialNumberBuffer, USB_HUB_TAG);
    }

Exit:

    ASSERT(Port > 0);
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
    PVOID NewDeviceHandle;
    PVOID Handle;
    PVOID OldDeviceHandle;
    PUSB_DEVICE_HANDLE * DeviceHandle;
    USB_PORT_STATUS_AND_CHANGE PortStatus;

    DPRINT("USBH_ResetDevice: HubExtension - %p, Port - %x, IsKeepDeviceData - %x, IsWait - %x\n",
           HubExtension,
           Port,
           IsKeepDeviceData,
           IsWait);

    Status = USBH_SyncGetPortStatus(HubExtension,
                                    Port,
                                    &PortStatus,
                                    sizeof(USB_PORT_STATUS_AND_CHANGE));

    if (!NT_SUCCESS(Status) ||
        !(PortStatus.PortStatus.Usb20PortStatus.CurrentConnectStatus))
    {
        return STATUS_UNSUCCESSFUL;
    }

    InterlockedIncrement(&HubExtension->PendingRequestCount);

    KeWaitForSingleObject(&HubExtension->ResetDeviceSemaphore,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    ASSERT(Port > 0);
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

    PortExtension = PortDevice->DeviceExtension;
    DeviceHandle = &PortExtension->DeviceHandle;

    OldDeviceHandle = InterlockedExchangePointer(&PortExtension->DeviceHandle,
                                                 NULL);

    if (OldDeviceHandle)
    {
        if (!(PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_REMOVING_PORT_PDO))
        {
            Status = USBD_RemoveDeviceEx(HubExtension,
                                         OldDeviceHandle,
                                         IsKeepDeviceData);

            PortExtension->PortPdoFlags |= USBHUB_PDO_FLAG_REMOVING_PORT_PDO;
        }
    }
    else
    {
        OldDeviceHandle = NULL;
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
                                    sizeof(USB_PORT_STATUS_AND_CHANGE));

    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit;
    }

    Status = USBD_CreateDeviceEx(HubExtension,
                                 DeviceHandle,
                                 PortStatus.PortStatus,
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
                                          OldDeviceHandle,
                                          *DeviceHandle);

            if (!NT_SUCCESS(Status))
            {
                Handle = InterlockedExchangePointer(DeviceHandle, NULL);

                USBD_RemoveDeviceEx(HubExtension, Handle, 0);
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

    NewDeviceHandle = InterlockedExchangePointer(DeviceHandle,
                                                 OldDeviceHandle);

    if (NewDeviceHandle)
    {
        Status = USBD_RemoveDeviceEx(HubExtension, NewDeviceHandle, 0);
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
    BOOLEAN ShouldCompleteIrp;
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

            if (ControlCode == IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER)
            {
                Status = STATUS_NOT_SUPPORTED;
                USBH_CompleteIrp(Irp, Status);
                break;
            }

            if (ControlCode == IOCTL_KS_PROPERTY)
            {
                DPRINT1("USBH_PdoDispatch: IOCTL_KS_PROPERTY FIXME\n");
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
                                 &ShouldCompleteIrp);

            if (ShouldCompleteIrp)
            {
                USBH_CompleteIrp(Irp, Status);
            }

            break;

        case IRP_MJ_POWER:
            Status = USBH_PdoPower(PortExtension, Irp, IoStack->MinorFunction);
            break;

        case IRP_MJ_SYSTEM_CONTROL:
            DPRINT1("USBH_PdoDispatch: USBH_SystemControl() UNIMPLEMENTED. FIXME\n");
            //USBH_PortSystemControl(PortExtension, Irp);
            Status = Irp->IoStatus.Status;
            USBH_CompleteIrp(Irp, Status);
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

    DPRINT("USBH_FdoDispatch: HubExtension - %p, Irp - %p, MajorFunction - %X\n",
           HubExtension,
           Irp,
           IoStack->MajorFunction);

    MajorFunction = IoStack->MajorFunction;

    switch (MajorFunction)
    {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
            Status = STATUS_SUCCESS;
            USBH_CompleteIrp(Irp, Status);
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
            /* fall through */

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

    HubExtension = DeviceObject->DeviceExtension;
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

    DPRINT("USBH_AddDevice: call IoWMIRegistrationControl() UNIMPLEMENTED. FIXME\n");

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


    DeviceExtension = DeviceObject->DeviceExtension;
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
        Status = STATUS_ASSERTION_FAILURE;
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_RegQueryGenericUSBDeviceString(PVOID USBDeviceString)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];

    DPRINT("USBH_RegQueryGenericUSBDeviceString ... \n");

    RtlZeroMemory(QueryTable, sizeof(QueryTable));

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

