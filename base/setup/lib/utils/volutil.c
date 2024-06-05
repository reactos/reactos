/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Volume utility functions
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"
#include <mountdev.h>

#include "volutil.h"
#include "fsrec.h"
#include "devutils.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

/**
 * @brief
 * Retrieves a handle to the MountMgr controlling device.
 * The handle should be closed with NtClose() once it is no longer in use.
 **/
NTSTATUS
GetMountMgrHandle(
    _Out_ PHANDLE MountMgrHandle,
    _In_ ACCESS_MASK DesiredAccess)
{
    NTSTATUS Status;
    UNICODE_STRING MountMgrDevice;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    *MountMgrHandle = NULL;

    RtlInitUnicodeString(&MountMgrDevice, MOUNTMGR_DEVICE_NAME);
    InitializeObjectAttributes(&ObjectAttributes,
                               &MountMgrDevice,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(MountMgrHandle,
                        DesiredAccess | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile(%wZ) failed, Status 0x%08lx\n",
                &MountMgrDevice, Status);
    }
    return Status;
}

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
/* Differs from MOUNTMGR_IS_DRIVE_LETTER(): no '\DosDevices\' accounted for */
#define IS_DRIVE_LETTER(s) \
  ((s)->Length == 2*sizeof(WCHAR) && (s)->Buffer[0] >= 'A' && \
   (s)->Buffer[0] <= 'Z' && (s)->Buffer[1] == ':')

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE MountMgrHandle;
    ULONG Length;
    MOUNTMGR_VOLUME_PATHS VolumePath;
    PMOUNTMGR_VOLUME_PATHS VolumePathPtr;
    UNICODE_STRING DosPath;
    ULONG DeviceNameLength;
    /*
     * This variable is used to store the device name.
     * for the input buffer to IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH.
     * It's based on MOUNTMGR_TARGET_NAME (mountmgr.h).
     * Doing it this way prevents memory allocation.
     * The device name won't be longer.
     */
    struct
    {
        USHORT NameLength;
        WCHAR DeviceName[256];
    } DeviceName;

    /* Default to no letter */
    *DriveLetter = UNICODE_NULL;

    /* First, build the corresponding device name */
    DeviceName.NameLength = VolumeName->Length;
    RtlCopyMemory(&DeviceName.DeviceName, VolumeName->Buffer, VolumeName->Length);
    DeviceNameLength = FIELD_OFFSET(MOUNTMGR_TARGET_NAME, DeviceName) + DeviceName.NameLength;

    /* Now, query the MountMgr for the DOS path */
    Status = GetMountMgrHandle(&MountMgrHandle, FILE_READ_ATTRIBUTES);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MountMgr unavailable: Status 0x%08lx\n", Status);
        return Status;
    }

    VolumePathPtr = NULL;
    Status = NtDeviceIoControlFile(MountMgrHandle,
                                   NULL, NULL, NULL,
                                   &IoStatusBlock,
                                   IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH,
                                   &DeviceName, DeviceNameLength,
                                   &VolumePath, sizeof(VolumePath));

    /* The only tolerated failure here is buffer too small, which is expected */
    if (!NT_SUCCESS(Status) && (Status != STATUS_BUFFER_OVERFLOW))
    {
        goto Quit;
    }

    /* Compute the needed size to store the DOS path */
    Length = FIELD_OFFSET(MOUNTMGR_VOLUME_PATHS, MultiSz) + VolumePath.MultiSzLength;
    if (Length > MAXUSHORT)
    {
        Status = STATUS_INVALID_BUFFER_SIZE;
        goto Quit;
    }

    /* Reallocate the buffer */
    VolumePathPtr = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, Length);
    if (!VolumePathPtr)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quit;
    }

    /* Re-query the DOS path with the proper size */
    Status = NtDeviceIoControlFile(MountMgrHandle,
                                   NULL, NULL, NULL,
                                   &IoStatusBlock,
                                   IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH,
                                   &DeviceName, DeviceNameLength,
                                   VolumePathPtr, Length);
    if (!NT_SUCCESS(Status))
        goto Quit;

    /* Retrieve the drive letter */
    DosPath.Length = DosPath.MaximumLength = (USHORT)VolumePathPtr->MultiSzLength - 2 * sizeof(UNICODE_NULL);
    DosPath.Buffer = VolumePathPtr->MultiSz;
    if (IS_DRIVE_LETTER(&DosPath))
    {
        /* Return the drive letter, ensuring it's uppercased */
        *DriveLetter = towupper(DosPath.Buffer[0]);
    }

    /* We are done, return success */
    Status = STATUS_SUCCESS;

Quit:
    if (VolumePathPtr)
        RtlFreeHeap(ProcessHeap, 0, VolumePathPtr);
    NtClose(MountMgrHandle);
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
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE MountMgrHandle;
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

    /* First, build the corresponding device name */
    DeviceName.DeviceNameLength = VolumeName->Length;
    RtlCopyMemory(&DeviceName.DeviceName, VolumeName->Buffer, VolumeName->Length);

    /* Now, query the MountMgr for the drive letter */
    Status = GetMountMgrHandle(&MountMgrHandle, FILE_READ_ACCESS | FILE_WRITE_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MountMgr unavailable: Status 0x%08lx\n", Status);
        return Status;
    }

    Status = NtDeviceIoControlFile(MountMgrHandle,
                                   NULL, NULL, NULL,
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
    NtClose(MountMgrHandle);
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

    /* Get the volume drive letter */
    __debugbreak();
    RtlInitUnicodeString(&Name, Volume->DeviceName);
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
        // TODO: Should we notify MountMgr to delete the drive letter?
        Volume->DriveLetter = UNICODE_NULL;
        Volume->VolumeLabel[0] = UNICODE_NULL;
        Volume->FileSystem[0] = UNICODE_NULL;
    }

    return Status;
}

/* EOF */
