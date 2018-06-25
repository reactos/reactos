/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        lib\fslib\vfatlib\vfatlib.c
 * PURPOSE:     Main API
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Aleksey Bragin (aleksey@reactos.org)
 *              Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */
/* fsck.fat.c - User interface

   Copyright (C) 1993 Werner Almesberger <werner.almesberger@lrc.di.epfl.ch>
   Copyright (C) 1998 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
   Copyright (C) 2008-2014 Daniel Baumann <mail@daniel-baumann.ch>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.

   The complete text of the GNU General Public License
   can be found in /usr/share/common-licenses/GPL-3 file.
*/

/* INCLUDES *******************************************************************/

#include "vfatlib.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS & FUNCTIONS ********************************************************/

PFMIFSCALLBACK ChkdskCallback = NULL;
ULONG FsCheckFlags;
PVOID FsCheckMemQueue;
ULONG FsCheckTotalFiles;

NTSTATUS
NTAPI
VfatFormat(IN PUNICODE_STRING DriveRoot,
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
    NTSTATUS Status, LockStatus;

    DPRINT("VfatFormat(DriveRoot '%wZ')\n", DriveRoot);

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
                        FILE_GENERIC_READ | FILE_GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &Iosb,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_ALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed with status 0x%.08x\n", Status);
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
    
    /* If it already has a FAT FS, we'll use that type.
     * If it doesn't, we will determine the FAT type based on size and offset */
    if (PartitionInfo.PartitionType != PARTITION_FAT_12 &&
        PartitionInfo.PartitionType != PARTITION_FAT_16 &&
        PartitionInfo.PartitionType != PARTITION_HUGE &&
        PartitionInfo.PartitionType != PARTITION_XINT13 &&
        PartitionInfo.PartitionType != PARTITION_FAT32 &&
        PartitionInfo.PartitionType != PARTITION_FAT32_XINT13)
    {
        /* Determine the correct type based upon size and offset (copied from usetup) */
        if (PartitionInfo.PartitionLength.QuadPart < (4200LL * 1024LL))
        {
            /* FAT12 CHS partition (disk is smaller than 4.1MB) */
            PartitionInfo.PartitionType = PARTITION_FAT_12;
        }
        else if (PartitionInfo.StartingOffset.QuadPart < (1024LL * 255LL * 63LL * 512LL))
        {
            /* Partition starts below the 8.4GB boundary ==> CHS partition */
            
            if (PartitionInfo.PartitionLength.QuadPart < (32LL * 1024LL * 1024LL))
            {
                /* FAT16 CHS partition (partition size < 32MB) */
                PartitionInfo.PartitionType = PARTITION_FAT_16;
            }
            else if (PartitionInfo.PartitionLength.QuadPart < (512LL * 1024LL * 1024LL))
            {
                /* FAT16 CHS partition (partition size < 512MB) */
                PartitionInfo.PartitionType = PARTITION_HUGE;
            }
            else
            {
                /* FAT32 CHS partition (partition size >= 512MB) */
                PartitionInfo.PartitionType = PARTITION_FAT32;
            }
        }
        else
        {
            /* Partition starts above the 8.4GB boundary ==> LBA partition */
            
            if (PartitionInfo.PartitionLength.QuadPart < (512LL * 1024LL * 1024LL))
            {
                /* FAT16 LBA partition (partition size < 512MB) */
                PartitionInfo.PartitionType = PARTITION_XINT13;
            }
            else
            {
                /* FAT32 LBA partition (partition size >= 512MB) */
                PartitionInfo.PartitionType = PARTITION_FAT32_XINT13;
            }
        }
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
        Callback (PROGRESS, 0, (PVOID)&Context.Percent);
    }

    LockStatus = NtFsControlFile(FileHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &Iosb,
                                 FSCTL_LOCK_VOLUME,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
    if (!NT_SUCCESS(LockStatus))
    {
        DPRINT1("WARNING: Failed to lock volume for formatting! Format may fail! (Status: 0x%x)\n", LockStatus);
    }

    if (PartitionInfo.PartitionType == PARTITION_FAT_12)
    {
        /* FAT12 */
        Status = Fat12Format(FileHandle,
                             &PartitionInfo,
                             &DiskGeometry,
                             Label,
                             QuickFormat,
                             ClusterSize,
                             &Context);
    }
    else if (PartitionInfo.PartitionType == PARTITION_FAT_16 ||
             PartitionInfo.PartitionType == PARTITION_HUGE ||
             PartitionInfo.PartitionType == PARTITION_XINT13)
    {
        /* FAT16 */
        Status = Fat16Format(FileHandle,
                             &PartitionInfo,
                             &DiskGeometry,
                             Label,
                             QuickFormat,
                             ClusterSize,
                             &Context);
    }
    else if (PartitionInfo.PartitionType == PARTITION_FAT32 ||
             PartitionInfo.PartitionType == PARTITION_FAT32_XINT13)
    {
        /* FAT32 */
        Status = Fat32Format(FileHandle,
                             &PartitionInfo,
                             &DiskGeometry,
                             Label,
                             QuickFormat,
                             ClusterSize,
                             &Context);
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
    }

    /* Attempt to dismount formatted volume */
    LockStatus = NtFsControlFile(FileHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &Iosb,
                                 FSCTL_DISMOUNT_VOLUME,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
    if (!NT_SUCCESS(LockStatus))
    {
        DPRINT1("Failed to umount volume (Status: 0x%x)\n", LockStatus);
    }

    LockStatus = NtFsControlFile(FileHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &Iosb,
                                 FSCTL_UNLOCK_VOLUME,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
    if (!NT_SUCCESS(LockStatus))
    {
        DPRINT1("Failed to unlock volume (Status: 0x%x)\n", LockStatus);
    }

    NtClose(FileHandle);

    if (Callback != NULL)
    {
        Context.Success = (BOOLEAN)(NT_SUCCESS(Status));
        Callback(DONE, 0, (PVOID)&Context.Success);
    }

    DPRINT("VfatFormat() done. Status 0x%.08x\n", Status);

    return Status;
}


VOID
UpdateProgress(PFORMAT_CONTEXT Context,
               ULONG Increment)
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


VOID
VfatPrintV(PCHAR Format, va_list args)
{
    TEXTOUTPUT TextOut;
    CHAR TextBuf[512];

    _vsnprintf(TextBuf, sizeof(TextBuf), Format, args);

    /* Prepare parameters */
    TextOut.Lines = 1;
    TextOut.Output = TextBuf;

    DPRINT1("VfatPrint -- %s", TextBuf);

    /* Do the callback */
    if (ChkdskCallback)
        ChkdskCallback(OUTPUT, 0, &TextOut);
}


VOID
VfatPrint(PCHAR Format, ...)
{
    va_list args;

    va_start(args, Format);
    VfatPrintV(Format, args);
    va_end(args);
}


NTSTATUS
NTAPI
VfatChkdsk(IN PUNICODE_STRING DriveRoot,
           IN BOOLEAN FixErrors,
           IN BOOLEAN Verbose,
           IN BOOLEAN CheckOnlyIfDirty,
           IN BOOLEAN ScanDrive,
           IN PFMIFSCALLBACK Callback)
{
    BOOLEAN verify;
    BOOLEAN salvage_files;
    ULONG free_clusters;
    DOS_FS fs;
    NTSTATUS Status;

    RtlZeroMemory(&fs, sizeof(fs));

    /* Store callback pointer */
    ChkdskCallback = Callback;
    FsCheckMemQueue = NULL;

    /* Set parameters */
    FsCheckFlags = 0;
    if (Verbose)
        FsCheckFlags |= FSCHECK_VERBOSE;
    if (FixErrors)
        FsCheckFlags |= FSCHECK_READ_WRITE;

    FsCheckTotalFiles = 0;

    verify = TRUE;
    salvage_files = TRUE;

    /* Open filesystem and lock it */
    Status = fs_open(DriveRoot, FsCheckFlags & FSCHECK_READ_WRITE);
    if (Status == STATUS_ACCESS_DENIED)
    {
        /* We failed to lock, ask the caller whether we should continue */
        if (Callback(VOLUMEINUSE, 0, NULL))
        {
            Status = STATUS_SUCCESS;
        }
    }
    if (!NT_SUCCESS(Status))
    {
        fs_close(FALSE);
        return STATUS_DISK_CORRUPT_ERROR;
    }

    if (CheckOnlyIfDirty && !fs_isdirty())
    {
        /* Unlock volume if required */
        if (FsCheckFlags & FSCHECK_READ_WRITE)
            fs_lock(FALSE);

        /* No need to check FS */
        return (fs_close(FALSE) == 0 ? STATUS_SUCCESS : STATUS_DISK_CORRUPT_ERROR);
    }
    else if (CheckOnlyIfDirty && fs_isdirty())
    {
        if (!(FsCheckFlags & FSCHECK_READ_WRITE) && !(FsCheckFlags & FSCHECK_INTERACTIVE))
        {
            fs_close(FALSE);
            return STATUS_DISK_CORRUPT_ERROR;
        }
    }

    read_boot(&fs);

    if (verify)
        VfatPrint("Starting check/repair pass.\n");

    while (read_fat(&fs), scan_root(&fs))
        qfree(&FsCheckMemQueue);

    if (ScanDrive)
        fix_bad(&fs);

    if (salvage_files)
        reclaim_file(&fs);
    else
        reclaim_free(&fs);

    free_clusters = update_free(&fs);
    file_unused();
    qfree(&FsCheckMemQueue);
    if (verify)
    {
        FsCheckTotalFiles = 0;
        VfatPrint("Starting verification pass.\n");
        read_fat(&fs);
        scan_root(&fs);
        reclaim_free(&fs);
        qfree(&FsCheckMemQueue);
    }

    if (fs_changed())
    {
        if (FsCheckFlags & FSCHECK_READ_WRITE)
        {
            if (FsCheckFlags & FSCHECK_INTERACTIVE)
            {
                FixErrors = get_key("yn", "Perform changes ? (y/n)") == 'y';
                if (FixErrors)
                    FsCheckFlags |= FSCHECK_READ_WRITE;
                else
                    FsCheckFlags &= ~FSCHECK_READ_WRITE;
            }
            else
                VfatPrint("Performing changes.\n");
        }
        else
        {
            VfatPrint("Leaving filesystem unchanged.\n");
        }
    }

    VfatPrint("%wZ: %u files, %lu/%lu clusters\n", DriveRoot,
        FsCheckTotalFiles, fs.data_clusters - free_clusters, fs.data_clusters);

    if (FsCheckFlags & FSCHECK_READ_WRITE)
    {
        /* Dismount the volume */
        fs_dismount();

        /* Unlock the volume */
        fs_lock(FALSE);
    }

    // https://technet.microsoft.com/en-us/library/cc730714.aspx
    // https://support.microsoft.com/en-us/kb/265533

    /* Close the volume */
    return (fs_close(FsCheckFlags & FSCHECK_READ_WRITE) == 0 ? STATUS_SUCCESS : STATUS_DISK_CORRUPT_ERROR);
}

/* EOF */
