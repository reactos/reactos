/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2014 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystem/ntfs/volume.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMERS:      Eric Kohl
 *                   Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/**
* @name NtfsLoadVolumeBitmap
*
* Reads $Bitmap into memory once at mount. Cluster allocation and freeing work
* on this copy and write back only the sectors they change.
*
* @param DeviceExt
* Volume being mounted. Its file record lookaside list and MFT context must be ready.
*
* @return
* STATUS_SUCCESS, or an error if $Bitmap could not be found or read.
*/
NTSTATUS
NtfsLoadVolumeBitmap(
    _In_ PDEVICE_EXTENSION DeviceExt)
{
    PFILE_RECORD_HEADER BitmapRecord;
    ULONGLONG BitmapSize;
    ULONG BufferSize;
    NTSTATUS Status;

    BitmapRecord = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
    if (BitmapRecord == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = ReadFileRecord(DeviceExt, NTFS_FILE_BITMAP, BitmapRecord);
    if (NT_SUCCESS(Status))
    {
        /* The context keeps its own copy of the attribute record */
        Status = FindAttribute(DeviceExt, BitmapRecord, AttributeData, L"", 0, &DeviceExt->BitmapContext, NULL);
    }

    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, BitmapRecord);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Unable to find $Bitmap (Status %lx)\n", Status);
        DeviceExt->BitmapContext = NULL;
        return Status;
    }

    BitmapSize = AttributeDataLength(DeviceExt->BitmapContext->pRecord);
    if (BitmapSize > MAXULONG ||
        BitmapSize * 8 < DeviceExt->NtfsInfo.ClusterCount ||
        DeviceExt->NtfsInfo.ClusterCount > MAXULONG)
    {
        DPRINT1("$Bitmap size %I64u does not cover %I64u clusters\n",
                BitmapSize, DeviceExt->NtfsInfo.ClusterCount);
        Status = STATUS_FILE_CORRUPT_ERROR;
        goto Fail;
    }

    /* Whole sectors, so write back never reads first */
    BufferSize = ROUND_UP((ULONG)BitmapSize, DeviceExt->NtfsInfo.BytesPerSector);
    DeviceExt->BitmapBuffer = ExAllocatePoolWithTag(PagedPool, BufferSize, TAG_NTFS);
    if (DeviceExt->BitmapBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Fail;
    }
    RtlZeroMemory(DeviceExt->BitmapBuffer, BufferSize);

    if (ReadAttribute(DeviceExt, DeviceExt->BitmapContext, 0, (PCHAR)DeviceExt->BitmapBuffer, (ULONG)BitmapSize) != (ULONG)BitmapSize)
    {
        DPRINT1("Unable to read $Bitmap\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Fail;
    }

    DeviceExt->BitmapSize = (ULONG)BitmapSize;

    /* $Bitmap is rounded up to a cluster and covers a few clusters more than
     * the volume has. Bounding by ClusterCount ignores the surplus bits, but
     * write back still preserves them on disk. */
    RtlInitializeBitMap(&DeviceExt->ClusterBitmap, DeviceExt->BitmapBuffer, (ULONG)DeviceExt->NtfsInfo.ClusterCount);
    DeviceExt->FreeClusterCount = RtlNumberOfClearBits(&DeviceExt->ClusterBitmap);

    DPRINT("Volume bitmap loaded: %I64u of %I64u clusters free\n",
           DeviceExt->FreeClusterCount, DeviceExt->NtfsInfo.ClusterCount);

    return STATUS_SUCCESS;

Fail:
    NtfsFreeVolumeBitmap(DeviceExt);
    return Status;
}

/**
* @name NtfsFreeVolumeBitmap
*
* Releases what NtfsLoadVolumeBitmap() set up. Safe on a partly loaded volume.
*
* @param DeviceExt
* Volume whose bitmap copy is released.
*/
VOID
NtfsFreeVolumeBitmap(
    _In_ PDEVICE_EXTENSION DeviceExt)
{
    if (DeviceExt->BitmapBuffer != NULL)
    {
        ExFreePoolWithTag(DeviceExt->BitmapBuffer, TAG_NTFS);
        DeviceExt->BitmapBuffer = NULL;
    }

    if (DeviceExt->BitmapContext != NULL)
    {
        ReleaseAttributeContext(DeviceExt->BitmapContext);
        DeviceExt->BitmapContext = NULL;
    }
}

/**
* @name NtfsWriteVolumeBitmap
*
* Writes the sectors of $Bitmap that hold the given clusters' bits.
* The caller holds BitmapResource exclusively.
*
* @param DeviceExt
* Volume whose bitmap changed.
*
* @param FirstCluster
* First cluster whose bit changed.
*
* @param ClusterCount
* Number of clusters, starting at FirstCluster, to cover. Can't be 0.
*
* @return
* Status from WriteAttribute().
*/
NTSTATUS
NtfsWriteVolumeBitmap(
    _In_ PDEVICE_EXTENSION DeviceExt,
    _In_ ULONG FirstCluster,
    _In_ ULONG ClusterCount)
{
    ULONG SectorSize = DeviceExt->NtfsInfo.BytesPerSector;
    ULONG Start;
    ULONG End;
    ULONG LengthWritten;

    ASSERT(ClusterCount != 0);

    Start = ROUND_DOWN(FirstCluster / 8, SectorSize);
    End = ROUND_UP((FirstCluster + ClusterCount - 1) / 8 + 1, SectorSize);
    End = min(End, ROUND_UP(DeviceExt->BitmapSize, SectorSize));

    return WriteAttribute(DeviceExt,
                          DeviceExt->BitmapContext,
                          Start,
                          (PUCHAR)DeviceExt->BitmapBuffer + Start,
                          End - Start,
                          &LengthWritten,
                          NULL);
}

ULONGLONG
NtfsGetFreeClusters(PDEVICE_EXTENSION DeviceExt)
{
    ULONGLONG FreeClusters;

    DPRINT("NtfsGetFreeClusters(%p)\n", DeviceExt);

    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&DeviceExt->BitmapResource, TRUE);
    FreeClusters = DeviceExt->FreeClusterCount;
    ExReleaseResourceLite(&DeviceExt->BitmapResource);
    KeLeaveCriticalRegion();

    return FreeClusters;
}

/**
* NtfsAllocateClusters
* Allocates a run of clusters. The run allocated might be smaller than DesiredClusters.
*/
NTSTATUS
NtfsAllocateClusters(PDEVICE_EXTENSION DeviceExt,
                     ULONG FirstDesiredCluster,
                     ULONG DesiredClusters,
                     PULONG FirstAssignedCluster,
                     PULONG AssignedClusters)
{
    NTSTATUS Status;
    PRTL_BITMAP Bitmap = &DeviceExt->ClusterBitmap;
    ULONG AssignedRun;

    DPRINT("NtfsAllocateClusters(%p, %lu, %lu, %p, %p)\n", DeviceExt, FirstDesiredCluster, DesiredClusters, FirstAssignedCluster, AssignedClusters);

    *AssignedClusters = 0;

    /* Files growing in parallel must not both claim the same free run */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&DeviceExt->BitmapResource, TRUE);

    if (DeviceExt->FreeClusterCount < DesiredClusters)
    {
        Status = STATUS_DISK_FULL;
        goto Quit;
    }

    /* The hint is just past the file's last cluster, which can be past the volume end */
    if (FirstDesiredCluster >= Bitmap->SizeOfBitMap)
        FirstDesiredCluster = 0;

    // TODO: Observe MFT reservation zone

    // Can we get one contiguous run?
    AssignedRun = RtlFindClearBitsAndSet(Bitmap, DesiredClusters, FirstDesiredCluster);

    if (AssignedRun != 0xFFFFFFFF)
    {
        *FirstAssignedCluster = AssignedRun;
        *AssignedClusters = DesiredClusters;
    }
    else
    {
        // we can't get one contiguous run
        *AssignedClusters = RtlFindNextForwardRunClear(Bitmap, FirstDesiredCluster, FirstAssignedCluster);

        if (*AssignedClusters == 0)
        {
            // we couldn't find any runs starting at DesiredFirstCluster
            *AssignedClusters = RtlFindLongestRunClear(Bitmap, FirstAssignedCluster);
        }

        if (*AssignedClusters == 0)
        {
            Status = STATUS_DISK_FULL;
            goto Quit;
        }

        /* Hand out no more than was asked for, and mark it in use */
        *AssignedClusters = min(*AssignedClusters, DesiredClusters);
        RtlSetBits(Bitmap, *FirstAssignedCluster, *AssignedClusters);
    }

    Status = NtfsWriteVolumeBitmap(DeviceExt, *FirstAssignedCluster, *AssignedClusters);
    if (NT_SUCCESS(Status))
    {
        DeviceExt->FreeClusterCount -= *AssignedClusters;
    }
    else
    {
        /* Keep the copy in step with what the disk says */
        RtlClearBits(Bitmap, *FirstAssignedCluster, *AssignedClusters);
        *AssignedClusters = 0;
    }

Quit:
    ExReleaseResourceLite(&DeviceExt->BitmapResource);
    KeLeaveCriticalRegion();

    return Status;
}

static
NTSTATUS
NtfsGetFsVolumeInformation(PDEVICE_OBJECT DeviceObject,
                           PFILE_FS_VOLUME_INFORMATION FsVolumeInfo,
                           PULONG BufferLength)
{
    DPRINT("NtfsGetFsVolumeInformation() called\n");
    DPRINT("FsVolumeInfo = %p\n", FsVolumeInfo);
    DPRINT("BufferLength %lu\n", *BufferLength);

    DPRINT("Vpb %p\n", DeviceObject->Vpb);

    DPRINT("Required length %lu\n",
           sizeof(FILE_FS_VOLUME_INFORMATION) + DeviceObject->Vpb->VolumeLabelLength);
    DPRINT("LabelLength %hu\n",
           DeviceObject->Vpb->VolumeLabelLength);
    DPRINT("Label %.*S\n",
           DeviceObject->Vpb->VolumeLabelLength / sizeof(WCHAR),
           DeviceObject->Vpb->VolumeLabel);

    if (*BufferLength < sizeof(FILE_FS_VOLUME_INFORMATION))
        return STATUS_INFO_LENGTH_MISMATCH;

    if (*BufferLength < (sizeof(FILE_FS_VOLUME_INFORMATION) + DeviceObject->Vpb->VolumeLabelLength))
        return STATUS_BUFFER_OVERFLOW;

    /* valid entries */
    FsVolumeInfo->VolumeSerialNumber = DeviceObject->Vpb->SerialNumber;
    FsVolumeInfo->VolumeLabelLength = DeviceObject->Vpb->VolumeLabelLength;
    memcpy(FsVolumeInfo->VolumeLabel,
           DeviceObject->Vpb->VolumeLabel,
           DeviceObject->Vpb->VolumeLabelLength);

    /* dummy entries */
    FsVolumeInfo->VolumeCreationTime.QuadPart = 0;
    FsVolumeInfo->SupportsObjects = FALSE;

    *BufferLength -= (sizeof(FILE_FS_VOLUME_INFORMATION) + DeviceObject->Vpb->VolumeLabelLength);

    DPRINT("BufferLength %lu\n", *BufferLength);
    DPRINT("NtfsGetFsVolumeInformation() done\n");

    return STATUS_SUCCESS;
}


static
NTSTATUS
NtfsGetFsAttributeInformation(PDEVICE_EXTENSION DeviceExt,
                              PFILE_FS_ATTRIBUTE_INFORMATION FsAttributeInfo,
                              PULONG BufferLength)
{
    UNREFERENCED_PARAMETER(DeviceExt);

    DPRINT("NtfsGetFsAttributeInformation()\n");
    DPRINT("FsAttributeInfo = %p\n", FsAttributeInfo);
    DPRINT("BufferLength %lu\n", *BufferLength);
    DPRINT("Required length %lu\n", (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 8));

    if (*BufferLength < sizeof (FILE_FS_ATTRIBUTE_INFORMATION))
        return STATUS_INFO_LENGTH_MISMATCH;

    if (*BufferLength < (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 8))
        return STATUS_BUFFER_OVERFLOW;

    FsAttributeInfo->FileSystemAttributes =
        FILE_CASE_PRESERVED_NAMES | FILE_UNICODE_ON_DISK | FILE_READ_ONLY_VOLUME;
    FsAttributeInfo->MaximumComponentNameLength = 255;
    FsAttributeInfo->FileSystemNameLength = 8;

    memcpy(FsAttributeInfo->FileSystemName, L"NTFS", 8);

    DPRINT("Finished NtfsGetFsAttributeInformation()\n");

    *BufferLength -= (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 8);
    DPRINT("BufferLength %lu\n", *BufferLength);

    return STATUS_SUCCESS;
}


static
NTSTATUS
NtfsGetFsSizeInformation(PDEVICE_OBJECT DeviceObject,
                         PFILE_FS_SIZE_INFORMATION FsSizeInfo,
                         PULONG BufferLength)
{
    PDEVICE_EXTENSION DeviceExt;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("NtfsGetFsSizeInformation()\n");
    DPRINT("FsSizeInfo = %p\n", FsSizeInfo);

    if (*BufferLength < sizeof(FILE_FS_SIZE_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    DeviceExt = DeviceObject->DeviceExtension;

    FsSizeInfo->AvailableAllocationUnits.QuadPart = NtfsGetFreeClusters(DeviceExt);
    FsSizeInfo->TotalAllocationUnits.QuadPart = DeviceExt->NtfsInfo.ClusterCount;
    FsSizeInfo->SectorsPerAllocationUnit = DeviceExt->NtfsInfo.SectorsPerCluster;
    FsSizeInfo->BytesPerSector = DeviceExt->NtfsInfo.BytesPerSector;

    DPRINT("Finished NtfsGetFsSizeInformation()\n");
    if (NT_SUCCESS(Status))
        *BufferLength -= sizeof(FILE_FS_SIZE_INFORMATION);

    return Status;
}


static
NTSTATUS
NtfsGetFsDeviceInformation(PDEVICE_OBJECT DeviceObject,
                           PFILE_FS_DEVICE_INFORMATION FsDeviceInfo,
                           PULONG BufferLength)
{
    DPRINT("NtfsGetFsDeviceInformation()\n");
    DPRINT("FsDeviceInfo = %p\n", FsDeviceInfo);
    DPRINT("BufferLength %lu\n", *BufferLength);
    DPRINT("Required length %lu\n", sizeof(FILE_FS_DEVICE_INFORMATION));

    if (*BufferLength < sizeof(FILE_FS_DEVICE_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    FsDeviceInfo->DeviceType = FILE_DEVICE_DISK;
    FsDeviceInfo->Characteristics = DeviceObject->Characteristics;

    DPRINT("NtfsGetFsDeviceInformation() finished.\n");

    *BufferLength -= sizeof(FILE_FS_DEVICE_INFORMATION);
    DPRINT("BufferLength %lu\n", *BufferLength);

    return STATUS_SUCCESS;
}


NTSTATUS
NtfsQueryVolumeInformation(PNTFS_IRP_CONTEXT IrpContext)
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    FS_INFORMATION_CLASS FsInformationClass;
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID SystemBuffer;
    ULONG BufferLength;
    PDEVICE_EXTENSION DeviceExt;

    DPRINT("NtfsQueryVolumeInformation() called\n");

    ASSERT(IrpContext);

    Irp = IrpContext->Irp;
    DeviceObject = IrpContext->DeviceObject;
    DeviceExt = DeviceObject->DeviceExtension;
    Stack = IrpContext->Stack;

    if (!ExAcquireResourceSharedLite(&DeviceExt->DirResource,
                                     BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        return NtfsMarkIrpContextForQueue(IrpContext);
    }

    FsInformationClass = Stack->Parameters.QueryVolume.FsInformationClass;
    BufferLength = Stack->Parameters.QueryVolume.Length;
    SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
    RtlZeroMemory(SystemBuffer, BufferLength);

    DPRINT("FsInformationClass %d\n", FsInformationClass);
    DPRINT("SystemBuffer %p\n", SystemBuffer);

    switch (FsInformationClass)
    {
        case FileFsVolumeInformation:
            Status = NtfsGetFsVolumeInformation(DeviceObject,
                                                SystemBuffer,
                                                &BufferLength);
            break;

        case FileFsAttributeInformation:
            Status = NtfsGetFsAttributeInformation(DeviceObject->DeviceExtension,
                                                   SystemBuffer,
                                                   &BufferLength);
            break;

        case FileFsSizeInformation:
            Status = NtfsGetFsSizeInformation(DeviceObject,
                                              SystemBuffer,
                                              &BufferLength);
            break;

        case FileFsDeviceInformation:
            Status = NtfsGetFsDeviceInformation(DeviceObject,
                                                SystemBuffer,
                                                &BufferLength);
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
    }

    ExReleaseResourceLite(&DeviceExt->DirResource);

    if (NT_SUCCESS(Status))
        Irp->IoStatus.Information =
            Stack->Parameters.QueryVolume.Length - BufferLength;
    else
        Irp->IoStatus.Information = 0;

    return Status;
}


NTSTATUS
NtfsSetVolumeInformation(PNTFS_IRP_CONTEXT IrpContext)
{
    PIRP Irp;

    DPRINT("NtfsSetVolumeInformation() called\n");

    ASSERT(IrpContext);

    Irp = IrpContext->Irp;
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    return STATUS_NOT_SUPPORTED;
}

/* EOF */
