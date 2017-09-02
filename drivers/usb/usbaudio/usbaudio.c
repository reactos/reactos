/*
* PROJECT:     ReactOS Universal Audio Class Driver
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        drivers/usb/usbaudio/usbaudio.c
* PURPOSE:     USB Audio device driver.
* PROGRAMMERS:
*              Johannes Anderwald (johannes.anderwald@reactos.org)
*/

#include "usbaudio.h"

static KSDEVICE_DISPATCH KsDeviceDispatch = {
    USBAudioAddDevice,
    USBAudioPnPStart,
    NULL,
    USBAudioPnPQueryStop,
    USBAudioPnPCancelStop,
    USBAudioPnPStop,
    USBAudioPnPQueryRemove,
    USBAudioPnPCancelRemove,
    USBAudioPnPRemove,
    USBAudioPnPQueryCapabilities,
    USBAudioPnPSurpriseRemoval,
    USBAudioPnPQueryPower,
    USBAudioPnPSetPower
};

static KSDEVICE_DESCRIPTOR KsDeviceDescriptor = {
    &KsDeviceDispatch,
    0,
    NULL,
    0x100, //KSDEVICE_DESCRIPTOR_VERSION,
    0
};

NTSTATUS
SubmitUrbSync(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb)
{
    PIRP Irp;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    // init event
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    // build irp
    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB,
        DeviceObject,
        NULL,
        0,
        NULL,
        0,
        TRUE,
        &Event,
        &IoStatus);

    if (!Irp)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // get next stack location
    IoStack = IoGetNextIrpStackLocation(Irp);

    // store urb
    IoStack->Parameters.Others.Argument1 = Urb;

    // call driver
    Status = IoCallDriver(DeviceObject, Irp);

    // wait for the request to finish
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    // done
    return Status;
}

NTSTATUS
NTAPI
USBAudioSelectConfiguration(
    IN PKSDEVICE Device,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    PDEVICE_EXTENSION DeviceExtension;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    PUSBD_INTERFACE_LIST_ENTRY InterfaceList;
    PURB Urb;
    NTSTATUS Status;
    ULONG InterfaceDescriptorCount;

    /* alloc item for configuration request */
    InterfaceList = AllocFunction(sizeof(USBD_INTERFACE_LIST_ENTRY) * (ConfigurationDescriptor->bNumInterfaces + 1));
    if (!InterfaceList)
    {
        /* insufficient resources*/
        return USBD_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* grab interface descriptor */
    InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor, ConfigurationDescriptor, -1, -1, USB_DEVICE_CLASS_AUDIO, -1, -1);
    if (!InterfaceDescriptor)
    {
        /* no such interface */
        return STATUS_INVALID_PARAMETER;
    }

    /* lets enumerate the interfaces */
    InterfaceDescriptorCount = 0;
    while (InterfaceDescriptor != NULL)
    {
        if (InterfaceDescriptor->bInterfaceSubClass == 0x01) /* AUDIO_CONTROL*/
        {
            InterfaceList[InterfaceDescriptorCount++].InterfaceDescriptor = InterfaceDescriptor;
        }
        else if (InterfaceDescriptor->bInterfaceSubClass == 0x03) /* MIDI_STREAMING*/
        {
            InterfaceList[InterfaceDescriptorCount++].InterfaceDescriptor = InterfaceDescriptor;
        }

        InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor, (PVOID)((ULONG_PTR)InterfaceDescriptor + InterfaceDescriptor->bLength), -1, -1, USB_DEVICE_CLASS_AUDIO, -1, -1);
    }

    /* build urb */
    Urb = USBD_CreateConfigurationRequestEx(ConfigurationDescriptor, InterfaceList);
    if (!Urb)
    {
        /* no memory */
        FreeFunction(InterfaceList);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* device extension */
    DeviceExtension = Device->Context;

    /* submit configuration urb */
    Status = SubmitUrbSync(DeviceExtension->LowerDevice, Urb);
    if (!NT_SUCCESS(Status))
    {
        /* free resources */
        ExFreePool(Urb);
        FreeFunction(InterfaceList);
        return Status;
    }

    /* store configuration handle */
    DeviceExtension->ConfigurationHandle = Urb->UrbSelectConfiguration.ConfigurationHandle;

    /* alloc interface info */
    DeviceExtension->InterfaceInfo = AllocFunction(Urb->UrbSelectConfiguration.Interface.Length);
    if (DeviceExtension->InterfaceInfo)
    {
        /* copy interface info */
        RtlCopyMemory(DeviceExtension->InterfaceInfo, &Urb->UrbSelectConfiguration.Interface, Urb->UrbSelectConfiguration.Interface.Length);
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBAudioStartDevice(
    IN PKSDEVICE Device)
{
    PURB Urb;
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    ULONG Length;

    /* get device extension */
    DeviceExtension = Device->Context;

    /* allocate urb */
    Urb = AllocFunction(sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
    if (!Urb)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* alloc buffer for device descriptor */
    DeviceDescriptor = AllocFunction(sizeof(USB_DEVICE_DESCRIPTOR));
    if (!DeviceDescriptor)
    {
        /* insufficient resources */
        FreeFunction(Urb);
       return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* build descriptor request */
    UsbBuildGetDescriptorRequest(Urb, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST), USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, DeviceDescriptor, NULL, sizeof(USB_DEVICE_DESCRIPTOR), NULL);

    /* submit urb */
    Status = SubmitUrbSync(DeviceExtension->LowerDevice, Urb);
    if (!NT_SUCCESS(Status))
    {
        /* free resources */
        FreeFunction(Urb);
        FreeFunction(DeviceDescriptor);
        return Status;
    }

    /* now allocate some space for partial configuration descriptor */
    ConfigurationDescriptor = AllocFunction(sizeof(USB_CONFIGURATION_DESCRIPTOR));
    if (!ConfigurationDescriptor)
    {
        /* free resources */
        FreeFunction(Urb);
        FreeFunction(DeviceDescriptor);
        return Status;
    }

    /* build descriptor request */
    UsbBuildGetDescriptorRequest(Urb, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST), USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, ConfigurationDescriptor, NULL, sizeof(USB_CONFIGURATION_DESCRIPTOR), NULL);

    /* submit urb */
    Status = SubmitUrbSync(DeviceExtension->LowerDevice, Urb);
    if (!NT_SUCCESS(Status))
    {
        /* free resources */
        FreeFunction(Urb);
        FreeFunction(DeviceDescriptor);
        FreeFunction(ConfigurationDescriptor);
        return Status;
    }

    /* backup length */
    Length = ConfigurationDescriptor->wTotalLength;

    /* free old descriptor */
    FreeFunction(ConfigurationDescriptor);

    /* now allocate some space for full configuration descriptor */
    ConfigurationDescriptor = AllocFunction(Length);
    if (!ConfigurationDescriptor)
    {
        /* free resources */
        FreeFunction(Urb);
        FreeFunction(DeviceDescriptor);
        return Status;
    }

    /* build descriptor request */
    UsbBuildGetDescriptorRequest(Urb, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST), USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, ConfigurationDescriptor, NULL, Length, NULL);

    /* submit urb */
    Status = SubmitUrbSync(DeviceExtension->LowerDevice, Urb);

    /* free urb */
    FreeFunction(Urb);
    if (!NT_SUCCESS(Status))
    {
        /* free resources */
        FreeFunction(DeviceDescriptor);
        FreeFunction(ConfigurationDescriptor);
        return Status;
    }

    /* lets add to object bag */
    KsAddItemToObjectBag(Device->Bag, DeviceDescriptor, ExFreePool);
    KsAddItemToObjectBag(Device->Bag, ConfigurationDescriptor, ExFreePool);

    Status = USBAudioSelectConfiguration(Device, ConfigurationDescriptor);
    if (NT_SUCCESS(Status))
    {

        DeviceExtension->ConfigurationDescriptor = ConfigurationDescriptor;
        DeviceExtension->DeviceDescriptor = DeviceDescriptor;
    }
    return Status;
}


NTSTATUS
NTAPI
USBAudioAddDevice(
  _In_ PKSDEVICE Device)
{
    /* no op */
    DPRINT1("USBAudioAddDevice\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBAudioPnPStart(
  _In_     PKSDEVICE         Device,
  _In_     PIRP              Irp,
  _In_opt_ PCM_RESOURCE_LIST TranslatedResourceList,
  _In_opt_ PCM_RESOURCE_LIST UntranslatedResourceList)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension;

    if (!Device->Started)
    {
        /* alloc context  */
        DeviceExtension = AllocFunction(sizeof(DEVICE_EXTENSION));
        if (DeviceExtension == NULL)
        {
             /* insufficient resources */
             return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* init context */
        Device->Context = DeviceExtension;
        DeviceExtension->LowerDevice = Device->NextDeviceObject;

        /* add to object bag*/
        KsAddItemToObjectBag(Device->Bag, Device->Context, ExFreePool);

        /* init device*/
        Status = USBAudioStartDevice(Device);
        if (NT_SUCCESS(Status))
        {
            /* TODO retrieve interface */
            Status = USBAudioCreateFilterContext(Device);
        }
    }

    return Status;
}

NTSTATUS
NTAPI
USBAudioPnPQueryStop(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp)
{
    /* no op */
    return STATUS_SUCCESS;
}

VOID
NTAPI
USBAudioPnPCancelStop(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp)
{
    /* no op */
}

VOID
NTAPI
USBAudioPnPStop(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp)
{
    /* TODO: stop device */
	UNIMPLEMENTED;
}

NTSTATUS
NTAPI
USBAudioPnPQueryRemove(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp)
{
    /* no op */
    return STATUS_SUCCESS;
}


VOID
NTAPI
USBAudioPnPCancelRemove(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp)
{
    /* no op */
}

VOID
NTAPI
USBAudioPnPRemove(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp)
{
    /* TODO: stop device */
	UNIMPLEMENTED;
}

NTSTATUS
NTAPI
USBAudioPnPQueryCapabilities(
  _In_    PKSDEVICE            Device,
  _In_    PIRP                 Irp,
  _Inout_ PDEVICE_CAPABILITIES Capabilities)
{
    /* TODO: set caps */
	UNIMPLEMENTED;
	return STATUS_SUCCESS;
}

VOID
NTAPI
USBAudioPnPSurpriseRemoval(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp)
{
    /* TODO: stop streams */
	UNIMPLEMENTED;
}  
  
NTSTATUS
NTAPI
USBAudioPnPQueryPower(
  _In_ PKSDEVICE          Device,
  _In_ PIRP               Irp,
  _In_ DEVICE_POWER_STATE DeviceTo,
  _In_ DEVICE_POWER_STATE DeviceFrom,
  _In_ SYSTEM_POWER_STATE SystemTo,
  _In_ SYSTEM_POWER_STATE SystemFrom,
  _In_ POWER_ACTION       Action)
{
    /* no op */
    return STATUS_SUCCESS;
}

VOID
NTAPI
USBAudioPnPSetPower(
  _In_ PKSDEVICE          Device,
  _In_ PIRP               Irp,
  _In_ DEVICE_POWER_STATE To,
  _In_ DEVICE_POWER_STATE From)
{
    /* TODO: stop streams */
	UNIMPLEMENTED;
}

NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;

    // initialize driver
    Status = KsInitializeDriver(DriverObject, RegistryPath, &KsDeviceDescriptor);
    if (!NT_SUCCESS(Status))
    {
        // failed to initialize driver
        DPRINT1("Failed to initialize driver with %x\n", Status);
        return Status;
    }
    return Status;
}
