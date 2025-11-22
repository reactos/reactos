/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/disk.c
 * PURPOSE:         Disk and Drive functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Erik Bos, Alexandre Julliard :
 *                      GetLogicalDriveStringsA,
 *                      GetLogicalDriveStringsW, GetLogicalDrives
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */
//WINE copyright notice:
/*
 * DOS drives handling functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996 Alexandre Julliard
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>

#define MAX_DOS_DRIVES 26

/*
 * @implemented
 */
/* Synced to Wine-2008/12/28 */
DWORD
WINAPI
GetLogicalDriveStringsA(IN DWORD nBufferLength,
                        IN LPSTR lpBuffer)
{
    DWORD drive, count;
    DWORD dwDriveMap;
    LPSTR p;

    dwDriveMap = GetLogicalDrives();

    for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
    {
        if (dwDriveMap & (1<<drive))
            count++;
    }


    if ((count * 4) + 1 > nBufferLength) return ((count * 4) + 1);

    p = lpBuffer;

    for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
        if (dwDriveMap & (1<<drive))
        {
            *p++ = 'A' + (UCHAR)drive;
            *p++ = ':';
            *p++ = '\\';
            *p++ = '\0';
        }
    *p = '\0';

    return (count * 4);
}

/*
 * @implemented
 */
/* Synced to Wine-2008/12/28 */
DWORD
WINAPI
GetLogicalDriveStringsW(IN DWORD nBufferLength,
                        IN LPWSTR lpBuffer)
{
    DWORD drive, count;
    DWORD dwDriveMap;
    LPWSTR p;

    dwDriveMap = GetLogicalDrives();

    for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
    {
        if (dwDriveMap & (1<<drive))
            count++;
    }

    if ((count * 4) + 1 > nBufferLength) return ((count * 4) + 1);

    p = lpBuffer;
    for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
        if (dwDriveMap & (1<<drive))
        {
            *p++ = (WCHAR)('A' + drive);
            *p++ = (WCHAR)':';
            *p++ = (WCHAR)'\\';
            *p++ = (WCHAR)'\0';
        }
    *p = (WCHAR)'\0';

    return (count * 4);
}

/*
 * @implemented
 */
/* Synced to Wine-? */
DWORD
WINAPI
GetLogicalDrives(VOID)
{
    NTSTATUS Status;
    PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;

    /* Get the Device Map for this Process */
    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessDeviceMap,
                                       &ProcessDeviceMapInfo.Query,
                                       sizeof(ProcessDeviceMapInfo.Query),
                                       NULL);

    /* Return the Drive Map */
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    if (ProcessDeviceMapInfo.Query.DriveMap == 0)
    {
        SetLastError(ERROR_SUCCESS);
    }

    return ProcessDeviceMapInfo.Query.DriveMap;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetDiskFreeSpaceA(IN LPCSTR lpRootPathName,
                  OUT LPDWORD lpSectorsPerCluster,
                  OUT LPDWORD lpBytesPerSector,
                  OUT LPDWORD lpNumberOfFreeClusters,
                  OUT LPDWORD lpTotalNumberOfClusters)
{
    PCSTR RootPath;
    PUNICODE_STRING RootPathU;

    RootPath = lpRootPathName;
    if (RootPath == NULL)
    {
        RootPath = "\\";
    }

    RootPathU = Basep8BitStringToStaticUnicodeString(RootPath);
    if (RootPathU == NULL)
    {
        return FALSE;
    }

    return GetDiskFreeSpaceW(RootPathU->Buffer, lpSectorsPerCluster,
                             lpBytesPerSector, lpNumberOfFreeClusters,
                             lpTotalNumberOfClusters);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetDiskFreeSpaceW(IN LPCWSTR lpRootPathName,
                  OUT LPDWORD lpSectorsPerCluster,
                  OUT LPDWORD lpBytesPerSector,
                  OUT LPDWORD lpNumberOfFreeClusters,
                  OUT LPDWORD lpTotalNumberOfClusters)
{
    BOOL Below2GB;
    PCWSTR RootPath;
    NTSTATUS Status;
    HANDLE RootHandle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    FILE_FS_SIZE_INFORMATION FileFsSize;

    /* If no path provided, get root path */
    RootPath = lpRootPathName;
    if (lpRootPathName == NULL)
    {
        RootPath = L"\\";
    }

    /* Convert the path to NT path */
    if (!RtlDosPathNameToNtPathName_U(RootPath, &FileName, NULL, NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Open it for disk space query! */
    InitializeObjectAttributes(&ObjectAttributes, &FileName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&RootHandle, SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_FREE_SPACE_QUERY);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        RtlFreeHeap(RtlGetProcessHeap(), 0, FileName.Buffer);
        if (lpBytesPerSector != NULL)
        {
            *lpBytesPerSector = 0;
        }

        return FALSE;
    }

    /* We don't need the name any longer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, FileName.Buffer);

    /* Query disk space! */
    Status = NtQueryVolumeInformationFile(RootHandle, &IoStatusBlock, &FileFsSize,
                                          sizeof(FILE_FS_SIZE_INFORMATION),
                                          FileFsSizeInformation);
    NtClose(RootHandle);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Are we in some compatibility mode where size must be below 2GB? */
    Below2GB = ((NtCurrentPeb()->AppCompatFlags.LowPart & GetDiskFreeSpace2GB) == GetDiskFreeSpace2GB);

    /* If we're to overflow output, make sure we return the maximum */
    if (FileFsSize.TotalAllocationUnits.HighPart != 0)
    {
        FileFsSize.TotalAllocationUnits.LowPart = -1;
    }

    if (FileFsSize.AvailableAllocationUnits.HighPart != 0)
    {
        FileFsSize.AvailableAllocationUnits.LowPart = -1;
    }

    /* Return what user asked for */
    if (lpSectorsPerCluster != NULL)
    {
        *lpSectorsPerCluster = FileFsSize.SectorsPerAllocationUnit;
    }

    if (lpBytesPerSector != NULL)
    {
        *lpBytesPerSector = FileFsSize.BytesPerSector;
    }

    if (lpNumberOfFreeClusters != NULL)
    {
        if (!Below2GB)
        {
            *lpNumberOfFreeClusters = FileFsSize.AvailableAllocationUnits.LowPart;
        }
        /* If we have to remain below 2GB... */
        else
        {
            DWORD FreeClusters;

            /* Compute how many clusters there are in less than 2GB: 2 * 1024 * 1024 * 1024- 1 */
            FreeClusters = 0x7FFFFFFF / (FileFsSize.SectorsPerAllocationUnit * FileFsSize.BytesPerSector);
            /* If that's higher than what was queried, then return the queried value, it's OK! */
            if (FreeClusters > FileFsSize.AvailableAllocationUnits.LowPart)
            {
                FreeClusters = FileFsSize.AvailableAllocationUnits.LowPart;
            }

            *lpNumberOfFreeClusters = FreeClusters;
        }
    }

    if (lpTotalNumberOfClusters != NULL)
    {
        if (!Below2GB)
        {
            *lpTotalNumberOfClusters = FileFsSize.TotalAllocationUnits.LowPart;
        }
        /* If we have to remain below 2GB... */
        else
        {
            DWORD TotalClusters;

            /* Compute how many clusters there are in less than 2GB: 2 * 1024 * 1024 * 1024- 1 */
            TotalClusters = 0x7FFFFFFF / (FileFsSize.SectorsPerAllocationUnit * FileFsSize.BytesPerSector);
            /* If that's higher than what was queried, then return the queried value, it's OK! */
            if (TotalClusters > FileFsSize.TotalAllocationUnits.LowPart)
            {
                TotalClusters = FileFsSize.TotalAllocationUnits.LowPart;
            }

            *lpTotalNumberOfClusters = TotalClusters;
        }
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetDiskFreeSpaceExA(IN LPCSTR lpDirectoryName OPTIONAL,
                    OUT PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                    OUT PULARGE_INTEGER lpTotalNumberOfBytes,
                    OUT PULARGE_INTEGER lpTotalNumberOfFreeBytes)
{
    PCSTR RootPath;
    PUNICODE_STRING RootPathU;

    RootPath = lpDirectoryName;
    if (RootPath == NULL)
    {
        RootPath = "\\";
    }

    RootPathU = Basep8BitStringToStaticUnicodeString(RootPath);
    if (RootPathU == NULL)
    {
        return FALSE;
    }

    return GetDiskFreeSpaceExW(RootPathU->Buffer, lpFreeBytesAvailableToCaller,
                              lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetDiskFreeSpaceExW(IN LPCWSTR lpDirectoryName OPTIONAL,
                    OUT PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                    OUT PULARGE_INTEGER lpTotalNumberOfBytes,
                    OUT PULARGE_INTEGER lpTotalNumberOfFreeBytes)
{
    PCWSTR RootPath;
    NTSTATUS Status;
    HANDLE RootHandle;
    UNICODE_STRING FileName;
    DWORD BytesPerAllocationUnit;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    FILE_FS_SIZE_INFORMATION FileFsSize;

    /* If no path provided, get root path */
    RootPath = lpDirectoryName;
    if (lpDirectoryName == NULL)
    {
        RootPath = L"\\";
    }

    /* Convert the path to NT path */
    if (!RtlDosPathNameToNtPathName_U(RootPath, &FileName, NULL, NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Open it for disk space query! */
    InitializeObjectAttributes(&ObjectAttributes, &FileName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&RootHandle, SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_FREE_SPACE_QUERY);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        /* If error conversion lead to file not found, override to use path not found
         * which is more accurate
         */
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            SetLastError(ERROR_PATH_NOT_FOUND);
        }

        RtlFreeHeap(RtlGetProcessHeap(), 0, FileName.Buffer);

        return FALSE;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, FileName.Buffer);

    /* If user asks for lpTotalNumberOfFreeBytes, try to use full size information */
    if (lpTotalNumberOfFreeBytes != NULL)
    {
        FILE_FS_FULL_SIZE_INFORMATION FileFsFullSize;

        /* Issue the full fs size request */
        Status = NtQueryVolumeInformationFile(RootHandle, &IoStatusBlock, &FileFsFullSize,
                                              sizeof(FILE_FS_FULL_SIZE_INFORMATION),
                                              FileFsFullSizeInformation);
        /* If it succeed, complete out buffers */
        if (NT_SUCCESS(Status))
        {
            /* We can close here, we'll return */
            NtClose(RootHandle);

            /* Compute the size of an AU */
            BytesPerAllocationUnit = FileFsFullSize.SectorsPerAllocationUnit * FileFsFullSize.BytesPerSector;

            /* And then return what was asked */
            if (lpFreeBytesAvailableToCaller != NULL)
            {
                lpFreeBytesAvailableToCaller->QuadPart = FileFsFullSize.CallerAvailableAllocationUnits.QuadPart * BytesPerAllocationUnit;
            }

            if (lpTotalNumberOfBytes != NULL)
            {
                lpTotalNumberOfBytes->QuadPart = FileFsFullSize.TotalAllocationUnits.QuadPart * BytesPerAllocationUnit;
            }

            /* No need to check for nullness ;-) */
            lpTotalNumberOfFreeBytes->QuadPart = FileFsFullSize.ActualAvailableAllocationUnits.QuadPart * BytesPerAllocationUnit;

            return TRUE;
        }
    }

    /* Otherwise, fallback to normal size information */
    Status = NtQueryVolumeInformationFile(RootHandle, &IoStatusBlock,
                                          &FileFsSize, sizeof(FILE_FS_SIZE_INFORMATION),
                                          FileFsSizeInformation);
    NtClose(RootHandle);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Compute the size of an AU */
    BytesPerAllocationUnit = FileFsSize.SectorsPerAllocationUnit * FileFsSize.BytesPerSector;

    /* And then return what was asked, available is free, the same! */
    if (lpFreeBytesAvailableToCaller != NULL)
    {
        lpFreeBytesAvailableToCaller->QuadPart = FileFsSize.AvailableAllocationUnits.QuadPart * BytesPerAllocationUnit;
    }

    if (lpTotalNumberOfBytes != NULL)
    {
        lpTotalNumberOfBytes->QuadPart = FileFsSize.TotalAllocationUnits.QuadPart * BytesPerAllocationUnit;
    }

    if (lpTotalNumberOfFreeBytes != NULL)
    {
        lpTotalNumberOfFreeBytes->QuadPart = FileFsSize.AvailableAllocationUnits.QuadPart * BytesPerAllocationUnit;
    }

    return TRUE;
}

/*
 * @implemented
 */
UINT
WINAPI
GetDriveTypeA(IN LPCSTR lpRootPathName)
{
    PWSTR RootPathU;

    if (lpRootPathName != NULL)
    {
        PUNICODE_STRING RootPathUStr;

        RootPathUStr = Basep8BitStringToStaticUnicodeString(lpRootPathName);
        if (RootPathUStr == NULL)
        {
            return DRIVE_NO_ROOT_DIR;
        }

        RootPathU = RootPathUStr->Buffer;
    }
    else
    {
        RootPathU = NULL;
    }

    return GetDriveTypeW(RootPathU);
}

/*
 * @implemented
 */
UINT
WINAPI
GetDriveTypeW(IN LPCWSTR lpRootPathName)
{
    BOOL RetryOpen;
    PCWSTR RootPath;
    NTSTATUS Status;
    WCHAR DriveLetter;
    HANDLE RootHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING PathName, VolumeString;
    FILE_FS_DEVICE_INFORMATION FileFsDevice;
    WCHAR Buffer[MAX_PATH], VolumeName[MAX_PATH];

    /* If no path, get one */
    if (lpRootPathName == NULL)
    {
        RootPath = Buffer;
        /* This will be current drive (<letter>:\ - drop the rest)*/
        if (RtlGetCurrentDirectory_U(sizeof(Buffer), Buffer) > 3 * sizeof(WCHAR))
        {
            Buffer[3] = UNICODE_NULL;
        }
    }
    else
    {
        /* Handle broken value */
        if (lpRootPathName == (PVOID)-1)
        {
            return DRIVE_UNKNOWN;
        }

        RootPath = lpRootPathName;
        /* If provided path is 2-len, it might be a drive letter... */
        if (wcslen(lpRootPathName) == 2)
        {
            /* Check it! */
            DriveLetter = RtlUpcaseUnicodeChar(lpRootPathName[0]);
            /* That's a drive letter! */
            if (DriveLetter >= L'A' && DriveLetter <= L'Z' && lpRootPathName[1] == L':')
            {
                /* Make it a volume */
                Buffer[0] = DriveLetter;
                Buffer[1] = L':';
                Buffer[2] = L'\\';
                Buffer[3] = UNICODE_NULL;
                RootPath = Buffer;
            }
        }
    }

    /* If the provided looks like a DOS device... Like <letter>:\<0> */
    DriveLetter = RtlUpcaseUnicodeChar(RootPath[0]);
    /* We'll take the quick path!
     * We'll find the device type looking at the device map (and types ;-))
     * associated with the current process
     */
    if (DriveLetter >= L'A' && DriveLetter <= L'Z' && RootPath[1] == L':' &&
        RootPath[2] == L'\\' && RootPath[3] == UNICODE_NULL)
    {
        USHORT Index;
        PROCESS_DEVICEMAP_INFORMATION DeviceMap;

        /* Query the device map */
        Status = NtQueryInformationProcess(NtCurrentProcess(),
                                           ProcessDeviceMap,
                                           &DeviceMap.Query,
                                           sizeof(DeviceMap.Query),
                                           NULL);
        /* Zero output if we failed */
        if (!NT_SUCCESS(Status))
        {
            RtlZeroMemory(&DeviceMap, sizeof(PROCESS_DEVICEMAP_INFORMATION));
        }

        /* Get our index in the device map */
        Index = DriveLetter - L'A';
        /* Check we're in the device map (bit set) */
        if (((1 << Index) & DeviceMap.Query.DriveMap) != 0)
        {
            /* Validate device type and return it */
            if (DeviceMap.Query.DriveType[Index] >= DRIVE_REMOVABLE &&
                DeviceMap.Query.DriveType[Index] <= DRIVE_RAMDISK)
            {
                return DeviceMap.Query.DriveType[Index];
            }
            /* Otherwise, return we don't know the type */
            else
            {
                return DRIVE_UNKNOWN;
            }
        }

        /* We couldn't find ourselves, do it the slow way */
    }

    /* No path provided, use root */
    if (lpRootPathName == NULL)
    {
        RootPath = L"\\";
    }

    /* Convert to NT path */
    if (!RtlDosPathNameToNtPathName_U(RootPath, &PathName, NULL, NULL))
    {
        return DRIVE_NO_ROOT_DIR;
    }

    /* If not a directory, fail, we need a volume */
    if (PathName.Buffer[(PathName.Length / sizeof(WCHAR)) - 1] != L'\\')
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, PathName.Buffer);
        return DRIVE_NO_ROOT_DIR;
    }

    /* We will work with a device object, so trim the trailing backslash now */
    PathName.Length -= sizeof(WCHAR);

    /* Let's probe for it, by forcing open failure! */
    RetryOpen = TRUE;
    InitializeObjectAttributes(&ObjectAttributes, &PathName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&RootHandle, SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                        &ObjectAttributes, &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
    /* It properly failed! */
    if (Status == STATUS_FILE_IS_A_DIRECTORY)
    {
        /* It might be a mount point, then, query for target */
        if (BasepGetVolumeNameFromReparsePoint(lpRootPathName, VolumeName, MAX_PATH, NULL))
        {
            /* We'll reopen the target */
            RtlInitUnicodeString(&VolumeString, VolumeName);
            VolumeName[1] = L'?';
            VolumeString.Length -= sizeof(WCHAR);
            InitializeObjectAttributes(&ObjectAttributes, &VolumeString,
                                       OBJ_CASE_INSENSITIVE, NULL, NULL);
        }
    }
    else
    {
        /* heh. It worked? Or failed for whatever other reason?
         * Check we have a directory if we get farther in path
         */
        PathName.Length += sizeof(WCHAR);
        if (IsThisARootDirectory(0, &PathName))
        {
            /* Yes? Heh, then it's fine, keep our current handle */
            RetryOpen = FALSE;
        }
        else
        {
            /* Then, retry to open without forcing non directory type */
            PathName.Length -= sizeof(WCHAR);
            if (NT_SUCCESS(Status))
            {
                NtClose(RootHandle);
            }
        }
    }

    /* Now, we retry without forcing file type - should work now */
    if (RetryOpen)
    {
        Status = NtOpenFile(&RootHandle, SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                            &ObjectAttributes, &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_SYNCHRONOUS_IO_NONALERT);
    }

    /* We don't need path any longer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, PathName.Buffer);
    if (!NT_SUCCESS(Status))
    {
        return DRIVE_NO_ROOT_DIR;
    }

    /* Query the device for its type */
    Status = NtQueryVolumeInformationFile(RootHandle,
                                          &IoStatusBlock,
                                          &FileFsDevice,
                                          sizeof(FILE_FS_DEVICE_INFORMATION),
                                          FileFsDeviceInformation);
    /* No longer required */
    NtClose(RootHandle);
    if (!NT_SUCCESS(Status))
    {
        return DRIVE_UNKNOWN;
    }

    /* Do we have a remote device? Return so! */
    if ((FileFsDevice.Characteristics & FILE_REMOTE_DEVICE) == FILE_REMOTE_DEVICE)
    {
        return DRIVE_REMOTE;
    }

    /* Check the device type */
    switch (FileFsDevice.DeviceType)
    {
        /* CDROM, easy */
        case FILE_DEVICE_CD_ROM:
        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
            return DRIVE_CDROM;

        /* Disk... */
        case FILE_DEVICE_DISK:
        case FILE_DEVICE_DISK_FILE_SYSTEM:
            /* Removable media? Floppy is one */
            if ((FileFsDevice.Characteristics & FILE_REMOVABLE_MEDIA) == FILE_REMOVABLE_MEDIA ||
                (FileFsDevice.Characteristics & FILE_FLOPPY_DISKETTE) == FILE_FLOPPY_DISKETTE)
            {
                return DRIVE_REMOVABLE;
            }
            else
            {
                return DRIVE_FIXED;
            }

        /* Easy cases */
        case FILE_DEVICE_NETWORK:
        case FILE_DEVICE_NETWORK_FILE_SYSTEM:
            return DRIVE_REMOTE;

        case FILE_DEVICE_VIRTUAL_DISK:
            return DRIVE_RAMDISK;
    }

    /* Nothing matching, just fail */
    return DRIVE_UNKNOWN;
}

/* EOF */
