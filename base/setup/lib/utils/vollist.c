/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Volume utilities and list functions
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"
#include <mountdev.h>

#include "vollist.h"
#include "fsrec.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

/**
 * @brief
 * Requests the MountMgr to retrieve the drive letter,
 * if any, associated to the given volume.
 *
 * @param[in]   VolumeName
 * Device name of the volume.
 *
 * @param[out]  DriveLetter
 * Pointer to a WCHAR buffer receiving the corresponding drive letter,
 * or UNICODE_NULL if none has been assigned.
 *
 * @return  A status code.
 **/
NTSTATUS
GetVolumeDriveLetter(
    _In_ PCUNICODE_STRING VolumeName,
    _Out_ PWCHAR DriveLetter)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    UNICODE_STRING MountMgrDevice;
    ULONG Length;
    MOUNTMGR_MOUNT_POINTS MountPoints;
    PMOUNTMGR_MOUNT_POINTS MountPointsPtr;
    /*
     * This variable is used to store the device name
     * for the input buffer to IOCTL_MOUNTMGR_QUERY_POINTS.
     * It's based on MOUNTMGR_MOUNT_POINT (mountmgr.h).
     * Doing it this way prevents memory allocation.
     * The device name won't be longer.
     */
    struct
    {
        MOUNTMGR_MOUNT_POINT;
        WCHAR DeviceName[256];
    } DeviceName;

    /* Default to no letter */
    *DriveLetter = UNICODE_NULL;

    /* First, retrieve the corresponding device name */
    DeviceName.SymbolicLinkNameOffset = DeviceName.UniqueIdOffset = 0;
    DeviceName.SymbolicLinkNameLength = DeviceName.UniqueIdLength = 0;
    DeviceName.DeviceNameOffset = (ULONG_PTR)&DeviceName.DeviceName - (ULONG_PTR)&DeviceName;
    DeviceName.DeviceNameLength = VolumeName->Length;
    RtlCopyMemory(&DeviceName.DeviceName, VolumeName->Buffer, VolumeName->Length);

    /* Now, query the MountMgr for the DOS path */
    RtlInitUnicodeString(&MountMgrDevice, MOUNTMGR_DEVICE_NAME);
    InitializeObjectAttributes(&ObjectAttributes,
                               &MountMgrDevice,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&Handle,
                        FILE_READ_ATTRIBUTES | SYNCHRONIZE, // FILE_READ_DATA
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed, Status 0x%08lx\n", Status);
        return Status;
    }

    MountPointsPtr = NULL;
    Status = NtDeviceIoControlFile(Handle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_MOUNTMGR_QUERY_POINTS,
                                   &DeviceName, sizeof(DeviceName),
                                   &MountPoints, sizeof(MountPoints));

    /* Only tolerated failure here is buffer too small, which is expected */
    if (!NT_SUCCESS(Status) && (Status != STATUS_BUFFER_OVERFLOW))
    {
        goto Quit;
    }

    /* Compute the needed size to store the mount points list */
    Length = MountPoints.Size;

    /* Reallocate the memory, even in case of success, because
     * that's the buffer that will be returned to the caller */
    MountPointsPtr = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, Length);
    if (!MountPointsPtr)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quit;
    }

    /* Re-query the mount points list with proper size */
    Status = NtDeviceIoControlFile(Handle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_MOUNTMGR_QUERY_POINTS,
                                   &DeviceName, sizeof(DeviceName),
                                   MountPointsPtr, Length);
    if (!NT_SUCCESS(Status))
        goto Quit;

    /* Find the drive letter amongst the mount points */
    for (MountPointsPtr->Size = 0; // Reuse the Size field to store the current index.
         MountPointsPtr->Size < MountPointsPtr->NumberOfMountPoints;
         MountPointsPtr->Size++)
    {
        UNICODE_STRING SymLink;

        SymLink.Length = SymLink.MaximumLength = MountPointsPtr->MountPoints[MountPointsPtr->Size].SymbolicLinkNameLength;
        SymLink.Buffer = (PWCHAR)((ULONG_PTR)MountPointsPtr + MountPointsPtr->MountPoints[MountPointsPtr->Size].SymbolicLinkNameOffset);

        if (MOUNTMGR_IS_DRIVE_LETTER(&SymLink))
        {
            /* Return the drive letter, ensuring it's uppercased */
            *DriveLetter = towupper(SymLink.Buffer[12]);
            break;
        }
    }

    /* We are done, return success */
    Status = STATUS_SUCCESS;

Quit:
    if (MountPointsPtr)
        RtlFreeHeap(ProcessHeap, 0, MountPointsPtr);
    NtClose(Handle);
    return Status;
}

/**
 * @brief
 * Requests the MountMgr to either assign the next available drive letter
 * to the given volume, if none already exists, or to retrieve its existing
 * associated drive letter.
 *
 * @param[in]   VolumeName
 * Device name of the volume.
 *
 * @param[out]  DriveLetter
 * Pointer to a WCHAR buffer receiving the corresponding drive letter,
 * or UNICODE_NULL if none has been assigned.
 *
 * @return  A status code.
 **/
NTSTATUS
GetOrAssignNextVolumeDriveLetter(
    _In_ PCUNICODE_STRING VolumeName,
    _Out_ PWCHAR DriveLetter)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    UNICODE_STRING MountMgrDevice;
    MOUNTMGR_DRIVE_LETTER_INFORMATION LetterInfo;
    /*
     * This variable is used to store the device name
     * for the input buffer to IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER.
     * It's based on MOUNTMGR_DRIVE_LETTER_TARGET (mountmgr.h).
     * Doing it this way prevents memory allocation.
     * The device name won't be longer.
     */
    struct
    {
        USHORT DeviceNameLength;
        WCHAR DeviceName[256];
    } DeviceName;

    /* Default to no letter */
    *DriveLetter = UNICODE_NULL;

    /* First, retrieve the corresponding device name */
    DeviceName.DeviceNameLength = VolumeName->Length;
    RtlCopyMemory(&DeviceName.DeviceName, VolumeName->Buffer, VolumeName->Length);

    /* Now, query the MountMgr for the DOS path */
    RtlInitUnicodeString(&MountMgrDevice, MOUNTMGR_DEVICE_NAME);
    InitializeObjectAttributes(&ObjectAttributes,
                               &MountMgrDevice,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&Handle,
                        FILE_READ_ATTRIBUTES | SYNCHRONIZE, // FILE_READ_DATA
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed, Status 0x%08lx\n", Status);
        return Status;
    }

    Status = NtDeviceIoControlFile(Handle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER,
                                   &DeviceName, sizeof(DeviceName),
                                   &LetterInfo, sizeof(LetterInfo));
    if (!NT_SUCCESS(Status))
        goto Quit;

    DPRINT1("DriveLetterWasAssigned = %s, CurrentDriveLetter = %c\n",
            LetterInfo.DriveLetterWasAssigned ? "TRUE" : "FALSE",
            LetterInfo.CurrentDriveLetter ? LetterInfo.CurrentDriveLetter : '-');

    /* Return the drive letter the MountMgr potentially assigned,
     * ensuring it's uppercased */
    *DriveLetter = towupper(LetterInfo.CurrentDriveLetter);

Quit:
    NtClose(Handle);
    return Status;
}

/**
 * @brief
 * Attempts to mount the designated volume, and retrieve and cache
 * some of its properties (file system, volume label, ...).
 **/
NTSTATUS
MountVolume(
    _Inout_ PVOLINFO Volume,
    _In_opt_ UCHAR MbrPartitionType)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE VolumeHandle;

    /* Try to open the volume so as to mount it */
    RtlInitUnicodeString(&Name, Volume->DeviceName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    VolumeHandle = NULL;
    Status = NtOpenFile(&VolumeHandle,
                        FILE_READ_DATA | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed, Status 0x%08lx\n", Status);
    }

    if (VolumeHandle)
    {
        ASSERT(NT_SUCCESS(Status));

        /* We don't have a FS, try to guess one */
        Status = InferFileSystem(NULL, VolumeHandle,
                                 Volume->FileSystem,
                                 sizeof(Volume->FileSystem));
        if (!NT_SUCCESS(Status))
            DPRINT1("InferFileSystem() failed, Status 0x%08lx\n", Status);
    }
    if (*Volume->FileSystem) // (!IsUnknown(Volume))
    {
        ASSERT(VolumeHandle);

        /*
         * Handle volume mounted with RawFS: it is
         * either unformatted or has an unknown format.
         */
        if (IsUnformatted(Volume)) // (_wcsicmp(Volume->FileSystem, L"RAW") == 0)
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
                DismountVolume(Volume);
                *Volume->FileSystem = UNICODE_NULL;
            }
        }
        /* Else, the volume is formatted */
    }
    /* Else, the volume has an unknown format */

    /* Retrieve the volume label */
    if (VolumeHandle)
    {
        UCHAR LabelBuffer[sizeof(FILE_FS_VOLUME_INFORMATION) + 256 * sizeof(WCHAR)];
        PFILE_FS_VOLUME_INFORMATION LabelInfo = (PFILE_FS_VOLUME_INFORMATION)LabelBuffer;

        Status = NtQueryVolumeInformationFile(VolumeHandle,
                                              &IoStatusBlock,
                                              &LabelBuffer,
                                              sizeof(LabelBuffer),
                                              FileFsVolumeInformation);
        if (NT_SUCCESS(Status))
        {
            /* Copy the (possibly truncated) volume label and NULL-terminate it */
            RtlStringCbCopyNW(Volume->VolumeLabel, sizeof(Volume->VolumeLabel),
                              LabelInfo->VolumeLabel, LabelInfo->VolumeLabelLength);
        }
        else
        {
            DPRINT1("NtQueryVolumeInformationFile() failed, Status 0x%08lx\n", Status);
        }
    }

    /* Get the volume drive letter */
    __debugbreak();
    Status = GetVolumeDriveLetter(&Name, &Volume->DriveLetter);
    UNREFERENCED_PARAMETER(Status);

    /* Close the volume */
    if (VolumeHandle)
        NtClose(VolumeHandle);

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Attempts to dismount the designated volume.
 **/
NTSTATUS
DismountVolume(
    _Inout_ PVOLINFO Volume)
{
    NTSTATUS Status;
    NTSTATUS LockStatus;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE VolumeHandle;

    /* Check whether the volume was mounted by the system.
     * If the volume is not mounted, just return success. */
    if (!*Volume->FileSystem)
        return STATUS_SUCCESS;

    /* Open the volume */
    RtlInitUnicodeString(&Name, Volume->DeviceName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&VolumeHandle,
                        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Cannot open volume %wZ for dismounting! (Status 0x%lx)\n",
                &Name, Status);
        return Status;
    }

    /* Lock the volume */
    LockStatus = NtFsControlFile(VolumeHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_LOCK_VOLUME,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
    if (!NT_SUCCESS(LockStatus))
    {
        DPRINT1("WARNING: Failed to lock volume! Operations may fail! (Status 0x%lx)\n", LockStatus);
    }

    /* Dismount the volume */
    Status = NtFsControlFile(VolumeHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             FSCTL_DISMOUNT_VOLUME,
                             NULL,
                             0,
                             NULL,
                             0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to unmount volume (Status 0x%lx)\n", Status);
    }

    /* Unlock the volume */
    LockStatus = NtFsControlFile(VolumeHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_UNLOCK_VOLUME,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
    if (!NT_SUCCESS(LockStatus))
    {
        DPRINT1("Failed to unlock volume (Status 0x%lx)\n", LockStatus);
    }

    /* Close the volume */
    NtClose(VolumeHandle);

    /* Zero out some data only if dismount succeeded */
    if (NT_SUCCESS(Status))
    {
        // TODO: Should we notify MountMgr to delete the drive letter?
        Volume->DriveLetter = UNICODE_NULL;

        // *Volume->VolumeLabel = UNICODE_NULL;
        RtlZeroMemory(Volume->VolumeLabel, sizeof(Volume->VolumeLabel));
        // *Volume->FileSystem = UNICODE_NULL;
        RtlZeroMemory(Volume->FileSystem, sizeof(Volume->FileSystem));
    }

    return Status;
}

/* EOF */
