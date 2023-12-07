/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Volume utility functions
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "diskpart.h"
#include <ntddscsi.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

LIST_ENTRY VolumeListHead;

/* FUNCTIONS THAT CAME FROM PARTLIST TODO USE THEM ****************************/

// AssignDriveLetters

// DetectFileSystem


/* FUNCTIONS ******************************************************************/

#if 0 // FIXME
//
// FIXME: Improve
//
static
VOID
GetVolumeExtents(
    _In_ HANDLE VolumeHandle,
    _In_ PVOLENTRY VolumeEntry)
{
    DWORD dwBytesReturned = 0, dwLength, i;
    PVOLUME_DISK_EXTENTS pExtents;
    BOOL bResult;
    DWORD dwError;

    dwLength = sizeof(VOLUME_DISK_EXTENTS);
    pExtents = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, dwLength);
    if (pExtents == NULL)
        return;

    bResult = DeviceIoControl(VolumeHandle,
                              IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                              NULL,
                              0,
                              pExtents,
                              dwLength,
                              &dwBytesReturned,
                              NULL);
    if (!bResult)
    {
        dwError = GetLastError();

        if (dwError != ERROR_MORE_DATA)
        {
            RtlFreeHeap(ProcessHeap, 0, pExtents);
            return;
        }
        else
        {
            dwLength = sizeof(VOLUME_DISK_EXTENTS) + ((pExtents->NumberOfDiskExtents - 1) * sizeof(DISK_EXTENT));
            RtlFreeHeap(ProcessHeap, 0, pExtents);
            pExtents = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, dwLength);
            if (pExtents == NULL)
            {
                return;
            }

            bResult = DeviceIoControl(VolumeHandle,
                                      IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                      NULL,
                                      0,
                                      pExtents,
                                      dwLength,
                                      &dwBytesReturned,
                                      NULL);
            if (!bResult)
            {
                RtlFreeHeap(ProcessHeap, 0, pExtents);
                return;
            }
        }
    }

    for (i = 0; i < pExtents->NumberOfDiskExtents; i++)
        VolumeEntry->Size.QuadPart += pExtents->Extents[i].ExtentLength.QuadPart;

    VolumeEntry->pExtents = pExtents;
}

//
// FIXME: Improve
//
static
VOID
GetVolumeType(
    _In_ HANDLE VolumeHandle,
    _In_ PVOLENTRY VolumeEntry)
{
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtQueryVolumeInformationFile(VolumeHandle,
                                          &IoStatusBlock,
                                          &DeviceInfo,
                                          sizeof(FILE_FS_DEVICE_INFORMATION),
                                          FileFsDeviceInformation);
    if (!NT_SUCCESS(Status))
        return;

    switch (DeviceInfo.DeviceType)
    {
        case FILE_DEVICE_CD_ROM:
        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
            VolumeEntry->VolumeType = VOLUME_TYPE_CDROM;
            break;

        case FILE_DEVICE_DISK:
        case FILE_DEVICE_DISK_FILE_SYSTEM:
            if (DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA)
                VolumeEntry->VolumeType = VOLUME_TYPE_REMOVABLE;
            else
                VolumeEntry->VolumeType = VOLUME_TYPE_PARTITION;
            break;

        default:
            VolumeEntry->VolumeType = VOLUME_TYPE_UNKNOWN;
            break;
    }
}

//
// FIXME: Improve
//
static
VOID
AddVolumeToList(
    ULONG ulVolumeNumber,
    PWSTR pszVolumeName)
{
    PVOLENTRY VolumeEntry;
    HANDLE VolumeHandle;

    DWORD dwError, dwLength;
    WCHAR szPathNames[MAX_PATH + 1];
    WCHAR szVolumeName[MAX_PATH + 1];
    WCHAR szFilesystem[MAX_PATH + 1];

    DWORD  CharCount            = 0;
    size_t Index                = 0;

    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    DPRINT("AddVolumeToList(%S)\n", pszVolumeName);

    VolumeEntry = RtlAllocateHeap(ProcessHeap,
                                  HEAP_ZERO_MEMORY,
                                  sizeof(VOLENTRY));
    if (VolumeEntry == NULL)
        return;

    VolumeEntry->VolumeNumber = ulVolumeNumber;
    wcscpy(VolumeEntry->VolumeName, pszVolumeName);

    Index = wcslen(pszVolumeName) - 1;

    pszVolumeName[Index] = L'\0';

    CharCount = QueryDosDeviceW(&pszVolumeName[4], VolumeEntry->DeviceName, ARRAYSIZE(VolumeEntry->DeviceName)); 

    pszVolumeName[Index] = L'\\';

    if (CharCount == 0)
    {
        RtlFreeHeap(ProcessHeap, 0, VolumeEntry);
        return;
    }

    DPRINT("DeviceName: %S\n", VolumeEntry->DeviceName);

    RtlInitUnicodeString(&Name, VolumeEntry->DeviceName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenFile(&VolumeHandle,
                        SYNCHRONIZE,
                        &ObjectAttributes,
                        &Iosb,
                        0,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);
    if (NT_SUCCESS(Status))
    {
        GetVolumeType(VolumeHandle, VolumeEntry);
        GetVolumeExtents(VolumeHandle, VolumeEntry);
        NtClose(VolumeHandle);
    }

    if (GetVolumeInformationW(pszVolumeName,
                              szVolumeName,
                              MAX_PATH + 1,
                              NULL, //  [out, optional] LPDWORD lpVolumeSerialNumber,
                              NULL, //  [out, optional] LPDWORD lpMaximumComponentLength,
                              NULL, //  [out, optional] LPDWORD lpFileSystemFlags,
                              szFilesystem,
                              MAX_PATH + 1))
    {
        VolumeEntry->pszLabel = RtlAllocateHeap(ProcessHeap,
                                                0,
                                                (wcslen(szVolumeName) + 1) * sizeof(WCHAR));
        if (VolumeEntry->pszLabel)
            wcscpy(VolumeEntry->pszLabel, szVolumeName);

        VolumeEntry->pszFilesystem = RtlAllocateHeap(ProcessHeap,
                                                     0,
                                                     (wcslen(szFilesystem) + 1) * sizeof(WCHAR));
        if (VolumeEntry->pszFilesystem)
            wcscpy(VolumeEntry->pszFilesystem, szFilesystem);
    }
    else
    {
        dwError = GetLastError();
        if (dwError == ERROR_UNRECOGNIZED_VOLUME)
        {
            VolumeEntry->pszFilesystem = RtlAllocateHeap(ProcessHeap,
                                                         0,
                                                         (3 + 1) * sizeof(WCHAR));
            if (VolumeEntry->pszFilesystem)
                wcscpy(VolumeEntry->pszFilesystem, L"RAW");
        }
    }

    if (GetVolumePathNamesForVolumeNameW(pszVolumeName,
                                         szPathNames,
                                         ARRAYSIZE(szPathNames),
                                         &dwLength))
    {
        VolumeEntry->DriveLetter = szPathNames[0];
    }

    InsertTailList(&VolumeListHead,
                   &VolumeEntry->ListEntry);
}
#endif


NTSTATUS
MountVolume(
    _Inout_ PVOLINFO Volume,
    _In_opt_ UCHAR MbrPartitionType)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    HANDLE VolumeHandle;

#if 0
    /* Reset some volume information */
    Volume->DriveLetter = L'\0';
    Volume->FileSystem[0] = L'\0';
    RtlZeroMemory(Volume->VolumeLabel, sizeof(Volume->VolumeLabel));
#endif

    /* Specify the partition as initially unformatted */
    Volume->FileSystem[0] = L'\0';

    /* Initialize the partition volume label */
    RtlZeroMemory(Volume->VolumeLabel, sizeof(Volume->VolumeLabel));

#if 0
    if (!IsRecognizedPartition(MbrPartitionType))
    {
        /* Unknown partition, hence unknown format (may or may not be actually formatted) */
        Volume->FormatState = UnknownFormat;
        return STATUS_SUCCESS;
    }
#else
    if (!*Volume->DeviceName)
    {
        /* No volume attached, bail out */
        return STATUS_SUCCESS;
    }
#endif

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

    /* Try to guess the mounted FS */
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
    if (!*Volume->DeviceName || !*Volume->FileSystem)
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


//
// FIXME: Improve; for the moment a temporary function is written below.
//
#if 0
NTSTATUS
CreateVolumeList(
    _Out_ PLIST_ENTRY VolumeListHead)
{
    BOOL Success;
    HANDLE hVolume = INVALID_HANDLE_VALUE;
    ULONG ulVolumeNumber = 0;
    WCHAR szVolumeName[MAX_PATH];

    InitializeListHead(VolumeListHead);

    hVolume = FindFirstVolumeW(szVolumeName, ARRAYSIZE(szVolumeName));
    if (hVolume == INVALID_HANDLE_VALUE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    AddVolumeToList(ulVolumeNumber++, szVolumeName);

    for (;;)
    {
        Success = FindNextVolumeW(hVolume, szVolumeName, ARRAYSIZE(szVolumeName));
        if (!Success)
        {
            break;
        }

        AddVolumeToList(ulVolumeNumber++, szVolumeName);
    }

    FindVolumeClose(hVolume);

    return STATUS_SUCCESS;
}
#else
NTSTATUS
CreateVolumeList(
    _Out_ PLIST_ENTRY VolumeListHead)
{
}
#endif


//
// FIXME: Improve, see also DestroyVolumeList
//
#if 0
VOID
RemoveVolume(
    _In_ PVOLENTRY VolumeEntry)
{
    RemoveEntryList(&VolumeEntry->ListEntry);

    if (VolumeEntry->pszLabel)
        RtlFreeHeap(ProcessHeap, 0, VolumeEntry->pszLabel);

    if (VolumeEntry->pszFilesystem)
        RtlFreeHeap(ProcessHeap, 0, VolumeEntry->pszFilesystem);

    if (VolumeEntry->pExtents)
        RtlFreeHeap(ProcessHeap, 0, VolumeEntry->pExtents);

    /* Release volume entry */
    RtlFreeHeap(ProcessHeap, 0, VolumeEntry);
}
#else
VOID
RemoveVolume(
    _In_ PVOLENTRY VolumeEntry)
{
    // VolumeEntry->FormatState = Unformatted;
    // VolumeEntry->FileSystem[0] = L'\0';
    // VolumeEntry->DriveLetter = 0;
    // RtlZeroMemory(VolumeEntry->VolumeLabel, sizeof(VolumeEntry->VolumeLabel));
    RtlFreeHeap(ProcessHeap, 0, VolumeEntry);
}
#endif

//
// TODO: Improve, see also RemoveVolume
//
VOID
DestroyVolumeList(
    _In_ PLIST_ENTRY VolumeListHead)
{
    PLIST_ENTRY Entry;
    PVOLENTRY VolumeEntry;

    /* Release volume info */
    while (!IsListEmpty(VolumeListHead))
    {
        Entry = RemoveHeadList(VolumeListHead);
        VolumeEntry = CONTAINING_RECORD(Entry, VOLENTRY, ListEntry);

        if (VolumeEntry->pszLabel)
            RtlFreeHeap(ProcessHeap, 0, VolumeEntry->pszLabel);

        if (VolumeEntry->pszFilesystem)
            RtlFreeHeap(ProcessHeap, 0, VolumeEntry->pszFilesystem);

        if (VolumeEntry->pExtents)
            RtlFreeHeap(ProcessHeap, 0, VolumeEntry->pExtents);

        /* Release volume entry */
        RtlFreeHeap(ProcessHeap, 0, VolumeEntry);
    }
}

//
// TODO: Improve. For example, do this calculation lookup while
// listing the volumes during list creation, then here, just do
// a quick lookup (for example each VOLENTRY could contain a
// linked-list to the partition(s) on which it is based upon).
//
PVOLENTRY
GetVolumeFromPartition(
    _In_ PLIST_ENTRY VolumeListHead,
    _In_ PPARTENTRY PartEntry)
{
    PLIST_ENTRY Entry;
    PVOLENTRY VolumeEntry;
    ULONG i;

    if (!PartEntry || !PartEntry->DiskEntry)
        return NULL;

    for (Entry = VolumeListHead->Flink;
         Entry != VolumeListHead;
         Entry = Entry->Flink)
    {
        VolumeEntry = CONTAINING_RECORD(Entry, VOLENTRY, ListEntry);

        if (VolumeEntry->pExtents == NULL)
            return NULL;

        for (i = 0; i < VolumeEntry->pExtents->NumberOfDiskExtents; i++)
        {
            if (VolumeEntry->pExtents->Extents[i].DiskNumber == PartEntry->DiskEntry->DiskNumber)
            {
                if ((VolumeEntry->pExtents->Extents[i].StartingOffset.QuadPart == PartEntry->StartSector.QuadPart * PartEntry->DiskEntry->BytesPerSector) &&
                    (VolumeEntry->pExtents->Extents[i].ExtentLength.QuadPart == PartEntry->SectorCount.QuadPart * PartEntry->DiskEntry->BytesPerSector))
                {
                    return VolumeEntry;
                }
            }
        }
    }

    return NULL;
}

/* EOF */
