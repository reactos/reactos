/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbccgp/descriptor.c
 * PURPOSE:     USB  device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 *              Cameron Gutman
 */

#include "usbccgp.h"

NTSTATUS
USBCCGP_QueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PUSBC_DEVICE_CONFIGURATION_INTERFACE_V1 BusInterface)
{
    KEVENT Event;
    NTSTATUS Status;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    PIO_STACK_LOCATION Stack;

    //
    // sanity checks
    //
    ASSERT(DeviceObject);

    //
    // initialize event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // init interface
    //
    RtlZeroMemory(BusInterface, sizeof(USBC_DEVICE_CONFIGURATION_INTERFACE_V1));
    BusInterface->Version = USBC_DEVICE_CONFIGURATION_INTERFACE_VERSION_1;
    BusInterface->Size = sizeof(USBC_DEVICE_CONFIGURATION_INTERFACE_V1);

    //
    // create irp
    //
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       DeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);

    //
    // was irp built
    //
    if (Irp == NULL)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize request
    //
    Stack=IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = IRP_MJ_PNP;
    Stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    Stack->Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
    Stack->Parameters.QueryInterface.InterfaceType = (LPGUID)&USB_BUS_INTERFACE_USBC_CONFIGURATION_GUID;
    Stack->Parameters.QueryInterface.Version = 2;
    Stack->Parameters.QueryInterface.Interface = (PINTERFACE)&BusInterface;
    Stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    //
    // call driver
    //
    Status= IoCallDriver(DeviceObject, Irp);

    //
    // did operation complete
    //
    if (Status == STATUS_PENDING)
    {
        //
        // wait for completion
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

        //
        // collect status
        //
        Status = IoStatus.Status;
    }

    return Status;
}

NTSTATUS
USBCCGP_CustomEnumWithInterface(
    IN PDEVICE_OBJECT DeviceObject)
{
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    ULONG FunctionDescriptorBufferLength = 0;
    NTSTATUS Status;
    PUSBC_FUNCTION_DESCRIPTOR  FunctionDescriptorBuffer = NULL;

    //
    // get device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    if (FDODeviceExtension->BusInterface.StartDeviceCallback == NULL)
    {
        //
        // not supported
        //
        return STATUS_NOT_SUPPORTED;
    }

    //
    // invoke callback
    //
    Status = FDODeviceExtension->BusInterface.StartDeviceCallback(FDODeviceExtension->DeviceDescriptor, 
                                                                  FDODeviceExtension->ConfigurationDescriptor, 
                                                                  &FunctionDescriptorBuffer,
                                                                  &FunctionDescriptorBufferLength,
                                                                  DeviceObject,
                                                                  FDODeviceExtension->PhysicalDeviceObject);

    DPRINT1("USBCCGP_CustomEnumWithInterface Status %x\n", Status);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed
        //
        return Status;
    }

    DPRINT1("FunctionDescriptorBufferLength %lu\n", FunctionDescriptorBufferLength);
    DPRINT1("FunctionDescriptorBuffer %p\n", FunctionDescriptorBuffer);

    //
    // assume length % function buffer size
    //
    ASSERT(FunctionDescriptorBufferLength);
    ASSERT(FunctionDescriptorBufferLength % sizeof(USBC_FUNCTION_DESCRIPTOR) == 0);

    //
    // store result
    //
    FDODeviceExtension->FunctionDescriptor = FunctionDescriptorBuffer;
    FDODeviceExtension->FunctionDescriptorCount = FunctionDescriptorBufferLength / sizeof(USBC_FUNCTION_DESCRIPTOR);

    //
    // success
    //
    return STATUS_SUCCESS;
}

ULONG
USBCCGP_CountAssociationDescriptors(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    PUSB_INTERFACE_ASSOCIATION_DESCRIPTOR Descriptor;
    PUCHAR Offset, End;
    ULONG Count = 0;

    //
    // init offsets
    //
    Offset = (PUCHAR)ConfigurationDescriptor + ConfigurationDescriptor->bLength;
    End = (PUCHAR)ConfigurationDescriptor + ConfigurationDescriptor->wTotalLength;

    while(Offset < End)
    {
        //
        // get association descriptor
        //
        Descriptor = (PUSB_INTERFACE_ASSOCIATION_DESCRIPTOR)Offset;

        if (Descriptor->bLength == sizeof(USB_INTERFACE_ASSOCIATION_DESCRIPTOR) && Descriptor->bDescriptorType == USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE)
        {
            //
            // found descriptor
            //
            Count++;
        }

        //
        // move to next descriptor
        //
        Offset += Descriptor->bLength;
    }

    //
    // done
    //
    return Count;
}

PUSB_INTERFACE_ASSOCIATION_DESCRIPTOR
USBCCGP_GetAssociationDescriptorAtIndex(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN ULONG Index)
{
    PUSB_INTERFACE_ASSOCIATION_DESCRIPTOR Descriptor;
    PUCHAR Offset, End;
    ULONG Count = 0;

    //
    // init offsets
    //
    Offset = (PUCHAR)ConfigurationDescriptor + ConfigurationDescriptor->bLength;
    End = (PUCHAR)ConfigurationDescriptor + ConfigurationDescriptor->wTotalLength;

    while(Offset < End)
    {
        //
        // get association descriptor
        //
        Descriptor = (PUSB_INTERFACE_ASSOCIATION_DESCRIPTOR)Offset;

        if (Descriptor->bLength == sizeof(USB_INTERFACE_ASSOCIATION_DESCRIPTOR) && Descriptor->bDescriptorType == USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE)
        {
            if (Index == Count)
            {
                //
                // found descriptor
                //
                return Descriptor;
            }

            //
            // not the searched one
            //
            Count++;
        }

        //
        // move to next descriptor
        //
        Offset += Descriptor->bLength;
    }

    //
    // failed to find descriptor at the specified index
    //
    return NULL;
}

NTSTATUS
USBCCGP_InitInterfaceListOfFunctionDescriptor(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PUSB_INTERFACE_ASSOCIATION_DESCRIPTOR AssociationDescriptor,
    OUT PUSBC_FUNCTION_DESCRIPTOR FunctionDescriptor)
{
    PUSB_INTERFACE_DESCRIPTOR Descriptor;
    PUCHAR Offset, End;
    ULONG Count = 0;

    //
    // init offsets
    //
    Offset = (PUCHAR)AssociationDescriptor + AssociationDescriptor->bLength;
    End = (PUCHAR)ConfigurationDescriptor + ConfigurationDescriptor->wTotalLength;

    while(Offset < End)
    {
        //
        // get association descriptor
        //
        Descriptor = (PUSB_INTERFACE_DESCRIPTOR)Offset;

        if (Descriptor->bLength == sizeof(USB_INTERFACE_DESCRIPTOR) && Descriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
        {
            //
            // store interface descriptor
            //
            FunctionDescriptor->InterfaceDescriptorList[Count] = Descriptor;
            Count++;

            if (Count == AssociationDescriptor->bInterfaceCount)
            {
                //
                // got all interfaces
                //
                return STATUS_SUCCESS;
            }
        }

        if (Descriptor->bLength == sizeof(USB_INTERFACE_ASSOCIATION_DESCRIPTOR) && Descriptor->bDescriptorType == USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE)
        {
            //
            // WTF? a association descriptor which overlaps the next association descriptor
            //
            DPRINT1("Invalid association descriptor\n");
            ASSERT(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        //
        // move to next descriptor
        //
        Offset += Descriptor->bLength;
    }

    //
    // invalid association descriptor
    //
    DPRINT1("Invalid association descriptor\n");
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
USBCCGP_InitFunctionDescriptor(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension,
    IN ULONG FunctionNumber,
    OUT PUSBC_FUNCTION_DESCRIPTOR FunctionDescriptor)
{
    PUSB_INTERFACE_ASSOCIATION_DESCRIPTOR Descriptor;
    NTSTATUS Status;
    LPWSTR DescriptionBuffer;
    WCHAR Buffer[100];
    ULONG Index;

    // init function number
    FunctionDescriptor->FunctionNumber = (UCHAR)FunctionNumber;

    // get association descriptor
    Descriptor = USBCCGP_GetAssociationDescriptorAtIndex(FDODeviceExtension->ConfigurationDescriptor, FunctionNumber);
    ASSERT(Descriptor);

    // store number interfaces
    FunctionDescriptor->NumberOfInterfaces = Descriptor->bInterfaceCount;

    // allocate array for interface count
    FunctionDescriptor->InterfaceDescriptorList = AllocateItem(NonPagedPool, sizeof(PUSB_INTERFACE_DESCRIPTOR) * Descriptor->bInterfaceCount);
    if (FunctionDescriptor->InterfaceDescriptorList)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // init interface list
    Status = USBCCGP_InitInterfaceListOfFunctionDescriptor(FDODeviceExtension->ConfigurationDescriptor, Descriptor, FunctionDescriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed
        //
        return Status;
    }

    //
    // now init interface description
    //
    if (Descriptor->iFunction)
    {
        //
        // get interface description
        //
         Status = USBCCGP_GetDescriptor(FDODeviceExtension->NextDeviceObject, 
                                        USB_STRING_DESCRIPTOR_TYPE, 
                                        100 * sizeof(WCHAR), 
                                        Descriptor->iFunction, 
                                        0x0409, //FIXME
                                        (PVOID*)&DescriptionBuffer);
        if (!NT_SUCCESS(Status))
        {
            //
            // no description
            //
            RtlInitUnicodeString(&FunctionDescriptor->FunctionDescription, L"");
        }
        else
        {
            //
            // init description
            //
            RtlInitUnicodeString(&FunctionDescriptor->FunctionDescription, DescriptionBuffer);
        }
        DPRINT1("FunctionDescription %wZ\n", &FunctionDescriptor->FunctionDescription);
    }

    //
    // now init hardware id
    //
    Index = swprintf(Buffer, L"USB\\VID_%04x&PID_%04x&Rev_%04x&MI_%02x", FDODeviceExtension->DeviceDescriptor->idVendor,
                                                                         FDODeviceExtension->DeviceDescriptor->idProduct,
                                                                         FDODeviceExtension->DeviceDescriptor->bcdDevice,
                                                                         Descriptor->bFirstInterface) + 1;
    Index = swprintf(&Buffer[Index], L"USB\\VID_%04x&PID_%04x&MI_%02x", FDODeviceExtension->DeviceDescriptor->idVendor,
                                                                        FDODeviceExtension->DeviceDescriptor->idProduct,
                                                                        Descriptor->bFirstInterface) + 1;

    // allocate result buffer
    DescriptionBuffer = AllocateItem(NonPagedPool, (Index + 1) * sizeof(WCHAR));
    if (!DescriptionBuffer)
    {
        //
        // failed to allocate memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // copy description
    RtlCopyMemory(DescriptionBuffer, Buffer, Index * sizeof(WCHAR));
    RtlInitUnicodeString(&FunctionDescriptor->HardwareId, DescriptionBuffer);

    //
    // now init the compatible id
    //
    Index = swprintf(Buffer, L"USB\\Class_%02x&SubClass_%02x&Prot_%02x", Descriptor->bFunctionClass, Descriptor->bFunctionSubClass, Descriptor->bFunctionProtocol) + 1;
    Index = swprintf(&Buffer[Index], L"USB\\Class_%04x&SubClass_%04x",  Descriptor->bFunctionClass, Descriptor->bFunctionSubClass) + 1;
    Index = swprintf(&Buffer[Index], L"USB\\Class_%04x", Descriptor->bFunctionClass) + 1;

    // allocate result buffer
    DescriptionBuffer = AllocateItem(NonPagedPool, (Index + 1) * sizeof(WCHAR));
    if (!DescriptionBuffer)
    {
        //
        // failed to allocate memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // copy description
    // copy description
    RtlCopyMemory(DescriptionBuffer, Buffer, Index * sizeof(WCHAR));
    RtlInitUnicodeString(&FunctionDescriptor->CompatibleId, DescriptionBuffer);

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
USBCCGP_EnumWithAssociationDescriptor(
    IN PDEVICE_OBJECT DeviceObject)
{
    ULONG DescriptorCount, Index;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // get device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // count association descriptors
    //
    DescriptorCount = USBCCGP_CountAssociationDescriptors(FDODeviceExtension->ConfigurationDescriptor);
    if (!DescriptorCount)
    {
        //
        // no descriptors found
        //
        return STATUS_NOT_SUPPORTED;
    }

    //
    // allocate function descriptor array
    //
    FDODeviceExtension->FunctionDescriptor = AllocateItem(NonPagedPool, sizeof(USBC_FUNCTION_DESCRIPTOR) * DescriptorCount);
    if (!FDODeviceExtension->FunctionDescriptorCount)
    {
        //
        // no memory
        //
        DPRINT1("USBCCGP_EnumWithAssociationDescriptor failed to allocate function descriptor count %x\n", DescriptorCount);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for(Index = 0; Index < DescriptorCount; Index++)
    {
        //
        // init function descriptors
        //
        Status = USBCCGP_InitFunctionDescriptor(FDODeviceExtension, Index, &FDODeviceExtension->FunctionDescriptor[Index]);
        if (!NT_SUCCESS(Status))
        {
            //
            // failed
            //
            return Status;
        }
    }

    //
    // store function descriptor count
    //
    FDODeviceExtension->FunctionDescriptorCount = DescriptorCount;

    //
    // done
    //
    return Status;
}


NTSTATUS
USBCCGP_EnumWithAudioLegacy(
    IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
USBCCGP_EnumWithUnionFunctionDescriptors(
    IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
USBCCGP_EnumerateFunctions(
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    //
    // get device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // first try with filter driver
    //
    Status = USBCCGP_CustomEnumWithInterface(DeviceObject);
    if (NT_SUCCESS(Status))
    {
        //
        // succeeded
        //
        return Status;
    }

    //
    // enumerate functions with interface association descriptor
    //
    Status = USBCCGP_EnumWithAssociationDescriptor(DeviceObject);
    if (NT_SUCCESS(Status))
    {
        //
        // succeeded
        //
        return Status;
    }

    //
    // try with union function descriptors
    //
    Status = USBCCGP_EnumWithUnionFunctionDescriptors(DeviceObject);
    if (NT_SUCCESS(Status))
    {
        //
        // succeeded
        //
        return Status;
    }

    //
    // try with legacy audio methods
    //
    return USBCCGP_EnumWithAudioLegacy(DeviceObject);
}
