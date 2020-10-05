/*
 * PROJECT:     ReactOS USB Port Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBPort main driver functions
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbport.h"

#define NDEBUG
#include <debug.h>

#define NDEBUG_USBPORT_CORE
#define NDEBUG_USBPORT_INTERRUPT
#define NDEBUG_USBPORT_TIMER
#include "usbdebug.h"

LIST_ENTRY USBPORT_MiniPortDrivers = {NULL, NULL};
LIST_ENTRY USBPORT_USB1FdoList = {NULL, NULL};
LIST_ENTRY USBPORT_USB2FdoList = {NULL, NULL};

KSPIN_LOCK USBPORT_SpinLock;
BOOLEAN USBPORT_Initialized = FALSE;

PDEVICE_OBJECT
NTAPI
USBPORT_FindUSB2Controller(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_DEVICE_EXTENSION USB2FdoExtension;
    KIRQL OldIrql;
    PLIST_ENTRY USB2FdoEntry;
    PDEVICE_OBJECT USB2FdoDevice = NULL;

    DPRINT("USBPORT_FindUSB2Controller: FdoDevice - %p\n", FdoDevice);

    FdoExtension = FdoDevice->DeviceExtension;

    KeAcquireSpinLock(&USBPORT_SpinLock, &OldIrql);

    USB2FdoEntry = USBPORT_USB2FdoList.Flink;

    while (USB2FdoEntry && USB2FdoEntry != &USBPORT_USB2FdoList)
    {
        USB2FdoExtension = CONTAINING_RECORD(USB2FdoEntry,
                                             USBPORT_DEVICE_EXTENSION,
                                             ControllerLink);

        if (USB2FdoExtension->BusNumber == FdoExtension->BusNumber &&
            USB2FdoExtension->PciDeviceNumber == FdoExtension->PciDeviceNumber)
        {
            USB2FdoDevice = USB2FdoExtension->CommonExtension.SelfDevice;
            break;
        }

        USB2FdoEntry = USB2FdoEntry->Flink;
    }

    KeReleaseSpinLock(&USBPORT_SpinLock, OldIrql);

    return USB2FdoDevice;
}

VOID
NTAPI
USBPORT_AddUSB1Fdo(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT("USBPORT_AddUSB1Fdo: FdoDevice - %p\n", FdoDevice);

    FdoExtension = FdoDevice->DeviceExtension;
    FdoExtension->Flags |= USBPORT_FLAG_REGISTERED_FDO;

    ExInterlockedInsertTailList(&USBPORT_USB1FdoList,
                                &FdoExtension->ControllerLink,
                                &USBPORT_SpinLock);
}

VOID
NTAPI
USBPORT_AddUSB2Fdo(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT("USBPORT_AddUSB2Fdo: FdoDevice - %p\n", FdoDevice);

    FdoExtension = FdoDevice->DeviceExtension;
    FdoExtension->Flags |= USBPORT_FLAG_REGISTERED_FDO;

    ExInterlockedInsertTailList(&USBPORT_USB2FdoList,
                                &FdoExtension->ControllerLink,
                                &USBPORT_SpinLock);
}

VOID
NTAPI
USBPORT_RemoveUSBxFdo(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    KIRQL OldIrql;

    DPRINT("USBPORT_RemoveUSBxFdo: FdoDevice - %p\n", FdoDevice);

    FdoExtension = FdoDevice->DeviceExtension;

    KeAcquireSpinLock(&USBPORT_SpinLock, &OldIrql);
    RemoveEntryList(&FdoExtension->ControllerLink);
    KeReleaseSpinLock(&USBPORT_SpinLock, OldIrql);

    FdoExtension->Flags &= ~USBPORT_FLAG_REGISTERED_FDO;

    FdoExtension->ControllerLink.Flink = NULL;
    FdoExtension->ControllerLink.Blink = NULL;
}

BOOLEAN
NTAPI
USBPORT_IsCompanionFdoExtension(IN PDEVICE_OBJECT USB2FdoDevice,
                                IN PUSBPORT_DEVICE_EXTENSION USB1FdoExtension)
{
    PUSBPORT_DEVICE_EXTENSION USB2FdoExtension;

    DPRINT("USBPORT_IsCompanionFdoExtension: USB2Fdo - %p, USB1FdoExtension - %p\n",
           USB2FdoDevice,
           USB1FdoExtension);

    USB2FdoExtension = USB2FdoDevice->DeviceExtension;

    return USB2FdoExtension->BusNumber == USB1FdoExtension->BusNumber &&
           USB2FdoExtension->PciDeviceNumber == USB1FdoExtension->PciDeviceNumber;
}

PDEVICE_RELATIONS
NTAPI
USBPORT_FindCompanionControllers(IN PDEVICE_OBJECT USB2FdoDevice,
                                 IN BOOLEAN IsObRefer,
                                 IN BOOLEAN IsFDOsReturned)
{
    PLIST_ENTRY USB1FdoList;
    PUSBPORT_DEVICE_EXTENSION USB1FdoExtension;
    ULONG NumControllers = 0; 
    PDEVICE_OBJECT * Entry;
    PDEVICE_RELATIONS ControllersList = NULL;
    KIRQL OldIrql;

    DPRINT("USBPORT_FindCompanionControllers: USB2Fdo - %p, IsObRefer - %x, IsFDOs - %x\n",
           USB2FdoDevice,
           IsObRefer,
           IsFDOsReturned);

    KeAcquireSpinLock(&USBPORT_SpinLock, &OldIrql);

    USB1FdoList = USBPORT_USB1FdoList.Flink;

    while (USB1FdoList && USB1FdoList != &USBPORT_USB1FdoList)
    {
        USB1FdoExtension = CONTAINING_RECORD(USB1FdoList,
                                             USBPORT_DEVICE_EXTENSION,
                                             ControllerLink);

        if (USB1FdoExtension->Flags & USBPORT_FLAG_COMPANION_HC &&
            USBPORT_IsCompanionFdoExtension(USB2FdoDevice, USB1FdoExtension))
        {
            ++NumControllers;
        }

        USB1FdoList = USB1FdoExtension->ControllerLink.Flink;
    }

    DPRINT("USBPORT_FindCompanionControllers: NumControllers - %x\n",
           NumControllers);

    if (!NumControllers)
    {
        goto Exit;
    }

    ControllersList = ExAllocatePoolWithTag(NonPagedPool,
                                            NumControllers * sizeof(DEVICE_RELATIONS),
                                            USB_PORT_TAG);

    if (!ControllersList)
    {
        goto Exit;
    }

    RtlZeroMemory(ControllersList, NumControllers * sizeof(DEVICE_RELATIONS));

    ControllersList->Count = NumControllers;

    USB1FdoList = USBPORT_USB1FdoList.Flink;

    Entry = &ControllersList->Objects[0];

    while (USB1FdoList && USB1FdoList != &USBPORT_USB1FdoList)
    {
        USB1FdoExtension = CONTAINING_RECORD(USB1FdoList,
                                             USBPORT_DEVICE_EXTENSION,
                                             ControllerLink);

        if (USB1FdoExtension->Flags & USBPORT_FLAG_COMPANION_HC &&
            USBPORT_IsCompanionFdoExtension(USB2FdoDevice, USB1FdoExtension))
        {
            *Entry = USB1FdoExtension->CommonExtension.LowerPdoDevice;

            if (IsObRefer)
            {
                ObReferenceObject(USB1FdoExtension->CommonExtension.LowerPdoDevice);
            }

            if (IsFDOsReturned)
            {
                *Entry = USB1FdoExtension->CommonExtension.SelfDevice;
            }

            ++Entry;
        }

        USB1FdoList = USB1FdoExtension->ControllerLink.Flink;
    }

Exit:

    KeReleaseSpinLock(&USBPORT_SpinLock, OldIrql);

    return ControllersList;
}

MPSTATUS
NTAPI
USBPORT_NtStatusToMpStatus(NTSTATUS NtStatus)
{
    DPRINT("USBPORT_NtStatusToMpStatus: NtStatus - %x\n", NtStatus);

    if (NtStatus == STATUS_SUCCESS)
    {
        return MP_STATUS_SUCCESS;
    }
    else
    {
        return MP_STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
NTAPI
USBPORT_SetRegistryKeyValue(IN PDEVICE_OBJECT DeviceObject,
                            IN BOOL UseDriverKey,
                            IN ULONG Type,
                            IN PCWSTR ValueNameString,
                            IN PVOID Data,
                            IN ULONG DataSize)
{
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    NTSTATUS Status;

    DPRINT("USBPORT_SetRegistryKeyValue: ValueNameString - %S \n",
           ValueNameString);

    if (UseDriverKey)
    {
        Status = IoOpenDeviceRegistryKey(DeviceObject,
                                         PLUGPLAY_REGKEY_DRIVER,
                                         STANDARD_RIGHTS_ALL,
                                         &KeyHandle);
    }
    else
    {
        Status = IoOpenDeviceRegistryKey(DeviceObject,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         STANDARD_RIGHTS_ALL,
                                         &KeyHandle);
    }

    if (NT_SUCCESS(Status))
    {
        RtlInitUnicodeString(&ValueName, ValueNameString);

        Status = ZwSetValueKey(KeyHandle,
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
USBPORT_GetRegistryKeyValueFullInfo(IN PDEVICE_OBJECT FdoDevice,
                                    IN PDEVICE_OBJECT PdoDevice,
                                    IN BOOL UseDriverKey,
                                    IN PCWSTR SourceString,
                                    IN ULONG LengthStr,
                                    IN PVOID Buffer,
                                    IN ULONG BufferLength)
{
    NTSTATUS Status;
    PKEY_VALUE_FULL_INFORMATION KeyValue;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    ULONG LengthKey;

    DPRINT("USBPORT_GetRegistryKeyValue: UseDriverKey - %x, SourceString - %S, LengthStr - %x, Buffer - %p, BufferLength - %x\n",
           UseDriverKey,
           SourceString,
           LengthStr,
           Buffer,
           BufferLength);

    if (UseDriverKey)
    {
        Status = IoOpenDeviceRegistryKey(PdoDevice,
                                         PLUGPLAY_REGKEY_DRIVER,
                                         STANDARD_RIGHTS_ALL,
                                         &KeyHandle);
    }
    else
    {
        Status = IoOpenDeviceRegistryKey(PdoDevice,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         STANDARD_RIGHTS_ALL,
                                         &KeyHandle);
    }

    if (NT_SUCCESS(Status))
    {
        RtlInitUnicodeString(&ValueName, SourceString);

        LengthKey = sizeof(KEY_VALUE_FULL_INFORMATION) +
                    LengthStr +
                    BufferLength;

        KeyValue = ExAllocatePoolWithTag(PagedPool,
                                         LengthKey,
                                         USB_PORT_TAG);

        if (KeyValue)
        {
            RtlZeroMemory(KeyValue, LengthKey);

            Status = ZwQueryValueKey(KeyHandle,
                                     &ValueName,
                                     KeyValueFullInformation,
                                     KeyValue,
                                     LengthKey,
                                     &LengthKey);

            if (NT_SUCCESS(Status))
            {
                RtlCopyMemory(Buffer,
                              (PUCHAR)KeyValue + KeyValue->DataOffset,
                              BufferLength);
            }

            ExFreePoolWithTag(KeyValue, USB_PORT_TAG);
        }

        ZwClose(KeyHandle);
    }

    return Status;
}

MPSTATUS
NTAPI
USBPORT_GetMiniportRegistryKeyValue(IN PVOID MiniPortExtension,
                                    IN BOOL UseDriverKey,
                                    IN PCWSTR SourceString,
                                    IN SIZE_T LengthStr,
                                    IN PVOID Buffer,
                                    IN SIZE_T BufferLength)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PDEVICE_OBJECT FdoDevice;
    NTSTATUS Status;

    DPRINT("USBPORT_GetMiniportRegistryKeyValue: MiniPortExtension - %p, UseDriverKey - %x, SourceString - %S, LengthStr - %x, Buffer - %p, BufferLength - %x\n",
           MiniPortExtension,
           UseDriverKey,
           SourceString,
           LengthStr,
           Buffer,
           BufferLength);

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)((ULONG_PTR)MiniPortExtension -
                                               sizeof(USBPORT_DEVICE_EXTENSION));

    FdoDevice = FdoExtension->CommonExtension.SelfDevice;

    Status = USBPORT_GetRegistryKeyValueFullInfo(FdoDevice,
                                                 FdoExtension->CommonExtension.LowerPdoDevice,
                                                 UseDriverKey,
                                                 SourceString,
                                                 LengthStr,
                                                 Buffer,
                                                 BufferLength);

    return USBPORT_NtStatusToMpStatus(Status);
}

NTSTATUS
NTAPI
USBPORT_GetSetConfigSpaceData(IN PDEVICE_OBJECT FdoDevice,
                              IN BOOLEAN IsReadData,
                              IN PVOID Buffer,
                              IN ULONG Offset,
                              IN ULONG Length)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    ULONG BytesReadWrite;

    DPRINT("USBPORT_GetSetConfigSpaceData ... \n");

    FdoExtension = FdoDevice->DeviceExtension;

    BytesReadWrite = Length;

    if (IsReadData)
    {
        RtlZeroMemory(Buffer, Length);

        BytesReadWrite = (*FdoExtension->BusInterface.GetBusData)
                          (FdoExtension->BusInterface.Context,
                           PCI_WHICHSPACE_CONFIG,
                           Buffer,
                           Offset,
                           Length);
    }
    else
    {
        BytesReadWrite = (*FdoExtension->BusInterface.SetBusData)
                          (FdoExtension->BusInterface.Context,
                           PCI_WHICHSPACE_CONFIG,
                           Buffer,
                           Offset,
                           Length);
    }

    if (BytesReadWrite == Length)
    {
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

MPSTATUS
NTAPI
USBPORT_ReadWriteConfigSpace(IN PVOID MiniPortExtension,
                             IN BOOLEAN IsReadData,
                             IN PVOID Buffer,
                             IN ULONG Offset,
                             IN ULONG Length)
{
    NTSTATUS Status;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PDEVICE_OBJECT FdoDevice;

    DPRINT("USBPORT_ReadWriteConfigSpace: ... \n");

    //FdoExtension->MiniPortExt = (PVOID)((ULONG_PTR)FdoExtension + sizeof(USBPORT_DEVICE_EXTENSION));
    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)((ULONG_PTR)MiniPortExtension -
                                               sizeof(USBPORT_DEVICE_EXTENSION));

    FdoDevice = FdoExtension->CommonExtension.SelfDevice;

    Status = USBPORT_GetSetConfigSpaceData(FdoDevice,
                                           IsReadData,
                                           Buffer,
                                           Offset,
                                           Length);

    return USBPORT_NtStatusToMpStatus(Status);
}

NTSTATUS
NTAPI
USBPORT_USBDStatusToNtStatus(IN PURB Urb,
                             IN USBD_STATUS USBDStatus)
{
    NTSTATUS Status;

    if (USBD_ERROR(USBDStatus))
    {
        DPRINT1("USBPORT_USBDStatusToNtStatus: Urb - %p, USBDStatus - %x\n",
                Urb,
                USBDStatus);
    }

    if (Urb)
        Urb->UrbHeader.Status = USBDStatus;

    switch (USBDStatus)
    {
        case USBD_STATUS_SUCCESS:
            Status = STATUS_SUCCESS;
            break;

        case USBD_STATUS_INSUFFICIENT_RESOURCES:
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;

        case USBD_STATUS_DEVICE_GONE:
            Status = STATUS_DEVICE_NOT_CONNECTED;
            break;

        case USBD_STATUS_CANCELED:
            Status = STATUS_CANCELLED;
            break;

        case USBD_STATUS_NOT_SUPPORTED:
            Status = STATUS_NOT_SUPPORTED;
            break;

        case USBD_STATUS_INVALID_URB_FUNCTION:
        case USBD_STATUS_INVALID_PARAMETER:
        case USBD_STATUS_INVALID_PIPE_HANDLE:
        case USBD_STATUS_BAD_START_FRAME:
            Status = STATUS_INVALID_PARAMETER;
            break;

        default:
            if (USBD_ERROR(USBDStatus))
                Status = STATUS_UNSUCCESSFUL;
            else
                Status = STATUS_SUCCESS;

            break;
    }

    return Status;
}

NTSTATUS
NTAPI
USBPORT_Wait(IN PVOID MiniPortExtension,
             IN ULONG Milliseconds)
{
    LARGE_INTEGER Interval = {{0, 0}};

    DPRINT("USBPORT_Wait: Milliseconds - %x\n", Milliseconds);
    Interval.QuadPart -= 10000 * Milliseconds + (KeQueryTimeIncrement() - 1);
    return KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}

VOID
NTAPI
USBPORT_MiniportInterrupts(IN PDEVICE_OBJECT FdoDevice,
                           IN BOOLEAN IsEnable)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    BOOLEAN IsLock;
    KIRQL OldIrql;

    DPRINT_INT("USBPORT_MiniportInterrupts: IsEnable - %p\n", IsEnable);

    FdoExtension = FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    IsLock = (Packet->MiniPortFlags & USB_MINIPORT_FLAGS_NOT_LOCK_INT) == 0;

    if (IsLock)
        KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);

    if (IsEnable)
    {
        FdoExtension->Flags |= USBPORT_FLAG_INTERRUPT_ENABLED;
        Packet->EnableInterrupts(FdoExtension->MiniPortExt);
    }
    else
    {
        Packet->DisableInterrupts(FdoExtension->MiniPortExt);
        FdoExtension->Flags &= ~USBPORT_FLAG_INTERRUPT_ENABLED;
    }

    if (IsLock)
        KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);
}

VOID
NTAPI
USBPORT_SoftInterruptDpc(IN PRKDPC Dpc,
                         IN PVOID DeferredContext,
                         IN PVOID SystemArgument1,
                         IN PVOID SystemArgument2)
{
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT_INT("USBPORT_SoftInterruptDpc: ... \n");

    FdoDevice = DeferredContext;
    FdoExtension = FdoDevice->DeviceExtension;

    if (!KeInsertQueueDpc(&FdoExtension->IsrDpc, NULL, (PVOID)1))
    {
        InterlockedDecrement(&FdoExtension->IsrDpcCounter);
    }
}

VOID
NTAPI
USBPORT_SoftInterrupt(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    LARGE_INTEGER DueTime = {{0, 0}};

    DPRINT_INT("USBPORT_SoftInterrupt: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;

    KeInitializeTimer(&FdoExtension->TimerSoftInterrupt);

    KeInitializeDpc(&FdoExtension->SoftInterruptDpc,
                    USBPORT_SoftInterruptDpc,
                    FdoDevice);

    DueTime.QuadPart -= 10000 + (KeQueryTimeIncrement() - 1);

    KeSetTimer(&FdoExtension->TimerSoftInterrupt,
               DueTime,
               &FdoExtension->SoftInterruptDpc);
}

VOID
NTAPI
USBPORT_InvalidateControllerHandler(IN PDEVICE_OBJECT FdoDevice,
                                    IN ULONG Type)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT_CORE("USBPORT_InvalidateControllerHandler: Invalidate Type - %x\n",
                Type);

    FdoExtension = FdoDevice->DeviceExtension;

    switch (Type)
    {
        case USBPORT_INVALIDATE_CONTROLLER_RESET:
            DPRINT1("USBPORT_InvalidateControllerHandler: INVALIDATE_CONTROLLER_RESET UNIMPLEMENTED. FIXME. \n");
            break;

        case USBPORT_INVALIDATE_CONTROLLER_SURPRISE_REMOVE:
            DPRINT1("USBPORT_InvalidateControllerHandler: INVALIDATE_CONTROLLER_SURPRISE_REMOVE UNIMPLEMENTED. FIXME. \n");
            break;

        case USBPORT_INVALIDATE_CONTROLLER_SOFT_INTERRUPT:
            if (InterlockedIncrement(&FdoExtension->IsrDpcCounter))
            {
                InterlockedDecrement(&FdoExtension->IsrDpcCounter);
            }
            else
            {
                USBPORT_SoftInterrupt(FdoDevice);
            }
            break;
    }
}

ULONG
NTAPI
USBPORT_InvalidateController(IN PVOID MiniPortExtension,
                             IN ULONG Type)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PDEVICE_OBJECT FdoDevice;

    DPRINT("USBPORT_InvalidateController: Invalidate Type - %x\n", Type);

    //FdoExtension->MiniPortExt = (PVOID)((ULONG_PTR)FdoExtension + sizeof(USBPORT_DEVICE_EXTENSION));
    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)((ULONG_PTR)MiniPortExtension -
                                               sizeof(USBPORT_DEVICE_EXTENSION));
    FdoDevice = FdoExtension->CommonExtension.SelfDevice;

    USBPORT_InvalidateControllerHandler(FdoDevice, Type);

    return 0;
}

ULONG
NTAPI
USBPORT_NotifyDoubleBuffer(IN PVOID MiniPortExtension,
                           IN PVOID MiniPortTransfer,
                           IN PVOID Buffer,
                           IN SIZE_T Length)
{
    DPRINT1("USBPORT_NotifyDoubleBuffer: UNIMPLEMENTED. FIXME. \n");
    return 0;
}

VOID
NTAPI
USBPORT_WorkerRequestDpc(IN PRKDPC Dpc,
                         IN PVOID DeferredContext,
                         IN PVOID SystemArgument1,
                         IN PVOID SystemArgument2)
{
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT("USBPORT_WorkerRequestDpc: ... \n");

    FdoDevice = DeferredContext;
    FdoExtension = FdoDevice->DeviceExtension;

    if (!InterlockedIncrement(&FdoExtension->IsrDpcHandlerCounter))
    {
        USBPORT_DpcHandler(FdoDevice);
    }

    InterlockedDecrement(&FdoExtension->IsrDpcHandlerCounter);
}

VOID
NTAPI
USBPORT_DoneTransfer(IN PUSBPORT_TRANSFER Transfer)
{
    PUSBPORT_ENDPOINT          Endpoint;
    PDEVICE_OBJECT             FdoDevice;
    PUSBPORT_DEVICE_EXTENSION  FdoExtension;
    PURB                       Urb;
    PIRP                       Irp;
    KIRQL                      CancelIrql;
    KIRQL                      OldIrql;

    DPRINT_CORE("USBPORT_DoneTransfer: Transfer - %p\n", Transfer);

    Endpoint = Transfer->Endpoint;
    FdoDevice = Endpoint->FdoDevice;
    FdoExtension = FdoDevice->DeviceExtension;

    Urb = Transfer->Urb;
    Irp = Transfer->Irp;

    KeAcquireSpinLock(&FdoExtension->FlushTransferSpinLock, &OldIrql);

    if (Irp)
    {
        IoAcquireCancelSpinLock(&CancelIrql);
        IoSetCancelRoutine(Irp, NULL);
        IoReleaseCancelSpinLock(CancelIrql);

        USBPORT_RemoveActiveTransferIrp(FdoDevice, Irp);
    }

    KeReleaseSpinLock(&FdoExtension->FlushTransferSpinLock, OldIrql);

    USBPORT_USBDStatusToNtStatus(Transfer->Urb, Transfer->USBDStatus);
    USBPORT_CompleteTransfer(Urb, Urb->UrbHeader.Status);

    DPRINT_CORE("USBPORT_DoneTransfer: exit\n");
}

VOID
NTAPI
USBPORT_FlushDoneTransfers(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PLIST_ENTRY DoneTransferList;
    PUSBPORT_TRANSFER Transfer;
    PUSBPORT_ENDPOINT Endpoint;
    ULONG TransferCount;
    KIRQL OldIrql;
    BOOLEAN IsHasTransfers;

    DPRINT_CORE("USBPORT_FlushDoneTransfers: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;
    DoneTransferList = &FdoExtension->DoneTransferList;

    while (TRUE)
    {
        KeAcquireSpinLock(&FdoExtension->DoneTransferSpinLock, &OldIrql);

        if (IsListEmpty(DoneTransferList))
            break;

        Transfer = CONTAINING_RECORD(DoneTransferList->Flink,
                                     USBPORT_TRANSFER,
                                     TransferLink);

        RemoveHeadList(DoneTransferList);
        KeReleaseSpinLock(&FdoExtension->DoneTransferSpinLock, OldIrql);

        if (Transfer)
        {
            Endpoint = Transfer->Endpoint;

            if ((Transfer->Flags & TRANSFER_FLAG_SPLITED))
            {
                USBPORT_DoneSplitTransfer(Transfer);
            }
            else
            {
                USBPORT_DoneTransfer(Transfer);
            }

            IsHasTransfers = USBPORT_EndpointHasQueuedTransfers(FdoDevice,
                                                                Endpoint,
                                                                &TransferCount);

            if (IsHasTransfers && !TransferCount)
            {
                USBPORT_InvalidateEndpointHandler(FdoDevice,
                                                  Endpoint,
                                                  INVALIDATE_ENDPOINT_WORKER_DPC);
            }
        }
    }

    KeReleaseSpinLock(&FdoExtension->DoneTransferSpinLock, OldIrql);
}


VOID
NTAPI
USBPORT_TransferFlushDpc(IN PRKDPC Dpc,
                         IN PVOID DeferredContext,
                         IN PVOID SystemArgument1,
                         IN PVOID SystemArgument2)
{
    PDEVICE_OBJECT FdoDevice;

    DPRINT_CORE("USBPORT_TransferFlushDpc: ... \n");
    FdoDevice = DeferredContext;
    USBPORT_FlushDoneTransfers(FdoDevice);
}

BOOLEAN
NTAPI
USBPORT_QueueDoneTransfer(IN PUSBPORT_TRANSFER Transfer,
                          IN USBD_STATUS USBDStatus)
{
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION  FdoExtension;

    DPRINT_CORE("USBPORT_QueueDoneTransfer: Transfer - %p, USBDStatus - %p\n",
                Transfer,
                USBDStatus);

    FdoDevice = Transfer->Endpoint->FdoDevice;
    FdoExtension = FdoDevice->DeviceExtension;

    RemoveEntryList(&Transfer->TransferLink);
    Transfer->USBDStatus = USBDStatus;

    ExInterlockedInsertTailList(&FdoExtension->DoneTransferList,
                                &Transfer->TransferLink,
                                &FdoExtension->DoneTransferSpinLock);

    return KeInsertQueueDpc(&FdoExtension->TransferFlushDpc, NULL, NULL);
}

VOID
NTAPI
USBPORT_DpcHandler(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_ENDPOINT Endpoint;
    PLIST_ENTRY Entry;
    LIST_ENTRY List;
    LONG LockCounter;

    DPRINT_CORE("USBPORT_DpcHandler: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;

    InitializeListHead(&List);

    KeAcquireSpinLockAtDpcLevel(&FdoExtension->EndpointListSpinLock);
    Entry = FdoExtension->EndpointList.Flink;

    while (Entry && Entry != &FdoExtension->EndpointList)
    {
        Endpoint = CONTAINING_RECORD(Entry,
                                     USBPORT_ENDPOINT,
                                     EndpointLink);

        LockCounter = InterlockedIncrement(&Endpoint->LockCounter);

        if (USBPORT_GetEndpointState(Endpoint) != USBPORT_ENDPOINT_ACTIVE ||
            LockCounter ||
            Endpoint->Flags & ENDPOINT_FLAG_ROOTHUB_EP0)
        {
            InterlockedDecrement(&Endpoint->LockCounter);
        }
        else
        {
            InsertTailList(&List, &Endpoint->DispatchLink);

            if (Endpoint->WorkerLink.Flink && Endpoint->WorkerLink.Blink)
            {
                RemoveEntryList(&Endpoint->WorkerLink);

                Endpoint->WorkerLink.Flink = NULL;
                Endpoint->WorkerLink.Blink = NULL;
            }
        }

        Entry = Endpoint->EndpointLink.Flink;
    }

    KeReleaseSpinLockFromDpcLevel(&FdoExtension->EndpointListSpinLock);

    while (!IsListEmpty(&List))
    {
        Endpoint = CONTAINING_RECORD(List.Flink,
                                     USBPORT_ENDPOINT,
                                     DispatchLink);

        RemoveEntryList(List.Flink);
        Endpoint->DispatchLink.Flink = NULL;
        Endpoint->DispatchLink.Blink = NULL;

        USBPORT_EndpointWorker(Endpoint, TRUE);
        USBPORT_FlushPendingTransfers(Endpoint);
    }

    KeAcquireSpinLockAtDpcLevel(&FdoExtension->EndpointListSpinLock);

    if (!IsListEmpty(&FdoExtension->WorkerList))
    {
        USBPORT_SignalWorkerThread(FdoDevice);
    }

    KeReleaseSpinLockFromDpcLevel(&FdoExtension->EndpointListSpinLock);

    USBPORT_FlushDoneTransfers(FdoDevice);
}

VOID
NTAPI
USBPORT_IsrDpcHandler(IN PDEVICE_OBJECT FdoDevice,
                      IN BOOLEAN IsDpcHandler)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    PUSBPORT_ENDPOINT Endpoint;
    PLIST_ENTRY List;
    ULONG FrameNumber;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    DPRINT_CORE("USBPORT_IsrDpcHandler: IsDpcHandler - %x\n", IsDpcHandler);

    FdoExtension = FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    if (InterlockedIncrement(&FdoExtension->IsrDpcHandlerCounter))
    {
        KeInsertQueueDpc(&FdoExtension->IsrDpc, NULL, NULL);
        InterlockedDecrement(&FdoExtension->IsrDpcHandlerCounter);
        return;
    }

    for (List = ExInterlockedRemoveHeadList(&FdoExtension->EpStateChangeList,
                                            &FdoExtension->EpStateChangeSpinLock);
         List != NULL;
         List = ExInterlockedRemoveHeadList(&FdoExtension->EpStateChangeList,
                                            &FdoExtension->EpStateChangeSpinLock))
    {
        Endpoint = CONTAINING_RECORD(List,
                                     USBPORT_ENDPOINT,
                                     StateChangeLink);

        DPRINT_CORE("USBPORT_IsrDpcHandler: Endpoint - %p\n", Endpoint);

        KeAcquireSpinLockAtDpcLevel(&Endpoint->EndpointSpinLock);

        KeAcquireSpinLockAtDpcLevel(&FdoExtension->MiniportSpinLock);
        FrameNumber = Packet->Get32BitFrameNumber(FdoExtension->MiniPortExt);
        KeReleaseSpinLockFromDpcLevel(&FdoExtension->MiniportSpinLock);

        if (FrameNumber <= Endpoint->FrameNumber &&
            !(Endpoint->Flags & ENDPOINT_FLAG_NUKE))
        {
            KeReleaseSpinLockFromDpcLevel(&Endpoint->EndpointSpinLock);

            ExInterlockedInsertHeadList(&FdoExtension->EpStateChangeList,
                                        &Endpoint->StateChangeLink,
                                        &FdoExtension->EpStateChangeSpinLock);

            KeAcquireSpinLockAtDpcLevel(&FdoExtension->MiniportSpinLock);
            Packet->InterruptNextSOF(FdoExtension->MiniPortExt);
            KeReleaseSpinLockFromDpcLevel(&FdoExtension->MiniportSpinLock);

            break;
        }

        KeReleaseSpinLockFromDpcLevel(&Endpoint->EndpointSpinLock);

        KeAcquireSpinLockAtDpcLevel(&Endpoint->StateChangeSpinLock);
        Endpoint->StateLast = Endpoint->StateNext;
        KeReleaseSpinLockFromDpcLevel(&Endpoint->StateChangeSpinLock);

        DPRINT_CORE("USBPORT_IsrDpcHandler: Endpoint->StateLast - %x\n",
                    Endpoint->StateLast);

        if (IsDpcHandler)
        {
            USBPORT_InvalidateEndpointHandler(FdoDevice,
                                              Endpoint,
                                              INVALIDATE_ENDPOINT_ONLY);
        }
        else
        {
            USBPORT_InvalidateEndpointHandler(FdoDevice,
                                              Endpoint,
                                              INVALIDATE_ENDPOINT_WORKER_THREAD);
        }
    }

    if (IsDpcHandler)
    {
        USBPORT_DpcHandler(FdoDevice);
    }

    InterlockedDecrement(&FdoExtension->IsrDpcHandlerCounter);
}

VOID
NTAPI
USBPORT_IsrDpc(IN PRKDPC Dpc,
               IN PVOID DeferredContext,
               IN PVOID SystemArgument1,
               IN PVOID SystemArgument2)
{
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    BOOLEAN InterruptEnable;

    DPRINT_INT("USBPORT_IsrDpc: DeferredContext - %p, SystemArgument2 - %p\n",
               DeferredContext,
               SystemArgument2);

    FdoDevice = DeferredContext;
    FdoExtension = FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    if (SystemArgument2)
    {
        InterlockedDecrement(&FdoExtension->IsrDpcCounter);
    }

    KeAcquireSpinLockAtDpcLevel(&FdoExtension->MiniportInterruptsSpinLock);
    InterruptEnable = (FdoExtension->Flags & USBPORT_FLAG_INTERRUPT_ENABLED) ==
                       USBPORT_FLAG_INTERRUPT_ENABLED;

    Packet->InterruptDpc(FdoExtension->MiniPortExt, InterruptEnable);

    KeReleaseSpinLockFromDpcLevel(&FdoExtension->MiniportInterruptsSpinLock);

    if (FdoExtension->Flags & USBPORT_FLAG_HC_SUSPEND &&
        FdoExtension->TimerFlags & USBPORT_TMFLAG_WAKE)
    {
        USBPORT_CompletePdoWaitWake(FdoDevice);
    }
    else
    {
        USBPORT_IsrDpcHandler(FdoDevice, TRUE);
    }

    DPRINT_INT("USBPORT_IsrDpc: exit\n");
}

BOOLEAN
NTAPI
USBPORT_InterruptService(IN PKINTERRUPT Interrupt,
                         IN PVOID ServiceContext)
{
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    BOOLEAN Result = FALSE;

    FdoDevice = ServiceContext;
    FdoExtension = FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    DPRINT_INT("USBPORT_InterruptService: FdoExtension[%p]->Flags - %08X\n",
           FdoExtension,
           FdoExtension->Flags);

    if (FdoExtension->Flags & USBPORT_FLAG_INTERRUPT_ENABLED &&
        FdoExtension->MiniPortFlags & USBPORT_MPFLAG_INTERRUPTS_ENABLED)
    {
        Result = Packet->InterruptService(FdoExtension->MiniPortExt);

        if (Result)
        {
            KeInsertQueueDpc(&FdoExtension->IsrDpc, NULL, NULL);
        }
    }

    DPRINT_INT("USBPORT_InterruptService: return - %x\n", Result);

    return Result;
}

VOID
NTAPI
USBPORT_SignalWorkerThread(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    KIRQL OldIrql;

    DPRINT_CORE("USBPORT_SignalWorkerThread ... \n");

    FdoExtension = FdoDevice->DeviceExtension;

    KeAcquireSpinLock(&FdoExtension->WorkerThreadEventSpinLock, &OldIrql);
    KeSetEvent(&FdoExtension->WorkerThreadEvent, EVENT_INCREMENT, FALSE);
    KeReleaseSpinLock(&FdoExtension->WorkerThreadEventSpinLock, OldIrql);
}

VOID
NTAPI
USBPORT_WorkerThreadHandler(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    PLIST_ENTRY workerList;
    KIRQL OldIrql;
    PUSBPORT_ENDPOINT Endpoint;
    LIST_ENTRY list;
    BOOLEAN Result;

    DPRINT_CORE("USBPORT_WorkerThreadHandler: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);

    if (!(FdoExtension->Flags & USBPORT_FLAG_HC_SUSPEND))
    {
        Packet->CheckController(FdoExtension->MiniPortExt);
    }

    KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);

    InitializeListHead(&list);

    USBPORT_FlushAllEndpoints(FdoDevice);

    while (TRUE)
    {
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        KeAcquireSpinLockAtDpcLevel(&FdoExtension->EndpointListSpinLock);

        workerList = &FdoExtension->WorkerList;

        if (IsListEmpty(workerList))
            break;

        Endpoint = CONTAINING_RECORD(workerList->Flink,
                                     USBPORT_ENDPOINT,
                                     WorkerLink);

        DPRINT_CORE("USBPORT_WorkerThreadHandler: Endpoint - %p\n", Endpoint);

        RemoveHeadList(workerList);
        Endpoint->WorkerLink.Blink = NULL;
        Endpoint->WorkerLink.Flink = NULL;

        KeReleaseSpinLockFromDpcLevel(&FdoExtension->EndpointListSpinLock);

        Result = USBPORT_EndpointWorker(Endpoint, FALSE);
        KeAcquireSpinLockAtDpcLevel(&FdoExtension->EndpointListSpinLock);

        if (Result)
        {
            if (Endpoint->FlushAbortLink.Flink == NULL ||
                Endpoint->FlushAbortLink.Blink == NULL)
            {
                InsertTailList(&list, &Endpoint->FlushAbortLink);
            }
        }

        while (!IsListEmpty(&list))
        {
            Endpoint = CONTAINING_RECORD(list.Flink,
                                         USBPORT_ENDPOINT,
                                         FlushAbortLink);

            RemoveHeadList(&list);

            Endpoint->FlushAbortLink.Flink = NULL;
            Endpoint->FlushAbortLink.Blink = NULL;

            if (Endpoint->WorkerLink.Flink == NULL ||
                Endpoint->WorkerLink.Blink == NULL)
            {
                InsertTailList(&FdoExtension->WorkerList,
                               &Endpoint->WorkerLink);

                USBPORT_SignalWorkerThread(FdoDevice);
            }
        }

        KeReleaseSpinLockFromDpcLevel(&FdoExtension->EndpointListSpinLock);
        KeLowerIrql(OldIrql);
    }

    KeReleaseSpinLockFromDpcLevel(&FdoExtension->EndpointListSpinLock);
    KeLowerIrql(OldIrql);

    USBPORT_FlushClosedEndpointList(FdoDevice);
}

VOID
NTAPI
USBPORT_DoRootHubCallback(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PRH_INIT_CALLBACK RootHubInitCallback;
    PVOID RootHubInitContext;

    FdoExtension = FdoDevice->DeviceExtension;

    DPRINT("USBPORT_DoRootHubCallback: FdoDevice - %p\n", FdoDevice);

    PdoDevice = FdoExtension->RootHubPdo;

    if (PdoDevice)
    {
        PdoExtension = PdoDevice->DeviceExtension;

        RootHubInitContext = PdoExtension->RootHubInitContext;
        RootHubInitCallback = PdoExtension->RootHubInitCallback;

        PdoExtension->RootHubInitCallback = NULL;
        PdoExtension->RootHubInitContext = NULL;

        if (RootHubInitCallback)
        {
            RootHubInitCallback(RootHubInitContext);
        }
    }

    DPRINT("USBPORT_DoRootHubCallback: exit\n");
}

VOID
NTAPI
USBPORT_SynchronizeRootHubCallback(IN PDEVICE_OBJECT FdoDevice,
                                   IN PDEVICE_OBJECT Usb2FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    PUSBPORT_DEVICE_EXTENSION Usb2FdoExtension;
    PDEVICE_RELATIONS CompanionControllersList;
    PUSBPORT_DEVICE_EXTENSION CompanionFdoExtension;
    PDEVICE_OBJECT * Entry;
    ULONG ix;

    DPRINT("USBPORT_SynchronizeRootHubCallback: FdoDevice - %p, Usb2FdoDevice - %p\n",
           FdoDevice,
           Usb2FdoDevice);

    FdoExtension = FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    if (Usb2FdoDevice == NULL &&
        !(Packet->MiniPortFlags & USB_MINIPORT_FLAGS_USB2))
    {
        /* Not Companion USB11 Controller */
        USBPORT_DoRootHubCallback(FdoDevice);

        FdoExtension->Flags &= ~USBPORT_FLAG_RH_INIT_CALLBACK;
        InterlockedCompareExchange(&FdoExtension->RHInitCallBackLock, 0, 1);

        DPRINT("USBPORT_SynchronizeRootHubCallback: exit \n");
        return;
    }

    /* USB2 or Companion USB11 */

    DPRINT("USBPORT_SynchronizeRootHubCallback: FdoExtension->Flags - %p\n",
           FdoExtension->Flags);

    if (!(FdoExtension->Flags & USBPORT_FLAG_COMPANION_HC))
    {
        KeWaitForSingleObject(&FdoExtension->ControllerSemaphore,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        FdoExtension->Flags |= USBPORT_FLAG_PWR_AND_CHIRP_LOCK;

        if (!(FdoExtension->Flags & (USBPORT_FLAG_HC_SUSPEND |
                                     USBPORT_FLAG_POWER_AND_CHIRP_OK)))
        {
            USBPORT_RootHubPowerAndChirpAllCcPorts(FdoDevice);
            FdoExtension->Flags |= USBPORT_FLAG_POWER_AND_CHIRP_OK;
        }

        FdoExtension->Flags &= ~USBPORT_FLAG_PWR_AND_CHIRP_LOCK;

        KeReleaseSemaphore(&FdoExtension->ControllerSemaphore,
                           LOW_REALTIME_PRIORITY,
                           1,
                           FALSE);

        CompanionControllersList = USBPORT_FindCompanionControllers(FdoDevice,
                                                                    FALSE,
                                                                    TRUE);

        if (CompanionControllersList)
        {
            Entry = &CompanionControllersList->Objects[0];

            for (ix = 0; ix < CompanionControllersList->Count; ++ix)
            {
                CompanionFdoExtension = ((*Entry)->DeviceExtension);

                InterlockedCompareExchange(&CompanionFdoExtension->RHInitCallBackLock,
                                           0,
                                           1);

                ++Entry;
            }

            ExFreePoolWithTag(CompanionControllersList, USB_PORT_TAG);
        }

        USBPORT_DoRootHubCallback(FdoDevice);

        FdoExtension->Flags &= ~USBPORT_FLAG_RH_INIT_CALLBACK;
        InterlockedCompareExchange(&FdoExtension->RHInitCallBackLock, 0, 1);
    }
    else
    {
        Usb2FdoExtension = Usb2FdoDevice->DeviceExtension;

        USBPORT_Wait(FdoDevice, 50);

        while (FdoExtension->RHInitCallBackLock)
        {
            USBPORT_Wait(FdoDevice, 10);

            Usb2FdoExtension->Flags |= USBPORT_FLAG_RH_INIT_CALLBACK;
            USBPORT_SignalWorkerThread(Usb2FdoDevice);
        }

        USBPORT_DoRootHubCallback(FdoDevice);

        FdoExtension->Flags &= ~USBPORT_FLAG_RH_INIT_CALLBACK;
    }

    DPRINT("USBPORT_SynchronizeRootHubCallback: exit \n");
}

VOID
NTAPI
USBPORT_WorkerThread(IN PVOID StartContext)
{
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    LARGE_INTEGER OldTime;
    LARGE_INTEGER NewTime;
    KIRQL OldIrql;

    DPRINT_CORE("USBPORT_WorkerThread ... \n");

    FdoDevice = StartContext;
    FdoExtension = FdoDevice->DeviceExtension;

    FdoExtension->WorkerThread = KeGetCurrentThread();

    do
    {
        KeQuerySystemTime(&OldTime);

        KeWaitForSingleObject(&FdoExtension->WorkerThreadEvent,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);

        if (FdoExtension->Flags & USBPORT_FLAG_WORKER_THREAD_EXIT)
        {
            break;
        }

        KeQuerySystemTime(&NewTime);

        KeAcquireSpinLock(&FdoExtension->WorkerThreadEventSpinLock, &OldIrql);
        KeClearEvent(&FdoExtension->WorkerThreadEvent);
        KeReleaseSpinLock(&FdoExtension->WorkerThreadEventSpinLock, OldIrql);
        DPRINT_CORE("USBPORT_WorkerThread: run \n");

        if (FdoExtension->MiniPortFlags & USBPORT_MPFLAG_INTERRUPTS_ENABLED)
        {
            USBPORT_DoSetPowerD0(FdoDevice);

            if (FdoExtension->Flags & USBPORT_FLAG_RH_INIT_CALLBACK)
            {
                PDEVICE_OBJECT USB2FdoDevice = NULL;

                USB2FdoDevice = USBPORT_FindUSB2Controller(FdoDevice);
                USBPORT_SynchronizeRootHubCallback(FdoDevice, USB2FdoDevice);
            }
        }

        USBPORT_WorkerThreadHandler(FdoDevice);
    }
    while (!(FdoExtension->Flags & USBPORT_FLAG_WORKER_THREAD_ON));

    PsTerminateSystemThread(0);
}

NTSTATUS
NTAPI
USBPORT_CreateWorkerThread(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    NTSTATUS Status;

    DPRINT("USBPORT_CreateWorkerThread ... \n");

    FdoExtension = FdoDevice->DeviceExtension;

    FdoExtension->Flags &= ~USBPORT_FLAG_WORKER_THREAD_ON;

    KeInitializeEvent(&FdoExtension->WorkerThreadEvent,
                      NotificationEvent,
                      FALSE);

    Status = PsCreateSystemThread(&FdoExtension->WorkerThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  NULL,
                                  NULL,
                                  NULL,
                                  USBPORT_WorkerThread,
                                  (PVOID)FdoDevice);

    return Status;
}

VOID
NTAPI
USBPORT_StopWorkerThread(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    NTSTATUS Status;

    DPRINT("USBPORT_StopWorkerThread ... \n");

    FdoExtension = FdoDevice->DeviceExtension;

    FdoExtension->Flags |= USBPORT_FLAG_WORKER_THREAD_EXIT;
    USBPORT_SignalWorkerThread(FdoDevice);
    Status = ZwWaitForSingleObject(FdoExtension->WorkerThreadHandle, FALSE, NULL);
    NT_ASSERT(Status == STATUS_SUCCESS);
}

VOID
NTAPI
USBPORT_SynchronizeControllersStart(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PDEVICE_OBJECT USB2FdoDevice = NULL;
    PUSBPORT_DEVICE_EXTENSION USB2FdoExtension;
    BOOLEAN IsOn; 

    DPRINT_TIMER("USBPORT_SynchronizeControllersStart: FdoDevice - %p\n",
                 FdoDevice);

    FdoExtension = FdoDevice->DeviceExtension;

    PdoDevice = FdoExtension->RootHubPdo;

    if (!PdoDevice)
    {
        return;
    }

    PdoExtension = PdoDevice->DeviceExtension;

    if (PdoExtension->RootHubInitCallback == NULL ||
        FdoExtension->Flags & USBPORT_FLAG_RH_INIT_CALLBACK)
    {
        return;
    }

    DPRINT_TIMER("USBPORT_SynchronizeControllersStart: Flags - %p\n",
                 FdoExtension->Flags);

    if (FdoExtension->Flags & USBPORT_FLAG_COMPANION_HC)
    {
        IsOn = FALSE;

        USB2FdoDevice = USBPORT_FindUSB2Controller(FdoDevice);

        DPRINT_TIMER("USBPORT_SynchronizeControllersStart: USB2FdoDevice - %p\n",
                     USB2FdoDevice);

        if (USB2FdoDevice)
        {
            USB2FdoExtension = USB2FdoDevice->DeviceExtension;

            if (USB2FdoExtension->CommonExtension.PnpStateFlags &
                USBPORT_PNP_STATE_STARTED)
            {
                IsOn = TRUE;
            }
        }

        if (!(FdoExtension->Flags & USBPORT_FLAG_NO_HACTION))
        {
            goto Start;
        }

        USB2FdoDevice = NULL;
    }

    IsOn = TRUE;

  Start:

    if (IsOn &&
        !InterlockedCompareExchange(&FdoExtension->RHInitCallBackLock, 1, 0))
    {
        FdoExtension->Flags |= USBPORT_FLAG_RH_INIT_CALLBACK;
        USBPORT_SignalWorkerThread(FdoDevice);

        if (USB2FdoDevice)
        {
            USB2FdoExtension = USB2FdoDevice->DeviceExtension;

            USB2FdoExtension->Flags |= USBPORT_FLAG_RH_INIT_CALLBACK;
            USBPORT_SignalWorkerThread(USB2FdoDevice);
        }
    }

    DPRINT_TIMER("USBPORT_SynchronizeControllersStart: exit\n");
}

VOID
NTAPI
USBPORT_TimerDpc(IN PRKDPC Dpc,
                 IN PVOID DeferredContext,
                 IN PVOID SystemArgument1,
                 IN PVOID SystemArgument2)
{
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    LARGE_INTEGER DueTime = {{0, 0}};
    ULONG TimerFlags;
    PTIMER_WORK_QUEUE_ITEM IdleQueueItem;
    KIRQL OldIrql;
    KIRQL TimerOldIrql;

    DPRINT_TIMER("USBPORT_TimerDpc: Dpc - %p, DeferredContext - %p\n",
           Dpc,
           DeferredContext);

    FdoDevice = DeferredContext;
    FdoExtension = FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    KeAcquireSpinLock(&FdoExtension->TimerFlagsSpinLock, &TimerOldIrql);

    TimerFlags = FdoExtension->TimerFlags;

    DPRINT_TIMER("USBPORT_TimerDpc: Flags - %p, TimerFlags - %p\n",
                 FdoExtension->Flags,
                 TimerFlags);

    if (FdoExtension->Flags & USBPORT_FLAG_HC_SUSPEND &&
        FdoExtension->Flags & USBPORT_FLAG_HC_WAKE_SUPPORT &&
        !(TimerFlags & USBPORT_TMFLAG_HC_RESUME))
    {
        KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);
        Packet->PollController(FdoExtension->MiniPortExt);
        KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);
    }

    USBPORT_SynchronizeControllersStart(FdoDevice);

    if (TimerFlags & USBPORT_TMFLAG_HC_SUSPENDED)
    {
        USBPORT_BadRequestFlush(FdoDevice);
        goto Exit;
    }

    KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);

    if (!(FdoExtension->Flags & USBPORT_FLAG_HC_SUSPEND))
    {
        Packet->CheckController(FdoExtension->MiniPortExt);
    }

    KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);

    if (FdoExtension->Flags & USBPORT_FLAG_HC_POLLING)
    {
        KeAcquireSpinLock(&FdoExtension->MiniportSpinLock, &OldIrql);
        Packet->PollController(FdoExtension->MiniPortExt);
        KeReleaseSpinLock(&FdoExtension->MiniportSpinLock, OldIrql);
    }

    USBPORT_IsrDpcHandler(FdoDevice, FALSE);

    DPRINT_TIMER("USBPORT_TimerDpc: USBPORT_TimeoutAllEndpoints UNIMPLEMENTED.\n");
    //USBPORT_TimeoutAllEndpoints(FdoDevice);
    DPRINT_TIMER("USBPORT_TimerDpc: USBPORT_CheckIdleEndpoints UNIMPLEMENTED.\n");
    //USBPORT_CheckIdleEndpoints(FdoDevice);

    USBPORT_BadRequestFlush(FdoDevice);

    if (FdoExtension->IdleLockCounter > -1 &&
        !(TimerFlags & USBPORT_TMFLAG_IDLE_QUEUEITEM_ON))
    {
        IdleQueueItem = ExAllocatePoolWithTag(NonPagedPool,
                                              sizeof(TIMER_WORK_QUEUE_ITEM),
                                              USB_PORT_TAG);

        DPRINT("USBPORT_TimerDpc: IdleLockCounter - %x, IdleQueueItem - %p\n",
               FdoExtension->IdleLockCounter,
               IdleQueueItem);

        if (IdleQueueItem)
        {
            RtlZeroMemory(IdleQueueItem, sizeof(TIMER_WORK_QUEUE_ITEM));

            IdleQueueItem->WqItem.List.Flink = NULL;
            IdleQueueItem->WqItem.WorkerRoutine = USBPORT_DoIdleNotificationCallback;
            IdleQueueItem->WqItem.Parameter = IdleQueueItem;

            IdleQueueItem->FdoDevice = FdoDevice;
            IdleQueueItem->Context = 0;

            FdoExtension->TimerFlags |= USBPORT_TMFLAG_IDLE_QUEUEITEM_ON;

            ExQueueWorkItem(&IdleQueueItem->WqItem, CriticalWorkQueue);
        }
    }

Exit:

    KeReleaseSpinLock(&FdoExtension->TimerFlagsSpinLock, TimerOldIrql);

    if (TimerFlags & USBPORT_TMFLAG_TIMER_QUEUED)
    {
        DueTime.QuadPart -= FdoExtension->TimerValue * 10000 +
                            (KeQueryTimeIncrement() - 1);

        KeSetTimer(&FdoExtension->TimerObject,
                   DueTime,
                   &FdoExtension->TimerDpc);
    }

    DPRINT_TIMER("USBPORT_TimerDpc: exit\n");
}

BOOLEAN
NTAPI
USBPORT_StartTimer(IN PDEVICE_OBJECT FdoDevice,
                   IN ULONG Time)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    LARGE_INTEGER DueTime = {{0, 0}};
    ULONG TimeIncrement;
    BOOLEAN Result;

    DPRINT_TIMER("USBPORT_StartTimer: FdoDevice - %p, Time - %x\n",
           FdoDevice,
           Time);

    FdoExtension = FdoDevice->DeviceExtension;

    TimeIncrement = KeQueryTimeIncrement();

    FdoExtension->TimerFlags |= USBPORT_TMFLAG_TIMER_QUEUED;
    FdoExtension->TimerValue = Time;

    KeInitializeTimer(&FdoExtension->TimerObject);
    KeInitializeDpc(&FdoExtension->TimerDpc, USBPORT_TimerDpc, FdoDevice);

    DueTime.QuadPart -= 10000 * Time + (TimeIncrement - 1);

    Result = KeSetTimer(&FdoExtension->TimerObject,
                        DueTime,
                        &FdoExtension->TimerDpc);

    return Result;
}

PUSBPORT_COMMON_BUFFER_HEADER
NTAPI
USBPORT_AllocateCommonBuffer(IN PDEVICE_OBJECT FdoDevice,
                             IN SIZE_T BufferLength)
{
    PUSBPORT_COMMON_BUFFER_HEADER HeaderBuffer = NULL;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PDMA_ADAPTER DmaAdapter;
    PDMA_OPERATIONS DmaOperations;
    SIZE_T HeaderSize;
    ULONG Length = 0;
    ULONG LengthPadded;
    PHYSICAL_ADDRESS LogicalAddress;
    ULONG_PTR BaseVA;
    ULONG_PTR StartBufferVA;
    ULONG StartBufferPA;

    DPRINT("USBPORT_AllocateCommonBuffer: FdoDevice - %p, BufferLength - %p\n",
           FdoDevice,
           BufferLength);

    if (BufferLength == 0)
        goto Exit;

    FdoExtension = FdoDevice->DeviceExtension;

    DmaAdapter = FdoExtension->DmaAdapter;
    DmaOperations = DmaAdapter->DmaOperations;

    HeaderSize = sizeof(USBPORT_COMMON_BUFFER_HEADER);
    Length = ROUND_TO_PAGES(BufferLength + HeaderSize);
    LengthPadded = Length - (BufferLength + HeaderSize);

    BaseVA = (ULONG_PTR)DmaOperations->AllocateCommonBuffer(DmaAdapter,
                                                            Length,
                                                            &LogicalAddress,
                                                            TRUE);

    if (!BaseVA)
        goto Exit;

    StartBufferVA = BaseVA & ~(PAGE_SIZE - 1);
    StartBufferPA = LogicalAddress.LowPart & ~(PAGE_SIZE - 1);

    HeaderBuffer = (PUSBPORT_COMMON_BUFFER_HEADER)(StartBufferVA +
                                                   BufferLength +
                                                   LengthPadded);

    HeaderBuffer->Length = Length;
    HeaderBuffer->BaseVA = BaseVA;
    HeaderBuffer->LogicalAddress = LogicalAddress;

    HeaderBuffer->BufferLength = BufferLength + LengthPadded;
    HeaderBuffer->VirtualAddress = StartBufferVA;
    HeaderBuffer->PhysicalAddress = StartBufferPA;

    RtlZeroMemory((PVOID)StartBufferVA, BufferLength + LengthPadded);

Exit:
    return HeaderBuffer;
}

VOID
NTAPI
USBPORT_FreeCommonBuffer(IN PDEVICE_OBJECT FdoDevice,
                         IN PUSBPORT_COMMON_BUFFER_HEADER HeaderBuffer)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PDMA_ADAPTER DmaAdapter;
    PDMA_OPERATIONS DmaOperations;

    DPRINT("USBPORT_FreeCommonBuffer: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;

    DmaAdapter = FdoExtension->DmaAdapter;
    DmaOperations = DmaAdapter->DmaOperations;

    DmaOperations->FreeCommonBuffer(FdoExtension->DmaAdapter,
                                    HeaderBuffer->Length,
                                    HeaderBuffer->LogicalAddress,
                                    (PVOID)HeaderBuffer->VirtualAddress,
                                    TRUE);
}

PUSBPORT_MINIPORT_INTERFACE
NTAPI
USBPORT_FindMiniPort(IN PDRIVER_OBJECT DriverObject)
{
    KIRQL OldIrql;
    PLIST_ENTRY List;
    PUSBPORT_MINIPORT_INTERFACE MiniPortInterface;
    BOOLEAN IsFound = FALSE;

    DPRINT("USBPORT_FindMiniPort: ... \n");

    KeAcquireSpinLock(&USBPORT_SpinLock, &OldIrql);

    for (List = USBPORT_MiniPortDrivers.Flink;
         List != &USBPORT_MiniPortDrivers;
         List = List->Flink)
    {
        MiniPortInterface = CONTAINING_RECORD(List,
                                              USBPORT_MINIPORT_INTERFACE,
                                              DriverLink);

        if (MiniPortInterface->DriverObject == DriverObject)
        {
            DPRINT("USBPORT_FindMiniPort: find MiniPortInterface - %p\n",
                   MiniPortInterface);

            IsFound = TRUE;
            break;
        }
    }

    KeReleaseSpinLock(&USBPORT_SpinLock, OldIrql);

    if (IsFound)
        return MiniPortInterface;
    else
        return NULL;
    
}

NTSTATUS
NTAPI
USBPORT_AddDevice(IN PDRIVER_OBJECT DriverObject,
                  IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS Status;
    PUSBPORT_MINIPORT_INTERFACE MiniPortInterface;
    ULONG DeviceNumber = 0;
    WCHAR CharDeviceName[64];
    UNICODE_STRING DeviceName;
    PDEVICE_OBJECT DeviceObject;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_COMMON_DEVICE_EXTENSION FdoCommonExtension;
    PDEVICE_OBJECT LowerDevice;
    ULONG Length;

    DPRINT("USBPORT_AddDevice: DriverObject - %p, PhysicalDeviceObject - %p\n",
           DriverObject,
           PhysicalDeviceObject);

    MiniPortInterface = USBPORT_FindMiniPort(DriverObject);

    if (!MiniPortInterface)
    {
        DPRINT("USBPORT_AddDevice: USBPORT_FindMiniPort not found MiniPortInterface\n");
        return STATUS_UNSUCCESSFUL;
    }

    while (TRUE)
    {
        /* Construct device name */
        RtlStringCbPrintfW(CharDeviceName,
                           sizeof(CharDeviceName),
                           L"\\Device\\USBFDO-%d",
                           DeviceNumber);

        RtlInitUnicodeString(&DeviceName, CharDeviceName);

        ASSERT(MiniPortInterface->Packet.MiniPortExtensionSize <=
               MAXULONG - sizeof(USBPORT_DEVICE_EXTENSION) - sizeof(USB2_HC_EXTENSION));
        Length = (ULONG)(sizeof(USBPORT_DEVICE_EXTENSION) +
                         MiniPortInterface->Packet.MiniPortExtensionSize +
                         sizeof(USB2_HC_EXTENSION));

        /* Create device */
        Status = IoCreateDevice(DriverObject,
                                Length,
                                &DeviceName,
                                FILE_DEVICE_CONTROLLER,
                                0,
                                FALSE,
                                &DeviceObject);

        /* Check for success */
        if (NT_SUCCESS(Status)) break;

        /* Is there a device object with that same name */
        if ((Status == STATUS_OBJECT_NAME_EXISTS) ||
            (Status == STATUS_OBJECT_NAME_COLLISION))
        {
            /* Try the next name */
            DeviceNumber++;
            continue;
        }

        /* Bail out on other errors */
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("USBPORT_AddDevice: failed to create %wZ, Status %x\n",
                    &DeviceName,
                    Status);

            return Status;
        }
    }

    DPRINT("USBPORT_AddDevice: created device %p <%wZ>, Status %x\n",
           DeviceObject,
           &DeviceName,
           Status);

    FdoExtension = DeviceObject->DeviceExtension;
    FdoCommonExtension = &FdoExtension->CommonExtension;

    RtlZeroMemory(FdoExtension, sizeof(USBPORT_DEVICE_EXTENSION));

    FdoCommonExtension->SelfDevice = DeviceObject;
    FdoCommonExtension->LowerPdoDevice = PhysicalDeviceObject;
    FdoCommonExtension->IsPDO = FALSE;

    LowerDevice = IoAttachDeviceToDeviceStack(DeviceObject,
                                              PhysicalDeviceObject);

    FdoCommonExtension->LowerDevice = LowerDevice;

    FdoCommonExtension->DevicePowerState = PowerDeviceD3;

    FdoExtension->MiniPortExt = (PVOID)((ULONG_PTR)FdoExtension +
                                        sizeof(USBPORT_DEVICE_EXTENSION));

    if (MiniPortInterface->Packet.MiniPortFlags & USB_MINIPORT_FLAGS_USB2)
    {
        FdoExtension->Usb2Extension =
        (PUSB2_HC_EXTENSION)((ULONG_PTR)FdoExtension->MiniPortExt +
                             MiniPortInterface->Packet.MiniPortExtensionSize);

        DPRINT("USBPORT_AddDevice: Usb2Extension - %p\n",
               FdoExtension->Usb2Extension);

        USB2_InitController(FdoExtension->Usb2Extension);
    }
    else
    {
        FdoExtension->Usb2Extension = NULL;
    }

    FdoExtension->MiniPortInterface = MiniPortInterface;
    FdoExtension->FdoNameNumber = DeviceNumber;

    KeInitializeSemaphore(&FdoExtension->DeviceSemaphore, 1, 1);
    KeInitializeSemaphore(&FdoExtension->ControllerSemaphore, 1, 1);

    InitializeListHead(&FdoExtension->EndpointList);
    InitializeListHead(&FdoExtension->DoneTransferList);
    InitializeListHead(&FdoExtension->WorkerList);
    InitializeListHead(&FdoExtension->EpStateChangeList);
    InitializeListHead(&FdoExtension->MapTransferList);
    InitializeListHead(&FdoExtension->DeviceHandleList);
    InitializeListHead(&FdoExtension->IdleIrpList);
    InitializeListHead(&FdoExtension->BadRequestList);
    InitializeListHead(&FdoExtension->EndpointClosedList);

    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return Status;
}

VOID
NTAPI
USBPORT_Unload(IN PDRIVER_OBJECT DriverObject)
{
    PUSBPORT_MINIPORT_INTERFACE MiniPortInterface;

    DPRINT1("USBPORT_Unload: FIXME!\n");

    MiniPortInterface = USBPORT_FindMiniPort(DriverObject);

    if (!MiniPortInterface)
    {
        DPRINT("USBPORT_Unload: CRITICAL ERROR!!! Not found MiniPortInterface\n");
        KeBugCheckEx(BUGCODE_USB_DRIVER, 1, 0, 0, 0);
    }

    DPRINT1("USBPORT_Unload: UNIMPLEMENTED. FIXME. \n");
    //MiniPortInterface->DriverUnload(DriverObject); // Call MiniPort _HCI_Unload
}

VOID
NTAPI
USBPORT_MiniportCompleteTransfer(IN PVOID MiniPortExtension,
                                 IN PVOID MiniPortEndpoint,
                                 IN PVOID TransferParameters,
                                 IN USBD_STATUS USBDStatus,
                                 IN ULONG TransferLength)
{
    PUSBPORT_TRANSFER Transfer;
    PUSBPORT_TRANSFER ParentTransfer;
    PUSBPORT_TRANSFER SplitTransfer;
    PLIST_ENTRY SplitHead;
    PLIST_ENTRY Entry;
    KIRQL OldIrql;

    DPRINT_CORE("USBPORT_MiniportCompleteTransfer: USBDStatus - %x, TransferLength - %x\n",
                USBDStatus,
                TransferLength);

    Transfer = CONTAINING_RECORD(TransferParameters,
                                 USBPORT_TRANSFER,
                                 TransferParameters);

    Transfer->Flags |= TRANSFER_FLAG_COMPLETED;
    Transfer->CompletedTransferLen = TransferLength;

    if (((Transfer->Flags & TRANSFER_FLAG_SPLITED) == 0) ||
        TransferLength >= Transfer->TransferParameters.TransferBufferLength)
    {
        goto Exit;
    }

    ParentTransfer = Transfer->ParentTransfer;

    KeAcquireSpinLock(&ParentTransfer->TransferSpinLock, &OldIrql);

    if (IsListEmpty(&ParentTransfer->SplitTransfersList))
    {
        goto Exit;
    }

    SplitHead = &ParentTransfer->SplitTransfersList;
    Entry = SplitHead->Flink;

    while (Entry && !IsListEmpty(SplitHead))
    {
        SplitTransfer = CONTAINING_RECORD(Entry,
                                          USBPORT_TRANSFER,
                                          SplitLink);

        if (!(SplitTransfer->Flags & TRANSFER_FLAG_SUBMITED))
        {
            DPRINT1("USBPORT_MiniportCompleteTransfer: SplitTransfer->Flags - %X\n",
                    SplitTransfer->Flags);
            //Add TRANSFER_FLAG_xxx
        }

        Entry = Entry->Flink;
    }

    KeReleaseSpinLock(&ParentTransfer->TransferSpinLock, OldIrql);

Exit:
    USBPORT_QueueDoneTransfer(Transfer, USBDStatus);
}

VOID
NTAPI
USBPORT_AsyncTimerDpc(IN PRKDPC Dpc,
                      IN PVOID DeferredContext,
                      IN PVOID SystemArgument1,
                      IN PVOID SystemArgument2)
{
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_ASYNC_CALLBACK_DATA AsyncCallbackData;

    DPRINT("USBPORT_AsyncTimerDpc: ... \n");

    AsyncCallbackData = DeferredContext;
    FdoDevice = AsyncCallbackData->FdoDevice;
    FdoExtension = FdoDevice->DeviceExtension;

    (*AsyncCallbackData->CallbackFunction)(FdoExtension->MiniPortExt,
                                           &AsyncCallbackData->CallbackContext);

    ExFreePoolWithTag(AsyncCallbackData, USB_PORT_TAG);
}

ULONG
NTAPI
USBPORT_RequestAsyncCallback(IN PVOID MiniPortExtension,
                             IN ULONG TimerValue,
                             IN PVOID Buffer,
                             IN SIZE_T Length,
                             IN ASYNC_TIMER_CALLBACK * Callback)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_ASYNC_CALLBACK_DATA AsyncCallbackData;
    LARGE_INTEGER DueTime = {{0, 0}};

    DPRINT("USBPORT_RequestAsyncCallback: ... \n");

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)((ULONG_PTR)MiniPortExtension -
                                               sizeof(USBPORT_DEVICE_EXTENSION));

    FdoDevice = FdoExtension->CommonExtension.SelfDevice;

    AsyncCallbackData = ExAllocatePoolWithTag(NonPagedPool,
                                              sizeof(USBPORT_ASYNC_CALLBACK_DATA) + Length,
                                              USB_PORT_TAG);

    if (!AsyncCallbackData)
    {
        DPRINT1("USBPORT_RequestAsyncCallback: Not allocated AsyncCallbackData!\n");
        return 0;
    }

    RtlZeroMemory(AsyncCallbackData,
                  sizeof(USBPORT_ASYNC_CALLBACK_DATA) + Length);

    if (Length)
    {
        RtlCopyMemory(&AsyncCallbackData->CallbackContext, Buffer, Length);
    }

    AsyncCallbackData->FdoDevice = FdoDevice;
    AsyncCallbackData->CallbackFunction = Callback;

    KeInitializeTimer(&AsyncCallbackData->AsyncTimer);

    KeInitializeDpc(&AsyncCallbackData->AsyncTimerDpc,
                    USBPORT_AsyncTimerDpc,
                    AsyncCallbackData);

    DueTime.QuadPart -= (KeQueryTimeIncrement() - 1) + 10000 * TimerValue;

    KeSetTimer(&AsyncCallbackData->AsyncTimer,
               DueTime,
               &AsyncCallbackData->AsyncTimerDpc);

    return 0;
}

PVOID
NTAPI
USBPORT_GetMappedVirtualAddress(IN ULONG PhysicalAddress,
                                IN PVOID MiniPortExtension,
                                IN PVOID MiniPortEndpoint)
{
    PUSBPORT_COMMON_BUFFER_HEADER HeaderBuffer;
    PUSBPORT_ENDPOINT Endpoint;
    ULONG Offset;
    ULONG_PTR VirtualAddress;

    DPRINT_CORE("USBPORT_GetMappedVirtualAddress ... \n");

    Endpoint = (PUSBPORT_ENDPOINT)((ULONG_PTR)MiniPortEndpoint -
                                   sizeof(USBPORT_ENDPOINT));

    if (!Endpoint)
    {
        ASSERT(FALSE);
    }

    HeaderBuffer = Endpoint->HeaderBuffer;

    Offset = PhysicalAddress - HeaderBuffer->PhysicalAddress;
    VirtualAddress = HeaderBuffer->VirtualAddress + Offset;

    return (PVOID)VirtualAddress;
}

ULONG
NTAPI
USBPORT_InvalidateEndpoint(IN PVOID MiniPortExtension,
                           IN PVOID MiniPortEndpoint)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_ENDPOINT Endpoint;

    DPRINT_CORE("USBPORT_InvalidateEndpoint: ... \n");

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)((ULONG_PTR)MiniPortExtension -
                                               sizeof(USBPORT_DEVICE_EXTENSION));

    FdoDevice = FdoExtension->CommonExtension.SelfDevice;

    if (!MiniPortEndpoint)
    {
        USBPORT_InvalidateEndpointHandler(FdoDevice,
                                          NULL,
                                          INVALIDATE_ENDPOINT_ONLY);
        return 0;
    }

    Endpoint = (PUSBPORT_ENDPOINT)((ULONG_PTR)MiniPortEndpoint -
                                   sizeof(USBPORT_ENDPOINT));

    USBPORT_InvalidateEndpointHandler(FdoDevice,
                                      Endpoint,
                                      INVALIDATE_ENDPOINT_ONLY);

    return 0;
}

VOID
NTAPI
USBPORT_CompleteTransfer(IN PURB Urb,
                         IN USBD_STATUS TransferStatus)
{
    struct _URB_CONTROL_TRANSFER *UrbTransfer;
    PUSBPORT_TRANSFER Transfer;
    NTSTATUS Status;
    PIRP Irp;
    KIRQL OldIrql;
    PRKEVENT Event;
    BOOLEAN WriteToDevice;
    BOOLEAN IsFlushSuccess;
    PMDL Mdl;
    ULONG_PTR CurrentVa;
    SIZE_T TransferLength;
    PUSBPORT_ENDPOINT Endpoint;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PDMA_OPERATIONS DmaOperations;

    DPRINT("USBPORT_CompleteTransfer: Urb - %p, TransferStatus - %X\n",
           Urb,
           TransferStatus);

    UrbTransfer = &Urb->UrbControlTransfer;
    Transfer = UrbTransfer->hca.Reserved8[0];

    Transfer->USBDStatus = TransferStatus;
    Status = USBPORT_USBDStatusToNtStatus(Urb, TransferStatus);

    UrbTransfer->TransferBufferLength = Transfer->CompletedTransferLen;

    if (Transfer->Flags & TRANSFER_FLAG_DMA_MAPPED)
    {
        Endpoint = Transfer->Endpoint;
        FdoDevice = Endpoint->FdoDevice;
        FdoExtension = FdoDevice->DeviceExtension;
        DmaOperations = FdoExtension->DmaAdapter->DmaOperations;

        WriteToDevice = Transfer->Direction == USBPORT_DMA_DIRECTION_TO_DEVICE;
        Mdl = UrbTransfer->TransferBufferMDL;
        CurrentVa = (ULONG_PTR)MmGetMdlVirtualAddress(Mdl);
        TransferLength = UrbTransfer->TransferBufferLength;

        IsFlushSuccess = DmaOperations->FlushAdapterBuffers(FdoExtension->DmaAdapter,
                                                            Mdl,
                                                            Transfer->MapRegisterBase,
                                                            (PVOID)CurrentVa,
                                                            TransferLength,
                                                            WriteToDevice);

        if (!IsFlushSuccess)
        {
            DPRINT("USBPORT_CompleteTransfer: no FlushAdapterBuffers !!!\n");
            ASSERT(FALSE);
        }

        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

        DmaOperations->FreeMapRegisters(FdoExtension->DmaAdapter,
                                        Transfer->MapRegisterBase,
                                        Transfer->NumberOfMapRegisters);

        KeLowerIrql(OldIrql);
    }

    if (Urb->UrbHeader.UsbdFlags & USBD_FLAG_ALLOCATED_MDL)
    {
        IoFreeMdl(Transfer->TransferBufferMDL);
        Urb->UrbHeader.UsbdFlags |= ~USBD_FLAG_ALLOCATED_MDL;
    }

    Urb->UrbControlTransfer.hca.Reserved8[0] = NULL;
    Urb->UrbHeader.UsbdFlags |= ~USBD_FLAG_ALLOCATED_TRANSFER;

    Irp = Transfer->Irp;

    if (Irp)
    {
        if (!NT_SUCCESS(Status))
        {
            //DbgBreakPoint();
            DPRINT1("USBPORT_CompleteTransfer: Irp - %p complete with Status - %lx\n",
                    Irp,
                    Status);

            USBPORT_DumpingURB(Urb);
        }

        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;

        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        KeLowerIrql(OldIrql);
    }

    Event = Transfer->Event;

    if (Event)
    {
        KeSetEvent(Event, EVENT_INCREMENT, FALSE);
    }

    ExFreePoolWithTag(Transfer, USB_PORT_TAG);

    DPRINT_CORE("USBPORT_CompleteTransfer: exit\n");
}

IO_ALLOCATION_ACTION
NTAPI
USBPORT_MapTransfer(IN PDEVICE_OBJECT FdoDevice,
                    IN PIRP Irp,
                    IN PVOID MapRegisterBase,
                    IN PVOID Context)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PDMA_ADAPTER DmaAdapter;
    PUSBPORT_TRANSFER Transfer;
    PURB Urb;
    PUSBPORT_ENDPOINT Endpoint;
    PMDL Mdl;
    ULONG_PTR CurrentVa;
    PUSBPORT_SCATTER_GATHER_LIST sgList;
    SIZE_T CurrentLength;
    ULONG ix;
    BOOLEAN WriteToDevice;
    PHYSICAL_ADDRESS PhAddr = {{0, 0}};
    PHYSICAL_ADDRESS PhAddress = {{0, 0}};
    ULONG TransferLength;
    SIZE_T SgCurrentLength;
    SIZE_T ElementLength;
    PUSBPORT_DEVICE_HANDLE DeviceHandle;
    PDMA_OPERATIONS DmaOperations;
    USBD_STATUS USBDStatus;
    LIST_ENTRY List;
    PUSBPORT_TRANSFER transfer;

    DPRINT_CORE("USBPORT_MapTransfer: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;
    DmaAdapter = FdoExtension->DmaAdapter;
    DmaOperations = DmaAdapter->DmaOperations;

    Transfer = Context;

    Urb = Transfer->Urb;
    Endpoint = Transfer->Endpoint;
    TransferLength = Transfer->TransferParameters.TransferBufferLength;

    Mdl = Urb->UrbControlTransfer.TransferBufferMDL;
    CurrentVa = (ULONG_PTR)MmGetMdlVirtualAddress(Mdl);

    sgList = &Transfer->SgList;

    sgList->Flags = 0;
    sgList->CurrentVa = CurrentVa;
    sgList->MappedSystemVa = MmGetSystemAddressForMdlSafe(Mdl,
                                                          NormalPagePriority);
    Transfer->MapRegisterBase = MapRegisterBase;

    ix = 0;
    CurrentLength = 0;

    do
    {
        WriteToDevice = Transfer->Direction == USBPORT_DMA_DIRECTION_TO_DEVICE;
        ASSERT(Transfer->Direction != 0);

        PhAddress = DmaOperations->MapTransfer(DmaAdapter,
                                               Mdl,
                                               MapRegisterBase,
                                               (PVOID)CurrentVa,
                                               &TransferLength,
                                               WriteToDevice);

        DPRINT_CORE("USBPORT_MapTransfer: PhAddress.LowPart - %p, PhAddress.HighPart - %x, TransferLength - %x\n",
               PhAddress.LowPart,
               PhAddress.HighPart,
               TransferLength);

        PhAddress.HighPart = 0;
        SgCurrentLength = TransferLength;

        do
        {
            ElementLength = PAGE_SIZE - (PhAddress.LowPart & (PAGE_SIZE - 1));

            if (ElementLength > SgCurrentLength)
                ElementLength = SgCurrentLength;

            DPRINT_CORE("USBPORT_MapTransfer: PhAddress.LowPart - %p, HighPart - %x, ElementLength - %x\n",
                   PhAddress.LowPart,
                   PhAddress.HighPart,
                   ElementLength);

            sgList->SgElement[ix].SgPhysicalAddress = PhAddress;
            sgList->SgElement[ix].SgTransferLength = ElementLength;
            sgList->SgElement[ix].SgOffset = CurrentLength +
                                             (TransferLength - SgCurrentLength);

            PhAddress.LowPart += ElementLength;
            SgCurrentLength -= ElementLength;

            ++ix;
        }
        while (SgCurrentLength);

        if (PhAddr.QuadPart == PhAddress.QuadPart)
        {
            DPRINT1("USBPORT_MapTransfer: PhAddr == PhAddress\n");
            ASSERT(FALSE);
        }

        PhAddr = PhAddress;

        CurrentLength += TransferLength;
        CurrentVa += TransferLength;

        TransferLength = Transfer->TransferParameters.TransferBufferLength -
                         CurrentLength;
    }
    while (CurrentLength != Transfer->TransferParameters.TransferBufferLength);

    sgList->SgElementCount = ix;

    if (Endpoint->EndpointProperties.DeviceSpeed == UsbHighSpeed)
    {
        Transfer->Flags |= TRANSFER_FLAG_HIGH_SPEED;
    }

    Transfer->Flags |= TRANSFER_FLAG_DMA_MAPPED;

    if ((Transfer->Flags & TRANSFER_FLAG_ISO) == 0)
    {
        KeAcquireSpinLock(&Endpoint->EndpointSpinLock,
                          &Endpoint->EndpointOldIrql);

        USBPORT_SplitTransfer(FdoDevice, Endpoint, Transfer, &List);

        while (!IsListEmpty(&List))
        {
            transfer = CONTAINING_RECORD(List.Flink,
                                         USBPORT_TRANSFER,
                                         TransferLink);

            RemoveHeadList(&List);
            InsertTailList(&Endpoint->TransferList, &transfer->TransferLink);
        }

        KeReleaseSpinLock(&Endpoint->EndpointSpinLock,
                          Endpoint->EndpointOldIrql);
    }
    else
    {
        USBDStatus = USBPORT_InitializeIsoTransfer(FdoDevice,
                                                   &Urb->UrbIsochronousTransfer,
                                                   Transfer);

        if (USBDStatus != USBD_STATUS_SUCCESS)
        {
            KeAcquireSpinLock(&Endpoint->EndpointSpinLock,
                              &Endpoint->EndpointOldIrql);

            USBPORT_QueueDoneTransfer(Transfer, USBDStatus);

            KeReleaseSpinLock(&Endpoint->EndpointSpinLock,
                              Endpoint->EndpointOldIrql);
        }
    }

    DeviceHandle = Urb->UrbHeader.UsbdDeviceHandle;
    InterlockedDecrement(&DeviceHandle->DeviceHandleLock);

    if (USBPORT_EndpointWorker(Endpoint, 0))
    {
        USBPORT_InvalidateEndpointHandler(FdoDevice,
                                          Endpoint,
                                          INVALIDATE_ENDPOINT_WORKER_THREAD);
    }

    return DeallocateObjectKeepRegisters;
}

VOID
NTAPI
USBPORT_FlushMapTransfers(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PLIST_ENTRY MapTransferList;
    PUSBPORT_TRANSFER Transfer;
    ULONG NumMapRegisters;
    PMDL Mdl;
    SIZE_T TransferBufferLength;
    ULONG_PTR VirtualAddr;
    KIRQL OldIrql;
    NTSTATUS Status;
    PDMA_OPERATIONS DmaOperations;

    DPRINT_CORE("USBPORT_FlushMapTransfers: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;
    DmaOperations = FdoExtension->DmaAdapter->DmaOperations;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    while (TRUE)
    {
        MapTransferList = &FdoExtension->MapTransferList;

        if (IsListEmpty(&FdoExtension->MapTransferList))
        {
            KeLowerIrql(OldIrql);
            return;
        }

        Transfer = CONTAINING_RECORD(MapTransferList->Flink,
                                     USBPORT_TRANSFER,
                                     TransferLink);

        RemoveHeadList(MapTransferList);

        Mdl = Transfer->Urb->UrbControlTransfer.TransferBufferMDL;
        TransferBufferLength = Transfer->TransferParameters.TransferBufferLength;
        VirtualAddr = (ULONG_PTR)MmGetMdlVirtualAddress(Mdl);

        NumMapRegisters = ADDRESS_AND_SIZE_TO_SPAN_PAGES(VirtualAddr,
                                                         TransferBufferLength);

        Transfer->NumberOfMapRegisters = NumMapRegisters;

        Status = DmaOperations->AllocateAdapterChannel(FdoExtension->DmaAdapter,
                                                       FdoDevice,
                                                       NumMapRegisters,
                                                       USBPORT_MapTransfer,
                                                       Transfer);

        if (!NT_SUCCESS(Status))
            ASSERT(FALSE);
    }

    KeLowerIrql(OldIrql);
}

USBD_STATUS
NTAPI
USBPORT_AllocateTransfer(IN PDEVICE_OBJECT FdoDevice,
                         IN PURB Urb,
                         IN PUSBPORT_DEVICE_HANDLE DeviceHandle,
                         IN PIRP Irp,
                         IN PRKEVENT Event)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    SIZE_T TransferLength;
    PMDL Mdl;
    ULONG_PTR VirtualAddr;
    ULONG PagesNeed = 0;
    SIZE_T PortTransferLength;
    SIZE_T FullTransferLength;
    PUSBPORT_TRANSFER Transfer;
    PUSBPORT_PIPE_HANDLE PipeHandle;
    USBD_STATUS USBDStatus;
    SIZE_T IsoBlockLen = 0;

    DPRINT_CORE("USBPORT_AllocateTransfer: FdoDevice - %p, Urb - %p, DeviceHandle - %p, Irp - %p, Event - %p\n",
           FdoDevice,
           Urb,
           DeviceHandle,
           Irp,
           Event);

    FdoExtension = FdoDevice->DeviceExtension;

    TransferLength = Urb->UrbControlTransfer.TransferBufferLength;
    PipeHandle = Urb->UrbControlTransfer.PipeHandle;

    if (TransferLength)
    {
        Mdl = Urb->UrbControlTransfer.TransferBufferMDL;
        VirtualAddr = (ULONG_PTR)MmGetMdlVirtualAddress(Mdl);

        PagesNeed = ADDRESS_AND_SIZE_TO_SPAN_PAGES(VirtualAddr,
                                                   TransferLength);
        if (PagesNeed > 0)
        {
            PagesNeed--;
        }
    }

    if (Urb->UrbHeader.Function == URB_FUNCTION_ISOCH_TRANSFER)
    {
        DPRINT1("USBPORT_AllocateTransfer: ISOCH_TRANSFER UNIMPLEMENTED. FIXME\n");

        //IsoBlockLen = sizeof(USBPORT_ISO_BLOCK) +
        //              Urb->UrbIsochronousTransfer.NumberOfPackets *
        //              sizeof(USBPORT_ISO_BLOCK_PACKET);
    }

    PortTransferLength = sizeof(USBPORT_TRANSFER) +
                         PagesNeed * sizeof(USBPORT_SCATTER_GATHER_ELEMENT) +
                         IsoBlockLen;

    FullTransferLength = PortTransferLength +
                         FdoExtension->MiniPortInterface->Packet.MiniPortTransferSize;

    Transfer = ExAllocatePoolWithTag(NonPagedPool,
                                     FullTransferLength,
                                     USB_PORT_TAG);

    if (!Transfer)
    {
        DPRINT1("USBPORT_AllocateTransfer: Transfer not allocated!\n");
        return USBD_STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Transfer, FullTransferLength);

    Transfer->Irp = Irp;
    Transfer->Urb = Urb;
    Transfer->Endpoint = PipeHandle->Endpoint;
    Transfer->Event = Event;
    Transfer->PortTransferLength = PortTransferLength;
    Transfer->FullTransferLength = FullTransferLength;
    Transfer->IsoBlockPtr = NULL;
    Transfer->Period = 0;
    Transfer->ParentTransfer = Transfer;

    if (IsoBlockLen)
    {
        Transfer->IsoBlockPtr = (PVOID)((ULONG_PTR)Transfer +
                                 PortTransferLength - IsoBlockLen);

        Transfer->Period = PipeHandle->Endpoint->EndpointProperties.Period;
        Transfer->Flags |= TRANSFER_FLAG_ISO;
    }

    Transfer->MiniportTransfer = (PVOID)((ULONG_PTR)Transfer +
                                                    PortTransferLength);

    KeInitializeSpinLock(&Transfer->TransferSpinLock);

    Urb->UrbControlTransfer.hca.Reserved8[0] = Transfer;
    Urb->UrbHeader.UsbdFlags |= USBD_FLAG_ALLOCATED_TRANSFER;

    USBDStatus = USBD_STATUS_SUCCESS;

    DPRINT_CORE("USBPORT_AllocateTransfer: return USBDStatus - %x\n",
                USBDStatus);

    return USBDStatus;
}

NTSTATUS
NTAPI
USBPORT_Dispatch(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    PUSBPORT_COMMON_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status = STATUS_SUCCESS;

    DeviceExtension = DeviceObject->DeviceExtension;
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (DeviceExtension->PnpStateFlags & USBPORT_PNP_STATE_FAILED)
    {
        DPRINT1("USBPORT_Dispatch: USBPORT_PNP_STATE_FAILED\n");
        DbgBreakPoint();
    }

    switch (IoStack->MajorFunction)
    {
        case IRP_MJ_DEVICE_CONTROL:
            if (DeviceExtension->IsPDO)
            {
                DPRINT("USBPORT_Dispatch: PDO IRP_MJ_DEVICE_CONTROL. Major - %d, Minor - %d\n",
                       IoStack->MajorFunction,
                       IoStack->MinorFunction);

                Status = USBPORT_PdoDeviceControl(DeviceObject, Irp);
            }
            else
            {
                DPRINT("USBPORT_Dispatch: FDO IRP_MJ_DEVICE_CONTROL. Major - %d, Minor - %d\n",
                       IoStack->MajorFunction,
                       IoStack->MinorFunction);

                Status = USBPORT_FdoDeviceControl(DeviceObject, Irp);
            }

            break;

        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            if (DeviceExtension->IsPDO)
            {
                DPRINT("USBPORT_Dispatch: PDO IRP_MJ_INTERNAL_DEVICE_CONTROL. Major - %d, Minor - %d\n",
                       IoStack->MajorFunction,
                       IoStack->MinorFunction);

                Status = USBPORT_PdoInternalDeviceControl(DeviceObject, Irp);
            }
            else
            {
                DPRINT("USBPORT_Dispatch: FDO IRP_MJ_INTERNAL_DEVICE_CONTROL. Major - %d, Minor - %d\n",
                       IoStack->MajorFunction,
                       IoStack->MinorFunction);

                Status = USBPORT_FdoInternalDeviceControl(DeviceObject, Irp);
            }

            break;

        case IRP_MJ_POWER:
            if (DeviceExtension->IsPDO)
            {
                DPRINT("USBPORT_Dispatch: PDO IRP_MJ_POWER. Major - %d, Minor - %d\n",
                       IoStack->MajorFunction,
                       IoStack->MinorFunction);

                Status = USBPORT_PdoPower(DeviceObject, Irp);
            }
            else
            {
                DPRINT("USBPORT_Dispatch: FDO IRP_MJ_POWER. Major - %d, Minor - %d\n",
                       IoStack->MajorFunction,
                       IoStack->MinorFunction);

                Status = USBPORT_FdoPower(DeviceObject, Irp);
            }

            break;

        case IRP_MJ_SYSTEM_CONTROL:
            if (DeviceExtension->IsPDO)
            {
                DPRINT("USBPORT_Dispatch: PDO IRP_MJ_SYSTEM_CONTROL. Major - %d, Minor - %d\n",
                       IoStack->MajorFunction,
                       IoStack->MinorFunction);

                Status = Irp->IoStatus.Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            else
            {
                DPRINT("USBPORT_Dispatch: FDO IRP_MJ_SYSTEM_CONTROL. Major - %d, Minor - %d\n",
                       IoStack->MajorFunction,
                       IoStack->MinorFunction);

                IoSkipCurrentIrpStackLocation(Irp);
                Status = IoCallDriver(DeviceExtension->LowerDevice, Irp);
            }

            break;

        case IRP_MJ_PNP:
            if (DeviceExtension->IsPDO)
            {
                DPRINT("USBPORT_Dispatch: PDO IRP_MJ_PNP. Major - %d, Minor - %d\n",
                       IoStack->MajorFunction,
                       IoStack->MinorFunction);

                Status = USBPORT_PdoPnP(DeviceObject, Irp);
            }
            else
            {
                DPRINT("USBPORT_Dispatch: FDO IRP_MJ_PNP. Major - %d, Minor - %d\n",
                       IoStack->MajorFunction,
                       IoStack->MinorFunction);

                Status = USBPORT_FdoPnP(DeviceObject, Irp);
            }

            break;

        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
            DPRINT("USBPORT_Dispatch: IRP_MJ_CREATE | IRP_MJ_CLOSE\n");
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        default:
            if (DeviceExtension->IsPDO)
            {
                DPRINT("USBPORT_Dispatch: PDO unhandled IRP_MJ_???. Major - %d, Minor - %d\n",
                       IoStack->MajorFunction,
                       IoStack->MinorFunction);
            }
            else
            {
                DPRINT("USBPORT_Dispatch: FDO unhandled IRP_MJ_???. Major - %d, Minor - %d\n",
                       IoStack->MajorFunction,
                       IoStack->MinorFunction);
            }

            Status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
    }

    DPRINT("USBPORT_Dispatch: Status - %x\n", Status);
    return Status;
}

ULONG
NTAPI
USBPORT_GetHciMn(VOID)
{
    return USBPORT_HCI_MN;
}

NTSTATUS
NTAPI
USBPORT_RegisterUSBPortDriver(IN PDRIVER_OBJECT DriverObject,
                              IN ULONG Version,
                              IN PUSBPORT_REGISTRATION_PACKET RegPacket)
{
    PUSBPORT_MINIPORT_INTERFACE MiniPortInterface;

    DPRINT("USBPORT_RegisterUSBPortDriver: DriverObject - %p, Version - %p, RegPacket - %p\n",
           DriverObject,
           Version,
           RegPacket);

    DPRINT("USBPORT_RegisterUSBPortDriver: sizeof(USBPORT_MINIPORT_INTERFACE) - %x\n",
           sizeof(USBPORT_MINIPORT_INTERFACE));

    DPRINT("USBPORT_RegisterUSBPortDriver: sizeof(USBPORT_DEVICE_EXTENSION)   - %x\n",
           sizeof(USBPORT_DEVICE_EXTENSION));

    if (Version < USB10_MINIPORT_INTERFACE_VERSION)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!USBPORT_Initialized)
    {
        InitializeListHead(&USBPORT_MiniPortDrivers);
        InitializeListHead(&USBPORT_USB1FdoList);
        InitializeListHead(&USBPORT_USB2FdoList);

        KeInitializeSpinLock(&USBPORT_SpinLock);
        USBPORT_Initialized = TRUE;
    }

    MiniPortInterface = ExAllocatePoolWithTag(NonPagedPool,
                                              sizeof(USBPORT_MINIPORT_INTERFACE),
                                              USB_PORT_TAG);
    if (!MiniPortInterface)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(MiniPortInterface, sizeof(USBPORT_MINIPORT_INTERFACE));

    MiniPortInterface->DriverObject = DriverObject;
    MiniPortInterface->DriverUnload = DriverObject->DriverUnload;
    MiniPortInterface->Version = Version;

    ExInterlockedInsertTailList(&USBPORT_MiniPortDrivers,
                                &MiniPortInterface->DriverLink,
                                &USBPORT_SpinLock);

    DriverObject->DriverExtension->AddDevice = USBPORT_AddDevice;
    DriverObject->DriverUnload = USBPORT_Unload;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = USBPORT_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = USBPORT_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = USBPORT_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = USBPORT_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP] = USBPORT_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER] = USBPORT_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = USBPORT_Dispatch;

    RegPacket->UsbPortDbgPrint = USBPORT_DbgPrint;
    RegPacket->UsbPortTestDebugBreak = USBPORT_TestDebugBreak;
    RegPacket->UsbPortAssertFailure = USBPORT_AssertFailure;
    RegPacket->UsbPortGetMiniportRegistryKeyValue = USBPORT_GetMiniportRegistryKeyValue;
    RegPacket->UsbPortInvalidateRootHub = USBPORT_InvalidateRootHub;
    RegPacket->UsbPortInvalidateEndpoint = USBPORT_InvalidateEndpoint;
    RegPacket->UsbPortCompleteTransfer = USBPORT_MiniportCompleteTransfer;
    RegPacket->UsbPortCompleteIsoTransfer = USBPORT_CompleteIsoTransfer;
    RegPacket->UsbPortLogEntry = USBPORT_LogEntry;
    RegPacket->UsbPortGetMappedVirtualAddress = USBPORT_GetMappedVirtualAddress;
    RegPacket->UsbPortRequestAsyncCallback = USBPORT_RequestAsyncCallback;
    RegPacket->UsbPortReadWriteConfigSpace = USBPORT_ReadWriteConfigSpace;
    RegPacket->UsbPortWait = USBPORT_Wait;
    RegPacket->UsbPortInvalidateController = USBPORT_InvalidateController;
    RegPacket->UsbPortBugCheck = USBPORT_BugCheck;
    RegPacket->UsbPortNotifyDoubleBuffer = USBPORT_NotifyDoubleBuffer;

    RtlCopyMemory(&MiniPortInterface->Packet,
                  RegPacket,
                  sizeof(USBPORT_REGISTRATION_PACKET));

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    return STATUS_SUCCESS;
}
