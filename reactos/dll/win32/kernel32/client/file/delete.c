/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/delete.c
 * PURPOSE:         Deleting files
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <k32.h>
#define NDEBUG
#include <reactos/debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
DeleteFileA(IN LPCSTR lpFileName)
{
    PUNICODE_STRING FileName;

    /* Convert the string to unicode, and call the wide function */
    FileName = Basep8BitStringToStaticUnicodeString(lpFileName);
    if (FileName) return DeleteFileW(FileName->Buffer);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DeleteFileW(IN LPCWSTR lpFileName)
{
    FILE_DISPOSITION_INFORMATION FileDispInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING NtPathU;
    HANDLE FileHandle;
    NTSTATUS Status;
    RTL_RELATIVE_NAME_U RelativeName;
    PWCHAR PathBuffer;
    FILE_ATTRIBUTE_TAG_INFORMATION FileTagInformation;

    /* Convert to NT path and get the relative name too */
    if (!RtlDosPathNameToNtPathName_U(lpFileName,
                                      &NtPathU,
                                      NULL,
                                      &RelativeName))
    {
        /* Bail out if the path name makes no sense */
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Save the path buffer in case we free it later */
    PathBuffer = NtPathU.Buffer;

    /* If we have a relative name... */
    if (RelativeName.RelativeName.Length)
    {
        /* Do a relative open with only the relative path set */
        NtPathU = RelativeName.RelativeName;
    }
    else
    {
        /* Do a full path open with no containing directory */
        RelativeName.ContainingDirectory = NULL;
    }

    /* Now open the directory name that was passed in */
    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathU,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.ContainingDirectory,
                               NULL);
    Status = NtOpenFile(&FileHandle,
                        DELETE | FILE_READ_ATTRIBUTES,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_NON_DIRECTORY_FILE |
                        FILE_OPEN_FOR_BACKUP_INTENT |
                        FILE_OPEN_REPARSE_POINT);
    if (NT_SUCCESS(Status))
    {
        /* Check if there's a reparse point associated with this file handle */
        Status = NtQueryInformationFile(FileHandle,
                                        &IoStatusBlock,
                                        &FileTagInformation,
                                        sizeof(FileTagInformation),
                                        FileAttributeTagInformation);
        if ((NT_SUCCESS(Status)) &&
            (FileTagInformation.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
            (FileTagInformation.ReparseTag != IO_REPARSE_TAG_MOUNT_POINT))
        {
            /* There is, so now try to open it with reparse behavior */
            NtClose(FileHandle);
            Status = NtOpenFile(&FileHandle,
                                DELETE,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                FILE_SHARE_DELETE |
                                FILE_SHARE_READ |
                                FILE_SHARE_WRITE,
                                FILE_NON_DIRECTORY_FILE |
                                FILE_OPEN_FOR_BACKUP_INTENT);
            if (!NT_SUCCESS(Status))
            {
                /* We failed -- maybe whoever is handling this tag isn't there */
                if (Status == STATUS_IO_REPARSE_TAG_NOT_HANDLED)
                {
                    /* Try to open it for delete, without reparse behavior */
                    Status = NtOpenFile(&FileHandle,
                                        DELETE,
                                        &ObjectAttributes,
                                        &IoStatusBlock,
                                        FILE_SHARE_READ |
                                        FILE_SHARE_WRITE |
                                        FILE_SHARE_DELETE,
                                        FILE_NON_DIRECTORY_FILE |
                                        FILE_OPEN_FOR_BACKUP_INTENT |
                                        FILE_OPEN_REPARSE_POINT);
                }

                if (!NT_SUCCESS(Status))
                {
                     RtlReleaseRelativeName(&RelativeName);
                     RtlFreeHeap(RtlGetProcessHeap(), 0, PathBuffer);
                     BaseSetLastNTError(Status);
                     return FALSE;
                }
            }
        }
        else if (!(NT_SUCCESS(Status)) &&
                 (Status != STATUS_NOT_IMPLEMENTED) &&
                 (Status != STATUS_INVALID_PARAMETER))
        {
            /* We had some critical error querying the attributes, bail out */
            RtlReleaseRelativeName(&RelativeName);
            RtlFreeHeap(RtlGetProcessHeap(), 0, PathBuffer);
            NtClose(FileHandle);
            BaseSetLastNTError(Status);
            return FALSE;
        }
    }
    else
    {
        /* It's possible that FILE_OPEN_REPARSE_POINT was not understood */
        if (Status == STATUS_INVALID_PARAMETER)
        {
            /* Try opening the file normally, with reparse behavior */
            Status = NtOpenFile(&FileHandle,
                                DELETE,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                FILE_SHARE_DELETE |
                                FILE_SHARE_READ |
                                FILE_SHARE_WRITE,
                                FILE_NON_DIRECTORY_FILE |
                                FILE_OPEN_FOR_BACKUP_INTENT);
            if (!NT_SUCCESS(Status))
            {
                /* This failed too, fail */
                RtlReleaseRelativeName(&RelativeName);
                RtlFreeHeap(RtlGetProcessHeap(), 0, PathBuffer);
                BaseSetLastNTError(Status);
                return FALSE;
            }
        }
        else
        {
            /* Maybe we didn't have READ_ATTRIBUTE rights? */
            if (Status != STATUS_ACCESS_DENIED)
            {
                /* Nope, it was something else, let's fail */
                RtlReleaseRelativeName(&RelativeName);
                RtlFreeHeap(RtlGetProcessHeap(), 0, PathBuffer);
                BaseSetLastNTError(Status);
                return FALSE;
            }

            /* Let's try again, without querying attributes */
            Status = NtOpenFile(&FileHandle,
                                DELETE,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                FILE_SHARE_DELETE |
                                FILE_SHARE_READ |
                                FILE_SHARE_WRITE,
                                FILE_NON_DIRECTORY_FILE |
                                FILE_OPEN_FOR_BACKUP_INTENT |
                                FILE_OPEN_REPARSE_POINT);
            if (!NT_SUCCESS(Status))
            {
                /* This failed too, so bail out */
                RtlReleaseRelativeName(&RelativeName);
                RtlFreeHeap(RtlGetProcessHeap(), 0, PathBuffer);
                BaseSetLastNTError(Status);
                return FALSE;
            }
        }
    }

    /* Ready to delete the file, so cleanup temporary data */
    RtlReleaseRelativeName(&RelativeName);
    RtlFreeHeap(RtlGetProcessHeap(), 0, PathBuffer);

    /* Ask for the file to be deleted */
    FileDispInfo.DeleteFile = TRUE;
    Status = NtSetInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileDispInfo,
                                  sizeof(FILE_DISPOSITION_INFORMATION),
                                  FileDispositionInformation);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        /* Deletion failed, tell the caller */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Tell the caller deletion worked */
    return TRUE;
}

/* EOF */
