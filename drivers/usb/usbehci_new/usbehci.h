#ifndef USBEHCI_H__
#define USBEHCI_H__

#include <ntddk.h>
#define YDEBUG
#include <debug.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbioctl.h>
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

typedef struct
{
    BOOLEAN IsFDO;                                               // is device a FDO or PDO
    BOOLEAN IsHub;                                               // is device a hub / child - not yet used
    PDISPATCHIRP Dispatcher;                                     // dispatches the code
}COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

//
// tag for allocations
//
#define TAG_USBEHCI 'ICHE'

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

#endif
