#ifndef LIBUSB_H__
#define LIBUSB_H__

#include <ntddk.h>
#include <hubbusif.h>
#include <usbbusif.h>

extern "C"
{
    #include <usbdlib.h>
}

//
// FIXME: 
// #include <usbprotocoldefs.h>
//
#include <stdio.h>
#include <wdmguid.h>

//
// FIXME:
// the following includes are required to get kcom to compile
//
#include <portcls.h>
#include <kcom.h>

PVOID
__cdecl
operator new(
    size_t iSize,
    POOL_TYPE poolType,
    ULONG tag);


#include "common_interfaces.h"

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


typedef struct _WORK_ITEM_DATA
{
    WORK_QUEUE_ITEM WorkItem;                                   // work item
    PVOID CallbackContext;                                      // callback context
    PRH_INIT_CALLBACK CallbackRoutine;                          // callback routine
} INIT_ROOT_HUB_CONTEXT, *PINIT_ROOT_HUB_CONTEXT;

//
// tag for allocations
//
#define TAG_USBLIB 'LBSU'

//
// assert for c++ - taken from portcls
//
#define PC_ASSERT(exp) \
  (VOID)((!(exp)) ? \
    RtlAssert((PVOID) #exp, (PVOID)__FILE__, __LINE__, NULL ), FALSE : TRUE)

// hcd_controller.cpp
extern "C"
{
NTSTATUS NTAPI CreateHCDController(PHCDCONTROLLER *HcdController);

// hardware.cpp
NTSTATUS NTAPI CreateUSBHardware(PUSBHARDWAREDEVICE *OutHardware);

// misc.cpp
NTSTATUS NTAPI SyncForwardIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI GetBusInterface(PDEVICE_OBJECT DeviceObject, PBUS_INTERFACE_STANDARD busInterface);

// root_hub_controller.cpp
NTSTATUS NTAPI CreateHubController(PHUBCONTROLLER * OutHubController);

// memory_manager.cpp
NTSTATUS NTAPI CreateDMAMemoryManager(PDMAMEMORYMANAGER *OutMemoryManager);

// usb_device.cpp
NTSTATUS NTAPI CreateUSBDevice(PUSBDEVICE *OutDevice);

// libusb.cpp
NTSTATUS NTAPI USBLIB_AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject);
NTSTATUS NTAPI USBLIB_Dispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp);

}

#endif /* LIBUSB_H__ */
