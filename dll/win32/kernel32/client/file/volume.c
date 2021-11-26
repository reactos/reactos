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
        FileSystemNameU.MaximumLength = sizeof(WCHAR) * (nFileSystemNameSize + 1);
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
BOOL
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

    /* Convert length to bytes */
    nVolumeNameSize *= sizeof(WCHAR);
    nFileSystemNameSize *= sizeof(WCHAR);

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
    BOOL Ret;
    UNICODE_STRING VolumeNameU;
    PUNICODE_STRING RootPathNameU;

    if (lpRootPathName == NULL)
    {
        lpRootPathName = "\\";
    }

    RootPathNameU = Basep8BitStringToStaticUnicodeString(lpRootPathName);
    if (RootPathNameU == NULL)
    {
        return FALSE;
    }

    if (lpVolumeName != NULL)
    {
        if (!Basep8BitStringToDynamicUnicodeString(&VolumeNameU, lpVolumeName))
        {
            return FALSE;
        }
    }
    else
    {
        VolumeNameU.Buffer = NULL;
    }

    Ret = SetVolumeLabelW(RootPathNameU->Buffer, VolumeNameU.Buffer);
    RtlFreeUnicodeString(&VolumeNameU);
    return Ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetVolumeLabelW(IN LPCWSTR lpRootPathName,
                IN LPCWSTR lpVolumeName OPTIONAL) /* NULL if deleting label */
{
    BOOL Ret;
    NTSTATUS Status;
    PWSTR VolumeRoot;
    HANDLE VolumeHandle;
    WCHAR VolumeGuid[MAX_PATH];
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PFILE_FS_LABEL_INFORMATION FsLabelInfo;
    UNICODE_STRING VolumeName, NtVolumeName;

    /* If no root path provided, default to \ */
    VolumeRoot = L"\\";

    /* If user wants to set a label, make it a string */
    if (lpVolumeName != NULL)
    {
        RtlInitUnicodeString(&VolumeName, lpVolumeName);
    }
    else
    {
        VolumeName.Length = 0;
        VolumeName.MaximumLength = 0;
        VolumeName.Buffer = NULL;
    }

    /* If we received a volume, try to get its GUID name */
    if (lpRootPathName != NULL)
    {
        Ret = GetVolumeNameForVolumeMountPointW(lpRootPathName, VolumeGuid, MAX_PATH);
    }
    else
    {
        Ret = FALSE;
    }

    /* If we got the GUID name, use it */
    if (Ret)
    {
        VolumeRoot = VolumeGuid;
    }
    else
    {
        /* Otherwise, use the name provided by the caller */
        if (lpRootPathName != NULL)
        {
            VolumeRoot = (PWSTR)lpRootPathName;
        }
    }

    /* Convert to a NT path */
    if (!RtlDosPathNameToNtPathName_U(VolumeRoot, &NtVolumeName, NULL, NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }


    /* Check we really end with a backslash */
    if (NtVolumeName.Buffer[(NtVolumeName.Length / sizeof(WCHAR)) - 1] != L'\\')
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtVolumeName.Buffer);
        BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
        return FALSE;
    }

    /* Try to open the root directory */
    InitializeObjectAttributes(&ObjectAttributes, &NtVolumeName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtOpenFile(&VolumeHandle, SYNCHRONIZE | FILE_WRITE_DATA,
                        &ObjectAttributes, &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtVolumeName.Buffer);
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Validate it's really a root path */
    if (!IsThisARootDirectory(VolumeHandle, NULL))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtVolumeName.Buffer);
        NtClose(VolumeHandle);
        SetLastError(ERROR_DIR_NOT_ROOT);
        return FALSE;
    }

    /* Done */
    NtClose(VolumeHandle);

    /* Now, open the volume to perform the label change */
    NtVolumeName.Length -= sizeof(WCHAR);
    InitializeObjectAttributes(&ObjectAttributes, &NtVolumeName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtOpenFile(&VolumeHandle, SYNCHRONIZE | FILE_WRITE_DATA,
                        &ObjectAttributes, &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);

    RtlFreeHeap(RtlGetProcessHeap(), 0, NtVolumeName.Buffer);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Assume success */
    Ret = TRUE;

    /* Allocate a buffer that can hold new label and its size */
    FsLabelInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(FILE_FS_LABEL_INFORMATION) + VolumeName.Length);
    if (FsLabelInfo != NULL)
    {
        /* Copy name and set its size */
        RtlCopyMemory(FsLabelInfo->VolumeLabel, VolumeName.Buffer, VolumeName.Length);
        FsLabelInfo->VolumeLabelLength = VolumeName.Length;

        /* And finally, set new label */
        Status = NtSetVolumeInformationFile(VolumeHandle, &IoStatusBlock, FsLabelInfo, sizeof(FILE_FS_LABEL_INFORMATION) + VolumeName.Length, FileFsLabelInformation);
    }
    else
    {
        /* Allocation failed */
        Status = STATUS_NO_MEMORY;
    }

    /* In case of failure, set status and mark failure */
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        Ret = FALSE;
    }

    /* We're done */
    NtClose(VolumeHandle);

    /* Free buffer if required */
    if (FsLabelInfo != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, FsLabelInfo);
    }

    return Ret;
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
    BOOL Ret;
    PUNICODE_STRING FileNameU;
    ANSI_STRING VolumePathName;
    UNICODE_STRING VolumePathNameU;

    /* Convert file name to unicode */
    FileNameU = Basep8BitStringToStaticUnicodeString(lpszFileName);
    if (FileNameU == NULL)
    {
        return FALSE;
    }

    /* Initialize all the strings we'll need */
    VolumePathName.Buffer = lpszVolumePathName;
    VolumePathName.Length = 0;
    VolumePathName.MaximumLength = cchBufferLength - 1;

    VolumePathNameU.Length = 0;
    VolumePathNameU.MaximumLength = (cchBufferLength - 1) * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    /* Allocate a buffer for calling the -W */
    VolumePathNameU.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, VolumePathNameU.MaximumLength);
    if (VolumePathNameU.Buffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Call the -W implementation */
    Ret = GetVolumePathNameW(FileNameU->Buffer, VolumePathNameU.Buffer, cchBufferLength);
    /* If it succeed */
    if (Ret)
    {
        NTSTATUS Status;

        /* Convert back to ANSI */
        RtlInitUnicodeString(&VolumePathNameU, VolumePathNameU.Buffer);
        Status = RtlUnicodeStringToAnsiString(&VolumePathName, &VolumePathNameU, FALSE);
        /* If conversion failed, just set error code and fail the rest */
        if (!NT_SUCCESS(Status))
        {
            BaseSetLastNTError(Status);
            Ret = FALSE;
        }
        /* Otherwise, null terminate the string (it's OK, we computed -1) */
        else
        {
            VolumePathName.Buffer[VolumePathName.Length] = ANSI_NULL;
        }
    }

    /* Free the buffer allocated for -W call */
    RtlFreeHeap(RtlGetProcessHeap(), 0, VolumePathNameU.Buffer);
    return Ret;
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
    BOOL MountPoint;
    DWORD FullPathLen;
    WCHAR OldFilePart;
    UNICODE_STRING FullPath;
    PWSTR FullPathBuf, FilePart, VolumeNameBuf;

    /* Probe for full path len */
    FullPathLen = GetFullPathNameW(lpszFileName, 0, NULL, NULL);
    if (FullPathLen == 0)
    {
        return FALSE;
    }

    /* Allocate a big enough buffer to receive it */
    FullPathBuf = RtlAllocateHeap(RtlGetProcessHeap(), 0, (FullPathLen + 10) * sizeof(WCHAR));
    if (FullPathBuf == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* And get full path name */
    if (GetFullPathNameW(lpszFileName, FullPathLen + 10, FullPathBuf, &FilePart) == 0)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, FullPathBuf);
        return FALSE;
    }

    /* Make a string out of it */
    RtlInitUnicodeString(&FullPath, FullPathBuf);
    /* We will finish our string with '\', for ease of the parsing after */
    if (FullPath.Buffer[(FullPath.Length / sizeof(WCHAR)) - 1] != L'\\')
    {
        FullPath.Length += sizeof(WCHAR);
        FullPath.Buffer[(FullPath.Length / sizeof(WCHAR)) - 1] = L'\\';
        FullPath.Buffer[FullPath.Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    /* Allocate a buffer big enough to receive our volume name */
    VolumeNameBuf = RtlAllocateHeap(RtlGetProcessHeap(), 0, 0x2000 * sizeof(WCHAR));
    if (VolumeNameBuf == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, FullPathBuf);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* We don't care about file part: we added an extra backslash, so there's no
     * file, we're back at the dir level.
     * We'll recompute file part afterwards
     */
    FilePart = NULL;
    /* Keep track of the letter we could drop to shorten the string */
    OldFilePart = UNICODE_NULL;
    /* As long as querying volume name fails, keep looping */
    while (!BasepGetVolumeNameForVolumeMountPoint(FullPath.Buffer, VolumeNameBuf, 0x2000u, &MountPoint))
    {
        USHORT LastSlash;

        /* Not a mount point, but opening returning access denied? Assume it's one, just not
         * a reparse backed one (classic mount point, a device)!
         */
        if (!MountPoint && GetLastError() == ERROR_ACCESS_DENIED)
        {
            MountPoint = TRUE;
        }

        /* BasepGetVolumeNameForVolumeMountPoint failed, but returned a volume name.
         * This can happen when we are given a reparse point where MountMgr could find associated
         * volume name which is not a valid DOS volume
         * A valid DOS name always starts with \\
         */
        if (VolumeNameBuf[0] != UNICODE_NULL && (FullPath.Buffer[0] != L'\\' || FullPath.Buffer[1] != L'\\'))
        {
            CHAR RootPathName[4];

            /* Construct a simple <letter>:\ string to get drive type */
            RootPathName[0] = FullPath.Buffer[0];
            RootPathName[1] = ':';
            RootPathName[2] = '\\';
            RootPathName[3] = ANSI_NULL;

            /* If we weren't given a drive letter actually, or if that's not a remote drive
             * Note: in this code path, we're recursive and stop fail loop
             */
            if (FullPath.Buffer[1] != L':' || GetDriveTypeA(RootPathName) != DRIVE_REMOTE)
            {
                BOOL Ret;

                /* We won't need the full path, we'll now work with the returned volume name */
                RtlFreeHeap(RtlGetProcessHeap(), 0, FullPathBuf);
                /* If it wasn't an NT name which was returned */
                if ((VolumeNameBuf[0] != L'\\') || (VolumeNameBuf[1] != L'?') ||
                    (VolumeNameBuf[2] != L'?') || (VolumeNameBuf[3] != L'\\'))
                {
                    PWSTR GlobalPath;
                    UNICODE_STRING GlobalRoot;

                    /* Create a new name in the NT namespace (from Win32) */
                    RtlInitUnicodeString(&FullPath, VolumeNameBuf);
                    RtlInitUnicodeString(&GlobalRoot, L"\\\\?\\GLOBALROOT");

                    /* We allocate a buffer than can contain both the namespace and the volume name */
                    GlobalPath = RtlAllocateHeap(RtlGetProcessHeap(), 0, FullPath.Length + GlobalRoot.Length);
                    if (GlobalPath == NULL)
                    {
                        RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeNameBuf);
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        return FALSE;
                    }

                    /* Fill in the new query name */
                    RtlCopyMemory(GlobalPath, GlobalRoot.Buffer, GlobalRoot.Length);
                    RtlCopyMemory((PVOID)((ULONG_PTR)GlobalPath + GlobalRoot.Length), FullPath.Buffer, FullPath.Length);
                    GlobalPath[(FullPath.Length + GlobalRoot.Length) / sizeof(WCHAR)] = UNICODE_NULL;

                    /* Give it another try */
                    Ret = GetVolumePathNameW(GlobalPath, lpszVolumePathName, cchBufferLength);

                    RtlFreeHeap(RtlGetProcessHeap(), 0, GlobalPath);
                }
                else
                {
                    /* If we don't have a drive letter in the Win32 name space \\.\<letter>: */
                    if ((VolumeNameBuf[4] != UNICODE_NULL) && (VolumeNameBuf[5] != L':'))
                    {
                        /* Shit our starting \\ */
                        RtlInitUnicodeString(&FullPath, VolumeNameBuf);
                        RtlMoveMemory(VolumeNameBuf, (PVOID)((ULONG_PTR)VolumeNameBuf + (2 * sizeof(WCHAR))), FullPath.Length - (3 * sizeof(WCHAR)));
                    }
                    /* Otherwise, just make sure we're double \ at the being to query again with the
                     * proper namespace
                     */
                    else
                    {
                        VolumeNameBuf[1] = L'\\';
                    }

                    /* Give it another try */
                    Ret = GetVolumePathNameW(VolumeNameBuf, lpszVolumePathName, cchBufferLength);
                }

                /* And done! */
                RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeNameBuf);
                return Ret;
            }
        }

        /* No mount point but with a file part? Restore filepart and exit */
        if (!MountPoint && FilePart != NULL)
        {
            FilePart[0] = OldFilePart;
            RtlInitUnicodeString(&FullPath, FullPathBuf);
            break;
        }

        /* We cannot go down the path any longer, too small */
        if (FullPath.Length <= sizeof(WCHAR))
        {
            break;
        }

        /* Prepare the next split */
        LastSlash = (FullPath.Length / sizeof(WCHAR)) - 2;
        if (FullPath.Length / sizeof(WCHAR) != 2)
        {
            do
            {
                if (FullPath.Buffer[LastSlash] == L'\\')
                {
                    break;
                }

                --LastSlash;
            } while (LastSlash != 0);
        }

        /* We couldn't split path, quit */
        if (LastSlash == 0)
        {
            break;
        }

        /* If that's a mount point, keep track of the directory name */
        if (MountPoint)
        {
            FilePart = &FullPath.Buffer[LastSlash + 1];
            OldFilePart = FilePart[0];
            /* And null terminate the string */
            FilePart[0] = UNICODE_NULL;
        }
        /* Otherwise, just null terminate the string */
        else
        {
            FullPath.Buffer[LastSlash + 1] = UNICODE_NULL;
        }

        /* We went down a bit in the path, fix the string and retry */
        RtlInitUnicodeString(&FullPath, FullPathBuf);
    }

    /* Once here, we'll return something from the full path buffer, so release
     * output buffer
     */
    RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeNameBuf);

    /* Not a mount point, bail out */
    if (!MountPoint && FilePart == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, FullPathBuf);
        return FALSE;
    }

    /* Make sure we have enough room to copy our volume */
    if ((cchBufferLength * sizeof(WCHAR)) < FullPath.Length + sizeof(UNICODE_NULL))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, FullPathBuf);
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return FALSE;
    }

    /* Copy and null terminate */
    RtlCopyMemory(lpszVolumePathName, FullPath.Buffer, FullPath.Length);
    lpszVolumePathName[FullPath.Length / sizeof(WCHAR)] = UNICODE_NULL;

    RtlFreeHeap(RtlGetProcessHeap(), 0, FullPathBuf);

    /* Done! */
    return TRUE;
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
 * @implemented
 */
BOOL
WINAPI
GetVolumePathNamesForVolumeNameA(IN LPCSTR lpszVolumeName,
                                 IN LPSTR lpszVolumePathNames,
                                 IN DWORD cchBufferLength,
                                 OUT PDWORD lpcchReturnLength)
{
    BOOL Ret;
    NTSTATUS Status;
    DWORD cchReturnLength;
    ANSI_STRING VolumePathName;
    PUNICODE_STRING VolumeNameU;
    UNICODE_STRING VolumePathNamesU;

    /* Convert volume name to unicode */
    VolumeNameU = Basep8BitStringToStaticUnicodeString(lpszVolumeName);
    if (VolumeNameU == NULL)
    {
        return FALSE;
    }

    /* Initialize the strings we'll use later on */
    VolumePathName.Length = 0;
    VolumePathName.MaximumLength = cchBufferLength;
    VolumePathName.Buffer = lpszVolumePathNames;

    VolumePathNamesU.Length = 0;
    VolumePathNamesU.MaximumLength = sizeof(WCHAR) * cchBufferLength;
    /* If caller provided a non 0 sized string, allocate a buffer for our unicode string */
    if (VolumePathNamesU.MaximumLength != 0)
    {
        VolumePathNamesU.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, VolumePathNamesU.MaximumLength);
        if (VolumePathNamesU.Buffer == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }
    else
    {
        VolumePathNamesU.Buffer = NULL;
    }

    /* Call the -W implementation */
    Ret = GetVolumePathNamesForVolumeNameW(VolumeNameU->Buffer, VolumePathNamesU.Buffer,
                                           cchBufferLength, &cchReturnLength);
    /* Call succeed, we'll return the total length */
    if (Ret)
    {
        VolumePathNamesU.Length = sizeof(WCHAR) * cchReturnLength;
    }
    else
    {
        /* Else, if we fail for anything else than too small buffer, quit */
        if (GetLastError() != ERROR_MORE_DATA)
        {
            if (VolumePathNamesU.Buffer != NULL)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, VolumePathNamesU.Buffer);
            }

            return FALSE;
        }

        /* Otherwise, we'll just copy as much as we can */
        VolumePathNamesU.Length = sizeof(WCHAR) * cchBufferLength;
    }

    /* Convert our output string back to ANSI */
    Status = RtlUnicodeStringToAnsiString(&VolumePathName, &VolumePathNamesU, FALSE);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);

        if (VolumePathNamesU.Buffer != NULL)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, VolumePathNamesU.Buffer);
        }

        return FALSE;
    }

    /* If caller wants return length, two cases... */
    if (lpcchReturnLength != NULL)
    {
        /* We succeed: return the copied length */
        if (Ret)
        {
            *lpcchReturnLength = VolumePathName.Length;
        }
        /* We failed, return the size we would have loved having! */
        else
        {
            *lpcchReturnLength = sizeof(WCHAR) * cchReturnLength;
        }
    }

    /* Release our buffer if allocated */
    if (VolumePathNamesU.Buffer != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, VolumePathNamesU.Buffer);
    }

    return Ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetVolumePathNamesForVolumeNameW(IN LPCWSTR lpszVolumeName,
                                 IN LPWSTR lpszVolumePathNames,
                                 IN DWORD cchBufferLength,
                                 OUT PDWORD lpcchReturnLength)
{
    BOOL Ret;
    PWSTR MultiSz;
    DWORD BytesReturned;
    HANDLE MountMgrHandle;
    UNICODE_STRING VolumeName;
    PMOUNTMGR_TARGET_NAME TargetName;
    PMOUNTMGR_VOLUME_PATHS VolumePaths;
    ULONG BufferSize, CharsInMgr, CharsInOutput, Paths;

    /* First look that our volume name looks somehow correct */
    RtlInitUnicodeString(&VolumeName, lpszVolumeName);
    if (VolumeName.Buffer[(VolumeName.Length / sizeof(WCHAR)) - 1] != L'\\')
    {
        BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
        return FALSE;
    }

    /* Validate it's a DOS volume name finishing with a backslash */
    if (!MOUNTMGR_IS_DOS_VOLUME_NAME_WB(&VolumeName))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Allocate an input MOUNTMGR_TARGET_NAME */
    TargetName = RtlAllocateHeap(RtlGetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR) + sizeof(USHORT));
    if (TargetName == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* And fill it */
    RtlZeroMemory(TargetName, MAX_PATH * sizeof(WCHAR) + sizeof(USHORT));
    TargetName->DeviceNameLength = VolumeName.Length - sizeof(WCHAR);
    RtlCopyMemory(TargetName->DeviceName, VolumeName.Buffer, TargetName->DeviceNameLength);
    TargetName->DeviceName[1] = L'?';

    /* Open the mount manager */
    MountMgrHandle = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, 0,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                 INVALID_HANDLE_VALUE);
    if (MountMgrHandle == INVALID_HANDLE_VALUE)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, TargetName);
        return FALSE;
    }

    /* Allocate an initial output buffer, just to get length */
    VolumePaths = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(MOUNTMGR_VOLUME_PATHS));
    if (VolumePaths == NULL)
    {
        CloseHandle(MountMgrHandle);
        RtlFreeHeap(RtlGetProcessHeap(), 0, TargetName);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Query the paths */
    Ret = DeviceIoControl(MountMgrHandle, IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS,
                          TargetName, MAX_PATH * sizeof(WCHAR) + sizeof(USHORT),
                          VolumePaths, sizeof(MOUNTMGR_VOLUME_PATHS), &BytesReturned,
                          NULL);
    /* Loop until we can query everything */
    while (!Ret)
    {
        /* If failed for another reason than too small buffer, fail */
        if (GetLastError() != ERROR_MORE_DATA)
        {
            CloseHandle(MountMgrHandle);
            RtlFreeHeap(RtlGetProcessHeap(), 0, TargetName);
            RtlFreeHeap(RtlGetProcessHeap(), 0, VolumePaths);
            return FALSE;
        }

        /* Get the required length */
        BufferSize = VolumePaths->MultiSzLength + sizeof(MOUNTMGR_VOLUME_PATHS);

        /* And reallocate our output buffer (big enough this time) */
        RtlFreeHeap(RtlGetProcessHeap(), 0, VolumePaths);
        VolumePaths = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferSize);
        if (VolumePaths == NULL)
        {
            CloseHandle(MountMgrHandle);
            RtlFreeHeap(RtlGetProcessHeap(), 0, TargetName);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        /* Query again the mount mgr */
        Ret = DeviceIoControl(MountMgrHandle, IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS,
                              TargetName, MAX_PATH * sizeof(WCHAR) + sizeof(USHORT),
                              VolumePaths, BufferSize, &BytesReturned, NULL);
    }

    /* We're done, no need for input nor mount mgr any longer */
    CloseHandle(MountMgrHandle);
    RtlFreeHeap(RtlGetProcessHeap(), 0, TargetName);

    /* Initialize:
     - Number of paths we saw (useful to count extra \)
     - Progress in mount mgr output
     - Progress in output buffer
     - Direct buffer to returned MultiSz
    */
    Paths = 0;
    CharsInMgr = 0;
    CharsInOutput = 0;
    MultiSz = VolumePaths->MultiSz;

    /* If we have an output buffer */
    if (cchBufferLength != 0)
    {
        /* Loop on the output to recopy it back to the caller
         * Note that we loop until -1 not to handle last 0 (will be done later on)
         */
        for (; (CharsInMgr < VolumePaths->MultiSzLength / sizeof(WCHAR) - 1) && (CharsInOutput < cchBufferLength);
             ++CharsInMgr, ++CharsInOutput)
        {
            /* When we reach the end of a path */
            if (MultiSz[CharsInMgr] == UNICODE_NULL)
            {
                /* On path done (count), add an extra \ at the end */
                ++Paths;
                lpszVolumePathNames[CharsInOutput] = L'\\';
                ++CharsInOutput;
                /* Make sure we don't overflow */
                if (CharsInOutput == cchBufferLength)
                {
                    break;
                }
            }

            /* Copy the char to the caller
             * So, in case we're in the end of a path, we wrote two chars to
             * the output buffer: \\ and \0
             */
            lpszVolumePathNames[CharsInOutput] = MultiSz[CharsInMgr];
        }
    }

    /* If output buffer was too small (ie, we couldn't parse all the input buffer) */
    if (CharsInMgr < VolumePaths->MultiSzLength / sizeof(WCHAR) - 1)
    {
        /* Keep looping on it, to count the number of extra \ that will be required
         * So that on the next call, caller can allocate enough space
         */
        for (; CharsInMgr < VolumePaths->MultiSzLength / sizeof(WCHAR) - 1; ++CharsInMgr)
        {
            if (MultiSz[CharsInMgr] == UNICODE_NULL)
            {
                ++Paths;
            }
        }
    }

    /* If we couldn't write as much as we wanted to the output buffer
     * This handles the case where we could write everything excepted the
     * terminating \0 for multi SZ
     */
    if (CharsInOutput >= cchBufferLength)
    {
        /* Fail and set appropriate error code */
        Ret = FALSE;
        SetLastError(ERROR_MORE_DATA);
        /* If caller wants to know how many chars to allocate, return it */
        if (lpcchReturnLength != NULL)
        {
            /* It's amount of extra \ + number of chars in MultiSz (including double \0) */
            *lpcchReturnLength = Paths + (VolumePaths->MultiSzLength / sizeof(WCHAR));
        }
    }
    else
    {
        /* It succeed so terminate the multi SZ (second \0) */
        lpszVolumePathNames[CharsInOutput] = UNICODE_NULL;
        Ret = TRUE;

        /* If caller wants the amount of chars written, return it */
        if (lpcchReturnLength != NULL)
        {
            /* Including the terminating \0 we just added */
            *lpcchReturnLength = CharsInOutput + 1;
        }
    }

    /* Free last bits */
    RtlFreeHeap(RtlGetProcessHeap(), 0, VolumePaths);

    /* And return */
    return Ret;
}

/* EOF */
