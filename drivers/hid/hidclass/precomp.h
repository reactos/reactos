#pragma once

#define _HIDPI_NO_FUNCTION_MACROS_
#include <ntddk.h>
#include <hidport.h>
#include <hidpddi.h>
#include <debug.h>


typedef struct
{
    PDRIVER_OBJECT DriverObject;
    ULONG DeviceExtensionSize;
    BOOLEAN DevicesArePolled;
    PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
    PDRIVER_ADD_DEVICE AddDevice;
    PDRIVER_UNLOAD DriverUnload;
    KSPIN_LOCK Lock;

}HIDCLASS_DRIVER_EXTENSION, *PHIDCLASS_DRIVER_EXTENSION;

typedef struct
{
    HID_DEVICE_EXTENSION HidDeviceExtension;
    BOOLEAN IsFDO;
}HIDCLASS_COMMON_DEVICE_EXTENSION, *PHIDCLASS_COMMON_DEVICE_EXTENSION;



typedef struct
{
    //
    // parts shared by fdo and pdo
    //
    HIDCLASS_COMMON_DEVICE_EXTENSION Common;

    //
    // driver extension
    //
    PHIDCLASS_DRIVER_EXTENSION DriverExtension;

    //
    // device capabilities
    //
    DEVICE_CAPABILITIES Capabilities;

    //
    // hid descriptor
    //
    HID_DESCRIPTOR HidDescriptor;

    //
    // hid attributes
    //
    HID_DEVICE_ATTRIBUTES Attributes;

    //
    // report descriptor
    //
    PUCHAR ReportDescriptor;

    //
    // device description
    //
    HIDP_DEVICE_DESC DeviceDescription;

}HIDCLASS_FDO_EXTENSION, *PHIDCLASS_FDO_EXTENSION;


/* fdo.c */
NTSTATUS
HidClassFDO_PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

