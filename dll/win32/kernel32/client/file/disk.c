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
#include <strsafe.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(kernel32file);

#define MAX_DOS_DRIVES 26
HANDLE WINAPI InternalOpenDirW(IN LPCWSTR DirName, IN BOOLEAN Write);

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
                                       &ProcessDeviceMapInfo,
                                       sizeof(ProcessDeviceMapInfo),
                                       NULL);

    /* Return the Drive Map */
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
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
    PWCHAR RootPathNameW=NULL;

    if (lpRootPathName)
    {
        if (!(RootPathNameW = FilenameA2W(lpRootPathName, FALSE)))
            return FALSE;
    }

    return GetDiskFreeSpaceW (RootPathNameW,
                              lpSectorsPerCluster,
                              lpBytesPerSector,
                              lpNumberOfFreeClusters,
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
    FILE_FS_SIZE_INFORMATION FileFsSize;
    IO_STATUS_BLOCK IoStatusBlock;
    WCHAR RootPathName[MAX_PATH];
    HANDLE hFile;
    NTSTATUS errCode;

    if (lpRootPathName)
    {
        wcsncpy (RootPathName, lpRootPathName, 3);
    }
    else
    {
        GetCurrentDirectoryW (MAX_PATH, RootPathName);
    }
    RootPathName[3] = 0;

    hFile = InternalOpenDirW(RootPathName, FALSE);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    errCode = NtQueryVolumeInformationFile(hFile,
                                           &IoStatusBlock,
                                           &FileFsSize,
                                           sizeof(FILE_FS_SIZE_INFORMATION),
                                           FileFsSizeInformation);
    if (!NT_SUCCESS(errCode))
    {
        CloseHandle(hFile);
        BaseSetLastNTError (errCode);
        return FALSE;
    }

    if (lpSectorsPerCluster)
        *lpSectorsPerCluster = FileFsSize.SectorsPerAllocationUnit;
    if (lpBytesPerSector)
        *lpBytesPerSector = FileFsSize.BytesPerSector;
    if (lpNumberOfFreeClusters)
        *lpNumberOfFreeClusters = FileFsSize.AvailableAllocationUnits.u.LowPart;
    if (lpTotalNumberOfClusters)
        *lpTotalNumberOfClusters = FileFsSize.TotalAllocationUnits.u.LowPart;
    CloseHandle(hFile);

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
    PWCHAR DirectoryNameW=NULL;

    if (lpDirectoryName)
    {
        if (!(DirectoryNameW = FilenameA2W(lpDirectoryName, FALSE)))
            return FALSE;
    }

    return GetDiskFreeSpaceExW (DirectoryNameW ,
                                lpFreeBytesAvailableToCaller,
                                lpTotalNumberOfBytes,
                                lpTotalNumberOfFreeBytes);
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
    union
    {
        FILE_FS_SIZE_INFORMATION FsSize;
        FILE_FS_FULL_SIZE_INFORMATION FsFullSize;
    } FsInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    ULARGE_INTEGER BytesPerCluster;
    HANDLE hFile;
    NTSTATUS Status;

    if (lpDirectoryName == NULL)
        lpDirectoryName = L"\\";

    hFile = InternalOpenDirW(lpDirectoryName, FALSE);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        return FALSE;
    }

    if (lpFreeBytesAvailableToCaller != NULL || lpTotalNumberOfBytes != NULL)
    {
        /* To get the free space available to the user associated with the
           current thread, try FileFsFullSizeInformation. If this is not
           supported by the file system, fall back to FileFsSize */

        Status = NtQueryVolumeInformationFile(hFile,
                                              &IoStatusBlock,
                                              &FsInfo.FsFullSize,
                                              sizeof(FsInfo.FsFullSize),
                                              FileFsFullSizeInformation);

        if (NT_SUCCESS(Status))
        {
            /* Close the handle before returning data
               to avoid a handle leak in case of a fault! */
            CloseHandle(hFile);

            BytesPerCluster.QuadPart =
                FsInfo.FsFullSize.BytesPerSector * FsInfo.FsFullSize.SectorsPerAllocationUnit;

            if (lpFreeBytesAvailableToCaller != NULL)
            {
                lpFreeBytesAvailableToCaller->QuadPart =
                    BytesPerCluster.QuadPart * FsInfo.FsFullSize.CallerAvailableAllocationUnits.QuadPart;
            }

            if (lpTotalNumberOfBytes != NULL)
            {
                lpTotalNumberOfBytes->QuadPart =
                    BytesPerCluster.QuadPart * FsInfo.FsFullSize.TotalAllocationUnits.QuadPart;
            }

            if (lpTotalNumberOfFreeBytes != NULL)
            {
                lpTotalNumberOfFreeBytes->QuadPart =
                    BytesPerCluster.QuadPart * FsInfo.FsFullSize.ActualAvailableAllocationUnits.QuadPart;
            }

            return TRUE;
        }
    }

    Status = NtQueryVolumeInformationFile(hFile,
                                          &IoStatusBlock,
                                          &FsInfo.FsSize,
                                          sizeof(FsInfo.FsSize),
                                          FileFsSizeInformation);

    /* Close the handle before returning data
       to avoid a handle leak in case of a fault! */
    CloseHandle(hFile);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError (Status);
        return FALSE;
    }

    BytesPerCluster.QuadPart =
        FsInfo.FsSize.BytesPerSector * FsInfo.FsSize.SectorsPerAllocationUnit;

    if (lpFreeBytesAvailableToCaller)
    {
        lpFreeBytesAvailableToCaller->QuadPart =
            BytesPerCluster.QuadPart * FsInfo.FsSize.AvailableAllocationUnits.QuadPart;
    }

    if (lpTotalNumberOfBytes)
    {
        lpTotalNumberOfBytes->QuadPart =
            BytesPerCluster.QuadPart * FsInfo.FsSize.TotalAllocationUnits.QuadPart;
    }

    if (lpTotalNumberOfFreeBytes)
    {
        lpTotalNumberOfFreeBytes->QuadPart =
            BytesPerCluster.QuadPart * FsInfo.FsSize.AvailableAllocationUnits.QuadPart;
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
    PWCHAR RootPathNameW;

    if (!lpRootPathName)
        return GetDriveTypeW(NULL);

    if (!(RootPathNameW = FilenameA2W(lpRootPathName, FALSE)))
        return DRIVE_UNKNOWN;

    return GetDriveTypeW(RootPathNameW);
}

/*
 * @implemented
 */
UINT
WINAPI
GetDriveTypeW(IN LPCWSTR lpRootPathName)
{
    FILE_FS_DEVICE_INFORMATION FileFsDevice;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING PathName;
    HANDLE FileHandle;
    NTSTATUS Status;
    PWSTR CurrentDir = NULL;
    PCWSTR lpRootPath;

    if (!lpRootPathName)
    {
        /* If NULL is passed, use current directory path */
        DWORD BufferSize = GetCurrentDirectoryW(0, NULL);
        CurrentDir = HeapAlloc(GetProcessHeap(), 0, BufferSize * sizeof(WCHAR));
        if (!CurrentDir)
            return DRIVE_UNKNOWN;
        if (!GetCurrentDirectoryW(BufferSize, CurrentDir))
        {
            HeapFree(GetProcessHeap(), 0, CurrentDir);
            return DRIVE_UNKNOWN;
        }

        if (wcslen(CurrentDir) > 3)
            CurrentDir[3] = 0;

        lpRootPath = CurrentDir;
    }
    else
    {
        size_t Length = wcslen(lpRootPathName);

        TRACE("lpRootPathName: %S\n", lpRootPathName);

        lpRootPath = lpRootPathName;
        if (Length == 2)
        {
            WCHAR DriveLetter = RtlUpcaseUnicodeChar(lpRootPathName[0]);

            if (DriveLetter >= L'A' && DriveLetter <= L'Z' && lpRootPathName[1] == L':')
            {
                Length = (Length + 2) * sizeof(WCHAR);

                CurrentDir = HeapAlloc(GetProcessHeap(), 0, Length);
                if (!CurrentDir)
                    return DRIVE_UNKNOWN;

                StringCbPrintfW(CurrentDir, Length, L"%s\\", lpRootPathName);

                lpRootPath = CurrentDir;
            }
        }
    }

    TRACE("lpRootPath: %S\n", lpRootPath);

    if (!RtlDosPathNameToNtPathName_U(lpRootPath, &PathName, NULL, NULL))
    {
        if (CurrentDir != NULL)
            HeapFree(GetProcessHeap(), 0, CurrentDir);

        return DRIVE_NO_ROOT_DIR;
    }

    TRACE("PathName: %S\n", PathName.Buffer);

    if (CurrentDir != NULL)
        HeapFree(GetProcessHeap(), 0, CurrentDir);

    if (PathName.Buffer[(PathName.Length >> 1) - 1] != L'\\')
    {
        return DRIVE_NO_ROOT_DIR;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &PathName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);

    RtlFreeHeap(RtlGetProcessHeap(), 0, PathName.Buffer);
    if (!NT_SUCCESS(Status))
        return DRIVE_NO_ROOT_DIR; /* According to WINE regression tests */

    Status = NtQueryVolumeInformationFile(FileHandle,
                                          &IoStatusBlock,
                                          &FileFsDevice,
                                          sizeof(FILE_FS_DEVICE_INFORMATION),
                                          FileFsDeviceInformation);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        return 0;
    }

    switch (FileFsDevice.DeviceType)
    {
        case FILE_DEVICE_CD_ROM:
        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
            return DRIVE_CDROM;
        case FILE_DEVICE_VIRTUAL_DISK:
            return DRIVE_RAMDISK;
        case FILE_DEVICE_NETWORK_FILE_SYSTEM:
            return DRIVE_REMOTE;
        case FILE_DEVICE_DISK:
        case FILE_DEVICE_DISK_FILE_SYSTEM:
            if (FileFsDevice.Characteristics & FILE_REMOTE_DEVICE)
                return DRIVE_REMOTE;
            if (FileFsDevice.Characteristics & FILE_REMOVABLE_MEDIA)
                return DRIVE_REMOVABLE;
        return DRIVE_FIXED;
    }

    ERR("Returning DRIVE_UNKNOWN for device type %lu\n", FileFsDevice.DeviceType);

    return DRIVE_UNKNOWN;
}

/* EOF */
