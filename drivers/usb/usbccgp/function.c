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

#define NDEBUG
#include <debug.h>

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

    /* Sanity checks */
    ASSERT(DeviceObject);

    /* Initialize event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Init interface */
    RtlZeroMemory(BusInterface, sizeof(USBC_DEVICE_CONFIGURATION_INTERFACE_V1));
    BusInterface->Version = USBC_DEVICE_CONFIGURATION_INTERFACE_VERSION_1;
    BusInterface->Size = sizeof(USBC_DEVICE_CONFIGURATION_INTERFACE_V1);

    /* Create irp */
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
    Stack = IoGetNextIrpStackLocation(Irp);
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
    Status = IoCallDriver(DeviceObject, Irp);

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

    DPRINT("USBCCGP_CustomEnumWithInterface Status %lx\n", Status);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed
        //
        return Status;
    }

    DPRINT("FunctionDescriptorBufferLength %lu\n", FunctionDescriptorBufferLength);
    DPRINT("FunctionDescriptorBuffer %p\n", FunctionDescriptorBuffer);

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

    while (Offset < End)
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

    while (Offset < End)
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

    while (Offset < End)
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
    if (!FunctionDescriptor->InterfaceDescriptorList)
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
         Status = USBCCGP_GetStringDescriptor(FDODeviceExtension->NextDeviceObject,
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
    Index += swprintf(&Buffer[Index], L"USB\\VID_%04x&PID_%04x&MI_%02x", FDODeviceExtension->DeviceDescriptor->idVendor,
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
    RtlCopyMemory(DescriptionBuffer, Buffer, (Index + 1) * sizeof(WCHAR));
    FunctionDescriptor->HardwareId.Buffer = DescriptionBuffer;
    FunctionDescriptor->HardwareId.Length = Index * sizeof(WCHAR);
    FunctionDescriptor->HardwareId.MaximumLength = (Index + 1) * sizeof(WCHAR);

    //
    // now init the compatible id
    //
    Index = swprintf(Buffer, L"USB\\Class_%02x&SubClass_%02x&Prot_%02x", Descriptor->bFunctionClass, Descriptor->bFunctionSubClass, Descriptor->bFunctionProtocol) + 1;
    Index += swprintf(&Buffer[Index], L"USB\\Class_%02x&SubClass_%02x",  Descriptor->bFunctionClass, Descriptor->bFunctionSubClass) + 1;
    Index += swprintf(&Buffer[Index], L"USB\\Class_%02x", Descriptor->bFunctionClass) + 1;

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
    RtlCopyMemory(DescriptionBuffer, Buffer, (Index + 1) * sizeof(WCHAR));
    FunctionDescriptor->CompatibleId.Buffer = DescriptionBuffer;
    FunctionDescriptor->CompatibleId.Length = Index * sizeof(WCHAR);
    FunctionDescriptor->CompatibleId.MaximumLength = (Index + 1) * sizeof(WCHAR);

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
    if (!FDODeviceExtension->FunctionDescriptor)
    {
        //
        // no memory
        //
        DPRINT1("USBCCGP_EnumWithAssociationDescriptor failed to allocate function descriptor count %x\n", DescriptorCount);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (Index = 0; Index < DescriptorCount; Index++)
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
USBCCG_InitIdsWithInterfaceDescriptor(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension,
    IN PUSB_INTERFACE_DESCRIPTOR Descriptor,
    IN ULONG FunctionIndex,
    OUT PUSBC_FUNCTION_DESCRIPTOR FunctionDescriptor)
{
    ULONG Index;
    WCHAR Buffer[200];
    LPWSTR DescriptionBuffer;
    NTSTATUS Status;

    //
    // now init interface description
    //
    if (Descriptor->iInterface)
    {
        //
        // get interface description
        //
         Status = USBCCGP_GetStringDescriptor(FDODeviceExtension->NextDeviceObject,
                                              100 * sizeof(WCHAR),
                                              Descriptor->iInterface,
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
                                                                         FunctionIndex) + 1;
    Index += swprintf(&Buffer[Index], L"USB\\VID_%04x&PID_%04x&MI_%02x", FDODeviceExtension->DeviceDescriptor->idVendor,
                                                                         FDODeviceExtension->DeviceDescriptor->idProduct,
                                                                         FunctionIndex) + 1;

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
    RtlCopyMemory(DescriptionBuffer, Buffer, (Index + 1) * sizeof(WCHAR));
    FunctionDescriptor->HardwareId.Buffer = DescriptionBuffer;
    FunctionDescriptor->HardwareId.Length = Index * sizeof(WCHAR);
    FunctionDescriptor->HardwareId.MaximumLength = (Index + 1) * sizeof(WCHAR);

    //
    // now init the compatible id
    //
    Index = swprintf(Buffer, L"USB\\Class_%02x&SubClass_%02x&Prot_%02x", Descriptor->bInterfaceClass, Descriptor->bInterfaceSubClass, Descriptor->bInterfaceProtocol) + 1;
    Index += swprintf(&Buffer[Index], L"USB\\Class_%02x&SubClass_%02x",  Descriptor->bInterfaceClass, Descriptor->bInterfaceSubClass) + 1;
    Index += swprintf(&Buffer[Index], L"USB\\Class_%02x", Descriptor->bInterfaceClass) + 1;

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
    RtlCopyMemory(DescriptionBuffer, Buffer, (Index + 1) * sizeof(WCHAR));
    FunctionDescriptor->CompatibleId.Buffer = DescriptionBuffer;
    FunctionDescriptor->CompatibleId.Length = Index * sizeof(WCHAR);
    FunctionDescriptor->CompatibleId.MaximumLength = (Index + 1) * sizeof(WCHAR);

    //
    // done
    //
    return STATUS_SUCCESS;
}


NTSTATUS
USBCCGP_LegacyEnum(
    IN PDEVICE_OBJECT DeviceObject)
{
    ULONG Index, SubIndex;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    NTSTATUS Status = STATUS_SUCCESS;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor, NextInterfaceDescriptor;

    //
    // get device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // sanity check
    //
    ASSERT(FDODeviceExtension->ConfigurationDescriptor->bNumInterfaces);

    //
    // allocate function array
    //
    FDODeviceExtension->FunctionDescriptor = AllocateItem(NonPagedPool, sizeof(USBC_FUNCTION_DESCRIPTOR) * FDODeviceExtension->ConfigurationDescriptor->bNumInterfaces);
    if (!FDODeviceExtension->FunctionDescriptor)
    {
        //
        // no memory
        //
        DPRINT1("USBCCGP_EnumWithAssociationDescriptor failed to allocate function descriptor %lu\n", FDODeviceExtension->ConfigurationDescriptor->bNumInterfaces);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // init function descriptors
    //
    FDODeviceExtension->FunctionDescriptorCount = FDODeviceExtension->ConfigurationDescriptor->bNumInterfaces;
    for (Index = 0; Index < FDODeviceExtension->ConfigurationDescriptor->bNumInterfaces; Index++)
    {
        // get interface descriptor
        InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(FDODeviceExtension->ConfigurationDescriptor, FDODeviceExtension->ConfigurationDescriptor, Index, 0, -1, -1, -1);
        if (InterfaceDescriptor == NULL)
        {
            //
            // failed to find interface descriptor
            //
            DPRINT1("[USBCCGP] Failed to find interface descriptor index %lu\n", Index);
            ASSERT(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        SubIndex = 0;
        if (InterfaceDescriptor->bInterfaceClass == 0x01)
        {
            // AUDIO CLASS lets group all audio interfaces together
            //
            // init function descriptor
            //
            FDODeviceExtension->FunctionDescriptor[Index].FunctionNumber = Index;
            FDODeviceExtension->FunctionDescriptor[Index].NumberOfInterfaces = 1;
            FDODeviceExtension->FunctionDescriptor[Index].InterfaceDescriptorList = AllocateItem(NonPagedPool, sizeof(PUSB_INTERFACE_DESCRIPTOR) * FDODeviceExtension->ConfigurationDescriptor->bNumInterfaces);
            if (!FDODeviceExtension->FunctionDescriptor[Index].InterfaceDescriptorList)
            {
                //
                // no memory
                //
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // store interface descriptor
            //
            FDODeviceExtension->FunctionDescriptor[Index].InterfaceDescriptorList[SubIndex] = InterfaceDescriptor;
            do
            {
                NextInterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(FDODeviceExtension->ConfigurationDescriptor, FDODeviceExtension->ConfigurationDescriptor, Index + SubIndex + 1, 0, -1, -1, -1);
                if (NextInterfaceDescriptor)
                {
                    if (NextInterfaceDescriptor->bInterfaceClass != 0x01)
                    {
                        break;
                    }
                    SubIndex++;
                    FDODeviceExtension->FunctionDescriptor[Index].NumberOfInterfaces++;
                    FDODeviceExtension->FunctionDescriptor[Index].InterfaceDescriptorList[SubIndex] = NextInterfaceDescriptor;
                }
                else
                {
                    break;
                }
            } while (TRUE);
        }
        else
        {
            //
            // init function descriptor
            //
            FDODeviceExtension->FunctionDescriptor[Index].FunctionNumber = Index;
            FDODeviceExtension->FunctionDescriptor[Index].NumberOfInterfaces = 1;
            FDODeviceExtension->FunctionDescriptor[Index].InterfaceDescriptorList = AllocateItem(NonPagedPool, sizeof(PUSB_INTERFACE_DESCRIPTOR) * 1);
            if (!FDODeviceExtension->FunctionDescriptor[Index].InterfaceDescriptorList)
            {
                //
                // no memory
                //
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // store interface descriptor
            //
            FDODeviceExtension->FunctionDescriptor[Index].InterfaceDescriptorList[0] = InterfaceDescriptor;
        }

        //
        // now init the device ids
        //
        Status = USBCCG_InitIdsWithInterfaceDescriptor(FDODeviceExtension, InterfaceDescriptor, Index, &FDODeviceExtension->FunctionDescriptor[Index]);
        if (!NT_SUCCESS(Status))
        {
            //
            // failed to init ids
            //
            DPRINT1("[USBCCGP] Failed to init ids with %lx\n", Status);
            return Status;
        }
        //
        // increment interface count
        //
        Index += SubIndex;
    }

    //
    // done
    //
    return Status;
}

NTSTATUS
USBCCGP_EnumWithUnionFunctionDescriptors(
    IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
USBCCGP_EnumWithAudioLegacy(
    IN PDEVICE_OBJECT DeviceObject)
{
    ULONG Index;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor, FirstDescriptor = NULL;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // get device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);


    //
    // first check if all interfaces belong to the same audio class
    //
    for (Index = 0; Index < FDODeviceExtension->ConfigurationDescriptor->bNumInterfaces; Index++)
    {
        //
        // get interface descriptor
        //
        InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(FDODeviceExtension->ConfigurationDescriptor, FDODeviceExtension->ConfigurationDescriptor, Index, 0, -1, -1, -1);
        DPRINT1("Index %lu Descriptor %p\n", Index, InterfaceDescriptor);
        ASSERT(InterfaceDescriptor);

        if (InterfaceDescriptor->bInterfaceClass != 0x1)
        {
            //
            // collection contains non audio class
            //
            return STATUS_UNSUCCESSFUL;
        }

        if (FirstDescriptor == NULL)
        {
            //
            // store interface descriptor
            //
            FirstDescriptor = InterfaceDescriptor;
            continue;
        }

        if (FirstDescriptor->bInterfaceSubClass == InterfaceDescriptor->bInterfaceSubClass)
        {
            //
            // interface subclass must be different from the first interface
            //
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    // this is an composite audio device
    //
    DPRINT("[USBCCGP] Audio Composite Device detected\n");

    //
    // audio interfaces are all grouped into one single function
    //
    FDODeviceExtension->FunctionDescriptor = AllocateItem(NonPagedPool, sizeof(USBC_FUNCTION_DESCRIPTOR));
    if (!FDODeviceExtension->FunctionDescriptor)
    {
        //
        // no memory
        //
        DPRINT1("USBCCGP_EnumWithAssociationDescriptor failed to allocate function descriptor count\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // init function number
    //
    FDODeviceExtension->FunctionDescriptor[0].FunctionNumber = 0;

    //
    // store interfaces
    //
    Status = AllocateInterfaceDescriptorsArray(FDODeviceExtension->ConfigurationDescriptor, &FDODeviceExtension->FunctionDescriptor[0].InterfaceDescriptorList);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate descriptor array
        //
        DPRINT1("[USBCCGP] Failed to allocate descriptor array %lx\n", Status);
        return Status;
    }

    //
    // now init the device ids
    //
    Status = USBCCG_InitIdsWithInterfaceDescriptor(FDODeviceExtension, FirstDescriptor, 0, &FDODeviceExtension->FunctionDescriptor[0]);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to init ids
        //
        DPRINT1("[USBCCGP] Failed to init ids with %lx\n", Status);
        return Status;
    }

    //
    // number of interfaces
    //
    FDODeviceExtension->FunctionDescriptor[0].NumberOfInterfaces = FDODeviceExtension->ConfigurationDescriptor->bNumInterfaces;

    //
    // store function count
    //
    FDODeviceExtension->FunctionDescriptorCount = 1;

    //
    // done
    //
    return STATUS_SUCCESS;
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

#if 0
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
#endif

    //
    // try with legacy audio methods
    //
    Status = USBCCGP_EnumWithAudioLegacy(DeviceObject);
    if (NT_SUCCESS(Status))
    {
        //
        // succeeded
        //
        return Status;
    }

    //
    // try with legacy enumeration
    //
    return USBCCGP_LegacyEnum(DeviceObject);
}
