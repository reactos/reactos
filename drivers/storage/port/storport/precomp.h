/*
 * PROJECT:     ReactOS Storport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Storport driver common header file
 * COPYRIGHT:   Copyright 2017 Eric Kohl (eric.kohl@reactos.org)
 */

#ifndef _STORPORT_PCH_
#define _STORPORT_PCH_

#include <wdm.h>
#include <ntddk.h>
#include <stdio.h>
#include <memory.h>

/* Declare STORPORT_API functions as exports rather than imports */
#define _STORPORT_
#include <storport.h>

#include <ntddscsi.h>
#include <ntdddisk.h>
#include <mountdev.h>
#include <wdmguid.h>

/* Memory Tags */
#define TAG_GLOBAL_DATA    'DGtS'
#define TAG_INIT_DATA      'DItS'
#define TAG_MINIPORT_DATA  'DMtS'

typedef enum
{
    dsStopped,
    dsStarted,
    dsPaused,
    dsRemoved,
    dsSurpriseRemoved
} DEVICE_STATE;

typedef enum
{
    InvalidExtension = 0,
    DriverExtension,
    FdoExtension,
    PdoExtension
} EXTENSION_TYPE;

typedef struct _DRIVER_INIT_DATA
{
    LIST_ENTRY Entry;
    HW_INITIALIZATION_DATA HwInitData;
} DRIVER_INIT_DATA, *PDRIVER_INIT_DATA;

typedef struct _DRIVER_OBJECT_EXTENSION
{
    EXTENSION_TYPE ExtensionType;
    PDRIVER_OBJECT DriverObject;

    KSPIN_LOCK AdapterListLock;
    LIST_ENTRY AdapterListHead;
    ULONG AdapterCount;

    LIST_ENTRY InitDataListHead;
} DRIVER_OBJECT_EXTENSION, *PDRIVER_OBJECT_EXTENSION;

typedef struct _MINIPORT_DEVICE_EXTENSION
{
    struct _MINIPORT *Miniport;
    UCHAR HwDeviceExtension[0];
} MINIPORT_DEVICE_EXTENSION, *PMINIPORT_DEVICE_EXTENSION;

typedef struct _MINIPORT
{
    struct _FDO_DEVICE_EXTENSION *DeviceExtension;
    PHW_INITIALIZATION_DATA InitData;
    PMINIPORT_DEVICE_EXTENSION MiniportExtension;
} MINIPORT, *PMINIPORT;

typedef struct _FDO_DEVICE_EXTENSION
{
    EXTENSION_TYPE ExtensionType;

    PDEVICE_OBJECT Device;
    PDEVICE_OBJECT LowerDevice;
    PDEVICE_OBJECT PhysicalDevice;

    PDRIVER_OBJECT_EXTENSION DriverExtension;

    DEVICE_STATE PnpState;
    LIST_ENTRY AdapterListEntry;

    MINIPORT Miniport;

} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;


typedef struct _PDO_DEVICE_EXTENSION
{
    EXTENSION_TYPE ExtensionType;

    PDEVICE_OBJECT AttachedFdo;

    DEVICE_STATE PnpState;

} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;


/* fdo.c */

NTSTATUS
NTAPI
PortFdoPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp);


/* miniport.c */

NTSTATUS
MiniportInitialize(
    _In_ PMINIPORT Miniport,
    _In_ PFDO_DEVICE_EXTENSION DeviceExtension,
    _In_ PHW_INITIALIZATION_DATA HwInitializationData);

NTSTATUS
MiniportFindAdapter(
    _In_ PMINIPORT Miniport);

NTSTATUS
MiniportHwInitialize(
    _In_ PMINIPORT Miniport);


/* misc.c */

NTSTATUS
ForwardIrpAndWait(
    _In_ PDEVICE_OBJECT LowerDevice,
    _In_ PIRP Irp);

NTSTATUS
NTAPI
ForwardIrpAndForget(
    _In_ PDEVICE_OBJECT LowerDevice,
    _In_ PIRP Irp);

INTERFACE_TYPE
GetBusInterface(
    PDEVICE_OBJECT DeviceObject);

/* pdo.c */

NTSTATUS
NTAPI
PortPdoPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp);


/* storport.c */

PHW_INITIALIZATION_DATA
PortGetDriverInitData(
    PDRIVER_OBJECT_EXTENSION DriverExtension,
    INTERFACE_TYPE InterfaceType);

NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath);

#endif /* _STORPORT_PCH_ */
