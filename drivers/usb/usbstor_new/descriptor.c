/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbstor/descriptor.c
 * PURPOSE:     USB block storage device driver.
 * PROGRAMMERS:
 *              James Tabor
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbstor.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
USBSTOR_GetDescriptor(
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
    Status = USBSTOR_SyncUrbRequest(DeviceObject, Urb);

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
USBSTOR_GetDescriptors(
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
     Status = USBSTOR_GetDescriptor(DeviceExtension->LowerDeviceObject, USB_DEVICE_DESCRIPTOR_TYPE, sizeof(USB_DEVICE_DESCRIPTOR), 0, 0, (PVOID*)&DeviceExtension->DeviceDescriptor);
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
     Status = USBSTOR_GetDescriptor(DeviceExtension->LowerDeviceObject, USB_CONFIGURATION_DESCRIPTOR_TYPE, sizeof(USB_CONFIGURATION_DESCRIPTOR), 0, 0, (PVOID*)&DeviceExtension->ConfigurationDescriptor);
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
     Status = USBSTOR_GetDescriptor(DeviceExtension->LowerDeviceObject, USB_CONFIGURATION_DESCRIPTOR_TYPE, DescriptorLength, 0, 0, (PVOID*)&DeviceExtension->ConfigurationDescriptor);
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
     // check if there is a serial number provided
     //
     if (DeviceExtension->DeviceDescriptor->iSerialNumber)
     {
         //
         // get serial number
         //
         Status = USBSTOR_GetDescriptor(DeviceExtension->LowerDeviceObject, USB_STRING_DESCRIPTOR_TYPE, 100 * sizeof(WCHAR), DeviceExtension->DeviceDescriptor->iSerialNumber, 0x0409, (PVOID*)&DeviceExtension->SerialNumber);
         if (!NT_SUCCESS(Status))
         {
             //
             // failed to get serial number descriptor, free device descriptor
             //
             FreeItem(DeviceExtension->DeviceDescriptor);
             DeviceExtension->DeviceDescriptor = NULL;

             //
             // free configuration descriptor
             //
             FreeItem(DeviceExtension->ConfigurationDescriptor);
             DeviceExtension->ConfigurationDescriptor = NULL;

             //
             // set serial number to zero
             //
             DeviceExtension->SerialNumber = NULL;
             return Status;
          }
     }

     return Status;
}

NTSTATUS
NTAPI
USBSTOR_ScanConfigurationDescriptor(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    OUT PUSB_INTERFACE_DESCRIPTOR * OutInterfaceDescriptor,
    OUT PUSB_ENDPOINT_DESCRIPTOR  * InEndpointDescriptor,
    OUT PUSB_ENDPOINT_DESCRIPTOR  * OutEndpointDescriptor)
{
    PUSB_CONFIGURATION_DESCRIPTOR CurrentDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor;

    //
    // sanity checks
    //
    ASSERT(ConfigurationDescriptor);
    ASSERT(OutInterfaceDescriptor);
    ASSERT(InEndpointDescriptor);
    ASSERT(OutEndpointDescriptor);

    //
    // nullify pointers
    //
    *OutInterfaceDescriptor = NULL;
    *InEndpointDescriptor = NULL;
    *OutEndpointDescriptor = NULL;

    //
    // start scanning
    //
    CurrentDescriptor = ConfigurationDescriptor;

    do
    {
        //
        // check current descriptor type
        //
        if (CurrentDescriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
        {
            //
            // found interface descriptor
            //
            if (*OutInterfaceDescriptor)
            {
                //
                // we only process the first interface descriptor as ms does -> see documentation
                //
                break;
            }

            //
            // store interface descriptor
            //
            *OutInterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)CurrentDescriptor;
        }
        else if (CurrentDescriptor->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE)
        {
            //
            // convert to endpoint descriptor
            //
            EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)CurrentDescriptor;

            //
            // sanity check
            //
            ASSERT(*OutInterfaceDescriptor);

            //
            // get endpoint type
            //
            if ((EndpointDescriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK)
            {
                 //
                 // bulk endpoint type
                 //
                 if (USB_ENDPOINT_DIRECTION_IN(EndpointDescriptor->bEndpointAddress))
                 {
                     //
                     // bulk in
                     //
                     *InEndpointDescriptor = EndpointDescriptor;
                 }
                 else
                 {
                     //
                     // bulk out
                     //
                     *OutEndpointDescriptor = EndpointDescriptor;
                 }
            }
            else if ((EndpointDescriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_INTERRUPT)
            {
                 //
                 // interrupt endpoint type
                 //
                 UNIMPLEMENTED;
            }
        }

        //
        // move to next descriptor
        //
        CurrentDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)((ULONG_PTR)CurrentDescriptor + CurrentDescriptor->bLength);

        //
        // was it the last descriptor
        //
        if ((ULONG_PTR)CurrentDescriptor >= ((ULONG_PTR)ConfigurationDescriptor + ConfigurationDescriptor->wTotalLength))
        {
            //
            // reached last descriptor
            //
            break;
        }

    }while(TRUE);

    //
    // check if everything has been found
    //
    if (*OutInterfaceDescriptor == NULL || *InEndpointDescriptor == NULL || *OutEndpointDescriptor == NULL)
    {
        //
        // failed to find interface / endpoint descriptor
        //
        DPRINT1("USBSTOR_ScanConfigurationDescriptor: Failed to find InterfaceDescriptor %p InEndpointDescriptor %p OutEndpointDescriptor %p\n", *OutInterfaceDescriptor, *InEndpointDescriptor, *OutEndpointDescriptor);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // completed successfully
    //
    return STATUS_SUCCESS;
}

VOID
DumpConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    DPRINT1("Dumping ConfigurationDescriptor %p\n", ConfigurationDescriptor);
    DPRINT1("bLength %x\n", ConfigurationDescriptor->bLength);
    DPRINT1("bDescriptorType %x\n", ConfigurationDescriptor->bDescriptorType);
    DPRINT1("wTotalLength %x\n", ConfigurationDescriptor->wTotalLength);
    DPRINT1("bNumInterfaces %x\n", ConfigurationDescriptor->bNumInterfaces);
    DPRINT1("bConfigurationValue %x\n", ConfigurationDescriptor->bConfigurationValue);
    DPRINT1("iConfiguration %x\n", ConfigurationDescriptor->iConfiguration);
    DPRINT1("bmAttributes %x\n", ConfigurationDescriptor->bmAttributes);
    DPRINT1("MaxPower %x\n", ConfigurationDescriptor->MaxPower);
}

NTSTATUS
USBSTOR_SelectConfigurationAndInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension)
{
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR InEndpointDescriptor, OutEndpointDescriptor;
    NTSTATUS Status;
    PURB Urb;
    PUSBD_INTERFACE_LIST_ENTRY InterfaceList;

    //
    // now scan configuration descriptors
    //
    Status = USBSTOR_ScanConfigurationDescriptor(DeviceExtension->ConfigurationDescriptor, &InterfaceDescriptor, &InEndpointDescriptor, &OutEndpointDescriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to scan
        //
        return Status;
    }

    //
    // now allocate one interface entry and terminating null entry
    //
    InterfaceList = (PUSBD_INTERFACE_LIST_ENTRY)AllocateItem(PagedPool, sizeof(USBD_INTERFACE_LIST_ENTRY) * 2);
    if (!InterfaceList)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize interface list entry
    //
    InterfaceList[0].InterfaceDescriptor = InterfaceDescriptor;

    //
    // now allocate the urb
    //
    Urb = USBD_CreateConfigurationRequestEx(DeviceExtension->ConfigurationDescriptor, InterfaceList);
    if (!Urb)
    {
        //
        // no memory
        //
        FreeItem(InterfaceList);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // sanity check
    //
    ASSERT(InterfaceList[0].Interface);

    //
    // submit urb
    //
    Status = USBSTOR_SyncUrbRequest(DeviceExtension->LowerDeviceObject, Urb);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to set configuration
        //
        DPRINT1("USBSTOR_SelectConfiguration failed to set interface %x\n", Status);
        FreeItem(InterfaceList);
        ExFreePoolWithTag(Urb, 0);
        return Status;
    }

    //
    // backup interface information
    //
    DeviceExtension->InterfaceInformation = (PUSBD_INTERFACE_INFORMATION)AllocateItem(NonPagedPool, Urb->UrbSelectConfiguration.Interface.Length);
    if (!DeviceExtension->InterfaceInformation)
    {
        //
        // failed to allocate interface information structure
        //
        FreeItem(InterfaceList);
        ExFreePoolWithTag(Urb, 0);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // copy interface information
    //
    RtlCopyMemory(DeviceExtension->InterfaceInformation, &Urb->UrbSelectConfiguration.Interface, Urb->UrbSelectConfiguration.Interface.Length);

    //
    // store pipe handle
    //
    DeviceExtension->ConfigurationHandle = Urb->UrbSelectConfiguration.ConfigurationHandle;

    //
    // now prepare interface urb
    //
    UsbBuildSelectInterfaceRequest(Urb, GET_SELECT_INTERFACE_REQUEST_SIZE(InterfaceDescriptor->bNumEndpoints), DeviceExtension->ConfigurationHandle, InterfaceDescriptor->bInterfaceNumber, InterfaceDescriptor->bAlternateSetting);

    //
    // copy interface information structure back - as offset for SelectConfiguration / SelectInterface request do differ
    //
    RtlCopyMemory(&Urb->UrbSelectInterface.Interface, DeviceExtension->InterfaceInformation, DeviceExtension->InterfaceInformation->Length);

    //
    // now select the interface
    //
    Status = USBSTOR_SyncUrbRequest(DeviceExtension->LowerDeviceObject, Urb);

    //
    // did it succeed
    //
    if (NT_SUCCESS(Status))
    {
        //
        // update configuration info
        //
        ASSERT(Urb->UrbSelectInterface.Interface.Length == DeviceExtension->InterfaceInformation->Length);
        RtlCopyMemory(DeviceExtension->InterfaceInformation, &Urb->UrbSelectInterface.Interface, Urb->UrbSelectInterface.Interface.Length);
    }

    //
    // free interface list & urb
    //
    FreeItem(InterfaceList);
    ExFreePoolWithTag(Urb, 0);

    //
    // done
    //
    return Status;
}

NTSTATUS
USBSTOR_GetPipeHandles(
    IN PFDO_DEVICE_EXTENSION DeviceExtension)
{
    ULONG Index;
    BOOLEAN BulkInFound = FALSE, BulkOutFound = FALSE;

    //
    // no enumerate all pipes and extract bulk-in / bulk-out pipe handle
    //
    for(Index = 0; Index < DeviceExtension->InterfaceInformation->NumberOfPipes; Index++)
    {
        //
        // check pipe type
        //
        if (DeviceExtension->InterfaceInformation->Pipes[Index].PipeType == UsbdPipeTypeBulk)
        {
            //
            // check direction
            //
            if (USB_ENDPOINT_DIRECTION_IN(DeviceExtension->InterfaceInformation->Pipes[Index].EndpointAddress))
            {
                //
                // bulk in pipe
                //
                DeviceExtension->BulkInPipeIndex = Index;

                //
                // there should not be another bulk in pipe
                //
                ASSERT(BulkInFound == FALSE);
                BulkInFound = TRUE;
            }
            else
            {
                //
                // bulk out pipe
                //
                DeviceExtension->BulkOutPipeIndex = Index;

                //
                // there should not be another bulk out pipe
                //
                ASSERT(BulkOutFound == FALSE);
                BulkOutFound = TRUE;
            }
        }
    }

    //
    // check if both bulk pipes have been found
    //
    if (!BulkInFound || !BulkOutFound)
    {
        //
        // WTF? usb port driver does not give us bulk pipe access
        //
        DPRINT1("USBSTOR_GetPipeHandles> BulkInFound %c BulkOutFound %c missing!!!\n", BulkInFound, BulkOutFound);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    //
    // device is configured
    //
    return STATUS_SUCCESS;
}
