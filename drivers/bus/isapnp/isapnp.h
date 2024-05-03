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
#include "isapnpres.h"

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
    ULONG Cards;
    LIST_ENTRY BusLink;
} ISAPNP_FDO_EXTENSION, *PISAPNP_FDO_EXTENSION;

typedef struct _ISAPNP_PDO_EXTENSION
{
    ISAPNP_COMMON_EXTENSION Common;
    PISAPNP_FDO_EXTENSION FdoExt;

    ULONG Flags;
#define ISAPNP_ENUMERATED               0x00000001 /**< @brief Whether the device has been reported to the PnP manager. */
#define ISAPNP_SCANNED_BY_READ_PORT     0x00000002 /**< @brief The bus has been scanned by Read Port PDO. */
#define ISAPNP_READ_PORT_ALLOW_FDO_SCAN 0x00000004 /**< @brief Allows the active FDO to scan the bus. */
#define ISAPNP_READ_PORT_NEED_REBALANCE 0x00000008 /**< @brief The I/O resource requirements have changed. */

    union
    {
        /* Data belonging to logical devices */
        struct
        {
            PISAPNP_LOGICAL_DEVICE IsaPnpDevice;

            PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;

            PCM_RESOURCE_LIST ResourceList;
            ULONG ResourceListSize;
        };

        ULONG SelectedPort;
    };

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
BOOLEAN
FindIoDescriptor(
    _In_ PISAPNP_LOGICAL_DEVICE LogDevice,
    _In_opt_ ULONG Base,
    _In_ ULONG RangeStart,
    _In_ ULONG RangeEnd,
    _Out_opt_ PUCHAR Information,
    _Out_opt_ PULONG Length,
    _Out_opt_ PUCHAR WriteOrder);

CODE_SEG("PAGE")
BOOLEAN
FindIrqDescriptor(
    _In_ PISAPNP_LOGICAL_DEVICE LogDevice,
    _In_ ULONG Vector,
    _Out_opt_ PUCHAR WriteOrder);

CODE_SEG("PAGE")
BOOLEAN
FindDmaDescriptor(
    _In_ PISAPNP_LOGICAL_DEVICE LogDevice,
    _In_ ULONG Channel,
    _Out_opt_ PUCHAR WriteOrder);

CODE_SEG("PAGE")
BOOLEAN
FindMemoryDescriptor(
    _In_ PISAPNP_LOGICAL_DEVICE LogDevice,
    _In_ ULONG RangeStart,
    _In_ ULONG RangeEnd,
    _Out_opt_ PBOOLEAN Memory32,
    _Out_opt_ PUCHAR Information,
    _Out_opt_ PUCHAR WriteOrder);

CODE_SEG("PAGE")
PIO_RESOURCE_REQUIREMENTS_LIST
IsaPnpCreateReadPortDORequirements(
    _In_opt_ ULONG SelectedReadPort);

CODE_SEG("PAGE")
PCM_RESOURCE_LIST
IsaPnpCreateReadPortDOResources(VOID);

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
UCHAR
IsaHwTryReadDataPort(
    _In_ PUCHAR ReadDataPort);

_Requires_lock_held_(FdoExt->DeviceSyncEvent)
CODE_SEG("PAGE")
NTSTATUS
IsaHwFillDeviceList(
    _In_ PISAPNP_FDO_EXTENSION FdoExt);

CODE_SEG("PAGE")
NTSTATUS
IsaHwConfigureDevice(
    _In_ PISAPNP_FDO_EXTENSION FdoExt,
    _In_ PISAPNP_LOGICAL_DEVICE LogicalDevice,
    _In_ PCM_RESOURCE_LIST Resources);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
IsaHwWakeDevice(
    _In_ PISAPNP_LOGICAL_DEVICE LogicalDevice);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
IsaHwDeactivateDevice(
    _In_ PISAPNP_LOGICAL_DEVICE LogicalDevice);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
IsaHwActivateDevice(
    _In_ PISAPNP_FDO_EXTENSION FdoExt,
    _In_ PISAPNP_LOGICAL_DEVICE LogicalDevice);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
IsaHwWaitForKey(VOID);

#ifdef __cplusplus
}
#endif

#endif /* _ISAPNP_PCH_ */
