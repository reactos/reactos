#pragma once

#define USB_BUSIFFN __stdcall
#include <ntifs.h>
#include <ntddk.h>
#include <usb.h>

/* usbbusif.h and hubbusif.h need to be imported */
typedef PVOID PUSB_DEVICE_HANDLE;

typedef
VOID
USB_BUSIFFN
RH_INIT_CALLBACK (PVOID CallBackContext);

typedef RH_INIT_CALLBACK *PRH_INIT_CALLBACK;

typedef struct _USB_EXTPORT_INFORMATION_0
{
    ULONG PhysicalPortNumber;
    ULONG PortLabelNumber;
    USHORT VidOverride;
    USHORT PidOverride;
    ULONG PortAttributes;
} USB_EXTPORT_INFORMATION_0, *PUSB_EXTPORT_INFORMATION;

typedef struct _USB_EXTHUB_INFORMATION_0
{
    ULONG InformationLevel;
    ULONG NumberOfPorts;
    USB_EXTPORT_INFORMATION_0 Port[255];
} USB_EXTHUB_INFORMATION_0, *PUSB_EXTHUB_INFORMATION_0;

typedef struct _USB_BUS_INTERFACE_USBDI_V2
{
    USHORT Size;
    USHORT Version;
    PVOID BusContext;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    PVOID GetUSBDIVersion;
    PVOID QueryBusTime;
    PVOID SubmitIsoOutUrb;
    PVOID QueryBusInformation;
    PVOID IsDeviceHighSpeed;
    PVOID EnumLogEntry;
} USB_BUS_INTERFACE_USBDI_V2, *PUSB_BUS_INTERFACE_USBDI_V2;

typedef struct _USB_BUS_INTERFACE_HUB_V5
{
    USHORT Size;
    USHORT Version;
    PVOID BusContext;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    PVOID CreateUsbDevice;
    PVOID InitializeUsbDevice;
    PVOID GetUsbDescriptors;
    PVOID RemoveUsbDevice;
    PVOID RestoreUsbDevice;
    PVOID GetPortHackFlags;
    PVOID QueryDeviceInformation;
    PVOID GetControllerInformation;
    PVOID ControllerSelectiveSuspend;
    PVOID GetExtendedHubInformation;
    PVOID GetRootHubSymbolicName;
    PVOID GetDeviceBusContext;
    PVOID Initialize20Hub;
    PVOID RootHubInitNotification;
    PVOID FlushTransfers;
    PVOID SetDeviceHandleData;
} USB_BUS_INTERFACE_HUB_V5, *PUSB_BUS_INTERFACE_HUB_V5;

VOID
USB_BUSIFFN
InterfaceReference(PVOID BusContext);

VOID
USB_BUSIFFN
InterfaceDereference(PVOID BusContext);

NTSTATUS
USB_BUSIFFN
CreateUsbDevice(PVOID BusContext,
    PUSB_DEVICE_HANDLE *NewDevice,
    PUSB_DEVICE_HANDLE HubDeviceHandle,
    USHORT PortStatus, USHORT PortNumber);

NTSTATUS
USB_BUSIFFN
InitializeUsbDevice(PVOID BusContext, PUSB_DEVICE_HANDLE DeviceHandle);

NTSTATUS
USB_BUSIFFN
GetUsbDescriptors(PVOID BusContext,
    PUSB_DEVICE_HANDLE DeviceHandle,
    PUCHAR DeviceDescriptorBuffer,
    PULONG DeviceDescriptorBufferLength,
    PUCHAR ConfigurationBuffer,
    PULONG ConfigDescriptorBufferLength);

NTSTATUS
USB_BUSIFFN
RemoveUsbDevice(PVOID BusContext, PUSB_DEVICE_HANDLE DeviceHandle, ULONG Flags);

NTSTATUS
USB_BUSIFFN
RestoreUsbDevice(PVOID BusContext, PUSB_DEVICE_HANDLE OldDeviceHandle, PUSB_DEVICE_HANDLE NewDeviceHandle);

NTSTATUS
USB_BUSIFFN
GetPortHackFlags(PVOID BusContext, PULONG Flags);

NTSTATUS
USB_BUSIFFN
QueryDeviceInformation(PVOID BusContext,
                       PUSB_DEVICE_HANDLE DeviceHandle,
                       PVOID DeviceInformationBuffer,
                       ULONG DeviceInformationBufferLength,
                       PULONG LengthReturned);

NTSTATUS
USB_BUSIFFN
GetControllerInformation(PVOID BusContext,
                         PVOID ControllerInformationBuffer,
                         ULONG ControllerInformationBufferLength,
                         PULONG LengthReturned);

NTSTATUS
USB_BUSIFFN
ControllerSelectiveSuspend(PVOID BusContext, BOOLEAN Enable);

NTSTATUS
USB_BUSIFFN
GetExtendedHubInformation(PVOID BusContext,
                          PDEVICE_OBJECT HubPhysicalDeviceObject,
                          PVOID HubInformationBuffer,
                          ULONG HubInformationBufferLength,
                          PULONG LengthReturned);

NTSTATUS
USB_BUSIFFN
GetRootHubSymbolicName(PVOID BusContext,
                       PVOID HubSymNameBuffer,
                       ULONG HubSymNameBufferLength,
                       PULONG HubSymNameActualLength);

PVOID
USB_BUSIFFN
GetDeviceBusContext(PVOID HubBusContext, PVOID DeviceHandle);

NTSTATUS
USB_BUSIFFN
Initialize20Hub(PVOID BusContext, PUSB_DEVICE_HANDLE HubDeviceHandle, ULONG TtCount);

NTSTATUS
USB_BUSIFFN
RootHubInitNotification(PVOID BusContext, PVOID CallbackContext, PRH_INIT_CALLBACK CallbackRoutine);

VOID
USB_BUSIFFN
FlushTransfers(PVOID BusContext, PVOID DeviceHandle);

VOID
USB_BUSIFFN
SetDeviceHandleData(PVOID BusContext, PVOID DeviceHandle, PDEVICE_OBJECT UsbDevicePdo);

NTSTATUS
USB_BUSIFFN
GetUSBDIVersion(PVOID BusContext, PUSBD_VERSION_INFORMATION VersionInformation, PULONG HcdCapabilites);

NTSTATUS
USB_BUSIFFN
QueryBusTime(PVOID BusContext, PULONG CurrentFrame);

NTSTATUS
USB_BUSIFFN
SubmitIsoOutUrb(PVOID BusContext, PURB Urb);

NTSTATUS
USB_BUSIFFN
QueryBusInformation(PVOID BusContext,
                    ULONG Level,
                    PVOID BusInformationBuffer,
                    PULONG BusInformationBufferLength,
                    PULONG BusInformationActualLength);

BOOLEAN
USB_BUSIFFN
IsDeviceHighSpeed(PVOID BusCOntext);

NTSTATUS
USB_BUSIFFN
EnumLogEntry(PVOID BusContext, ULONG DriverTag, ULONG EnumTag, ULONG P1, ULONG P2);
