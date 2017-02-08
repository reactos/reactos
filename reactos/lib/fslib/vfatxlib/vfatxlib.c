/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFATx filesystem library
 * FILE:        vfatxlib.c
 * PURPOSE:     Main API
 * PROGRAMMERS: 
 * REVISIONS:
 *   CSH 05/04-2003 Created
 */

#include "vfatxlib.h"

#include <ndk/obfuncs.h>

#define NDEBUG
#include <debug.h>

NTSTATUS NTAPI
VfatxFormat(IN PUNICODE_STRING DriveRoot,
            IN FMIFS_MEDIA_FLAG MediaFlag,
            IN PUNICODE_STRING Label,
            IN BOOLEAN QuickFormat,
            IN ULONG ClusterSize,
            IN PFMIFSCALLBACK Callback)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    DISK_GEOMETRY DiskGeometry;
    IO_STATUS_BLOCK Iosb;
    HANDLE FileHandle;
    PARTITION_INFORMATION PartitionInfo;
    FORMAT_CONTEXT Context;
    NTSTATUS Status;

    DPRINT("VfatxFormat(DriveRoot '%wZ')\n", DriveRoot);

    Context.TotalSectorCount = 0;
    Context.CurrentSectorCount = 0;
    Context.Callback = Callback;
    Context.Success = FALSE;
    Context.Percent = 0;

    InitializeObjectAttributes(&ObjectAttributes,
                               DriveRoot,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                        &ObjectAttributes,
                        &Iosb,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_ALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtOpenFile() failed with status 0x%.08x\n", Status);
        return Status;
    }

    Status = NtDeviceIoControlFile(FileHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                   NULL,
                                   0,
                                   &DiskGeometry,
                                   sizeof(DISK_GEOMETRY));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IOCTL_DISK_GET_DRIVE_GEOMETRY failed with status 0x%.08x\n", Status);
        NtClose(FileHandle);
        return Status;
    }

    if (DiskGeometry.MediaType == FixedMedia)
    {
        DPRINT("Cylinders %I64d\n", DiskGeometry.Cylinders.QuadPart);
        DPRINT("TracksPerCylinder %ld\n", DiskGeometry.TracksPerCylinder);
        DPRINT("SectorsPerTrack %ld\n", DiskGeometry.SectorsPerTrack);
        DPRINT("BytesPerSector %ld\n", DiskGeometry.BytesPerSector);
        DPRINT("DiskSize %I64d\n",
               DiskGeometry.Cylinders.QuadPart *
               (ULONGLONG)DiskGeometry.TracksPerCylinder *
               (ULONGLONG)DiskGeometry.SectorsPerTrack *
               (ULONGLONG)DiskGeometry.BytesPerSector);

        Status = NtDeviceIoControlFile(FileHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &Iosb,
                                       IOCTL_DISK_GET_PARTITION_INFO,
                                       NULL,
                                       0,
                                       &PartitionInfo,
                                       sizeof(PARTITION_INFORMATION));
        if (!NT_SUCCESS(Status))
        {
            DPRINT("IOCTL_DISK_GET_PARTITION_INFO failed with status 0x%.08x\n", Status);
            NtClose(FileHandle);
            return Status;
        }
    }
    else
    {
        PartitionInfo.PartitionType = 0;
        PartitionInfo.StartingOffset.QuadPart = 0ULL;
        PartitionInfo.PartitionLength.QuadPart =
            DiskGeometry.Cylinders.QuadPart *
            (ULONGLONG)DiskGeometry.TracksPerCylinder *
            (ULONGLONG)DiskGeometry.SectorsPerTrack *
            (ULONGLONG)DiskGeometry.BytesPerSector;
        PartitionInfo.HiddenSectors = 0;
        PartitionInfo.PartitionNumber = 0;
        PartitionInfo.BootIndicator = FALSE;
        PartitionInfo.RewritePartition = FALSE;
        PartitionInfo.RecognizedPartition = FALSE;
    }

    DPRINT("PartitionType 0x%x\n", PartitionInfo.PartitionType);
    DPRINT("StartingOffset %I64d\n", PartitionInfo.StartingOffset.QuadPart);
    DPRINT("PartitionLength %I64d\n", PartitionInfo.PartitionLength.QuadPart);
    DPRINT("HiddenSectors %lu\n", PartitionInfo.HiddenSectors);
    DPRINT("PartitionNumber %d\n", PartitionInfo.PartitionNumber);
    DPRINT("BootIndicator 0x%x\n", PartitionInfo.BootIndicator);
    DPRINT("RewritePartition %d\n", PartitionInfo.RewritePartition);
    DPRINT("RecognizedPartition %d\n", PartitionInfo.RecognizedPartition);

    if (Callback != NULL)
    {
        Context.Percent = 0;
        Callback(PROGRESS, 0, (PVOID)&Context.Percent);
    }

    Status = FatxFormat(FileHandle,
                        &PartitionInfo,
                        &DiskGeometry,
                        QuickFormat,
                        &Context);
    NtClose(FileHandle);

    if (Callback != NULL)
    {
        Context.Success = (BOOLEAN)(NT_SUCCESS(Status));
        Callback(DONE, 0, (PVOID)&Context.Success);
    }

    DPRINT("VfatxFormat() done. Status 0x%.08x\n", Status);

    return Status;
}


VOID
VfatxUpdateProgress(IN PFORMAT_CONTEXT Context,
                    IN ULONG Increment)
{
    ULONG NewPercent;

    Context->CurrentSectorCount += (ULONGLONG)Increment;

    NewPercent = (Context->CurrentSectorCount * 100ULL) / Context->TotalSectorCount;

    if (NewPercent > Context->Percent)
    {
        Context->Percent = NewPercent;
        if (Context->Callback != NULL)
        {
            Context->Callback(PROGRESS, 0, &Context->Percent);
        }
    }
}

/* EOF */
