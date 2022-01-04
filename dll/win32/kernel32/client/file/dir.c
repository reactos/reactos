/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/dir.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>

/* Short File Name length in chars (8.3) */
#define SFN_LENGTH 12

/* Match a volume name like:
 * \\?\Volume{GUID}
 */
#define IS_VOLUME_NAME(s, l)                       \
  ((l == 96 || (l == 98 && s[48] == '\\')) &&      \
   s[0] == '\\'&& (s[1] == '?' || s[1] == '\\') && \
   s[2] == '?' && s[3] == '\\' && s[4] == 'V' &&   \
   s[5] == 'o' && s[6] == 'l' && s[7] == 'u' &&    \
   s[8] == 'm' && s[9] == 'e' && s[10] == '{' &&   \
   s[19] == '-' && s[24] == '-' && s[29] == '-' && \
   s[34] == '-' && s[47] == '}')

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
CreateDirectoryA(IN LPCSTR lpPathName,
                 IN LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    PUNICODE_STRING PathNameW;

    PathNameW = Basep8BitStringToStaticUnicodeString(lpPathName);
    if (!PathNameW)
    {
        return FALSE;
    }

    return CreateDirectoryW(PathNameW->Buffer,
                            lpSecurityAttributes);
}

/*
 * @implemented
 */
BOOL
WINAPI
CreateDirectoryExA(IN LPCSTR lpTemplateDirectory,
                   IN LPCSTR lpNewDirectory,
                   IN LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    PUNICODE_STRING TemplateDirectoryW;
    UNICODE_STRING NewDirectoryW;
    BOOL ret;

    TemplateDirectoryW = Basep8BitStringToStaticUnicodeString(lpTemplateDirectory);
    if (!TemplateDirectoryW)
    {
        return FALSE;
    }

    if (!Basep8BitStringToDynamicUnicodeString(&NewDirectoryW, lpNewDirectory))
    {
        return FALSE;
    }

    ret = CreateDirectoryExW(TemplateDirectoryW->Buffer,
                             NewDirectoryW.Buffer,
                             lpSecurityAttributes);

    RtlFreeUnicodeString(&NewDirectoryW);

    return ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
CreateDirectoryW(IN LPCWSTR lpPathName,
                 IN LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    DWORD Length;
    NTSTATUS Status;
    HANDLE DirectoryHandle;
    UNICODE_STRING NtPathU;
    PWSTR PathUBuffer, FilePart;
    IO_STATUS_BLOCK IoStatusBlock;
    RTL_RELATIVE_NAME_U RelativeName;
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* Get relative name */
    if (!RtlDosPathNameToRelativeNtPathName_U(lpPathName, &NtPathU, NULL, &RelativeName))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Check if path length is < MAX_PATH (with space for file name).
     * If not, prefix is required.
     */
    if (NtPathU.Length > (MAX_PATH - SFN_LENGTH) * sizeof(WCHAR) && lpPathName[0] != L'\\' &&
        lpPathName[1] != L'\\' && lpPathName[2] != L'?' && lpPathName[3] != L'\\')
    {
        /* Get file name position and full path length */
        Length = GetFullPathNameW(lpPathName, 0, NULL, &FilePart);
        if (Length == 0)
        {
            RtlReleaseRelativeName(&RelativeName);
            RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathU.Buffer);
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return FALSE;
        }

        /* Keep place for 8.3 file name */
        Length += SFN_LENGTH;
        /* No prefix, so, must be smaller than MAX_PATH */
        if (Length > MAX_PATH)
        {
            RtlReleaseRelativeName(&RelativeName);
            RtlFreeHeap(GetProcessHeap(), 0, NtPathU.Buffer);
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return FALSE;
        }
    }

    /* Save buffer to allow later freeing */
    PathUBuffer = NtPathU.Buffer;

    /* If we have relative name (and root dir), use them instead */
    if (RelativeName.RelativeName.Length != 0)
    {
        NtPathU.Length = RelativeName.RelativeName.Length;
        NtPathU.MaximumLength = RelativeName.RelativeName.MaximumLength;
        NtPathU.Buffer = RelativeName.RelativeName.Buffer;
    }
    else
    {
        RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathU,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.ContainingDirectory,
                               (lpSecurityAttributes ? lpSecurityAttributes->lpSecurityDescriptor : NULL));

    Status = NtCreateFile(&DirectoryHandle,
                          FILE_LIST_DIRECTORY | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_CREATE,
                          FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                          NULL,
                          0);

    RtlReleaseRelativeName(&RelativeName);
    RtlFreeHeap(RtlGetProcessHeap(), 0, PathUBuffer);

    if (NT_SUCCESS(Status))
    {
        NtClose(DirectoryHandle);
        return TRUE;
    }

    if (RtlIsDosDeviceName_U(lpPathName))
    {
        Status = STATUS_NOT_A_DIRECTORY;
    }

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
CreateDirectoryExW(IN LPCWSTR lpTemplateDirectory,
                   IN LPCWSTR lpNewDirectory,
                   IN LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    DWORD Length;
    NTSTATUS Status;
    PVOID EaBuffer = NULL;
    BOOL ReparsePoint = FALSE;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_EA_INFORMATION FileEaInfo;
    ULONG EaLength = 0, StreamSize;
    OBJECT_ATTRIBUTES ObjectAttributes;
    FILE_BASIC_INFORMATION FileBasicInfo;
    PREPARSE_DATA_BUFFER ReparseDataBuffer;
    HANDLE TemplateHandle, DirectoryHandle;
    PFILE_STREAM_INFORMATION FileStreamInfo;
    FILE_ATTRIBUTE_TAG_INFORMATION FileTagInfo;
    UNICODE_STRING NtPathU, NtTemplatePathU, NewDirectory;
    RTL_RELATIVE_NAME_U RelativeName, TemplateRelativeName;
    PWSTR TemplateBuffer, PathUBuffer, FilePart, SubstituteName;

    /* Get relative name of the template */
    if (!RtlDosPathNameToRelativeNtPathName_U(lpTemplateDirectory, &NtTemplatePathU, NULL, &TemplateRelativeName))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Save buffer for further freeing */
    TemplateBuffer = NtTemplatePathU.Buffer;

    /* If we have relative name (and root dir), use them instead */
    if (TemplateRelativeName.RelativeName.Length != 0)
    {
        NtTemplatePathU.Length = TemplateRelativeName.RelativeName.Length;
        NtTemplatePathU.MaximumLength = TemplateRelativeName.RelativeName.MaximumLength;
        NtTemplatePathU.Buffer = TemplateRelativeName.RelativeName.Buffer;
    }
    else
    {
        TemplateRelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &NtTemplatePathU,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open template directory */
    Status = NtOpenFile(&TemplateHandle,
                        FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | FILE_READ_EA,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_OPEN_REPARSE_POINT | FILE_OPEN_FOR_BACKUP_INTENT);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_INVALID_PARAMETER)
        {
            RtlReleaseRelativeName(&TemplateRelativeName);
            RtlFreeHeap(RtlGetProcessHeap(), 0, TemplateBuffer);
            BaseSetLastNTError(Status);
            return FALSE;
        }

OpenWithoutReparseSupport:
        /* Opening failed due to lacking reparse points support in the FSD, try without */
        Status = NtOpenFile(&TemplateHandle,
                            FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | FILE_READ_EA,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT);

        if (!NT_SUCCESS(Status))
        {
            RtlReleaseRelativeName(&TemplateRelativeName);
            RtlFreeHeap(RtlGetProcessHeap(), 0, TemplateBuffer);
            BaseSetLastNTError(Status);
            return FALSE;
        }

        /* Request file attributes */
        FileBasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
        Status = NtQueryInformationFile(TemplateHandle,
                                        &IoStatusBlock,
                                        &FileBasicInfo,
                                        sizeof(FileBasicInfo),
                                        FileBasicInformation);
        if (!NT_SUCCESS(Status))
        {
            RtlReleaseRelativeName(&TemplateRelativeName);
            RtlFreeHeap(RtlGetProcessHeap(), 0, TemplateBuffer);
            CloseHandle(TemplateHandle);
            BaseSetLastNTError(Status);
            return FALSE;

        }
    }
    else
    {
        /* Request file attributes */
        FileBasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
        Status = NtQueryInformationFile(TemplateHandle,
                                        &IoStatusBlock,
                                        &FileBasicInfo,
                                        sizeof(FileBasicInfo),
                                        FileBasicInformation);
        if (!NT_SUCCESS(Status))
        {
            RtlReleaseRelativeName(&TemplateRelativeName);
            RtlFreeHeap(RtlGetProcessHeap(), 0, TemplateBuffer);
            CloseHandle(TemplateHandle);
            BaseSetLastNTError(Status);
            return FALSE;

        }

        /* If it is a reparse point, then get information about it */
        if (FileBasicInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            Status = NtQueryInformationFile(TemplateHandle,
                                            &IoStatusBlock,
                                            &FileTagInfo,
                                            sizeof(FileTagInfo),
                                            FileAttributeTagInformation);
            if (!NT_SUCCESS(Status))
            {
                RtlReleaseRelativeName(&TemplateRelativeName);
                RtlFreeHeap(RtlGetProcessHeap(), 0, TemplateBuffer);
                CloseHandle(TemplateHandle);
                BaseSetLastNTError(Status);
                return FALSE;
            }

            /* Only mount points are supported, retry without if anything different */
            if (FileTagInfo.ReparseTag != IO_REPARSE_TAG_MOUNT_POINT)
            {
                CloseHandle(TemplateHandle);
                goto OpenWithoutReparseSupport;
            }

            /* Mark we are playing with a reparse point */
            ReparsePoint = TRUE;
        }
    }

    /* Get relative name of the directory */
    if (!RtlDosPathNameToRelativeNtPathName_U(lpNewDirectory, &NtPathU, NULL, &RelativeName))
    {
        RtlReleaseRelativeName(&TemplateRelativeName);
        RtlFreeHeap(RtlGetProcessHeap(), 0, TemplateBuffer);
        NtClose(TemplateHandle);
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Save its buffer for further freeing */
    PathUBuffer = NtPathU.Buffer;

    /* Template & directory can't be the same */
    if (RtlEqualUnicodeString(&NtPathU,
                              &NtTemplatePathU,
                              TRUE))
    {
        RtlReleaseRelativeName(&RelativeName);
        RtlReleaseRelativeName(&TemplateRelativeName);
        RtlFreeHeap(RtlGetProcessHeap(), 0, TemplateBuffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, PathUBuffer);
        NtClose(TemplateHandle);
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RtlReleaseRelativeName(&TemplateRelativeName);
    RtlFreeHeap(RtlGetProcessHeap(), 0, TemplateBuffer);

    /* Check if path length is < MAX_PATH (with space for file name).
     * If not, prefix is required.
     */
    if (NtPathU.Length > (MAX_PATH - SFN_LENGTH) * sizeof(WCHAR) && lpNewDirectory[0] != L'\\' &&
        lpNewDirectory[1] != L'\\' && lpNewDirectory[2] != L'?' && lpNewDirectory[3] != L'\\')
    {
        /* Get file name position and full path length */
        Length = GetFullPathNameW(lpNewDirectory, 0, NULL, &FilePart);
        if (Length == 0)
        {
            RtlReleaseRelativeName(&RelativeName);
            RtlFreeHeap(RtlGetProcessHeap(), 0, PathUBuffer);
            CloseHandle(TemplateHandle);
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return FALSE;
        }

        /* Keep place for 8.3 file name */
        Length += SFN_LENGTH;
        /* No prefix, so, must be smaller than MAX_PATH */
        if (Length > MAX_PATH)
        {
            RtlReleaseRelativeName(&RelativeName);
            RtlFreeHeap(RtlGetProcessHeap(), 0, PathUBuffer);
            CloseHandle(TemplateHandle);
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return FALSE;
        }
    }

    /* If we have relative name (and root dir), use them instead */
    if (RelativeName.RelativeName.Length != 0)
    {
        NtPathU.Length = RelativeName.RelativeName.Length;
        NtPathU.MaximumLength = RelativeName.RelativeName.MaximumLength;
        NtPathU.Buffer = RelativeName.RelativeName.Buffer;
    }
    else
    {
        RelativeName.ContainingDirectory = NULL;
    }

    /* Get extended attributes */
    Status = NtQueryInformationFile(TemplateHandle,
                                    &IoStatusBlock,
                                    &FileEaInfo,
                                    sizeof(FileEaInfo),
                                    FileEaInformation);
    if (!NT_SUCCESS(Status))
    {
        RtlReleaseRelativeName(&RelativeName);
        RtlFreeHeap(RtlGetProcessHeap(), 0, PathUBuffer);
        CloseHandle(TemplateHandle);
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Start reading extended attributes */
    if (FileEaInfo.EaSize != 0)
    {
        for (EaLength = FileEaInfo.EaSize * 2; ; EaLength = EaLength * 2)
        {
            /* Allocate buffer for reading */
            EaBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, EaLength);
            if (!EaBuffer)
            {
                RtlReleaseRelativeName(&RelativeName);
                RtlFreeHeap(RtlGetProcessHeap(), 0, PathUBuffer);
                CloseHandle(TemplateHandle);
                BaseSetLastNTError(STATUS_NO_MEMORY);
                return FALSE;
            }

            /* Query EAs */
            Status = NtQueryEaFile(TemplateHandle,
                                   &IoStatusBlock,
                                   EaBuffer,
                                   EaLength,
                                   FALSE,
                                   NULL,
                                   0,
                                   NULL,
                                   TRUE);
            if (!NT_SUCCESS(Status))
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, EaBuffer);
                IoStatusBlock.Information = 0;
            }

            /* If we don't fail because of too small buffer, stop here */
            if (Status != STATUS_BUFFER_OVERFLOW &&
                Status != STATUS_BUFFER_TOO_SMALL)
            {
                EaLength = IoStatusBlock.Information;
                break;
            }
        }
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathU,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.ContainingDirectory,
                               (lpSecurityAttributes ? lpSecurityAttributes->lpSecurityDescriptor : NULL));

    /* Ensure attributes are valid */
    FileBasicInfo.FileAttributes &= FILE_ATTRIBUTE_VALID_FLAGS;

    /* Create the new directory */
    Status = NtCreateFile(&DirectoryHandle,
                          FILE_LIST_DIRECTORY | SYNCHRONIZE | FILE_WRITE_ATTRIBUTES |
                          FILE_READ_ATTRIBUTES | (FileBasicInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ? FILE_ADD_FILE : 0),
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FileBasicInfo.FileAttributes,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_CREATE,
                          FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |
                          FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT,
                          EaBuffer,
                          EaLength);
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_INVALID_PARAMETER || Status == STATUS_ACCESS_DENIED)
        {
            /* If creation failed, it might be because FSD doesn't support reparse points
             * Retry without asking for such support in case template is not a reparse point
             */
            if (!ReparsePoint)
            {
                Status = NtCreateFile(&DirectoryHandle,
                                      FILE_LIST_DIRECTORY | SYNCHRONIZE |
                                      FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES,
                                      &ObjectAttributes,
                                      &IoStatusBlock,
                                      NULL,
                                      FileBasicInfo.FileAttributes,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      FILE_CREATE,
                                      FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |
                                      FILE_OPEN_FOR_BACKUP_INTENT,
                                      EaBuffer,
                                      EaLength);
            }
            else
            {
                RtlReleaseRelativeName(&RelativeName);
                RtlFreeHeap(RtlGetProcessHeap(), 0, PathUBuffer);
                if (EaBuffer)
                {
                    RtlFreeHeap(RtlGetProcessHeap(), 0, EaBuffer);
                }
                CloseHandle(TemplateHandle);
                BaseSetLastNTError(Status);
                return FALSE;
            }
        }
    }

    RtlReleaseRelativeName(&RelativeName);
    RtlFreeHeap(RtlGetProcessHeap(), 0, PathUBuffer);
    if (EaBuffer)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, EaBuffer);
    }

    if (!NT_SUCCESS(Status))
    {
        NtClose(TemplateHandle);
        if (RtlIsDosDeviceName_U(lpNewDirectory))
        {
            Status = STATUS_NOT_A_DIRECTORY;
        }
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* If template is a reparse point, copy reparse data */
    if (ReparsePoint)
    {
        ReparseDataBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0,
                                            MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
        if (!ReparseDataBuffer)
        {
            NtClose(TemplateHandle);
            NtClose(DirectoryHandle);
            SetLastError(STATUS_NO_MEMORY);
            return FALSE;
        }

        /* First query data */
        Status = NtFsControlFile(TemplateHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_GET_REPARSE_POINT,
                                 NULL,
                                 0,
                                 ReparseDataBuffer,
                                 MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
        if (!NT_SUCCESS(Status))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
            NtClose(TemplateHandle);
            NtClose(DirectoryHandle);
            SetLastError(Status);
            return FALSE;
        }

        /* Once again, ensure it is a mount point */
        if (ReparseDataBuffer->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
            NtClose(TemplateHandle);
            NtClose(DirectoryHandle);
            SetLastError(STATUS_OBJECT_NAME_INVALID);
            return FALSE;
        }

        /* Get volume name */
        SubstituteName = (PWSTR)((ULONG_PTR)ReparseDataBuffer->MountPointReparseBuffer.PathBuffer +
                                 ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameOffset);
        if (IS_VOLUME_NAME(SubstituteName, ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameLength))
        {
            /* Prepare to define a new mount point for that volume */
            RtlInitUnicodeString(&NewDirectory, lpNewDirectory);
            NewDirectory.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, NewDirectory.Length + 2 * sizeof(WCHAR));
            if (!NewDirectory.Buffer)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
                NtClose(TemplateHandle);
                NtClose(DirectoryHandle);
                return FALSE;
            }

            RtlCopyMemory(&NewDirectory.Buffer, lpNewDirectory, NewDirectory.Length);
            if (NewDirectory.Buffer[NewDirectory.Length / sizeof(WCHAR)] != L'\\')
            {
                NewDirectory.Buffer[NewDirectory.Length / sizeof(WCHAR)] = L'\\';
                NewDirectory.Buffer[(NewDirectory.Length / sizeof(WCHAR)) + 1] = UNICODE_NULL;
            }

            /* Define a new mount point for that volume */
            SetVolumeMountPointW(NewDirectory.Buffer, SubstituteName);

            RtlFreeHeap(RtlGetProcessHeap(), 0, NewDirectory.Buffer);
            RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
            NtClose(TemplateHandle);
            NtClose(DirectoryHandle);
            return TRUE;
        }

        /* Otherwise copy data raw */
        Status = NtFsControlFile(DirectoryHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_SET_REPARSE_POINT,
                                 ReparseDataBuffer,
                                 ReparseDataBuffer->ReparseDataLength +
                                 FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer),
                                 NULL,
                                 0);

        RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
        NtClose(TemplateHandle);
        NtClose(DirectoryHandle);

        if (NT_SUCCESS(Status))
        {
            return TRUE;
        }

        BaseSetLastNTError(Status);
        return FALSE;
    }
    /* In case it's not a reparse point, handle streams on the file */
    else
    {
        for (StreamSize = 0x1000; ; StreamSize = StreamSize * 2)
        {
            FileStreamInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, StreamSize);
            if (!FileStreamInfo)
            {
                BaseMarkFileForDelete(DirectoryHandle, FileBasicInfo.FileAttributes);
                SetLastError(STATUS_NO_MEMORY);
                break;
            }

            /* Query stream information */
            Status = NtQueryInformationFile(TemplateHandle,
                                            &IoStatusBlock,
                                            FileStreamInfo,
                                            StreamSize,
                                            FileStreamInformation);
            if (NT_SUCCESS(Status))
            {
                break;
            }

            RtlFreeHeap(RtlGetProcessHeap(), 0, FileStreamInfo);
            FileStreamInfo = NULL;

            /* If it failed, ensure that's not because of too small buffer */
            if (Status != STATUS_BUFFER_OVERFLOW &&
                Status != STATUS_BUFFER_TOO_SMALL)
            {
                break;
            }
        }

        if (!NT_SUCCESS(Status) || IoStatusBlock.Information == 0)
        {
            if (FileStreamInfo)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, FileStreamInfo);
            }

            NtClose(TemplateHandle);
            NtClose(DirectoryHandle);
            return TRUE;
        }

#if 1
        /* FIXME: TODO */
        DPRINT1("Warning: streams copying is unimplemented!\n");
        RtlFreeHeap(RtlGetProcessHeap(), 0, FileStreamInfo);
        NtClose(TemplateHandle);
        NtClose(DirectoryHandle);
#endif
        return TRUE;
    }
}

/*
 * @implemented
 */
BOOL
WINAPI
RemoveDirectoryA(IN LPCSTR lpPathName)
{
    PUNICODE_STRING PathNameW;

    PathNameW = Basep8BitStringToStaticUnicodeString(lpPathName);
    if (!PathNameW)
    {
        return FALSE;
    }

    return RemoveDirectoryW(PathNameW->Buffer);
}

/*
 * @implemented
 */
BOOL
WINAPI
RemoveDirectoryW(IN LPCWSTR lpPathName)
{
    NTSTATUS Status;
    DWORD BytesReturned;
    HANDLE DirectoryHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING NtPathU, PathName;
    RTL_RELATIVE_NAME_U RelativeName;
    PWSTR PathUBuffer, SubstituteName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PREPARSE_DATA_BUFFER ReparseDataBuffer;
    FILE_DISPOSITION_INFORMATION FileDispInfo;
    FILE_ATTRIBUTE_TAG_INFORMATION FileTagInfo;

    /* Get relative name */
    if (!RtlDosPathNameToRelativeNtPathName_U(lpPathName, &NtPathU, NULL, &RelativeName))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Save buffer to allow later freeing */
    PathUBuffer = NtPathU.Buffer;

    /* If we have relative name (and root dir), use them instead */
    if (RelativeName.RelativeName.Length != 0)
    {
        NtPathU.Length = RelativeName.RelativeName.Length;
        NtPathU.MaximumLength = RelativeName.RelativeName.MaximumLength;
        NtPathU.Buffer = RelativeName.RelativeName.Buffer;
    }
    else
    {
        RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathU,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.ContainingDirectory,
                               NULL);

    /* Try to open directory */
    Status = NtOpenFile(&DirectoryHandle,
                        DELETE | SYNCHRONIZE | FAILED_ACCESS_ACE_FLAG,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |
                        FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT);
    if (!NT_SUCCESS(Status))
    {
        /* We only accept failure for reparse points not being supported */
        if (Status != STATUS_INVALID_PARAMETER)
        {
            goto Cleanup;
        }

        /* Try to open, with reparse points support */
        Status = NtOpenFile(&DirectoryHandle,
                            DELETE | SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |
                            FILE_OPEN_FOR_BACKUP_INTENT);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        /* Success, mark directory */
        goto MarkFileForDelete;
    }

    /* Get information about file (and reparse point) */
    Status = NtQueryInformationFile(DirectoryHandle,
                                    &IoStatusBlock,
                                    &FileTagInfo,
                                    sizeof(FileTagInfo),
                                    FileAttributeTagInformation);
    if (!NT_SUCCESS(Status))
    {
        /* FSD might not support querying reparse points information */
        if (Status != STATUS_NOT_IMPLEMENTED &&
            Status != STATUS_INVALID_PARAMETER)
        {
            goto CleanupHandle;
        }

        /* If that's the case, then just delete directory */
        goto MarkFileForDelete;
    }

    /* If that's not a reparse point, nothing more to do than just delete */
    if (!(FileTagInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
    {
        goto MarkFileForDelete;
    }

    /* Check if that's a mount point */
    if (FileTagInfo.ReparseTag != IO_REPARSE_TAG_MOUNT_POINT)
    {
        /* It's not */
        NtClose(DirectoryHandle);

        /* So, try to reopen directory, ignoring mount point */
        Status = NtOpenFile(&DirectoryHandle,
                            DELETE | SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |
                            FILE_OPEN_FOR_BACKUP_INTENT);
        if (NT_SUCCESS(Status))
        {
            /* It succeed, we can safely delete directory (and ignore reparse point) */
            goto MarkFileForDelete;
        }

        /* If it failed, only allow case where IO mount point was ignored */
        if (Status != STATUS_IO_REPARSE_TAG_NOT_HANDLED)
        {
            goto Cleanup;
        }

        /* Reopen with reparse point support */
        Status = NtOpenFile(&DirectoryHandle,
                            DELETE | SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |
                            FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT);
        if (NT_SUCCESS(Status))
        {
            /* And mark for delete */
            goto MarkFileForDelete;
        }

        goto Cleanup;
    }

    /* Here, we have a mount point, prepare to query information about it */
    ReparseDataBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0,
                                        MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if (!ReparseDataBuffer)
    {
        RtlReleaseRelativeName(&RelativeName);
        RtlFreeHeap(RtlGetProcessHeap(), 0, PathUBuffer);
        NtClose(DirectoryHandle);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Query */
    if (!DeviceIoControl(DirectoryHandle,
                         FSCTL_GET_REPARSE_POINT,
                         NULL, 0,
                         ReparseDataBuffer,
                         MAXIMUM_REPARSE_DATA_BUFFER_SIZE,
                         &BytesReturned,
                         NULL))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
        goto MarkFileForDelete;
    }

    /* Get volume name */
    SubstituteName = (PWSTR)((ULONG_PTR)ReparseDataBuffer->MountPointReparseBuffer.PathBuffer +
                             ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameOffset);
    if (!IS_VOLUME_NAME(SubstituteName, ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameLength))
    {
        /* This is not a volume, we can safely delete */
        RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
        goto MarkFileForDelete;
    }

    /* Prepare to delete mount point */
    RtlInitUnicodeString(&PathName, lpPathName);
    PathName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, PathName.Length + 2 * sizeof(WCHAR));
    if (!PathName.Buffer)
    {
        RtlReleaseRelativeName(&RelativeName);
        RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);
        NtClose(DirectoryHandle);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    RtlCopyMemory(&PathName.Buffer, lpPathName, PathName.Length);
    if (PathName.Buffer[PathName.Length / sizeof(WCHAR)] != L'\\')
    {
        PathName.Buffer[PathName.Length / sizeof(WCHAR)] = L'\\';
        PathName.Buffer[(PathName.Length / sizeof(WCHAR)) + 1] = UNICODE_NULL;
    }

    /* Delete mount point for that volume */
    DeleteVolumeMountPointW(PathName.Buffer);
    RtlFreeHeap(RtlGetProcessHeap(), 0, PathName.Buffer);
    RtlFreeHeap(RtlGetProcessHeap(), 0, ReparseDataBuffer);

    /* And mark directory for delete */
MarkFileForDelete:
    RtlReleaseRelativeName(&RelativeName);
    RtlFreeHeap(RtlGetProcessHeap(), 0, PathUBuffer);

    /* Mark & set */
    FileDispInfo.DeleteFile = TRUE;
    Status = NtSetInformationFile(DirectoryHandle,
                                  &IoStatusBlock,
                                  &FileDispInfo,
                                  sizeof(FILE_DISPOSITION_INFORMATION),
                                  FileDispositionInformation);
    NtClose(DirectoryHandle);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;

CleanupHandle:
    NtClose(DirectoryHandle);

Cleanup:
    RtlReleaseRelativeName(&RelativeName);
    RtlFreeHeap(RtlGetProcessHeap(), 0, PathUBuffer);
    BaseSetLastNTError(Status);
    return FALSE;
}

/* EOF */
