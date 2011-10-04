/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/usbiffn.c
 * PURPOSE:     Direct Call Interface Functions.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 */

/* Many of these direct calls are documented on http://www.osronline.com */

#include "usbehci.h"
#include <hubbusif.h>
#include <usbbusif.h>
#include "hardware.h"
#include "transfer.h"

PVOID InternalCreateUsbDevice(ULONG Port, PUSB_DEVICE Parent, BOOLEAN Hub)
{
    PUSB_DEVICE UsbDevicePointer = NULL;

    UsbDevicePointer = ExAllocatePoolWithTag(NonPagedPool, sizeof(USB_DEVICE), USB_POOL_TAG);

    if (!UsbDevicePointer)
    {
        DPRINT1("Out of memory\n");
        return NULL;
    }

    RtlZeroMemory(UsbDevicePointer, sizeof(USB_DEVICE));

    if ((Hub) && (!Parent))
    {
        DPRINT1("This is the root hub\n");
    }

    UsbDevicePointer->Port = Port - 1;
    UsbDevicePointer->ParentDevice = Parent;

    UsbDevicePointer->IsHub = Hub;

    return UsbDevicePointer;
}

VOID
USB_BUSIFFN
InterfaceReference(PVOID BusContext)
{
    DPRINT1("Ehci: InterfaceReference called\n");
}

VOID
USB_BUSIFFN
InterfaceDereference(PVOID BusContext)
{
    DPRINT1("Ehci: InterfaceDereference called\n");
}

/* Bus Interface Hub V5 Functions */


/* Hub Driver calls this routine for each new device it is informed about on USB Bus
   osronline documents that this is where the device address is assigned. It also
   states the same for InitializeUsbDevice. This function only gets the device descriptor 
   from the device and checks that the members for device are correct. */

NTSTATUS
USB_BUSIFFN
CreateUsbDevice(PVOID BusContext,
                PUSB_DEVICE_HANDLE *NewDevice,
                PUSB_DEVICE_HANDLE HubDeviceHandle,
                USHORT PortStatus, USHORT PortNumber)
{
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    PEHCI_HOST_CONTROLLER hcd;
    PUSB_DEVICE UsbDevice;
    LONG i;

    DPRINT1("Ehci: CreateUsbDevice: HubDeviceHandle %x, PortStatus %x, PortNumber %x\n", HubDeviceHandle, PortStatus, PortNumber);
    
    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)BusContext)->DeviceExtension;
    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)PdoDeviceExtension->ControllerFdo->DeviceExtension;

    hcd = &FdoDeviceExtension->hcd;

    if (PdoDeviceExtension->UsbDevices[0] != HubDeviceHandle)
    {
        DPRINT1("Not a valid HubDeviceHandle\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    UsbDevice = NULL;
    /* Add it to the list */
    for (i=0; i < MAX_USB_DEVICES; i++)
    {
        if (PdoDeviceExtension->UsbDevices[i] == NULL)
        {
            PdoDeviceExtension->UsbDevices[i] = InternalCreateUsbDevice(PortNumber, HubDeviceHandle, FALSE);
            
            if (!PdoDeviceExtension->UsbDevices[i])
                return STATUS_INSUFFICIENT_RESOURCES;
            UsbDevice = PdoDeviceExtension->UsbDevices[i];
            break;
        }
    }

    /* Check that a device was created */
    if (!UsbDevice)
    {
        DPRINT1("Too many usb devices attached. Max is %d\n", MAX_USB_DEVICES);
        return STATUS_UNSUCCESSFUL;
    }

    hcd->Ports[PortNumber - 1].PortStatus = PortStatus;

    /* Get the device descriptor */
    CtrlSetup.bRequest = USB_REQUEST_GET_DESCRIPTOR;
    CtrlSetup.wValue.LowByte = 0;
    CtrlSetup.wValue.HiByte = USB_DEVICE_DESCRIPTOR_TYPE;
    CtrlSetup.wIndex.W = 0;
    CtrlSetup.wLength = sizeof(USB_DEVICE_DESCRIPTOR);    
    CtrlSetup.bmRequestType.B = 0x80;
    ExecuteTransfer(FdoDeviceExtension->DeviceObject,
                    UsbDevice,
                    0,
                    &CtrlSetup,
                    0,
                    &UsbDevice->DeviceDescriptor,
                    sizeof(USB_DEVICE_DESCRIPTOR),
                    NULL);

    /* Check status and bLength and bDescriptor members */
    if ((UsbDevice->DeviceDescriptor.bLength != 0x12) || (UsbDevice->DeviceDescriptor.bDescriptorType != 0x1))
    {
        return STATUS_DEVICE_DATA_ERROR;
    }

    DumpDeviceDescriptor(&UsbDevice->DeviceDescriptor);

    /* Return it */
    *NewDevice = UsbDevice;
    return STATUS_SUCCESS;
}

/* Assigns the device an address, gets the configuration, interface, and endpoint descriptors
   from the device. All this data is saved as part of this driver */

NTSTATUS
USB_BUSIFFN
InitializeUsbDevice(PVOID BusContext, PUSB_DEVICE_HANDLE DeviceHandle)
{
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    USB_DEVICE_DESCRIPTOR DeviceDesc;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDesc;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDesc;
    PUSB_DEVICE UsbDevice;
    PVOID Buffer;
    PUCHAR Ptr;
    UCHAR NewAddress = 0;

    LONG i, j, k, InitAttept;

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)BusContext)->DeviceExtension;
    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)PdoDeviceExtension->ControllerFdo->DeviceExtension;

    UsbDevice = DeviceHandleToUsbDevice(BusContext, DeviceHandle);

    if (!UsbDevice)
    {
        DPRINT1("Invalid DeviceHandle or device not connected\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    for (i=0; i<127; i++)
    {
        if (UsbDevice == PdoDeviceExtension->UsbDevices[i])
        {
            NewAddress = i;
            break;
        }
    }

    ASSERT(NewAddress);

    /* Linux drivers make 3 attemps to set the device address because of problems with some devices. Do the same */
    InitAttept = 0;
    while (InitAttept < 3)
    {
        /* Set the device address */
        CtrlSetup.bmRequestType.B = 0x00;
        CtrlSetup.bRequest = USB_REQUEST_SET_ADDRESS;
        CtrlSetup.wValue.W = NewAddress;
        CtrlSetup.wIndex.W = 0;
        CtrlSetup.wLength = 0;

        DPRINT1("Setting Address to %x\n", NewAddress);
        ExecuteTransfer(PdoDeviceExtension->ControllerFdo,
                        UsbDevice,
                        0,
                        &CtrlSetup,
                        0,
                        NULL,
                        0,
                        NULL);

        KeStallExecutionProcessor(300 * InitAttept);

        /* Send 0 length packet to endpoint 0 for ack */
/*
        ExecuteTransfer(PdoDeviceExtension->ControllerFdo,
                        UsbDevice,
                        0,
                        NULL,
                        0,
                        NULL,
                        0,
                        NULL);
*/

        CtrlSetup.bRequest = USB_REQUEST_GET_DESCRIPTOR;
        CtrlSetup.wValue.LowByte = 0;
        CtrlSetup.wValue.HiByte = USB_DEVICE_DESCRIPTOR_TYPE;
        CtrlSetup.wIndex.W = 0;
        CtrlSetup.wLength = sizeof(USB_DEVICE_DESCRIPTOR);    
        CtrlSetup.bmRequestType.B = 0x80;

        UsbDevice->Address = NewAddress;
        ExecuteTransfer(FdoDeviceExtension->DeviceObject,
                        UsbDevice,
                        0,
                        &CtrlSetup,
                        0,
                        &DeviceDesc,
                        sizeof(USB_DEVICE_DESCRIPTOR),
                        NULL);

        DPRINT1("Length %d, DescriptorType %d\n", DeviceDesc.bLength, DeviceDesc.bDescriptorType);
        if ((DeviceDesc.bLength == 0x12) && (DeviceDesc.bDescriptorType == 0x01))
            break;
        
        /* If the descriptor was not gotten */
        UsbDevice->Address = 0;
        InitAttept++;
    }
    
    if (InitAttept == 3)
    {
        DPRINT1("Unable to initialize usb device connected on port %d!\n", UsbDevice->Port);
        /* FIXME: Should the memory allocated for this device be deleted? */
        return STATUS_DEVICE_DATA_ERROR;
    }
    DumpDeviceDescriptor(&DeviceDesc);

    if (UsbDevice->DeviceDescriptor.bNumConfigurations == 0)
    {
        DPRINT1("Device on port %d has no configurations!\n", UsbDevice->Port);
        /* FIXME: Should the memory allocated for this device be deleted? */
        return STATUS_DEVICE_DATA_ERROR;
    }
    UsbDevice->Configs = ExAllocatePoolWithTag(NonPagedPool,
                                               sizeof(PVOID) * UsbDevice->DeviceDescriptor.bNumConfigurations,
                                               USB_POOL_TAG);

    if (!UsbDevice->Configs)
    {
        DPRINT1("Out of memory\n");
        /* FIXME: Should the memory allocated for this device be deleted? */
        return STATUS_NO_MEMORY;
    }

    Buffer = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, USB_POOL_TAG);

    if (!Buffer)
    {
        DPRINT1("Out of memory\n");
        /* FIXME: Should the memory allocated for this device be deleted? */
        return STATUS_NO_MEMORY;
    }

    Ptr = Buffer;

    for (i = 0; i < UsbDevice->DeviceDescriptor.bNumConfigurations; i++)
    {
        /* Get the Device Configuration Descriptor */
        CtrlSetup.bmRequestType._BM.Recipient = BMREQUEST_TO_DEVICE;
        CtrlSetup.bmRequestType._BM.Type = BMREQUEST_STANDARD;
        CtrlSetup.bmRequestType._BM.Reserved = 0;
        CtrlSetup.bmRequestType._BM.Dir = BMREQUEST_DEVICE_TO_HOST;
        CtrlSetup.bRequest = USB_REQUEST_GET_DESCRIPTOR;
        CtrlSetup.wValue.LowByte = 0;
        CtrlSetup.wValue.HiByte = USB_CONFIGURATION_DESCRIPTOR_TYPE;
        CtrlSetup.wIndex.W = 0;
        CtrlSetup.wLength = PAGE_SIZE;
        ExecuteTransfer(PdoDeviceExtension->ControllerFdo,
                        UsbDevice,
                        0,
                        &CtrlSetup,
                        0,
                        Buffer,
                        PAGE_SIZE,
                        NULL);

        ConfigDesc = (PUSB_CONFIGURATION_DESCRIPTOR)Ptr;

        DumpFullConfigurationDescriptor(ConfigDesc);
        ASSERT(ConfigDesc->wTotalLength <= PAGE_SIZE);

        UsbDevice->Configs[i] = ExAllocatePoolWithTag(NonPagedPool,
                                                      sizeof(USB_CONFIGURATION) + sizeof(PVOID) * ConfigDesc->bNumInterfaces,
                                                      USB_POOL_TAG);
        UsbDevice->Configs[i]->Device = UsbDevice;

        RtlCopyMemory(&UsbDevice->Configs[0]->ConfigurationDescriptor,
                      ConfigDesc, sizeof(USB_CONFIGURATION_DESCRIPTOR));
        Ptr += ConfigDesc->bLength;

        for (j = 0; j < ConfigDesc->bNumInterfaces; j++)
        {
            InterfaceDesc = (PUSB_INTERFACE_DESCRIPTOR) Ptr;
            UsbDevice->Configs[i]->Interfaces[j] = ExAllocatePoolWithTag(NonPagedPool,
                                                                         sizeof(USB_INTERFACE) + sizeof(PVOID) * InterfaceDesc->bNumEndpoints,
                                                                         USB_POOL_TAG);
            RtlCopyMemory(&UsbDevice->Configs[i]->Interfaces[j]->InterfaceDescriptor,
                          InterfaceDesc,
                          sizeof(USB_INTERFACE_DESCRIPTOR));

            Ptr += InterfaceDesc->bLength;

            for (k = 0; k < InterfaceDesc->bNumEndpoints; k++)
            {
                EndpointDesc = (PUSB_ENDPOINT_DESCRIPTOR)Ptr;
                UsbDevice->Configs[i]->Interfaces[j]->EndPoints[k] = ExAllocatePoolWithTag(NonPagedPool, sizeof(USB_ENDPOINT), USB_POOL_TAG);
                RtlCopyMemory(&UsbDevice->Configs[i]->Interfaces[j]->EndPoints[k]->EndPointDescriptor,
                              EndpointDesc, sizeof(USB_ENDPOINT_DESCRIPTOR));
                Ptr += sizeof(USB_ENDPOINT_DESCRIPTOR);
            }
        }
    }

    UsbDevice->ActiveConfig = UsbDevice->Configs[0];
    UsbDevice->ActiveInterface = UsbDevice->Configs[0]->Interfaces[0];

    UsbDevice->DeviceState = DEVICEINTIALIZED;

    return STATUS_SUCCESS;
}

/* Return the descriptors that will fit. Descriptors were saved when the InitializeUsbDevice function was called */
NTSTATUS
USB_BUSIFFN
GetUsbDescriptors(PVOID BusContext,
                  PUSB_DEVICE_HANDLE DeviceHandle,
                  PUCHAR DeviceDescriptorBuffer,
                  PULONG DeviceDescriptorBufferLength,
                  PUCHAR ConfigDescriptorBuffer,
                  PULONG ConfigDescriptorBufferLength)
{
    PUSB_DEVICE UsbDevice;
    DPRINT1("Ehci: GetUsbDescriptor %x, %x, %x, %x\n", DeviceDescriptorBuffer, *DeviceDescriptorBufferLength,
                                                       ConfigDescriptorBuffer, *ConfigDescriptorBufferLength);

    UsbDevice = DeviceHandleToUsbDevice(BusContext, DeviceHandle);

    if (!UsbDevice)
    {
        DPRINT1("Invalid DeviceHandle or device not connected\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if ((DeviceDescriptorBuffer) && (DeviceDescriptorBufferLength))
    {
        RtlCopyMemory(DeviceDescriptorBuffer, &UsbDevice->DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));
        *DeviceDescriptorBufferLength = sizeof(USB_DEVICE_DESCRIPTOR);
    }

    if ((ConfigDescriptorBuffer) && (ConfigDescriptorBufferLength))
    {
        RtlCopyMemory(ConfigDescriptorBuffer, &UsbDevice->ActiveConfig->ConfigurationDescriptor, sizeof(USB_CONFIGURATION_DESCRIPTOR));
        *ConfigDescriptorBufferLength = sizeof(USB_CONFIGURATION_DESCRIPTOR);
    }

    return STATUS_SUCCESS;
}

/* Documented http://www.osronline.com/ddkx/buses/usbinterkr_1m7m.htm */
NTSTATUS
USB_BUSIFFN
RemoveUsbDevice(PVOID BusContext, PUSB_DEVICE_HANDLE DeviceHandle, ULONG Flags)
{
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    PUSB_DEVICE UsbDevice;
    LONG i, j, k;

    DPRINT1("RemoveUsbDevice called, DeviceHandle %x, Flags %x\n", DeviceHandle, Flags);

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)BusContext)->DeviceExtension;

    UsbDevice = DeviceHandleToUsbDevice(BusContext, DeviceHandle);

    if (!UsbDevice)
    {
        DPRINT1("Invalid DeviceHandle or device not connected\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    switch (Flags)
    {
       case 0:
            DPRINT1("Number of Configurations %d\n", UsbDevice->DeviceDescriptor.bNumConfigurations);
            for (i = 0; i < UsbDevice->DeviceDescriptor.bNumConfigurations; i++)
            {
                for (j = 0; j < UsbDevice->Configs[i]->ConfigurationDescriptor.bNumInterfaces; j++)
                {
                    for (k = 0; k < UsbDevice->Configs[i]->Interfaces[j]->InterfaceDescriptor.bNumEndpoints; k++)
                    {
                        ExFreePool(UsbDevice->Configs[i]->Interfaces[j]->EndPoints[k]);
                    }
                    ExFreePool(UsbDevice->Configs[i]->Interfaces[j]);
                }
                ExFreePool(UsbDevice->Configs[i]);
            }

            for (i = 0; i < 127; i++)
            {
                if (PdoDeviceExtension->UsbDevices[i] == UsbDevice)
                    PdoDeviceExtension->UsbDevices[i] = NULL;
            }

            ExFreePool(UsbDevice);
            break;
        case USBD_MARK_DEVICE_BUSY:
            UsbDevice->DeviceState |= DEVICEBUSY;
            /* Fall through */
        case USBD_KEEP_DEVICE_DATA:
            UsbDevice->DeviceState |= DEVICEREMOVED;
            break;
        default:
            DPRINT1("Unknown Remove Flags %x\n", Flags);
    }
    return STATUS_SUCCESS;
}

/* Documented at http://www.osronline.com/ddkx/buses/usbinterkr_01te.htm */
NTSTATUS
USB_BUSIFFN
RestoreUsbDevice(PVOID BusContext, PUSB_DEVICE_HANDLE OldDeviceHandle, PUSB_DEVICE_HANDLE NewDeviceHandle)
{
    PUSB_DEVICE OldUsbDevice;
    PUSB_DEVICE NewUsbDevice;
    PUSB_CONFIGURATION ConfigToDelete;
    int i;

    DPRINT1("Ehci: RestoreUsbDevice %x, %x, %x\n", BusContext, OldDeviceHandle, NewDeviceHandle);
ASSERT(FALSE);
    OldUsbDevice = DeviceHandleToUsbDevice(BusContext, OldDeviceHandle);
    NewUsbDevice = DeviceHandleToUsbDevice(BusContext, NewDeviceHandle);

    if (!OldUsbDevice)
    {
        DPRINT1("OldDeviceHandle is invalid\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if (!(OldUsbDevice->DeviceState & DEVICEREMOVED))
    {
        DPRINT1("UsbDevice is not marked as Removed!\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (!NewUsbDevice)
    {
        DPRINT1("NewDeviceHandle is invalid\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if ((OldUsbDevice->DeviceDescriptor.idVendor == NewUsbDevice->DeviceDescriptor.idVendor) &&
        (OldUsbDevice->DeviceDescriptor.idProduct == NewUsbDevice->DeviceDescriptor.idProduct))
    {
        NewUsbDevice->DeviceState &= ~DEVICEBUSY;
        NewUsbDevice->DeviceState &= ~DEVICEREMOVED;

        NewUsbDevice->ActiveConfig = OldUsbDevice->ActiveConfig;
        NewUsbDevice->ActiveInterface = OldUsbDevice->ActiveInterface;

        for (i = 0; i < NewUsbDevice->DeviceDescriptor.bNumConfigurations; i++)
        {
            ConfigToDelete = NewUsbDevice->Configs[i];
            ASSERT(OldUsbDevice->Configs[i]);
            NewUsbDevice->Configs[i] = OldUsbDevice->Configs[i];
            OldUsbDevice->Configs[i] = ConfigToDelete;
        }

        RemoveUsbDevice(BusContext, OldDeviceHandle, 0);
        return STATUS_SUCCESS;
    }
    else
    {
        DPRINT1("VendorId or ProductId did not match!\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }
}

/* FIXME: Research this */
NTSTATUS
USB_BUSIFFN
GetPortHackFlags(PVOID BusContext, PULONG Flags)
{
    DPRINT1("Ehci: GetPortHackFlags not implemented. %x, %x\n", BusContext, Flags);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
USB_BUSIFFN
QueryDeviceInformation(PVOID BusContext,
                       PUSB_DEVICE_HANDLE DeviceHandle,
                       PVOID DeviceInformationBuffer,
                       ULONG DeviceInformationBufferLength,
                       PULONG LengthReturned)
{
    PUSB_DEVICE_INFORMATION_0 DeviceInfo = DeviceInformationBuffer;
    PUSB_DEVICE UsbDevice;
    ULONG SizeNeeded;
    LONG i;

    DPRINT1("Ehci: QueryDeviceInformation (%x, %x, %x, %d, %x\n", BusContext, DeviceHandle, DeviceInformationBuffer,
                                                                  DeviceInformationBufferLength, LengthReturned);

    UsbDevice = DeviceHandleToUsbDevice(BusContext, DeviceHandle);

    if (!UsbDevice)
    {
        DPRINT1("Invalid DeviceHandle or device not connected\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    SizeNeeded = FIELD_OFFSET(USB_DEVICE_INFORMATION_0, PipeList[UsbDevice->ActiveInterface->InterfaceDescriptor.bNumEndpoints]);
    *LengthReturned = SizeNeeded;

    DeviceInfo->ActualLength = SizeNeeded;

    if (DeviceInformationBufferLength < SizeNeeded)
    {
        DPRINT1("Buffer to small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (DeviceInfo->InformationLevel != 0)
    {
        DPRINT1("Invalid Param\n");
        return STATUS_INVALID_PARAMETER;
    }

    DeviceInfo->HubAddress = 0;
    DeviceInfo->DeviceAddress = UsbDevice->Address;
    DeviceInfo->DeviceSpeed = UsbDevice->DeviceSpeed;
    DeviceInfo->DeviceType = UsbDevice->DeviceType;

    if (!UsbDevice->DeviceState)
    {
        DeviceInfo->CurrentConfigurationValue = 0;
        DeviceInfo->NumberOfOpenPipes = 0;
        DeviceInfo->PortNumber = 0;
    }
    else
    {
        DeviceInfo->CurrentConfigurationValue = UsbDevice->ActiveConfig->ConfigurationDescriptor.bConfigurationValue;
        /* FIXME: Use correct number of open pipes instead of all available */
        DeviceInfo->NumberOfOpenPipes = UsbDevice->ActiveInterface->InterfaceDescriptor.bNumEndpoints;
        DeviceInfo->PortNumber = UsbDevice->Port;
    }

    RtlCopyMemory(&DeviceInfo->DeviceDescriptor, &UsbDevice->DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));

    for (i = 0; i < UsbDevice->ActiveInterface->InterfaceDescriptor.bNumEndpoints; i++)
    {
        RtlCopyMemory(&DeviceInfo->PipeList[i].EndpointDescriptor,
                      &UsbDevice->ActiveInterface->EndPoints[i]->EndPointDescriptor,
                      sizeof(USB_ENDPOINT_DESCRIPTOR));
    }
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
GetControllerInformation(PVOID BusContext,
                         PVOID ControllerInformationBuffer,
                         ULONG ControllerInformationBufferLength,
                         PULONG LengthReturned)
{
    PUSB_CONTROLLER_INFORMATION_0 ControllerInfo;

    DPRINT1("Ehci: GetControllerInformation called\n");

    if (!LengthReturned)
        return STATUS_INVALID_PARAMETER;

    ControllerInfo = ControllerInformationBuffer;

    if (ControllerInformationBufferLength < sizeof(USB_CONTROLLER_INFORMATION_0))
    {
        DPRINT1("Buffer to small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (ControllerInfo->InformationLevel != 0)
    {
        DPRINT1("InformationLevel other than 0 not supported\n");
        return STATUS_NOT_SUPPORTED;
    }

    ControllerInfo->ActualLength = sizeof(USB_CONTROLLER_INFORMATION_0);
    ControllerInfo->SelectiveSuspendEnabled = FALSE;
    ControllerInfo->IsHighSpeedController = TRUE;

    *LengthReturned = ControllerInfo->ActualLength;

    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
ControllerSelectiveSuspend(PVOID BusContext, BOOLEAN Enable)
{
    DPRINT1("Ehci: ControllerSelectiveSuspend not implemented\n");
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
USB_BUSIFFN
GetExtendedHubInformation(PVOID BusContext,
                          PDEVICE_OBJECT HubPhysicalDeviceObject,
                          PVOID HubInformationBuffer,
                          ULONG HubInformationBufferLength,
                          PULONG LengthReturned)
{

    PUSB_EXTHUB_INFORMATION_0 UsbExtHubInfo = HubInformationBuffer;
    PPDO_DEVICE_EXTENSION PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)BusContext)->DeviceExtension;
    PFDO_DEVICE_EXTENSION FdoDeviceExntension = (PFDO_DEVICE_EXTENSION)PdoDeviceExtension->ControllerFdo->DeviceExtension;
    LONG i;
    DPRINT1("Ehci: GetExtendedHubInformation BusContext %x, PDO %x, InformationBuffer %x\n", 
            BusContext, HubPhysicalDeviceObject, HubInformationBuffer);

    /* Set the default return value */
    *LengthReturned = 0;

    DPRINT("InformationLevel %x\n", UsbExtHubInfo->InformationLevel);

    /* Caller is suppose to have set InformationLevel to 0. However usbehci from MS seems to ignore this */
    if (UsbExtHubInfo->InformationLevel != 0)
    {
        DPRINT1("InformationLevel should really be set to 0. Ignoring\n");
    }

    UsbExtHubInfo->NumberOfPorts = FdoDeviceExntension->hcd.ECHICaps.HCSParams.PortCount;

    for (i=0; i < UsbExtHubInfo->NumberOfPorts; i++)
    {
        UsbExtHubInfo->Port[i].PhysicalPortNumber = i + 1;
        UsbExtHubInfo->Port[i].PortLabelNumber = i + 1;
        UsbExtHubInfo->Port[i].VidOverride = 0;
        UsbExtHubInfo->Port[i].PidOverride = 0;
        UsbExtHubInfo->Port[i].PortAttributes = USB_PORTATTR_SHARED_USB2;// | USB_PORTATTR_OWNED_BY_CC;
    }

    *LengthReturned = FIELD_OFFSET(USB_EXTHUB_INFORMATION_0, Port[8]);

    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
GetRootHubSymbolicName(PVOID BusContext,
                       PVOID HubSymNameBuffer,
                       ULONG HubSymNameBufferLength,
                       PULONG HubSymNameActualLength)
{
    DPRINT1("Ehci: GetRootHubSymbolicName called\n");

    if (HubSymNameBufferLength < 16)
        return STATUS_UNSUCCESSFUL;
    RtlCopyMemory(HubSymNameBuffer, L"ROOT_HUB", HubSymNameBufferLength);
    *HubSymNameActualLength = 16;

    return STATUS_SUCCESS;
}

PVOID
USB_BUSIFFN
GetDeviceBusContext(PVOID HubBusContext, PVOID DeviceHandle)
{
    PUSB_DEVICE UsbDevice;

    DPRINT1("Ehci: GetDeviceBusContext called\n");
    UsbDevice = DeviceHandleToUsbDevice(HubBusContext, DeviceHandle);

    if (!UsbDevice)
    {
        DPRINT1("Invalid DeviceHandle or device not connected\n");
        return NULL;
    }

    return NULL;
}

NTSTATUS
USB_BUSIFFN
Initialize20Hub(PVOID BusContext, PUSB_DEVICE_HANDLE HubDeviceHandle, ULONG TtCount)
{
    DPRINT1("Ehci: Initialize20Hub called, HubDeviceHandle: %x, TtCount %x\n", HubDeviceHandle, TtCount);
    /* FIXME: */
    /* Create the Irp Queue for SCE */
    /* Should queue be created for each device or each enpoint??? */
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
RootHubInitNotification(PVOID BusContext, PVOID CallbackContext, PRH_INIT_CALLBACK CallbackRoutine)
{
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    DPRINT1("Ehci: RootHubInitNotification %x, %x, %x\n", BusContext, CallbackContext, CallbackRoutine);

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)BusContext)->DeviceExtension;
    PdoDeviceExtension->CallbackContext = CallbackContext;
    PdoDeviceExtension->CallbackRoutine = CallbackRoutine;
    if (PdoDeviceExtension->CallbackRoutine)
    {
        DPRINT1("Called Callbackrountine\n");
        PdoDeviceExtension->CallbackRoutine(PdoDeviceExtension->CallbackContext);
        DPRINT1("Done Callbackrountine\n");
    }
    else
    {
        DPRINT1("PdoDeviceExtension->CallbackRoutine is NULL!\n");
    }

    return STATUS_SUCCESS;
}

VOID
USB_BUSIFFN
FlushTransfers(PVOID BusContext, PVOID DeviceHandle)
{
    PUSB_DEVICE UsbDevice;
    UsbDevice = DeviceHandleToUsbDevice(BusContext, DeviceHandle);

    if (!UsbDevice)
    {
        DPRINT1("Invalid DeviceHandle or device not connected\n");
    }

    DPRINT1("FlushTransfers not implemented.\n");
}

VOID
USB_BUSIFFN
SetDeviceHandleData(PVOID BusContext, PVOID DeviceHandle, PDEVICE_OBJECT UsbDevicePdo)
{
    PUSB_DEVICE UsbDevice;
    
    DPRINT1("Ehci: SetDeviceHandleData %x, %x, %x\n", BusContext, DeviceHandle, UsbDevicePdo);
    UsbDevice = DeviceHandleToUsbDevice(BusContext, DeviceHandle);
    if (!UsbDevice)
    {
        DPRINT1("Invalid DeviceHandle or device not connected\n");
        return;
    }    
    
    UsbDevice->UsbDevicePdo = UsbDevicePdo;
}


/* USB_BUS_INTERFACE_USBDI_V2 Functions */

VOID
USB_BUSIFFN
GetUSBDIVersion(PVOID BusContext, PUSBD_VERSION_INFORMATION VersionInformation, PULONG HcdCapabilites)
{
    DPRINT1("Ehci: GetUSBDIVersion called\n");
    return;
}

NTSTATUS
USB_BUSIFFN
QueryBusTime(PVOID BusContext, PULONG CurrentFrame)
{
    DPRINT1("Ehci: QueryBusTime called\n");
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
USB_BUSIFFN
SubmitIsoOutUrb(PVOID BusContext, PURB Urb)
{
    DPRINT1("Ehci: SubmitIsoOutUrb called\n");
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
USB_BUSIFFN
QueryBusInformation(PVOID BusContext,
                    ULONG Level,
                    PVOID BusInformationBuffer,
                    PULONG BusInformationBufferLength,
                    PULONG BusInformationActualLength)
{
    DPRINT1("Ehci: QueryBusInformation called\n");
    return STATUS_NOT_SUPPORTED;
}

BOOLEAN
USB_BUSIFFN
IsDeviceHighSpeed(PVOID BusContext)
{
    DPRINT1("Ehci: IsDeviceHighSpeed called\n");
    return TRUE;
}

NTSTATUS
USB_BUSIFFN
EnumLogEntry(PVOID BusContext, ULONG DriverTag, ULONG EnumTag, ULONG P1, ULONG P2)
{
    DPRINT1("Ehci: EnumLogEntry called %x, %x, %x, %x\n", DriverTag, EnumTag, P1, P2);
    
    return STATUS_SUCCESS;
}
