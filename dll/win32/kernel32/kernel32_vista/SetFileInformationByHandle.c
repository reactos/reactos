/*
 * PROJECT:     ReactOS system libraries
 * FILE:        dll/win32/kernel32/kernel32_vista/SetFileInformationByHandle.c
 * PURPOSE:     Win32 wrapper around NtSetInformationFile
 * PROGRAMMERS: Mohammed Al-Houmi <mhmdlylyalhwmy461@gmail.com>
 *
 * NOTE:        kernel32 exports this API for Windows Vista and up. The
 *              translation below is the one from Wine's kernelbase/file.c.
 *              FILE_RENAME_INFO and the NT FILE_RENAME_INFORMATION have the
 *              same layout, so the caller's buffer is passed through as is.
 */

#include "k32_vista.h"

#include <ndk/rtlfuncs.h>
#include <ndk/iofuncs.h>
#include <string.h>

#define NDEBUG
#include <debug.h>

#undef FIXME
#define FIXME DPRINT1

/* Taken from Wine kernelbase/file.c */

/***********************************************************************
 *	SetFileInformationByHandle   (kernel32_vista.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFileInformationByHandle(HANDLE file,
                                                         FILE_INFO_BY_HANDLE_CLASS class,
                                                         LPVOID info,
                                                         DWORD size)
{
    NTSTATUS status;
    IO_STATUS_BLOCK io;

    switch (class)
    {
    /* These have no matching set operation in the kernel, or are only ever
     * used to query information, so there is nothing to pass along. */
    case FileNameInfo:
    case FileAllocationInfo:
    case FileStreamInfo:
    case FileIdBothDirectoryInfo:
    case FileIdBothDirectoryRestartInfo:
    case FileFullDirectoryInfo:
    case FileFullDirectoryRestartInfo:
    case FileStorageInfo:
    case FileAlignmentInfo:
    case FileIdInfo:
    case FileIdExtdDirectoryInfo:
    case FileIdExtdDirectoryRestartInfo:
    case FileDispositionInfoEx:
    case FileIoPriorityHintInfo:
        FIXME("(%p, %u, %p, %lu)\n", file, class, info, size);
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;

    case FileBasicInfo:
        status = NtSetInformationFile(file, &io, info, size, FileBasicInformation);
        break;

    case FileEndOfFileInfo:
        status = NtSetInformationFile(file, &io, info, size, FileEndOfFileInformation);
        break;

    case FileDispositionInfo:
        status = NtSetInformationFile(file, &io, info, size, FileDispositionInformation);
        break;

    case FileRenameInfo:
        {
            FILE_RENAME_INFO *source = info;
            FILE_RENAME_INFO *rename_info;
            UNICODE_STRING nt_name;
            ULONG length;

            status = RtlDosPathNameToNtPathName_U(source->FileName, &nt_name, NULL, NULL) ?
                     STATUS_SUCCESS : STATUS_OBJECT_PATH_NOT_FOUND;
            if (!NT_SUCCESS(status))
                break;

            /* The NT path is longer than the Win32 one the caller provided,
             * so it needs a buffer of its own. */
            length = FIELD_OFFSET(FILE_RENAME_INFO, FileName) +
                     nt_name.Length + sizeof(WCHAR);
            rename_info = HeapAlloc(GetProcessHeap(), 0, length);
            if (rename_info)
            {
                rename_info->ReplaceIfExists = source->ReplaceIfExists;
                rename_info->RootDirectory = source->RootDirectory;
                rename_info->FileNameLength = nt_name.Length;
                memcpy(rename_info->FileName, nt_name.Buffer,
                       nt_name.Length + sizeof(WCHAR));

                status = NtSetInformationFile(file, &io, rename_info, length,
                                              FileRenameInformation);
                HeapFree(GetProcessHeap(), 0, rename_info);
            }
            else
            {
                status = STATUS_NO_MEMORY;
            }

            RtlFreeUnicodeString(&nt_name);
            break;
        }

    case FileStandardInfo:
    case FileCompressionInfo:
    case FileAttributeTagInfo:
    case FileRemoteProtocolInfo:
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    return TRUE;
}
