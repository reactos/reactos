/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/filesup.c
 * PURPOSE:         File support functions
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

static
NTSTATUS
SetupCreateSingleDirectory(
    PWCHAR DirectoryName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING PathName;
    HANDLE DirectoryHandle;
    NTSTATUS Status;

    if (!RtlCreateUnicodeString(&PathName, DirectoryName))
        return STATUS_NO_MEMORY;

    if (PathName.Length > sizeof(WCHAR) &&
        PathName.Buffer[PathName.Length / sizeof(WCHAR) - 2] == L'\\' &&
        PathName.Buffer[PathName.Length / sizeof(WCHAR) - 1] == L'.')
    {
        PathName.Length -= sizeof(WCHAR);
        PathName.Buffer[PathName.Length / sizeof(WCHAR)] = 0;
    }

    if (PathName.Length > sizeof(WCHAR) &&
        PathName.Buffer[PathName.Length / sizeof(WCHAR) - 1] == L'\\')
    {
        PathName.Length -= sizeof(WCHAR);
        PathName.Buffer[PathName.Length / sizeof(WCHAR)] = 0;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &PathName,
                               OBJ_CASE_INSENSITIVE | OBJ_INHERIT,
                               NULL,
                               NULL);

    Status = NtCreateFile(&DirectoryHandle,
                          FILE_LIST_DIRECTORY | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_DIRECTORY,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          FILE_OPEN_FOR_BACKUP_INTENT | FILE_DIRECTORY_FILE,
                          NULL,
                          0);
    if (NT_SUCCESS(Status))
    {
        NtClose(DirectoryHandle);
    }

    RtlFreeUnicodeString(&PathName);

    return Status;
}

NTSTATUS
SetupCreateDirectory(
    PWCHAR PathName)
{
    PWCHAR PathBuffer = NULL;
    PWCHAR Ptr, EndPtr;
    ULONG BackslashCount;
    ULONG Size;
    NTSTATUS Status = STATUS_SUCCESS;

    Size = (wcslen(PathName) + 1) * sizeof(WCHAR);
    PathBuffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, Size);
    if (PathBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    wcscpy(PathBuffer, PathName);
    EndPtr = PathBuffer + wcslen(PathName);

    Ptr = PathBuffer;

    /* Skip the '\Device\HarddiskX\PartitionY\ part */
    BackslashCount = 0;
    while (Ptr < EndPtr && BackslashCount < 4)
    {
        if (*Ptr == L'\\')
            BackslashCount++;

        Ptr++;
    }

    while (Ptr < EndPtr)
    {
        if (*Ptr == L'\\')
        {
            *Ptr = 0;

            DPRINT("PathBuffer: %S\n", PathBuffer);
            if (!DoesDirExist(NULL, PathBuffer))
            {
                DPRINT("Create: %S\n", PathBuffer);
                Status = SetupCreateSingleDirectory(PathBuffer);
                if (!NT_SUCCESS(Status))
                    goto done;
            }

            *Ptr = L'\\';
        }

        Ptr++;
    }

    if (!DoesDirExist(NULL, PathBuffer))
    {
        DPRINT("Create: %S\n", PathBuffer);
        Status = SetupCreateSingleDirectory(PathBuffer);
        if (!NT_SUCCESS(Status))
            goto done;
    }

done:
    DPRINT("Done.\n");
    if (PathBuffer != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, PathBuffer);

    return Status;
}

BOOLEAN
IsValidPath(
    IN PCWSTR InstallDir)
{
    UINT i, Length;

    Length = wcslen(InstallDir);

    // TODO: Add check for 8.3 too.

    /* Path must be at least 2 characters long */
//    if (Length < 2)
//        return FALSE;

    /* Path must start with a backslash */
//    if (InstallDir[0] != L'\\')
//        return FALSE;

    /* Path must not end with a backslash */
    if (InstallDir[Length - 1] == L'\\')
        return FALSE;

    /* Path must not contain whitespace characters */
    for (i = 0; i < Length; i++)
    {
        if (iswspace(InstallDir[i]))
            return FALSE;
    }

    /* Path component must not end with a dot */
    for (i = 0; i < Length; i++)
    {
        if (InstallDir[i] == L'\\' && i > 0)
        {
            if (InstallDir[i - 1] == L'.')
                return FALSE;
        }
    }

    if (InstallDir[Length - 1] == L'.')
        return FALSE;

    return TRUE;
}

/* EOF */
