/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/partlist.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Eric Kohl
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

// DismountVolume

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
