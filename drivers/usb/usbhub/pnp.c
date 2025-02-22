/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBHub plug and play functions
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbhub.h"

#define NDEBUG
#include <debug.h>

#define NDEBUG_USBHUB_PNP
#define NDEBUG_USBHUB_ENUM
#include "dbg_uhub.h"

NTSTATUS
NTAPI
USBH_IrpCompletion(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp,
                   IN PVOID Context)
{
    PRKEVENT Event;

    DPRINT("USBH_IrpCompletion: Irp - %p\n", Irp);

    Event = Context;
    KeSetEvent(Event, EVENT_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
USBH_HubPnPIrpComplete(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp,
                       IN PVOID Context)
{
    PUSBHUB_FDO_EXTENSION HubExtension;

    DPRINT("USBH_HubPnPIrpComplete: Irp - %p\n", Irp);

     HubExtension = Context;

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        DPRINT1("USBH_HubPnPIrpComplete: Irp failed - %lX\n", Irp->IoStatus.Status);
        HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_FAILED;
    }

    Irp->IoStatus.Status = STATUS_MORE_PROCESSING_REQUIRED;

    KeSetEvent(&HubExtension->LowerDeviceEvent, EVENT_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
USBH_QueryCapsComplete(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp,
                       IN PVOID Context)
{
    PIO_STACK_LOCATION IoStack;
    PDEVICE_CAPABILITIES Capabilities;

    DPRINT("USBH_QueryCapsComplete: ... \n");

    ASSERT(NT_SUCCESS(Irp->IoStatus.Status));

    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    IoStack= IoGetCurrentIrpStackLocation(Irp);
    Capabilities = IoStack->Parameters.DeviceCapabilities.Capabilities;

    Capabilities->SurpriseRemovalOK = 1;

    return STATUS_CONTINUE_COMPLETION;
}

NTSTATUS
NTAPI
USBHUB_GetBusInterface(IN PDEVICE_OBJECT DeviceObject,
                       OUT PUSB_BUS_INTERFACE_HUB_V5 BusInterface)
{
    PIRP Irp;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    KEVENT Event;

    DPRINT("USBHUB_GetBusInterface: ... \n");

    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    if (!Irp)
    {
        DPRINT1("USBHUB_GetBusInterface: IoAllocateIrp() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(Irp,
                           USBH_IrpCompletion,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    IoStack = IoGetNextIrpStackLocation(Irp);

    IoStack->MajorFunction = IRP_MJ_PNP;
    IoStack->MinorFunction = IRP_MN_QUERY_INTERFACE;

    IoStack->Parameters.QueryInterface.InterfaceType = &USB_BUS_INTERFACE_HUB_GUID;
    IoStack->Parameters.QueryInterface.Size = sizeof(USB_BUS_INTERFACE_HUB_V5);
    IoStack->Parameters.QueryInterface.Version = USB_BUSIF_HUB_VERSION_5;
    IoStack->Parameters.QueryInterface.Interface = (PINTERFACE)BusInterface;
    IoStack->Parameters.QueryInterface.InterfaceSpecificData = DeviceObject;

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);

        Status = Irp->IoStatus.Status;
    }

    IoFreeIrp(Irp);

    return Status;
}

NTSTATUS
NTAPI
USBHUB_GetBusInterfaceUSBDI(IN PDEVICE_OBJECT DeviceObject,
                            OUT PUSB_BUS_INTERFACE_USBDI_V2 BusInterfaceUSBDI)
{
    PIRP Irp;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    KEVENT Event;

    DPRINT("USBHUB_GetBusInterfaceUSBDI: ... \n");

    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    if (!Irp)
    {
        DPRINT1("USBHUB_GetBusInterfaceUSBDI: IoAllocateIrp() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(Irp,
                           USBH_IrpCompletion,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    IoStack = IoGetNextIrpStackLocation(Irp);

    IoStack->MajorFunction = IRP_MJ_PNP;
    IoStack->MinorFunction = IRP_MN_QUERY_INTERFACE;

    IoStack->Parameters.QueryInterface.InterfaceType = &USB_BUS_INTERFACE_USBDI_GUID;
    IoStack->Parameters.QueryInterface.Size = sizeof(USB_BUS_INTERFACE_USBDI_V2);
    IoStack->Parameters.QueryInterface.Version = USB_BUSIF_USBDI_VERSION_2;
    IoStack->Parameters.QueryInterface.Interface = (PINTERFACE)BusInterfaceUSBDI;
    IoStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);

        Status = Irp->IoStatus.Status;
    }

    IoFreeIrp(Irp);

    return Status;
}

VOID
NTAPI
USBH_QueryCapabilities(IN PDEVICE_OBJECT DeviceObject,
                       IN PDEVICE_CAPABILITIES DeviceCapabilities)
{
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    KEVENT Event;

    DPRINT("USBH_QueryCapabilities: ... \n");

    RtlZeroMemory(DeviceCapabilities, sizeof(DEVICE_CAPABILITIES));

    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    if (!Irp)
    {
        DPRINT1("USBH_QueryCapabilities: IoAllocateIrp() failed\n");
        return;
    }

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(Irp,
                           USBH_IrpCompletion,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    IoStack = IoGetNextIrpStackLocation(Irp);

    IoStack->MajorFunction = IRP_MJ_PNP;
    IoStack->MinorFunction = IRP_MN_QUERY_CAPABILITIES;

    IoStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;
    IoStack->Parameters.DeviceCapabilities.Capabilities->Size = sizeof(DEVICE_CAPABILITIES);
    IoStack->Parameters.DeviceCapabilities.Capabilities->Version = 1;
    IoStack->Parameters.DeviceCapabilities.Capabilities->Address = MAXULONG;
    IoStack->Parameters.DeviceCapabilities.Capabilities->UINumber = MAXULONG;

    if (IoCallDriver(DeviceObject, Irp) == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    IoFreeIrp(Irp);
}

NTSTATUS
NTAPI
USBH_OpenConfiguration(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PUSB_INTERFACE_DESCRIPTOR Pid;
    PURB Urb;
    NTSTATUS Status;
    USBD_INTERFACE_LIST_ENTRY InterfaceList[2] = {{NULL, NULL}, {NULL, NULL}};

    DPRINT("USBH_OpenConfiguration ... \n");

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_USB20_HUB &&
        HubExtension->LowerPDO != HubExtension->RootHubPdo)
    {
        Pid = USBD_ParseConfigurationDescriptorEx(HubExtension->HubConfigDescriptor,
                                                  HubExtension->HubConfigDescriptor,
                                                  -1,
                                                  -1,
                                                  USB_DEVICE_CLASS_HUB,
                                                  -1,
                                                  2);

        if (Pid)
        {
            HubExtension->HubFlags |= USBHUB_FDO_FLAG_MULTIPLE_TTS;
        }
        else
        {
            Pid = USBD_ParseConfigurationDescriptorEx(HubExtension->HubConfigDescriptor,
                                                      HubExtension->HubConfigDescriptor,
                                                      -1,
                                                      -1,
                                                      USB_DEVICE_CLASS_HUB,
                                                      -1,
                                                      1);

            if (Pid)
            {
                goto Next;
            }

            Pid = USBD_ParseConfigurationDescriptorEx(HubExtension->HubConfigDescriptor,
                                                      HubExtension->HubConfigDescriptor,
                                                      -1,
                                                      -1,
                                                      USB_DEVICE_CLASS_HUB,
                                                      -1,
                                                      0);
        }
    }
    else
    {
        Pid = USBD_ParseConfigurationDescriptorEx(HubExtension->HubConfigDescriptor,
                                                  HubExtension->HubConfigDescriptor,
                                                  -1,
                                                  -1,
                                                  USB_DEVICE_CLASS_HUB,
                                                  -1,
                                                  -1);
    }

    if (!Pid)
    {
        return STATUS_UNSUCCESSFUL;
    }

  Next:

    if (Pid->bInterfaceClass != USB_DEVICE_CLASS_HUB)
    {
        return STATUS_UNSUCCESSFUL;
    }

    InterfaceList[0].InterfaceDescriptor = Pid;

    Urb = USBD_CreateConfigurationRequestEx(HubExtension->HubConfigDescriptor,
                                            InterfaceList);

    if (!Urb)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = USBH_FdoSyncSubmitUrb(HubExtension->Common.SelfDevice, Urb);

    if (NT_SUCCESS(Status))
    {
        RtlCopyMemory(&HubExtension->PipeInfo,
                      InterfaceList[0].Interface->Pipes,
                      sizeof(USBD_PIPE_INFORMATION));

        HubExtension->ConfigHandle = Urb->UrbSelectConfiguration.ConfigurationHandle;
    }

    ExFreePool(Urb);

    return Status;
}

NTSTATUS
NTAPI
USBD_Initialize20Hub(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PUSB_BUSIFFN_INITIALIZE_20HUB Initialize20Hub;
    ULONG TtCount;
    PUSB_DEVICE_HANDLE DeviceHandle;

    DPRINT("USBD_InitUsb2Hub ... \n");

    Initialize20Hub = HubExtension->BusInterface.Initialize20Hub;

    if (!Initialize20Hub)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    TtCount = 1;

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_MULTIPLE_TTS)
    {
        TtCount = HubExtension->HubDescriptor->bNumberOfPorts;
    }

    DeviceHandle = USBH_SyncGetDeviceHandle(HubExtension->LowerDevice);

    return Initialize20Hub(HubExtension->BusInterface.BusContext,
                           DeviceHandle,
                           TtCount);
}

NTSTATUS
NTAPI
USBH_AbortInterruptPipe(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    struct _URB_PIPE_REQUEST * Urb;
    NTSTATUS Status;

    DPRINT("USBH_AbortInterruptPipe: HubExtension - %p\n", HubExtension);

    Urb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(struct _URB_PIPE_REQUEST),
                                USB_HUB_TAG);

    if (!Urb)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Urb, sizeof(struct _URB_PIPE_REQUEST));

    Urb->Hdr.Length = sizeof(struct _URB_PIPE_REQUEST);
    Urb->Hdr.Function = URB_FUNCTION_ABORT_PIPE;
    Urb->PipeHandle = HubExtension->PipeInfo.PipeHandle;

    Status = USBH_FdoSyncSubmitUrb(HubExtension->Common.SelfDevice,
                                   (PURB)Urb);

    if (NT_SUCCESS(Status))
    {
        KeWaitForSingleObject(&HubExtension->StatusChangeEvent,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    ExFreePoolWithTag(Urb, USB_HUB_TAG);

    return Status;
}

VOID
NTAPI
USBH_FdoCleanup(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PIRP IdleIrp = NULL;
    PIRP WakeIrp = NULL;
    PUSBHUB_PORT_DATA PortData;
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    PIRP PortIdleIrp = NULL;
    PIRP PortWakeIrp = NULL;
    PVOID DeviceHandle;
    NTSTATUS Status;
    USHORT Port;
    UCHAR NumberPorts;
    KIRQL Irql;

    DPRINT("USBH_FdoCleanup: HubExtension - %p\n", HubExtension);

    USBD_UnRegisterRootHubCallBack(HubExtension);

    HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_STOPPING;

    if (HubExtension->ResetRequestCount)
    {
        IoCancelIrp(HubExtension->ResetPortIrp);

        KeWaitForSingleObject(&HubExtension->IdleEvent,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    IoFreeIrp(HubExtension->ResetPortIrp);

    HubExtension->ResetPortIrp = NULL;

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_WAIT_IDLE_REQUEST)
    {
        KeWaitForSingleObject(&HubExtension->IdleEvent,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    IoAcquireCancelSpinLock(&Irql);

    if (HubExtension->PendingWakeIrp)
    {
        WakeIrp = HubExtension->PendingWakeIrp;
        HubExtension->PendingWakeIrp = NULL;
    }

    if (HubExtension->PendingIdleIrp)
    {
        IdleIrp = HubExtension->PendingIdleIrp;
        HubExtension->PendingIdleIrp = NULL;
    }

    IoReleaseCancelSpinLock(Irql);

    if (WakeIrp)
    {
        USBH_HubCancelWakeIrp(HubExtension, WakeIrp);
    }

    USBH_HubCompletePortWakeIrps(HubExtension, STATUS_DELETE_PENDING);

    if (IdleIrp)
    {
        USBH_HubCancelIdleIrp(HubExtension, IdleIrp);
    }

    if (InterlockedDecrement(&HubExtension->PendingRequestCount) > 0)
    {
        KeWaitForSingleObject(&HubExtension->PendingRequestEvent,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    if (HubExtension->SCEIrp)
    {
        Status = USBH_AbortInterruptPipe(HubExtension);

        if (!NT_SUCCESS(Status) && IoCancelIrp(HubExtension->SCEIrp))
        {
            KeWaitForSingleObject(&HubExtension->StatusChangeEvent,
                                  Suspended,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }

        IoFreeIrp(HubExtension->SCEIrp);

        HubExtension->SCEIrp = NULL;
    }

    if (!HubExtension->PortData ||
        !HubExtension->HubDescriptor)
    {
        goto Exit;
    }

    PortData = HubExtension->PortData;
    NumberPorts = HubExtension->HubDescriptor->bNumberOfPorts;

    for (Port = 0; Port < NumberPorts; Port++)
    {
        if (PortData[Port].DeviceObject)
        {
            PortExtension = PortData[Port].DeviceObject->DeviceExtension;

            IoAcquireCancelSpinLock(&Irql);

            PortIdleIrp = PortExtension->IdleNotificationIrp;

            if (PortIdleIrp)
            {
                PortExtension->PortPdoFlags &= ~USBHUB_PDO_FLAG_IDLE_NOTIFICATION;
                PortExtension->IdleNotificationIrp = NULL;

                if (PortIdleIrp->Cancel)
                {
                    PortIdleIrp = NULL;
                }

                if (PortIdleIrp)
                {
                    IoSetCancelRoutine(PortIdleIrp, NULL);
                }
            }

            PortWakeIrp = PortExtension->PdoWaitWakeIrp;

            if (PortWakeIrp)
            {
                PortExtension->PortPdoFlags &= ~USBHUB_PDO_FLAG_WAIT_WAKE;
                PortExtension->PdoWaitWakeIrp = NULL;

                if (PortWakeIrp->Cancel || !IoSetCancelRoutine(PortWakeIrp, NULL))
                {
                    PortWakeIrp = NULL;

                    if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
                    {
                        KeSetEvent(&HubExtension->PendingRequestEvent,
                                   EVENT_INCREMENT,
                                   FALSE);
                    }
                }
            }

            IoReleaseCancelSpinLock(Irql);

            if (PortIdleIrp)
            {
                PortIdleIrp->IoStatus.Status = STATUS_CANCELLED;
                IoCompleteRequest(PortIdleIrp, IO_NO_INCREMENT);
            }

            if (PortWakeIrp)
            {
                USBH_CompletePowerIrp(HubExtension,
                                      PortWakeIrp,
                                      STATUS_CANCELLED);
            }

            if (!(PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_POWER_D3))
            {
                DeviceHandle = InterlockedExchangePointer(&PortExtension->DeviceHandle,
                                                          NULL);

                if (DeviceHandle)
                {
                    USBD_RemoveDeviceEx(HubExtension, DeviceHandle, 0);
                }

                PortExtension->PortPdoFlags |= USBHUB_PDO_FLAG_POWER_D3;
            }
        }

        USBH_SyncDisablePort(HubExtension, Port + 1);
    }

Exit:

    if (HubExtension->SCEBitmap)
    {
        ExFreePoolWithTag(HubExtension->SCEBitmap, USB_HUB_TAG);
    }

    if (HubExtension->HubDescriptor)
    {
        ExFreePoolWithTag(HubExtension->HubDescriptor, USB_HUB_TAG);
    }

    if (HubExtension->HubConfigDescriptor)
    {
        ExFreePoolWithTag(HubExtension->HubConfigDescriptor, USB_HUB_TAG);
    }

    HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_DEVICE_STARTED;

    HubExtension->HubDescriptor = NULL;
    HubExtension->HubConfigDescriptor = NULL;

    HubExtension->SCEIrp = NULL;
    HubExtension->SCEBitmap = NULL;
}

NTSTATUS
NTAPI
USBH_StartHubFdoDevice(IN PUSBHUB_FDO_EXTENSION HubExtension,
                       IN PIRP Irp)
{
    NTSTATUS Status;
    ULONG DisableRemoteWakeup = 0;
    ULONG HubCount = 0;
    PUSB_DEVICE_HANDLE DeviceHandle;
    USB_DEVICE_TYPE DeviceType;
    DEVICE_CAPABILITIES  DeviceCapabilities;
    BOOLEAN IsBusPowered;
    static WCHAR DisableWakeValueName[] = L"DisableRemoteWakeup";

    DPRINT("USBH_StartHubFdoDevice: ... \n");

    KeInitializeEvent(&HubExtension->IdleEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&HubExtension->ResetEvent, NotificationEvent, TRUE);
    KeInitializeEvent(&HubExtension->PendingRequestEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&HubExtension->LowerDeviceEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&HubExtension->StatusChangeEvent, NotificationEvent, TRUE);
    KeInitializeEvent(&HubExtension->RootHubNotificationEvent,
                      NotificationEvent,
                      TRUE);

    KeInitializeSpinLock(&HubExtension->RelationsWorkerSpinLock);
    KeInitializeSpinLock(&HubExtension->CheckIdleSpinLock);

    KeInitializeSemaphore(&HubExtension->ResetDeviceSemaphore, 1, 1);
    KeInitializeSemaphore(&HubExtension->HubPortSemaphore, 1, 1);
    KeInitializeSemaphore(&HubExtension->HubSemaphore, 1, 1);

    HubExtension->HubFlags = 0;
    HubExtension->HubConfigDescriptor = NULL;
    HubExtension->HubDescriptor = NULL;
    HubExtension->SCEIrp = NULL;
    HubExtension->SCEBitmap = NULL;
    HubExtension->SystemPowerState.SystemState = PowerSystemWorking;
    HubExtension->PendingRequestCount = 1;
    HubExtension->ResetRequestCount = 0;
    HubExtension->PendingIdleIrp = NULL;
    HubExtension->PendingWakeIrp = NULL;

    InitializeListHead(&HubExtension->PdoList);

    HubExtension->HubFlags |= USBHUB_FDO_FLAG_WITEM_INIT;
    InitializeListHead(&HubExtension->WorkItemList);
    KeInitializeSpinLock(&HubExtension->WorkItemSpinLock);

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp,
                           USBH_HubPnPIrpComplete,
                           HubExtension,
                           TRUE,
                           TRUE,
                           TRUE);

    if (IoCallDriver(HubExtension->LowerDevice, Irp) == STATUS_PENDING)
    {
        KeWaitForSingleObject(&HubExtension->LowerDeviceEvent,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    HubExtension->RootHubPdo = NULL;

    Status = USBH_SyncGetRootHubPdo(HubExtension->LowerDevice,
                                    &HubExtension->RootHubPdo,
                                    &HubExtension->RootHubPdo2);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_SyncGetRootHubPdo() failed - %lX\n", Status);
        goto ErrorExit;
    }

    USBH_WriteFailReasonID(HubExtension->LowerPDO, USBHUB_FAIL_NO_FAIL);

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_FAILED)
    {
        DPRINT1("USBH_StartHubFdoDevice: USBHUB_FDO_FLAG_DEVICE_FAILED - TRUE\n");
        Status = STATUS_UNSUCCESSFUL;
        goto ErrorExit;
    }

    HubExtension->HubFlags |= USBHUB_FDO_FLAG_REMOTE_WAKEUP;

    Status = USBD_GetPdoRegistryParameter(HubExtension->LowerPDO,
                                          &DisableRemoteWakeup,
                                          sizeof(DisableRemoteWakeup),
                                          DisableWakeValueName,
                                          sizeof(DisableWakeValueName));

    if (NT_SUCCESS(Status) && DisableRemoteWakeup)
    {
        DPRINT("USBH_StartHubFdoDevice: DisableRemoteWakeup - TRUE\n");
        HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_REMOTE_WAKEUP;
    }

    HubExtension->CurrentPowerState.DeviceState = PowerDeviceD0;

    USBH_SyncGetHubCount(HubExtension->LowerDevice,
                         &HubCount);

    Status = USBHUB_GetBusInterface(HubExtension->RootHubPdo,
                                    &HubExtension->BusInterface);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_StartHubFdoDevice: USBHUB_GetBusInterface() failed - %lX\n",
                Status);
        goto ErrorExit;
    }

    Status = USBHUB_GetBusInterfaceUSBDI(HubExtension->LowerDevice,
                                         &HubExtension->BusInterfaceUSBDI);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_StartHubFdoDevice: USBHUB_GetBusInterfaceUSBDI() failed - %lX\n",
                Status);
        goto ErrorExit;
    }

    DeviceHandle = USBH_SyncGetDeviceHandle(HubExtension->LowerDevice);

    if (DeviceHandle)
    {
        Status = USBH_GetDeviceType(HubExtension, DeviceHandle, &DeviceType);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("USBH_StartHubFdoDevice: USBH_GetDeviceType() failed - %lX\n",
                    Status);

            goto ErrorExit;
        }

        if (DeviceType == Usb20Device)
        {
            HubExtension->HubFlags |= USBHUB_FDO_FLAG_USB20_HUB;
        }
    }

    if (HubCount > USBHUB_MAX_CASCADE_LEVELS)
    {
        PUSBHUB_PORT_PDO_EXTENSION ParentPdoExtension;
        PUSBHUB_FDO_EXTENSION ParentHubExtension;
        USHORT ParentPort;
        PUSBHUB_PORT_DATA PortData;

        DPRINT1("USBH_StartHubFdoDevice: HubCount > 6 - %x\n", HubCount);

        USBH_WriteFailReasonID(HubExtension->LowerPDO,
                               USBHUB_FAIL_NESTED_TOO_DEEPLY);

        ParentPdoExtension = HubExtension->LowerPDO->DeviceExtension;
        ParentHubExtension = ParentPdoExtension->HubExtension;

        ParentPort = ParentPdoExtension->PortNumber - 1;
        PortData = &ParentHubExtension->PortData[ParentPort];
        PortData->ConnectionStatus = DeviceHubNestedTooDeeply;

        HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_FAILED;
    }

    USBH_QueryCapabilities(HubExtension->LowerDevice, &DeviceCapabilities);

    HubExtension->SystemWake = DeviceCapabilities.SystemWake;
    HubExtension->DeviceWake = DeviceCapabilities.DeviceWake;

    RtlCopyMemory(HubExtension->DeviceState,
                  &DeviceCapabilities.DeviceState,
                  POWER_SYSTEM_MAXIMUM * sizeof(DEVICE_POWER_STATE));

    Status = USBH_GetDeviceDescriptor(HubExtension->Common.SelfDevice,
                                      &HubExtension->HubDeviceDescriptor);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_StartHubFdoDevice: USBH_GetDeviceDescriptor() failed - %lX\n",
                Status);
        goto ErrorExit;
    }

    Status = USBH_GetConfigurationDescriptor(HubExtension->Common.SelfDevice,
                                             &HubExtension->HubConfigDescriptor);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_StartHubFdoDevice: USBH_GetConfigurationDescriptor() failed - %lX\n",
                Status);
        goto ErrorExit;
    }

    Status = USBH_SyncGetHubDescriptor(HubExtension);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_StartHubFdoDevice: USBH_SyncGetHubDescriptor() failed - %lX\n",
                Status);
        goto ErrorExit;
    }

    IsBusPowered = USBH_HubIsBusPowered(HubExtension->Common.SelfDevice,
                                        HubExtension->HubConfigDescriptor);

    if (IsBusPowered)
    {
        /* bus-powered hub is allowed a maximum of 100 mA only for each port */
        HubExtension->MaxPowerPerPort = 100;

        /* can have 4 ports (4 * 100 mA) and 100 mA remains for itself;
           expressed in 2 mA units (i.e., 250 = 500 mA). */
        HubExtension->HubConfigDescriptor->MaxPower = 250;
    }
    else
    {
        /* self-powered hub is allowed a maximum of 500 mA for each port */
        HubExtension->MaxPowerPerPort = 500;
    }

    Status = USBH_OpenConfiguration(HubExtension);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_StartHubFdoDevice: USBH_OpenConfiguration() failed - %lX\n",
                Status);
        goto ErrorExit;
    }

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_USB20_HUB)
    {
        Status = USBD_Initialize20Hub(HubExtension);
    }

    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit;
    }

    HubExtension->SCEIrp = IoAllocateIrp(HubExtension->Common.SelfDevice->StackSize,
                                         FALSE);

    HubExtension->ResetPortIrp = IoAllocateIrp(HubExtension->Common.SelfDevice->StackSize,
                                               FALSE);

    if (!HubExtension->SCEIrp || !HubExtension->ResetPortIrp)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    HubExtension->SCEBitmapLength = HubExtension->PipeInfo.MaximumPacketSize;

    HubExtension->SCEBitmap = ExAllocatePoolWithTag(NonPagedPool,
                                                    HubExtension->SCEBitmapLength,
                                                    USB_HUB_TAG);

    if (!HubExtension->SCEBitmap)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    RtlZeroMemory(HubExtension->SCEBitmap, HubExtension->SCEBitmapLength);

    Status = USBH_SyncPowerOnPorts(HubExtension);

    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit;
    }
    else
    {
        USHORT Port;

        HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_STARTED;

        for (Port = 1;
             Port <= HubExtension->HubDescriptor->bNumberOfPorts;
             Port++)
        {
            USBH_SyncClearPortStatus(HubExtension,
                                     Port,
                                     USBHUB_FEATURE_C_PORT_CONNECTION);
        }
    }

    if (HubExtension->LowerPDO == HubExtension->RootHubPdo)
    {
        USBD_RegisterRootHubCallBack(HubExtension);
    }
    else
    {
        HubExtension->HubFlags |= USBHUB_FDO_FLAG_DO_ENUMERATION;
        USBH_SubmitStatusChangeTransfer(HubExtension);
    }

    goto Exit;

  ErrorExit:

    if (HubExtension->HubDescriptor)
    {
        ExFreePoolWithTag(HubExtension->HubDescriptor, USB_HUB_TAG);
        HubExtension->HubDescriptor = NULL;
    }

    if (HubExtension->SCEIrp)
    {
        IoFreeIrp(HubExtension->SCEIrp);
        HubExtension->SCEIrp = NULL;
    }

    if (HubExtension->ResetPortIrp)
    {
        IoFreeIrp(HubExtension->ResetPortIrp);
        HubExtension->ResetPortIrp = NULL;
    }

    if (HubExtension->SCEBitmap)
    {
        ExFreePoolWithTag(HubExtension->SCEBitmap, USB_HUB_TAG);
        HubExtension->SCEBitmap = NULL;
    }

    if (HubExtension->HubConfigDescriptor)
    {
        ExFreePoolWithTag(HubExtension->HubConfigDescriptor, USB_HUB_TAG);
        HubExtension->HubConfigDescriptor = NULL;
    }

  Exit:

    USBH_CompleteIrp(Irp, Status);

    return Status;
}

NTSTATUS
NTAPI
USBH_FdoStartDevice(IN PUSBHUB_FDO_EXTENSION HubExtension,
                    IN PIRP Irp)
{
    NTSTATUS Status;

    DPRINT("USBH_FdoStartDevice: HubExtension - %p\n", HubExtension);

    HubExtension->RootHubPdo = NULL;

    Status = USBH_SyncGetRootHubPdo(HubExtension->LowerDevice,
                                    &HubExtension->RootHubPdo,
                                    &HubExtension->RootHubPdo2);

    if (NT_SUCCESS(Status))
    {
        if (HubExtension->RootHubPdo)
        {
            Status = USBH_StartHubFdoDevice(HubExtension, Irp);
        }
        else
        {
            DPRINT1("USBH_FdoStartDevice: FIXME. start ParentDevice\n");
            DbgBreakPoint();
        }
    }
    else
    {
        DPRINT1("USBH_FdoStartDevice: FIXME. USBH_SyncGetRootHubPdo return - %lX\n",
                Status);

        DbgBreakPoint();
        USBH_CompleteIrp(Irp, Status);
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_FdoQueryBusRelations(IN PUSBHUB_FDO_EXTENSION HubExtension,
                          IN PIRP Irp)
{
    PDEVICE_RELATIONS DeviceRelations = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    LIST_ENTRY GhostPdoList;
    KIRQL OldIrql;
    PLIST_ENTRY PdoList;
    UCHAR NumberPorts;
    USHORT Port;
    USHORT GhostPort;
    PUSBHUB_PORT_DATA PortData;
    PDEVICE_OBJECT PdoDevice;
    PUSBHUB_PORT_PDO_EXTENSION PdoExtension;
    PUSBHUB_PORT_PDO_EXTENSION pdoExtension;
    NTSTATUS NtStatus;
    PVOID SerialNumber;
    PVOID DeviceHandle;
    USB_PORT_STATUS UsbPortStatus;
    PLIST_ENTRY Entry;
    ULONG Length;

    DPRINT_ENUM("USBH_FdoQueryBusRelations: HubFlags - %lX\n",
                HubExtension->HubFlags);

    if (!(HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STARTED))
    {
       Status = STATUS_INVALID_DEVICE_STATE;
       goto RelationsWorker;
    }

    if (!HubExtension->HubDescriptor)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto RelationsWorker;
    }

    if (!(HubExtension->HubFlags & USBHUB_FDO_FLAG_DO_ENUMERATION))
    {
        // FIXME: this delay makes devices discovery during early boot more reliable
        LARGE_INTEGER Interval;
        Status = STATUS_SUCCESS;
        IoInvalidateDeviceRelations(HubExtension->LowerPDO, BusRelations);
        Interval.QuadPart = -10000LL * 1000; // 1 sec.
        KeDelayExecutionThread(KernelMode, FALSE, &Interval);

        DPRINT_ENUM("USBH_FdoQueryBusRelations: Skip enumeration\n");
        goto RelationsWorker;
    }

    InterlockedIncrement(&HubExtension->PendingRequestCount);

    KeWaitForSingleObject(&HubExtension->ResetDeviceSemaphore,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    NumberPorts = HubExtension->HubDescriptor->bNumberOfPorts;
    DPRINT_ENUM("USBH_FdoQueryBusRelations: NumberPorts - %x\n", NumberPorts);

    Length = FIELD_OFFSET(DEVICE_RELATIONS, Objects) +
             NumberPorts * sizeof(PDEVICE_OBJECT);

    if (Irp->IoStatus.Information)
    {
        DPRINT1("FIXME: leaking old bus relations\n");
    }

    DeviceRelations = ExAllocatePoolWithTag(NonPagedPool, Length, USB_HUB_TAG);

    if (!DeviceRelations)
    {
        HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_NOT_ENUMERATED;

        KeReleaseSemaphore(&HubExtension->ResetDeviceSemaphore,
                           LOW_REALTIME_PRIORITY,
                           1,
                           FALSE);

        if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
        {
            KeSetEvent(&HubExtension->PendingRequestEvent, EVENT_INCREMENT, FALSE);
        }

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto RelationsWorker;
    }

    RtlZeroMemory(DeviceRelations, Length);

    DeviceRelations->Count = 0;

EnumStart:

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_ESD_RECOVERING)
    {
        HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_NOT_ENUMERATED;

        KeReleaseSemaphore(&HubExtension->ResetDeviceSemaphore,
                           LOW_REALTIME_PRIORITY,
                           1,
                           FALSE);

        if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
        {
            KeSetEvent(&HubExtension->PendingRequestEvent, EVENT_INCREMENT, FALSE);
        }

        Status = STATUS_SUCCESS;
        goto RelationsWorker;
    }

    HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_ENUM_POST_RECOVER;

    for (Port = 1; Port <= NumberPorts; Port++)
    {
        PortData = &HubExtension->PortData[Port - 1];

        if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_FAILED)
        {
            continue;
        }

        Status = USBH_SyncGetPortStatus(HubExtension,
                                        Port,
                                        &PortData->PortStatus,
                                        sizeof(USB_PORT_STATUS_AND_CHANGE));

        if (!NT_SUCCESS(Status))
        {
            DPRINT_ENUM("USBH_FdoQueryBusRelations: Status - %X\n", Status);
            HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_FAILED;
            DeviceRelations->Count = 0;
            goto EnumStart;
        }

        DPRINT_ENUM("USBH_FdoQueryBusRelations: Port - %x, ConnectStatus - %x\n",
                    Port,
                    PortData->PortStatus.PortStatus.Usb20PortStatus.CurrentConnectStatus);

        PdoDevice = PortData->DeviceObject;

        if (PortData->DeviceObject)
        {
            PdoExtension = PdoDevice->DeviceExtension;

            if (PdoExtension->PortPdoFlags & USBHUB_PDO_FLAG_OVERCURRENT_PORT)
            {
                PortData->PortStatus.PortStatus.Usb20PortStatus.CurrentConnectStatus = 1;
            }
        }

        if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_FAILED)
        {
            DPRINT1("USBH_FdoQueryBusRelations: DbgBreakPoint() \n");
            DbgBreakPoint();
        }

        if (!PortData->PortStatus.PortStatus.Usb20PortStatus.CurrentConnectStatus)
        {
            if (PdoDevice)
            {
                PdoExtension = PdoDevice->DeviceExtension;

                PdoExtension->PortPdoFlags |= USBHUB_PDO_FLAG_DELETE_PENDING;
                PdoExtension->EnumFlags &= ~USBHUB_ENUM_FLAG_DEVICE_PRESENT;

                SerialNumber = InterlockedExchangePointer((PVOID)&PdoExtension->SerialNumber,
                                                          NULL);

                if (SerialNumber)
                {
                    ExFreePoolWithTag(SerialNumber, USB_HUB_TAG);
                }

                DeviceHandle = InterlockedExchangePointer(&PdoExtension->DeviceHandle,
                                                          NULL);

                if (DeviceHandle)
                {
                    USBD_RemoveDeviceEx(HubExtension, DeviceHandle, 0);
                    USBH_SyncDisablePort(HubExtension, Port);
                }
            }

            PortData->DeviceObject = NULL;
            PortData->ConnectionStatus = NoDeviceConnected;
            continue;
        }

        if (PdoDevice)
        {
            ObReferenceObject(PdoDevice);

            PdoDevice->Flags |= DO_POWER_PAGABLE;
            PdoDevice->Flags &= ~DO_DEVICE_INITIALIZING;

            DeviceRelations->Objects[DeviceRelations->Count++] = PdoDevice;

            PdoExtension = PdoDevice->DeviceExtension;
            PdoExtension->PortPdoFlags &= ~USBHUB_PDO_FLAG_POWER_D1_OR_D2;

            continue;
        }

        USBH_Wait(100);

        NtStatus = USBH_SyncResetPort(HubExtension, Port);

        if (!NT_SUCCESS(NtStatus))
        {
            if (HubExtension->HubFlags & USBHUB_FDO_FLAG_USB20_HUB)
            {
                PortData->DeviceObject = NULL;
                PortData->ConnectionStatus = NoDeviceConnected;
                continue;
            }
        }
        else
        {
            NtStatus = USBH_SyncGetPortStatus(HubExtension,
                                              Port,
                                              &PortData->PortStatus,
                                              sizeof(USB_PORT_STATUS_AND_CHANGE));

            UsbPortStatus = PortData->PortStatus.PortStatus;

            if (NT_SUCCESS(NtStatus))
            {
                ULONG ix = 0;

                for (NtStatus = USBH_CreateDevice(HubExtension, Port, UsbPortStatus, ix);
                     !NT_SUCCESS(NtStatus);
                     NtStatus = USBH_CreateDevice(HubExtension, Port, UsbPortStatus, ix))
                {
                    USBH_Wait(500);

                    if (ix >= 2)
                    {
                        break;
                    }

                    if (PortData->DeviceObject)
                    {
                        IoDeleteDevice(PortData->DeviceObject);
                        PortData->DeviceObject = NULL;
                        PortData->ConnectionStatus = NoDeviceConnected;
                    }

                    USBH_SyncResetPort(HubExtension, Port);

                    ix++;
                }

                if (NT_SUCCESS(NtStatus))
                {
                    PdoExtension = PortData->DeviceObject->DeviceExtension;

                    if (!(PdoExtension->PortPdoFlags & USBHUB_PDO_FLAG_PORT_LOW_SPEED) &&
                        !(PdoExtension->PortPdoFlags & USBHUB_PDO_FLAG_PORT_HIGH_SPEED) &&
                        !(HubExtension->HubFlags & USBHUB_FDO_FLAG_USB20_HUB))
                    {
                        DPRINT1("USBH_FdoQueryBusRelations: FIXME USBH_DeviceIs2xDualMode()\n");

                        if (0)//USBH_DeviceIs2xDualMode(PdoExtension))
                        {
                            PdoExtension->PortPdoFlags |= USBHUB_PDO_FLAG_HS_USB1_DUALMODE;
                        }
                    }

                    ObReferenceObject(PortData->DeviceObject);

                    DeviceRelations->Objects[DeviceRelations->Count] = PortData->DeviceObject;

                    PortData->DeviceObject->Flags |= DO_POWER_PAGABLE;
                    PortData->DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

                    DeviceRelations->Count++;

                    PortData->ConnectionStatus = DeviceConnected;

                    continue;
                }
            }
        }

        PortData->ConnectionStatus = DeviceFailedEnumeration;

        if (NT_ERROR(USBH_SyncDisablePort(HubExtension, Port)))
        {
            HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_FAILED;
        }

        if (PortData->DeviceObject)
        {
            ObReferenceObject(PortData->DeviceObject);
            PortData->DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
            DeviceRelations->Objects[DeviceRelations->Count++] = PortData->DeviceObject;
        }
    }

    HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_NOT_ENUMERATED;

    KeReleaseSemaphore(&HubExtension->ResetDeviceSemaphore,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
    {
        KeSetEvent(&HubExtension->PendingRequestEvent, EVENT_INCREMENT, FALSE);
    }

RelationsWorker:

    Irp->IoStatus.Status = Status;

    if (!NT_SUCCESS(Status))
    {
        //Irp->IoStatus.Information = 0;

        if (DeviceRelations)
        {
            ExFreePoolWithTag(DeviceRelations, USB_HUB_TAG);
        }

        USBH_CompleteIrp(Irp, Status);

        return Status;
    }

    KeAcquireSpinLock(&HubExtension->RelationsWorkerSpinLock, &OldIrql);

    if (DeviceRelations && DeviceRelations->Count)
    {
        for (Port = 0; Port < DeviceRelations->Count; Port++)
        {
            PdoDevice = DeviceRelations->Objects[Port];
            Entry = HubExtension->PdoList.Flink;

            while (Entry != &HubExtension->PdoList)
            {
                pdoExtension = CONTAINING_RECORD(Entry,
                                                 USBHUB_PORT_PDO_EXTENSION,
                                                 PortLink);

                if (pdoExtension == PdoDevice->DeviceExtension)
                {
                    PdoExt(PdoDevice)->EnumFlags |= USBHUB_ENUM_FLAG_GHOST_DEVICE;
                    goto PortNext;
                }

                Entry = Entry->Flink;
            }

            PdoExt(PdoDevice)->EnumFlags |= USBHUB_ENUM_FLAG_DEVICE_PRESENT;

    PortNext:;
        }

        for (Port = 0; Port < DeviceRelations->Count; Port++)
        {
            PdoDevice = DeviceRelations->Objects[Port];

            if (PdoExt(PdoDevice)->EnumFlags & USBHUB_ENUM_FLAG_GHOST_DEVICE)
            {
                for (GhostPort = Port;
                     GhostPort < DeviceRelations->Count;
                     GhostPort++)
                {
                    DeviceRelations->Objects[GhostPort] =
                    DeviceRelations->Objects[GhostPort + 1];
                }

                ObDereferenceObject(PdoDevice);

                DeviceRelations->Count--;

                if (PdoExt(PdoDevice)->EnumFlags & USBHUB_ENUM_FLAG_DEVICE_PRESENT)
                {
                    PdoExt(PdoDevice)->EnumFlags &= ~USBHUB_ENUM_FLAG_GHOST_DEVICE;
                }
            }
        }
    }

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    InitializeListHead(&GhostPdoList);
    PdoList = &HubExtension->PdoList;

    while (!IsListEmpty(PdoList))
    {
        Entry = RemoveHeadList(PdoList);

        PdoExtension = CONTAINING_RECORD(Entry,
                                         USBHUB_PORT_PDO_EXTENSION,
                                         PortLink);

        PdoExtension->EnumFlags &= ~USBHUB_ENUM_FLAG_DEVICE_PRESENT;

        if (PdoExtension->EnumFlags & USBHUB_ENUM_FLAG_GHOST_DEVICE)
        {
            InsertTailList(&GhostPdoList, &PdoExtension->PortLink);
        }
    }

    KeReleaseSpinLock(&HubExtension->RelationsWorkerSpinLock, OldIrql);

    while (!IsListEmpty(&GhostPdoList))
    {
        Entry = RemoveHeadList(&GhostPdoList);

        PdoExtension = CONTAINING_RECORD(Entry,
                                         USBHUB_PORT_PDO_EXTENSION,
                                         PortLink);

        IoDeleteDevice(PdoExtension->Common.SelfDevice);
    }

    return USBH_PassIrp(HubExtension->LowerDevice, Irp);
}

NTSTATUS
NTAPI
USBH_FdoStopDevice(IN PUSBHUB_FDO_EXTENSION HubExtension,
                   IN PIRP Irp)
{
    DPRINT1("USBH_FdoStopDevice: UNIMPLEMENTED. FIXME\n");
    DbgBreakPoint();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBH_FdoRemoveDevice(IN PUSBHUB_FDO_EXTENSION HubExtension,
                     IN PIRP Irp)
{
    PUSB_HUB_DESCRIPTOR HubDescriptor;
    PUSBHUB_PORT_DATA PortData;
    USHORT NumPorts;
    USHORT ix;
    PDEVICE_OBJECT PortDevice;
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    NTSTATUS Status;

    DPRINT("USBH_FdoRemoveDevice: HubExtension - %p\n", HubExtension);

    HubDescriptor = HubExtension->HubDescriptor;

    if (HubDescriptor && HubExtension->PortData)
    {
        NumPorts = HubDescriptor->bNumberOfPorts;

        for (ix = 0; ix < NumPorts; ++ix)
        {
            PortData = HubExtension->PortData + ix;

            PortDevice = PortData->DeviceObject;

            if (PortDevice)
            {
                PortData->PortStatus.AsUlong32 = 0;
                PortData->DeviceObject = NULL;

                PortExtension = PortDevice->DeviceExtension;
                PortExtension->EnumFlags &= ~USBHUB_ENUM_FLAG_DEVICE_PRESENT;

                USBH_PdoRemoveDevice(PortExtension, HubExtension);
            }
        }
    }

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STARTED)
    {
        USBH_FdoCleanup(HubExtension);
    }

    if (HubExtension->PortData)
    {
        ExFreePoolWithTag(HubExtension->PortData, USB_HUB_TAG);
        HubExtension->PortData = NULL;
    }

    DPRINT1("USBH_FdoRemoveDevice: call IoWMIRegistrationControl UNIMPLEMENTED. FIXME\n");

    Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);

    IoDetachDevice(HubExtension->LowerDevice);
    IoDeleteDevice(HubExtension->Common.SelfDevice);

    return Status;
}

VOID
NTAPI
USBH_FdoSurpriseRemoveDevice(IN PUSBHUB_FDO_EXTENSION HubExtension,
                             IN PIRP Irp)
{
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    PUSBHUB_PORT_DATA PortData;
    ULONG NumberPorts;
    ULONG Port;

    DPRINT("USBH_FdoSurpriseRemoveDevice: HubExtension - %p, Irp - %p\n",
           HubExtension,
           Irp);

    if (!HubExtension->PortData ||
        !HubExtension->HubDescriptor)
    {
        return;
    }

    PortData = HubExtension->PortData;
    NumberPorts = HubExtension->HubDescriptor->bNumberOfPorts;

    for (Port = 0; Port < NumberPorts; Port++)
    {
        if (PortData[Port].DeviceObject)
        {
            PortExtension = PdoExt(PortData[Port].DeviceObject);
            PortExtension->PortPdoFlags |= USBHUB_PDO_FLAG_DELETE_PENDING;
            PortExtension->EnumFlags &= ~USBHUB_ENUM_FLAG_DEVICE_PRESENT;

            PortData[Port].DeviceObject = NULL;
            PortData[Port].ConnectionStatus = NoDeviceConnected;
        }
    }
}

NTSTATUS
NTAPI
USBH_PdoQueryId(IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
                IN PIRP Irp)
{
    ULONG IdType;
    WCHAR Buffer[200];
    PWCHAR EndBuffer;
    size_t Remaining = sizeof(Buffer);
    size_t Length;
    PWCHAR Id = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;

    IdType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryId.IdType;
    DeviceDescriptor = &PortExtension->DeviceDescriptor;
    InterfaceDescriptor = &PortExtension->InterfaceDescriptor;

    RtlZeroMemory(Buffer, sizeof(Buffer));

    switch (IdType)
    {
        case BusQueryDeviceID:
            DPRINT("USBH_PdoQueryId: BusQueryDeviceID\n");

            if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_INIT_PORT_FAILED)
            {
                DPRINT("USBH_PdoQueryId: USBHUB_PDO_FLAG_INIT_PORT_FAILED\n");
                RtlStringCbPrintfExW(Buffer,
                                     Remaining,
                                     NULL,
                                     &Remaining,
                                     0,
                                     L"USB\\Vid_0000&Pid_0000");
            }
            else
            {
                RtlStringCbPrintfExW(Buffer,
                                     Remaining,
                                     NULL,
                                     &Remaining,
                                     0,
                                     L"USB\\Vid_%04x&Pid_%04x",
                                     DeviceDescriptor->idVendor,
                                     DeviceDescriptor->idProduct);
            }

            Length = sizeof(Buffer) - (Remaining - sizeof(UNICODE_NULL));

            Id = ExAllocatePoolWithTag(PagedPool, Length, USB_HUB_TAG);

            if (!Id)
            {
                break;
            }

            RtlCopyMemory(Id, Buffer, Length);
            DPRINT("USBH_PdoQueryId: BusQueryDeviceID - %S\n", Id);
            break;

        case BusQueryHardwareIDs:
            DPRINT("USBH_PdoQueryId: BusQueryHardwareIDs\n");

            if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_INIT_PORT_FAILED)
            {
                DPRINT("USBH_PdoQueryId: USBHUB_PDO_FLAG_INIT_PORT_FAILED\n");

                RtlStringCbPrintfExW(Buffer,
                                     Remaining,
                                     NULL,
                                     &Remaining,
                                     0,
                                     L"USB\\UNKNOWN");
            }
            else
            {
                RtlStringCbPrintfExW(Buffer,
                                     Remaining,
                                     &EndBuffer,
                                     &Remaining,
                                     0,
                                     L"USB\\Vid_%04x&Pid_%04x&Rev_%04x",
                                     DeviceDescriptor->idVendor,
                                     DeviceDescriptor->idProduct,
                                     DeviceDescriptor->bcdDevice);

                EndBuffer++;
                Remaining -= sizeof(UNICODE_NULL);

                RtlStringCbPrintfExW(EndBuffer,
                                     Remaining,
                                     NULL,
                                     &Remaining,
                                     0,
                                     L"USB\\Vid_%04x&Pid_%04x",
                                     DeviceDescriptor->idVendor,
                                     DeviceDescriptor->idProduct);
            }

            Length = sizeof(Buffer) - (Remaining - 2 * sizeof(UNICODE_NULL));

            Id = ExAllocatePoolWithTag(PagedPool, Length, USB_HUB_TAG);

            if (!Id)
            {
                break;
            }

            RtlCopyMemory(Id, Buffer, Length);

            if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_INIT_PORT_FAILED)
            {
                DPRINT("USBH_PdoQueryId: BusQueryHardwareID - %S\n", Id);
            }
            else
            {
                USBHUB_DumpingIDs(Id);
            }

            break;

        case BusQueryCompatibleIDs:
            DPRINT("USBH_PdoQueryId: BusQueryCompatibleIDs\n");

            if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_INIT_PORT_FAILED)
            {
                DPRINT("USBH_PdoQueryId: USBHUB_PDO_FLAG_INIT_PORT_FAILED\n");

                RtlStringCbPrintfExW(Buffer,
                                     Remaining,
                                     NULL,
                                     &Remaining,
                                     0,
                                     L"USB\\UNKNOWN");
            }
            else if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_MULTI_INTERFACE)
            {
                RtlStringCbPrintfExW(Buffer,
                                     Remaining,
                                     &EndBuffer,
                                     &Remaining,
                                     0,
                                     L"USB\\DevClass_%02x&SubClass_%02x&Prot_%02x",
                                     InterfaceDescriptor->bInterfaceClass,
                                     InterfaceDescriptor->bInterfaceSubClass,
                                     InterfaceDescriptor->bInterfaceProtocol);

                EndBuffer++;
                Remaining -= sizeof(UNICODE_NULL);

                RtlStringCbPrintfExW(EndBuffer,
                                     Remaining,
                                     &EndBuffer,
                                     &Remaining,
                                     0,
                                     L"USB\\DevClass_%02x&SubClass_%02x",
                                     InterfaceDescriptor->bInterfaceClass,
                                     InterfaceDescriptor->bInterfaceSubClass);

                EndBuffer++;
                Remaining -= sizeof(UNICODE_NULL);

                RtlStringCbPrintfExW(EndBuffer,
                                     Remaining,
                                     &EndBuffer,
                                     &Remaining,
                                     0,
                                     L"USB\\DevClass_%02x",
                                     InterfaceDescriptor->bInterfaceClass);

                EndBuffer++;
                Remaining -= sizeof(UNICODE_NULL);

                RtlStringCbPrintfExW(EndBuffer,
                                     Remaining,
                                     NULL,
                                     &Remaining,
                                     0,
                                     L"USB\\COMPOSITE");
            }
            else
            {
                RtlStringCbPrintfExW(Buffer,
                                     Remaining,
                                     &EndBuffer,
                                     &Remaining,
                                     0,
                                     L"USB\\Class_%02x&SubClass_%02x&Prot_%02x",
                                     InterfaceDescriptor->bInterfaceClass,
                                     InterfaceDescriptor->bInterfaceSubClass,
                                     InterfaceDescriptor->bInterfaceProtocol);

                EndBuffer++;
                Remaining -= sizeof(UNICODE_NULL);

                RtlStringCbPrintfExW(EndBuffer,
                                     Remaining,
                                     &EndBuffer,
                                     &Remaining,
                                     0,
                                     L"USB\\Class_%02x&SubClass_%02x",
                                     InterfaceDescriptor->bInterfaceClass,
                                     InterfaceDescriptor->bInterfaceSubClass);

                EndBuffer++;
                Remaining -= sizeof(UNICODE_NULL);

                RtlStringCbPrintfExW(EndBuffer,
                                     Remaining,
                                     NULL,
                                     &Remaining,
                                     0,
                                     L"USB\\Class_%02x",
                                     InterfaceDescriptor->bInterfaceClass);
            }

            Length = sizeof(Buffer) - (Remaining - 2 * sizeof(UNICODE_NULL));

            Id = ExAllocatePoolWithTag(PagedPool, Length, USB_HUB_TAG);

            if (!Id)
            {
                break;
            }

            RtlCopyMemory(Id, Buffer, Length);

            if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_INIT_PORT_FAILED)
            {
                DPRINT("USBH_PdoQueryId: BusQueryCompatibleID - %S\n", Id);
            }
            else
            {
                USBHUB_DumpingIDs(Id);
            }

            break;

        case BusQueryInstanceID:
            DPRINT("USBH_PdoQueryId: BusQueryInstanceID\n");

            if (PortExtension->SerialNumber)
            {
                Id = ExAllocatePoolWithTag(PagedPool,
                                           PortExtension->SN_DescriptorLength,
                                           USB_HUB_TAG);

                if (Id)
                {
                    RtlZeroMemory(Id, PortExtension->SN_DescriptorLength);

                    RtlCopyMemory(Id,
                                  PortExtension->SerialNumber,
                                  PortExtension->SN_DescriptorLength);
                }
            }
            else
            {
                 Length = sizeof(PortExtension->InstanceID) +
                          sizeof(UNICODE_NULL);

                 Id = ExAllocatePoolWithTag(PagedPool, Length, USB_HUB_TAG);

                 if (Id)
                 {
                     RtlZeroMemory(Id, Length);

                     RtlCopyMemory(Id,
                                   PortExtension->InstanceID,
                                   sizeof(PortExtension->InstanceID));
                 }
            }

            DPRINT("USBH_PdoQueryId: BusQueryInstanceID - %S\n", Id);
            break;

        default:
            DPRINT1("USBH_PdoQueryId: unknown query id type 0x%lx\n", IdType);
            return Irp->IoStatus.Status;
    }

    Irp->IoStatus.Information = (ULONG_PTR)Id;

    if (!Id)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_PdoQueryDeviceText(IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
                        IN PIRP Irp)
{
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION IoStack;
    DEVICE_TEXT_TYPE DeviceTextType;
    USHORT LanguageId;
    USHORT DefaultId;
    PUSB_STRING_DESCRIPTOR Descriptor;
    PWCHAR DeviceText;
    UCHAR iProduct = 0;
    NTSTATUS Status;
    size_t NumSymbols;
    size_t Length;

    DPRINT("USBH_PdoQueryDeviceText ... \n");

    DeviceObject = PortExtension->Common.SelfDevice;
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceTextType = IoStack->Parameters.QueryDeviceText.DeviceTextType;

    if (DeviceTextType != DeviceTextDescription &&
        DeviceTextType != DeviceTextLocationInformation)
    {
        return Irp->IoStatus.Status;
    }

    LanguageId = LANGIDFROMLCID(IoStack->Parameters.QueryDeviceText.LocaleId);
    DefaultId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

    if (!LanguageId)
    {
        LanguageId = DefaultId;
    }

    iProduct = PortExtension->DeviceDescriptor.iProduct;

    if (PortExtension->DeviceHandle && iProduct &&
        !PortExtension->IgnoringHwSerial &&
        !(PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_INIT_PORT_FAILED))
    {
        Descriptor = ExAllocatePoolWithTag(NonPagedPool,
                                           MAXIMUM_USB_STRING_LENGTH,
                                           USB_HUB_TAG);

        if (Descriptor)
        {
            RtlZeroMemory(Descriptor, MAXIMUM_USB_STRING_LENGTH);

            for (Status = USBH_CheckDeviceLanguage(DeviceObject, LanguageId);
                 ;
                 Status = USBH_CheckDeviceLanguage(DeviceObject, DefaultId))
            {
                if (NT_SUCCESS(Status))
                {
                    Status = USBH_SyncGetStringDescriptor(DeviceObject,
                                                          iProduct,
                                                          LanguageId,
                                                          Descriptor,
                                                          MAXIMUM_USB_STRING_LENGTH,
                                                          NULL,
                                                          TRUE);

                    if (NT_SUCCESS(Status))
                    {
                        break;
                    }
                }

                if (LanguageId == DefaultId)
                {
                    goto Exit;
                }

                LanguageId = DefaultId;
            }

            if (Descriptor->bLength <= sizeof(USB_COMMON_DESCRIPTOR))
            {
                Status = STATUS_UNSUCCESSFUL;
            }

            if (NT_SUCCESS(Status))
            {
                Length = Descriptor->bLength -
                         FIELD_OFFSET(USB_STRING_DESCRIPTOR, bString);

                DeviceText = ExAllocatePoolWithTag(PagedPool,
                                                   Length + sizeof(UNICODE_NULL),
                                                   USB_HUB_TAG);

                if (DeviceText)
                {
                    RtlZeroMemory(DeviceText, Length + sizeof(UNICODE_NULL));

                    RtlCopyMemory(DeviceText, Descriptor->bString, Length);

                    Irp->IoStatus.Information = (ULONG_PTR)DeviceText;

                    DPRINT("USBH_PdoQueryDeviceText: Descriptor->bString - %S\n",
                           DeviceText);
                }
                else
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

        Exit:

            ExFreePoolWithTag(Descriptor, USB_HUB_TAG);

            if (NT_SUCCESS(Status))
            {
                return Status;
            }
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        Status = STATUS_NOT_SUPPORTED;
    }

    if (!GenericUSBDeviceString)
    {
        return Status;
    }

    NumSymbols = wcslen(GenericUSBDeviceString);
    Length = (NumSymbols + 1) * sizeof(WCHAR);

    DeviceText = ExAllocatePoolWithTag(PagedPool, Length, USB_HUB_TAG);

    if (!DeviceText)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(DeviceText, Length);

    RtlCopyMemory(DeviceText,
                  GenericUSBDeviceString,
                  NumSymbols * sizeof(WCHAR));

    Irp->IoStatus.Information = (ULONG_PTR)DeviceText;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBH_SymbolicLink(IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
                  IN const GUID * InterfaceClassGuid,
                  IN BOOLEAN IsEnable)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID NameBuffer;

    DPRINT("USBH_SymbolicLink ... \n");

    if (IsEnable)
    {
        Status = IoRegisterDeviceInterface(PortExtension->Common.SelfDevice,
                                           InterfaceClassGuid,
                                           NULL,
                                           &PortExtension->SymbolicLinkName);

        if (NT_SUCCESS(Status))
        {
            USBH_SetPdoRegistryParameter(PortExtension->Common.SelfDevice,
                                         L"SymbolicName",
                                         PortExtension->SymbolicLinkName.Buffer,
                                         PortExtension->SymbolicLinkName.Length,
                                         REG_SZ,
                                         PLUGPLAY_REGKEY_DEVICE);

            Status = IoSetDeviceInterfaceState(&PortExtension->SymbolicLinkName,
                                               TRUE);
        }
    }
    else
    {
        NameBuffer = PortExtension->SymbolicLinkName.Buffer;

        if (NameBuffer)
        {
            Status = IoSetDeviceInterfaceState(&PortExtension->SymbolicLinkName,
                                               FALSE);

            ExFreePool(PortExtension->SymbolicLinkName.Buffer);

            PortExtension->SymbolicLinkName.Buffer = NULL;
        }
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_RestoreDevice(IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
                   IN BOOLEAN IsKeepDeviceData)
{
    PUSBHUB_FDO_EXTENSION HubExtension;
    PUSBHUB_PORT_DATA PortData;
    NTSTATUS Status;
    ULONG ix;

    DPRINT("USBH_RestoreDevice ... \n");

    HubExtension = PortExtension->HubExtension;

    if (!HubExtension)
    {
        Status = STATUS_UNSUCCESSFUL;
        return Status;
    }

    ASSERT(PortExtension->PortNumber > 0);
    PortData = &HubExtension->PortData[PortExtension->PortNumber - 1];

    if (PortExtension->Common.SelfDevice != PortData->DeviceObject)
    {
        Status = STATUS_UNSUCCESSFUL;
        return Status;
    }

    Status = USBH_SyncGetPortStatus(HubExtension,
                                    PortExtension->PortNumber,
                                    &PortData->PortStatus,
                                    sizeof(USB_PORT_STATUS_AND_CHANGE));

    if (NT_SUCCESS(Status))
    {
        for (ix = 0; ix < 3; ix++)
        {
            Status = USBH_ResetDevice((PUSBHUB_FDO_EXTENSION)HubExtension,
                                      PortExtension->PortNumber,
                                      IsKeepDeviceData,
                                      ix == 0);

            if (NT_SUCCESS(Status) || Status == STATUS_NO_SUCH_DEVICE)
            {
                break;
            }

            USBH_Wait(1000);
        }
    }

    PortExtension->PortPdoFlags &= ~USBHUB_PDO_FLAG_POWER_D3;

    if (NT_SUCCESS(Status))
    {
        PortExtension->PortPdoFlags &= ~USBHUB_PDO_FLAG_PORT_RESTORE_FAIL;
    }
    else
    {
        PortExtension->PortPdoFlags |= (USBHUB_PDO_FLAG_INIT_PORT_FAILED |
                                        USBHUB_PDO_FLAG_PORT_RESTORE_FAIL);
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_PdoStartDevice(IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
                    IN PIRP Irp)
{
    PUSBHUB_FDO_EXTENSION HubExtension;
    const GUID * Guid;
    NTSTATUS Status;

    DPRINT("USBH_PdoStartDevice: PortExtension - %p\n", PortExtension);

    if (!PortExtension->HubExtension &&
        PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_POWER_D3)
    {
        PortExtension->HubExtension = PortExtension->RootHubExtension;
    }

    HubExtension = PortExtension->HubExtension;

    if (HubExtension)
    {
        USBHUB_SetDeviceHandleData(HubExtension,
                                   PortExtension->Common.SelfDevice,
                                   PortExtension->DeviceHandle);
    }

    if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_HUB_DEVICE)
    {
        Guid = &GUID_DEVINTERFACE_USB_HUB;
    }
    else
    {
        Guid = &GUID_DEVINTERFACE_USB_DEVICE;
    }

    Status = USBH_SymbolicLink(PortExtension, Guid, TRUE);

    if (NT_SUCCESS(Status))
    {
        PortExtension->PortPdoFlags |= USBHUB_PDO_FLAG_REG_DEV_INTERFACE;
    }

    if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_POWER_D3)
    {
        Status = USBH_RestoreDevice(PortExtension, 0);
    }

    PortExtension->PortPdoFlags |= USBHUB_PDO_FLAG_DEVICE_STARTED;

    PortExtension->CurrentPowerState.DeviceState = PowerDeviceD0;

    DPRINT1("USBH_PdoStartDevice: call IoWMIRegistrationControl UNIMPLEMENTED. FIXME\n");

    return Status;
}

NTSTATUS
NTAPI
USBH_PdoRemoveDevice(IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
                     IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT PortDevice;
    PUSBHUB_PORT_PDO_EXTENSION PortExt;
    PUSBHUB_PORT_DATA PortData;
    PIRP IdleNotificationIrp;
    PIRP WakeIrp;
    PVOID DeviceHandle;
    PDEVICE_OBJECT Pdo;
    PVOID SerialNumber;
    USHORT Port;
    KIRQL Irql;

    DPRINT("USBH_PdoRemoveDevice ... \n");

    PortDevice = PortExtension->Common.SelfDevice;
    PortExtension->HubExtension = NULL;

    Port = PortExtension->PortNumber;
    ASSERT(Port > 0);

    if (HubExtension &&
        HubExtension->CurrentPowerState.DeviceState != PowerDeviceD0 &&
        (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STARTED) != 0)
    {
        USBH_HubSetD0(HubExtension);
    }

    IoAcquireCancelSpinLock(&Irql);
    IdleNotificationIrp = PortExtension->IdleNotificationIrp;

    if (IdleNotificationIrp)
    {
        PortExtension->IdleNotificationIrp = NULL;
        PortExtension->PortPdoFlags &= ~USBHUB_PDO_FLAG_IDLE_NOTIFICATION;

        if (IdleNotificationIrp->Cancel)
        {
            IdleNotificationIrp = NULL;
        }

        if (IdleNotificationIrp)
        {
            IoSetCancelRoutine(IdleNotificationIrp, NULL);
        }
    }

    WakeIrp = PortExtension->PdoWaitWakeIrp;

    if (WakeIrp)
    {
        PortExtension->PdoWaitWakeIrp = NULL;
        PortExtension->PortPdoFlags &= ~USBHUB_PDO_FLAG_WAIT_WAKE;

        if (WakeIrp->Cancel || !IoSetCancelRoutine(WakeIrp, NULL))
        {
            WakeIrp = NULL;

            ASSERT(HubExtension);
            if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
            {
                KeSetEvent(&HubExtension->PendingRequestEvent,
                           EVENT_INCREMENT,
                           FALSE);
            }
        }
    }

    IoReleaseCancelSpinLock(Irql);

    if (IdleNotificationIrp)
    {
        IdleNotificationIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(IdleNotificationIrp, IO_NO_INCREMENT);
    }

    if (WakeIrp)
    {
        ASSERT(HubExtension);
        USBH_CompletePowerIrp(HubExtension, WakeIrp, STATUS_CANCELLED);
    }

    PortExtension->PortPdoFlags |= USBHUB_PDO_FLAG_POWER_D3;

    if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_REG_DEV_INTERFACE)
    {
        Status = USBH_SymbolicLink(PortExtension, NULL, FALSE);

        if (NT_SUCCESS(Status))
        {
            PortExtension->PortPdoFlags &= ~USBHUB_PDO_FLAG_REG_DEV_INTERFACE;
        }
    }

    DeviceHandle = InterlockedExchangePointer(&PortExtension->DeviceHandle,
                                              NULL);

    if (DeviceHandle)
    {
        ASSERT(HubExtension);
        Status = USBD_RemoveDeviceEx(HubExtension, DeviceHandle, 0);

        if (HubExtension->PortData &&
            HubExtension->PortData[Port - 1].DeviceObject == PortDevice)
        {
            USBH_SyncDisablePort(HubExtension, Port);
        }
    }

    if (NT_SUCCESS(Status))
    {
        PortExtension->PortPdoFlags &= ~USBHUB_PDO_FLAG_DEVICE_STARTED;

        if (HubExtension && HubExtension->PortData)
        {
            PortData = &HubExtension->PortData[Port - 1];

            if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_DELETE_PENDING)
            {
                Pdo = PortData->DeviceObject;

                if (Pdo)
                {
                    PortData->DeviceObject = NULL;
                    PortData->ConnectionStatus = NoDeviceConnected;

                    if (PdoExt(Pdo)->EnumFlags & USBHUB_ENUM_FLAG_DEVICE_PRESENT)
                    {
                        PortExt = PdoExt(Pdo);

                        InsertTailList(&HubExtension->PdoList,
                                       &PortExt->PortLink);
                    }
                }
            }
        }

        if (!(PortExtension->EnumFlags & USBHUB_ENUM_FLAG_DEVICE_PRESENT) &&
            !(PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_NOT_CONNECTED))
        {
            PortExtension->PortPdoFlags |= USBHUB_PDO_FLAG_NOT_CONNECTED;

            SerialNumber = InterlockedExchangePointer((PVOID)&PortExtension->SerialNumber,
                                                      NULL);

            if (SerialNumber)
            {
                ExFreePoolWithTag(SerialNumber, USB_HUB_TAG);
            }

            DPRINT1("USBH_PdoRemoveDevice: call IoWMIRegistrationControl UNIMPLEMENTED. FIXME\n");

            if (HubExtension)
                USBHUB_FlushAllTransfers(HubExtension);

            IoDeleteDevice(PortDevice);
        }
    }

    if (HubExtension)
    {
        DPRINT("USBH_PdoRemoveDevice: call USBH_CheckIdleDeferred()\n");
        USBH_CheckIdleDeferred(HubExtension);
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_PdoStopDevice(IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
                   IN PIRP Irp)
{
    DPRINT1("USBH_PdoStopDevice: UNIMPLEMENTED. FIXME\n");
    DbgBreakPoint();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBH_FdoPnP(IN PUSBHUB_FDO_EXTENSION HubExtension,
            IN PIRP Irp,
            IN UCHAR Minor)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    DEVICE_RELATION_TYPE RelationsType;
    BOOLEAN IsCheckIdle;

    DPRINT_PNP("USBH_FdoPnP: HubExtension - %p, Irp - %p, Minor - %X\n",
               HubExtension,
               Irp,
               Minor);

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_WAIT_IDLE_REQUEST &&
        (Minor == IRP_MN_REMOVE_DEVICE || Minor == IRP_MN_STOP_DEVICE))
    {
        HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_SUSPENDED;
    }

    KeWaitForSingleObject(&HubExtension->IdleSemaphore,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    DPRINT_PNP("USBH_FdoPnP: HubFlags - %lX\n", HubExtension->HubFlags);

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_GOING_IDLE)
    {
        HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_SUSPENDED;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    RelationsType = IoStack->Parameters.QueryDeviceRelations.Type;

    if ((HubExtension->CurrentPowerState.DeviceState == PowerDeviceD0) ||
        !(HubExtension->HubFlags & (USBHUB_FDO_FLAG_DEVICE_STOPPED | USBHUB_FDO_FLAG_DEVICE_STARTED)) ||
        (Minor == IRP_MN_QUERY_DEVICE_RELATIONS && RelationsType == TargetDeviceRelation))
    {
        IsCheckIdle = FALSE;
    }
    else
    {
        DPRINT_PNP("USBH_FdoPnP: IsCheckIdle - TRUE\n");
        IsCheckIdle = TRUE;
        USBH_HubSetD0(HubExtension);
    }

    switch (Minor)
    {
        case IRP_MN_START_DEVICE:
            DPRINT_PNP("FDO IRP_MN_START_DEVICE\n");
            IsCheckIdle = FALSE;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_FdoStartDevice(HubExtension, Irp);
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            DPRINT_PNP("FDO IRP_MN_QUERY_REMOVE_DEVICE\n");
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE:
            DPRINT_PNP("FDO IRP_MN_REMOVE_DEVICE\n");
            IsCheckIdle = FALSE;
            HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_REMOVED;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_FdoRemoveDevice(HubExtension, Irp);
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            DPRINT_PNP("FDO IRP_MN_CANCEL_REMOVE_DEVICE\n");
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_STOP_DEVICE:
            DPRINT_PNP("FDO IRP_MN_STOP_DEVICE\n");
            IsCheckIdle = FALSE;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_FdoStopDevice(HubExtension, Irp);
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
            DPRINT_PNP("FDO IRP_MN_QUERY_STOP_DEVICE\n");
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:
            DPRINT_PNP("FDO IRP_MN_CANCEL_STOP_DEVICE\n");
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            DPRINT_PNP("FDO IRP_MN_QUERY_DEVICE_RELATIONS\n");

            if (RelationsType != BusRelations)
            {
                Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
                break;
            }

            HubExtension->HubFlags |= USBHUB_FDO_FLAG_HUB_BUSY;

            IsCheckIdle = TRUE;
            DPRINT_PNP("USBH_FdoPnP: IsCheckIdle - TRUE\n");

            Status = USBH_FdoQueryBusRelations(HubExtension, Irp);

            HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_HUB_BUSY;
            break;

        case IRP_MN_QUERY_INTERFACE:
            DPRINT_PNP("FDO IRP_MN_QUERY_INTERFACE\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            DPRINT_PNP("FDO IRP_MN_QUERY_CAPABILITIES\n");
            IoCopyCurrentIrpStackLocationToNext(Irp);

            IoSetCompletionRoutine(Irp,
                                   USBH_QueryCapsComplete,
                                   HubExtension,
                                   TRUE,
                                   FALSE,
                                   FALSE);

            Status = IoCallDriver(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_RESOURCES:
            DPRINT_PNP("FDO IRP_MN_QUERY_RESOURCES\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            DPRINT_PNP("FDO IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_DEVICE_TEXT:
            DPRINT_PNP("FDO IRP_MN_QUERY_DEVICE_TEXT\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            DPRINT_PNP("FDO IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_READ_CONFIG:
            DPRINT_PNP("FDO IRP_MN_READ_CONFIG\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_WRITE_CONFIG:
            DPRINT_PNP("FDO IRP_MN_WRITE_CONFIG\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_EJECT:
            DPRINT_PNP("FDO IRP_MN_EJECT\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_SET_LOCK:
            DPRINT_PNP("FDO IRP_MN_SET_LOCK\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_ID:
            DPRINT_PNP("FDO IRP_MN_QUERY_ID\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            DPRINT_PNP("FDO IRP_MN_QUERY_PNP_DEVICE_STATE\n");

            if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_FAILED)
            {
                Irp->IoStatus.Information |= PNP_DEVICE_FAILED;
            }

            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_BUS_INFORMATION:
            DPRINT_PNP("FDO IRP_MN_QUERY_BUS_INFORMATION\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            DPRINT_PNP("FDO IRP_MN_DEVICE_USAGE_NOTIFICATION\n");
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            DPRINT_PNP("FDO IRP_MN_SURPRISE_REMOVAL\n");
            USBH_FdoSurpriseRemoveDevice(HubExtension, Irp);
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        default:
            DPRINT_PNP("FDO unknown IRP_MN_???\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;
    }

    KeReleaseSemaphore(&HubExtension->IdleSemaphore,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    if (IsCheckIdle)
    {
        DPRINT_PNP("USBH_FdoPnP: call USBH_CheckIdleDeferred()\n");
        USBH_CheckIdleDeferred(HubExtension);
    }

    HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_STATE_CHANGING;

    return Status;
}

NTSTATUS
NTAPI
USBH_PdoPnP(IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
            IN PIRP Irp,
            IN UCHAR Minor,
            OUT BOOLEAN * IsCompleteIrp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    PPNP_BUS_INFORMATION BusInfo;
    PDEVICE_CAPABILITIES DeviceCapabilities;
    USHORT Size;
    USHORT Version;
    PUSBHUB_FDO_EXTENSION HubExtension;
    PDEVICE_RELATIONS DeviceRelation;

    DPRINT_PNP("USBH_PdoPnP: PortExtension - %p, Irp - %p, Minor - %X\n",
               PortExtension,
               Irp,
               Minor);

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    *IsCompleteIrp = TRUE;

    switch (Minor)
    {
        case IRP_MN_START_DEVICE:
            DPRINT_PNP("PDO IRP_MN_START_DEVICE\n");
            return USBH_PdoStartDevice(PortExtension, Irp);

        case IRP_MN_QUERY_REMOVE_DEVICE:
            DPRINT_PNP("PDO IRP_MN_QUERY_REMOVE_DEVICE\n");
            return STATUS_SUCCESS;

        case IRP_MN_REMOVE_DEVICE:
            DPRINT_PNP("PDO IRP_MN_REMOVE_DEVICE\n");
            return USBH_PdoRemoveDevice(PortExtension, PortExtension->HubExtension);

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            DPRINT_PNP("PDO IRP_MN_CANCEL_REMOVE_DEVICE\n");
            return STATUS_SUCCESS;

        case IRP_MN_STOP_DEVICE:
            DPRINT_PNP("PDO IRP_MN_STOP_DEVICE\n");
            return USBH_PdoStopDevice(PortExtension, Irp);

        case IRP_MN_QUERY_STOP_DEVICE:
            DPRINT_PNP("PDO IRP_MN_QUERY_STOP_DEVICE\n");
            return STATUS_SUCCESS;

        case IRP_MN_CANCEL_STOP_DEVICE:
            DPRINT_PNP("PDO IRP_MN_CANCEL_STOP_DEVICE\n");
            return STATUS_SUCCESS;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            DPRINT_PNP("PDO IRP_MN_QUERY_DEVICE_RELATIONS\n");

            if (IoStack->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation)
            {
                return Irp->IoStatus.Status;
            }

            DeviceRelation = ExAllocatePoolWithTag(PagedPool,
                                                   sizeof(DEVICE_RELATIONS),
                                                   USB_HUB_TAG);

            if (DeviceRelation)
            {
                RtlZeroMemory(DeviceRelation, sizeof(DEVICE_RELATIONS));

                DeviceRelation->Count = 1;
                DeviceRelation->Objects[0] = PortExtension->Common.SelfDevice;

                ObReferenceObject(DeviceRelation->Objects[0]);

                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

            Irp->IoStatus.Information = (ULONG_PTR)DeviceRelation;
            break;

        case IRP_MN_QUERY_INTERFACE:
            DPRINT_PNP("PDO IRP_MN_QUERY_INTERFACE\n");

            *IsCompleteIrp = 0;

            if (IsEqualGUIDAligned(IoStack->Parameters.QueryInterface.InterfaceType,
                                   &USB_BUS_INTERFACE_USBDI_GUID))
            {
                IoStack->Parameters.QueryInterface.InterfaceSpecificData = PortExtension->DeviceHandle;
            }

            HubExtension = PortExtension->HubExtension;

            if (!HubExtension)
            {
                HubExtension = PortExtension->RootHubExtension;
            }

            Status = USBH_PassIrp(HubExtension->RootHubPdo, Irp);
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            DPRINT_PNP("PDO IRP_MN_QUERY_CAPABILITIES\n");

            DeviceCapabilities = IoStack->Parameters.DeviceCapabilities.Capabilities;

            Size = DeviceCapabilities->Size;
            Version = DeviceCapabilities->Version;

            RtlCopyMemory(DeviceCapabilities,
                          &PortExtension->Capabilities,
                          sizeof(DEVICE_CAPABILITIES));

            DeviceCapabilities->Size = Size;
            DeviceCapabilities->Version = Version;

            /* All devices connected to a hub are removable */
            DeviceCapabilities->Removable = 1;

            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_RESOURCES:
            DPRINT_PNP("PDO IRP_MN_QUERY_RESOURCES\n");
            Status = Irp->IoStatus.Status;
            break;

        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            DPRINT_PNP("PDO IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
            PortExtension->PortPdoFlags |= USBHUB_PDO_FLAG_ENUMERATED;

            /* FIXME HKEY_LOCAL_MACHINE\SYSTEM\ControlSetXXX\Enum\USB\
               Vid_????&Pid_????\????????????\Device Parameters\
               if (ExtPropDescSemaphore)
            */

            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_DEVICE_TEXT:
            DPRINT_PNP("PDO IRP_MN_QUERY_DEVICE_TEXT\n");
            return USBH_PdoQueryDeviceText(PortExtension, Irp);

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            DPRINT_PNP("PDO IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
            Status = Irp->IoStatus.Status;
            break;

        case IRP_MN_READ_CONFIG:
            DPRINT_PNP("PDO IRP_MN_READ_CONFIG\n");
            DbgBreakPoint();
            Status = Irp->IoStatus.Status;
            break;

        case IRP_MN_WRITE_CONFIG:
            DPRINT_PNP("PDO IRP_MN_WRITE_CONFIG\n");
            DbgBreakPoint();
            Status = Irp->IoStatus.Status;
            break;

        case IRP_MN_EJECT:
            DPRINT_PNP("PDO IRP_MN_EJECT\n");
            DbgBreakPoint();
            Status = Irp->IoStatus.Status;
            break;

        case IRP_MN_SET_LOCK:
            DPRINT_PNP("PDO IRP_MN_SET_LOCK\n");
            DbgBreakPoint();
            Status = Irp->IoStatus.Status;
            break;

        case IRP_MN_QUERY_ID:
            DPRINT_PNP("PDO IRP_MN_QUERY_ID\n");
            return USBH_PdoQueryId(PortExtension, Irp);

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            DPRINT_PNP("PDO IRP_MN_QUERY_PNP_DEVICE_STATE\n");
            if (PortExtension->PortPdoFlags & (USBHUB_PDO_FLAG_INSUFFICIENT_PWR |
                                               USBHUB_PDO_FLAG_OVERCURRENT_PORT |
                                               USBHUB_PDO_FLAG_PORT_RESTORE_FAIL |
                                               USBHUB_PDO_FLAG_INIT_PORT_FAILED))
            {
                Irp->IoStatus.Information |= PNP_DEVICE_FAILED;
            }

            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_BUS_INFORMATION:
            DPRINT_PNP("PDO IRP_MN_QUERY_BUS_INFORMATION\n");

            BusInfo = ExAllocatePoolWithTag(PagedPool,
                                            sizeof(PNP_BUS_INFORMATION),
                                            USB_HUB_TAG);

            if (!BusInfo)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlZeroMemory(BusInfo, sizeof(PNP_BUS_INFORMATION));

            RtlCopyMemory(&BusInfo->BusTypeGuid,
                          &GUID_BUS_TYPE_USB,
                          sizeof(BusInfo->BusTypeGuid));

            BusInfo->LegacyBusType = PNPBus;
            BusInfo->BusNumber = 0;

            Irp->IoStatus.Information = (ULONG_PTR)BusInfo;
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            DPRINT_PNP("PDO IRP_MN_DEVICE_USAGE_NOTIFICATION\n");
            DbgBreakPoint();
            Status = Irp->IoStatus.Status;
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            DPRINT_PNP("PDO IRP_MN_SURPRISE_REMOVAL\n");
            if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_REG_DEV_INTERFACE)
            {
                Status = USBH_SymbolicLink(PortExtension, NULL, FALSE);

                if (NT_SUCCESS(Status))
                {
                    PortExtension->PortPdoFlags &= ~USBHUB_PDO_FLAG_REG_DEV_INTERFACE;
                }
            }

            Status = STATUS_SUCCESS;
            break;

        default:
            DPRINT_PNP("PDO unknown IRP_MN_???\n");
            Status = Irp->IoStatus.Status;
            break;
    }

    return Status;
}
