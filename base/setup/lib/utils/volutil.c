/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Volume utility functions
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"

#include "volutil.h"
#include "fsrec.h"
#include "devutils.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS
MountVolume(
    _Inout_ PVOLINFO Volume,
    _In_opt_ UCHAR MbrPartitionType)
{
    NTSTATUS Status;
    HANDLE VolumeHandle;

    /* If the volume is already mounted, just return success */
    if (*Volume->FileSystem)
        return STATUS_SUCCESS;

    /* Try to open the volume so as to mount it */
    VolumeHandle = NULL;
    Status = pOpenDevice(Volume->DeviceName, &VolumeHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("pOpenDevice() failed, Status 0x%08lx\n", Status);

        /* We failed, reset some data and bail out */
        Volume->DriveLetter = UNICODE_NULL;
        Volume->VolumeLabel[0] = UNICODE_NULL;
        Volume->FileSystem[0] = UNICODE_NULL;

        return Status;
    }
    ASSERT(VolumeHandle);

    /* We don't have a FS, try to guess one */
    Status = InferFileSystem(NULL, VolumeHandle,
                             Volume->FileSystem,
                             sizeof(Volume->FileSystem));
    if (!NT_SUCCESS(Status))
        DPRINT1("InferFileSystem() failed, Status 0x%08lx\n", Status);

    if (*Volume->FileSystem)
    {
        /*
         * Handle volume mounted with RawFS: it is
         * either unformatted or has an unknown format.
         */
        if (IsUnformatted(Volume)) // FileSystem is "RAW"
        {
            /*
             * True unformatted partitions on NT are created with their
             * partition type set to either one of the following values,
             * and are mounted with RawFS. This is done this way since we
             * are assured to have FAT support, which is the only FS that
             * uses these partition types. Therefore, having a partition
             * mounted with RawFS and with these partition types means that
             * the FAT FS was unable to mount it beforehand and thus the
             * partition is unformatted.
             * However, any partition mounted by RawFS that does NOT have
             * any of these partition types must be considered as having
             * an unknown format.
             */
            if (MbrPartitionType == PARTITION_FAT_12 ||
                MbrPartitionType == PARTITION_FAT_16 ||
                MbrPartitionType == PARTITION_HUGE   ||
                MbrPartitionType == PARTITION_XINT13 ||
                MbrPartitionType == PARTITION_FAT32  ||
                MbrPartitionType == PARTITION_FAT32_XINT13)
            {
                /* The volume is unformatted */
            }
            else
            {
                /* Close the volume before dismounting */
                NtClose(VolumeHandle);
                VolumeHandle = NULL;
                /*
                 * Dismount the volume since RawFS owns it, and reset its
                 * format (it is unknown, may or may not be actually formatted).
                 */
                DismountVolume(Volume, TRUE);
                Volume->FileSystem[0] = UNICODE_NULL;
            }
        }
        /* Else, the volume is formatted */
    }
    /* Else, the volume has an unknown format */

    /* Retrieve the volume label */
    if (VolumeHandle)
    {
        IO_STATUS_BLOCK IoStatusBlock;
        struct
        {
            FILE_FS_VOLUME_INFORMATION;
            WCHAR Data[255];
        } LabelInfo;

        Status = NtQueryVolumeInformationFile(VolumeHandle,
                                              &IoStatusBlock,
                                              &LabelInfo,
                                              sizeof(LabelInfo),
                                              FileFsVolumeInformation);
        if (NT_SUCCESS(Status))
        {
            /* Copy the (possibly truncated) volume label and NULL-terminate it */
            RtlStringCbCopyNW(Volume->VolumeLabel, sizeof(Volume->VolumeLabel),
                              LabelInfo.VolumeLabel, LabelInfo.VolumeLabelLength);
        }
        else
        {
            DPRINT1("NtQueryVolumeInformationFile() failed, Status 0x%08lx\n", Status);
        }
    }

    /* Close the volume */
    if (VolumeHandle)
        NtClose(VolumeHandle);

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Attempts to dismount the designated volume.
 *
 * @param[in,out]   Volume
 * The volume to dismount.
 *
 * @param[in]   Force
 * Whether the volume is forcibly dismounted, even
 * if there are open handles to files on this volume.
 *
 * @return  An NTSTATUS code indicating success or failure.
 **/
NTSTATUS
DismountVolume(
    _Inout_ PVOLINFO Volume,
    _In_ BOOLEAN Force)
{
    NTSTATUS Status, LockStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE VolumeHandle;

    /* If the volume is not mounted, just return success */
    if (!*Volume->FileSystem)
        return STATUS_SUCCESS;

    /* Open the volume */
    Status = pOpenDeviceEx(Volume->DeviceName, &VolumeHandle,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Cannot open volume %S for dismounting! (Status 0x%lx)\n",
                Volume->DeviceName, Status);
        return Status;
    }

    /* Lock the volume (succeeds only if there are no open handles to files) */
    LockStatus = NtFsControlFile(VolumeHandle,
                                 NULL, NULL, NULL,
                                 &IoStatusBlock,
                                 FSCTL_LOCK_VOLUME,
                                 NULL, 0,
                                 NULL, 0);
    if (!NT_SUCCESS(LockStatus))
        DPRINT1("WARNING: Failed to lock volume (Status 0x%lx)\n", LockStatus);

    /* Dismount the volume (succeeds even when lock fails and there are open handles) */
    Status = STATUS_ACCESS_DENIED; // Suppose dismount failure.
    if (NT_SUCCESS(LockStatus) || Force)
    {
        Status = NtFsControlFile(VolumeHandle,
                                 NULL, NULL, NULL,
                                 &IoStatusBlock,
                                 FSCTL_DISMOUNT_VOLUME,
                                 NULL, 0,
                                 NULL, 0);
        if (!NT_SUCCESS(Status))
            DPRINT1("Failed to unmount volume (Status 0x%lx)\n", Status);
    }

    /* Unlock the volume */
    if (NT_SUCCESS(LockStatus))
    {
        LockStatus = NtFsControlFile(VolumeHandle,
                                     NULL, NULL, NULL,
                                     &IoStatusBlock,
                                     FSCTL_UNLOCK_VOLUME,
                                     NULL, 0,
                                     NULL, 0);
        if (!NT_SUCCESS(LockStatus))
            DPRINT1("Failed to unlock volume (Status 0x%lx)\n", LockStatus);
    }

    /* Close the volume */
    NtClose(VolumeHandle);

    /* Reset some data only if dismount succeeded */
    if (NT_SUCCESS(Status))
    {
        Volume->DriveLetter = UNICODE_NULL;
        Volume->VolumeLabel[0] = UNICODE_NULL;
        Volume->FileSystem[0] = UNICODE_NULL;
    }

    return Status;
}

/* EOF */
