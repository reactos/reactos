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
#define TAG_GLOBAL_DATA     'DGtS'
#define TAG_INIT_DATA       'DItS'
#define TAG_MINIPORT_DATA   'DMtS'
#define TAG_ACCRESS_RANGE   'RAtS'
#define TAG_RESOURCE_LIST   'LRtS'
#define TAG_ADDRESS_MAPPING 'MAtS'
#define TAG_INQUIRY_DATA    'QItS'
#define TAG_SENSE_DATA      'NStS'

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
    PORT_CONFIGURATION_INFORMATION PortConfig;
    PMINIPORT_DEVICE_EXTENSION MiniportExtension;
} MINIPORT, *PMINIPORT;

typedef struct _UNIT_DATA
{
    LIST_ENTRY ListEntry;
    INQUIRYDATA InquiryData;
} UNIT_DATA, *PUNIT_DATA;

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
    ULONG BusNumber;
    ULONG SlotNumber;
    PCM_RESOURCE_LIST AllocatedResources;
    PCM_RESOURCE_LIST TranslatedResources;
    BUS_INTERFACE_STANDARD BusInterface;
    BOOLEAN BusInitialized;
    PMAPPED_ADDRESS MappedAddressList;
    PVOID UncachedExtensionVirtualBase;
    PHYSICAL_ADDRESS UncachedExtensionPhysicalBase;
    ULONG UncachedExtensionSize;
    PHW_PASSIVE_INITIALIZE_ROUTINE HwPassiveInitRoutine;
    PKINTERRUPT Interrupt;
    ULONG InterruptIrql;

    KSPIN_LOCK PdoListLock;
    LIST_ENTRY PdoListHead;
    ULONG PdoCount;
} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;


typedef struct _PDO_DEVICE_EXTENSION
{
    EXTENSION_TYPE ExtensionType;

    PDEVICE_OBJECT Device;
    PFDO_DEVICE_EXTENSION FdoExtension;
    DEVICE_STATE PnpState;
    LIST_ENTRY PdoListEntry;

    ULONG Bus;
    ULONG Target;
    ULONG Lun;
    PINQUIRYDATA InquiryBuffer;


} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;


/* fdo.c */

NTSTATUS
NTAPI
PortFdoScsi(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp);

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

BOOLEAN
MiniportHwInterrupt(
    _In_ PMINIPORT Miniport);

BOOLEAN
MiniportStartIo(
    _In_ PMINIPORT Miniport,
    _In_ PSCSI_REQUEST_BLOCK Srb);

/* misc.c */

NTSTATUS
NTAPI
ForwardIrpAndForget(
    _In_ PDEVICE_OBJECT LowerDevice,
    _In_ PIRP Irp);

INTERFACE_TYPE
GetBusInterface(
    PDEVICE_OBJECT DeviceObject);

PCM_RESOURCE_LIST
CopyResourceList(
    POOL_TYPE PoolType,
    PCM_RESOURCE_LIST Source);

NTSTATUS
QueryBusInterface(
    PDEVICE_OBJECT DeviceObject,
    PGUID Guid,
    USHORT Size,
    USHORT Version,
    PBUS_INTERFACE_STANDARD Interface,
    PVOID InterfaceSpecificData);

BOOLEAN
TranslateResourceListAddress(
    PFDO_DEVICE_EXTENSION DeviceExtension,
    INTERFACE_TYPE BusType,
    ULONG SystemIoBusNumber,
    STOR_PHYSICAL_ADDRESS IoAddress,
    ULONG NumberOfBytes,
    BOOLEAN InIoSpace,
    PPHYSICAL_ADDRESS TranslatedAddress);

NTSTATUS
GetResourceListInterrupt(
    PFDO_DEVICE_EXTENSION DeviceExtension,
    PULONG Vector,
    PKIRQL Irql,
    KINTERRUPT_MODE *InterruptMode,
    PBOOLEAN ShareVector,
    PKAFFINITY Affinity);

NTSTATUS
AllocateAddressMapping(
    PMAPPED_ADDRESS *MappedAddressList,
    STOR_PHYSICAL_ADDRESS IoAddress,
    PVOID MappedAddress,
    ULONG NumberOfBytes,
    ULONG BusNumber);

/* pdo.c */

NTSTATUS
PortCreatePdo(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ ULONG Bus,
    _In_ ULONG Target,
    _In_ ULONG Lun,
    _Out_ PPDO_DEVICE_EXTENSION *PdoExtension);

NTSTATUS
PortDeletePdo(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension);

NTSTATUS
NTAPI
PortPdoScsi(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp);

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
