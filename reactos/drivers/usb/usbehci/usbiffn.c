/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/usbiffn.c
 * PURPOSE:     Direct Call Interface Functions.
 * PROGRAMMERS:
 *              Michael Martin
 */

/* usbbusif.h and hubbusif.h need to be imported */
#include "usbehci.h"
#include "usbiffn.h"
#define	NDEBUG
#include <debug.h>

VOID
USB_BUSIFFN
InterfaceReference(PVOID BusContext)
{
    DPRINT1("InterfaceReference called\n");
}

VOID
USB_BUSIFFN
InterfaceDereference(PVOID BusContext)
{
    DPRINT1("InterfaceDereference called\n");
}

/* Bus Interface Hub V5 Functions */

NTSTATUS
USB_BUSIFFN
CreateUsbDevice(PVOID BusContext,
                PUSB_DEVICE_HANDLE *NewDevice,
                PUSB_DEVICE_HANDLE HubDeviceHandle,
                USHORT PortStatus, USHORT PortNumber)
{
    DPRINT1("CreateUsbDevice called\n");
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
InitializeUsbDevice(PVOID BusContext, PUSB_DEVICE_HANDLE DeviceHandle)
{
    DPRINT1("InitializeUsbDevice called\n");
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
GetUsbDescriptors(PVOID BusContext,
                  PUSB_DEVICE_HANDLE DeviceHandle,
                  PUCHAR DeviceDescriptorBuffer,
                  PULONG DeviceDescriptorBufferLength,
                  PUCHAR ConfigurationBuffer,
                  PULONG ConfigDescriptorBufferLength)
{
    DPRINT1("GetUsbDescriptor called\n");
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
RemoveUsbDevice(PVOID BusContext, PUSB_DEVICE_HANDLE DeviceHandle, ULONG Flags)
{
    DPRINT1("RemoveUsbDevice called\n");
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
RestoreUsbDevice(PVOID BusContext, PUSB_DEVICE_HANDLE OldDeviceHandle, PUSB_DEVICE_HANDLE NewDeviceHandle)
{
    DPRINT1("RestoreUsbDevice called\n");
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
GetPortHackFlags(PVOID BusContext, PULONG Flags)
{
    DPRINT1("GetPortHackFlags called\n");
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
QueryDeviceInformation(PVOID BusContext,
                       PUSB_DEVICE_HANDLE DeviceHandle,
                       PVOID DeviceInformationBuffer,
                       ULONG DeviceInformationBufferLength,
                       PULONG LengthReturned)
{
    DPRINT1("QueryDeviceInformation called\n");
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
GetControllerInformation(PVOID BusContext,
                         PVOID ControllerInformationBuffer,
                         ULONG ControllerInformationBufferLength,
                         PULONG LengthReturned)
{
    DPRINT1("GetControllerInformation called\n");
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
ControllerSelectiveSuspend(PVOID BusContext, BOOLEAN Enable)
{
    DPRINT1("ControllerSelectiveSuspend called\n");
    return STATUS_SUCCESS;
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

    /* Set the default return value */
    *LengthReturned = 0;
    /* Caller must have set InformationLevel to 0 */
    if (UsbExtHubInfo->InformationLevel != 0)
    {
        return STATUS_NOT_SUPPORTED;
    }

    UsbExtHubInfo->NumberOfPorts = 8;

    for (i=0; i < UsbExtHubInfo->NumberOfPorts; i++)
    {
        UsbExtHubInfo->Port[i].PhysicalPortNumber = i + 1;
        UsbExtHubInfo->Port[i].PortLabelNumber = FdoDeviceExntension->ECHICaps.HCSParams.PortCount;
        UsbExtHubInfo->Port[i].VidOverride = 0;
        UsbExtHubInfo->Port[i].PidOverride = 0;
        UsbExtHubInfo->Port[i].PortAttributes = USB_PORTATTR_SHARED_USB2;
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
    DPRINT1("GetRootHubSymbolicName called\n");
    return STATUS_SUCCESS;
}

PVOID
USB_BUSIFFN
GetDeviceBusContext(PVOID HubBusContext, PVOID DeviceHandle)
{
    DPRINT1("GetDeviceBusContext called\n");
    return NULL;
}

NTSTATUS
USB_BUSIFFN
Initialize20Hub(PVOID BusContext, PUSB_DEVICE_HANDLE HubDeviceHandle, ULONG TtCount)
{
    DPRINT1("Initialize20Hub called\n");
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
RootHubInitNotification(PVOID BusContext, PVOID CallbackContext, PRH_INIT_CALLBACK CallbackRoutine)
{
    DPRINT1("RootHubInitNotification\n");
    return STATUS_SUCCESS;
}

VOID
USB_BUSIFFN
FlushTransfers(PVOID BusContext, PVOID DeviceHandle)
{
    DPRINT1("FlushTransfers\n");
}

VOID
USB_BUSIFFN
SetDeviceHandleData(PVOID BusContext, PVOID DeviceHandle, PDEVICE_OBJECT UsbDevicePdo)
{
    DPRINT1("SetDeviceHandleData called\n");
}


/* USB_BUS_INTERFACE_USBDI_V2 Functions */

NTSTATUS
USB_BUSIFFN
GetUSBDIVersion(PVOID BusContext, PUSBD_VERSION_INFORMATION VersionInformation, PULONG HcdCapabilites)
{
    DPRINT1("GetUSBDIVersion called\n");
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
QueryBusTime(PVOID BusContext, PULONG CurrentFrame)
{
    DPRINT1("QueryBusTime called\n");
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
SubmitIsoOutUrb(PVOID BusContext, PURB Urb)
{
    DPRINT1("SubmitIsoOutUrb called\n");
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
QueryBusInformation(PVOID BusContext,
                    ULONG Level,
                    PVOID BusInformationBuffer,
                    PULONG BusInformationBufferLength,
                    PULONG BusInformationActualLength)
{
    DPRINT1("QueryBusInformation called\n");
    return STATUS_SUCCESS;
}

BOOLEAN
USB_BUSIFFN
IsDeviceHighSpeed(PVOID BusCOntext)
{
    DPRINT1("IsDeviceHighSpeed called\n");
    return TRUE;
}

NTSTATUS
USB_BUSIFFN
EnumLogEntry(PVOID BusContext, ULONG DriverTag, ULONG EnumTag, ULONG P1, ULONG P2)
{
    DPRINT1("EnumLogEntry called\n");
    return STATUS_SUCCESS;
}

