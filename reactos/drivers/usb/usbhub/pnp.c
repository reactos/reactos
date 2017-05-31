#include "usbhub.h"

//#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
USBH_IrpCompletion(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp,
                   IN PVOID Context)
{
    PRKEVENT Event;

    DPRINT("USBH_IrpCompletion: Irp - %p\n", Irp);

    Event = (PRKEVENT)Context;
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

     HubExtension = (PUSBHUB_FDO_EXTENSION)Context;

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        DPRINT1("USBH_HubPnPIrpComplete: Irp failed - %p\n", Irp->IoStatus.Status);
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
    NTSTATUS Status;
    PDEVICE_CAPABILITIES Capabilities;

    DPRINT("USBH_QueryCapsComplete: ... \n");

    Status = Irp->IoStatus.Status;

    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    IoStack= IoGetCurrentIrpStackLocation(Irp);
    Capabilities = IoStack->Parameters.DeviceCapabilities.Capabilities;

    Capabilities->SurpriseRemovalOK = 1;

    return Status;
}

NTSTATUS
NTAPI
USBHUB_GetBusInterface(IN PDEVICE_OBJECT DeviceObject,
                       IN PUSB_BUS_INTERFACE_HUB_V5 BusInterface)
{
    PIRP Irp;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    KEVENT Event;

    DPRINT("USBHUB_GetBusInterface: ... \n");

    Irp = IoAllocateIrp(DeviceObject->StackSize, 0);

    if (!Irp)
    {
        DPRINT1("USBHUB_GetBusInterface: IoAllocateIrp() failed\n");
        return STATUS_UNSUCCESSFUL;
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
                            IN PUSB_BUS_INTERFACE_USBDI_V2 BusInterfaceUSBDI)
{
    PIRP Irp;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    KEVENT Event;

    DPRINT("USBHUB_GetBusInterfaceUSBDI: ... \n");

    Irp = IoAllocateIrp(DeviceObject->StackSize, 0);

    if (!Irp)
    {
        DPRINT1("USBHUB_GetBusInterfaceUSBDI: IoAllocateIrp() failed\n");
        return STATUS_UNSUCCESSFUL;
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

    Irp = IoAllocateIrp(DeviceObject->StackSize, 0);

    if (!Irp)
    {
        DPRINT1("USBH_QueryCapabilities: IoAllocateIrp() failed\n");
        return;
    }

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    RtlZeroMemory(DeviceCapabilities, sizeof(DEVICE_CAPABILITIES));

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
    IoStack->Parameters.DeviceCapabilities.Capabilities->Address = -1;
    IoStack->Parameters.DeviceCapabilities.Capabilities->UINumber = -1;

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

    ExFreePool(Urb);

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
    ULONG NumberPorts;
    ULONG Port;
    PIRP PortIdleIrp = NULL;
    PIRP PortWakeIrp = NULL;
    LONG DeviceHandle;
    NTSTATUS Status;
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

    if (!HubExtension->HubDescriptor->bNumberOfPorts)
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

                if (PortIdleIrp->Cancel ||
                    !IoSetCancelRoutine(PortWakeIrp, NULL))
                {
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
                DeviceHandle = InterlockedExchange((PLONG)&PortExtension->DeviceHandle,
                                                   0);

                if (DeviceHandle)
                {
                    USBD_RemoveDeviceEx(HubExtension,
                                        (PUSB_DEVICE_HANDLE)DeviceHandle,
                                        0);
                }

                PortExtension->PortPdoFlags |= USBHUB_PDO_FLAG_POWER_D3;
            }
        }

        USBH_SyncDisablePort(HubExtension, Port + 1);
    }

Exit:

    if (HubExtension->SCEBitmap)
    {
        ExFreePool(HubExtension->SCEBitmap);
    }

    if (HubExtension->HubDescriptor)
    {
        ExFreePool(HubExtension->HubDescriptor);
    }

    if (HubExtension->HubConfigDescriptor)
    {
        ExFreePool(HubExtension->HubConfigDescriptor);
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
    WCHAR KeyName[64];
    PVOID DisableRemoteWakeup = NULL;
    ULONG HubCount = 0;
    PUSB_DEVICE_HANDLE DeviceHandle;
    USB_DEVICE_TYPE DeviceType;
    DEVICE_CAPABILITIES  DeviceCapabilities;
    BOOLEAN IsBusPowered;

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
        DPRINT1("USBH_SyncGetRootHubPdo() failed - %p\n", Status);
        goto ErrorExit;
    }

    USBH_WriteFailReasonID(HubExtension->LowerPDO, 5);

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_FAILED)
    {
        DPRINT1("USBH_StartHubFdoDevice: USBHUB_FDO_FLAG_DEVICE_FAILED - TRUE\n");
        Status = STATUS_UNSUCCESSFUL;
        goto ErrorExit;
    }

    HubExtension->HubFlags |= USBHUB_FDO_FLAG_REMOTE_WAKEUP;

    swprintf(KeyName, L"DisableRemoteWakeup");

    Status = USBD_GetPdoRegistryParameter(HubExtension->LowerPDO,
                                          &DisableRemoteWakeup,
                                          sizeof(DisableRemoteWakeup),
                                          KeyName,
                                          (wcslen(KeyName) + 1) * sizeof(WCHAR));

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
        DPRINT1("USBH_StartHubFdoDevice: USBHUB_GetBusInterface() failed - %p\n",
                Status);
        goto ErrorExit;
    }

    Status = USBHUB_GetBusInterfaceUSBDI(HubExtension->LowerDevice,
                                         &HubExtension->BusInterfaceUSBDI);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_StartHubFdoDevice: USBHUB_GetBusInterfaceUSBDI() failed - %p\n",
                Status);
        goto ErrorExit;
    }

    DeviceHandle = USBH_SyncGetDeviceHandle(HubExtension->LowerDevice);

    if (DeviceHandle)
    {
        Status = USBH_GetDeviceType(HubExtension, DeviceHandle, &DeviceType);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("USBH_StartHubFdoDevice: USBH_GetDeviceType() failed - %p\n",
                    Status);

            goto ErrorExit;
        }

        if (DeviceType == Usb20Device)
        {
            HubExtension->HubFlags |= USBHUB_FDO_FLAG_USB20_HUB;
        }
    }

    if (HubCount > 6)
    {
        DPRINT1("USBH_StartHubFdoDevice: HubCount > 6 - %x\n", HubCount);
        USBH_WriteFailReasonID(HubExtension->LowerPDO, 6);
        HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_FAILED;
        DbgBreakPoint();
    }

    RtlZeroMemory(&DeviceCapabilities, sizeof(DEVICE_CAPABILITIES));

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
        DPRINT1("USBH_StartHubFdoDevice: USBH_GetDeviceDescriptor() failed - %p\n",
                Status);
        goto ErrorExit;
    }

    Status = USBH_GetConfigurationDescriptor(HubExtension->Common.SelfDevice,
                                             &HubExtension->HubConfigDescriptor);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_StartHubFdoDevice: USBH_GetConfigurationDescriptor() failed - %p\n",
                Status);
        goto ErrorExit;
    }

    Status = USBH_SyncGetHubDescriptor(HubExtension);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_StartHubFdoDevice: USBH_SyncGetHubDescriptor() failed - %p\n",
                Status);
        goto ErrorExit;
    }

    IsBusPowered = USBH_HubIsBusPowered(HubExtension->Common.SelfDevice,
                                        HubExtension->HubConfigDescriptor);

    if (IsBusPowered)
    {
        HubExtension->MaxPower = 100;
        HubExtension->HubConfigDescriptor->MaxPower = 250; // 500 mA
    }
    else
    {
        HubExtension->MaxPower = 500;
    }

    Status = USBH_OpenConfiguration(HubExtension);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_StartHubFdoDevice: USBH_OpenConfiguration() failed - %p\n",
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

        Port = 1;

        if (HubExtension->HubDescriptor->bNumberOfPorts >= 1)
        {
            do
            {
                USBH_SyncClearPortStatus(HubExtension,
                                         Port++,
                                         USBHUB_FEATURE_C_PORT_CONNECTION);
            }
            while (Port <= HubExtension->HubDescriptor->bNumberOfPorts);
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
        ExFreePool(HubExtension->HubDescriptor);
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
        ExFreePool(HubExtension->SCEBitmap);
        HubExtension->SCEBitmap = NULL;
    }

    if (HubExtension->HubConfigDescriptor)
    {
        ExFreePool(HubExtension->HubConfigDescriptor);
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
        DPRINT1("USBH_FdoStartDevice: FIXME. USBH_SyncGetRootHubPdo return - %p\n",
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
    PUSBHUB_PORT_DATA PortData;
    PDEVICE_OBJECT PdoDevice;
    PUSBHUB_PORT_PDO_EXTENSION PdoExtension;
    NTSTATUS NtStatus;
    LONG SerialNumber;
    LONG DeviceHandle;
    USB_PORT_STATUS UsbPortStatus;
    PLIST_ENTRY Entry;

    DPRINT("USBH_FdoQueryBusRelations: HubFlags - %p\n", HubExtension->HubFlags);

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
        DPRINT("USBH_FdoQueryBusRelations: Skip enumeration\n");
        goto RelationsWorker;
    }

    InterlockedIncrement(&HubExtension->PendingRequestCount);

    KeWaitForSingleObject(&HubExtension->ResetDeviceSemaphore,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    NumberPorts = HubExtension->HubDescriptor->bNumberOfPorts;
    DPRINT("USBH_FdoQueryBusRelations: NumberPorts - %x\n", NumberPorts);

    DeviceRelations = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(ULONG) + NumberPorts * sizeof(PDEVICE_OBJECT),
                                            USB_HUB_TAG);

    if (!DeviceRelations)
    {
        HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_NOT_ENUMERATED;//0xFF7FFFFF

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

    RtlZeroMemory(DeviceRelations, sizeof(ULONG) + NumberPorts * sizeof(PDEVICE_OBJECT));

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

    PortData = HubExtension->PortData;
    Port = 1;

    while (Port <= NumberPorts)
    {
        DPRINT("USBH_FdoQueryBusRelations: Port - %x, ConnectStatus - %x\n",
               Port,
               PortData->PortStatus.UsbPortStatus.ConnectStatus);

        if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_FAILED)
        {
            goto NextPort;
        }

        Status = USBH_SyncGetPortStatus(HubExtension,
                                        Port,
                                        &PortData->PortStatus,
                                        sizeof(USBHUB_PORT_STATUS));

        if (!NT_SUCCESS(Status))
        {
            HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_FAILED;
            DeviceRelations->Count = 0;
            goto EnumStart;
        }

        PdoDevice = PortData->DeviceObject;

        if (PortData->DeviceObject)
        {
            PdoExtension = (PUSBHUB_PORT_PDO_EXTENSION)PdoDevice->DeviceExtension;

            if (PdoExtension->PortPdoFlags & USBHUB_PDO_FLAG_OVERCURRENT_PORT)
            {
                PortData->PortStatus.UsbPortStatus.ConnectStatus = 1;
            }
        }

        if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_FAILED)
        {
            DPRINT1("USBH_FdoQueryBusRelations: DbgBreakPoint() \n");
            DbgBreakPoint();
        }

        if (!PortData->PortStatus.UsbPortStatus.ConnectStatus)
        {
            if (PdoDevice)
            {
                PdoExtension = (PUSBHUB_PORT_PDO_EXTENSION)PdoDevice->DeviceExtension;

                PdoExtension->PortPdoFlags |= USBHUB_PDO_FLAG_DELETE_PENDING;
                PdoExtension->EnumFlags &= ~USBHUB_ENUM_FLAG_DEVICE_PRESENT;

                SerialNumber = InterlockedExchange((PLONG)&PdoExtension->SerialNumber,
                                                   0);

                if (SerialNumber)
                {
                    ExFreePool((PVOID)SerialNumber);
                }

                DeviceHandle = InterlockedExchange((PLONG)&PdoExtension->DeviceHandle,
                                                   0);

                if (DeviceHandle)
                {
                    USBD_RemoveDeviceEx(HubExtension,
                                        (PUSB_DEVICE_HANDLE)DeviceHandle,
                                        0);

                    USBH_SyncDisablePort(HubExtension, Port);
                }
            }

            PortData->DeviceObject = NULL;
            PortData->ConnectionStatus = NoDeviceConnected;
            goto NextPort;
        }

        if (PdoDevice)
        {
            ObReferenceObject(PdoDevice);

            PdoDevice->Flags |= DO_POWER_PAGABLE;
            PdoDevice->Flags &= ~DO_DEVICE_INITIALIZING;

            DeviceRelations->Objects[DeviceRelations->Count++] = PdoDevice;

            PdoExtension = (PUSBHUB_PORT_PDO_EXTENSION)PdoDevice->DeviceExtension;
            PdoExtension->PortPdoFlags &= ~USBHUB_PDO_FLAG_POWER_D1_OR_D2;

            goto NextPort;
        }

        USBH_Wait(100);

        NtStatus = USBH_SyncResetPort(HubExtension, Port);

        if (!NT_SUCCESS(NtStatus))
        {
            if (HubExtension->HubFlags & USBHUB_FDO_FLAG_USB20_HUB)
            {
                PortData->DeviceObject = NULL;
                PortData->ConnectionStatus = NoDeviceConnected;
                goto NextPort;
            }
        }
        else
        {
            NtStatus = USBH_SyncGetPortStatus(HubExtension,
                                              Port,
                                              &PortData->PortStatus,
                                              sizeof(USBHUB_PORT_STATUS));

            UsbPortStatus = PortData->PortStatus.UsbPortStatus;

            if (NT_SUCCESS(NtStatus))
            {
                ULONG ix = 0;

                for (NtStatus = USBH_CreateDevice(HubExtension, Port, UsbPortStatus, 0);
                     NtStatus < 0;
                     NtStatus = USBH_CreateDevice(HubExtension, Port, UsbPortStatus, ix))
                {
                    ++ix;

                    USBH_Wait(500);

                    if (ix >= 3)
                    {
                        goto AddObject;
                    }

                    if (PortData->DeviceObject)
                    {
                        IoDeleteDevice(PortData->DeviceObject);
                        PortData->DeviceObject = NULL;
                        PortData->ConnectionStatus = NoDeviceConnected;
                    }

                    USBH_SyncResetPort(HubExtension, Port);
                }

                if (PortData->DeviceObject)
                {
                    PdoExtension = (PUSBHUB_PORT_PDO_EXTENSION)PortData->DeviceObject->DeviceExtension;

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
                }

          AddObject:

                if (NtStatus >= 0)
                {
                    ObReferenceObject(PortData->DeviceObject);

                    DeviceRelations->Objects[DeviceRelations->Count] = PortData->DeviceObject;

                    PortData->DeviceObject->Flags |= DO_POWER_PAGABLE;
                    PortData->DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

                    ++DeviceRelations->Count;

                    PortData->ConnectionStatus = DeviceConnected;

                    goto NextPort;
                }
            }
        }

        PortData->ConnectionStatus = DeviceFailedEnumeration;

        if ((USBH_SyncDisablePort(HubExtension, Port) & 0xC0000000) == 0xC0000000)
        {
            HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_FAILED;
        }

        if (PortData->DeviceObject)
        {
            ObReferenceObject(PortData->DeviceObject);
            PortData->DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
            DeviceRelations->Objects[DeviceRelations->Count++] = PortData->DeviceObject;
        }

  NextPort:
        ++Port;
        ++PortData;
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
        Irp->IoStatus.Information = 0;

        if (DeviceRelations)
        {
            ExFreePool(DeviceRelations);
        }

        USBH_CompleteIrp(Irp, Status);

        return Status;
    }

    KeAcquireSpinLock(&HubExtension->RelationsWorkerSpinLock, &OldIrql);

    if (DeviceRelations && DeviceRelations->Count)
    {
        Port = 0;

        do
        {
            PdoDevice = DeviceRelations->Objects[Port];
            Entry = HubExtension->PdoList.Flink;

            if (Entry == &HubExtension->PdoList)
            {
                PdoExt(PdoDevice)->EnumFlags |= USBHUB_ENUM_FLAG_DEVICE_PRESENT;
            }
            else
            {
                PUSBHUB_PORT_PDO_EXTENSION pdoExtension;

                while (TRUE)
                {
                    pdoExtension = CONTAINING_RECORD(Entry,
                                                     USBHUB_PORT_PDO_EXTENSION,
                                                     PortLink);

                    if (pdoExtension == (PUSBHUB_PORT_PDO_EXTENSION)PdoDevice->DeviceExtension)
                    {
                        break;
                    }

                    Entry = Entry->Flink;

                    if (Entry == &HubExtension->PdoList)
                    {
                        PdoExt(PdoDevice)->EnumFlags |= USBHUB_ENUM_FLAG_DEVICE_PRESENT;
                        goto PortNext;
                    }
                }

                PdoExt(PdoDevice)->EnumFlags |= USBHUB_ENUM_FLAG_GHOST_DEVICE;
            }

    PortNext:
            ++Port;
        }
        while (Port < DeviceRelations->Count);

        Port = 0;

        if (DeviceRelations->Count)
        {
            do
            {
                PdoDevice = DeviceRelations->Objects[Port];

                ++Port;

                if (PdoExt(PdoDevice)->EnumFlags & USBHUB_ENUM_FLAG_GHOST_DEVICE)
                {
                    while (Port < DeviceRelations->Count)
                    {
                        DeviceRelations->Objects[Port-1] = DeviceRelations->Objects[Port];
                        ++Port;
                    }

                    ObDereferenceObject(PdoDevice);

                    --DeviceRelations->Count;

                    if (PdoExt(PdoDevice)->EnumFlags & USBHUB_ENUM_FLAG_DEVICE_PRESENT)
                    {
                        PdoExt(PdoDevice)->EnumFlags &= ~USBHUB_ENUM_FLAG_GHOST_DEVICE;
                    }
                }
            }
            while (Port < DeviceRelations->Count);
        }
    }

    Irp->IoStatus.Information = (ULONG)DeviceRelations;

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

    while (TRUE)
    {
        if (IsListEmpty(&GhostPdoList))
        {
            break;
        }

        Entry = RemoveHeadList(&GhostPdoList);

        PdoExtension = CONTAINING_RECORD(Entry,
                                         USBHUB_PORT_PDO_EXTENSION,
                                         PortLink);

        IoDeleteDevice(PdoExtension->Common.SelfDevice);
    }

    Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
    return Status;
}

NTSTATUS
NTAPI
USBH_FdoStopDevice(IN PUSBHUB_FDO_EXTENSION HubExtension,
                   IN PIRP Irp)
{
    DPRINT1("USBH_FdoStopDevice: UNIMPLEMENTED. FIXME. \n");
    DbgBreakPoint();
    return 0;
}

NTSTATUS
NTAPI
USBH_FdoRemoveDevice(IN PUSBHUB_FDO_EXTENSION HubExtension,
                     IN PIRP Irp)
{
    PUSB_HUB_DESCRIPTOR HubDescriptor;
    PUSBHUB_PORT_DATA PortData;
    USHORT NumPorts;
    PDEVICE_OBJECT PortDevice;
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    NTSTATUS Status;

    DPRINT("USBH_FdoRemoveDevice: HubExtension - %p\n", HubExtension);

    PortData = HubExtension->PortData;

    if (PortData)
    {
        HubDescriptor = HubExtension->HubDescriptor;

        if (HubDescriptor)
        {
            if (HubDescriptor->bNumberOfPorts)
            {
                NumPorts = HubDescriptor->bNumberOfPorts;

                do
                {
                   PortDevice = PortData->DeviceObject;

                   if (PortDevice)
                    {
                        PortExtension = (PUSBHUB_PORT_PDO_EXTENSION)PortDevice->DeviceExtension;

                        PortData->PortStatus.AsULONG = 0;
                        PortData->DeviceObject = NULL;

                        PortExtension->EnumFlags &= ~USBHUB_ENUM_FLAG_DEVICE_PRESENT;

                        USBH_PdoRemoveDevice(PortExtension, HubExtension);
                    }

                    ++PortData;
                    --NumPorts;
                }
                while (NumPorts);
            }
        }
    }

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STARTED)
    {
        USBH_FdoCleanup(HubExtension);
    }

    if (HubExtension->PortData)
    {
        ExFreePool(HubExtension->PortData);
        HubExtension->PortData = NULL;

    }

    DPRINT1("USBH_FdoRemoveDevice: call IoWMIRegistrationControl UNIMPLEMENTED. FIXME. \n");

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

    if (!HubExtension->HubDescriptor->bNumberOfPorts)
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
    ULONG Index = 0;
    WCHAR Buffer[200];
    PVOID Id = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    IdType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryId.IdType;

    RtlZeroMemory(Buffer, 200 * sizeof(WCHAR));

    switch (IdType)
    {
        case BusQueryDeviceID:
            DPRINT("USBH_PdoQueryId: BusQueryDeviceID\n");

            if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_INIT_PORT_FAILED)
            {
                Index = swprintf(Buffer, L"USB\\Vid_0000&Pid0000") + 2;
            }
            else
            {
                Index = swprintf(Buffer,
                                 L"USB\\Vid_%04x&Pid_%04x",
                                 PortExtension->DeviceDescriptor.idVendor,
                                 PortExtension->DeviceDescriptor.idProduct) + 2;
            }

            Id = ExAllocatePool(PagedPool, Index * sizeof(WCHAR));

            if (!Id)
            {
                break;
            }

            RtlZeroMemory(Id, Index * sizeof(WCHAR));
            RtlCopyMemory(Id, Buffer, (Index-2) * sizeof(WCHAR));

            //DPRINT("USBH_PdoQueryId: BusQueryDeviceID - %S\n", Id);
            break;

        case BusQueryHardwareIDs:
            DPRINT("USBH_PdoQueryId: BusQueryHardwareIDs\n");
            if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_INIT_PORT_FAILED)
            {
                Index = swprintf(Buffer, L"USB\\UNKNOWN") + 2;
            }
            else
            {
                Index += swprintf(&Buffer[Index],
                                  L"USB\\Vid_%04x&Pid_%04x&Rev_%04x",
                                  PortExtension->DeviceDescriptor.idVendor,
                                  PortExtension->DeviceDescriptor.idProduct,
                                  PortExtension->DeviceDescriptor.bcdDevice) + 1;

                Index += swprintf(&Buffer[Index],
                                  L"USB\\Vid_%04x&Pid_%04x",
                                  PortExtension->DeviceDescriptor.idVendor,
                                  PortExtension->DeviceDescriptor.idProduct) + 2;
            }

            Id = ExAllocatePool(PagedPool, Index * sizeof(WCHAR));

            if (!Id)
            {
                break;
            }

            RtlZeroMemory(Id, Index * sizeof(WCHAR));
            RtlCopyMemory(Id, Buffer, (Index-2) * sizeof(WCHAR));

            break;

        case BusQueryCompatibleIDs:
            DPRINT("USBH_PdoQueryId: BusQueryCompatibleIDs\n");
            if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_INIT_PORT_FAILED)
            {
                Index = swprintf(Buffer, L"USB\\UNKNOWN") + 2;
            }
            else if (PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_MULTI_INTERFACE)
            {
                Index += swprintf(&Buffer[Index],
                                  L"USB\\DevClass_%02x&SubClass_%02x&Prot_%02x",
                                  PortExtension->InterfaceDescriptor.bInterfaceClass,
                                  PortExtension->InterfaceDescriptor.bInterfaceSubClass,
                                  PortExtension->InterfaceDescriptor.bInterfaceProtocol) + 1;

                Index += swprintf(&Buffer[Index],
                                  L"USB\\DevClass_%02x&SubClass_%02x",
                                  PortExtension->InterfaceDescriptor.bInterfaceClass,
                                  PortExtension->InterfaceDescriptor.bInterfaceSubClass) + 1;

                Index += swprintf(&Buffer[Index],
                                  L"USB\\DevClass_%02x",
                                  PortExtension->InterfaceDescriptor.bInterfaceClass) + 1;

                Index += swprintf(&Buffer[Index], L"USB\\COMPOSITE") + 2;
            }
            else
            {
                Index += swprintf(&Buffer[Index],
                                  L"USB\\Class_%02x&SubClass_%02x&Prot_%02x",
                                  PortExtension->InterfaceDescriptor.bInterfaceClass,
                                  PortExtension->InterfaceDescriptor.bInterfaceSubClass,
                                  PortExtension->InterfaceDescriptor.bInterfaceProtocol) + 1;

                Index += swprintf(&Buffer[Index],
                                  L"USB\\Class_%02x&SubClass_%02x",
                                  PortExtension->InterfaceDescriptor.bInterfaceClass,
                                  PortExtension->InterfaceDescriptor.bInterfaceSubClass) + 1;

                Index += swprintf(&Buffer[Index],
                                  L"USB\\Class_%02x",
                                  PortExtension->InterfaceDescriptor.bInterfaceClass) + 2;
            }

            Id = ExAllocatePool(PagedPool, Index * sizeof(WCHAR));

            if (!Id)
            {
                break;
            }

            RtlZeroMemory(Id, Index * sizeof(WCHAR));
            RtlCopyMemory(Id, Buffer, (Index-2) * sizeof(WCHAR));

            break;

        case BusQueryInstanceID:
            DPRINT("USBH_PdoQueryId: BusQueryInstanceID\n");

            if (PortExtension->SerialNumber)
            {
                Id = ExAllocatePoolWithTag(PagedPool,
                                           PortExtension->SN_DescriptorLength + 2,
                                           USB_HUB_TAG);

                if (Id)
                {
                    RtlZeroMemory(Id, PortExtension->SN_DescriptorLength + 2);

                    RtlCopyMemory(Id,
                                  PortExtension->SerialNumber,
                                  PortExtension->SN_DescriptorLength - 2);
                }
            }
            else
            {
                 Id = ExAllocatePoolWithTag(PagedPool,
                                            4 * sizeof(WCHAR) + 2,
                                            USB_HUB_TAG);

                 if (Id)
                 {
                     RtlZeroMemory(Id, 4 * sizeof(WCHAR) + 2);

                     RtlCopyMemory(Id,
                                   &PortExtension->InstanceID,
                                   4 * sizeof(WCHAR));
                 }
            }

            break;

        default:
            DPRINT1("USBH_PdoQueryId: unknown query id type 0x%lx\n", IdType);
            return Irp->IoStatus.Status;
    }

    Irp->IoStatus.Information = (ULONG)Id;

    if (!Id)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        USBPORT_DumpingIDs(Id);
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
    PUSB_STRING_DESCRIPTOR Descriptor;
    PVOID DeviceText;
    UCHAR iProduct = 0;
    NTSTATUS Status;
    ULONG NumSymbols;
    ULONG Length;

    DPRINT("USBH_PdoQueryDeviceText ... \n");

    DeviceObject = PortExtension->Common.SelfDevice;
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceTextType = IoStack->Parameters.QueryDeviceText.DeviceTextType;

    if (DeviceTextType != DeviceTextDescription &&
        DeviceTextType != DeviceTextLocationInformation)
    {
        return Irp->IoStatus.Status;
    }

    LanguageId = IoStack->Parameters.QueryDeviceText.LocaleId;

    if (!LanguageId)
    {
        LanguageId = 0x0409;
    }

    iProduct = PortExtension->DeviceDescriptor.iProduct;

    if (PortExtension->DeviceHandle && iProduct &&
        !PortExtension->IgnoringHwSerial &&
        !(PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_INIT_PORT_FAILED))
    {
        Descriptor = ExAllocatePoolWithTag(NonPagedPool,
                                           0xFF,
                                           USB_HUB_TAG);

        if (Descriptor)
        {
            RtlZeroMemory(Descriptor, 0xFF);

            for (Status = USBH_CheckDeviceLanguage(DeviceObject, LanguageId);
                 ; 
                 Status = USBH_CheckDeviceLanguage(DeviceObject, 0x0409))
            {
                if (NT_SUCCESS(Status))
                {
                    Status = USBH_SyncGetStringDescriptor(DeviceObject,
                                                          iProduct,
                                                          LanguageId,
                                                          Descriptor,
                                                          0xFF,
                                                          0,
                                                          1);

                    if (NT_SUCCESS(Status))
                    {
                        break;
                    }
                }

                if (LanguageId == 0x0409)
                {
                    goto Exit;
                }

                LanguageId = 0x0409;
            }

            if (Descriptor->bLength <= 2)
            {
                Status = STATUS_UNSUCCESSFUL;
            }

            if (NT_SUCCESS(Status))
            {
                DeviceText = ExAllocatePoolWithTag(PagedPool,
                                                   Descriptor->bLength,
                                                   USB_HUB_TAG);

                if (DeviceText)
                {
                    RtlZeroMemory(DeviceText, Descriptor->bLength);

                    RtlCopyMemory(DeviceText,
                                  Descriptor->bString,
                                  Descriptor->bLength - 2);

                    Irp->IoStatus.Information = (ULONG)DeviceText;

                    DPRINT("USBH_PdoQueryDeviceText: Descriptor->bString - %S\n",
                           DeviceText);
                }
                else
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

        Exit:

            ExFreePool(Descriptor);

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

    if (GenericUSBDeviceString)
    {
        NumSymbols = 0;

        if (*(PWCHAR)GenericUSBDeviceString)
        {
            do
            {
                ++NumSymbols;
            }
            while (*((PWCHAR)GenericUSBDeviceString + NumSymbols));
        }

        Length = NumSymbols * sizeof(WCHAR) + 2;

        DeviceText = ExAllocatePoolWithTag(PagedPool, Length, USB_HUB_TAG);

        if (DeviceText)
        {
            RtlZeroMemory(DeviceText, Length);

            RtlCopyMemory(DeviceText,
                          GenericUSBDeviceString,
                          NumSymbols * sizeof(WCHAR));

            Status = STATUS_SUCCESS;

            Irp->IoStatus.Information = (ULONG)DeviceText;
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return Status;
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

        if (NameBuffer && NameBuffer != (PVOID)-2)
        {
            Status = IoSetDeviceInterfaceState(&PortExtension->SymbolicLinkName,
                                               FALSE);

            ExFreePool(PortExtension->SymbolicLinkName.Buffer);

            PortExtension->SymbolicLinkName.Buffer = (PVOID)-2;
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
    ULONG ix = 0;

    DPRINT("USBH_RestoreDevice ... \n");

    HubExtension = PortExtension->HubExtension;

    if (!HubExtension)
    {
        Status = STATUS_UNSUCCESSFUL;
        return Status;
    }

    PortData = &HubExtension->PortData[PortExtension->PortNumber - 1];

    if ( PortExtension->Common.SelfDevice == PortData->DeviceObject )
    {
        Status = STATUS_UNSUCCESSFUL;
        return Status;
    }

    Status = USBH_SyncGetPortStatus(HubExtension,
                                PortExtension->PortNumber,
                                &PortData->PortStatus,
                                sizeof(USBHUB_PORT_STATUS));

    if (NT_SUCCESS(Status))
    {
        do
        {
            Status = USBH_ResetDevice((PUSBHUB_FDO_EXTENSION)HubExtension,
                                      PortExtension->PortNumber,
                                      IsKeepDeviceData,
                                      ix++);

            if (NT_SUCCESS(Status))
            {
                break;
            }

            if (Status == STATUS_NO_SUCH_DEVICE)
            {
                break;
            }

            USBH_Wait(1000);
        }
        while (ix < 3);
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

    DPRINT1("USBH_PdoStartDevice: call IoWMIRegistrationControl UNIMPLEMENTED. FIXME. \n");
    //IoWMIRegistrationControl()

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
    LONG DeviceHandle;
    PDEVICE_OBJECT Pdo;
    LONG SerialNumber;
    USHORT Port;
    KIRQL Irql;

    DPRINT("USBH_PdoRemoveDevice ... \n");

    PortDevice = PortExtension->Common.SelfDevice;
    PortExtension->HubExtension = NULL;

    Port = PortExtension->PortNumber;

    if (HubExtension)
    {
        if (HubExtension->CurrentPowerState.DeviceState != PowerDeviceD0)
        {
            if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STARTED)
            {
                USBH_HubSetD0(HubExtension);
            }
        }
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

    DeviceHandle = InterlockedExchange((PLONG)&PortExtension->DeviceHandle, 0);

    if (DeviceHandle)
    {
        Status = USBD_RemoveDeviceEx(HubExtension,
                                     (PUSB_DEVICE_HANDLE)DeviceHandle,
                                     0);

        if (HubExtension->PortData)
        {
            if (HubExtension->PortData[Port - 1].DeviceObject == PortDevice)
            {
                USBH_SyncDisablePort(HubExtension, Port);
            }
        }
    }

    if (NT_SUCCESS(Status))
    {
        PortExtension->PortPdoFlags &= ~USBHUB_PDO_FLAG_DEVICE_STARTED;

        if (HubExtension)
        {
            if (HubExtension->PortData)
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
        }

        if (!(PortExtension->EnumFlags & USBHUB_ENUM_FLAG_DEVICE_PRESENT))
        {
            if (!(PortExtension->PortPdoFlags & USBHUB_PDO_FLAG_NOT_CONNECTED))
            {
                PortExtension->PortPdoFlags |= USBHUB_PDO_FLAG_NOT_CONNECTED;

                SerialNumber = InterlockedExchange((PLONG)&PortExtension->SerialNumber,
                                                   0);

                if (SerialNumber)
                {
                    ExFreePool((PVOID)SerialNumber);
                }

                DPRINT1("USBH_PdoRemoveDevice: call IoWMIRegistrationControl UNIMPLEMENTED. FIXME. \n");

                if (HubExtension)
                {
                    USBHUB_FlushAllTransfers(HubExtension);
                }

                IoDeleteDevice(PortDevice);
            }
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
    DPRINT1("USBH_PdoStopDevice: UNIMPLEMENTED. FIXME. \n");
    DbgBreakPoint();
    return 0;
}

NTSTATUS
NTAPI
USBH_FdoPnP(IN PUSBHUB_FDO_EXTENSION HubExtension,
            IN PIRP Irp,
            IN UCHAR Minor)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    BOOLEAN IsCheckIdle = FALSE;

    DPRINT("USBH_FdoPnP: HubExtension - %p, Irp - %p, Minor - %x\n",
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

    DPRINT("USBH_FdoPnP: HubFlags - %p\n", HubExtension->HubFlags);

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_GOING_IDLE)
    {
        HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_SUSPENDED;
    }

    if ((HubExtension->CurrentPowerState.DeviceState != PowerDeviceD0) &&
        (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STOPPED) &&
        (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STARTED) &&
        (Minor != IRP_MN_QUERY_DEVICE_RELATIONS || Minor != IRP_MN_STOP_DEVICE))
    {
        DPRINT("USBH_FdoPnP: IsCheckIdle - TRUE\n");
        IsCheckIdle = TRUE;
        USBH_HubSetD0(HubExtension);
    }

    switch (Minor)
    {
        case IRP_MN_START_DEVICE: // 0
            DPRINT("FDO IRP_MN_START_DEVICE\n");
            IsCheckIdle = FALSE;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_FdoStartDevice(HubExtension, Irp);
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE: // 1
            DPRINT("FDO IRP_MN_QUERY_REMOVE_DEVICE\n");
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE: // 2
            DPRINT("FDO IRP_MN_REMOVE_DEVICE\n");
            IsCheckIdle = FALSE;
            HubExtension->HubFlags |= USBHUB_FDO_FLAG_DEVICE_REMOVED;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_FdoRemoveDevice(HubExtension, Irp);
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE: // 3
            DPRINT("FDO IRP_MN_CANCEL_REMOVE_DEVICE\n");
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_STOP_DEVICE: // 4
            DPRINT("FDO IRP_MN_STOP_DEVICE\n");
            IsCheckIdle = FALSE;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_FdoStopDevice(HubExtension, Irp);
            break;

        case IRP_MN_QUERY_STOP_DEVICE: // 5
            DPRINT("FDO IRP_MN_QUERY_STOP_DEVICE\n");
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_CANCEL_STOP_DEVICE: // 6
            DPRINT("FDO IRP_MN_CANCEL_STOP_DEVICE\n");
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS: // 7
            DPRINT("FDO IRP_MN_QUERY_DEVICE_RELATIONS\n");

            IoStack = IoGetCurrentIrpStackLocation(Irp);

            if (IoStack->Parameters.QueryDeviceRelations.Type != BusRelations)
            {
                Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
                break;
            }

            HubExtension->HubFlags |= USBHUB_FDO_FLAG_HUB_BUSY;

            IsCheckIdle = TRUE;
            DPRINT("USBH_FdoPnP: IsCheckIdle - TRUE\n");

            Status = USBH_FdoQueryBusRelations(HubExtension, Irp);

            HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_HUB_BUSY;
            break;

        case IRP_MN_QUERY_INTERFACE: // 8
            DPRINT("FDO IRP_MN_QUERY_INTERFACE\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_CAPABILITIES: // 9
            DPRINT("FDO IRP_MN_QUERY_CAPABILITIES\n");
            IoCopyCurrentIrpStackLocationToNext(Irp);

            IoSetCompletionRoutine(Irp,
                                   USBH_QueryCapsComplete,
                                   HubExtension,
                                   TRUE,
                                   FALSE,
                                   FALSE);

            Status = IoCallDriver(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_RESOURCES: // 10
            DPRINT("FDO IRP_MN_QUERY_RESOURCES\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: // 11
            DPRINT("FDO IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_DEVICE_TEXT: // 12
            DPRINT("FDO IRP_MN_QUERY_DEVICE_TEXT\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: // 13
            DPRINT("FDO IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_READ_CONFIG: // 15
            DPRINT("FDO IRP_MN_READ_CONFIG\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_WRITE_CONFIG: // 16
            DPRINT("FDO IRP_MN_WRITE_CONFIG\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_EJECT: // 17
            DPRINT("FDO IRP_MN_EJECT\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_SET_LOCK: // 18
            DPRINT("FDO IRP_MN_SET_LOCK\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_ID: // 19
            DPRINT("FDO IRP_MN_QUERY_ID\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE: // 20
            DPRINT("FDO IRP_MN_QUERY_PNP_DEVICE_STATE\n");

            if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_FAILED)
            {
                Irp->IoStatus.Information |= PNP_DEVICE_FAILED;
            }

            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_QUERY_BUS_INFORMATION: // 21
            DPRINT("FDO IRP_MN_QUERY_BUS_INFORMATION\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION: // 22
            DPRINT("FDO IRP_MN_DEVICE_USAGE_NOTIFICATION\n");
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        case IRP_MN_SURPRISE_REMOVAL: // 23
            DPRINT("FDO IRP_MN_SURPRISE_REMOVAL\n");
            USBH_FdoSurpriseRemoveDevice(HubExtension, Irp);
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;

        default:
            DPRINT("FDO unknown IRP_MN_???\n");
            Status = USBH_PassIrp(HubExtension->LowerDevice, Irp);
            break;
    }

    KeReleaseSemaphore(&HubExtension->IdleSemaphore,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    if (IsCheckIdle)
    {
        DPRINT("USBH_FdoPnP: call USBH_CheckIdleDeferred()\n");
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

    DPRINT("USBH_PdoPnP: PortExtension - %p, Irp - %p, Minor - %d\n",
           PortExtension,
           Irp,
           Minor);

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    *IsCompleteIrp = TRUE;

    switch (Minor)
    {
        case IRP_MN_START_DEVICE: // 0
            DPRINT("PDO IRP_MN_START_DEVICE\n");
            return USBH_PdoStartDevice(PortExtension, Irp);

        case IRP_MN_QUERY_REMOVE_DEVICE: // 1
            DPRINT("PDO IRP_MN_QUERY_REMOVE_DEVICE\n");
            return STATUS_SUCCESS;

        case IRP_MN_REMOVE_DEVICE: // 2
            DPRINT("PDO IRP_MN_REMOVE_DEVICE\n");
            return USBH_PdoRemoveDevice(PortExtension, PortExtension->HubExtension);

        case IRP_MN_CANCEL_REMOVE_DEVICE: // 3
            DPRINT("PDO IRP_MN_CANCEL_REMOVE_DEVICE\n");
            return STATUS_SUCCESS;

        case IRP_MN_STOP_DEVICE: // 4
            DPRINT("PDO IRP_MN_STOP_DEVICE\n");
            return USBH_PdoStopDevice(PortExtension, Irp);

        case IRP_MN_QUERY_STOP_DEVICE: // 5
            DPRINT("PDO IRP_MN_QUERY_STOP_DEVICE\n");
            return STATUS_SUCCESS;

        case IRP_MN_CANCEL_STOP_DEVICE: // 6
            DPRINT("PDO IRP_MN_CANCEL_STOP_DEVICE\n");
            return STATUS_SUCCESS;

        case IRP_MN_QUERY_DEVICE_RELATIONS: // 7
            DPRINT("PDO IRP_MN_QUERY_DEVICE_RELATIONS\n");

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

        case IRP_MN_QUERY_INTERFACE: // 8
            DPRINT("PDO IRP_MN_QUERY_INTERFACE\n");

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

        case IRP_MN_QUERY_CAPABILITIES: // 9
            DPRINT("PDO IRP_MN_QUERY_CAPABILITIES\n");

            DeviceCapabilities = IoStack->Parameters.DeviceCapabilities.Capabilities;

            Size = DeviceCapabilities->Size;
            Version = DeviceCapabilities->Version;

            RtlCopyMemory(DeviceCapabilities,
                          &PortExtension->Capabilities,
                          sizeof(DEVICE_CAPABILITIES));

            DeviceCapabilities->Size = Size;
            DeviceCapabilities->Version = Version;

            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_RESOURCES: // 10
            DPRINT("PDO IRP_MN_QUERY_RESOURCES\n");
            Status = Irp->IoStatus.Status;
            break;

        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: // 11
            DPRINT("PDO IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
            PortExtension->PortPdoFlags |= 0x04000000;

            /* FIXME HKEY_LOCAL_MACHINE\SYSTEM\ControlSetXXX\Enum\USB\
               Vid_????&Pid_????\????????????\Device Parameters\
               if (ExtPropDescSemaphore)
            */

            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_DEVICE_TEXT: // 12
            DPRINT("PDO IRP_MN_QUERY_DEVICE_TEXT\n");
            return USBH_PdoQueryDeviceText(PortExtension, Irp);

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: // 13
            DPRINT("PDO IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
            Status = Irp->IoStatus.Status;
            break;

        case IRP_MN_READ_CONFIG: // 15
            DPRINT("PDO IRP_MN_READ_CONFIG\n");
            DbgBreakPoint();
            Status = Irp->IoStatus.Status;
            break;

        case IRP_MN_WRITE_CONFIG: // 16
            DPRINT("PDO IRP_MN_WRITE_CONFIG\n");
            DbgBreakPoint();
            Status = Irp->IoStatus.Status;
            break;

        case IRP_MN_EJECT: // 17
            DPRINT("PDO IRP_MN_EJECT\n");
            DbgBreakPoint();
            Status = Irp->IoStatus.Status;
            break;

        case IRP_MN_SET_LOCK: // 18
            DPRINT("PDO IRP_MN_SET_LOCK\n");
            DbgBreakPoint();
            Status = Irp->IoStatus.Status;
            break;

        case IRP_MN_QUERY_ID: // 19
            DPRINT("PDO IRP_MN_QUERY_ID\n");
            return USBH_PdoQueryId(PortExtension, Irp);

        case IRP_MN_QUERY_PNP_DEVICE_STATE: // 20
            DPRINT("PDO IRP_MN_QUERY_PNP_DEVICE_STATE\n");
            if (PortExtension->PortPdoFlags & (USBHUB_PDO_FLAG_INSUFFICIENT_PWR |
                                               USBHUB_PDO_FLAG_OVERCURRENT_PORT |
                                               USBHUB_PDO_FLAG_PORT_RESTORE_FAIL |
                                               USBHUB_PDO_FLAG_INIT_PORT_FAILED))
            {
                Irp->IoStatus.Information |= PNP_DEVICE_FAILED;
            }

            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_BUS_INFORMATION: // 21
            DPRINT("PDO IRP_MN_QUERY_BUS_INFORMATION\n");

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

        case IRP_MN_DEVICE_USAGE_NOTIFICATION: // 22
            DPRINT("PDO IRP_MN_DEVICE_USAGE_NOTIFICATION\n");
            DbgBreakPoint();
            Status = Irp->IoStatus.Status;
            break;

        case IRP_MN_SURPRISE_REMOVAL: // 23
            DPRINT("PDO IRP_MN_SURPRISE_REMOVAL\n");
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
            DPRINT("PDO unknown IRP_MN_???\n");
            Status = Irp->IoStatus.Status;
            break;
    }

    return Status;
}
