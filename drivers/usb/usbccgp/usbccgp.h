#ifndef USBEHCI_H__
#define USBEHCI_H__

#include <ntddk.h>
#define YDEBUG
#include <debug.h>
#include <initguid.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbioctl.h>
#include <usbdlib.h>

//
// FIXME: 
// #include <usbprotocoldefs.h>
//
#include <usb.h>
#include <stdio.h>
#include <wdmguid.h>

typedef struct
{
    BOOLEAN IsFDO;                                               // is device a FDO or PDO
}COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct
{
    COMMON_DEVICE_EXTENSION Common;                          // shared with PDO
    PDRIVER_OBJECT DriverObject;                             // driver object
    PDEVICE_OBJECT PhysicalDeviceObject;                     // physical device object
    PDEVICE_OBJECT NextDeviceObject;                         // lower device object
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;                 // usb device descriptor
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;   // usb configuration descriptor
    DEVICE_CAPABILITIES Capabilities;                        // device capabilities
    PUSBD_INTERFACE_LIST_ENTRY InterfaceList;                // interface list
    ULONG InterfaceListCount;                                // interface list count
    USBD_CONFIGURATION_HANDLE ConfigurationHandle;           // configuration handle
    USBC_DEVICE_CONFIGURATION_INTERFACE_V1 BusInterface;     // bus custom enumeration interface
    PUSBC_FUNCTION_DESCRIPTOR FunctionDescriptor;            // usb function descriptor
    ULONG FunctionDescriptorCount;                           // number of function descriptor
}FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

#define USBCCPG_TAG 'cbsu'

typedef struct
{
    COMMON_DEVICE_EXTENSION Common;                          // shared with FDO

}PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

/* descriptor.c */

NTSTATUS
USBCCGP_GetDescriptors(
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
USBCCGP_SelectConfiguration(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
NTAPI
USBCCGP_GetDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR DescriptorType,
    IN ULONG DescriptorLength,
    IN UCHAR DescriptorIndex,
    IN LANGID LanguageId,
    OUT PVOID *OutDescriptor);

/* misc.c */

NTSTATUS
NTAPI
USBCCGP_SyncForwardIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

NTSTATUS
USBCCGP_SyncUrbRequest(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PURB UrbRequest);

PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN ULONG ItemSize);

VOID
FreeItem(
    IN PVOID Item);

/* fdo.c */

NTSTATUS
FDO_Dispatch(
    PDEVICE_OBJECT DeviceObject, 
    PIRP Irp);

/* pdo.c */

NTSTATUS
PDO_Dispatch(
    PDEVICE_OBJECT DeviceObject, 
    PIRP Irp);

/* function.c */

NTSTATUS
USBCCGP_QueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PUSBC_DEVICE_CONFIGURATION_INTERFACE_V1 BusInterface);

NTSTATUS
USBCCGP_EnumerateFunctions(
    IN PDEVICE_OBJECT DeviceObject);

#endif
