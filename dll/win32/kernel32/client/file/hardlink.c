/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/hardlink.c
 * PURPOSE:         Hardlink functions
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                  Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
CreateHardLinkW(IN LPCWSTR lpFileName,
                IN LPCWSTR lpExistingFileName,
                IN LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    NTSTATUS Status;
    BOOL Ret = FALSE;
    ULONG NeededSize;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hTarget = INVALID_HANDLE_VALUE;
    PFILE_LINK_INFORMATION LinkInformation = NULL;
    UNICODE_STRING LinkTarget, LinkName;

    /* Initialize */
    LinkTarget.Buffer = LinkName.Buffer = NULL;

    /* Validate parameters */
    if (!(lpFileName) || !(lpExistingFileName))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    _SEH2_TRY
    {
        /* Get target UNC path */
        if (!RtlDosPathNameToNtPathName_U(lpExistingFileName,
                                          &LinkTarget,
                                          NULL,
                                          NULL))
        {
            /* Set the error and fail */
            SetLastError(ERROR_PATH_NOT_FOUND);
            _SEH2_LEAVE;
        }

        /* Open target */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &LinkTarget,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   lpSecurityAttributes ?
                                   lpSecurityAttributes->lpSecurityDescriptor :
                                   NULL);
        Status = NtOpenFile(&hTarget,
                            SYNCHRONIZE | FILE_WRITE_ATTRIBUTES,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT);
        if (!NT_SUCCESS(Status))
        {
            /* Convert the error and fail */
            BaseSetLastNTError(Status);
            _SEH2_LEAVE;
        }

        /* Get UNC path name for link */
        if (!RtlDosPathNameToNtPathName_U(lpFileName, &LinkName, NULL, NULL))
        {
            /* Set the error and fail */
            SetLastError(ERROR_PATH_NOT_FOUND);
            _SEH2_LEAVE;
        }

        /* Allocate data for link */
        NeededSize = sizeof(FILE_LINK_INFORMATION) + LinkName.Length;
        LinkInformation = RtlAllocateHeap(RtlGetProcessHeap(), 0, NeededSize);
        if (!LinkInformation)
        {
            /* We're out of memory */
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            _SEH2_LEAVE;
        }

        /* Setup data for link and create it */
        RtlMoveMemory(LinkInformation->FileName, LinkName.Buffer, LinkName.Length);
        LinkInformation->ReplaceIfExists = FALSE;
        LinkInformation->RootDirectory = 0;
        LinkInformation->FileNameLength = LinkName.Length;
        Status = NtSetInformationFile(hTarget,
                                      &IoStatusBlock,
                                      LinkInformation,
                                      NeededSize,
                                      FileLinkInformation);
        if (NT_SUCCESS(Status))
        {
            /* Set success code */
            Ret = TRUE;
        }
        else
        {
            /* Convert error code and return default (FALSE) */
            BaseSetLastNTError(Status);
        }
    }
    _SEH2_FINALLY
    {
        /* Cleanup all allocations */
        if (LinkTarget.Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, LinkTarget.Buffer);
        if (hTarget != INVALID_HANDLE_VALUE) NtClose(hTarget);
        if (LinkName.Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, LinkName.Buffer);
        if (LinkInformation) RtlFreeHeap(RtlGetProcessHeap(), 0, LinkInformation);
    }
    _SEH2_END;
    return Ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
CreateHardLinkA(IN LPCSTR lpFileName,
                IN LPCSTR lpExistingFileName,
                IN LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    BOOL Ret;
    PUNICODE_STRING lpFileNameW;
    UNICODE_STRING ExistingFileNameW;

    /* Convert the filename to unicode, using MAX_PATH limitations */
    lpFileNameW = Basep8BitStringToStaticUnicodeString(lpFileName);
    if (!lpFileNameW) return FALSE;

    /* Check if there's an existing name as well */
    if (lpExistingFileName)
    {
        /* We're already using the static string above, so do this dynamically */
        if (!Basep8BitStringToDynamicUnicodeString(&ExistingFileNameW,
                                                   lpExistingFileName))
        {
            /* Out of memory -- fail */
            return FALSE;
        }
    }
    else
    {
        /* No existing file name */
        ExistingFileNameW.Buffer = NULL;
    }

    /* Call the Wide function, and then free our dynamic string */
    Ret = CreateHardLinkW(lpFileNameW->Buffer,
                          ExistingFileNameW.Buffer,
                          lpSecurityAttributes);
    RtlFreeUnicodeString(&ExistingFileNameW);
    return Ret;
}

/* EOF */
