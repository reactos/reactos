#ifndef USBOHCI_H__
#define USBOHCI_H__

#include <ntddk.h>
#define YDEBUG
#include <debug.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbioctl.h>

extern
"C"
{
#include <usbdlib.h>
}

//
// FIXME: 
// #include <usbprotocoldefs.h>
//
#include <usb.h>
#include <stdio.h>
#include <wdmguid.h>

//
// FIXME:
// the following includes are required to get kcom to compile
//
#include <portcls.h>
#include <dmusicks.h>
#include <kcom.h>

#include "interfaces.h"

//
// flags for handling USB_REQUEST_SET_FEATURE / USB_REQUEST_GET_FEATURE
//
#define PORT_ENABLE         1
#define PORT_SUSPEND        2
#define PORT_OVER_CURRENT   3
#define PORT_RESET          4
#define PORT_POWER          8
#define C_PORT_CONNECTION   16
#define C_PORT_ENABLE       17
#define C_PORT_SUSPEND      18
#define C_PORT_OVER_CURRENT 19
#define C_PORT_RESET        20

typedef struct
{
    BOOLEAN IsFDO;                                               // is device a FDO or PDO
    BOOLEAN IsHub;                                               // is device a hub / child - not yet used
    PDISPATCHIRP Dispatcher;                                     // dispatches the code
}COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

//
// tag for allocations
//
#define TAG_USBOHCI 'ICHO'

//
// assert for c++ - taken from portcls
//
#define PC_ASSERT(exp) \
  (VOID)((!(exp)) ? \
    RtlAssert((PVOID) #exp, (PVOID)__FILE__, __LINE__, NULL ), FALSE : TRUE)

//
// hcd_controller.cpp
//
NTSTATUS CreateHCDController(PHCDCONTROLLER *HcdController);

//
// hardware.cpp
//
NTSTATUS CreateUSBHardware(PUSBHARDWAREDEVICE *OutHardware);

//
// misc.cpp
//
NTSTATUS NTAPI SyncForwardIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI GetBusInterface(PDEVICE_OBJECT DeviceObject, PBUS_INTERFACE_STANDARD busInterface);

//
// root_hub_controller.cpp
//
NTSTATUS CreateHubController(PHUBCONTROLLER * OutHubController);

//
// memory_manager.cpp
//
NTSTATUS CreateDMAMemoryManager(PDMAMEMORYMANAGER *OutMemoryManager);


//
// usb_device.cpp
//
NTSTATUS CreateUSBDevice(PUSBDEVICE *OutDevice);

//
// usb_queue.cpp
//
NTSTATUS CreateUSBQueue(PUSBQUEUE *OutUsbQueue);

//
// usb_request.cpp
//
NTSTATUS InternalCreateUSBRequest(PUSBREQUEST *OutRequest);


typedef struct _USB_ENDPOINT
{
    USB_ENDPOINT_DESCRIPTOR EndPointDescriptor;
    UCHAR HubAddress;
    UCHAR HubPort;
    UCHAR DataToggle;
} USB_ENDPOINT, *PUSB_ENDPOINT;

typedef struct _USB_INTERFACE
{
    USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    USB_ENDPOINT *EndPoints;
} USB_INTERFACE, *PUSB_INTERFACE;

typedef struct _USB_CONFIGURATION
{
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;
    USB_INTERFACE *Interfaces;
} USB_CONFIGURATION, *PUSB_CONFIGURATION;


#endif
