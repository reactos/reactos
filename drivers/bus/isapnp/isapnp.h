/*
 * PROJECT:     ReactOS ISA PnP Bus driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Common header file
 * COPYRIGHT:   Copyright 2010 Cameron Gutman <cameron.gutman@reactos.org>
 *              Copyright 2020 Hervé Poussineau <hpoussin@reactos.org>
 */

#ifndef _ISAPNP_PCH_
#define _ISAPNP_PCH_

#include <wdm.h>
#include <ntstrsafe.h>
#include <section_attribs.h>
#include "isapnphw.h"

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
    LIST_ENTRY DeviceLink;
} ISAPNP_LOGICAL_DEVICE, *PISAPNP_LOGICAL_DEVICE;

typedef struct _ISAPNP_COMMON_EXTENSION
{
    PDEVICE_OBJECT Self;
    BOOLEAN IsFdo;
    ISAPNP_DEVICE_STATE State;
} ISAPNP_COMMON_EXTENSION, *PISAPNP_COMMON_EXTENSION;

typedef struct _ISAPNP_FDO_EXTENSION
{
    ISAPNP_COMMON_EXTENSION Common;
    PDEVICE_OBJECT Ldo;
    PDEVICE_OBJECT Pdo;
    PDEVICE_OBJECT ReadPortPdo;
    KEVENT DeviceSyncEvent;
    LIST_ENTRY DeviceListHead;
    ULONG DeviceCount;
    PDRIVER_OBJECT DriverObject;
    PUCHAR ReadDataPort;
} ISAPNP_FDO_EXTENSION, *PISAPNP_FDO_EXTENSION;

typedef struct _ISAPNP_PDO_EXTENSION
{
    ISAPNP_COMMON_EXTENSION Common;
    PISAPNP_LOGICAL_DEVICE IsaPnpDevice;
    PISAPNP_FDO_EXTENSION FdoExt;
    UNICODE_STRING DeviceID;
    UNICODE_STRING HardwareIDs;
    UNICODE_STRING CompatibleIDs;
    UNICODE_STRING InstanceID;
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;
    PCM_RESOURCE_LIST ResourceList;
    ULONG ResourceListSize;
} ISAPNP_PDO_EXTENSION, *PISAPNP_PDO_EXTENSION;

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

#define RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE         1
#define RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING   2

NTSTATUS
NTAPI
IsaPnpDuplicateUnicodeString(
    IN ULONG Flags,
    IN PCUNICODE_STRING SourceString,
    OUT PUNICODE_STRING DestinationString);

CODE_SEG("PAGE")
NTSTATUS
IsaPnpFillDeviceRelations(
    _In_ PISAPNP_FDO_EXTENSION FdoExt,
    _Inout_ PIRP Irp,
    _In_ BOOLEAN IncludeDataPort);

CODE_SEG("INIT")
DRIVER_INITIALIZE DriverEntry;

NTSTATUS
NTAPI
IsaForwardIrpSynchronous(
    _In_ PISAPNP_FDO_EXTENSION FdoExt,
    _Inout_ PIRP Irp);

/* fdo.c */
CODE_SEG("PAGE")
NTSTATUS
IsaFdoPnp(
    _In_ PISAPNP_FDO_EXTENSION FdoExt,
    _Inout_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpSp);

/* pdo.c */
CODE_SEG("PAGE")
NTSTATUS
IsaPdoPnp(
    _In_ PISAPNP_PDO_EXTENSION PdoDeviceExtension,
    _Inout_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpSp);

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
