/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/drivesup.c
 * PURPOSE:         Drive support functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS
GetSourcePaths(
    OUT PUNICODE_STRING SourcePath,
    OUT PUNICODE_STRING SourceRootPath,
    OUT PUNICODE_STRING SourceRootDir)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING LinkName = RTL_CONSTANT_STRING(L"\\SystemRoot");
    UNICODE_STRING SourceName;
    WCHAR SourceBuffer[MAX_PATH] = L"";
    HANDLE Handle;
    ULONG Length;
    PWCHAR Ptr;

    InitializeObjectAttributes(&ObjectAttributes,
                               &LinkName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenSymbolicLinkObject(&Handle,
                                      SYMBOLIC_LINK_ALL_ACCESS,
                                      &ObjectAttributes);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlInitEmptyUnicodeString(&SourceName, SourceBuffer, sizeof(SourceBuffer));

    Status = NtQuerySymbolicLinkObject(Handle,
                                       &SourceName,
                                       &Length);
    NtClose(Handle);

    if (!NT_SUCCESS(Status))
        return Status;

    RtlCreateUnicodeString(SourcePath,
                           SourceName.Buffer);

    /* Strip trailing directory */
    Ptr = wcsrchr(SourceName.Buffer, OBJ_NAME_PATH_SEPARATOR);
    if (Ptr)
    {
        RtlCreateUnicodeString(SourceRootDir, Ptr);
        *Ptr = UNICODE_NULL;
    }
    else
    {
        RtlCreateUnicodeString(SourceRootDir, L"");
    }

    RtlCreateUnicodeString(SourceRootPath,
                           SourceName.Buffer);

    return STATUS_SUCCESS;
}

/* EOF */
