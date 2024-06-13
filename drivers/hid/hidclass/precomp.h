#ifndef _HIDCLASS_PCH_
#define _HIDCLASS_PCH_

#define _HIDPI_NO_FUNCTION_MACROS_

#include <wdm.h>
#include <hidpddi.h>
#include <stdio.h>
#include <hidport.h>
#include "cyclicbuffer.h"

#define HIDCLASS_TAG 'CdiH'

typedef struct
{
    PDRIVER_OBJECT DriverObject;
    ULONG DeviceExtensionSize;
    BOOLEAN DevicesArePolled;
    PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
    PDRIVER_ADD_DEVICE AddDevice;
    PDRIVER_UNLOAD DriverUnload;
    KSPIN_LOCK Lock;

} HIDCLASS_DRIVER_EXTENSION, *PHIDCLASS_DRIVER_EXTENSION;

typedef struct
{
    //
    // hid device extension
    //
    HID_DEVICE_EXTENSION HidDeviceExtension;

    //
    // if it is a pdo
    //
    BOOLEAN IsFDO;

    //
    // driver extension
    //
    PHIDCLASS_DRIVER_EXTENSION DriverExtension;

    //
    // device description
    //
    HIDP_DEVICE_DESC DeviceDescription;

    //
    // hid attributes
    //
    HID_DEVICE_ATTRIBUTES Attributes;
} HIDCLASS_COMMON_DEVICE_EXTENSION, *PHIDCLASS_COMMON_DEVICE_EXTENSION;

typedef struct
{
    //
    // parts shared by fdo and pdo
    //
    HIDCLASS_COMMON_DEVICE_EXTENSION Common;

    //
    // device capabilities
    //
    DEVICE_CAPABILITIES Capabilities;

    //
    // hid descriptor
    //
    HID_DESCRIPTOR HidDescriptor;

    //
    // report descriptor
    //
    PUCHAR ReportDescriptor;

    //
    // device relations
    //
    PDEVICE_RELATIONS DeviceRelations;

    BOOLEAN IsReadLoopStarted;
    PUCHAR InputBuffer;
    USHORT InputBufferSize;
    PUCHAR InputWriteAddress;
    PIRP InputIRP;
} HIDCLASS_FDO_EXTENSION, *PHIDCLASS_FDO_EXTENSION;

typedef struct
{
    //
    // parts shared by fdo and pdo
    //
    HIDCLASS_COMMON_DEVICE_EXTENSION Common;

    //
    // device capabilities
    //
    DEVICE_CAPABILITIES Capabilities;

    //
    // collection index
    //
    ULONG CollectionNumber;

    //
    // device interface
    //
    UNICODE_STRING DeviceInterface;

    //
    // FDO device object
    //
    PDEVICE_OBJECT FDODeviceObject;

    //
    // fdo device extension
    //
    PHIDCLASS_FDO_EXTENSION FDODeviceExtension;



    HIDCHASS_CYCLIC_BUFFER InputBuffer;

    LIST_ENTRY PendingIRPList;

    KSPIN_LOCK ReadLock;
} HIDCLASS_PDO_DEVICE_EXTENSION, *PHIDCLASS_PDO_DEVICE_EXTENSION;

DRIVER_CANCEL HidClassPDO_IrpCancelRoutine;

/* fdo.c */
NTSTATUS
HidClassFDO_PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
HidClassFDO_DispatchRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
HidClassFDO_DispatchRequestSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
NTAPI
HidClassFDO_ReadCompletion(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context);

NTSTATUS
HidClassFDO_SubmitRead(IN PHIDCLASS_FDO_EXTENSION DeviceExtension);

NTSTATUS
HidClassFDO_InitiateRead(IN PHIDCLASS_FDO_EXTENSION DeviceExtension);

/* pdo.c */
NTSTATUS
HidClassPDO_CreatePDO(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDEVICE_RELATIONS *OutDeviceRelations);

NTSTATUS
HidClassPDO_PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

PHIDP_COLLECTION_DESC
HidClassPDO_GetCollectionDescription(
    PHIDP_DEVICE_DESC DeviceDescription,
    ULONG CollectionNumber);

PHIDP_REPORT_IDS
HidClassPDO_GetReportDescription(
    PHIDP_DEVICE_DESC DeviceDescription,
    ULONG CollectionNumber);

PHIDP_REPORT_IDS
HidClassPDO_GetReportDescriptionByReportID(
    PHIDP_DEVICE_DESC DeviceDescription,
    UCHAR ReportID);

#endif /* _HIDCLASS_PCH_ */
