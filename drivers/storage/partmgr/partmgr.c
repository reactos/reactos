/*
 * PROJECT:     Partition manager driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main file
 * COPYRIGHT:   2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

/* The Partition Manager Driver in ReactOS complements disk.sys/classpnp.sys drivers
 * (which are derived from Windows 10 drivers) so does not do exactly what Windows 2003 partmgr.sys
 * does. Here is acts like both partition and volume manager, because volmgr.sys does not (yet)
 * exist in ReactOS. Thus handles some IOCTL_VOLUME_*, and IOCTL_MOUNTMGR_* IOCTLs.
 */

#include "partmgr.h"


static
CODE_SEG("PAGE")
PDRIVE_LAYOUT_INFORMATION
PartMgrConvertExtendedToLayout(
    _In_ CONST PDRIVE_LAYOUT_INFORMATION_EX LayoutEx)
{
    PDRIVE_LAYOUT_INFORMATION Layout;
    PPARTITION_INFORMATION Partition;
    PPARTITION_INFORMATION_EX PartitionEx;

    PAGED_CODE();

    ASSERT(LayoutEx);

    if (LayoutEx->PartitionStyle != PARTITION_STYLE_MBR)
    {
        ASSERT(FALSE);
        return NULL;
    }

    size_t layoutSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION, PartitionEntry[0]) +
                        LayoutEx->PartitionCount * sizeof (PARTITION_INFORMATION);

    Layout = ExAllocatePoolWithTag(PagedPool, layoutSize, TAG_PARTMGR);

    if (Layout == NULL)
    {
        return NULL;
    }

    Layout->Signature = LayoutEx->Mbr.Signature;
    Layout->PartitionCount = LayoutEx->PartitionCount;

    for (UINT32 i = 0; i < LayoutEx->PartitionCount; i++)
    {
        Partition = &Layout->PartitionEntry[i];
        PartitionEx = &LayoutEx->PartitionEntry[i];

        Partition->StartingOffset = PartitionEx->StartingOffset;
        Partition->PartitionLength = PartitionEx->PartitionLength;
        Partition->RewritePartition = PartitionEx->RewritePartition;
        Partition->PartitionNumber = PartitionEx->PartitionNumber;

        Partition->PartitionType = PartitionEx->Mbr.PartitionType;
        Partition->BootIndicator = PartitionEx->Mbr.BootIndicator;
        Partition->RecognizedPartition = PartitionEx->Mbr.RecognizedPartition;
        Partition->HiddenSectors = PartitionEx->Mbr.HiddenSectors;
    }

    return Layout;
}

static
CODE_SEG("PAGE")
PDRIVE_LAYOUT_INFORMATION_EX
PartMgrConvertLayoutToExtended(
    _In_ CONST PDRIVE_LAYOUT_INFORMATION Layout)
{
    PDRIVE_LAYOUT_INFORMATION_EX layoutEx;

    PAGED_CODE();

    ASSERT(Layout != NULL);

    size_t layoutSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry[0]) +
                        Layout->PartitionCount * sizeof (PARTITION_INFORMATION_EX);

    layoutEx = ExAllocatePoolUninitialized(PagedPool, layoutSize, TAG_PARTMGR);

    if (layoutEx == NULL)
    {
        return NULL;
    }

    layoutEx->PartitionStyle = PARTITION_STYLE_MBR;
    layoutEx->PartitionCount = Layout->PartitionCount;
    layoutEx->Mbr.Signature = Layout->Signature;

    for (UINT32 i = 0; i < Layout->PartitionCount; i++)
    {
        PPARTITION_INFORMATION part = &Layout->PartitionEntry[i];

        layoutEx->PartitionEntry[i] = (PARTITION_INFORMATION_EX) {
            .PartitionStyle = PARTITION_STYLE_MBR,
            .StartingOffset = part->StartingOffset,
            .PartitionLength = part->PartitionLength,
            .RewritePartition = part->RewritePartition,
            .PartitionNumber = part->PartitionNumber,
            .Mbr = {
                .PartitionType = part->PartitionType,
                .BootIndicator = part->BootIndicator,
                .RecognizedPartition = part->RecognizedPartition,
                .HiddenSectors = part->HiddenSectors,
            }
        };
    }

    return layoutEx;
}

static
CODE_SEG("PAGE")
VOID
PartMgrUpdatePartitionDevices(
    _In_ PFDO_EXTENSION FdoExtension,
    _Inout_ PDRIVE_LAYOUT_INFORMATION_EX NewLayout)
{
    NTSTATUS status;
    PSINGLE_LIST_ENTRY curEntry, prevEntry;
    UINT32 totalPartitions = 0;

    // Clear the partition numbers from the list entries
    for (UINT32 i = 0; i < NewLayout->PartitionCount; i++)
    {
        NewLayout->PartitionEntry[i].PartitionNumber = 0;
    }

    // iterate over old partition list
    prevEntry = &FdoExtension->PartitionList;
    curEntry = FdoExtension->PartitionList.Next;
    while (curEntry != NULL)
    {
        PPARTITION_EXTENSION partExt = CONTAINING_RECORD(curEntry, PARTITION_EXTENSION, ListEntry);
        UINT32 partNumber = 0; // count detected partitions for device symlinks
        BOOLEAN found = FALSE;
        PPARTITION_INFORMATION_EX partEntry;

        // trying to find this partition in returned layout
        for (UINT32 i = 0; i < NewLayout->PartitionCount; i++)
        {
            partEntry = &NewLayout->PartitionEntry[i];

            // skip unused and container partitions
            if (NewLayout->PartitionStyle == PARTITION_STYLE_MBR &&
                (partEntry->Mbr.PartitionType == PARTITION_ENTRY_UNUSED ||
                    IsContainerPartition(partEntry->Mbr.PartitionType)))
            {
                continue;
            }

            partNumber++;

            // skip already found partitions
            if (partEntry->PartitionNumber)
            {
                continue;
            }

            // skip if partitions are not equal
            if (partEntry->StartingOffset.QuadPart != partExt->StartingOffset ||
                partEntry->PartitionLength.QuadPart != partExt->PartitionLength)
            {
                continue;
            }

            // found matching partition - processing it
            found = TRUE;
            break;
        }

        if (found)
        {
            // update (possibly changed) partition metadata
            if (NewLayout->PartitionStyle == PARTITION_STYLE_MBR)
            {
                partExt->Mbr.PartitionType = partEntry->Mbr.PartitionType;
                partExt->Mbr.BootIndicator = partEntry->Mbr.BootIndicator;
            }
            else
            {
                partExt->Gpt.PartitionType = partEntry->Gpt.PartitionType;
                partExt->Gpt.PartitionId = partEntry->Gpt.PartitionId;
                partExt->Gpt.Attributes = partEntry->Gpt.Attributes;

                RtlCopyMemory(partExt->Gpt.Name, partEntry->Gpt.Name, sizeof(partExt->Gpt.Name));
            }

            partExt->OnDiskNumber = partNumber;
            partEntry->PartitionNumber = partNumber; // mark it as a found one
            totalPartitions++;
        }
        else
        {
            // detach the device from the list
            prevEntry->Next = curEntry->Next;
            curEntry = prevEntry;
            partExt->Attached = FALSE;

            // enumerated PDOs will receive IRP_MN_REMOVE_DEVICE
            if (!partExt->IsEnumerated)
            {
                PartitionHandleRemove(partExt, TRUE);
            }
        }

        prevEntry = curEntry;
        curEntry = curEntry->Next;
    }

    UINT32 partNumber = 0;
    UINT32 pdoNumber = 1;

    // now looking through remaining "new" partitions
    for (UINT32 i = 0; i < NewLayout->PartitionCount; i++)
    {
        PPARTITION_INFORMATION_EX partEntry = &NewLayout->PartitionEntry[i];

        // again, skip unused and container partitions
        if (NewLayout->PartitionStyle == PARTITION_STYLE_MBR &&
            (partEntry->Mbr.PartitionType == PARTITION_ENTRY_UNUSED ||
                IsContainerPartition(partEntry->Mbr.PartitionType)))
        {
            continue;
        }

        partNumber++;

        // and skip processed partitions
        if (partEntry->PartitionNumber != 0)
        {
            continue;
        }

        // find the first free PDO index
        for (PSINGLE_LIST_ENTRY curEntry = FdoExtension->PartitionList.Next;
             curEntry != NULL;
             curEntry = curEntry->Next)
        {
            PPARTITION_EXTENSION partExt = CONTAINING_RECORD(curEntry,
                                                             PARTITION_EXTENSION,
                                                             ListEntry);

            if (partExt->DetectedNumber == pdoNumber)
            {
                // found a matching pdo number - restart the search
                curEntry = FdoExtension->PartitionList.Next;
                pdoNumber++;
            }
        }

        partEntry->PartitionNumber = partNumber;

        PDEVICE_OBJECT partitionDevice;
        status = PartitionCreateDevice(FdoExtension->DeviceObject,
                                       partEntry,
                                       pdoNumber,
                                       NewLayout->PartitionStyle,
                                       &partitionDevice);

        if (!NT_SUCCESS(status))
        {
            partEntry->PartitionNumber = 0;
            continue;
        }

        totalPartitions++;

        // insert the structure to the partition list
        curEntry = FdoExtension->PartitionList.Next;
        prevEntry = NULL;
        while (curEntry != NULL)
        {
            PPARTITION_EXTENSION curPart = CONTAINING_RECORD(curEntry,
                                                             PARTITION_EXTENSION,
                                                             ListEntry);
            if (curPart->OnDiskNumber < partNumber)
            {
                prevEntry = curEntry;
                curEntry = curPart->ListEntry.Next;
            }
            else
            { // we found where to put the partition
                break;
            }
        }

        PPARTITION_EXTENSION partExt = partitionDevice->DeviceExtension;

        if (prevEntry)
        {
            // insert after prevEntry
            partExt->ListEntry.Next = prevEntry->Next;
            prevEntry->Next = &partExt->ListEntry;
        }
        else
        {
            // insert in the beginning
            partExt->ListEntry.Next = FdoExtension->PartitionList.Next;
            FdoExtension->PartitionList.Next = &partExt->ListEntry;
        }

        partExt->Attached = TRUE;
    }

    FdoExtension->EnumeratedPartitionsTotal = totalPartitions;
}

// requires partitioning lock held
static
CODE_SEG("PAGE")
NTSTATUS
PartMgrGetDriveLayout(
    _In_ PFDO_EXTENSION FdoExtension,
    _Out_ PDRIVE_LAYOUT_INFORMATION_EX *DriveLayout)
{
    PAGED_CODE();

    if (FdoExtension->LayoutValid)
    {
        *DriveLayout = FdoExtension->LayoutCache;
        return STATUS_SUCCESS;
    }

    PDRIVE_LAYOUT_INFORMATION_EX layoutEx = NULL;
    NTSTATUS status = IoReadPartitionTableEx(FdoExtension->LowerDevice, &layoutEx);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (FdoExtension->LayoutCache)
    {
        ExFreePool(FdoExtension->LayoutCache);
    }

    FdoExtension->LayoutCache = layoutEx;
    FdoExtension->LayoutValid = TRUE;

    *DriveLayout = layoutEx;

    return status;
}

static
CODE_SEG("PAGE")
NTSTATUS
FdoIoctlDiskGetDriveGeometryEx(
    _In_ PFDO_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();

    // We're patching the DISK_PARTITION_INFO part of the returned structure
    // as disk.sys doesn't really know about the partition table on a disk

    PDISK_GEOMETRY_EX_INTERNAL geometryEx = Irp->AssociatedIrp.SystemBuffer;
    size_t outBufferLength = ioStack->Parameters.DeviceIoControl.OutputBufferLength;
    NTSTATUS status;

    status = IssueSyncIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                                       FdoExtension->LowerDevice,
                                       NULL,
                                       0,
                                       geometryEx,
                                       outBufferLength,
                                       FALSE);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // if DISK_PARTITION_INFO fits the output size
    if (outBufferLength >= FIELD_OFFSET(DISK_GEOMETRY_EX_INTERNAL, Detection))
    {
        PartMgrAcquireLayoutLock(FdoExtension);

        geometryEx->Partition.SizeOfPartitionInfo = sizeof(geometryEx->Partition);
        geometryEx->Partition.PartitionStyle = FdoExtension->DiskData.PartitionStyle;

        switch (geometryEx->Partition.PartitionStyle)
        {
            case PARTITION_STYLE_MBR:
                geometryEx->Partition.Mbr.Signature = FdoExtension->DiskData.Mbr.Signature;
                // checksum?
                break;

            case PARTITION_STYLE_GPT:
                geometryEx->Partition.Gpt.DiskId = FdoExtension->DiskData.Gpt.DiskId;
                break;

            default:
                RtlZeroMemory(&geometryEx->Partition, sizeof(geometryEx->Partition));
        }

        PartMgrReleaseLayoutLock(FdoExtension);
    }

    // the logic is copied from disk.sys
    Irp->IoStatus.Information = min(outBufferLength, sizeof(DISK_GEOMETRY_EX_INTERNAL));

    return status;
}

static
CODE_SEG("PAGE")
NTSTATUS
FdoIoctlDiskGetPartitionInfo(
    _In_ PFDO_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PPARTITION_INFORMATION partInfo = Irp->AssociatedIrp.SystemBuffer;

    PAGED_CODE();

    if (!VerifyIrpOutBufferSize(Irp, sizeof(*partInfo)))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PartMgrAcquireLayoutLock(FdoExtension);

    *partInfo = (PARTITION_INFORMATION){
        .PartitionType = PARTITION_ENTRY_UNUSED,
        .StartingOffset.QuadPart = 0,
        .PartitionLength.QuadPart = FdoExtension->DiskData.DiskSize,
        .HiddenSectors = 0,
        .PartitionNumber = 0,
        .BootIndicator = FALSE,
        .RewritePartition = FALSE,
        .RecognizedPartition = FALSE,
    };

    PartMgrReleaseLayoutLock(FdoExtension);

    Irp->IoStatus.Information = sizeof(*partInfo);
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
FdoIoctlDiskGetPartitionInfoEx(
    _In_ PFDO_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PPARTITION_INFORMATION_EX partInfoEx = Irp->AssociatedIrp.SystemBuffer;

    PAGED_CODE();

    if (!VerifyIrpOutBufferSize(Irp, sizeof(*partInfoEx)))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    PartMgrAcquireLayoutLock(FdoExtension);

    // most of the fields a zeroed for Partition0
    *partInfoEx = (PARTITION_INFORMATION_EX){
        .PartitionLength.QuadPart = FdoExtension->DiskData.DiskSize,
        .PartitionStyle = FdoExtension->DiskData.PartitionStyle,
    };

    PartMgrReleaseLayoutLock(FdoExtension);

    Irp->IoStatus.Information = sizeof(*partInfoEx);
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
FdoIoctlDiskGetDriveLayout(
    _In_ PFDO_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    PartMgrAcquireLayoutLock(FdoExtension);

    PDRIVE_LAYOUT_INFORMATION_EX layoutEx;
    NTSTATUS status = PartMgrGetDriveLayout(FdoExtension, &layoutEx);

    if (!NT_SUCCESS(status))
    {
        PartMgrReleaseLayoutLock(FdoExtension);
        return status;
    }

    // checking this value from layoutEx in case it has been changed
    if (layoutEx->PartitionStyle != PARTITION_STYLE_MBR)
    {
        PartMgrReleaseLayoutLock(FdoExtension);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    size_t size = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION, PartitionEntry[0]);
    size += layoutEx->PartitionCount * sizeof(PARTITION_INFORMATION);

    if (!VerifyIrpOutBufferSize(Irp, size))
    {
        PartMgrReleaseLayoutLock(FdoExtension);
        return STATUS_BUFFER_TOO_SMALL;
    }

    PDRIVE_LAYOUT_INFORMATION partitionList = PartMgrConvertExtendedToLayout(layoutEx);

    PartMgrReleaseLayoutLock(FdoExtension);

    if (partitionList == NULL)
    {
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer, partitionList, size);
    ExFreePoolWithTag(partitionList, TAG_PARTMGR);

    Irp->IoStatus.Information = size;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
FdoIoctlDiskGetDriveLayoutEx(
    _In_ PFDO_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    PartMgrAcquireLayoutLock(FdoExtension);

    PDRIVE_LAYOUT_INFORMATION_EX layoutEx;
    NTSTATUS status = PartMgrGetDriveLayout(FdoExtension, &layoutEx);
    if (!NT_SUCCESS(status))
    {
        PartMgrReleaseLayoutLock(FdoExtension);
        return status;
    }

    size_t size = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry[0]);
    size += layoutEx->PartitionCount * sizeof(PARTITION_INFORMATION_EX);

    if (!VerifyIrpOutBufferSize(Irp, size))
    {
        PartMgrReleaseLayoutLock(FdoExtension);
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, layoutEx, size);

    PartMgrReleaseLayoutLock(FdoExtension);

    Irp->IoStatus.Information = size;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
FdoIoctlDiskSetDriveLayout(
    _In_ PFDO_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PDRIVE_LAYOUT_INFORMATION layoutInfo = Irp->AssociatedIrp.SystemBuffer;

    PAGED_CODE();

    if (!VerifyIrpInBufferSize(Irp, sizeof(*layoutInfo)))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    size_t layoutSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION, PartitionEntry[0]);
    layoutSize += layoutInfo->PartitionCount * sizeof(PARTITION_INFORMATION);

    if (!VerifyIrpInBufferSize(Irp, layoutSize))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    PDRIVE_LAYOUT_INFORMATION_EX layoutEx = PartMgrConvertLayoutToExtended(layoutInfo);

    if (layoutEx == NULL)
    {
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    PartMgrAcquireLayoutLock(FdoExtension);

    // this in fact updates the bus relations
    PartMgrUpdatePartitionDevices(FdoExtension, layoutEx);

    // write the partition table to the disk
    NTSTATUS status = IoWritePartitionTableEx(FdoExtension->LowerDevice, layoutEx);
    if (NT_SUCCESS(status))
    {
        // save the layout cache
        if (FdoExtension->LayoutCache)
        {
            ExFreePool(FdoExtension->LayoutCache);
        }
        FdoExtension->LayoutCache = layoutEx;
        FdoExtension->LayoutValid = TRUE;

        // set updated partition numbers
        for (UINT32 i = 0; i < layoutInfo->PartitionCount; i++)
        {
            PPARTITION_INFORMATION part = &layoutInfo->PartitionEntry[i];

            part->PartitionNumber = layoutEx->PartitionEntry[i].PartitionNumber;
        }
    }
    else
    {
        FdoExtension->LayoutValid = FALSE;
    }

    PartMgrReleaseLayoutLock(FdoExtension);

    IoInvalidateDeviceRelations(FdoExtension->PhysicalDiskDO, BusRelations);

    // notify everyone that the disk layout has changed
    TARGET_DEVICE_CUSTOM_NOTIFICATION notification;

    notification.Event = GUID_IO_DISK_LAYOUT_CHANGE;
    notification.Version = 1;
    notification.Size = FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer);
    notification.FileObject = NULL;
    notification.NameBufferOffset = -1;

    IoReportTargetDeviceChangeAsynchronous(FdoExtension->PhysicalDiskDO,
                                           &notification,
                                           NULL,
                                           NULL);

    Irp->IoStatus.Information = layoutSize;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
FdoIoctlDiskSetDriveLayoutEx(
    _In_ PFDO_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PDRIVE_LAYOUT_INFORMATION_EX layoutEx, layoutUser = Irp->AssociatedIrp.SystemBuffer;
    NTSTATUS status;

    PAGED_CODE();

    if (!VerifyIrpInBufferSize(Irp, sizeof(*layoutUser)))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    size_t layoutSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry[0]);
    layoutSize += layoutUser->PartitionCount * sizeof(PARTITION_INFORMATION_EX);

    if (!VerifyIrpInBufferSize(Irp, layoutSize))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    // we need to copy the structure from the IRP input buffer
    layoutEx = ExAllocatePoolWithTag(PagedPool, layoutSize, TAG_PARTMGR);
    if (!layoutEx)
    {
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(layoutEx, layoutUser, layoutSize);

    PartMgrAcquireLayoutLock(FdoExtension);

    // if partition count is 0, it's the same as IOCTL_DISK_CREATE_DISK
    if (layoutEx->PartitionCount == 0)
    {
        CREATE_DISK createDisk = {0};
        createDisk.PartitionStyle = layoutEx->PartitionStyle;
        if (createDisk.PartitionStyle == PARTITION_STYLE_MBR)
        {
            createDisk.Mbr.Signature = layoutEx->Mbr.Signature;
        }
        else if (createDisk.PartitionStyle == PARTITION_STYLE_GPT)
        {
            createDisk.Gpt.DiskId = layoutEx->Gpt.DiskId;
        }

        status = IoCreateDisk(FdoExtension->LowerDevice, &createDisk);
    }
    else
    {
        // this in fact updates the bus relations
        PartMgrUpdatePartitionDevices(FdoExtension, layoutEx);

        // write the partition table to the disk
        status = IoWritePartitionTableEx(FdoExtension->LowerDevice, layoutEx);
        if (NT_SUCCESS(status))
        {
            // set updated partition numbers
            for (UINT32 i = 0; i < layoutEx->PartitionCount; i++)
            {
                PPARTITION_INFORMATION_EX part = &layoutEx->PartitionEntry[i];

                part->PartitionNumber = layoutEx->PartitionEntry[i].PartitionNumber;
            }
        }
    }

    // update the layout cache
    if (NT_SUCCESS(status))
    {
        if (FdoExtension->LayoutCache)
        {
            ExFreePool(FdoExtension->LayoutCache);
        }
        FdoExtension->LayoutCache = layoutEx;
        FdoExtension->LayoutValid = TRUE;
    }
    else
    {
        FdoExtension->LayoutValid = FALSE;
    }

    PartMgrReleaseLayoutLock(FdoExtension);

    IoInvalidateDeviceRelations(FdoExtension->PhysicalDiskDO, BusRelations);

    // notify everyone that the disk layout has changed
    TARGET_DEVICE_CUSTOM_NOTIFICATION notification;

    notification.Event = GUID_IO_DISK_LAYOUT_CHANGE;
    notification.Version = 1;
    notification.Size = FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer);
    notification.FileObject = NULL;
    notification.NameBufferOffset = -1;

    IoReportTargetDeviceChangeAsynchronous(FdoExtension->PhysicalDiskDO,
                                           &notification,
                                           NULL,
                                           NULL);

    Irp->IoStatus.Information = layoutSize;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
FdoIoctlDiskUpdateProperties(
    _In_ PFDO_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    PartMgrAcquireLayoutLock(FdoExtension);
    FdoExtension->LayoutValid = FALSE;
    PartMgrReleaseLayoutLock(FdoExtension);

    IoInvalidateDeviceRelations(FdoExtension->PhysicalDiskDO, BusRelations);
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
FdoIoctlDiskCreateDisk(
    _In_ PFDO_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    PCREATE_DISK createDisk = Irp->AssociatedIrp.SystemBuffer;
    if (!VerifyIrpInBufferSize(Irp, sizeof(*createDisk)))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    PartMgrAcquireLayoutLock(FdoExtension);

    NTSTATUS status = IoCreateDisk(FdoExtension->LowerDevice, createDisk);

    FdoExtension->LayoutValid = FALSE;
    PartMgrReleaseLayoutLock(FdoExtension);

    IoInvalidateDeviceRelations(FdoExtension->PhysicalDiskDO, BusRelations);
    return status;
}

static
CODE_SEG("PAGE")
NTSTATUS
FdoIoctlDiskDeleteDriveLayout(
    _In_ PFDO_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    CREATE_DISK createDisk = { .PartitionStyle = PARTITION_STYLE_RAW };

    PAGED_CODE();

    PartMgrAcquireLayoutLock(FdoExtension);

    NTSTATUS status = IoCreateDisk(FdoExtension->LowerDevice, &createDisk);

    FdoExtension->LayoutValid = FALSE;
    PartMgrReleaseLayoutLock(FdoExtension);

    IoInvalidateDeviceRelations(FdoExtension->PhysicalDiskDO, BusRelations);
    return status;
}

static
CODE_SEG("PAGE")
NTSTATUS
FdoHandleStartDevice(
    _In_ PFDO_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    // obtain the disk device number
    // this is not expected to change thus not in PartMgrRefreshDiskData
    STORAGE_DEVICE_NUMBER deviceNumber;
    NTSTATUS status = IssueSyncIoControlRequest(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                                FdoExtension->LowerDevice,
                                                NULL,
                                                0,
                                                &deviceNumber,
                                                sizeof(deviceNumber),
                                                FALSE);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    FdoExtension->DiskData.DeviceNumber = deviceNumber.DeviceNumber;
    return status;
}

// requires partitioning lock held
static
CODE_SEG("PAGE")
NTSTATUS
PartMgrRefreshDiskData(
    _In_ PFDO_EXTENSION FdoExtension)
{
    NTSTATUS status;

    PAGED_CODE();

    // get the DiskSize and BytesPerSector
    DISK_GEOMETRY_EX geometryEx;
    status = IssueSyncIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                                       FdoExtension->LowerDevice,
                                       NULL,
                                       0,
                                       &geometryEx,
                                       sizeof(geometryEx),
                                       FALSE);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    FdoExtension->DiskData.DiskSize = geometryEx.DiskSize.QuadPart;
    FdoExtension->DiskData.BytesPerSector = geometryEx.Geometry.BytesPerSector;

    // get the partition style-related info
    PDRIVE_LAYOUT_INFORMATION_EX layoutEx = NULL;
    status = PartMgrGetDriveLayout(FdoExtension, &layoutEx);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    FdoExtension->DiskData.PartitionStyle = layoutEx->PartitionStyle;
    if (FdoExtension->DiskData.PartitionStyle == PARTITION_STYLE_MBR)
    {
        FdoExtension->DiskData.Mbr.Signature = layoutEx->Mbr.Signature;
        // FdoExtension->DiskData.Mbr.Checksum = geometryEx.Partition.Mbr.CheckSum;
    }
    else
    {
        FdoExtension->DiskData.Gpt.DiskId = layoutEx->Gpt.DiskId;
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
FdoHandleDeviceRelations(
    _In_ PFDO_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    DEVICE_RELATION_TYPE type = ioStack->Parameters.QueryDeviceRelations.Type;

    PAGED_CODE();

    if (type == BusRelations)
    {
        PartMgrAcquireLayoutLock(FdoExtension);

        NTSTATUS status = PartMgrRefreshDiskData(FdoExtension);
        if (!NT_SUCCESS(status))
        {
            PartMgrReleaseLayoutLock(FdoExtension);
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Irp->IoStatus.Status;
        }

        INFO("Partition style %u\n", FdoExtension->DiskData.PartitionStyle);

        // PartMgrAcquireLayoutLock calls PartMgrGetDriveLayout inside
        // so we're sure here that it returns only cached layout
        PDRIVE_LAYOUT_INFORMATION_EX layoutEx;
        PartMgrGetDriveLayout(FdoExtension, &layoutEx);

        PartMgrUpdatePartitionDevices(FdoExtension, layoutEx);

        // now fill the DeviceRelations structure
        TRACE("Reporting %u partitions\n", FdoExtension->EnumeratedPartitionsTotal);

        PDEVICE_RELATIONS deviceRelations =
            ExAllocatePoolWithTag(PagedPool,
                                  sizeof(DEVICE_RELATIONS)
                                  + sizeof(PDEVICE_OBJECT)
                                  * (FdoExtension->EnumeratedPartitionsTotal - 1),
                                  TAG_PARTMGR);

        if (!deviceRelations)
        {
            PartMgrReleaseLayoutLock(FdoExtension);
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Irp->IoStatus.Status;
        }

        deviceRelations->Count = 0;

        PSINGLE_LIST_ENTRY curEntry = FdoExtension->PartitionList.Next;
        while (curEntry != NULL)
        {
            PPARTITION_EXTENSION partExt = CONTAINING_RECORD(curEntry,
                                                             PARTITION_EXTENSION,
                                                             ListEntry);

            // mark the PDO to know that we don't need to manually delete it
            partExt->IsEnumerated = TRUE;
            deviceRelations->Objects[deviceRelations->Count++] = partExt->DeviceObject;
            ObReferenceObject(partExt->DeviceObject);

            curEntry = partExt->ListEntry.Next;
        }

        ASSERT(deviceRelations->Count == FdoExtension->EnumeratedPartitionsTotal);

        PartMgrReleaseLayoutLock(FdoExtension);

        Irp->IoStatus.Information = (ULONG_PTR)deviceRelations;
        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(FdoExtension->LowerDevice, Irp);
}

static
CODE_SEG("PAGE")
NTSTATUS
FdoHandleRemoveDevice(
    _In_ PFDO_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    for (PSINGLE_LIST_ENTRY curEntry = FdoExtension->PartitionList.Next;
         curEntry != NULL;
         curEntry = curEntry->Next)
    {
        PPARTITION_EXTENSION partExt = CONTAINING_RECORD(curEntry,
                                                         PARTITION_EXTENSION,
                                                         ListEntry);

        ASSERT(partExt->DeviceRemoved);
    }

    // Send the IRP down the stack
    IoSkipCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    NTSTATUS status = IoCallDriver(FdoExtension->LowerDevice, Irp);

    IoDetachDevice(FdoExtension->LowerDevice);
    IoDeleteDevice(FdoExtension->DeviceObject);
    return status;
}

static
CODE_SEG("PAGE")
NTSTATUS
FdoHandleSurpriseRemoval(
    _In_ PFDO_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    // all enumerated child devices should receive IRP_MN_REMOVE_DEVICE
    // removing only non-enumerated ones here
    for (PSINGLE_LIST_ENTRY curEntry = FdoExtension->PartitionList.Next;
         curEntry != NULL;
         curEntry = curEntry->Next)
    {
        PPARTITION_EXTENSION partExt = CONTAINING_RECORD(curEntry,
                                                         PARTITION_EXTENSION,
                                                         ListEntry);

        if (partExt->IsEnumerated)
        {
            PartitionHandleRemove(partExt, TRUE);
        }
    }

    // Send the IRP down the stack
    IoSkipCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return IoCallDriver(FdoExtension->LowerDevice, Irp);
}

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
PartMgrAddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    NTSTATUS status = IoCreateDevice(DriverObject,
                                     sizeof(FDO_EXTENSION),
                                     0,
                                     FILE_DEVICE_BUS_EXTENDER,
                                     FILE_AUTOGENERATED_DEVICE_NAME | FILE_DEVICE_SECURE_OPEN,
                                     FALSE,
                                     &deviceObject);

    if (!NT_SUCCESS(status))
    {
        ERR("Failed to create FDO 0x%x\n", status);
        return status;
    }

    PFDO_EXTENSION deviceExtension = deviceObject->DeviceExtension;
    RtlZeroMemory(deviceExtension, sizeof(*deviceExtension));

    deviceExtension->IsFDO = TRUE;
    deviceExtension->DeviceObject = deviceObject;
    deviceExtension->LowerDevice = IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);
    deviceExtension->PhysicalDiskDO = PhysicalDeviceObject;
    KeInitializeEvent(&deviceExtension->SyncEvent, SynchronizationEvent, TRUE);

    // the the attaching failed
    if (!deviceExtension->LowerDevice)
    {
        IoDeleteDevice(deviceObject);

        return STATUS_DEVICE_REMOVED;
    }
    deviceObject->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;

    // device is initialized
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
PartMgrDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    PFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    // Note: IRP_MJ_DEVICE_CONTROL handler in the storage stack must be able to pass IOCTLs
    // at an IRQL higher than PASSIVE_LEVEL

    INFO("IRP_MJ_DEVICE_CONTROL %p Irp %p IOCTL %x isFdo: %u\n",
        DeviceObject, Irp, ioStack->Parameters.DeviceIoControl.IoControlCode, fdoExtension->IsFDO);

    if (!fdoExtension->IsFDO)
    {
        return PartitionHandleDeviceControl(DeviceObject, Irp);
    }

    switch (ioStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:
            status = FdoIoctlDiskGetDriveGeometryEx(fdoExtension, Irp);
            break;

        case IOCTL_DISK_GET_PARTITION_INFO:
            status = FdoIoctlDiskGetPartitionInfo(fdoExtension, Irp);
            break;

        case IOCTL_DISK_GET_PARTITION_INFO_EX:
            status = FdoIoctlDiskGetPartitionInfoEx(fdoExtension, Irp);
            break;

        case IOCTL_DISK_GET_DRIVE_LAYOUT:
            status = FdoIoctlDiskGetDriveLayout(fdoExtension, Irp);
            break;

        case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
            status = FdoIoctlDiskGetDriveLayoutEx(fdoExtension, Irp);
            break;

        case IOCTL_DISK_SET_DRIVE_LAYOUT:
            status = FdoIoctlDiskSetDriveLayout(fdoExtension, Irp);
            break;

        case IOCTL_DISK_SET_DRIVE_LAYOUT_EX:
            status = FdoIoctlDiskSetDriveLayoutEx(fdoExtension, Irp);
            break;

        case IOCTL_DISK_UPDATE_PROPERTIES:
            status = FdoIoctlDiskUpdateProperties(fdoExtension, Irp);
            break;

        case IOCTL_DISK_CREATE_DISK:
            status = FdoIoctlDiskCreateDisk(fdoExtension, Irp);
            break;

        case IOCTL_DISK_DELETE_DRIVE_LAYOUT:
            status = FdoIoctlDiskDeleteDriveLayout(fdoExtension, Irp);
            break;
        // case IOCTL_DISK_GROW_PARTITION: // todo
        default:
            return ForwardIrpAndForget(DeviceObject, Irp);
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
PartMgrPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    PFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);

    INFO("IRP_MJ_PNP %p Irp %p %s isFDO: %u\n",
        DeviceObject, Irp, GetIRPMinorFunctionString(ioStack->MinorFunction), fdoExtension->IsFDO);

    if (!fdoExtension->IsFDO)
    {
        return PartitionHandlePnp(DeviceObject, Irp);
    }

    switch (ioStack->MinorFunction) {

        case IRP_MN_START_DEVICE:
        {
            NTSTATUS status;

            // if this is sent to the FDO so we should forward it down the
            // attachment chain before we can start the FDO

            if (!IoForwardIrpSynchronously(fdoExtension->LowerDevice, Irp))
            {
                status = STATUS_UNSUCCESSFUL;
            }
            else
            {
                status = FdoHandleStartDevice(fdoExtension, Irp);
            }

            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
        }
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            return FdoHandleDeviceRelations(fdoExtension, Irp);
        }
        case IRP_MN_SURPRISE_REMOVAL:
        {
            return FdoHandleSurpriseRemoval(fdoExtension, Irp);
        }
        case IRP_MN_REMOVE_DEVICE:
        {
            return FdoHandleRemoveDevice(fdoExtension, Irp);
        }
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
        {
            Irp->IoStatus.Status = STATUS_SUCCESS;
            // fallthrough
        }
        default:
        {
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(fdoExtension->LowerDevice, Irp);
        }
    }
}

static
NTSTATUS
NTAPI
PartMgrReadWrite(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PPARTITION_EXTENSION partExt = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);

    if (!partExt->IsFDO)
    {
        if (!partExt->IsEnumerated)
        {
            Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_DEVICE_DOES_NOT_EXIST;
        }
        else
        {
            ioStack->Parameters.Read.ByteOffset.QuadPart += partExt->StartingOffset;
        }
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(partExt->LowerDevice, Irp);
}

DRIVER_DISPATCH PartMgrPower;
NTSTATUS
NTAPI
PartMgrPower(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PPARTITION_EXTENSION partExt = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);

    PoStartNextPowerIrp(Irp);

    if (!partExt->IsFDO)
    {
        NTSTATUS status;

        if (!partExt->IsEnumerated)
        {
            status = STATUS_DEVICE_DOES_NOT_EXIST;
        }
        else if (ioStack->MinorFunction == IRP_MN_SET_POWER ||
                 ioStack->MinorFunction == IRP_MN_QUERY_POWER)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = Irp->IoStatus.Status;
        }

        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
    else
    {
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(partExt->LowerDevice, Irp);
    }
}

DRIVER_DISPATCH PartMgrShutdownFlush;
NTSTATUS
NTAPI
PartMgrShutdownFlush(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PPARTITION_EXTENSION partExt = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT lowerDevice;

    // forward to the partition0 device in both cases
    if (!partExt->IsFDO)
    {
        if (!partExt->IsEnumerated)
        {
            Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_DEVICE_DOES_NOT_EXIST;
        }
        else
        {
            PFDO_EXTENSION fdoExtension = partExt->LowerDevice->DeviceExtension;
            lowerDevice = fdoExtension->LowerDevice;
        }
    }
    else
    {
        lowerDevice = partExt->LowerDevice;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(lowerDevice, Irp);
}

CODE_SEG("PAGE")
VOID
NTAPI
PartMgrUnload(
    _In_ PDRIVER_OBJECT DriverObject)
{

}

CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    DriverObject->DriverUnload = PartMgrUnload;
    DriverObject->DriverExtension->AddDevice = PartMgrAddDevice;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = ForwardIrpAndForget;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = ForwardIrpAndForget;
    DriverObject->MajorFunction[IRP_MJ_READ]           = PartMgrReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE]          = PartMgrReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PartMgrDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP]            = PartMgrPnp;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]       = PartMgrShutdownFlush;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]  = PartMgrShutdownFlush;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = PartMgrPower;

    return STATUS_SUCCESS;
}
