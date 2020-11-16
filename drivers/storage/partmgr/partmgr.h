/*
 * PROJECT:     Partition manager driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main header
 * COPYRIGHT:   2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#ifndef _PARTMGR_H_
#define _PARTMGR_H_

#include <ntifs.h>
#include <mountdev.h>
#include <ntddvol.h>
#include <ntdddisk.h>
#include <ndk/psfuncs.h>
#include <ndk/section_attribs.h>
#include <ioevent.h>
#include <stdio.h>
#include <debug/driverdbg.h>

#include "debug.h"

#define TAG_PARTMGR 'MtrP'

// from disk.sys
typedef struct _DISK_GEOMETRY_EX_INTERNAL
{
    DISK_GEOMETRY Geometry;
    INT64 DiskSize;
    DISK_PARTITION_INFO Partition;
    DISK_DETECTION_INFO Detection;
} DISK_GEOMETRY_EX_INTERNAL, *PDISK_GEOMETRY_EX_INTERNAL;

typedef struct _FDO_EXTENSION
{
    BOOLEAN IsFDO;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT LowerDevice;
    PDEVICE_OBJECT PhysicalDiskDO;
    KEVENT SyncEvent;

    BOOLEAN LayoutValid;
    PDRIVE_LAYOUT_INFORMATION_EX LayoutCache;

    SINGLE_LIST_ENTRY PartitionList;
    UINT32 EnumeratedPartitionsTotal;

    struct {
        UINT64 DiskSize;
        UINT32 DeviceNumber;
        UINT32 BytesPerSector;
        PARTITION_STYLE PartitionStyle;
        union {
            struct {
                UINT32 Signature;
            } Mbr;
            struct {
                GUID DiskId;
            } Gpt;
        };
    } DiskData;
} FDO_EXTENSION, *PFDO_EXTENSION;

typedef struct _PARTITION_EXTENSION
{
    BOOLEAN IsFDO;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT LowerDevice;
    PDEVICE_OBJECT Part0Device;

    UINT64 StartingOffset;
    UINT64 PartitionLength;
    SINGLE_LIST_ENTRY ListEntry;

    UINT32 DetectedNumber;
    UINT32 OnDiskNumber; // partition number for issuing Io requests to the kernel
    BOOLEAN IsEnumerated; // reported via IRP_MN_QUERY_DEVICE_RELATIONS
    BOOLEAN SymlinkCreated;
    BOOLEAN DeviceRemoved; // !!!
    BOOLEAN Attached; // attached to PartitionList of the FDO
    union
    {
        struct
        {
            GUID PartitionType;
            GUID PartitionId;
            UINT64 Attributes;
            WCHAR Name[36];
        } Gpt;
        struct
        {
            UINT8 PartitionType;
            BOOLEAN BootIndicator;
            BOOLEAN RecognizedPartition;
            UINT32 HiddenSectors;
        } Mbr;
    };
    UNICODE_STRING PartitionInterfaceName;
    UNICODE_STRING VolumeInterfaceName;
    UNICODE_STRING DeviceName;
} PARTITION_EXTENSION, *PPARTITION_EXTENSION;

NTSTATUS
PartitionCreateDevice(
    _In_ PDEVICE_OBJECT FDObject,
    _In_ PPARTITION_INFORMATION_EX PartitionEntry,
    _In_ UINT32 OnDiskNumber,
    _In_ PARTITION_STYLE PartitionStyle,
    _Out_ PDEVICE_OBJECT *PDO);

NTSTATUS
PartitionHandleRemove(
    _In_ PPARTITION_EXTENSION PartExt,
    _In_ BOOLEAN FinalRemove);

NTSTATUS
PartitionHandlePnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp);

NTSTATUS
PartitionHandleDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp);

NTSTATUS
NTAPI
ForwardIrpAndForget(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp);

NTSTATUS
IssueSyncIoControlRequest(
    _In_ UINT32 IoControlCode,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _In_ PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_ BOOLEAN InternalDeviceIoControl);

FORCEINLINE
BOOLEAN
VerifyIrpOutBufferSize(
    _In_ PIRP Irp,
    _In_ SIZE_T Size)
{
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
    {
        Irp->IoStatus.Information = Size;
        return FALSE;
    }
    return TRUE;
}

FORCEINLINE
BOOLEAN
VerifyIrpInBufferSize(
    _In_ PIRP Irp,
    _In_ SIZE_T Size)
{
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    if (ioStack->Parameters.DeviceIoControl.InputBufferLength < Size)
    {
        Irp->IoStatus.Information = Size;
        return FALSE;
    }
    return TRUE;
}

FORCEINLINE
VOID
PartMgrAcquireLayoutLock(
    _In_ PFDO_EXTENSION FDOExtension)
{
    PAGED_CODE();

    KeWaitForSingleObject(&FDOExtension->SyncEvent, Executive, KernelMode, FALSE, NULL);
}

FORCEINLINE
VOID
PartMgrReleaseLayoutLock(
    _In_ PFDO_EXTENSION FDOExtension)
{
    PAGED_CODE();

    KeSetEvent(&FDOExtension->SyncEvent, IO_NO_INCREMENT, FALSE);
}

#endif // _PARTMGR_H_
