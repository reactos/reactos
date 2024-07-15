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

// Unique ID data for basic (disk partition-based) volumes.
// It is stored in the MOUNTDEV_UNIQUE_ID::UniqueId member
// as an array of bytes.
#include <pshpack1.h>
typedef union _BASIC_VOLUME_UNIQUE_ID
{
    struct
    {
        ULONG Signature;
        ULONGLONG StartingOffset;
    } Mbr;
    struct
    {
        ULONGLONG Signature; // UCHAR[8] // "DMIO:ID:"
        GUID PartitionGuid;
    } Gpt;
} BASIC_VOLUME_UNIQUE_ID, *PBASIC_VOLUME_UNIQUE_ID;
#include <poppack.h>
C_ASSERT(RTL_FIELD_SIZE(BASIC_VOLUME_UNIQUE_ID, Mbr) == 0x0C);
C_ASSERT(RTL_FIELD_SIZE(BASIC_VOLUME_UNIQUE_ID, Gpt) == 0x18);

#define DMIO_ID_SIGNATURE   (*(ULONGLONG*)"DMIO:ID:")

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
    BOOLEAN IsSuperFloppy;

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
    UNICODE_STRING DiskInterfaceName;
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

    /* Volume number in the "\Device\HarddiskVolumeN" name */
    UINT32 VolumeNumber;

    /* Partition number in the "\Device\HarddiskX\PartitionN" symlink name;
     * it is assigned to a partition in order to identify it to the system
     * and corresponds to the PDO number given to PartitionCreateDevice() */
    UINT32 DetectedNumber;

    /* Partition ordinal (i.e. the order of the partition on a disk),
     * used for calling IoSetPartitionInformation(Ex)() API */
    UINT32 OnDiskNumber;

    BOOLEAN IsEnumerated;   //< Reported via IRP_MN_QUERY_DEVICE_RELATIONS
    BOOLEAN SymlinkCreated;
    BOOLEAN Attached;       //< Attached to PartitionList of the FDO
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

CODE_SEG("PAGE")
NTSTATUS
PartitionCreateDevice(
    _In_ PDEVICE_OBJECT FDObject,
    _In_ PPARTITION_INFORMATION_EX PartitionEntry,
    _In_ UINT32 PdoNumber,
    _In_ PARTITION_STYLE PartitionStyle,
    _Out_ PDEVICE_OBJECT *PDO);

CODE_SEG("PAGE")
NTSTATUS
PartitionHandleRemove(
    _In_ PPARTITION_EXTENSION PartExt,
    _In_ BOOLEAN FinalRemove);

CODE_SEG("PAGE")
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
