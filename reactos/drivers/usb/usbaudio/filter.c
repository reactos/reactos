/*
* PROJECT:     ReactOS Universal Audio Class Driver
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        drivers/usb/usbaudio/filter.c
* PURPOSE:     USB Audio device driver.
* PROGRAMMERS:
*              Johannes Anderwald (johannes.anderwald@reactos.org)
*/

#include "usbaudio.h"

NTSTATUS
BuildUSBAudioFilterTopology(
    PKSDEVICE Device)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
USBAudioPinBuildDescriptors(
    PKSDEVICE Device,
    PKSPIN_DESCRIPTOR_EX *PinDescriptors,
    PULONG PinDescriptorsCount,
    PULONG PinDescriptorSize)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
USBAudioInitComponentId(
    PKSDEVICE Device,
    IN PKSCOMPONENTID ComponentId)
{
    PDEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = Device->Context;

    INIT_USBAUDIO_MID(&ComponentId->Manufacturer, DeviceExtension->DeviceDescriptor->idVendor);
    INIT_USBAUDIO_PID(&ComponentId->Product, DeviceExtension->DeviceDescriptor->idProduct);

    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
USBAudioCreateFilterContext(
    PKSDEVICE Device)
{
    KSFILTER_DESCRIPTOR FilterDescriptor;
    PKSCOMPONENTID ComponentId;
    NTSTATUS Status;

    /* clear filter descriptor */
    RtlZeroMemory(&FilterDescriptor, sizeof(KSFILTER_DESCRIPTOR));

    /* init filter descriptor*/
    FilterDescriptor.Version = KSFILTER_DESCRIPTOR_VERSION;
    FilterDescriptor.ReferenceGuid = &KSNAME_Filter;

    /* init component id*/
    ComponentId = AllocFunction(sizeof(KSCOMPONENTID));
    if (!ComponentId)
    {
        /* no memory */
        return USBD_STATUS_INSUFFICIENT_RESOURCES;
    }
    Status = USBAudioInitComponentId(Device, ComponentId);
    if (!NT_SUCCESS(Status))
    {
        /* failed*/
        FreeFunction(ComponentId);
        return Status;
    }
    FilterDescriptor.ComponentId = ComponentId;

    /* build pin descriptors */
    Status = USBAudioPinBuildDescriptors(Device, (PKSPIN_DESCRIPTOR_EX *)&FilterDescriptor.PinDescriptors, &FilterDescriptor.PinDescriptorsCount, &FilterDescriptor.PinDescriptorSize);
    if (!NT_SUCCESS(Status))
    {
        /* failed*/
        FreeFunction(ComponentId);
        return Status;
    }

    /* build topology */
    Status = BuildUSBAudioFilterTopology(Device);
    if (!NT_SUCCESS(Status))
    {
        /* failed*/
        FreeFunction(ComponentId);
        return Status;
    }

    return Status;
}


