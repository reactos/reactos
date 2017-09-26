/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/volume.c
 * PURPOSE:         File volume functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Erik Bos, Alexandre Julliard :
 *                      GetLogicalDriveStringsA,
 *                      GetLogicalDriveStringsW, GetLogicalDrives
 *                  Pierre Schweitzer (pierre@reactos.org)
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
DEBUG_CHANNEL(kernel32file);

HANDLE
WINAPI
InternalOpenDirW(IN LPCWSTR DirName,
                 IN BOOLEAN Write)
{
    UNICODE_STRING NtPathU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS errCode;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE hFile;

    if (!RtlDosPathNameToNtPathName_U(DirName, &NtPathU, NULL, NULL))
    {
        WARN("Invalid path\n");
        SetLastError(ERROR_BAD_PATHNAME);
        return INVALID_HANDLE_VALUE;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathU,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    errCode = NtCreateFile(&hFile,
                           Write ? FILE_GENERIC_WRITE : FILE_GENERIC_READ,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           0,
                           NULL,
                           0);

    RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathU.Buffer);

    if (!NT_SUCCESS(errCode))
    {
        BaseSetLastNTError(errCode);
        return INVALID_HANDLE_VALUE;
    }

    return hFile;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetVolumeInformationA(IN LPCSTR lpRootPathName,
                      IN LPSTR lpVolumeNameBuffer,
                      IN DWORD nVolumeNameSize,
                      OUT LPDWORD lpVolumeSerialNumber OPTIONAL,
                      OUT LPDWORD lpMaximumComponentLength OPTIONAL,
                      OUT LPDWORD lpFileSystemFlags OPTIONAL,
                      OUT LPSTR lpFileSystemNameBuffer OPTIONAL,
                      IN DWORD nFileSystemNameSize)
{
    BOOL Ret;
    NTSTATUS Status;
    PUNICODE_STRING RootPathNameU;
    ANSI_STRING VolumeName, FileSystemName;
    UNICODE_STRING VolumeNameU, FileSystemNameU;

    /* If no root path provided, default to \ */
    if (lpRootPathName == NULL)
    {
        lpRootPathName = "\\";
    }

    /* Convert root path to unicode */
    RootPathNameU = Basep8BitStringToStaticUnicodeString(lpRootPathName);
    if (RootPathNameU == NULL)
    {
        return FALSE;
    }

    /* Init all our STRINGS (U/A) */
    VolumeNameU.Buffer = NULL;
    VolumeNameU.MaximumLength = 0;
    FileSystemNameU.Buffer = NULL;
    FileSystemNameU.MaximumLength = 0;

    VolumeName.Buffer = lpVolumeNameBuffer;
    VolumeName.MaximumLength = nVolumeNameSize + 1;
    FileSystemName.Buffer = lpFileSystemNameBuffer;
    FileSystemName.MaximumLength = nFileSystemNameSize + 1;

    /* Assume failure for now */
    Ret = FALSE;

    /* If caller wants volume name, allocate a buffer to receive it */
    if (lpVolumeNameBuffer != NULL)
    {
        VolumeNameU.MaximumLength = sizeof(WCHAR) * (nVolumeNameSize + 1);
        VolumeNameU.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0,
	                                         VolumeNameU.MaximumLength);
        if (VolumeNameU.Buffer == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto CleanAndQuit;
        }
    }

    /* If caller wants file system name, allocate a buffer to receive it */
    if (lpFileSystemNameBuffer != NULL)
    {
        FileSystemNameU.MaximumLength = sizeof(WCHAR) * (nVolumeNameSize + 1);
        FileSystemNameU.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0,
	                                             FileSystemNameU.MaximumLength);
        if (FileSystemNameU.Buffer == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto CleanAndQuit;
        }
    }

    /* Call W */
    Ret = GetVolumeInformationW(RootPathNameU->Buffer,  VolumeNameU.Buffer,
                                nVolumeNameSize, lpVolumeSerialNumber,
                                lpMaximumComponentLength, lpFileSystemFlags,
                                FileSystemNameU.Buffer, nFileSystemNameSize);
    /* If it succeed, convert back to ANSI */
    if (Ret)
    {
        if (lpVolumeNameBuffer != NULL)
        {
            RtlInitUnicodeString(&VolumeNameU, VolumeNameU.Buffer);
            Status = RtlUnicodeStringToAnsiString(&VolumeName, &VolumeNameU, FALSE);
            if (!NT_SUCCESS(Status))
            {
                BaseSetLastNTError(Status);
                Ret = FALSE;

                goto CleanAndQuit;
            }
        }

        if (lpFileSystemNameBuffer != NULL)
        {
            RtlInitUnicodeString(&FileSystemNameU, FileSystemNameU.Buffer);
            Status = RtlUnicodeStringToAnsiString(&FileSystemName, &FileSystemNameU, FALSE);
            if (!NT_SUCCESS(Status))
            {
                BaseSetLastNTError(Status);
                Ret = FALSE;

                goto CleanAndQuit;
            }
        }
    }

    /* Clean and quit */
CleanAndQuit:
    if (VolumeNameU.Buffer != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeNameU.Buffer);
    }

    if (FileSystemNameU.Buffer != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, FileSystemNameU.Buffer);
    }

    return Ret;
}

/*
 * @implemented
 */
static BOOL
IsThisARootDirectory(IN HANDLE VolumeHandle,
                     IN PUNICODE_STRING NtPathName)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    struct
    {
        FILE_NAME_INFORMATION;
        WCHAR Buffer[MAX_PATH];
    } FileNameInfo;

    /* If we have a handle, query the name */
    if (VolumeHandle)
    {
        Status = NtQueryInformationFile(VolumeHandle, &IoStatusBlock, &FileNameInfo, sizeof(FileNameInfo), FileNameInformation);
        if (!NT_SUCCESS(Status))
        {
            return FALSE;
        }

        /* Check we properly end with a \ */
        if (FileNameInfo.FileName[FileNameInfo.FileNameLength / sizeof(WCHAR) - 1] != L'\\')
        {
            return FALSE;
        }
    }

    /* If we have a path */
    if (NtPathName != NULL)
    {
        HANDLE LinkHandle;
        WCHAR Buffer[512];
        ULONG ReturnedLength;
        UNICODE_STRING LinkTarget;
        OBJECT_ATTRIBUTES ObjectAttributes;

        NtPathName->Length -= sizeof(WCHAR);

        InitializeObjectAttributes(&ObjectAttributes, NtPathName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL, NULL);

        /* Try to see whether that's a symbolic name */
        Status = NtOpenSymbolicLinkObject(&LinkHandle, SYMBOLIC_LINK_QUERY, &ObjectAttributes);
        NtPathName->Length += sizeof(WCHAR);
        if (!NT_SUCCESS(Status))
        {
            return FALSE;
        }

        /* If so, query the target */
        LinkTarget.Buffer = Buffer;
        LinkTarget.Length = 0;
        LinkTarget.MaximumLength = sizeof(Buffer);

        Status = NtQuerySymbolicLinkObject(LinkHandle, &LinkTarget, &ReturnedLength);
        NtClose(LinkHandle);
        /* A root directory (NtName) is a symbolic link */
        if (!NT_SUCCESS(Status))
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetVolumeInformationW(IN LPCWSTR lpRootPathName,
                      IN LPWSTR lpVolumeNameBuffer,
                      IN DWORD nVolumeNameSize,
                      OUT LPDWORD lpVolumeSerialNumber OPTIONAL,
                      OUT LPDWORD lpMaximumComponentLength OPTIONAL,
                      OUT LPDWORD lpFileSystemFlags OPTIONAL,
                      OUT LPWSTR lpFileSystemNameBuffer OPTIONAL,
                      IN DWORD nFileSystemNameSize)
{
    BOOL Ret;
    NTSTATUS Status;
    HANDLE VolumeHandle;
    LPCWSTR RootPathName;
    UNICODE_STRING NtPathName;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PFILE_FS_VOLUME_INFORMATION VolumeInfo;
    PFILE_FS_ATTRIBUTE_INFORMATION VolumeAttr;
    ULONG OldMode, VolumeInfoSize, VolumeAttrSize;

    /* If no root path provided, default to \ */
    if (lpRootPathName == NULL)
    {
        RootPathName = L"\\";
    }
    else
    {
        RootPathName = lpRootPathName;
    }

    /* Convert to NT name */
    if (!RtlDosPathNameToNtPathName_U(RootPathName, &NtPathName, NULL, NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Check we really end with a backslash */
    if (NtPathName.Buffer[(NtPathName.Length / sizeof(WCHAR)) - 1] != L'\\')
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
        BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
        return FALSE;
    }

    /* Try to open the received path */
    InitializeObjectAttributes(&ObjectAttributes, &NtPathName,
                                OBJ_CASE_INSENSITIVE,
                                NULL, NULL);

    /* No errors to the user */
    RtlSetThreadErrorMode(RTL_SEM_FAILCRITICALERRORS, &OldMode);
    Status = NtOpenFile(&VolumeHandle, SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock, 0, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);
    RtlSetThreadErrorMode(OldMode, NULL);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Check whether that's a root directory */
    if (!IsThisARootDirectory(VolumeHandle, &NtPathName))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);
        NtClose(VolumeHandle);
        SetLastError(ERROR_DIR_NOT_ROOT);
        return FALSE;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);

    /* Assume we don't need to query FileFsVolumeInformation */
    VolumeInfo = NULL;
    /* If user wants volume name, allocate a buffer to query it */
    if (lpVolumeNameBuffer != NULL)
    {
        VolumeInfoSize = nVolumeNameSize + sizeof(FILE_FS_VOLUME_INFORMATION);
    }
    /* If user just wants the serial number, allocate a dummy buffer */
    else if (lpVolumeSerialNumber != NULL)
    {
        VolumeInfoSize = MAX_PATH * sizeof(WCHAR) + sizeof(FILE_FS_VOLUME_INFORMATION);
    }
    /* Otherwise, nothing to query */
    else
    {
        VolumeInfoSize = 0;
    }

    /* If we're to query, allocate a big enough buffer */
    if (VolumeInfoSize != 0)
    {
        VolumeInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, VolumeInfoSize);
        if (VolumeInfo == NULL)
        {
            NtClose(VolumeHandle);
            BaseSetLastNTError(STATUS_NO_MEMORY);
            return FALSE;
        }
    }

    /* Assume we don't need to query FileFsAttributeInformation */
    VolumeAttr = NULL;
    /* If user wants filesystem name, allocate a buffer to query it */
    if (lpFileSystemNameBuffer != NULL)
    {
        VolumeAttrSize = nFileSystemNameSize + sizeof(FILE_FS_ATTRIBUTE_INFORMATION);
    }
    /* If user just wants max compo len or flags, allocate a dummy buffer */
    else if (lpMaximumComponentLength != NULL || lpFileSystemFlags != NULL)
    {
        VolumeAttrSize = MAX_PATH * sizeof(WCHAR) + sizeof(FILE_FS_ATTRIBUTE_INFORMATION);
    }
    else
    {
        VolumeAttrSize = 0;
    }

    /* If we're to query, allocate a big enough buffer */
    if (VolumeAttrSize != 0)
    {
        VolumeAttr = RtlAllocateHeap(RtlGetProcessHeap(), 0, VolumeAttrSize);
        if (VolumeAttr == NULL)
        {
            if (VolumeInfo != NULL)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeInfo);
            }

            NtClose(VolumeHandle);
            BaseSetLastNTError(STATUS_NO_MEMORY);
            return FALSE;
        }
    }

    /* Assume we'll fail */
    Ret = FALSE;

    /* If we're to query FileFsVolumeInformation, do it now! */
    if (VolumeInfo != NULL)
    {
        Status = NtQueryVolumeInformationFile(VolumeHandle, &IoStatusBlock, VolumeInfo, VolumeInfoSize, FileFsVolumeInformation);
        if (!NT_SUCCESS(Status))
        {
            BaseSetLastNTError(Status);
            goto CleanAndQuit;
        }
    }

    /* If we're to query FileFsAttributeInformation, do it now! */
    if (VolumeAttr != NULL)
    {
        Status = NtQueryVolumeInformationFile(VolumeHandle, &IoStatusBlock, VolumeAttr, VolumeAttrSize, FileFsAttributeInformation);
        if (!NT_SUCCESS(Status))
        {
            BaseSetLastNTError(Status);
            goto CleanAndQuit;
        }
    }

    /* If user wants volume name */
    if (lpVolumeNameBuffer != NULL)
    {
        /* Check its buffer can hold it (+ 0) */
        if (VolumeInfo->VolumeLabelLength >= nVolumeNameSize)
        {
            SetLastError(ERROR_BAD_LENGTH);
            goto CleanAndQuit;
        }

        /* Copy and zero */
        RtlCopyMemory(lpVolumeNameBuffer, VolumeInfo->VolumeLabel, VolumeInfo->VolumeLabelLength);
        lpVolumeNameBuffer[VolumeInfo->VolumeLabelLength / sizeof(WCHAR)] = UNICODE_NULL;
    }

    /* If user wants wants serial number, return it */
    if (lpVolumeSerialNumber != NULL)
    {
        *lpVolumeSerialNumber = VolumeInfo->VolumeSerialNumber;
    }

    /* If user wants filesystem name */
    if (lpFileSystemNameBuffer != NULL)
    {
        /* Check its buffer can hold it (+ 0) */
        if (VolumeAttr->FileSystemNameLength >= nFileSystemNameSize)
        {
            SetLastError(ERROR_BAD_LENGTH);
            goto CleanAndQuit;
        }

        /* Copy and zero */
        RtlCopyMemory(lpFileSystemNameBuffer, VolumeAttr->FileSystemName, VolumeAttr->FileSystemNameLength);
        lpFileSystemNameBuffer[VolumeAttr->FileSystemNameLength / sizeof(WCHAR)] = UNICODE_NULL;
    }

    /* If user wants wants max compo len, return it */
    if (lpMaximumComponentLength != NULL)
    {
        *lpMaximumComponentLength = VolumeAttr->MaximumComponentNameLength;
    }

    /* If user wants wants FS flags, return them */
    if (lpFileSystemFlags != NULL)
    {
        *lpFileSystemFlags = VolumeAttr->FileSystemAttributes;
    }

    /* We did it! */
    Ret = TRUE;

CleanAndQuit:
    NtClose(VolumeHandle);

    if (VolumeInfo != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeInfo);
    }

    if (VolumeAttr != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeAttr);
    }

    return Ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetVolumeLabelA(IN LPCSTR lpRootPathName,
                IN LPCSTR lpVolumeName OPTIONAL) /* NULL if deleting label */
{
	PWCHAR RootPathNameW;
   PWCHAR VolumeNameW = NULL;
	BOOL Result;

   if (!(RootPathNameW = FilenameA2W(lpRootPathName, FALSE)))
      return FALSE;

   if (lpVolumeName)
   {
      if (!(VolumeNameW = FilenameA2W(lpVolumeName, TRUE)))
         return FALSE;
   }

   Result = SetVolumeLabelW (RootPathNameW,
                             VolumeNameW);

   if (VolumeNameW)
   {
	   RtlFreeHeap (RtlGetProcessHeap (),
	                0,
                   VolumeNameW );
   }

	return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetVolumeLabelW(IN LPCWSTR lpRootPathName,
                IN LPCWSTR lpVolumeName OPTIONAL) /* NULL if deleting label */
{
   PFILE_FS_LABEL_INFORMATION LabelInfo;
   IO_STATUS_BLOCK IoStatusBlock;
   ULONG LabelLength;
   HANDLE hFile;
   NTSTATUS Status;

   LabelLength = wcslen(lpVolumeName) * sizeof(WCHAR);
   LabelInfo = RtlAllocateHeap(RtlGetProcessHeap(),
			       0,
			       sizeof(FILE_FS_LABEL_INFORMATION) +
			       LabelLength);
   if (LabelInfo == NULL)
   {
       SetLastError(ERROR_NOT_ENOUGH_MEMORY);
       return FALSE;
   }
   LabelInfo->VolumeLabelLength = LabelLength;
   memcpy(LabelInfo->VolumeLabel,
	  lpVolumeName,
	  LabelLength);

   hFile = InternalOpenDirW(lpRootPathName, TRUE);
   if (INVALID_HANDLE_VALUE == hFile)
   {
        RtlFreeHeap(RtlGetProcessHeap(),
	            0,
	            LabelInfo);
        return FALSE;
   }

   Status = NtSetVolumeInformationFile(hFile,
				       &IoStatusBlock,
				       LabelInfo,
				       sizeof(FILE_FS_LABEL_INFORMATION) +
				       LabelLength,
				       FileFsLabelInformation);

   RtlFreeHeap(RtlGetProcessHeap(),
	       0,
	       LabelInfo);

   if (!NT_SUCCESS(Status))
     {
	WARN("Status: %x\n", Status);
	CloseHandle(hFile);
	BaseSetLastNTError(Status);
	return FALSE;
     }

   CloseHandle(hFile);
   return TRUE;
}

/*
 * @implemented (Wine 13 sep 2008)
 */
HANDLE
WINAPI
FindFirstVolumeW(IN LPWSTR volume,
                 IN DWORD len)
{
    DWORD size = 1024;
    DWORD br;
    HANDLE mgr = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE );
    if (mgr == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

    for (;;)
    {
        MOUNTMGR_MOUNT_POINT input;
        MOUNTMGR_MOUNT_POINTS *output;

        if (!(output = RtlAllocateHeap( RtlGetProcessHeap(), 0, size )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            break;
        }
        memset( &input, 0, sizeof(input) );

        if (!DeviceIoControl( mgr, IOCTL_MOUNTMGR_QUERY_POINTS, &input, sizeof(input),
                              output, size, &br, NULL ))
        {
            if (GetLastError() != ERROR_MORE_DATA) break;
            size = output->Size;
            RtlFreeHeap( RtlGetProcessHeap(), 0, output );
            continue;
        }
        CloseHandle( mgr );
        /* abuse the Size field to store the current index */
        output->Size = 0;
        if (!FindNextVolumeW( output, volume, len ))
        {
            RtlFreeHeap( RtlGetProcessHeap(), 0, output );
            return INVALID_HANDLE_VALUE;
        }
        return (HANDLE)output;
    }
    CloseHandle( mgr );
    return INVALID_HANDLE_VALUE;
}

/*
 * @implemented (Wine 13 sep 2008)
 */
HANDLE
WINAPI
FindFirstVolumeA(IN LPSTR volume,
                 IN DWORD len)
{
    WCHAR *buffer = NULL;
    HANDLE handle;

    buffer = RtlAllocateHeap( RtlGetProcessHeap(), 0, len * sizeof(WCHAR) );

    if (!buffer)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    handle = FindFirstVolumeW( buffer, len );

    if (handle != INVALID_HANDLE_VALUE)
    {
        if (!WideCharToMultiByte( CP_ACP, 0, buffer, -1, volume, len, NULL, NULL ))
        {
            FindVolumeClose( handle );
            handle = INVALID_HANDLE_VALUE;
        }
    }
    RtlFreeHeap( RtlGetProcessHeap(), 0, buffer );
    return handle;
}

/*
 * @implemented (Wine 13 sep 2008)
 */
BOOL
WINAPI
FindVolumeClose(IN HANDLE hFindVolume)
{
    return RtlFreeHeap(RtlGetProcessHeap(), 0, hFindVolume);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetVolumePathNameA(IN LPCSTR lpszFileName,
                   IN LPSTR lpszVolumePathName,
                   IN DWORD cchBufferLength)
{
    PWCHAR FileNameW = NULL;
    WCHAR VolumePathName[MAX_PATH];
    BOOL Result;

    if (lpszFileName)
    {
        if (!(FileNameW = FilenameA2W(lpszFileName, FALSE)))
            return FALSE;
    }

    Result = GetVolumePathNameW(FileNameW, VolumePathName, cchBufferLength);

    if (Result)
        FilenameW2A_N(lpszVolumePathName, MAX_PATH, VolumePathName, -1);

    return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetVolumePathNameW(IN LPCWSTR lpszFileName,
                   IN LPWSTR lpszVolumePathName,
                   IN DWORD cchBufferLength)
{
    DWORD PathLength;
    UNICODE_STRING UnicodeFilePath;
    LPWSTR FilePart;
    PWSTR FullFilePath, FilePathName;
    ULONG PathSize;
    WCHAR VolumeName[MAX_PATH];
    DWORD ErrorCode;
    BOOL Result = FALSE;

    if (!lpszFileName || !lpszVolumePathName || !cchBufferLength)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if (!(PathLength = GetFullPathNameW(lpszFileName, 0, NULL, NULL)))
    {
        return Result;
    }
    else
    {
        PathLength = PathLength + 10;
        PathSize = PathLength * sizeof(WCHAR);

        if (!(FullFilePath = RtlAllocateHeap(RtlGetProcessHeap(), 0, PathSize)))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return Result;
        }

        if (!GetFullPathNameW(lpszFileName, PathLength, FullFilePath, &FilePart))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, FullFilePath);
            return Result;
        }

        RtlInitUnicodeString(&UnicodeFilePath, FullFilePath);

        if (UnicodeFilePath.Buffer[UnicodeFilePath.Length / sizeof(WCHAR) - 1] != '\\')
        {
            UnicodeFilePath.Length += sizeof(WCHAR);
            UnicodeFilePath.Buffer[UnicodeFilePath.Length / sizeof(WCHAR) - 1] = '\\';
            UnicodeFilePath.Buffer[UnicodeFilePath.Length / sizeof(WCHAR)] = '\0';
        }

        if (!(FilePathName = RtlAllocateHeap(RtlGetProcessHeap(), 0, PathSize)))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, FullFilePath);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return Result;
        }

        while (!GetVolumeNameForVolumeMountPointW(UnicodeFilePath.Buffer,
                                                  VolumeName,
                                                  MAX_PATH))
        {
            if (((UnicodeFilePath.Length == 4) && (UnicodeFilePath.Buffer[0] == '\\') &&
                (UnicodeFilePath.Buffer[1] == '\\')) || ((UnicodeFilePath.Length == 6) &&
                (UnicodeFilePath.Buffer[1] == ':')))
            {
                break;
            }

            UnicodeFilePath.Length -= sizeof(WCHAR);
            UnicodeFilePath.Buffer[UnicodeFilePath.Length / sizeof(WCHAR)] = '\0';

            memcpy(FilePathName, UnicodeFilePath.Buffer, UnicodeFilePath.Length);
            FilePathName[UnicodeFilePath.Length / sizeof(WCHAR)] = '\0';

            if (!GetFullPathNameW(FilePathName, PathLength, FullFilePath, &FilePart))
            {
                goto Cleanup2;
            }

            if (!FilePart)
            {
                RtlInitUnicodeString(&UnicodeFilePath, FullFilePath);
                UnicodeFilePath.Length += sizeof(WCHAR);
                UnicodeFilePath.Buffer[UnicodeFilePath.Length / sizeof(WCHAR) - 1] = '\\';
                UnicodeFilePath.Buffer[UnicodeFilePath.Length / sizeof(WCHAR)] = '\0';
                break;
            }

            FilePart[0] = '\0';
            RtlInitUnicodeString(&UnicodeFilePath, FullFilePath);
        }
    }

    if (UnicodeFilePath.Length > (cchBufferLength * sizeof(WCHAR)) - sizeof(WCHAR))
    {
        ErrorCode = ERROR_FILENAME_EXCED_RANGE;
        goto Cleanup1;
    }

    memcpy(lpszVolumePathName, UnicodeFilePath.Buffer, UnicodeFilePath.Length);
    lpszVolumePathName[UnicodeFilePath.Length / sizeof(WCHAR)] = '\0';

    Result = TRUE;
    goto Cleanup2;

Cleanup1:
    SetLastError(ErrorCode);
Cleanup2:
    RtlFreeHeap(RtlGetProcessHeap(), 0, FullFilePath);
    RtlFreeHeap(RtlGetProcessHeap(), 0, FilePathName);
    return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
FindNextVolumeA(IN HANDLE handle,
                IN LPSTR volume,
                IN DWORD len)
{
    WCHAR *buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, len * sizeof(WCHAR));
    BOOL ret;

    if (!buffer)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if ((ret = FindNextVolumeW( handle, buffer, len )))
    {
        if (!WideCharToMultiByte( CP_ACP, 0, buffer, -1, volume, len, NULL, NULL )) ret = FALSE;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, buffer);
    return ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
FindNextVolumeW(IN HANDLE handle,
                IN LPWSTR volume,
                IN DWORD len)
{
    MOUNTMGR_MOUNT_POINTS *data = handle;

    while (data->Size < data->NumberOfMountPoints)
    {
        static const WCHAR volumeW[] = {'\\','?','?','\\','V','o','l','u','m','e','{',};
        WCHAR *link = (WCHAR *)((char *)data + data->MountPoints[data->Size].SymbolicLinkNameOffset);
        DWORD size = data->MountPoints[data->Size].SymbolicLinkNameLength;
        data->Size++;
        /* skip non-volumes */
        if (size < sizeof(volumeW) || memcmp( link, volumeW, sizeof(volumeW) )) continue;
        if (size + sizeof(WCHAR) >= len * sizeof(WCHAR))
        {
            SetLastError( ERROR_FILENAME_EXCED_RANGE );
            return FALSE;
        }
        memcpy( volume, link, size );
        volume[1] = '\\';  /* map \??\ to \\?\ */
        volume[size / sizeof(WCHAR)] = '\\';  /* Windows appends a backslash */
        volume[size / sizeof(WCHAR) + 1] = 0;
        DPRINT( "returning entry %u %s\n", data->Size - 1, volume );
        return TRUE;
    }
    SetLastError( ERROR_NO_MORE_FILES );
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetVolumePathNamesForVolumeNameA(IN LPCSTR lpszVolumeName,
                                 IN LPSTR lpszVolumePathNames,
                                 IN DWORD cchBufferLength,
                                 OUT PDWORD lpcchReturnLength)
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetVolumePathNamesForVolumeNameW(IN LPCWSTR lpszVolumeName,
                                 IN LPWSTR lpszVolumePathNames,
                                 IN DWORD cchBufferLength,
                                 OUT PDWORD lpcchReturnLength)
{
    STUB;
    return 0;
}

/* EOF */
