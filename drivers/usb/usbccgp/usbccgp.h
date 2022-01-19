#ifndef USBEHCI_H__
#define USBEHCI_H__

#include <wdm.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbdlib.h>

#include <stdio.h>

typedef struct
{
    BOOLEAN IsFDO;                                           // is device a FDO or PDO
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
    PDEVICE_OBJECT * ChildPDO;                               // child pdos
    LIST_ENTRY ResetPortListHead;                            // reset port list head
    LIST_ENTRY CyclePortListHead;                            // cycle port list head
    UCHAR ResetPortActive;                                   // reset port active
    UCHAR CyclePortActive;                                   // cycle port active
    KSPIN_LOCK Lock;                                         // reset / cycle port list lock
}FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

#define USBCCPG_TAG 'cbsu'

typedef struct
{
    COMMON_DEVICE_EXTENSION Common;                          // shared with FDO
    PUSBC_FUNCTION_DESCRIPTOR FunctionDescriptor;            // function descriptor
    PDEVICE_OBJECT NextDeviceObject;                         // next device object
    DEVICE_CAPABILITIES Capabilities;                        // device capabilities
    ULONG FunctionIndex;                                     // function index
    USB_DEVICE_DESCRIPTOR DeviceDescriptor;                  // usb device descriptor
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;   // usb configuration descriptor
    USBD_CONFIGURATION_HANDLE ConfigurationHandle;           // configuration handle
    PUSBD_INTERFACE_LIST_ENTRY InterfaceList;                // interface list
    ULONG InterfaceListCount;                                // interface list count
    PFDO_DEVICE_EXTENSION FDODeviceExtension;                // pointer to fdo's pdo list
}PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

/* descriptor.c */

VOID
DumpConfigurationDescriptor(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor);

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

NTSTATUS
NTAPI
USBCCGP_GetStringDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DescriptorLength,
    IN UCHAR DescriptorIndex,
    IN LANGID LanguageId,
    OUT PVOID *OutDescriptor);

ULONG
CountInterfaceDescriptors(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor);

NTSTATUS
AllocateInterfaceDescriptorsArray(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    OUT PUSB_INTERFACE_DESCRIPTOR **OutArray);

/* misc.c */

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

VOID
DumpFunctionDescriptor(
    IN PUSBC_FUNCTION_DESCRIPTOR FunctionDescriptor,
    IN ULONG FunctionDescriptorCount);

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

#endif /* USBEHCI_H__ */
