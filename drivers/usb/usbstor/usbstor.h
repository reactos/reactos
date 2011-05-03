
#pragma once

#include <ntddk.h>
#define NDEBUG
#include <debug.h>
#include <usbdi.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbioctl.h>
#include <usbiodef.h>
#include <usb.h>
#include <usbdlib.h>
#include <stdio.h>
#include <wdmguid.h>


#define USB_STOR_TAG 'sbsu'
#define USB_MAXCHILDREN              (16)

NTSTATUS NTAPI
IoAttachDeviceToDeviceStackSafe(
  IN PDEVICE_OBJECT SourceDevice,
  IN PDEVICE_OBJECT TargetDevice,
  OUT PDEVICE_OBJECT *AttachedToDeviceObject);

typedef struct _COMMON_DEVICE_EXTENSION
{
    BOOLEAN IsFDO;

}COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct
{
    COMMON_DEVICE_EXTENSION Common;                                                      // common device extension

    PDEVICE_OBJECT FunctionalDeviceObject;                                               // functional device object
    PDEVICE_OBJECT PhysicalDeviceObject;                                                 // physical device object
    PDEVICE_OBJECT LowerDeviceObject;                                                    // lower device object
    USB_BUS_INTERFACE_USBDI_V2 BusInterface;                                             // bus interface of device
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;                                             // usb device descriptor
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;                               // usb configuration descriptor
    PUSBD_INTERFACE_INFORMATION InterfaceInformation;                                    // usb interface information
    USBD_CONFIGURATION_HANDLE ConfigurationHandle;                                       // usb configuration handle
    UCHAR BulkInPipeIndex;                                                               // bulk in pipe index
    UCHAR BulkOutPipeIndex;                                                              // bulk out pipe index
    UCHAR MaxLUN;                                                                        // max lun for device
    PDEVICE_OBJECT ChildPDO[16];                                                         // max 16 child pdo devices
}FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

typedef struct
{
    COMMON_DEVICE_EXTENSION Common;
    PDEVICE_OBJECT LowerDeviceObject;                                                    // points to FDO

}PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;


//
// max lun command identifier
//
#define USB_BULK_GET_MAX_LUN             0xFE

//---------------------------------------------------------------------
//
// fdo.c routines
//
NTSTATUS
USBSTOR_FdoHandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp);

//---------------------------------------------------------------------
//
// pdo.c routines
//
NTSTATUS
USBSTOR_PdoHandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp);

NTSTATUS
USBSTOR_CreatePDO(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDEVICE_OBJECT *ChildDeviceObject);

//---------------------------------------------------------------------
//
// misc.c routines
//
NTSTATUS
NTAPI
USBSTOR_SyncForwardIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp);

NTSTATUS
NTAPI
USBSTOR_GetBusInterface(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PUSB_BUS_INTERFACE_USBDI_V2 BusInterface);

PVOID
AllocateItem(
    IN POOL_TYPE PoolType, 
    IN ULONG ItemSize);

VOID
FreeItem(
    IN PVOID Item);

NTSTATUS
USBSTOR_SyncUrbRequest(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PURB UrbRequest);

NTSTATUS
USBSTOR_GetMaxLUN(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension);


//---------------------------------------------------------------------
//
// descriptor.c routines
//

NTSTATUS
USBSTOR_GetDescriptors(
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
USBSTOR_SelectConfigurationAndInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
USBSTOR_GetPipeHandles(
    IN PFDO_DEVICE_EXTENSION DeviceExtension);

