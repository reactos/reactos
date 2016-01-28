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
NTAPI
USBCCGP_GetDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR DescriptorType,
    IN ULONG DescriptorLength,
    IN UCHAR DescriptorIndex,
    IN LANGID LanguageId,
    OUT PVOID *OutDescriptor)
{
    PURB Urb;
    NTSTATUS Status;
    PVOID Descriptor;

    //
    // sanity checks
    //
    ASSERT(DeviceObject);
    ASSERT(OutDescriptor);
    ASSERT(DescriptorLength);

    //
    // first allocate descriptor buffer
    //
    Descriptor = AllocateItem(NonPagedPool, DescriptorLength);
    if (!Descriptor)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // allocate urb
    //
    Urb = (PURB) AllocateItem(NonPagedPool, sizeof(URB));
    if (!Urb)
    {
        //
        // no memory
        //
        FreeItem(Descriptor);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize urb
    //
    UsbBuildGetDescriptorRequest(Urb,
                                 sizeof(Urb->UrbControlDescriptorRequest),
                                 DescriptorType,
                                 DescriptorIndex,
                                 LanguageId,
                                 Descriptor,
                                 NULL,
                                 DescriptorLength,
                                 NULL);

    //
    // submit urb
    //
    Status = USBCCGP_SyncUrbRequest(DeviceObject, Urb);

    //
    // free urb
    //
    FreeItem(Urb);

    if (NT_SUCCESS(Status))
    {
        //
        // store result
        //
        *OutDescriptor = Descriptor;
    }

    //
    // done
    //
    return Status;
}

NTSTATUS
NTAPI
USBCCGP_GetStringDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DescriptorLength,
    IN UCHAR DescriptorIndex,
    IN LANGID LanguageId,
    OUT PVOID *OutDescriptor)
{
    NTSTATUS Status;
    PUSB_STRING_DESCRIPTOR StringDescriptor;
    ULONG Size;
    PVOID Buffer;

    // retrieve descriptor
    Status = USBCCGP_GetDescriptor(DeviceObject, USB_STRING_DESCRIPTOR_TYPE, DescriptorLength, DescriptorIndex, LanguageId, OutDescriptor);
    if (!NT_SUCCESS(Status))
    {
        // failed
        return Status;
    }

    // get descriptor structure
    StringDescriptor = (PUSB_STRING_DESCRIPTOR)*OutDescriptor;

    // sanity check
    ASSERT(StringDescriptor->bLength < DescriptorLength - 2);

    if (StringDescriptor->bLength == 2)
    {
        // invalid descriptor
        FreeItem(StringDescriptor);
        return STATUS_DEVICE_DATA_ERROR;
    }

    // calculate size
    Size = StringDescriptor->bLength + sizeof(WCHAR);

    // allocate buffer
    Buffer = AllocateItem(NonPagedPool, Size);
    if (!Buffer)
    {
        // no memory
        FreeItem(StringDescriptor);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // copy result
    RtlCopyMemory(Buffer, StringDescriptor->bString, Size - FIELD_OFFSET(USB_STRING_DESCRIPTOR, bString));

    // free buffer
    FreeItem(StringDescriptor);

    // store result
    *OutDescriptor = (PVOID)Buffer;
    return STATUS_SUCCESS;
}


NTSTATUS
USBCCGP_GetDescriptors(
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PFDO_DEVICE_EXTENSION DeviceExtension;
    USHORT DescriptorLength;

    //
    // get device extension
    //
    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

     //
     // first get device descriptor
     //
     Status = USBCCGP_GetDescriptor(DeviceExtension->NextDeviceObject, USB_DEVICE_DESCRIPTOR_TYPE, sizeof(USB_DEVICE_DESCRIPTOR), 0, 0, (PVOID*)&DeviceExtension->DeviceDescriptor);
     if (!NT_SUCCESS(Status))
     {
         //
         // failed to get device descriptor
         //
         DeviceExtension->DeviceDescriptor = NULL;
         return Status;
     }

     //
     // now get basic configuration descriptor
     //
     Status = USBCCGP_GetDescriptor(DeviceExtension->NextDeviceObject, USB_CONFIGURATION_DESCRIPTOR_TYPE, sizeof(USB_CONFIGURATION_DESCRIPTOR), 0, 0, (PVOID*)&DeviceExtension->ConfigurationDescriptor);
     if (!NT_SUCCESS(Status))
     {
         //
         // failed to get configuration descriptor
         //
         FreeItem(DeviceExtension->DeviceDescriptor);
         DeviceExtension->DeviceDescriptor = NULL;
         return Status;
     }

     //
     // backup length
     //
     DescriptorLength = DeviceExtension->ConfigurationDescriptor->wTotalLength;

     //
     // release basic descriptor
     //
     FreeItem(DeviceExtension->ConfigurationDescriptor);
     DeviceExtension->ConfigurationDescriptor = NULL;

     //
     // allocate full descriptor
     //
     Status = USBCCGP_GetDescriptor(DeviceExtension->NextDeviceObject, USB_CONFIGURATION_DESCRIPTOR_TYPE, DescriptorLength, 0, 0, (PVOID*)&DeviceExtension->ConfigurationDescriptor);
     if (!NT_SUCCESS(Status))
     {
         //
         // failed to get configuration descriptor
         //
         FreeItem(DeviceExtension->DeviceDescriptor);
         DeviceExtension->DeviceDescriptor = NULL;
         return Status;
     }
     return Status;
}

NTSTATUS
AllocateInterfaceDescriptorsArray(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    OUT PUSB_INTERFACE_DESCRIPTOR **OutArray)
{
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    ULONG Count = 0;
    PUSB_INTERFACE_DESCRIPTOR *Array;

    //
    // allocate array
    //
    Array = AllocateItem(NonPagedPool, sizeof(PUSB_INTERFACE_DESCRIPTOR) * ConfigurationDescriptor->bNumInterfaces);
    if (!Array)
        return STATUS_INSUFFICIENT_RESOURCES;

    //
    // enumerate all interfaces
    //
    Count = 0;
    do
    {
        //
        // find next descriptor
        //
        InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor, ConfigurationDescriptor, Count, 0, -1, -1, -1);
        if (!InterfaceDescriptor)
            break;

        //
        // store descriptor
        //
        Array[Count] = InterfaceDescriptor;
        Count++;

    }while(TRUE);

    //
    // store result
    //
    *OutArray = Array;

    //
    // done
    //
    return STATUS_SUCCESS;
}

VOID
DumpFullConfigurationDescriptor(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    PUSB_COMMON_DESCRIPTOR Descriptor;

    Descriptor = (PUSB_COMMON_DESCRIPTOR)ConfigurationDescriptor;

    DbgPrint("Bogus ConfigurationDescriptor Found\n");
    DbgPrint("InterfaceCount %x\n", ConfigurationDescriptor->bNumInterfaces);

    do
    {
        if (((ULONG_PTR)Descriptor) >= ((ULONG_PTR)ConfigurationDescriptor + ConfigurationDescriptor->wTotalLength))
            break;

        DbgPrint("Descriptor Type %x Length %lu Offset %lu\n", Descriptor->bDescriptorType, Descriptor->bLength, ((ULONG_PTR)Descriptor - (ULONG_PTR)ConfigurationDescriptor));

        // check for invalid descriptors
        if (!Descriptor->bLength) 
        {
            DbgPrint("Bogus Descriptor!!!\n");
            break;
        }

        // advance to next descriptor
        Descriptor = (PUSB_COMMON_DESCRIPTOR)((ULONG_PTR)Descriptor + Descriptor->bLength);

    }while(TRUE);


}


NTSTATUS
NTAPI
USBCCGP_ScanConfigurationDescriptor(
    IN OUT PFDO_DEVICE_EXTENSION FDODeviceExtension,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    ULONG InterfaceIndex = 0;
    ULONG DescriptorCount;

    //
    // sanity checks
    //
    ASSERT(ConfigurationDescriptor);
    ASSERT(ConfigurationDescriptor->bNumInterfaces);

    //
    // count all interface descriptors
    //
    DescriptorCount = ConfigurationDescriptor->bNumInterfaces;
    if (DescriptorCount == 0)
    {
        DPRINT1("[USBCCGP] DescriptorCount is zero\n");
        return STATUS_INVALID_PARAMETER;
    }

    //
    // allocate array holding the interface descriptors
    //
    FDODeviceExtension->InterfaceList = AllocateItem(NonPagedPool, sizeof(USBD_INTERFACE_LIST_ENTRY) * (DescriptorCount + 1));
    if (!FDODeviceExtension->InterfaceList)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // reset interface list count
    //
    FDODeviceExtension->InterfaceListCount = 0;

    do
    {
        //
        // parse configuration descriptor
        //
        InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor, ConfigurationDescriptor, InterfaceIndex, -1, -1, -1, -1);
        if (InterfaceDescriptor)
        {
            //
            // store in interface list
            //
            ASSERT(FDODeviceExtension->InterfaceListCount < DescriptorCount);
            FDODeviceExtension->InterfaceList[FDODeviceExtension->InterfaceListCount].InterfaceDescriptor = InterfaceDescriptor;
            FDODeviceExtension->InterfaceListCount++;
        }
        else
        {
            DumpConfigurationDescriptor(ConfigurationDescriptor);
            DumpFullConfigurationDescriptor(FDODeviceExtension, ConfigurationDescriptor);

            //
            // see issue
            // CORE-6574 Test 3 (USB Web Cam)
            //
            if (FDODeviceExtension->DeviceDescriptor && FDODeviceExtension->DeviceDescriptor->idVendor == 0x0458 && FDODeviceExtension->DeviceDescriptor->idProduct == 0x705f)
                ASSERT(FALSE);
        }

        //
        // move to next interface
        //
        InterfaceIndex++;

    }while(InterfaceIndex < DescriptorCount);

    //
    // sanity check
    //
    ASSERT(FDODeviceExtension->InterfaceListCount);

    //
    // done
    //
    return STATUS_SUCCESS;
}

VOID
DumpConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    DbgPrint("Dumping ConfigurationDescriptor %x\n", ConfigurationDescriptor);
    DbgPrint("bLength %x\n", ConfigurationDescriptor->bLength);
    DbgPrint("bDescriptorType %x\n", ConfigurationDescriptor->bDescriptorType);
    DbgPrint("wTotalLength %x\n", ConfigurationDescriptor->wTotalLength);
    DbgPrint("bNumInterfaces %x\n", ConfigurationDescriptor->bNumInterfaces);
    DbgPrint("bConfigurationValue %x\n", ConfigurationDescriptor->bConfigurationValue);
    DbgPrint("iConfiguration %x\n", ConfigurationDescriptor->iConfiguration);
    DbgPrint("bmAttributes %x\n", ConfigurationDescriptor->bmAttributes);
    DbgPrint("MaxPower %x\n", ConfigurationDescriptor->MaxPower);
}

NTSTATUS
USBCCGP_SelectInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension,
    IN ULONG InterfaceIndex)
{
    NTSTATUS Status;
    PURB Urb;

    //
    // allocate urb
    //
    Urb = AllocateItem(NonPagedPool, GET_SELECT_INTERFACE_REQUEST_SIZE(DeviceExtension->InterfaceList[InterfaceIndex].InterfaceDescriptor->bNumEndpoints));
    if (!Urb)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // now prepare interface urb
    //
    UsbBuildSelectInterfaceRequest(Urb, GET_SELECT_INTERFACE_REQUEST_SIZE(DeviceExtension->InterfaceList[InterfaceIndex].InterfaceDescriptor->bNumEndpoints), DeviceExtension->ConfigurationHandle, DeviceExtension->InterfaceList[InterfaceIndex].InterfaceDescriptor->bInterfaceNumber, DeviceExtension->InterfaceList[InterfaceIndex].InterfaceDescriptor->bAlternateSetting);

    //
    // now select the interface
    //
    Status = USBCCGP_SyncUrbRequest(DeviceExtension->NextDeviceObject, Urb);

    //
    // did it succeeed
    //
    if (NT_SUCCESS(Status))
    {
        //
        // update configuration info
        //
        ASSERT(Urb->UrbSelectInterface.Interface.Length == DeviceExtension->InterfaceList[InterfaceIndex].Interface->Length);
        RtlCopyMemory(DeviceExtension->InterfaceList[InterfaceIndex].Interface, &Urb->UrbSelectInterface.Interface, Urb->UrbSelectInterface.Interface.Length);
    }

    //
    // free urb
    //
    FreeItem(Urb);

    //
    // done
    //
    return Status;
}

NTSTATUS
USBCCGP_SelectConfiguration(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension)
{
    PUSBD_INTERFACE_INFORMATION InterfaceInformation;
    NTSTATUS Status;
    PURB Urb;
    ULONG Index;

    //
    // now scan configuration descriptors
    //
    Status = USBCCGP_ScanConfigurationDescriptor(DeviceExtension, DeviceExtension->ConfigurationDescriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to scan
        //
        return Status;
    }

    //
    // now allocate the urb
    //
    Urb = USBD_CreateConfigurationRequestEx(DeviceExtension->ConfigurationDescriptor, DeviceExtension->InterfaceList);
    if (!Urb)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // submit urb
    //
    Status = USBCCGP_SyncUrbRequest(DeviceExtension->NextDeviceObject, Urb);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to set configuration
        //
        DPRINT1("USBCCGP_SyncUrbRequest failed to set interface %x\n", Status);
        ExFreePool(Urb);
        return Status;
    }

    //
    // get interface information
    //
    InterfaceInformation = &Urb->UrbSelectConfiguration.Interface;
    for(Index = 0; Index < DeviceExtension->InterfaceListCount; Index++)
    {
        //
        // allocate buffer to store interface information
        //
        DeviceExtension->InterfaceList[Index].Interface = AllocateItem(NonPagedPool, InterfaceInformation->Length);
        if (!DeviceExtension->InterfaceList[Index].Interface)
        {
            //
            // no memory
            //
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // copy interface information
        //
        RtlCopyMemory(DeviceExtension->InterfaceList[Index].Interface, InterfaceInformation, InterfaceInformation->Length);

        //
        // move to next interface
        //
        InterfaceInformation = (PUSBD_INTERFACE_INFORMATION)((ULONG_PTR)InterfaceInformation + InterfaceInformation->Length);
    }


    //
    // store pipe handle
    //
    DeviceExtension->ConfigurationHandle = Urb->UrbSelectConfiguration.ConfigurationHandle;

    //
    // free interface list & urb
    //
    ExFreePool(Urb);

    //
    // done
    //
    return Status;
}
