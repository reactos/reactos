/*
 * PROJECT:     ReactOS ISA PnP Bus driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Common header file
 * COPYRIGHT:   Copyright 2010 Cameron Gutman <cameron.gutman@reactos.org>
 *              Copyright 2020 Hervé Poussineau <hpoussin@reactos.org>
 */

#ifndef _ISAPNP_PCH_
#define _ISAPNP_PCH_

#include <ntddk.h>
#include <ntstrsafe.h>
#include <section_attribs.h>
#include "isapnphw.h"

#include <initguid.h>
#include <wdmguid.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_ISAPNP 'pasI'

typedef enum
{
    dsStopped,
    dsStarted
} ISAPNP_DEVICE_STATE;

typedef struct _ISAPNP_IO
{
    USHORT CurrentBase;
    ISAPNP_IO_DESCRIPTION Description;
} ISAPNP_IO, *PISAPNP_IO;

typedef struct _ISAPNP_IRQ
{
    UCHAR CurrentNo;
    UCHAR CurrentType;
    ISAPNP_IRQ_DESCRIPTION Description;
} ISAPNP_IRQ, *PISAPNP_IRQ;

typedef struct _ISAPNP_DMA
{
    UCHAR CurrentChannel;
    ISAPNP_DMA_DESCRIPTION Description;
} ISAPNP_DMA, *PISAPNP_DMA;

typedef struct _ISAPNP_LOGICAL_DEVICE
{
    PDEVICE_OBJECT Pdo;
    ISAPNP_LOGDEVID LogDevId;
    UCHAR VendorId[3];
    USHORT ProdId;
    ULONG SerialNumber;
    ISAPNP_IO Io[8];
    ISAPNP_IRQ Irq[2];
    ISAPNP_DMA Dma[2];
    UCHAR CSN;
    UCHAR LDN;

    ULONG Flags;
#define ISAPNP_PRESENT              0x00000001 /**< @brief Cleared when the device is physically removed. */

    LIST_ENTRY DeviceLink;
} ISAPNP_LOGICAL_DEVICE, *PISAPNP_LOGICAL_DEVICE;

typedef enum _ISAPNP_SIGNATURE
{
    IsaPnpBus = 'odFI',
    IsaPnpLogicalDevice = 'veDI',
    IsaPnpReadDataPort = 'pdRI'
} ISAPNP_SIGNATURE;

typedef struct _ISAPNP_COMMON_EXTENSION
{
    ISAPNP_SIGNATURE Signature;
    PDEVICE_OBJECT Self;
    ISAPNP_DEVICE_STATE State;
} ISAPNP_COMMON_EXTENSION, *PISAPNP_COMMON_EXTENSION;

typedef struct _ISAPNP_FDO_EXTENSION
{
    ISAPNP_COMMON_EXTENSION Common;
    PDEVICE_OBJECT Ldo;
    PDEVICE_OBJECT Pdo;
    PDEVICE_OBJECT ReadPortPdo; /**< @remarks The pointer is NULL for all inactive FDOs. */
    ULONG BusNumber;
    KEVENT DeviceSyncEvent;

    _Guarded_by_(DeviceSyncEvent)
    LIST_ENTRY DeviceListHead;

    _Guarded_by_(DeviceSyncEvent)
    ULONG DeviceCount;

    PDRIVER_OBJECT DriverObject;
    PUCHAR ReadDataPort;
    LIST_ENTRY BusLink;
} ISAPNP_FDO_EXTENSION, *PISAPNP_FDO_EXTENSION;

typedef struct _ISAPNP_PDO_EXTENSION
{
    ISAPNP_COMMON_EXTENSION Common;
    PISAPNP_LOGICAL_DEVICE IsaPnpDevice;
    PISAPNP_FDO_EXTENSION FdoExt;
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;

    PCM_RESOURCE_LIST ResourceList;
    ULONG ResourceListSize;

    ULONG Flags;
#define ISAPNP_ENUMERATED               0x00000001 /**< @brief Whether the device has been reported to the PnP manager. */
#define ISAPNP_SCANNED_BY_READ_PORT     0x00000002 /**< @brief The bus has been scanned by Read Port PDO. */
#define ISAPNP_READ_PORT_ALLOW_FDO_SCAN 0x00000004 /**< @brief Allows the active FDO to scan the bus. */

    _Write_guarded_by_(_Global_interlock_)
    volatile LONG SpecialFiles;
} ISAPNP_PDO_EXTENSION, *PISAPNP_PDO_EXTENSION;

extern KEVENT BusSyncEvent;

_Guarded_by_(BusSyncEvent)
extern BOOLEAN ReadPortCreated;

_Guarded_by_(BusSyncEvent)
extern LIST_ENTRY BusListHead;

_Requires_lock_not_held_(BusSyncEvent)
_Acquires_lock_(BusSyncEvent)
FORCEINLINE
VOID
IsaPnpAcquireBusDataLock(VOID)
{
    KeWaitForSingleObject(&BusSyncEvent, Executive, KernelMode, FALSE, NULL);
}

_Releases_lock_(BusSyncEvent)
FORCEINLINE
VOID
IsaPnpReleaseBusDataLock(VOID)
{
    KeSetEvent(&BusSyncEvent, IO_NO_INCREMENT, FALSE);
}

_Requires_lock_not_held_(FdoExt->DeviceSyncEvent)
_Acquires_lock_(FdoExt->DeviceSyncEvent)
FORCEINLINE
VOID
IsaPnpAcquireDeviceDataLock(
    _In_ PISAPNP_FDO_EXTENSION FdoExt)
{
    KeWaitForSingleObject(&FdoExt->DeviceSyncEvent, Executive, KernelMode, FALSE, NULL);
}

_Releases_lock_(FdoExt->DeviceSyncEvent)
FORCEINLINE
VOID
IsaPnpReleaseDeviceDataLock(
    _In_ PISAPNP_FDO_EXTENSION FdoExt)
{
    KeSetEvent(&FdoExt->DeviceSyncEvent, IO_NO_INCREMENT, FALSE);
}

/* isapnp.c */

CODE_SEG("PAGE")
VOID
IsaPnpRemoveReadPortDO(
    _In_ PDEVICE_OBJECT Pdo);

CODE_SEG("PAGE")
NTSTATUS
IsaPnpFillDeviceRelations(
    _In_ PISAPNP_FDO_EXTENSION FdoExt,
    _Inout_ PIRP Irp,
    _In_ BOOLEAN IncludeDataPort);

CODE_SEG("INIT")
DRIVER_INITIALIZE DriverEntry;

/* fdo.c */
CODE_SEG("PAGE")
NTSTATUS
IsaFdoPnp(
    _In_ PISAPNP_FDO_EXTENSION FdoExt,
    _Inout_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpSp);

/* interface.c */
CODE_SEG("PAGE")
NTSTATUS
IsaFdoQueryInterface(
    _In_ PISAPNP_FDO_EXTENSION FdoExt,
    _In_ PIO_STACK_LOCATION IrpSp);

/* pdo.c */
CODE_SEG("PAGE")
NTSTATUS
IsaPdoPnp(
    _In_ PISAPNP_PDO_EXTENSION PdoDeviceExtension,
    _Inout_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpSp);

CODE_SEG("PAGE")
VOID
IsaPnpRemoveLogicalDeviceDO(
    _In_ PDEVICE_OBJECT Pdo);

/* hardware.c */
CODE_SEG("PAGE")
NTSTATUS
IsaHwTryReadDataPort(
    _In_ PUCHAR ReadDataPort);

CODE_SEG("PAGE")
NTSTATUS
IsaHwFillDeviceList(
    _In_ PISAPNP_FDO_EXTENSION FdoExt);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
IsaHwDeactivateDevice(
    _In_ PISAPNP_LOGICAL_DEVICE LogicalDevice);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
IsaHwActivateDevice(
    _In_ PISAPNP_LOGICAL_DEVICE LogicalDevice);

#ifdef __cplusplus
}
#endif

#endif /* _ISAPNP_PCH_ */
